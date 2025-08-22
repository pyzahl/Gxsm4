# Save/Load current GVP to/from JSON file
import json
import time
import sys

filename = 'gvp_vector_program_GXSM4_backup.json'
gvp_vectors = []

load = False

#gxsm.get('dsp-gvp-du00')

vp_stores_list = ['VPA', 'VPB', 'VPC', 'VPD', 'VPE', 'VPF', 'VPG', 'VPH', 'VPI', 'VPJ', 'V0', ]

if load:

	for store in vp_stores_list:
		filename = 'gxsm4-gvp-program-'+store+'.json'

		with open(filename, 'r') as file:
			gvp_vectors = json.load(file)
		for v in gvp_vectors:
			for key in v:
				print (key, v[key])
				gxsm.set(key, str(v[key]))

		gxsm.action('DSP_VP_STO_'+store)
		time.sleep(2)

else:

	for store in vp_stores_list:
		gvp_vectors = []
		print ('RCL:', store)
		gxsm.action('DSP_VP_RCL_'+store)
		time.sleep(2)
		for eckey in gxsm.list_refnames ():
			if eckey.startswith('dsp-gvp'):
				print ('{} \t=>\t {}'.format(eckey, gxsm.get(eckey))) 
				gvp_vectors.append ({eckey: gxsm.get(eckey)})
			
		#print (gvp_vectors)

		filename = 'gxsm4-gvp-program-'+store+'.json'
		print (filename)
		with open(filename, 'w') as file:
			json.dump(gvp_vectors, file)