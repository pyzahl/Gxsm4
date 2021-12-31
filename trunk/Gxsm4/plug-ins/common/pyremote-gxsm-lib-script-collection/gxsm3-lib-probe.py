# Gxsm Python Script Library: /home/percy/.gxsm3/pyaction/gxsm3-lib-probe.py was created.


# Gxsm Python Script Library Template
# Vector Probe Operations

#GXSM_USE_LIBRARY <gxsm3-lib-utils>
#GXSM_USE_LIBRARY <gxsm3-lib-scan>

class probe_control():

        def __init__(self):
                self.fetch_conditions ()
                self.scan = scan_control()
                self.tip = tip_control()
                
        def fetch_conditions (self):
                # fetch current basic conditions
                self.bias = gxsm.get ("dsp-fbs-bias")
                self.motor = gxsm.get ("dsp-fbs-motor")
                self.current = gxsm.get ("dsp-fbs-mx0-current-set")

                self.cp = gxsm.get ("dsp-fbs-cp")
                self.ci = gxsm.get ("dsp-fbs-ci")

                self.start_x0 = gxsm.get ("OffsetX")
                self.start_y0 = gxsm.get ("OffsetY")

        def restore_conditions (self):
                gxsm.set ("dsp-fbs-bias","%f"%(self.bias))
                gxsm.set ("dsp-fbs-mx0-current-set","%f"%(self.current))
                

        def run_vp_test_print (self, coords, num):
                for i in range(0, num):
                        sx=coords[i][0]
                        sy=coords[i][1]
        # force offset and set scan coords
                        print "ScanXY: ", sx, sy
                        gxsm.moveto_scan_xy (sx, sy)
                        self.tip.wait_scan_pos()


        def run_vp (self, coords, num, ref_bias=1.8, ref_current=0.02, ff=0, run_ref=0, ref_bias_list=[0.1]):
                x0=gxsm.get ("OffsetX")
                y0=gxsm.get ("OffsetY")
                i_ac_amp=gxsm.get ("dsp-LCK-AC-Bias-Amp")
                for i in range(0, num):
                        if run_ref > 0 and i % run_ref == 0:
                                if ff:
                                        release_Z ()
                                gxsm.set ("dsp-LCK-AC-Bias-Amp","0")
                                for bias in ref_bias_list:
                                        run_ref_image (bias)
                                gxsm.set ("dsp-LCK-AC-Bias-Amp","%f"%i_ac_bias_amp)    
                                gxsm.set ("dsp-fbs-bias","%f"%(ref_bias))
                                gxsm.set ("dsp-fbs-mx0-current-set","%f"%(ref_current))
                        if ff:
                                self.tip.goto_refpoint ()
                                freeze_Z ()             

                        sx=coords[i][0]
                        sy=coords[i][1]
                        # force offset and set scan coords
                        gxsm.set ("OffsetX","%f"%(start_x0))
                        gxsm.set ("OffsetY","%f"%(start_y0))
                        gxsm.sleep (20)
                        print "ScanXY: ", sx, sy
                        gxsm.moveto_scan_xy (sx, sy)
                        wait_scan_pos()
                        gxsm.set ("dsp-fbs-bias","%f"%(ref_bias))
                        gxsm.set ("dsp-fbs-mx0-current-set","%f"%(ref_current))
                        print "Zzzz"
                        gxsm.sleep (20)
                        print "VP Execute #", i, " of ", len2, " (", (100.*float(i)/len2), "%)"
                        gxsm.action ("dsp-fbs-VP_IV_EXECUTE")
        #	        gxsm.action ("dsp-fbs-VP_LM_EXECUTE")
        # wait until VP action has finished
                        M = self.tip.wait_for_vp ()
                        if M < -3:
                                terminate = 1
                                break

                        if ff:
                                release_Z ()    

                        if M < -0.9:
                                 self.tip.pause_tip_retract ()

        def run_IV_forall_points (self, ref_ch=0):
                i=0
                # fetch dimensions
                dims=gxsm.get_dimensions(0)
                print (dims)
                geo=gxsm.get_geometry(0)
                print (geo)
                diffs=gxsm.get_differentials(0)
                print (diffs)
                
                while True:
                        o=gxsm.get_object(ref_ch, i)
                        print o
                        gxsm.logev('Object %d:'%i + str(o))

                        i=i+1
                        if o[0] == 'Point':
                                sx = (o[1]-dims[0]/2.)*diffs[0]
                                sy = -(o[2]-dims[1]/2.)*diffs[1]
                                gxsm.logev ("IV Simple at ScanXY= %f"%sx + ", %f"%sy)
                                gxsm.moveto_scan_xy (sx, sy)
	                        gxsm.sleep (10)
                                self.tip.wait_scan_pos()
	                        gxsm.sleep (20)
                                gxsm.action ("DSP_VP_IV_EXECUTE")
                                # wait until VP action has finished
	                        gxsm.sleep (50)
                                M = self.tip.wait_for_vp ()

                        if o == 'None':
                                break

                        # safety bail out
                        if i > 100:
                                break


                                 
        def run_iv_simple (self, coords, num, ref_bias=0.1, ref_current=0.01,run_ref=0,ac_amp=0.005):
                gxsm.set ("dsp-fbs-bias","%f"%(ref_bias))
                gxsm.set ("dsp-LCK-AC-Bias-Amp","%g"%(ac_amp))
                gxsm.set ("dsp-fbs-mx0-current-set","%f"%(ref_current))
                gxsm.sleep (20)
                i_ac_bias_amp=gxsm.get ("dsp-LCK-AC-Bias-Amp")
                for i in range(0, num):
                        if run_ref > 0 and i % run_ref == 0:
                                gxsm.set ("dsp-LCK-AC-Bias-Amp","0")
                                self.scan.run_ref_image (1, 0.05)
                                gxsm.set ("dsp-LCK-AC-Bias-Amp","%f"%i_ac_bias_amp)    
                                gxsm.set ("dsp-fbs-bias","%f"%(ref_bias))
                                gxsm.set ("dsp-fbs-mx0-current-set","%f"%(ref_current))
                        sx=coords[i][0]
                        sy=coords[i][1]
        # force offset and set scan coords
                        #print "ScanXY: ", sx, sy
                        gxsm.logev ("IV Simple at ScanXY= "%sx + ", %f"%sy)
                        gxsm.moveto_scan_xy (sx, sy)
	                gxsm.sleep (20)
                        self.tip.wait_scan_pos()
                        #print "VP Execute #", i, " of ", len2, " (", (100.*float(i)/num), "%)"
	                gxsm.sleep (20)
                        gxsm.action ("DSP_VP_IV_EXECUTE")
        # wait until VP action has finished
	                gxsm.sleep (50)
                        M = self.tip.wait_for_vp ()
                        if M < -3:
                                terminate = 1
                                break
                        if M < -0.9:
                                self.tip.pause_tip_retract ()
                gxsm.set ("dsp-LCK-AC-Bias-Amp","0.0")


        def run_iv_setpointlist (self, coords, num, ref_bias=0.1, ref_current=[0.01],run_ref=0,ac_amp=0.005):
                gxsm.set ("dsp-fbs-bias","%f"%(ref_bias))
                gxsm.set ("dsp-LCK-AC-Bias-Amp","%g"%(ac_amp))
                gxsm.set ("dsp-fbs-mx0-current-set","%f"%(ref_current[0]))
                i_ac_bias_amp=gxsm.get ("dsp-LCK-AC-Bias-Amp")
                for i in range(0, num):
                        if run_ref > 0 and i % run_ref == 0:
                                gxsm.set ("dsp-LCK-AC-Bias-Amp","0")
                                self.scan.run_ref_image (1, 0.05)
                                gxsm.set ("dsp-LCK-AC-Bias-Amp","%f"%i_ac_bias_amp)    
                                gxsm.set ("dsp-fbs-bias","%f"%(ref_bias))
                                gxsm.set ("dsp-fbs-mx0-current-set","%f"%(ref_current))
                        sx=coords[i][0]
                        sy=coords[i][1]
        # force offset and set scan coords
                        print "ScanXY: ", sx, sy
                        gxsm.logev ("IV at ScanXY= "%sx + ", %f"%sy)
                        gxsm.moveto_scan_xy (sx, sy)
                        wait_scan_pos()
                        print "VP Execute #", i, " of ", len2, " (", (100.*float(i)/len2), "%)"
                        for current in ref_current:
                                gxsm.set ("dsp-fbs-mx0-current-set","%f"%current)
                                gxsm.logev ("IV Setpoint= %f"%current)
                                gxsm.sleep(10)
                                gxsm.action ("DSP_VP_IV_EXECUTE")
        # wait until VP action has finished
                                M = wait_for_vp ()
                                if M < -3:
                                        terminate = 1
                                        break
                                if M < -0.9:
                                        pause_tip_retract ()
                        if M < -3:
                                terminate = 1
                                break
                gxsm.set ("dsp-LCK-AC-Bias-Amp","0.0")


        def run_lm_simple (self, coords, num, ref_bias=0.4, ref_current=0.1,run_ref=0):
                for i in range(0, num):
                        gxsm.logev ("Mosaic Scan #%d"%i+" at Offset %f"%coords[i][0] + ", %f"%coords[i][1])
                        gxsm.moveto_scan_xy (coords[i][0],coords[i][1])
                        self.tip.wait_scan_pos()
                        gxsm.logev ("DSP_VP_LM_EXECUTE")
                        gxsm.action ("DSP_VP_LM_EXECUTE")
                        M = wait_for_vp ()
                        if M < -3:
                                terminate = 1
                                break
                        if M < -0.9:
                                self.tip.pause_tip_retract ()


        def run_gvp_simple (self, coords, num, skip=0):
                print 'num=',num
                print 'skip=',skip
                for i in range(0, num):
                        print i
                        if i<skip:
                                print 'skipping: ',coords[i][0],coords[i][1]
                                continue
                        print coords[i][0],coords[i][1]
                        gxsm.moveto_scan_xy (coords[i][0],coords[i][1])
                        self.tip.wait_scan_pos()
                        gxsm.logev ("DSP_VP_GVP_EXECUTE")
                        gxsm.action ("DSP_VP_GVP_EXECUTE")
                        M = self.tip.wait_for_vp ()
                        if M < -3:
                                terminate = 1
                                break


        def run_lm (self, coords, num, ff, ref_bias=2.35, ref_current=0.045):
                for i in range(0, num):
        #		if i % 100 == 0:
        #			if ff:
        #			        release_Z ()
        #			gxsm.set ("dsp-LCK-AC-Bias-Amp","0")    
        #			run_ref_image (0.25)
                        sx=coords[i][0]
                        sy=coords[i][1]
                        print "ScanXY: ", sx, sy
                        gxsm.moveto_scan_xy (sx, sy)
                        self.tip.wait_scan_pos()
                        print "Zzzz"
                        gxsm.sleep (40)
                        print "VP Execute #", i, " of ", len2, " (", (100.*float(i)/len2), "%)"
        #	        gxsm.action ("dsp-fbs-VP_IV_EXECUTE")
                        gxsm.action ("dsp-fbs-VP_LM_EXECUTE")
        # wait until VP action has finished
                        M = wait_for_vp ()
                        if M < -3:
                                terminate = 1
                                break

        #		if ff:
        #		        release_Z ()    

        #		if M < -0.9:
        #			pause_tip_retract ()


