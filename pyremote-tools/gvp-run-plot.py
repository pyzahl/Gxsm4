import numpy as np
import matplotlib as mpl
mpl.use('Agg') # non interactive, no event loop

import matplotlib.pyplot as plt

# lil plotting helper
def plot_gvp(gvp, lab, u, how):
    fig=plt.figure(figsize=(8, 6))
    yl='Y'
    for y in how[1:]:
            plt.plot(gvp[how[0]], gvp[y], label=lab[y])
            yl='{}, {} in {}'.format(yl,lab[y],u[y])
    plt.title('GVP')
    plt.xlabel('{} in {}'.format(lab[how[0]], u[how[0]]))
    plt.ylabel(yl)
    plt.legend()
    plt.grid()
    #plt.show()
    plt.savefig('/tmp/gvp.png') # save to file, intyeractive may or may not work as of event loop clash

print ('#Events: ', gxsm.get_probe_event(0,-2)) # report
gvp, labs, units = gxsm.get_probe_event(0,-1)  # get last probe data set
#gxsm.get_probe_event(0,-99)  # clear all probe data from scan, else it accumulates until next scan start
np.save('/tmp/gvp.npy', gvp) 

print(gvp, labs, units)
plot_gvp(gvp, labs, units, [3,0,1,2])  ## assuming 3 or more sets -- customize!