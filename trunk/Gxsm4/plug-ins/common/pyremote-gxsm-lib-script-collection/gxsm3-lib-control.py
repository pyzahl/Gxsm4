# Gxsm Python Script Library: /home/percy/.gxsm3/pyaction/gxsm3-lib-control.py was created.


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
		print "z0-goto: Retract to ", set_z0, " Ang ** RTQuery Offset Z0 = ", z0, "V x20"
		gxsm.logev ('Z0 GOTO: Remote Z0 Retract')
		gxsm.sleep (30)
	#       
	#       while abs(z0-set_z0) > 1.:
	#               gxsm.action ("DSP_CMD_GOTO_Z0")
	#               gxsm.sleep (200)
	#               svec=gxsm.rtquery ("o")
	#               z0 = svec[0]
	#               svec=gxsm.rtquery ("z")
	#               print "Z0=", z0, svec
		return z0

	# Coarse Z0/Offset Tools/Approach custom
	def xxz0_retract(self):
		z0_goto(-160)

	def z0_retract(self):
		gxsm.set ("dspmover-z0-speed","1000")
		svec=gxsm.rtquery ("o")  ## in HV volts
		z0 = svec[0]
		print "Z0 Retract ** Offset Z0 = ", z0
		gxsm.logev ('Remote Z0 Retract')
		while z0 > -160.:
		        gxsm.action ("DSP_CMD_GOTO_Z0")
		        gxsm.sleep (10)
		        svec=gxsm.rtquery ("o")
		        z0 = svec[0]
		        svec=gxsm.rtquery ("z")
		        print "Z0=", z0, svec

	def z_in_range_check (zcheck=-15):
		svec=gxsm.rtquery ("z") ## in HV Volts
		print "Z in range check: ZXYS=", svec
		return svec[0] > zcheck

	def z0_approach(self):
		gxsm.set ("dspmover-z0-speed","600")
		svec=gxsm.rtquery ("o") ## in HV Volts
		z0 = svec[0]
		print "Z0 approach ** Offset Z0 = ", z0
		gxsm.logev ('Remote Z0 Approach')
		while z0 < 180. and not self.z_in_range_check ():
		        gxsm.action ("DSP_CMD_AUTOCENTER_Z0")
		        gxsm.sleep (10)
		        svec=gxsm.rtquery ("o")
		        z0 = svec[0]
		        svec=gxsm.rtquery ("z")
		        print "Z0=", z0, svec
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
		        print "dF=",df
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
		print "dF=",df
		zvec=gxsm.rtquery ("z")
		print 'Zvec=', zvec
		zvec=gxsm.rtquery ("z0")
		print 'Z0vec=', zvec
		gxsm.logev("Watchdog Check dF=%gHz" %df + " zvec=%g A" %zvec[0])
		print("Watchdog Abort")


	def watch_dog_df_autoapp(self):
		df_abort = 25
		df=0.
		z0_goto (300,400)
		svec=gxsm.rtquery ("o") ## in HV Volts
		z0 = svec[0]
		print "Approach ** Offset Z0 = ", z0
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

