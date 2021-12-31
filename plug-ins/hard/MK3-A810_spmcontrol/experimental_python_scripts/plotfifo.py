#!/usr/bin/env python

import numpy as np
import matplotlib.pyplot as plt

#void probe_append_header_and_positionvector (){ // size: 14
#	// Section header: [SRCS, N, TIME]_32 :: 6  LONG
#       // Section start situation vector [X Y Phase_Offset, XYZ_Scan, Upos(bias), 0xffff]_16 :: 8  WORD
#}

#void store_probe_data_srcs (){
#       0x01|02|10|20|40|80|100|200|400|800 ==> Zmon, Umon, INproc0..3 IN4..7 as WORDS
#       0x00008 LockIn_0 as LONG
#       0x01000 LockIn_1st as LONG
#       0x02000 src_input[0] as LONG
#       0x04000 src_input[1] as LONG
#       0x08000 src_input[2] as LONG
#       0x00004 src_input[3] as LONG
#}

#...

vpfifodata = np.load ("mk3_vpfifo_dump.npy")

i=0
for q in vpfifodata:
    print i,q&0x0000ffff
    i=i+1
    print i,(q&0xffff0000)>> 16
    i=i+1


x = np.arange (1.*vpfifodata.size)
#y1 = np.zeros (vpfifodata.size)
#y2 = np.zeros (vpfifodata.size)

fig, (ax0, ax1) = plt.subplots(nrows=2, sharex=True)

ax0.set_title ('VP DATA DUMP')
ax0.plot (x, vpfifodata)
ax0.grid (True)

ax1.set_title ('VP CH1')

plt.show ()



