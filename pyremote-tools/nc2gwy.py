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
import math
import gwyfile
from gwyfile.objects import GwyContainer, GwyDataField, GwySIUnit

import glob2

folder = '20251117-Au111-rad'
base   = 'Au111-'
input_files  = glob2.glob(folder+"/"+base+"*"+".nc")

## Gxsm Units

UTF8_ANGSTROEM ="\303\205"
UTF8_MU        ="\302\265"
UTF8_DEGREE    ="\302\260"

XsmUnitsTable = [
                #// Id (used in preferences), Units Symbol, Units Symbol (ps-Version), scale factor, precision1, precision2
                [ "AA", UTF8_ANGSTROEM,   "Ang",    1e0, ".1f", ".3f" ], #// UFT-8 Ang did not work // PS: "\305"
                [ "nm", "nm",  "nm",     10e0, ".1f", ".3f" ],
                [ "um", UTF8_MU+"m",  "um",     10e3, ".1f", ".3f" ], #// PS: "\265m"
                [ "mm", "mm",  "mm",     10e6, ".1f", ".3f" ],
                [ "BZ", "%BZ", "BZ",     1e0, ".1f", ".2f" ],
                [ "sec","\"",  "\"",     1e0, ".1f", ".2f" ],
                [ "V",  "V",   "V",      1e0, ".2f", ".3f" ],
                [ "mV", "mV",  "mV",     1e-3, ".2f", ".3f" ],
                [ "mV1", "mV",  "mV",    1e0, ".2f", ".3f" ],
                [ "V",  "*V", "V",       1e0, ".2f", ".3f" ],
                [ "*dV", "*dV","dV",     1e0, ".2f", ".3f" ],
                [ "*ddV", "*ddV","ddV",  1e0, ".2f", ".3f" ],
                [ "*V2", "V2", "V2",      1e0, ".2f", ".3f" ],
                [ "1",  " ",   " ",       1e0, ".3f", ".4f" ],
                [ "0",  " ",   " ",       1e0, ".3f", ".4f" ],
                [ "B",  "Bool",   "Bool", 1e0, ".3f", ".4f" ],
                [ "X",  "X",   "X",       1e0, ".3f", ".4f" ],
                [ "xV",  "xV",   "xV",    1e0, ".3f", ".4f" ],
                [ "deg", UTF8_DEGREE, "deg",       1e0, ".3f", ".4f" ],
                [ "Amp", "A",  "A",       1e9, "g", "g" ],
                [ "nA", "nA",  "nA",      1e0, ".2f", ".3f" ],
                [ "pA", "pA",  "pA",      1e-3, ".1f", ".2f" ],
                [ "nN", "nN",  "nN",      1e0, ".2f", ".3f" ],
                [ "Hz", "Hz",  "Hz",      1e0, ".2f", ".3f" ],
                [ "mHz", "mHz",  "mHz",   1e-3, ".2f", ".3f" ],
                [ "K",  "K",   "K",       1e0, ".2f", ".3f" ],
                [ "amu","amu", "amu",     1e0, ".1f", ".2f" ],
                [ "CPS","Cps", "Cps",     1e0, ".1f", ".2f" ],
                [ "CNT","CNT", "CNT",     1e0, ".1f", ".2f" ],
                [ "Int","Int", "Int",     1e0, ".1f", ".2f" ],
                [ "A/s",UTF8_ANGSTROEM+"/s", "A/s",     1e0, ".2f", ".3f" ],
                [ "Ang/V",UTF8_ANGSTROEM+"/V", "Ang/V", 1e0, ".3f", ".4f" ],
                [ "s","s", "s",           1e0, ".2f", ".3f" ],
                [ "ptsps","pts/s", "pts/s",  1e0, ".2f", ".3f" ], #// points / s
                [ "Vps","V/s", "V/s",  1e0, ".2f", ".3f" ],
                [ "ms","ms", "ms",        1e0, ".2f", ".3f" ],
                [ "us",UTF8_MU+"s", "us",   1e0, ".1f", ".2f" ],
                [ "Grad",UTF8_DEGREE, "deg", 1e0, ".2f", ".3f" ],
                [ "bool","B", "B", 1e0, ".0f", ".0f" ],
                [ "PC","%", "%", 1e0, ".0f", ".1f" ],
                [ "MB","MB", "MB", 1e0, ".1f", ".2f" ],
                [ "dB","dB", "dB", 1e0, ".2f", ".3f" ],
                [ "On/Off","On/Off", "On/Off", 1e0, "g", "g" ],
                [ "BC","BC", "BC", 1e0, "g", "g" ],
                [ "bin","b", "b", 1e0, "b", "b" ],
                [ "hex","h", "h", 1e0, "x", "x" ],
                [ "A", UTF8_ANGSTROEM,   "Ang",    1e0, ".1f", ".3f" ], #// fall back for some old def
]

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
    
    # use rangez for "Z" data unit assignment
    unitz_id  = get_attr_or_None(f['rangez'], 'unit').decode('utf-8')     ## contains Gxsm unit ID => see table above
    unitz_sym = get_attr_or_None(f['rangez'], 'var_unit').decode('utf-8') ## contains Gxsm unit Symbol, may be UTF special characters like  contain 'Ang" and 'Å' for length or '°' for degree (angle) or 'μ' prefix
    unitzfac  = dz
    unitz     = unitz_sym

    print ('NC-data Unit Z:', unitz_id, ', Symbol:', unitz_sym, ',  Scale by: ', unitzfac)

    # Translate a few custom units
    if unitz_id == 'AA': # Gxsm Unit ID for Angstroems
        unitz = 'm'
        unitzfac = 1e-10 * dz
    elif unitz_id == 'mV1': # Gxsm Unit ID for mVolt
        unitz = 'mV'
        unitzfac = dz
    elif unitz_id == 'sec': # Gxsm Unit ID for seconds
        unitz = 's'
        unitzfac = dz
    elif unitz_id == 'us': # Gxsm Unit ID for micro-seconds
        unitz = 's'
        unitzfac = 1e-6 * dz
    elif unitz_id == 'Amp': # Gxsm Unit ID for Ampere
        unitz = 'A'
        unitzfac = dz
    elif unitz_id == 'deg': # Gxsm Unit ID for Degrees (angle 360 base)
        unitz = 'rad'
        unitzfac = math.pi/180 * dz
  
    # GwySIUnit appears to not handle the prefix 'n' correctly
    if unitz_id.startswith('n') and len(unitz) >= 2:
        unitz = unitz[1:]
        unitzfac = unitzfac * 1e-9
      
    # make sure u-prefix, etc is correct
    if unitz_id.startswith('u') and len(unitz) >= 2:
        unitz = unitz[1:]
        unitzfac = unitzfac * 1e-6
      
        
    print ('=> NC-data Unit Z:', unitz, '  factor is: ', unitzfac)

        
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
