#!/usr/bin/python3

#python3 -m venv V
#V/bin/pip install gwyfile glob2 h5py numpy

''' 
    Converts the new (as of October 2025) GXSM netCDF-4 files 
    to a gwyddion data container file.

    Uses h5py to access the netCDF-4 files, which are effectively HDF5 files.

    Could also use the netCDF python API to open files 
    (would likely be backwards compatible to previous GXSM versions)

    TODO:
    - unit for z is 'V' for a topography image
    - GwySIUnit appears to not correctly handle 'nA' or 'nm'.
    - extend metadata by title, user etc. fields

    19.11.2025, Philipp Rahe (prahe@uos.de)
    25.11.2025, Percy Zahl (pzahl@bnl.gov) added batch list processing

'''

# %%%
import h5py
import numpy as np
import gwyfile
from gwyfile.objects import GwyContainer, GwyDataField, GwySIUnit

import glob2

folder = '20251117-Au111-rad'
base   = 'Au111-R1_'
input_files  = glob2.glob(folder+"/"+base+"*"+".nc")

########## Some helper functions #################

### if dataset has dtype='S1'
def read_string(ds):
    if not ds.dtype =='S1':
        return None
    s = ''
    for i in range(ds.shape[0]):
        s = s + ds.asstr()[i]
    return s


def list_attr_keys(ds):
    for k in ds.attrs.keys():
        try:
            print(f' {k}: {imgdata.attrs[k]}')
        except:
            print(f' {k}: [[not readable]]')
            pass


def get_attr_or_None(ds, name):
    if not name in ds.attrs:
        return None
    res = ds.attrs[name]
    return res


def convert(input_file):
    # open file
    f = h5py.File(input_file, 'r')

    # get image data
    imgdata = f['FloatField']
    # need to convert to numpy array with data type float64
    # (HDF5/GXSM uses float32)
    # Use squeeze to remove the first two dimensions (of size=1)
    dataarr = np.array(imgdata, dtype=np.float64).squeeze()

    # physical coordinates (for each pixel along the axis) in Angstrom
    dimx = f['dimx']
    # list_attr_keys(dimx)
    dimy = f['dimy']

    # total image sizes (in angstrom)
    rangex = f['rangex'][()]
    rangey = f['rangey'][()]
    # This should always return Ang:
    unitx = f['rangex'].attrs['var_unit'].decode('utf-8')
    unity = f['rangey'].attrs['var_unit'].decode('utf-8')

    # image offset (in angstrom)
    offsetx = f['offsetx'][()]
    offsety = f['offsety'][()]


    # scaling for the data
    # Note on units for 'z':
    # https://sourceforge.net/p/gwyddion/discussion/fileformats/thread/6c1ca39783/?limit=25
    dz = f['dz'][()]
    unitz = ''
    unitzfac = 1.0*dz
    uzattr = get_attr_or_None(f['FloatField'], 'unit')
    if uzattr:
        unitz = uzattr.decode('utf-8')
    else:
        uzattr = get_attr_or_None(f['FloatField'], 'unitSymbol')

    if uzattr:
        unitz = uzattr.decode('utf-8')
    else:
        uzattr = get_attr_or_None(f['FloatField'], 'var_unit')

    if uzattr:
        unitz = uzattr.decode('utf-8')
    else:
        uzattr = ''

    # GwySIUnit appears to not handle the prefix 'n' correctly
    if unitz.startswith('n') and len(unitz) >= 2:
        unitz = unitz[1:]
        unitzfac = 1e-9


    gwytitle = read_string(f['title'])

    meta = GwyContainer()
    # additional datasets to store as metadata
    # TODO: extend (currently limited by datatypes)
    metadatakeys = ['time', 'value']
    # add to metadata
    for k in f.keys():
        if k.startswith('JSON_'):
            meta[k] = str(f[k][()])
        if k in metadatakeys:
            # TODO: Need to handle different datatypes here
            meta[k] = str(f[k][()])



    ################# Write to gwy file
    nano = 1e-9
    obj = GwyContainer()
    obj['/0/data/title'] = gwytitle
    obj['/0/data'] = GwyDataField(data=dataarr*unitzfac, 
                                  xreal=rangex/10*nano, yreal=rangey/10*nano,
                                  xoff=offsetx/10*nano, yoff=offsety/10*nano,
                                  si_unit_xy=GwySIUnit(unitstr='m'), 
                                  si_unit_z=GwySIUnit(unitstr=unitz))
    obj['/0/meta'] = meta
    obj.tofile(f'{input_file}.gwy')



for f in input_files:
    print ('converting {}'.format(f))
    convert (f)
