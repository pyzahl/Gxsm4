# Example python action script file /home/percy/.gxsm3/pyaction/vp-gvp-initial.py was created.
# - see the Gxsm3 manual for more information
gxsm.set ("dsp-fbs-bias","0.1") # set Bias to 0.1V
gxsm.set ("dsp-fbs-mx0-current-set","0.01") # Set Current Setpoint to 0.01nA
dz=gxsm.get("dsp-gvp-dz02")
gxsm.set("dsp-gvp-dz04","%g"%(-dz))
du=gxsm.get("dsp-gvp-du03")
gxsm.set("dsp-gvp-du04","%g"%(-du))
#gxsm.set("dsp-gvp-du05","%g"%(-du))
# gxsm.sleep (2)  # sleep for 2/10s
