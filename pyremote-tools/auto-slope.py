import numpy as np
import time

# Slopes are given in  Vz /  Vxy
ZXY = (24*7.9) /  (12*46)  ## need Z Ang/Volt


print ('Please run a scan with > 400 points on a flat area.')


def auto_slope(dir='x'):

	print ('Auto slope compute', dir)
	nx = float(gxsm.get ("PointsX"))

	if nx< 400:
		print ('Please run a scan with > 400 points. Stop.')
	

	if gxsm.waitscan(False) < 1:
		print ('Please run a scan!. Stop.')
		return

		
	if dir == 'y':
		print ('Rotating to -90...')
		gxsm.set ("Rotation", "-90")
	else:
		gxsm.set ("Rotation", "0")

	dzp=0.0
	for i in range (0,6):
		y =gxsm.waitscan(False)
		yn = y+2
		while y < yn:
			y =gxsm.waitscan(False)
			if y < 0:
				print ('Please run a scan with > 400 points. Stop.')
				return
			time.sleep (1)
		ms = gxsm.get_slice(0, 0,0, y,1) # ch, v, t, yi, yn  

		med_left = np.median(ms[0,:100])
		med_right = np.median(ms[0,-100:])

		xr = float(gxsm.get ("RangeX"))
		nx = float(gxsm.get ("PointsX"))-80

		slpx = float(gxsm.get ("dsp-adv-scan-slope-"+dir))
		
		dz = slpx + (med_left -  med_right) * ZXY / xr * 2
		gxsm.set ("dsp-adv-scan-slope-"+dir, "{}".format(dz))  ## Slope in ration of volts Vz / Vxy
		if abs(dzp-dz) < 0.00005:
			print ('OK dzE=', abs(dzp-dz))
			break
		dzp=dz
		print (dir,'  Slope compute:', med_left, med_right, dz)

	gxsm.set ("Rotation", "0")


auto_slope('x')
auto_slope('y')
