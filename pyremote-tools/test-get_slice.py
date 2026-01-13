import numpy as np

gxsm.load(0, '/home/pzahl/BNLBox/Data/Percy-P0/Exxon-Yunlong/20220412-Cu111-PP-TI/Cu111-PP-TI113-Xp-McBSP-Freq.nc')

print('GET SLICE 0,0:')
ms = gxsm.get_slice(0, 0,0, 0,1)
#print(ms)

for i in range(0,320):
	print(i)
	ms = gxsm.get_slice(0, 0,0, i,1) # ch, v, t, yi, yn
	print('Median: ', np.median(ms))
	print('Min: ', np.min(ms))
	print('Max: ', np.max(ms))
	print('Range: ', np.max(ms) - np.min(ms))
