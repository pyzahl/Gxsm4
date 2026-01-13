# Save/Load current GVP to/from JSON file
import json

filename = 'gvp_vector_program_Pulse.json'
gvp_vectors = []

load = True

#gxsm.get('dsp-gvp-du00')

if load:
	with open(filename, 'r') as file:
        	gvp_vectors = json.load(file)
	for v in gvp_vectors:
		for key in v:
			print (key, v[key])
			gxsm.set(key, str(v[key]))

else:

	for eckey in gxsm.list_refnames ():
		if eckey.startswith('dsp-gvp'):
			print ('{} \t=>\t {}'.format(eckey, gxsm.get(eckey))) 
			gvp_vectors.append ({eckey: gxsm.get(eckey)})
			
	#print (gvp_vectors)

	with open(filename, 'w') as file:
		json.dump(gvp_vectors, file)