#!/usr/bin/env python

import pygtk
pygtk.require('2.0')

import gobject, gtk
import os               # use os because python IO is bugy
import time
import fcntl

#import GtkExtra
import struct
import array

import math
from pylab import *

import numpy as np
import matplotlib.pyplot as plt

Q=32767.
Q16=2.*Q
QV=3276.7
ACamp = 0.04*QV
Bias  = 1.0*QV

SLEN=128
SMSK=127
ph = arange(0.0, math.pi*2, 2*math.pi/SLEN)
S  = Q*sin(ph)

N=720
ph = arange(0., 360., 360./N)
dph = 1.*SLEN/N

Nave=7
ILN=8
L0  = zeros(N)
L1  = zeros(N)
Y   = zeros(N)
L0s = 0
K1s = zeros(ILN) 

CC = 0
data_sum=0
modi=0
aveCnt = 0
ix = -(ILN+2)
PhA=0
k=0
for i in range(0,SLEN*N*Nave+2*ILN):
    Uref = S[int(modi + CC + PhA)&SMSK]  # reference signal with phase
    Uin  = Bias + ACamp*S[modi]/Q16      # simulated test signal

    L0s = L0s + Uin             # just adding up Uin over period(s)
    K1s[k] = K1s[k] + Uref*Uin  # Korrelation summing of exactly #aveCNT full periods => storing # ILN set of period-korrel-sums in ring buffer for fast averaging

    if aveCnt >= Nave: # done with # periods?
        aveCnt=0
        if ix >= 0:  # just a initial startup delay, start with ix<0! to fill ILN buffers
            L1[ix] = K1s[0]
            for i in range(1, ILN):  # in addition to Nave, always summing korrelation data in a moving window of ILN (=8) groups of (#Nave) periods group
                L1[ix] = L1[ix]+K1s[i]
                L0[ix] = L0s

        ix = ix+1
        k = k+1 # manage ILN counter k
        if k >= ILN:
            k=0
        L0s = 0
        K1s[k] = 0

        PhA = PhA + dph # manage phase here for testing (externally done on DSP, normally fixed)
        # test code ----
        if ix >= 0:
            Y[ix] = int(modi + CC + PhA)&SMSK    # put any debug data into Y.... -- testing only

    modi=modi+1 # manage mode counter
    if modi >= SLEN:
        modi=0
        aveCnt = aveCnt+1

# pick a plot...
plt.plot(ph, Y)
#plt.plot(ph, L0)
#plt.plot(ph, L1)
plt.title('LockIn Phase Adjust Test')
plt.xlabel('phase (deg)')
plt.ylabel('voltage (mV)')
plt.grid(1)
plt.show()
