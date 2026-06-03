# Save/Load current GVP to/from JSON file
import json

filename = 'gvp_vector_program_Pulse.json'
gvp_vectors = []

load = True

#gxsm.get('dsp-gvp-du00')
#print(gxsm.action('GETCHECK-dsp-gvp-FB00'))

if load:
        with open(filename, 'r') as file:
                gvp_vectors = json.load(file)
        for v in gvp_vectors:
                for key in v:
                        print (key, v[key])
                        if key.startswith('GETCHECK-dsp-gvp'):
                                if str(v[key]) == 'TRUE':
                                        gxsm.action(key[3:])
                                else:
                                        gxsm.action('UN'+key[3:])
                        else:
                                gxsm.set(key, str(v[key]))

else:

        # Read all vector comonents
        for eckey in gxsm.list_refnames ():
                if eckey.startswith('dsp-gvp'):
                        print ('{} \t=>\t {}'.format(eckey, gxsm.get(eckey))) 
                        gvp_vectors.append ({eckey: gxsm.get(eckey)})
        # Read all vector check options (FB = Feedback/Control on/off in particular) results is a "TRUE" or "FALSE" string
        for eckey in gxsm.list_actions ():
                if eckey.startswith('GETCHECK-dsp-gvp'):
                        print ('{} \t=>\t {}'.format(eckey, gxsm.action(eckey))) 
                        gvp_vectors.append ({eckey: gxsm.action(eckey)})

        #print (gvp_vectors)

        with open(filename, 'w') as file:
                json.dump(gvp_vectors, file)
