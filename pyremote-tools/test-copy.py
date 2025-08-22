import netCDF4 as nc
import struct
import array
import math
import numpy as np


def copy_img_pkt(ch1, ch2):
	# fetch dimensions
	dims=gxsm.get_dimensions(ch1)
	print (dims)
	geo=gxsm.get_geometry(ch1)
	print (geo)
	diffs=gxsm.get_differentials(ch1)
	print (diffs)

	dims=gxsm.get_dimensions(ch1)
	m = gxsm.get_slice(ch1, 0,0, 0,dims[1]) # ch, v, t, yi, yn

	for y in range (0,dims[1]):
		for x in range (0, dims[0]):
			v=0
			gxsm.put_data_pkt(m[y][x]/diffs[ch1], ch2, x, y, v, 0)  # Z value in Ang now
	return m


copy_img_pkt(3,1)