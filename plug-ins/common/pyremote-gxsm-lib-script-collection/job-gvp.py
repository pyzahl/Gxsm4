#GXSM_USE_LIBRARY <gxsm3-lib-probe>

#print np.arange(-0.1,-1.0,-0.1)
#[-0.1 -0.2 -0.3 -0.4 -0.5 -0.6 -0.7 -0.8 -0.9]

tip = tip_control ()
probe = probe_control()

#	def make_grid_coords(self, spnx, stepx, spny, stepy, positions=1, position=[[0,0]], shuffle=False):

xy = tip.make_grid_coords (1.2, 0.2, 1.2, 0.2, 1, [[0.8,0.0]])
num = np.size(xy)/2

#print num
#print xy

#probe.run_iv_simple (xy, num, 1.8, 0.2)

probe.run_gvp_simple (xy, num)

#        def run_gvp_simple (self, coords, num, skip=0):
 