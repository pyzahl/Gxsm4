#!/usr/bin/python
import numpy
import math

Q31 = 0x7FFFFFFF # ((1<<31)-1);
Q32 = 0xFFFFFFFF # ((1<<32)-1);
Q44 = 1<<44

def adjust (hz):
    fclk = 125e6
    dphi = 2.*math.pi*(hz/fclk)

    #// unsigned int n2 = round (log2(dds_phaseinc (freq)));
    n2 = int(round (math.log2(round(Q44*hz/fclk))))
    #// unsigned long long phase_inc = 1LL << n2;
    phase_inc = 1 << n2
    #// double fact = dds_phaseinc_to_freq (phase_inc);
    m = Q44/phase_inc
    fhz = fclk/(Q44/phase_inc)
    bufN2 = 10
    print ('')
    print ('Frq-DDS-Sync : ', fhz, ' Hz  off=', (fhz-hz), ' Hz')
    print (' M-DDS-Sync  : ', m)
    print (' N2-DDS-Sync : ', n2)

    if (44-n2) > bufN2:
        print (' DECII2      : ', 44-n2-bufN2)
        print (' DECII       : ', (1 << (44-n2-bufN2))) # - 1)
        print (' DDSN-1      : ', (1 << 10) - 1)
    else:
        print (' DECII2      : ', 0)
        print (' DECII       : ', 0)
        print (' DDSN-1      : ', (1 << n2) - 1)
        
    print ('DECII * DDSN : ', ((1 << (44-n2-bufN2)) *  (1 << 10))  )

    hz = fhz
    
    dphi = 2.*math.pi*(hz/fclk)
    dRe = int (Q31 * math.cos (dphi))
    dIm = int (Q31 * math.sin (dphi))

    print ('fclk   : ', fclk, ' Hz')
    print ('Frq-DDS: ', hz, ' Hz')
    print ('dphi = ', dphi, ' rad')
    print ('dFreQ44 = ', fclk/Q44*(2*math.pi), ' rad, ', fclk*1000/Q44*360, ' mdeg')
    print ('dphiQ44 = ', round(Q44*dphi/(2*math.pi)), '; // @Q44 phase')
    print ('ddsn2   = ', n2, ';')
    print ('deltasRe = ', dRe, '; // SDB')
    print ('deltasIm = ', dIm, '; // SDB')
    
    return (dRe, dIm)

print (adjust (1000))
