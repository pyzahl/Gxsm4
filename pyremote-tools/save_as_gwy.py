import numpy as np
import time
import os
from datetime import datetime
from gwyfile.objects import GwyContainer, GwyDataField, GwySIUnit

ch = 0


# Create the top-level container
obj = GwyContainer()

full_original_name = gxsm.chfname(ch).split()[0]
print(full_original_name)

folder = os.path.dirname(full_original_name)
print(folder)

fname, ext = os.path.splitext(full_original_name)
print(fname)

# fetch dimensions, ...
dims = gxsm.get_dimensions(ch)
print ('Dims: ', dims) ## nx, ny, nt, nv

xunit = gxsm.get_scan_unit(ch,'x')
print (xunit)
yunit = gxsm.get_scan_unit(ch,'y')
print (yunit)
zunit = gxsm.get_scan_unit(ch,'z')
print (zunit)

geom = gxsm.get_geometry(ch) ## rx ,ry, rz, rv
if dims[2]>1 or dims[3]>1:
	tunit = gxsm.get_scan_unit(ch,'t')
	print (tunit)
	vunit = gxsm.get_scan_unit(ch,'v')
	print (vunit)
	print ('Geom: ', "{}{} x {}{} x {}{} x {}{} x {}{}".format(geom[0], xunit[1], geom[1], yunit[1], geom[2], zunit[1], geom[3], tunit[1], geom[4], vunit[1]))
else:
	print ('Geom: ', "{}{} x {}{}".format(geom[0], xunit[1], geom[1], yunit[1]))

## not needed, np objects are returned already scaled by 'dz" to unit.
#diffs= gxsm.get_differentials(ch)
#print ('Diffs: ', diffs) ## dx ,dy, dz, dv

data = gxsm.get_slice(ch, 0,0, 0, dims[1]) # ch, v, t, yi, yn

# Create a GwyDataField object from the data
data_field = GwyDataField(data*1e-10) ## Ang for Z/Topo onyl ** fix me, use zunit to scale conversion

# Set physical dimensions and units (e.g., 5 micrometers square in meters)
data_field.xreal = geom[0]*1e-10 # Ang to meters ** always in Ang -- so far
data_field.yreal = geom[1]*1e-10
data_field.si_unit_xy = GwySIUnit(unitstr="m")
data_field.si_unit_z = GwySIUnit(unitstr="m") # Units for the values (Z-axis) ** fix me, use zunit!

# Add the data field to the container with a title
obj['/0/data/title'] = 'Generated Noise Data'
obj['/0/data'] = data_field

# Save the container object to a Gwyddion file
obj.tofile(fname+'.gwy')
print('Save to gwy file: '+fname+'.gwy')
