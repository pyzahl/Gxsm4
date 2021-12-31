#!/usr/bin/env python

import numpy as np
import matplotlib.pyplot as plt

fit     = np.load ("mk3_tune_Fit.npy")
freqs   = np.load ("mk3_tune_Freq.npy")
amp2f   = np.load ("mk3_tune_ResAmp2F.npy")
amp1f   = np.load ("mk3_tune_ResAmp.npy")
phase2f = np.load ("mk3_tune_ResPhase2F.npy")
phase1f = np.load ("mk3_tune_ResPhase.npy")

phase1f = np.where (phase1f < -180,phase1f + 360, phase1f)

fig, (ax0, ax1) = plt.subplots(nrows=2, sharex=True)

ax0.set_title ('Resonator Phase')
ax0.plot (freqs, phase1f)
ax0.plot (freqs, phase2f)
ax0.grid (True)

ax1.set_title('Resonator Amplitude')
ax1.plot (freqs, amp1f)
#ax1.plot (freqs, fit)
ax1.plot (freqs, amp2f)
ax1.set_yscale('log')
ax1.grid (True)

plt.show ()



