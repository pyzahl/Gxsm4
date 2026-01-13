######## Before you run change file path. If not enough molecules found Lower threshold

######## NOTES TO WORK ON ########


#Tip goes to the center of the brightest spot and makes a box around it. 
#The boxes have to be away from the edges!!! Or else they behave poorly. and give a weird rectangle shape. Anything around the edges does not image the molecule!! 

##################  Imports ############################

from string import *
import os

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
import tensorflow as tf

#from tensorflow import keras

modelpath="/mnt/3cc2e62a-a3ab-488b-9244-b7be56aa4d53/BNLBox2T/GoodvBadv1.h5"

###############################################
## INSTRUMENT
ZAV=8.0 # 8 Ang/Volt in V

### SETUP
map_ch=6   # MAP IMAGE CHANNEL  (0....N)
map_diffs=gxsm.get_differentials(map_ch)

afm_ch=2

def get_gxsm_img(ch):
	dims=gxsm.get_dimensions(ch)
	return gxsm.get_slice(ch, 0,0, 0,dims[1]) # ch, v, t, yi, yn


def get_gxsm_img_cm(ch):
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
			m[y][x]=gxsm.get_data_pkt (ch, x, y, v, 0)*diffs[2] # Z value in Ang now

	cmx = 0
	cmy = 0
	csum = 0
	cmed = np.median(m)
	print ('Z base: ', cmed)
	b=2
	for y in range (b,dims[1]-b):
		for x in range (b, dims[0]-b):
			v=0
			m[y][x]=m[y][x] - cmed # Z value in Ang now
			if m[y][x] > 0.5:
				cmx = cmx+x*m[y][x]
				cmy = cmy+y*m[y][x]
				csum = csum + m[y][x]
	if csum > 0:
		cmx = cmx/csum
		cmy = cmy/csum
	else:
		cmx = dims[0]/2
		cmy = dims[1]/2
	gxsm.add_marker_object(ch, 'PointCM',1, int(round(cmx)), int(round(cmy)), 1.0)
	export_drawing(ch, '-CM')
	return m, cmx, cmy
	
def prepare(ch):
	IMG_SIZE = 256
	img_array = get_gxsm_img(ch)
	img_array = img_array/255.0
	new_array = cv2.resize(img_array, (IMG_SIZE, IMG_SIZE))
	new_array = np.array(new_array).reshape(-1, IMG_SIZE, IMG_SIZE, 1)
    
	return new_array
	



CATEGORIES = ["Far", "Good"]
model = tf.keras.models.load_model(modelpath)
prediction = model.predict(prepare(afm_ch))
print(CATEGORIES[int(prediction[0][0])])
