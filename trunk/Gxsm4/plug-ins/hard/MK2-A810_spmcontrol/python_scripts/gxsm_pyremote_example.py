
#Since no default file / script was found, here are some
# things you can do with the python interface.
# You can also create the more extensive default/example tools collection: File->Use default.
# - see the manual for more information
# Execute to try these
c = gxsm.get("script-control")
print "Control value: ", c
if c < 50:
    print "control value below limit - aborting!"
    print "you can set the value in the window above"
    # you could set Script_Control like any other variable - see below
else:
    gxsm.set ("RangeX","200")
    gxsm.set ("RangeY","200")
    gxsm.set ("PointsX","100")
    gxsm.set ("PointsY","100")
    gxsm.set ("dsp-fbs-bias","0.1")
    gxsm.set ("dsp-fbs-mx0-current-set","1.5")
    gxsm.set ("dsp-fbs-cp", "25")
    gxsm.set ("dsp-fbs-ci", "20")
    gxsm.set ("OffsetX", "0")
    gxsm.set ("OffsetY", "0")
    gxsm.action ("DSP_CMD_AUTOAPP")
    gxsm.sleep (10)
    #gxsm.startscan ()
    gxsm.moveto_scan_xy (100.,50.)
