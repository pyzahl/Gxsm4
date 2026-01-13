import numpy as np
import matplotlib.pyplot as plt
from scipy.signal import butter, freqz, ellip
import math
import time

# Setup for Cut Off
fc = 1   # rel to Lock-In Freq
attn = 40
ripple = 0.5


def LockInFreq():
	return float(gxsm.get("dsp-SPMC-LCK-FREQ"))

def dds_phaseinc (freq):
	fclk = 125e6
	return (1<<44)*freq/fclk

def LockInFreqN():
	freq = float(gxsm.get("dsp-SPMC-LCK-FREQ"))
	n2 = round (math.log2(dds_phaseinc (freq)))
	dph = 1<<n2
	fclk = 125e6
	return fclk*dph/(1<<44)

print ('Lock F N aligend: {:.2f} Hz' .format( LockInFreqN ()))
gxsm.set("dsp-SPMC-LCK-RF-FREQ", str(LockInFreqN ()))

# Setup for Cut Off at x * F-Lck
fc =2*fc

def Fs():
	freq = LockInFreq()
	print ('Lck Freq: {} Hz'.format(freq))
	n2 = round (math.log2(dds_phaseinc (freq)))
	lck_decimation_factor = (1 << (44 - 10 - n2)) - 1.
	return 125e6 / (lck_decimation_factor)  # pre-deciamtion to 1024 samples per lockin ref period

print ('Fs is ', Fs (), ' Hz')

def run_sosfilt(sos, x):
    n_samples = x.shape[0]
    n_sections = sos.shape[0]
    zx = np.zeros(n_sections)
    zy = np.zeros(n_sections)

    print (n_samples, n_sections)
    print (sos.shape)
    
    zi_slice = np.zeros((n_sections,2))
    for n in range(0,n_samples):
        x_cur=x[n]
        for s in range(n_sections):
            x_new            = sos[s, 0] * x_cur + zi_slice[s, 0]
            zi_slice[s, 0] = sos[s, 1] * x_cur - sos[s, 4] * x_new + zi_slice[s, 1]
            zi_slice[s, 1] = sos[s, 2] * x_cur - sos[s, 5] * x_new
            x_cur = x_new
        x[n]=x_cur
    return x

# Design a lowpass elliptic filter
# rp: Passband ripple in dB
# sa: Stopband attenuation in dB
def ellipt_filter(order=4, cutoff=0.2, sa=50, rp=0.5, filter_type='lowpass', fs=1.0):
    nyquist = 0.5 * fs  # Nyquist frequency
    normal_cutoff = cutoff / nyquist
    sos = ellip(order, rp, sa, cutoff, btype=filter_type, output='sos')
    sos = np.atleast_2d(sos)
    b ,a = ellip(order, rp, sa, cutoff, btype=filter_type, output='ba')
    return sos, b, a


# Design a Butterworth IIR filter
def butterworth_filter(order=2, cutoff=0.3, filter_type='low', fs=1.0):
    nyquist = 0.5 * fs  # Nyquist frequency
    normal_cutoff = cutoff / nyquist
    b, a = butter(order, normal_cutoff, btype=filter_type, analog=False)
    return b, a

# Compute and plot the frequency response
def plot_frequency_response(b, a, cutoff):
    w, h = freqz(b, a, worN=1024)
    plt.figure(figsize=(8, 4))
    plt.plot(w / np.pi, 20 * np.log10(abs(h)), 'b')
    plt.plot([cutoff, cutoff], [0, -20], 'r')
    plt.title('Frequency Response of the IIR Filter, low {} ({} Hz)'.format(cutoff, cutoff*Fs()))
    plt.xlabel('Normalized Frequency (×π rad/sample)')
    plt.ylabel('Magnitude (dB)')
    plt.grid()
    plt.show()

fc_norm = fc/Fs()

#b, a = butterworth_filter(order=1, cutoff=fc_norm)
#print (b.size)

sos, b, a = ellipt_filter(order=4, cutoff=fc_norm, sa=attn, rp=ripple)

print ('fCut_norm=', fc_norm, ' fc=', fc, ' Hz')
print (' b=',b,' a=',a)
print (' sos=',sos)


if 0: # AB type
	gxsm.set("dsp-SPMC-LCK-BQ-COEF-BA00", str(b[0]))
	gxsm.set("dsp-SPMC-LCK-BQ-COEF-BA01", str(b[1]))
	if b.size>2:
		gxsm.set("dsp-SPMC-LCK-BQ-COEF-BA02", str(b[2]))

	gxsm.set("dsp-SPMC-LCK-BQ-COEF-BA03", str(a[0]))
	gxsm.set("dsp-SPMC-LCK-BQ-COEF-BA04", str(a[1]))
	if a.size>2:
		gxsm.set("dsp-SPMC-LCK-BQ-COEF-BA05", str(a[2]))

if 1: # SOS ellipt cascaded BiQ type
	for s in range (0,2):
		for i in range (0,6):
			gxsm.set("dsp-SPMC-LCK-BQ{}-COEF-BA0{}".format(s+1,i), '0.1234')
			gxsm.set("dsp-SPMC-LCK-BQ{}-COEF-BA0{}".format(s+1,i), '0')
			gxsm.set("dsp-SPMC-LCK-BQ{}-COEF-BA0{}".format(s+1,i), str(sos[s][i]))

#plot_frequency_response(b, a, fc_norm)
