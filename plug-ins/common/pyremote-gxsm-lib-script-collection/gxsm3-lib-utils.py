# Gxsm Python Script Library: /home/percy/.gxsm3/pyaction/gxsm3-lib-utils.py was created.


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
                print "FB: ", s&1, " Scan: ", s&(2+4), "  VP: ", s&8, "   Mov: ", s&16, " PLL: ", s&32, " **Motor=", M
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
                print "FB: ", s&1, " Scan: ", s&(2+4), "  VP: ", s&8, "   Mov: ", s&16, " PLL: ", s&32, " **Motor=", M
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
                print "FB: ", s&1, " Scan: ", s&(2+4), "  VP: ", s&8, "   Mov: ", s&16, " PLL: ", s&32, " **Motor=", M
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
                print "FB: ", s&1, " Scan: ", s&(2+4), "  VP: ", s&8, "   Mov: ", s&16, " PLL: ", s&32, " **Motor=", M
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

