# Gxsm Python Script Library: /home/percy/.gxsm3/pyaction/gxsm3-lib-scan.py was created.


# Gxsm Python Script Library Template
# Scan Operations

#GXSM_USE_LIBRARY <gxsm3-lib-utils>

class scan_control():

        def __init__(self):
                self.fetch_conditions ()
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

                self.initial_zi = gxsm.get ("dsp-adv-dsp-zpos-ref")
                self.initial_speed = gxsm.get ("dsp-fbs-scan-speed-scan")

                self.diffs=gxsm.get_differentials(0)

        def restore_conditions (self):
                gxsm.set ("dsp-fbs-bias","%f"%(self.bias))
                gxsm.set ("dsp-fbs-mx0-current-set","%f"%(self.current))
                

        def run_ref_image (self, u,c=0.):
                self.fetch_conditions ()
                print "Ref Image, Bias=", u
                gxsm.set ("dsp-fbs-bias","%f"%(u))
                if c > 0.:
                        gxsm.set ("dsp-fbs-mx0-current-set","%f"%(c))
                gxsm.sleep (10)
                gxsm.startscan ()
                gxsm.waitscan ()

                if self.tip.wait_for_scan () < -1:
                        self.restore_conditions ()
                        return 1

                self.restore_conditions ()
                return 0

        def run_set (self, voltages, x=0, zfreeze=False):
                if x:
                        return 1
                for u in voltages:
                        print "Bias=", u
                        if (zfreeze):
                                self.tip.freeze_Z()
                        gxsm.set ("dsp-fbs-bias","%f"%(u))
                        gxsm.set ("dsp-fbs-bias","%f"%(u))
                        gxsm.sleep (10)
                        if (zfreeze):
                                self.tip.release_Z()
                        gxsm.startscan ()
                        gxsm.waitscan ()

                        if self.tip.wait_for_scan () < -1:
                                return 1
                                break
                return 0


        def run_survey (self, coords, num, bias=0.1, current=0.01):
                for i in range(0, num):
                        ox=coords[i][0]
                        oy=coords[i][1]
                        print "OffsetXY: ", ox, oy
                        gxsm.set ("OffsetY","%f"%(oy))
                        self.tip.wait_move_pos()
                        gxsm.set ("OffsetX","%f"%(ox))
                        self.tip.wait_move_pos()
                        self.scan.run_ref_image (bias, current)

        def run_mosaic (self, coords, xy00=[0,0], repo=1, repi=1):
                for i in range(0,repo):
                        for xy0 in coords:
                                ox=xy0[0]+xy00[0]
                                oy=xy0[1]+xy00[1]
                                print "OffsetXY: ", ox, oy
                                gxsm.set ("OffsetY","%f"%(oy))
                                self.tip.wait_move_pos()
                                gxsm.set ("OffsetX","%f"%(ox))
                                for i in range(0,repi):
                                        gxsm.logev ("Mosaic Scan #%d"%i+" at Offset %f"%ox+", %f"%oy)
                                        self.tip.wait_move_pos()
                                        gxsm.sleep (20)
                                        gxsm.startscan ()
                                        gxsm.waitscan ()


        def chdfrun (self, x,y):
                self.tip.release_Z (10)
                self.tip.goto_xy (0,0, 50)
                freeze_Z ()
                gxsm.set ("SPMC_SLS_Xs", "%d"%x)
                gxsm.set ("SPMC_SLS_Ys", "%d"%y)
                gxsm.startscan ()
                gxsm.waitscan ()

        def chdcircle (self, m=10,n=400):
                xn=m
                yn=m
                gxsm.set ("SPMC_SLS_Xn", "%d"%xn)
                gxsm.set ("SPMC_SLS_Yn", "%d"%(yn+2))
                rv=[[1,0],[0,-1],[0,-1],[-1,0],[-1,0],[0,1],[0,1],[1,0]]

                chdfrun (n/2,n/2-yn)
                for r in range (0,m):
                        x = n/2
                        y = n/2 + yn*r
                        for i in range (0,m-2):
                                for j in range (0,r+1):
                                        chdfrun (x,y)
                                        x = x + xn*rv[i][0]
                                        y = y + yn*rv[i][1]


        def forall_rectangles_subscan (self, ref_ch=0):
                i=0
                while True:
                        o=gxsm.get_object(ref_ch, i)
                        print o
                        gxsm.logev('Object %d:'%i + str(o))

                        i=i+1
                        if o[0] == 'Rectangle':
                                xs = min (o[1], o[3])
                                xn = abs (o[1] - o[3])
                                ys = min (o[2], o[4])
                                yn = abs (o[2] - o[4])
                                gxsm.set ("SPMC_SLS_Xs", "%d"%xs)
                                gxsm.set ("SPMC_SLS_Ys", "%d"%ys)
                                gxsm.set ("SPMC_SLS_Xn", "%d"%xn)
                                gxsm.set ("SPMC_SLS_Yn", "%d"%yn)
                                gxsm.startscan ()
                                gxsm.waitscan ()

                        if o == 'None':
                                break

                        # safety bail out
                        if i > 100:
                                break

        def get_zref_at_marker(self):
                gxsm.logev("AI0 Get Ref Z")
                gxsm.set ("dsp-fbs-bias","0.1")
                gxsm.set ("dsp-fbs-mx0-current-set","0.010")
                gxsm.set ("dsp-fbs-mx0-current-level","0.0")
                gxsm.sleep(50)

                zra=0
                for n in range (0,10):
                        zvec=gxsm.rtquery ("z")
                        gxsm.sleep(10)
                        zr=zvec[0]/10.404*32768*self.diffs[2]   # convert Z Volts to Angstroems via DIG
                        zra = zra+zr

                zr = zra/10.0
                print "Z=", zr
                gxsm.logev("AI0 Ref Z = %g"%zr)
                gxsm.set ("dsp-fbs-mx0-current-set","0.010")
                return zr

        #zr=get_zref_at_marker()
        #zr=7.0
        # Force Map Procedure -- assuming to use CZ-FUZZY-LOG feedback mixer ch0 transfer
        #zi=zr  ###3.0  # initial z -- typically the Z "on top" of the object of interest at 10pA and 100mV, and for sure a leveled in scan 

        def run_force_map(self, zi,zdown=-2.0,bias=0.1,dz=-0.1, level=0.22, allrectss=False, speed=6, ffr=5):
                gxsm.set ("dsp-fbs-bias","%f" %bias)
                gxsm.set ("dsp-adv-scan-fast-return","%g"%ffr)
                gxsm.set ("dsp-fbs-scan-speed-scan","%g"%speed)
                gxsm.set ("dsp-fbs-ci","5")
                gxsm.set ("dsp-fbs-cp","0")
                levelreg = level*0.98

                if zdown == 0.0:
                        z=0.0
                        zr = zi
                        zc = zr+z
                        gxsm.logev("AI0 Z-Pos/Setpoint = %g" %z+" (%g)"%zc)
                        gxsm.set ("dsp-adv-dsp-zpos-ref","%f" %zc )
                        gxsm.set ("dsp-fbs-mx0-current-level","%f"%level)
                        gxsm.set ("dsp-fbs-mx0-current-set","%f"%levelreg)
                        gxsm.set ("dsp-fbs-bias","%f" %bias)
                        if allrectss:
                                self.forall_rectangles_subscan ()
                        else:
                                gxsm.startscan ()
                                gxsm.waitscan ()
                else:
                        for z in np.arange(0, zdown, dz):
                        #	zr = get_zref_at_marker()
                                zr = zi
                                zc = zr+z
                                gxsm.logev("AI0 Z-Pos/Setpoint = %g" %z+" (%g)"%zc)
                                gxsm.set ("dsp-adv-dsp-zpos-ref","%f" %zc )
                                gxsm.set ("dsp-fbs-mx0-current-level","%f"%level)
                                gxsm.set ("dsp-fbs-mx0-current-set","%f"%levelreg)
                                gxsm.set ("dsp-fbs-bias","%f" %bias)
                                if allrectss:
                                        self.forall_rectangles_subscan ()
                                else:
                                        gxsm.startscan ()
                                        gxsm.waitscan ()

                # back to normal
                gxsm.set ("dsp-adv-dsp-zpos-ref","%f" %zi )
                gxsm.set ("dsp-adv-scan-fast-return","1")
                gxsm.set ("dsp-fbs-mx0-current-set","0.02")
                gxsm.set ("dsp-fbs-mx0-current-level","0.00")
                gxsm.set ("dsp-fbs-ci","50")
                gxsm.set ("dsp-fbs-cp","55")
                gxsm.set ("dsp-fbs-scan-speed-scan","50")


