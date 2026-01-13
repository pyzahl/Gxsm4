####### Fifth Version of AI: Version as of October 17th 2022

######## NOTES TO WORK ON ########
#Best algorithm to use for the AI decision. 

import sys
sys.modules[__name__].__dict__.clear()

### MODES

TEST_AI=True #False # Test Mode

do_auto_locate = False
do_stm = True
do_afm = True
do_AI_afm = True #False

###############################################
## INSTRUMENT
ZAV=8.0 # 8 Ang/Volt in V

### SETUP ENVIRONMENT CONFIGURATION
map_ch=6   # MAP IMAGE CHANNEL  (0....N)
map_diffs=gxsm.get_differentials(map_ch)
afm_ch=2  # HR_AFM IMAGE (0...N)

### DEFAULTS
# Up to 8 ScriptControls (sc)
# Level, I in pA
sc = dict(STM_Range=45, AFM_Range=45,  Molecule=1,  I_ref=20, CZ_Level=100.0,  Z_down=1.3, Z_start=0.0, Tip_Z=0.0,  Tip_Z_Ref=0.0, AutoAFMSpeed=1)

STM_ref_bias = 0.6 # 0.1
STM_scan_current = 0.0025 # 0.01
STM_points = 160
STM_dx=sc['STM_Range']/STM_points

AFM_points = 330
AFM_dx=sc['AFM_Range']/AFM_points
#sc['STM_Range']

#CZAFM_Iref=0.04
#CZAFM_Zoff=-0.1
#CZ_FUZZY_level =0.1




##################  Imports ############################
import sys
sys.modules[__name__].__dict__.clear()

from string import *
import os
import logging
from string import *
import os
import datetime


import time
import random as rng
import cv2
#import netCDF4 as nc
import struct
import array
import math
import numpy as np
from skimage.color import rgb2gray
from skimage.color import gray2rgb
import itertools

import torch

x = torch.rand(5, 3)
print(x)
print('CUDA: ',torch.cuda.is_available())
print('***')

device=torch.device("cuda:0")
x = torch.tensor([1.0]).to(device)



# Import Detectron2 should be installed in your local computer. https://detectron2.readthedocs.io/en/latest/tutorials/install.html
import detectron2
from detectron2.utils.logger import setup_logger
setup_logger()
# Import some common detectron2 utilities
from detectron2 import model_zoo
from detectron2.engine import DefaultPredictor
from detectron2.config import get_cfg

#Register the dataset and metadata for the model.
#the dataset are the files used for the training, testing, and validation used. No need for all 3 to run.

from detectron2.data.datasets import register_coco_instances
from detectron2.data import DatasetCatalog, MetadataCatalog

## Setup logfile
full_path_name = gxsm.chfname(0).split()[0]
##
print('Starting here, Logfile setup.')
print(full_path_name)
ncfname = os.path.basename(full_path_name)
folder = os.path.dirname(full_path_name)
name, ext = os.path.splitext(ncfname)
logfile_name = folder+'/'+name+'-AIrun-initial.log'
print('Logging to: ', logfile_name)

logging.basicConfig(filename=logfile_name, encoding='utf-8', level=logging.DEBUG, format='%(asctime)s %(message)s')
logging.info('Auto-AFM-AI logfile start. [{}]'.format(logfile_name))

#logging.debug('This message should go to the log file')
#logging.info('So should this')
#logging.warning('And this, too')
#logging.error('Error')




DatasetCatalog.clear()

#Register the data. Where is it located locally?
# download, and extract, adjust path below accordingly:  https://bnlbox.sdcc.bnl.gov/index.php/s/9xoFrTsPWmcB9Gp
#AI_base_dir = '/home/percy/AI-data'
AI_base_dir = '/home/pzahl/BNLBox/Data/AICLS'
register_coco_instances("3class_train", {}, AI_base_dir+'/labels_clasestres_2022-07-26-01-21-07.json', AI_base_dir+'/files')

dataset_dicts = DatasetCatalog.get("3class_train")
microcontroller_metadata = MetadataCatalog.get("3class_train")

cfg = get_cfg()
cfg.merge_from_file(model_zoo.get_config_file("COCO-InstanceSegmentation/mask_rcnn_R_50_FPN_3x.yaml"))
cfg.MODEL.ROI_HEADS.NUM_CLASSES = 4
# Load weights from model and decide on Threshold. Run predictor
cfg.MODEL.WEIGHTS = AI_base_dir+'/AI_model.pth'
cfg.MODEL.ROI_HEADS.SCORE_THRESH_TEST = 0.5
predictor = DefaultPredictor(cfg)




################################### Function Definitions############################################################################################
# Setup SCs
def SetupSC():
	for i, e in enumerate(sc.items()):
		id='py-sc{:02d}'.format(i+1)
		print (id, e[0], e[1])
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


#GetSC()
#SetSC()

gxsm.set('script-control','1')

def export_drawing(ch=0, postfix='-dwg'):
	full_original_name = gxsm.chfname(ch).split()[0]
	print(full_original_name)
	folder = os.path.dirname(full_original_name)
	ncfname = os.path.basename(full_original_name)
	name, ext = os.path.splitext(ncfname)
	dest_name = folder+'/'+name+postfix
	print('Exporting: ', dest_name)
	gxsm.chmodea(ch)
	gxsm.autodisplay()
	time.sleep(1)
	gxsm.save_drawing(ch, 0,0, dest_name+'.png')
	gxsm.save_drawing(ch, 0,0, dest_name+'.pdf')

def export_png(ch=0, postfix='autoexport'):
	full_original_name = gxsm.chfname(ch).split()[0]
	print(full_original_name)
	folder = os.path.dirname(full_original_name)
	ncfname = os.path.basename(full_original_name)
	name, ext = os.path.splitext(ncfname)
	dest_name = folder+'/'+name+postfix
	print('Exporting: ', dest_name)
	gxsm.chmodea(ch)
	gxsm.autodisplay()
	time.sleep(1)
	gxsm.save_drawing(ch, 0,0, dest_name+'.png')


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
			#print(r, " => Z: ", zxy, " Min/Max: ", zmin, zmax)
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

def auto_afm_scanspeed(y):
	ms = gxsm.get_slice(2, 0,0, y,1) # ch, v, t, yi, yn   ## AFM dFreq in CH3
	med = np.median(ms)
	dFspan = np.max(ms) - np.min(ms)
	if dFspan > 1.0:
		gxsm.set ("dsp-adv-scan-fast-return","5")
		time.sleep(1)
		gxsm.set ("dsp-fbs-scan-speed-scan","8")
	elif dFspan > 0.8:
		gxsm.set ("dsp-adv-scan-fast-return","5")
		time.sleep(1)
		gxsm.set ("dsp-fbs-scan-speed-scan","10")
	elif dFspan > 0.5:
		gxsm.set ("dsp-adv-scan-fast-return","5")
		time.sleep(1)
		gxsm.set ("dsp-fbs-scan-speed-scan","15")
	elif dFspan > 0.4:
		gxsm.set ("dsp-adv-scan-fast-return","2")
		time.sleep(1)
		gxsm.set ("dsp-fbs-scan-speed-scan","20")
	elif dFspan > 0.3:
		gxsm.set ("dsp-adv-scan-fast-return","2")
		time.sleep(1)
		gxsm.set ("dsp-fbs-scan-speed-scan","30")
	else:
		gxsm.set ("dsp-adv-scan-fast-return","1")
		time.sleep(1)
		gxsm.set ("dsp-fbs-scan-speed-scan","50")
	#print('Median: ', np.median(ms))
	#print('Min: ', np.min(ms))
	#print('Max: ', np.max(ms))
	#print('Range: ', np.max(ms) - np.min(ms))


def get_gxsm_img_bypkt(ch):
	# fetch dimensions
	dims=gxsm.get_dimensions(ch)
	#print (dims)
	geo=gxsm.get_geometry(ch)
	#print (geo)
	diffs=gxsm.get_differentials(ch)
	#print (diffs)
	m = np.zeros((dims[1],dims[0]), dtype=float)
	for y in range (0,dims[1]):
		for x in range (0, dims[0]):
			v=0
			m[y][x]=gxsm.get_data_pkt (ch, x, y, v, 0)*diffs[2]  # Z value in Ang now
	return m


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
	print('PointCM: ', int(round(cmx)), int(round(cmy)))
	gxsm.add_marker_object(ch, 'PointCM',1, int(round(cmx)), int(round(cmy)), 1.0)
	export_drawing(ch, '-CM')
	return m, cmx, cmy

def ai_decide(ch):
	full_path_name = gxsm.chfname(ch).split()[0]
	cfname = os.path.basename(full_path_name)
	folder = os.path.dirname(full_path_name)
	name, ext = os.path.splitext(ncfname)
	print ('AI decide on: ', name)

	img = get_gxsm_img(ch) # Load image from AFM channel
	norm_img = cv2.normalize(img, None, 0, 255, cv2.NORM_MINMAX, cv2.CV_8U) # Normalize the color scale of the image from 0 to 255
	rgb_img = gray2rgb(norm_img) # Turn Grayscale to RGB
	im_bgr = cv2.cvtColor(rgb_img, cv2.COLOR_RGB2BGR) # Turn RGB to BGR
	print(im_bgr.shape) #Print Shape to double check correct input. Should be (330,330,3) 
	outputs = predictor(im_bgr) # Using the predictor from the model.
	
	# Place the outputs into arrays to use them easier. 
	mask_array = outputs['instances'].pred_masks.to("cpu").numpy()
	num_instances = mask_array.shape[0]
	scores = outputs['instances'].scores.to("cpu").numpy()
	labels = outputs['instances'].pred_classes .to("cpu").numpy()
	bbox   = outputs['instances'].pred_boxes.to("cpu").tensor.numpy()
	
	#print ('Mask Array:')
	#print (mask_array)
	
	# Create a mask for the Input AFM image
	mask_array = np.moveaxis(mask_array, 0, -1)
	mask_array_instance = []
	height = im_bgr.shape[0]
	width = im_bgr.shape[1]
	img_mask = 255*np.ones([height, width, 3], np.uint8) # zeros
	
	for i in range(num_instances):
		if labels[i]==0:
#			color = (250, 43, 138) #Purple Color for close and distortion regions
			color = (128, 43, 250) #Purple Color for close and distortion regions
		elif labels[i]==1:
#			color = (0, 1, 255) #Red Color for far regions
			color = (255, 1, 0) #Red Color for far regions
		else:
#			color = (0,255,0) #Green color for ideal ring regions
			color = (0,255,0) #Green color for ideal ring regions
		image = np.zeros_like(im_bgr)
		mask_array_instance.append(mask_array[:, :, i:(i+1)])
		image = np.where(mask_array_instance[i] == True, 255, image)
		array_img = np.asarray(image)
		img_mask[np.where((array_img==[255,255,255]).all(axis=2))]=color
	img_mask = np.asarray(img_mask)
	#gxsm.load(9, img_mask)
	purple = np.where(img_mask[:,:,1] == 43)
	red = np.where(img_mask[:,:,1] == 1)
	green = np.where(img_mask[:,:,1] == 255)
	p = purple[0].size
	r = red[0].size
	g = green[0].size
	sum = p + r + g
	#print(purple[1].size)
	#print(red[1].size)
	#print(green[1].size)
	#print(sum)
	if ( g > p and g > r):
		print("Good")
		action = 'okay'
	elif( p > g and p > r):
		print("Close")
		action = 'close'
	else:
		print("Far")
		action = 'far'
	logging.info('ai_decide on {}: {}'.format(name, action))
	return action, img_mask


def ai_mask_to_gxsm_ch (ai_mask_img, chm):
	img_shape= ai_mask_img.shape
	rgb = np.moveaxis(ai_mask_img, 0, -1)
	rgb = np.moveaxis(rgb, 0, -1)
	n = np.ravel(rgb) # make 1-d
	mem2d = array.array('f', n.astype(float)) 
	mem2d=np.resize(mem2d, 4*img_shape[0]*img_shape[1])
	gxsm.chmodea (chm)
	gxsm.createscanf (chm, img_shape[1], img_shape[0], 4, 45, 45, mem2d, False)

	full_original_name = gxsm.chfname(afm_ch).split()[0]

	print(full_original_name)
	folder = os.path.dirname(full_original_name)
	ncfname = os.path.basename(full_original_name)
	name, ext = os.path.splitext(ncfname)
	dest_name = folder+'/'+name+'_ai_mask'
	print(dest_name)
	gxsm.save_drawing(chm, 0,0, dest_name+'.png')

    	
def locate_molecule_ch(ch,thresh_val):
	img = get_gxsm_img(ch)
	norm_img = cv2.normalize(img, None, 0, 255, cv2.NORM_MINMAX, cv2.CV_8U)
	gray_img = rgb2gray(norm_img)
	max_thresh = 255
	thresh = thresh_val
	def thresh_callback(val):
    		threshold = val
    		canny_output = cv2.Canny(gray_img, threshold, threshold*2)
    		contours,_ = cv2.findContours(canny_output, cv2.RETR_TREE, cv2.CHAIN_APPROX_SIMPLE)
    		contours_poly = [None]*len(contours)
    		boundRect = [None]*len(contours)
    		centers = [None]*len(contours)
    		radius = [None]*len(contours)
    		x_y_coord = []
    		for i, c in enumerate(contours):
    			contours_poly[i] = cv2.approxPolyDP(c, 3, True)
    			boundRect[i] = cv2.boundingRect(contours_poly[i])
    			centers[i], radius[i] = cv2.minEnclosingCircle(contours_poly[i])
    			if radius[i] < 3 and radius[i] > 1:
    				if centers[i] not in x_y_coord:
    					x_y_coord.append(centers[i])
    			
    		return (x_y_coord)
	x_y_molecules = thresh_callback(thresh)
	return (x_y_molecules)

def do_stm_and_lock_on_center(mi):		
	print('STM: Scanning M',mi)
	gxsm.startscan()
	time.sleep(2)
	gxsm.set ("dsp-fbs-scan-speed-scan","225")
	time.sleep(1)
	print('waiting....')

	l=0
	while l >= 0  and int(gxsm.get('script-control')) >0:
		l =gxsm.waitscan(False)
		#print ('Line=',l)
		time.sleep(2)

	gxsm.stopscan()
	time.sleep(2)
	
	print('STM completed. Centering.')

	#print('... cleanup old markers (make sure)')
	#r=gxsm.marker_getobject_action(0, 'PointCM','REMOVE')
	#print(r)
	time.sleep(1)
	print('calculate CM')
	m, cx, cy = get_gxsm_img_cm(0)
	print('CM:' ,cx,cy)
	time.sleep(1)
	print('Adjust Offset')
	r=gxsm.marker_getobject_action(0, 'PointCM','SET-OFFSET')
	print(r)
	time.sleep(2)
	print ('cleanup marker')
	r=gxsm.marker_getobject_action(0, 'PointCM','REMOVE')
	print(r)


def do_HR_AFM(mi, tipz):
	GetSC()
	z = sc['Tip_Z_Ref']
	sc['Tip_Z'] = -tipz
	sc['Molecule'] = mi
	SetSC()
	gxsm.set ("dsp-adv-dsp-zpos-ref", "{:8.2f}".format( z-tipz))
	print('HR-AFM: Scanning M',mi, ' at Z=', z-tipz)
	gxsm.startscan()
	time.sleep(2)
	print('waiting....')
	gxsm.set ("dsp-fbs-scan-speed-scan", "10")
	time.sleep(1)
	gxsm.set ("dsp-fbs-scan-speed-scan", "8")
	l=0
	lp=1
	
	next_ai_check=25

	#while gxsm.waitscan(False) >= 0 and  int(gxsm.get('script-control')) >0:
	while l >= 0  and int(gxsm.get('script-control')) >0:
		l =gxsm.waitscan(False)
		#print ('Line=',l)

		if do_AI_afm and l > next_ai_check:
			next_ai_check=l+25
			result, ai_mask_img = ai_decide(afm_ch)
			ai_mask_to_gxsm_ch (ai_mask_img, 10)

		time.sleep(5)
		GetSC()
		if l > lp and sc['AutoAFMSpeed'] > 0:
			auto_afm_scanspeed(l)
			lp=l+1

	print('HR-AFM completed, saving...')


################################################################## 

max_mol = 50


if  TEST_AI:
	afm_ch=0
	mask_ch=9
	ct = datetime.datetime.now()
	print (ct, ' ** Test AI on ch', afm_ch)
	result, ai_mask_img = ai_decide(afm_ch)
	print (ct, ' ** ')
	#ai_mask_to_gxsm_ch (ai_mask_img, 10)
	
	print ('Mask')
	img_shape= ai_mask_img.shape
	print (img_shape)
	#rgb = ai_mask_img
	
	afmimg = get_gxsm_img(afm_ch) # Load image from AFM channel
	print ('AFM img')
	print(afmimg.shape)
	afm_norm_img = cv2.normalize(afmimg, None, 0, 255, cv2.NORM_MINMAX, cv2.CV_8U) # Normalize the color scale of the image from 0 to 255
	afm_rgb_img = gray2rgb(afm_norm_img) # Turn Grayscale to RGB
	print(afm_rgb_img.shape)
	
	rgb = afm_rgb_img * (0.5*0.5*ai_mask_img/255)
	ai_mask_img = rgb
  
	rgb = np.moveaxis(ai_mask_img, 0, -1)
	rgb = np.moveaxis(rgb, 0, -1)
	print(rgb.shape)
  
        
	n = np.ravel(rgb) # make 1-d
	mem2d = array.array('f', n.astype(float)) 
	mem2d=np.resize(mem2d, 4*img_shape[0]*img_shape[1])
	#mem2d=np.roll(mem2d, -330*330)

	#afm = get_gxsm_img(afm_ch) # Load image from AFM channel
	#afmn = np.ravel(afm) # make 1-d
	#mem2d_afm = array.array('f', afmn.astype(float))
	#mem2d = np.concatenate ((mem2d_afm, mem2d))

	gxsm.chmodea (mask_ch)
	gxsm.createscanf (mask_ch,img_shape[1],img_shape[0],4, 45, 45, mem2d, False)
	#gxsm.add_layerinformation ("@ "+str(flv)+" Hz",10)
	#gxsm.createscanf : Create Scan float: gxsm.createscan (ch,nx,ny,nv pixels, rx,ry in A, array.array('f', [...]), append)
	
	max_mol = 0


################################################################## 

print('Map in CH', map_ch+1)

print('Removing all Rectangles!')
r=gxsm.marker_getobject_action(map_ch, 'Rectangle','REMOVE-ALL')
print(r)




if do_auto_locate:
	##### Locate Molecules using OpenCV Some functions might be extra could be use for later...###
	print('Finding Molecules')
	#molecule_coord = locate_molecule_nc(basefile, 65)
	molecule_coord = locate_molecule_ch(map_ch, 65)
	pro_molecule_coord = process(molecule_coord)
	time.sleep(1)
	print('Found Molecules:',len(pro_molecule_coord))

	##### Clean Up the channel being used################
	print('Removing all Rectangles!')
	r=gxsm.marker_getobject_action(map_ch, 'Rectangle','REMOVE-ALL')
	print(r)
	time.sleep(1)

	print('Cleanup Points')
	r=gxsm.marker_getobject_action(map_ch, 'Point','REMOVE-ALL')
	print(r)
	r=gxsm.marker_getobject_action(map_ch, '*Marker','REMOVE-ALL')
	print(r)
	time.sleep(1)

	#### Here add for loop to add boxes and labels to each molecule ######
	print('Marking Molecules at')
	print (pro_molecule_coord)

	for i in range(len(pro_molecule_coord)):	
		gxsm.add_marker_object(map_ch, 'PointM{:02d}'.format(i),1, int(pro_molecule_coord[i][0]),int(pro_molecule_coord[i][1]), 1.0)

	time.sleep(1)

	if max_mol > 0:
		gxsm.set('script-control','2')
		print('waiting as long as sc>1')
		while int(gxsm.get('script-control')) > 1:
			time.sleep(0.5)

print('List Objects, Mark Mol, Setup Rects')
k=0
for i in range(0, max_mol): ##len(pro_molecule_coord)):
	o=gxsm.get_object (map_ch, i+k) ## adjust for inserted object -- always pre pended to list!
	print('O', i, ' => ', o)
	if o == 'None':
		break
	print('Marking M', i)
	r=gxsm.add_marker_object(map_ch, 'RectangleM{:02d}'.format(i), 0xff00fff0, round(o[1]),round(o[2]), sc['AFM_Range']/map_diffs[0])
	k=k+1 # we have not one more object prepended to the object list!
	print(r)

SetSC()
time.sleep(1)

if max_mol > 0:
	gxsm.set('script-control','2')
	print('waiting as long as sc>1 -- check configurations now')
	while int(gxsm.get('script-control')) > 1:
		time.sleep(0.5)

GetSC()

# make sure STM safe mode
exit_force_map(0.1, current=0.006)


gxsm.set('script-control','3')

for mi in range(0,max_mol):  ##len(molecule_coord)):
	full_path_name = gxsm.chfname(0).split()[0]
	ncfname = os.path.basename(full_path_name)
	folder = os.path.dirname(full_path_name)
	name, ext = os.path.splitext(ncfname)
        
	sc['Molecule'] = mi
	SetSC()

	if int(gxsm.get('script-control')) < 1:
		break

	time.sleep(1)
	print('selecting M',mi)
	r=gxsm.marker_getobject_action(map_ch, 'RectangleM{:02d}'.format(mi),'GET-COORDS')
	print(r)
	if r != 'OK':
		break

	GetSC()
	STM_points = round(sc['STM_Range']/STM_dx)
	gxsm.set('PointsX', '{}'.format(STM_points)) # readjust points
	gxsm.set('PointsY', '{}'.format(STM_points))

	gxsm.set ('RangeX','{}'.format(sc['STM_Range'])) # Readjust range to make all of them the same size. 
	gxsm.set ('RangeY','{}'.format(sc['STM_Range']))
	time.sleep(1)

	if do_stm:
		do_stm_and_lock_on_center(mi)
		time.sleep(1)

	logging.info('*** Next Molecule #{:d} {} -- centering.'.format(mi, name))

	if int(gxsm.get('script-control')) < 1:
		break

	# Setup AFM Scan
	GetSC()
	AFM_points = round(sc['AFM_Range']/AFM_dx)
	gxsm.set('PointsX', '{}'.format(AFM_points)) # readjust points
	gxsm.set('PointsY', '{}'.format(AFM_points))

	gxsm.set ('RangeX','{}'.format(sc['AFM_Range'])) # Readjust range to make all of them the same size. 
	gxsm.set ('RangeY','{}'.format(sc['AFM_Range']))
	time.sleep(1)

	# only do reconfigure scan geom -- todo: do not save
	gxsm.startscan()
	time.sleep(3)
	gxsm.stopscan()
	# do STM orbital scans +2V/-1.5V or so?

	if do_afm:
		print('moving tip on top of molecule to measure Z at ref conditons for HR-AFM')

		# initial setpoint determinaion on this grid -- make better: assure on molecule!
		ds = 2.0
		c   = 0.0
		ref_xy_list = [ ]
		for i in np.arange(-1,2):
			for j in np.arange(-1,2):
				ref_xy_list.append([c+i*ds, c+j*ds])

		print('HR-AFM transitioning...')
		GetSC()
		z=init_force_map_ref_xy(0.02, level=sc['CZ_Level']*1e-3, ref_i=sc['I_ref']*1e-3, zoff = sc['Z_start'], xy_list=ref_xy_list)
		sc['Tip_Z_Ref'] = z
		SetSC()
		tipz =0
		logging.info('*** Init AFM mode, start at Tip_Z_Ref = {:.2f}A - tipz=0A'.format(z))
		z_ai_dir=0
		z_ai_step=0.3
		while int(gxsm.get('script-control')) >1 and tipz <= sc['Z_down'] and tipz < 2.0:
			logging.info('Starting AFM image')
			do_HR_AFM(mi, tipz)
			time.sleep(4)

			if tipz == 0:
				# set RectangleID/Label to McBSP Freq file name -- 1st of Z series
				full_original_name = gxsm.chfname(2).split()[0]
				ncfname = os.path.basename(full_original_name)
				bname, ext = os.path.splitext(ncfname)
				print(bname)
				### CUSTOM FOR TOIS FILE NAMEING SCHEME ###
				filenumber = bname[11:14]
				print('File Number: ', filenumber)
				r = gxsm.marker_getobject_action(map_ch, 'RectangleM{:02d}'.format(mi),'SET-LABEL-TO:'+filenumber)
				print(r)
				logging.info('*** AFM File: {} *** RectangleM{:02d} = #{}'.format(bname, mi, filenumber))
                                
			if do_AI_afm:
				result, ai_mask_img = ai_decide(afm_ch)
				ai_mask_to_gxsm_ch (ai_mask_img, 10)
				export_png(afm_ch, 'afm')
				if result == 'okay':
					logging.info('Z Adjust none')
					break  ## OK done
				elif result == 'far':
					if z_ai_dir < 0:
						z_ai_step = z_ai_step*0.5
						if z_ai_step < 0.1:
							logging.info('Z Adjust done. far, but been downwithin 10pm')
							break  ## OK done
						logging.info('Z Adjust revert {:.1f}A'.format(z_ai_step))
					tipz = tipz+z_ai_step	# down a step
					z_ai_dir=1
					logging.info('Z Adjust down')
				else:
					if z_ai_dir > 0:
						z_ai_step = z_ai_step*0.5
						if z_ai_step < 0.1:
							logging.info('Z Adjust done. close, but been up within 10pm')
							break ## OK done
						logging.info('Z Adjust revert {:.1f}A'.format(z_ai_step))
					tipz = tipz-z_ai_step	# up a step
					z_ai_dir=-1
					logging.info('Z Adjust up')
			else:
				tipz = tipz+0.3	# simple down steps as programmed
			logging.info('Z Adjust to Tip_Z_Ref = {:.2f}A - tipz={:.2f}A'.format(z, tipz))

		if int(gxsm.get('script-control')) >5:
				print('waiting for re run as long as sc>5')
				while int(gxsm.get('script-control')) > 5:
					time.sleep(0.5)
			
		logging.info('*** Exit AFM mode ***')
		exit_force_map(0.1, current=0.006)

logging.shutdown()




