# GXSM4 HR-AFM force-volume map analysis and interpolation tool
# PyZ 20250821

# A) LOAD DATA SET, TRANSPOSE TO HAVE SET IN LAYER DIMENSION, PUT INTO SCAN-CH (2)
# B) Optional: load vpdata files with ref F(z)'s (DnD on CH2) -- expects slow set from GVP index 600 .. 1600
# C) CONTROL VIA "STOP" parameter, 0: run interactive, 1: exit, >2: 5: run full interpolation, else remap tmp .npr to CH10

# **) no drift, etc correct at this time -- pre precessing required

import numpy as np
from numpy.polynomial import polynomial as poly
from scipy.optimize import curve_fit
import time
from datetime import datetime
import matplotlib.pyplot as plt
from scipy.signal import butter, freqz, ellip
import math
import time
import matplotlib as mpl
mpl.use('Agg')
from matplotlib import ticker
mpl.pyplot.close('all')

sc = dict(CH=2, A0=3,B0=32,Z0=0., dFmin=-5.0, Zmax=30, SZmax=10, SZmin=-0.5, F0off=-0.21, STOP=0)
rp_freq_dev = -0.1  ### eventual Hz offset/thermal drift for later measured dF(z)

# Setup SCs
def SetupSC():
	for i, e in enumerate(sc.items()):
		id='py-sc{:02d}'.format(i+1)
		#print (id, e[0], e[1])
		gxsm.set_sc_label(id, e[0])
		gxsm.set(id, '{:.4f}'.format(e[1]))

SetupSC()

# Read / Update dict
def GetSC():
	for i, e in enumerate(sc.items()):
		id='py-sc{:02d}'.format(i+1)
		#print (id, ' => ', e[0], e[1])
		sc[e[0]] = float(gxsm.get(id))
		#print (id, '<=', sc[e[0]])

# Update SCs
def SetSC():
	for i, e in enumerate(sc.items()):
		id='py-sc{:02d}'.format(i+1)
		gxsm.set(id, '{:.4f}'.format(e[1]))

## MODEL FUNCTIONS used for fitting
def LJ(z,d,s,e):
	z = z-sc['Z0']
	x = s/(z+d)
	return 4*e/s*(12*np.power(x, 13) - 6*np.power(x, 7))

def LJ_bgX(z,a,b,c,f0,z0):
	z = z-sc['Z0']
	return a*np.exp(-b*(z-z0))-f0

def LJ_bg(z,a,d):
	z = z-sc['Z0']
	return a*np.power(1/(z+d), 5)

def LJ_bgref(z,a,d):
	z = z-sc['Z0']
	return a*np.power(1/(z+d), 5)



## template
def plot_vpdata(cols=[1,2,4]):
    plt.close ('all')
    plt.figure(figsize=(8, 4))
    yl = ''
    for c in cols[1:]:
            plt.plot(columns[cols[0]], columns[c], label=labels[c])
            if yl != '':
                    yl = yl + ', ' + labels[c]
            else:
                    yl = labels[c]
    plt.title('VPDATA  FS={:.2f} Hz'.format(FS))
    plt.xlabel(labels[cols[0]])
    plt.ylabel(yl)
    plt.legend()
    plt.grid()
    plt.show()
    plt.savefig('/tmp/gvp.png')
    plt.show()

# run fits
def run_bg_and_lj_fits(lj_z, xy, B0, A0,j):
	try:
		popt, pcov = curve_fit(LJ_bg, xy[0][B0:], xy[1][B0:])
		bg_fit = LJ_bg(xy[0], *popt)
		bg_fit_curve = LJ_bg(lj_z, *popt)
		fmol = xy[1]-bg_fit
	except:		
		print ('Fit fail for BGfit#{}'.format(j+1))
		bg_fit_curve = np.zeros(0)
		fmol = xy[1]

	try:
		popt, pcov = curve_fit(LJ, xy[0][A0:], fmol[A0:])
		lj_fit = LJ(lj_z, *popt)
		lj_curve = lj_fit + bg_fit_curve
	except:
		print ('Fit fail for LJfit#{}'.format(j+1))
		#print ('Data:', xy[0][A0:], fmol[A0:])
		lj_curve = bg_fit_curve  #np.zeros(lj_z.size)

	return lj_curve, bg_fit_curve

# get data from scan
def dFreqZ(ch, p,zmap,n, m):
	fz = np.zeros ((1+m,n))
	fz[0] = np.array(zmap[0:n])
	for j in range (0,m):
		#print ('get dFreq for:',ch,j,n,m, ' P:',p)
		dfl = gxsm.get_slice_v(ch, p[j][0],0, p[j][1],1) # ch, x, t, yi, yn   ## AFM dFreq in CH3
		#print ('dfl:',dfl.shape)
		#print(dfl)
		fz[1+j] = np.array(dfl[0][0:n])  - sc['F0off']
	return fz

# get data and average over rectangle area in 2d (in z-plane)
def dFreqZrectav(ch, p,zmap,n, m):
	fz = np.zeros ((2,n))
	fz[0] = np.array(zmap[0:n])
	count=0
	for j in range (0,m):
		yn = p[j][3] - p[j][1]
		#print ('get dFreq for:',ch,j,n,m, ' P:',p)
		for x in range (p[j][0], p[j][2]):
			dfl = gxsm.get_slice_v(ch, x,0, p[j][1],yn) # ch, x, t, yi, yn   ## AFM dFreq in CH3
			#print ('dfl:',dfl.shape)
			#print(dfl)
			fz[1] = fz[1] + (np.array(dfl[0][0:n]) - sc['F0off'])
			count=count+1
	fz[1] = fz[1] / count
	return fz
	
# run bg reference fit
def run_bgref_fit(lj_z, xy, B0):
	try:
		popt, pcov = curve_fit(LJ_bgref, xy[0][B0:], xy[1][B0:])
		bg_fit = LJ_bg(xy[0], *popt)
		bg_fit_curve = LJ_bg(lj_z, *popt)
	except:		
		print ('Fit fail for BGreffit')
		bg_fit = np.zeros(xy[0].size)
		bg_fit_curve = np.zeros(lj_z.size)
	return bg_fit, bg_fit_curve

def CalcBgF(ch, bgr, zlist, nz, num_bgrs, B0):
	fz = dFreqZrectav(ch, bgr, zlist, nz, num_bgrs )
	print ('FZBG:', fz)
	return fz

# visualize, plot and refit curves
# use feh to auto plot/updated resulting graph from /tmp/fz.png!
def plot_xy(lj_z, xy, m, A0, B0, sbg_fit, sbg_curve):
	plt.close ('all')
	plt.figure(figsize=(12, 8))

	print ('Fit BG start Z: ', xy[0][A0])
	plt.axvline(x=xy[0][A0])

	print ('Fit BG start Z: ', xy[0][B0])
	plt.axvline(x=xy[0][B0])

	print ('PLOTTING, FITTING')

	plt.plot(xy[0][B0:], sbg_fit[B0:], 'o', label='FzSBg')
	plt.plot(lj_z, sbg_curve, label='SBGfit')

	for j in range (0,m):
		print ('SET #', j)
		plt.plot(xy[0], xy[1+j], 'x', label='Fz#{}'.format(j+1))
		
		print ('FITTING...#', j)
		#lj_curve, bg_curve = run_bg_and_lj_fits(lj_z, [xy[0],xy[1+j]-sbg_fit], B0, A0, j)
		lj_curve, bg_curve = run_bg_and_lj_fits(lj_z, [xy[0],xy[1+j]], B0, A0, j)
		if bg_curve.size > 0:
			plt.plot(lj_z, bg_curve, label='BGfit#{}'.format(j+1))
		if lj_curve.size > 0:
			plt.plot(lj_z, lj_curve, label='LJfit#{}'.format(j+1))
			#plt.plot(lj_z, lj_curve+sbg_curve, label='LJfit#{}'.format(j+1))

	# plot FZ probe(s) loaded
	Np = gxsm.get_probe_event(ch,1000)  # get count
	if Np > 0:
		for j in range (0,Np):
			#columns, labels, units = gxsm.get_probe_event(ch,-1)  # get last
			columns, labels, units = gxsm.get_probe_event(ch,j)  # get i-th
			#	print (columns, labels, units)
			col_z=0
			col_dF=0
			i=0
			for l in labels:
				if l == '	ZS-Topo':
					col_z=i
				if l == 'dFrequency':
					col_dF=i
				i=i+1
			z0 = 0 #columns[col_z][0]   ## SHIFT start to 0
			cz = columns[col_z][600:1600]
			cdf = columns[col_dF][600:1600] - sc['F0off']+rp_freq_dev
			plt.plot(cz-z0, cdf, alpha=0.3,label='FzProbe#{}'.format(j))

	plt.ylim(sc['dFmin'],1.0)

	plt.title('GXSM Force Volume Data Explorer * F(z)')
	plt.xlabel('Z in Ang, 0 is closed approach possible or data set')
	plt.ylabel('dFreq in Hz')
	plt.legend()
	plt.grid()
	#plt.show()
	plt.savefig('/tmp/fz.png')

# visualize, plot and refit curves
# use feh to auto plot/updated resulting graph from /tmp/fz.png!
def plot_zreferencing(lj_z, lj_curve, z_probe, df_probe, z_maps, z_mapdfs):
	plt.close ('all')
	plt.figure(figsize=(12, 8))

	plt.plot(lj_z, lj_curve, label='LJ Curve Fit')
	plt.plot(z_probe, df_probe, '.', alpha=0.3, label='LJ Probes')
	plt.plot(z_maps, z_mapdfs, 'x',  label='LJ Map Pointst')


	# plot FZ probe(s) loaded
	Np = gxsm.get_probe_event(ch,1000)  # get count
	if Np > 0:
		for j in range (0,Np):
			#columns, labels, units = gxsm.get_probe_event(ch,-1)  # get last
			columns, labels, units = gxsm.get_probe_event(ch,j)  # get i-th
			#	print (columns, labels, units)
			col_z=0
			col_dF=0
			i=0
			for l in labels:
				if l == '	ZS-Topo':
					col_z=i
				if l == 'dFrequency':
					col_dF=i
				i=i+1
			z0 = 0 #columns[col_z][0]   ## SHIFT start to 0
			cz = columns[col_z][600:1600]
			cdf = columns[col_dF][600:1600] - sc['F0off']+rp_freq_dev
			plt.plot(cz-z0, cdf, alpha=0.3,label='FzProbe#{}'.format(j))

	plt.ylim(sc['dFmin'],1.0)

	plt.title('FZAlign GXSM Force Volume Data Explorer * F(z)')
	plt.xlabel('Z in Ang, 0 is closed approach possible or data set')
	plt.ylabel('dFreq in Hz')
	plt.legend()
	plt.grid()
	#plt.show()
	plt.savefig('/tmp/fzalign.png')



# Compute full interpolated map
def calculate_interpolated_HRmap(ch, zlist, slices, zmin, zmax, A0, B0):
	N,M,V,T=gxsm.get_dimensions(ch)
	lj_z   = np.linspace(zmin, zmax, slices)
	print (lj_z)
	print (M,N,slices)
	dFvol = np.zeros((M,N,slices))
	for x in range(0, N):
		for y in range(0, M):
	### Testing on small window
	#for x in range(90, 160):
	#	for y in range(30, 120):
			print ('LJ inter @XY ', x,y)
			fz = dFreqZ(ch, [[x,y]], zlist, zlist.size, 1)
			lj , bg = run_bg_and_lj_fits(lj_z, fz, B0, A0, x+y*250)
			if lj.size > 0:
				dFvol[y][x] = np.array(lj)
		GetSC()
		if sc['STOP'] > 5:
			break
	return dFvol



# fetch marker positions: Rectangle(s) for BG ref, Point(s) for individual curves
def get_marker_pos(ch):
	num_points=0
	num_bgrects=0
	for mi in range(0,100):
		r=gxsm.get_object(ch,mi)
		if (r != 'None'):
			if r[0] == 'Rectangle':
				num_bgrects = num_bgrects+1
			if r[0] == 'Point':
				num_points = num_points+1

	p = np.zeros((num_points,2), int)
	bgref = np.zeros((num_bgrects,4), int)
	#columns, labels, units = gxsm.get_probe_event(0,-1)  # get last
	i=0
	j=0
	for mi in range(0,100):
		#print('selecting M, CH:',mi, ch)
		r=gxsm.get_object(ch,mi)
		if (r != 'None'):
			#print('found M, CH:',mi, ch)
			#print(r)
			if r[0] == 'Rectangle':
				bgref[j] = [int(r[1]), int(r[2]), int(r[3]), int(r[4])]
				j=j+1
			if r[0] == 'Point':
				p[i] = [int(r[1]), int(r[2])]
				i=i+1
		else:
			break
	return p, bgref, num_points, num_bgrects


def get_z_list_old():
	#[ 0.    0.1   0.2   0.3   0.4   0.5   0.6   0.7   0.8   0.9   1.    1.2   1.4   1.6   1.8   2.    2.25  2.5   2.75  3.    3.25  3.5   3.75  4.   4.5   5.    5.5   6.    6.5   7.    7.5  10.   20.  ]
	  
	zlist = np.arange(0.0, 1.0, 0.1)
	zlist = np.append(zlist, np.arange(1.0, 2.0, 0.2))
	zlist = np.append(zlist, np.arange(2.0, 4.0, 0.25))
	zlist = np.append(zlist, np.arange(4.0, 8.0, 0.5))
	zlist = np.append(zlist, 10.0)
	zlist = np.append(zlist, 20.0)

## 2025082022xx
def get_z_list_last():
	zlist = np.arange(0.0, 1.0, 0.1)
	zlist = np.append(zlist, np.arange(1.0, 2.0, 0.2))
	zlist = np.append(zlist, np.arange(2.0, 4.0, 0.25))
	zlist = np.append(zlist, np.arange(4.0, 8.0, 0.5))
	zlist = np.append(zlist, 9.0)
	zlist = np.append(zlist, 10.0)
	zlist = np.append(zlist, 12.5)
	zlist = np.append(zlist, 15.0)
	zlist = np.append(zlist, 20.0)
	zlist = np.append(zlist, 30.0)
	#zlist = np.append(zlist, 1.0)
	#zlist = np.append(zlist, 10.0)

	return zlist

## from topo data
def get_z_list(start=0, end=999):
	ch=4-1
	dx,dy,dz,dl  = gxsm.get_differentials (ch)
	nx,ny,nv,nt  = gxsm.get_dimensions (ch)
	if end < nt:
		nt=end
	if nt > 1000:
		return
	print ('Topo in CH ',ch+1, " #Z slices:",  nt)
	zlist = np.zeros(0)
	zi = dz*gxsm.get_data_pkt (ch, 10, 10, 0, start)
	
	for ti in range(start,nt):
		z = dz*gxsm.get_data_pkt (ch, 10, 10, 0, ti)
		print(ti,z)
		zlist = np.append(zlist, z)
	return zlist

def get_z_list_from_dFz_curves(ch, start=0, end=999):
	# start with topo Z and check/correct by average of all curves loaded
	chz=4-1
	dx,dy,dz,dl  = gxsm.get_differentials (chz)
	nx,ny,nv,nt  = gxsm.get_dimensions (chz)
	dx,dy,df,dl  = gxsm.get_differentials (ch)
	if end < nt:
		nt=end
	if nt > 1000:
		return
	print ('Topo in CH ',ch+1, " #Z slices:",  nt)
	zlist = np.zeros(0)
	zdflist = np.zeros(0)
	zi = dz*gxsm.get_data_pkt (chz, 10, 10, 0, start)
	
	for ti in range(start,nt):
		z = dz*gxsm.get_data_pkt (chz, 10, 10, 0, ti)
		zdf = df*gxsm.get_data_pkt (ch, 10, 10, 0, ti)
		print(ti,z)
		zlist = np.append(zlist, z)
		zdflist = np.append(zdflist, zdf)

	print ('zlist', zlist)
	print ('zdflist', zdflist)
		
	Np = gxsm.get_probe_event(ch,1000)  # get count
	if Np > 0:
		for j in range (0,Np):
			#columns, labels, units = gxsm.get_probe_event(ch,-1)  # get last
			columns, labels, units = gxsm.get_probe_event(ch,j)  # get i-th
			#	print (columns, labels, units)
			col_z=0
			col_dF=0
			i=0
			for l in labels:
				if l == '	ZS-Topo':
					col_z=i
				if l == 'dFrequency':
					col_dF=i
				i=i+1
			z0 = 0 #columns[col_z][0]   ## SHIFT start to 0
			cz = columns[col_z][600:1600]
			cdf = columns[col_dF][600:1600] - sc['F0off']+rp_freq_dev
			#plt.plot(cz-z0, cdf, alpha=0.3,label='FzProbe#{}'.format(j))
			if j == 0:
				z_probe = cz-z0
				df_probe = cdf
			else:
				z_probe = np.append (z_probe, cz-z0)
				df_probe = np.append (df_probe, cdf)
				
			break

		A0 = int(sc['A0']) ## !!! remap
		B0 = int(sc['B0'])  ## !!! remap via zlist and 1000 in linespace below
		print ('A0Z: ', zlist[A0])
		print ('B0Z:', zlist[B0])
		lj_z   = np.linspace(sc['Z0'] + sc['SZmin'], sc['Z0'] + sc['Zmax'], 1000)
		dzi = 1000/( sc['Zmax'] - sc['SZmin'] )
		A0z = int (round (dzi * ( zlist[A0]-sc['Z0'] - sc['SZmin'] )))
		B0z = int (round (dzi * ( zlist[B0]-sc['Z0'] - sc['SZmin'] )))
		print (dzi, ' => ',A0z, B0z, '    arg', zlist[B0]-sc['Z0'] - sc['SZmin'])
		#lj_curve, bg_curve = run_bg_and_lj_fits(lj_z, [z_probe,df_probe], B0z, A0z, 0) ## 
		popt, pcov = curve_fit (LJ_bg, z_probe, df_probe)
		lj_curve = LJ_bg(lj_z, *popt)

	print ('LJ_Z:',  lj_curve)
	z_maps = []
	z_mapdfs = []
	plot_zreferencing(lj_z, lj_curve, z_probe, df_probe, z_maps, z_mapdfs)
	
		# merge all curves
		# fit
		# find Z's for dF's
		
		#lj_curve, bg_curve = run_bg_and_lj_fits(lj_z, [xy[0],xy[1+j]], B0, A0, j)
		
		
	return zlist

##### MAIN  ######
print('HELP to watch plot: Install: apt-get feh, run $feh /tmp/fz.png')

# Init
GetSC()
ch = int(sc['CH'])-1
pp, rr, npp, nrr = get_marker_pos(ch)
npp=0
nrr =0

# Get Z mapping
zlist = get_z_list(1,39)
print ('Z-List: ', zlist)
print ('#Z:', zlist.size)
sc['Z0'] = zlist[0]
SetSC()


zlist_dum =  get_z_list_from_dFz_curves(ch, start=0, end=999)


# Watch Point Objects and update if changed
while sc['STOP'] == 0.:
	ch = int(sc['CH'])-1
	A0 = int(sc['A0'])
	B0 = int(sc['B0'])

	p, bgr, num_points, num_bgrs = get_marker_pos(ch)
	lj_z   = np.linspace(sc['Z0'] + sc['SZmin'], sc['Z0'] + sc['Zmax'], 300)
	
	# substrate background by rect areas
	if ((not np.array_equal(bgr ,rr) ) or  num_bgrs != nrr):
		rr =bgr
		nrr = num_bgrs
		#print ('BG Rects: ', bgr)
		fbg = CalcBgF(ch, bgr, zlist, zlist.size, num_bgrs, B0)
		#print ('FBG:', fbg)

		bg_fit, bg_curve =  run_bgref_fit(lj_z, fbg, B0)
		
		fz = dFreqZ(ch, p, zlist, zlist.size, num_points)
		plot_xy( lj_z, fz, num_points,A0,B0, fbg[1], bg_curve)

	# curves on points
	if ((not np.array_equal(p , pp) ) or  num_points != npp):
		pp = p
		npp = num_points
		print ('Points: ', p)

		fz = dFreqZ(ch, p, zlist, zlist.size, num_points)
		plot_xy(lj_z, fz, num_points,A0,B0, fbg[1], bg_curve)

	# get updated params
	GetSC()

######################################
print ('Stopped Interactive Mode')

# run full interpolate?
if sc['STOP'] > 2:
	# fetch dimensions
	dims=gxsm.get_dimensions(ch)
	print (dims)
	geo=gxsm.get_geometry(ch)
	print (geo)

	if sc['STOP'] == 5:
		print ('Calculating Map')
		map = calculate_interpolated_HRmap(ch,zlist, int(sc['slices']), sc['SZmin'], sc['SZmax'], A0, B0)

		# Adjust shape to matcgh GXSM interface [dim_layers, dimx, dimy]
		print ('Map shape:', map.shape, '   Size:', map.size)

		map=np.rollaxis(map, 2)
		print ('Map shape swapped for gxsm transfer:', map.shape, '   Size:', map.size)

		# store to npc for later
		print ('saving map')
		with open('/tmp/gxsm_map_tmp.npr', 'wb') as f:
	        	np.save(f, map)
	else:
		print ('loadng map')
		with open('/tmp/gxsm_map_tmp.npr', 'rb') as f:
	        	map=np.load(f)

		        	
	print ('Map shape for Gxsm:', map.shape, '   Size:', map.size)

	# put back into Gxsm scan window CH #10 for viewing
	tch=10
	n = np.ravel(map) # make 1-d
	mem2d = n.astype(np.float32) # make float32
	gxsm.chmodea (tch)
	nv,ny,nx = map.shape
	
	print (mem2d, mem2d.size, mem2d.shape)
	gxsm.createscanf (tch, nx, ny, nv, geo[0], geo[1], mem2d, 0)
	#gxsm.add_layerinformation ("@Hz",10)

	# be nice and auto update/autodisp
	gxsm.chmodea (tch)
	gxsm.direct ()
	gxsm.autodisplay ()

#### END #####



