######## Before you run change file path. If not enough molecules found Lower threshold

######## NOTES TO WORK ON ########


#Tip goes to the center of the brightest spot and makes a box around it. 
#The boxes have to be away from the edges!!! Or else they behave poorly. and give a weird rectangle shape. Anything around the edges does not image the molecule!! 

##################  Imports ############################


import time
import random as rng
import cv2
#import netCDF4 as nc
import struct
import array
import math
import numpy as np
from skimage.color import rgb2gray
import itertools
import matplotlib.pyplot as plt

###############################################
## INSTRUMENT
ZAV=8.0 # 8 Ang/Volt in V

### SETUP
map_ch=6   # MAP IMAGE CHANNEL  (0....N)
map_diffs=gxsm.get_differentials(map_ch)

HR_AFM_range = 45
HR_AFM_points = 330

do_auto_locate = False
do_stm = True
do_afm = True


gxsm.set('script-control','1')


def init_force_map_ref_xy(bias=0.02, level=0.111, ref_i=0.05, zoff=0.0, xy_list=[[0,0]]):
	print("Measuring Z at ref")
	# set ref condition
	gxsm.set ("dsp-fbs-bias","0.1") # set Bias to 0.1V
	gxsm.set ("dsp-fbs-mx0-current-set","{:8.4f}".format( ref_i))   # Set Current Setpoint to reference value (nA)
	gxsm.set ("dsp-fbs-mx0-current-level","0.00")

	time.sleep(1) # NOW SHOULD ME ON TOP OF MOLECULE
	gxsm.set ("dsp-fbs-bias","0.02") # set Bias to 20mV
	gxsm.set ("dsp-fbs-mx0-current-set","0.05") # Set Current Setpoint to 50pA
	# read Z ref and set
	svec=gxsm.rtquery ("z")
	print('RTQ z',  svec[0]*ZAV)
	pts=1
	z=svec[0]*ZAV
	zmin=zmax=z
	for r in xy_list:
		gxsm.moveto_scan_xy(r[0], r[1])
		time.sleep(0.1)
		for i in range(0,5):
			svec=gxsm.rtquery ("z")
			time.sleep(0.02)
			zxy=svec[0]*ZAV
			if zmin > zxy:
				zmin=zxy
			if zmax < zxy:
				zmax=zxy
			print(r, " => Z: ", zxy, " Min/Max: ", zmin, zmax)
			z=z+zxy
			pts=pts+1
	z=z/pts + zoff  # zoff=0 for auto
	time.sleep(1) # NOW SHOULD ME ON TOP OF MOLECULE

	print("Setting Z-Pos/Setpoint = {:8.2f} A".format( z))
	gxsm.set ("dsp-adv-dsp-zpos-ref", "{:8.2f}".format( z))
	gxsm.set ("dsp-fbs-bias","%f" %bias)
	gxsm.set ("dsp-adv-scan-fast-return","5")
	gxsm.set ("dsp-fbs-scan-speed-scan","8")
	gxsm.set ("dsp-fbs-ci","3")
	gxsm.set ("dsp-fbs-cp","0")
	levelreg = level*0.99
	gxsm.set ("dsp-fbs-mx0-current-level","%f"%level)
	gxsm.set ("dsp-fbs-mx0-current-set","%f"%levelreg)
	gxsm.set ("dsp-fbs-bias","%f" %bias)
	return z

def exit_force_map(bias=0.2, current=0.02):
	gxsm.set ("dsp-adv-scan-fast-return","1")
	gxsm.set ("dsp-fbs-mx0-current-set","%f"%current)
	gxsm.set ("dsp-fbs-mx0-current-level","0.00")
	gxsm.set ("dsp-fbs-ci","35")
	gxsm.set ("dsp-fbs-cp","40")
	gxsm.set ("dsp-fbs-scan-speed-scan","250")
	gxsm.set ("dsp-fbs-bias","%f" %bias)
	
def process(input_list, threshold=20):
    combos = itertools.combinations(input_list, 2)
    points_to_remove = [point2 for point1, point2 in combos if math.dist(point1, point2)<=threshold]
    points_to_keep = [point for point in input_list if point not in points_to_remove]
    return points_to_keep


def get_gxsm_img_pkt(ch):
	# fetch dimensions
	dims=gxsm.get_dimensions(ch)
	print (dims)
	geo=gxsm.get_geometry(ch)
	print (geo)
	diffs=gxsm.get_differentials(ch)
	print (diffs)
	m = np.zeros((dims[1],dims[0]), dtype=float)
	for y in range (0,dims[1]):
		for x in range (0, dims[0]):
			v=0
			m[y][x]=gxsm.get_data_pkt (ch, x, y, v, 0)*diffs[2]  # Z value in Ang now
			#gxsm.put_data_pkt (m[y][x]/diffs[2], ch+1, x, y, v, 0)  # Z value in Ang now
	return m

def get_gxsm_img(ch):
	dims=gxsm.get_dimensions(ch)
	return gxsm.get_slice(ch, 0,0, 0,dims[1]) # ch, v, t, yi, yn

def get_gxsm_img_cm_test(ch, level=0.5):
	# fetch dimensions
	dims=gxsm.get_dimensions(ch)
	print (dims)
	geo=gxsm.get_geometry(ch)
	print (geo)
	diffs=gxsm.get_differentials(ch)
	print (diffs)
	m = get_gxsm_img(ch)

	cmx = 0
	cmy = 0
	csum = 0
	cmed = np.median(m)
	cx=[]
	cy=[]
	print ('Z base: ', cmed)
	b=2
	for y in range (b,dims[1]-b):
		for x in range (b, dims[0]-b):
			v=0
			m[y][x]=m[y][x] - cmed # Z value in Ang now
			if m[y][x] > level:
				cmx = cmx+x*m[y][x]
				cmy = cmy+y*m[y][x]
				csum = csum + m[y][x]
				cx.append(x)
				cy.append(y)
				gxsm.put_data_pkt(m[y][x]/diffs[2],ch, x,y,v,0)
			else:
				gxsm.put_data_pkt(0,ch, x,y,v,0)
	if csum > 0:
		cmx = cmx/csum
		cmy = cmy/csum
	else:
		cmx = dims[0]/2
		cmy = dims[1]/2
	gxsm.add_marker_object(ch, 'PointCM',1, int(round(cmx)), int(round(cmy)), 1.0)
	return m, cmx, cmy, cx, cy


def get_gxsm_img_cm(ch, level=0.5):
	# fetch dimensions
	dims=gxsm.get_dimensions(ch)
	print (dims)
	geo=gxsm.get_geometry(ch)
	print (geo)
	diffs=gxsm.get_differentials(ch)
	print (diffs)
	m = get_gxsm_img(ch)

	cmx = 0
	cmy = 0
	csum = 0
	cmed = np.median(m)
	print ('Z base: ', cmed)
	b=2
	cx=[]
	cy=[]
	for y in range (b,dims[1]-b):
		for x in range (b, dims[0]-b):
			v=0
			m[y][x]=m[y][x] - cmed # Z value in Ang now
			if m[y][x] > level:
				cmx = cmx+x*m[y][x]
				cmy = cmy+y*m[y][x]
				csum = csum + m[y][x]
				cx.append(x)
				cy.append(y)
	if csum > 0:
		cmx = cmx/csum
		cmy = cmy/csum
	else:
		cmx = dims[0]/2
		cmy = dims[1]/2
	gxsm.add_marker_object(ch, 'PointCM',1, int(round(cmx)), int(round(cmy)), 1.0)
	return m, cmx, cmy, cx, cy


def find_bbox(cx, cy):
	dims=gxsm.get_dimensions(0)

	xl = np.amin(np.array(cx))
	xr = np.amax(np.array(cx))
	yt = np.amin(np.array(cy))
	yb = np.amax(np.array(cy))

	print (xl,', ', xr, ' : ', yt,', ', yb, '  N: ', xr-xl, ', ', yb - yt)

	b=25
	gxsm.set('SPMC_SLS_Xs','{}'.format(max(xl-b,0)))
	gxsm.set('SPMC_SLS_Ys','{}'.format(max(yt-b,0)))
	gxsm.set('SPMC_SLS_Xn','{}'.format(min(xr-xl+2*b, dims[0]-max(xr-b,0))))
	gxsm.set('SPMC_SLS_Yn','{}'.format(min(yb-yt+2*b, dims[1]-max(yt-b,0))))

	ch=0
	r=gxsm.add_marker_object(ch, 'PointBB', 0xff00fff0, xl, yt, 1)
	r=gxsm.add_marker_object(ch, 'PointBB', 0xff00fff0, xr, yt, 1)
	r=gxsm.add_marker_object(ch, 'PointBB', 0xff00fff0, xl, yb, 1)
	r=gxsm.add_marker_object(ch, 'PointBB', 0xff00fff0, xr, yb, 1)
	print (r)

	return xl,xr, yt, yb

def clear_bbbox():
	gxsm.set('SPMC_SLS_Xs','0')
	gxsm.set('SPMC_SLS_Ys','0')
	gxsm.set('SPMC_SLS_Xn','0')
	gxsm.set('SPMC_SLS_Xn','0')

#m=get_gxsm_img(0)
#fig, axs = plt.subplots(1, 1, sharey=True, tight_layout=True)
#axs.hist(m.flatten(), bins=100)
#plt.yscale('log')
#plt.savefig("/tmp/hist.pdf", format="pdf", bbox_inches="tight")


ch=0
r=gxsm.marker_getobject_action(ch, 'PointCM','REMOVE')
print(r)
time.sleep(1)
m, cmx, cmy, cx, cy = get_gxsm_img_cm(ch, 0.5) # STM for CM
#m, cmx, cmy, cx, cy = get_gxsm_img_cm_test(ch, 0.5) # STM for CM
#m, cmx, cmy, cx, cy = get_gxsm_img_cm_test(ch, 2.0) # AFM for BB
#find_bbox(cx, cy)
time.sleep(1)
r=gxsm.marker_getobject_action(ch, 'PointCM','SET-OFFSET')
print(r)
