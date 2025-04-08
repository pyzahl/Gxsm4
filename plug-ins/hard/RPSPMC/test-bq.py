import numpy as np
import matplotlib.pyplot as plt
from scipy.signal import butter, freqz
import math

def dds_phaseinc (freq):
	fclk = 125e6
	return (1<<44)*freq/fclk

def Fs():
	freq = float(gxsm.get("dsp-SPMC-LCK-FREQ"))
	n2 = round (math.log2(dds_phaseinc (freq)))
	lck_decimation_factor = (1 << (44 - 10 - n2)) - 1.
	print ('Flck is ', freq, ' Hz', 'lck_dec * 16 is ', lck_decimation_factor*16 )
	return 125e6 /(lck_decimation_factor*16)

print ('Fs is ', Fs (), ' Hz')

# Design a Butterworth IIR filter
def butterworth_filter(order=2, cutoff=0.3, filter_type='low', fs=1.0):
    nyquist = 0.5 * fs  # Nyquist frequency
    normal_cutoff = cutoff / nyquist
    b, a = butter(order, normal_cutoff, btype=filter_type, analog=False)
    return b, a

def ellips_filter(order=2, cutoff=0.3, filter_type='low', fs=1.0):
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
    plt.title('Frequency Response of the IIR Filter, low {}'.format(cutoff))
    plt.xlabel('Normalized Frequency (×π rad/sample)')
    plt.ylabel('Magnitude (dB)')
    plt.grid()
    plt.show()

# Example usage
fc = 18000
fc_norm = fc/Fs()
#b, a = butterworth_filter(order=1, cutoff=fc_norm)
b, a = ellipse_filter(order=1, cutoff=fc_norm)
print ('fCut_norm=', fc_norm, ' b=',b,' a=',a)
print (b.size)

# clear
cv = 0.00001
gxsm.set("dsp-SPMC-LCK-BQ-COEF-BA00", str(cv))
gxsm.set("dsp-SPMC-LCK-BQ-COEF-BA01", str(cv))
gxsm.set("dsp-SPMC-LCK-BQ-COEF-BA02", str(cv))
gxsm.set("dsp-SPMC-LCK-BQ-COEF-BA03", str(cv))
gxsm.set("dsp-SPMC-LCK-BQ-COEF-BA04", str(cv))
gxsm.set("dsp-SPMC-LCK-BQ-COEF-BA05", str(cv))
cv = 0.0
gxsm.set("dsp-SPMC-LCK-BQ-COEF-BA00", str(cv))
gxsm.set("dsp-SPMC-LCK-BQ-COEF-BA01", str(cv))
gxsm.set("dsp-SPMC-LCK-BQ-COEF-BA02", str(cv))
gxsm.set("dsp-SPMC-LCK-BQ-COEF-BA03", str(cv))
gxsm.set("dsp-SPMC-LCK-BQ-COEF-BA04", str(cv))
gxsm.set("dsp-SPMC-LCK-BQ-COEF-BA05", str(cv))


gxsm.set("dsp-SPMC-LCK-BQ-COEF-BA00", str(b[0]))
gxsm.set("dsp-SPMC-LCK-BQ-COEF-BA01", str(b[1]))
if b.size>2:
	gxsm.set("dsp-SPMC-LCK-BQ-COEF-BA02", str(b[2]))

gxsm.set("dsp-SPMC-LCK-BQ-COEF-BA03", str(a[0]))
gxsm.set("dsp-SPMC-LCK-BQ-COEF-BA04", str(a[1]))
if a.size>2:
	gxsm.set("dsp-SPMC-LCK-BQ-COEF-BA05", str(a[2]))


#plot_frequency_response(b, a, fc_norm)
