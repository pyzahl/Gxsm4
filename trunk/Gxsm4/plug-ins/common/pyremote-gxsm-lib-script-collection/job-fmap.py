#GXSM_USE_LIBRARY <gxsm3-lib-scan>
#* GXSM_USE_LIBRARY <gxsm3-lib-probe>

tip = tip_control ()
scan = scan_control()

#   def run_force_map(self, zi,zdown=-2.0,bias=0.1,dz=-0.1, level=0.22, allrectss=False,speed=6,ffr=5):
scan.run_force_map(15.2, -0.8, 0.02, -0.1, 0.55, False,6,5)


# probe = probe_control()

#xy = tip.make_grid_coords (20.0, 0.2, 10.0, 2.0, 1, [[0,17]])
#num = np.size(xy)/2
#probe.run_iv_simple (xy, num, 1.8, 0.2)