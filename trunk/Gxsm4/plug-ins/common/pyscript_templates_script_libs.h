/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 8 c-style: "K&R" -*- */

/* Gnome gxsm - Gnome X Scanning Microscopy
 * universal STM/AFM/SARLS/SPALEED/... controlling and
 * data analysis software
 *
 * Gxsm Plugin Name: pyremote.C
 * ========================================
 *
 * Copyright (C) 1999 The Free Software Foundation
 *
 * Authors: Percy Zahl <zahl@fkp.uni-hannover.de>
 * additional features: Andreas Klust <klust@fkp.uni-hannover.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 */

const gchar *template_library_demo = R"V0G0N(
# Gxsm Python Script Library Demo and Test

# GXSM_USE_LIBRARY <gxsm4-lib-scan>

# Gxsm control sctipting  test suit

gxsm.set("RangeX","500")
gxsm.set("PointsX","400")
gxsm.startscan()
gxsm.moveto_scan_xy (123, 234)


tip = tip_control()
scan = scan_control()
tip.set_refpoint_current ()
tip.goto_xy (100, 200,1)
gxsm.sleep(1)
tip.goto_refpoint()

xylist = tip.make_grid_coords(20, 1, 30, 1, 2, [[0,0], [10,20]])

#terminate = run_set (np.random.shuffle([2.0, 1.0, 0.5, 0.4, 0.2, 0.1, 0.05, -0.1, -0.2, -0.5, 1.0, 1.5, -1.0, -1.5, 2.0, -2.0, 2.5, 0.1, 0.1])
#terminate = run_set (np.arange(0.1, 1.0, 0.1 ))

#run_mosaic ([[0,0],[1100,0],[2200,0],[0,1100],[1100,1100],[2200,1100],[0,2200],[1100,2200],[2200,2200]],[-1100,-1100])
#run_mosaic (make_coords (600,7), [-600*3,-600*3])

#scan.run_force_map(0.2,-2.6,0.02,-0.1,0.35, False)
#gxsm.logev("ForceMap complete, starting ref scan, SS off.")
#scan.forall_rectangles_subscan ()

#autoapproach_via_z0()

#run_iv_setpointlist (xy1, len1, 0.2, [0.1, 0.2, 0.3], 0,0.05)
#run_gvp_simple (xy1, len1, skip)

#run_iv_simple (xy, len2, 0.4, 0.050, 51)
#run_lm_simple (xy, len2, 0.05, 0.02, 10)

#run_survey (xy, len2, 0.1, 0.01)

#tip.release_Z ()	


)V0G0N";

const gchar *template_library_utils = R"V0G0N(
# Gxsm Python Script Library Template
# Generic Utilities


# default imports
import array
import math
import numpy as np
from numpy.random import shuffle
import os

# basic tip control class 
class tip_control():

        def __init__(self):
                self.fetch_conditions ()
                
        def fetch_conditions (self):
                # fetch current basic conditions
                self.bias = gxsm.get ("dsp-fbs-bias")
                self.motor = gxsm.get ("dsp-fbs-motor")
                self.current = gxsm.get ("dsp-fbs-mx0-current-set")

                self.cp = gxsm.get ("dsp-fbs-cp")
                self.ci = gxsm.get ("dsp-fbs-ci")

                self.start_x0 = gxsm.get ("OffsetX")
                self.start_y0 = gxsm.get ("OffsetY")

        def set_refpoint_current (self):
                # Ref Tip Scan Pos Point from last tip pos (at script start)
                self.s_x0 = gxsm.get ("ScanX")
                self.s_y0 = gxsm.get ("ScanY")

        def set_refpoint_xy (self, x,y):
                # Ref Tip Scan Pos Point from last tip pos (at script start)
                self.s_x0 = x
                self.s_y0 = y


        # wait for VP finished and check/return Motor value/abort cond.
        def wait_for_vp (self):
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
                print ("FB: ", s&1, " Scan: ", s&(2+4), "  VP: ", s&8, "   Mov: ", s&16, " PLL: ", s&32, " **Motor=", M)
                gxsm.sleep(20)
                return M

        def wait_scan_pos(self):
                scaction = 1
                M = 0
                while scaction > 0 and M > -2.:
                        gxsm.sleep (10) # sleep 10/10 sec
                        M=gxsm.get ("dsp-fbs-motor")
                        svec=gxsm.rtquery ("s")
                        s = int(svec[0])
                        scaction = s&(2+4)
                        if os.path.exists("remote.py-stop"):
                                M = -10.
                                break
                print ("FB: ", s&1, " Scan: ", s&(2+4), "  VP: ", s&8, "   Mov: ", s&16, " PLL: ", s&32, " **Motor=", M)
                gxsm.sleep(20)
                return M

        def wait_move_pos(self):
                scaction = 1
                M = 0
                while scaction > 0 and M > -2.:
                        gxsm.sleep (10) # sleep 10/10 sec
                        M=gxsm.get ("dsp-fbs-motor")
                        svec=gxsm.rtquery ("s")
                        s = int(svec[0])
                        scaction = s&16
                        if os.path.exists("remote.py-stop"):
                                M = -10.
                                break
                print ("FB: ", s&1, " Scan: ", s&(2+4), "  VP: ", s&8, "   Mov: ", s&16, " PLL: ", s&32, " **Motor=", M)
                gxsm.sleep(20)
                return M

        # wait for scan finish or abort
        def wait_for_scan (self):
                scaction = 1
                M = 0.
                while scaction > 0 and M > -2.:
                        gxsm.sleep (10) # sleep 10/10 sec
                        M=gxsm.get ("dsp-fbs-motor")
                        svec=gxsm.rtquery ("s")
                        s = int(svec[0])
                        scaction = s&(2+4)        
                        if os.path.exists("remote.py-stop"):
                                M = -10.
                                breaksp
                print ("FB: ", s&1, " Scan: ", s&(2+4), "  VP: ", s&8, "   Mov: ", s&16, " PLL: ", s&32, " **Motor=", M)
                gxsm.sleep(20)
                return M


        def goto_refpoint(self, sleep=40):
                gxsm.moveto_scan_xy (self.s_x0, self.s_y0)
                self.wait_scan_pos()
                gxsm.sleep (sleep)

        def goto_xy(self, sx, sy, sleep=40):
                gxsm.moveto_scan_xy (sx, sy)
                self.wait_scan_pos()
                gxsm.sleep (sleep)


        def freeze_Z(self):
                self.fetch_conditions ()
                gxsm.set ("dsp-fbs-cp","0")
                gxsm.set ("dsp-fbs-ci","0")


        def const_height_slow_Z(self, cich=0.25):
                gxsm.set ("dsp-fbs-cp","0")
                gxsm.set ("dsp-fbs-ci","%f"%(cich))

        def release_Z(self, s=40):
                gxsm.set ("dsp-fbs-cp","%f"%(self.cp))
                gxsm.set ("dsp-fbs-ci","%f"%(self.ci))
                gxsm.sleep (s)

        def pause_tip_retract(self):
                M = -1;
                self.current=gxsm.get ("dsp-fbs-mx0-current-set")
                gxsm.set ("dsp-fbs-mx0-current-set","0")
                while M < -0.9:
                        gxsm.sleep (10) # sleep 10/10 sec
                        M=gxsm.get ("dsp-fbs-motor")
                gxsm.set ("dsp-fbs-mx0-current-set","%f"%(self.current/2.))
                gxsm.sleep (600) # sleep 600/10 sec
                gxsm.set ("dsp-fbs-mx0-current-set","%f"%(self.current))
                gxsm.sleep (100) # sleep 100/10 sec

        def make_grid_coords(self, spnx, stepx, spny, stepy, positions=1, position=[[0,0]], shuffle=False):
                sxlist = np.arange (-spnx, spnx+stepx, stepx)
                sylist = np.arange (-spny, spny+stepy, stepy)

                #position = [[-8.6,-8.1]             ]#, [20.0,80.0]]
                #positions = 1

                len1 = np.size(sxlist) * np.size(sylist)
                len2 = np.size(sxlist) * np.size(sylist) * positions
                xy = np.arange(2.0*len2).reshape(len2,2)
                xy1 = np.arange(2.0*len1).reshape(len1,2)

                i=0
                for r in position:
                        i1=0
                        for y in sylist:
                                for x in sxlist:
                                        xy[i][0] = x + r[0]
                                        xy[i][1] = y + r[1]
                                        xy1[i1][0] = x + r[0]
                                        xy1[i1][1] = y + r[1]
                                        i=i+1           
                                        i1=i1+1
                if shuffle:
                        np.random.shuffle(xy1)

                return xy1

        def make_coords(self, w, n):
                c=[]
                for i in range (0,n):
                        for j in range (0,n):
                                c.append ([i*w, j*w])
                return c

)V0G0N";


const gchar *template_library_control = R"V0G0N(
# Gxsm Python Script Library Template
# Instrument Control Operations

# default imports
import array
import math
import numpy as np
from numpy.random import shuffle
import os

# basic tip autoapp control class 
class approach_control():

        def __init__(self):
                self.fetch_conditions ()
                
        def fetch_conditions (self):
                # fetch current basic conditions
                self.bias = gxsm.get ("dsp-fbs-bias")
                self.motor = gxsm.get ("dsp-fbs-motor")
                self.current = gxsm.get ("dsp-fbs-mx0-current-set")
                self.cp = gxsm.get ("dsp-fbs-cp")
                self.ci = gxsm.get ("dsp-fbs-ci")

                self.z0goto = gxsm.get ("dspmover-z0-goto")
                self.z0speed = gxsm.get ("dspmover-z0-speed")

        def restore_conditions (self):
                gxsm.set ("dspmover-z0-goto", "%g"%self.z0goto)
                gxsm.set ("dspmover-z0-speed", "%g"%self.z0speed)
                

        # Watch dog script. Watching via RTQuery system parameters:
        # for example dF and if abs(dF) > limit DSP_CMD_STOPALL is issued (cancel auto approch, etc.)

        def z0_goto(self, set_z0=0, speed=400):
                gxsm.set ("dspmover-z0-goto","%g"%set_z0)
                gxsm.set ("dspmover-z0-speed","%g"%speed)
                gxsm.action ("DSP_CMD_GOTO_Z0")
                svec=gxsm.rtquery ("o")  ## in HV volts
                z0 = svec[0]
                print ("z0-goto: Retract to ", set_z0, " Ang ** RTQuery Offset Z0 = ", z0, "V x20")
                gxsm.logev ('Z0 GOTO: Remote Z0 Retract')
                gxsm.sleep (30)
        #       
        #       while abs(z0-set_z0) > 1.:
        #               gxsm.action ("DSP_CMD_GOTO_Z0")
        #               gxsm.sleep (200)
        #               svec=gxsm.rtquery ("o")
        #               z0 = svec[0]
        #               svec=gxsm.rtquery ("z")
        #               print ("Z0=", z0, svec)
                return z0

        # Coarse Z0/Offset Tools/Approach custom
        def xxz0_retract(self):
                z0_goto(-160)

        def z0_retract(self):
                gxsm.set ("dspmover-z0-speed","1000")
                svec=gxsm.rtquery ("o")  ## in HV volts
                z0 = svec[0]
                print ("Z0 Retract ** Offset Z0 = ", z0)
                gxsm.logev ('Remote Z0 Retract')
                while z0 > -160.:
                        gxsm.action ("DSP_CMD_GOTO_Z0")
                        gxsm.sleep (10)
                        svec=gxsm.rtquery ("o")
                        z0 = svec[0]
                        svec=gxsm.rtquery ("z")
                        print ("Z0=", z0, svec)

        def z_in_range_check (zcheck=-15):
                svec=gxsm.rtquery ("z") ## in HV Volts
                print ("Z in range check: ZXYS=", svec)
                return svec[0] > zcheck

        def z0_approach(self):
                gxsm.set ("dspmover-z0-speed","600")
                svec=gxsm.rtquery ("o") ## in HV Volts
                z0 = svec[0]
                print ("Z0 approach ** Offset Z0 = ", z0)
                gxsm.logev ('Remote Z0 Approach')
                while z0 < 180. and not self.z_in_range_check ():
                        gxsm.action ("DSP_CMD_AUTOCENTER_Z0")
                        gxsm.sleep (10)
                        svec=gxsm.rtquery ("o")
                        z0 = svec[0]
                        svec=gxsm.rtquery ("z")
                        print ("Z0=", z0, svec)
                        sc = gxsm.get ("script-control")
                        if sc < 1:
                                break
                return svec[0]

        def autoapproach_via_z0(self):
                gxsm.set ("dspmover-z0-goto","-1600")
                count=0
                gxsm.logev ('Remote Auto Approach start')
                df_abort = 25;
                #df = watch_dog_run (df_abort)
                df = 0
                while not self.z_in_range_check()  and abs(df) < df_abort:
                        z0_retract ()
                        gxsm.sleep (20)
                        gxsm.logev ('Remote Auto Approach Z0: XP-Auto Steps')
                        gxsm.action ("DSP_CMD_MOV-ZP_Auto")
                        gxsm.sleep (20)
                        zs=z0_approach ()
                        #df = watch_dog_run (df_abort)
                        count=count+1
                        gxsm.logev ('Remote Auto Approach #' + str(count) + ' ZS=' + str (zs))
                        sc = gxsm.get ("script-control")
                        if sc < 1:
                                break
                        
                gxsm.set ("dspmover-z0-speed","400")
                gxsm.logev('Remote Auto Approach via Z0 completed')


        def watch_dog_run(self, limit=25):
                gxsm.logev("Watchdog run for DSP AutoApp in current. Watching: dF=%gHz"%limit)
                gxsm.action ("DSP_CMD_AUTOAPP")
                df=0.
                while abs(df) < limit: 
                        gxsm.sleep (10)
                        fvec=gxsm.rtquery ("f")
                        df = fvec[0]
                        print ("dF=",df)
                        gxsm.logev("Watchdog dF=%gHz"%df)
                        sc = gxsm.get ("script-control")
                        if sc < 1:
                                break

                gxsm.action ("DSP_CMD_STOPALL")
                gxsm.logev("Watchdog DSP_CMD_STOPALL ACTION as of dF=%gHz"%df)
                return df

        def test_read ():
                gxsm.sleep (200)
                fvec=gxsm.rtquery ("f")
                df = fvec[0]
                print ("dF=",df)
                zvec=gxsm.rtquery ("z")
                print ('Zvec=', zvec)
                zvec=gxsm.rtquery ("z0")
                print ('Z0vec=', zvec)
                gxsm.logev("Watchdog Check dF=%gHz" %df + " zvec=%g A" %zvec[0])
                print ("Watchdog Abort")


        def watch_dog_df_autoapp(self):
                df_abort = 25
                df=0.
                z0_goto (300,400)
                svec=gxsm.rtquery ("o") ## in HV Volts
                z0 = svec[0]
                print ("Approach ** Offset Z0 = ", z0)
                gxsm.logev ('Remote Z0 Approach')
                while z0 < 180. and not self.z_in_range_check (-8) and abs(df) < df_abort:
                        df = self.watch_dog_run (df_abort)
                        gxsm.sleep (200)
                        sc = gxsm.get ("script-control")
                        if sc < 1:
                                break

                return svec[0]

# FOR XYZ GAINS 10 X 2
#autoapproach_via_z0()
#watch_dog_df_autoapp()

## FINISH DEFAULTS
#print ("Finished/Aborted.")
#gxsm.set ("dspmover-z0-goto","-1600")
#gxsm.set ("dspmover-z0-speed","400")

)V0G0N";


const gchar *template_library_scan = R"V0G0N(
# Gxsm Python Script Library Template
# Scan Operations

#GXSM_USE_LIBRARY <gxsm4-lib-utils>

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
                print ("Ref Image, Bias=", u)
                gxsm.set ("dsp-fbs-bias","%f"%(u))
                if c > 0.:
                        gxsm.set ("dsp-fbs-mx0-current-set","%f"%(c))
                gxsm.sleep (10)
                gxsm.startscan ()
                #                gxsm.waitscan ()
                while gxsm.waitscan() >= 0:
	                gxsm.sleep (50)

                if self.tip.wait_for_scan () < -1:
                        self.restore_conditions ()
                        return 1

                self.restore_conditions ()
                return 0

        def run_set (self, voltages, x=0, zfreeze=False):
                if x:
                        return 1
                for u in voltages:
                        print ("Bias=", u)
                        if (zfreeze):
                                self.tip.freeze_Z()
                        gxsm.set ("dsp-fbs-bias","%f"%(u))
                        gxsm.set ("dsp-fbs-bias","%f"%(u))
                        gxsm.sleep (10)
                        if (zfreeze):
                                self.tip.release_Z()
                        gxsm.startscan ()
                        gxsm.sleep(50)
                        while gxsm.waitscan () >= 0:
                            gxsm.sleep(50)
                        if self.tip.wait_for_scan () < -1:
                                return 1
                                break
                return 0


        def run_survey (self, coords, num, bias=0.1, current=0.01):
                for i in range(0, num):
                        ox=coords[i][0]
                        oy=coords[i][1]
                        print ("OffsetXY: ", ox, oy)
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
                                print ("OffsetXY: ", ox, oy)
                                gxsm.set ("OffsetY","%f"%(oy))
                                self.tip.wait_move_pos()
                                gxsm.set ("OffsetX","%f"%(ox))
                                for i in range(0,repi):
                                        gxsm.logev ("Mosaic Scan #%d"%i+" at Offset %f"%ox+", %f"%oy)
                                        self.tip.wait_move_pos()
                                        gxsm.sleep (20)
                                        gxsm.startscan ()
                                        gxsm.sleep (50)
                                        while gxsm.waitscan() >= 0:
	                                        gxsm.sleep (50)


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
                        print (o)
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
                print ("Z=", zr)
                gxsm.logev("AI0 Ref Z = %g"%zr)
                gxsm.set ("dsp-fbs-mx0-current-set","0.010")
                return zr

        #zr=get_zref_at_marker()
        #zr=7.0
        # Force Map Procedure -- assuming to use CZ-FUZZY-LOG feedback mixer ch0 transfer
        #zi=zr  ###3.0  # initial z -- typically the Z "on top" of the object of interest at 10pA and 100mV, and for sure a leveled in scan 

        def run_force_map(self, zi,zdown=-2.0,bias=0.1,dz=-0.1, level=0.22, allrectss=False):
                gxsm.set ("dsp-fbs-bias","%f" %bias)
                gxsm.set ("dsp-adv-scan-fast-return","5")
                gxsm.set ("dsp-fbs-scan-speed-scan","6")
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
                        #        zr = get_zref_at_marker()
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

)V0G0N";


const gchar *template_library_probe = R"V0G0N(
# Gxsm Python Script Library Template
# Vector Probe Operations

#GXSM_USE_LIBRARY <gxsm4-lib-utils>
#GXSM_USE_LIBRARY <gxsm4-lib-scan>

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
                        print ("ScanXY: ", sx, sy)
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
                        print ("ScanXY: ", sx, sy)
                        gxsm.moveto_scan_xy (sx, sy)
                        wait_scan_pos()
                        gxsm.set ("dsp-fbs-bias","%f"%(ref_bias))
                        gxsm.set ("dsp-fbs-mx0-current-set","%f"%(ref_current))
                        print ("Zzzz")
                        gxsm.sleep (20)
                        print ("VP Execute #", i, " of ", len2, " (", (100.*float(i)/len2), "%)")
                        gxsm.action ("dsp-fbs-VP_IV_EXECUTE")
        #                gxsm.action ("dsp-fbs-VP_LM_EXECUTE")
        # wait until VP action has finished
                        M = self.tip.wait_for_vp ()
                        if M < -3:
                                terminate = 1
                                break

                        if ff:
                                release_Z ()    

                        if M < -0.9:
                                 self.tip.pause_tip_retract ()

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
                        print ("ScanXY: ", sx, sy)
                        gxsm.logev ("IV Simple at ScanXY= %f"%sx + ", %f"%sy)
                        gxsm.moveto_scan_xy (sx, sy)
                        gxsm.sleep (20)
                        self.tip.wait_scan_pos()
                        #print ("VP Execute #", i, " of ", len2, " (", (100.*float(i)/num), "%)")
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
                        print ("ScanXY: ", sx, sy)
                        gxsm.logev ("IV at ScanXY= "%sx + ", %f"%sy)
                        gxsm.moveto_scan_xy (sx, sy)
                        wait_scan_pos()
                        print ("VP Execute #", i, " of ", len2, " (", (100.*float(i)/len2), "%)")
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
                print ('num=',num)
                print ('skip=',skip)
                for i in range(0, num):
                        print (i)
                        if i<skip:
                                print ('skipping: ',coords[i][0],coords[i][1])
                                continue
                        print (coords[i][0],coords[i][1])
                        gxsm.moveto_scan_xy (coords[i][0],coords[i][1])
                        wait_scan_pos()
                        gxsm.logev ("DSP_VP_GVP_EXECUTE")
                        gxsm.action ("DSP_VP_GVP_EXECUTE")
                        M = self.tip.wait_for_vp ()
                        if M < -3:
                                terminate = 1
                                break


        def run_lm (self, coords, num, ff, ref_bias=2.35, ref_current=0.045):
                for i in range(0, num):
        #                if i % 100 == 0:
        #                        if ff:
        #                                release_Z ()
        #                        gxsm.set ("dsp-LCK-AC-Bias-Amp","0")    
        #                        run_ref_image (0.25)
                        sx=coords[i][0]
                        sy=coords[i][1]
                        print ("ScanXY: ", sx, sy)
                        gxsm.moveto_scan_xy (sx, sy)
                        self.tip.wait_scan_pos()
                        print ("Zzzz")
                        gxsm.sleep (40)
                        print ("VP Execute #", i, " of ", len2, " (", (100.*float(i)/len2), "%)")
        #                gxsm.action ("dsp-fbs-VP_IV_EXECUTE")
                        gxsm.action ("dsp-fbs-VP_LM_EXECUTE")
        # wait until VP action has finished
                        M = wait_for_vp ()
                        if M < -3:
                                terminate = 1
                                break

        #                if ff:
        #                        release_Z ()    

        #                if M < -0.9:
        #                        pause_tip_retract ()


)V0G0N";


const gchar *template_library_analysis = R"V0G0N(
# Gxsm Python Script Library Template
# Analysis functions

# fetch dimensions
def get_dimensions(ch):
    dims=gxsm.get_dimensions(ch)
    geo=gxsm.get_geometry(ch)
    diffs_f=gxsm.get_differentials(ch)
    return dims, geo, diffs

)V0G0N";


/*
const gchar *template_name = R"V0G0N(
...py script ...
)V0G0N";
*/
