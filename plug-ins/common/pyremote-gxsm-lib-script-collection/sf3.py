# Example python action script file /home/percy/.gxsm3/pyaction/sf3.py was created.
# - see the Gxsm3 manual for more information
#gxsm.set ("dsp-fbs-bias","0.05") # set Bias to 0.1V
#gxsm.set ("dsp-fbs-bias","0.05") # set Bias to 0.1V
#gxsm.set ("dsp-fbs-mx0-current-set","0.05") # Set Current Setpoint to 0.01nA
#gxsm.set ("dsp-fbs-mx0-current-set","0.05") # Set Current Setpoint to 0.01nA
# gxsm.sleep (2)  # sleep for 2/10s


def init_force_map(bias=0.1, level=0.22):
	gxsm.set ("dsp-fbs-bias","%f" %bias)
	gxsm.set ("dsp-adv-scan-fast-return","5")
	gxsm.set ("dsp-fbs-scan-speed-scan","6")
	gxsm.set ("dsp-fbs-ci","6")
	gxsm.set ("dsp-fbs-cp","0")
	levelreg = level*0.90
#	gxsm.logev("AI0 Z-Pos/Setpoint = %g" %z+" (%g)"%zc)
#	gxsm.set ("dsp-adv-dsp-zpos-ref","%f" %zc )
	gxsm.set ("dsp-fbs-mx0-current-level","%f"%level)
	gxsm.set ("dsp-fbs-mx0-current-set","%f"%levelreg)
	gxsm.set ("dsp-fbs-bias","%f" %bias)

def exit_force_map(bias=0.2, current=0.02):
#	gxsm.set ("dsp-adv-dsp-zpos-ref","%f" %zi )
	gxsm.set ("dsp-adv-scan-fast-return","1")
	gxsm.set ("dsp-fbs-mx0-current-set","%f"%current)
	gxsm.set ("dsp-fbs-mx0-current-level","0.00")
	gxsm.set ("dsp-fbs-ci","50")
	gxsm.set ("dsp-fbs-cp","60")
	gxsm.set ("dsp-fbs-scan-speed-scan","250")
	gxsm.set ("dsp-fbs-bias","%f"%bias)

exit_force_map(0.2, 0.02)
exit_force_map(0.2, 0.02)
