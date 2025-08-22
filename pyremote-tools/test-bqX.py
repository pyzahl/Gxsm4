#!/usr/bin/python3
import numpy as np
import matplotlib.pyplot as plt
from scipy.signal import butter, freqz, ellip
import math
import time
import matplotlib as mpl
mpl.use('Agg')
from matplotlib import ticker
mpl.pyplot.close('all')

# Set Filter F-Cut to
fc = 30 # Hz

# Ellip filter characteristics
stop_attn_db = 50
ripple_db=1


# ******** Lck Settings internal
FS = 1
DECII=7

def plot_vpdata(cols=[1,2,4]):
    plt.close ('all')
    plt.figure(figsize=(8, 4))
    yl = ''
    for c in cols[1:]:
            plt.plot(columns[cols[0]], columns[c], label=labels[c])
            if yl != '':
                    yl = yl + ', ' + labels[c]
            else:
                    yl = labels[c]
    plt.title('VPDATA  FS={:.2f} Hz'.format(FS))
    plt.xlabel(labels[cols[0]])
    plt.ylabel(yl)
    plt.legend()
    plt.grid()
    plt.show()
    plt.savefig('/tmp/gvp.png')
    plt.show()


def dds_phaseinc (freq):
	fclk = 125e6
	return (1<<44)*freq/fclk

def Fs():
	freq = float(gxsm.get("dsp-SPMC-LCK-FREQ"))
	print ('Lck Freq: {} Hz'.format(freq))
	n2 = round (math.log2(dds_phaseinc (freq)))
	lck_decimation_factor = (1 << (44 - DECII - n2)) - 1.
	return 125e6 / (lck_decimation_factor)  # pre-deciamtion to 1024 samples per lockin ref period

print ('Fs LockIn is ', Fs (), ' Hz')

def run_sosfilt(sos, x):
    n_samples = x.shape[0]
    n_sections = sos.shape[0]
    zx = np.zeros(n_sections)
    zy = np.zeros(n_sections)

    print (n_samples, n_sections)
    print (sos.shape)

    y = np.zeros(n_samples)

    z0 = np.zeros((n_sections,n_samples))
    z1 = np.zeros((n_sections,n_samples))
    
    zi_slice = np.zeros((n_sections,2))
    for n in range(0,n_samples):
        x_cur=x[n]
        for s in range(n_sections):
            x_new          = sos[s, 0] * x_cur                     + zi_slice[s, 0]
            zi_slice[s, 0] = sos[s, 1] * x_cur - sos[s, 4] * x_new + zi_slice[s, 1]
            zi_slice[s, 1] = sos[s, 2] * x_cur - sos[s, 5] * x_new
            x_cur = x_new
            z0[s,n] = zi_slice[s, 0]
            z1[s,n] = zi_slice[s, 1]
        y[n]=x_cur
    return y, z0, z1


def run_sosfilt_Q24(sos, x):
    Q24 = 1<<24
    Q28 = 1<<28
    n_samples = x.shape[0]
    n_sections = sos.shape[0]
    zx = np.zeros(n_sections)
    zy = np.zeros(n_sections)

    print (n_samples, n_sections)
    print (sos.shape)

    sos = sos*Q28
    sos = sos.astype(np.int64)

    print ('SOS_Q28:', sos)
    
    xi = (x*Q24).astype(np.int64)

    print ('xQ24:', xi)
    
    y = np.zeros(n_samples, type(np.int64))
    
    zi_slice = np.zeros((n_sections,2), type(np.int64))
    for n in range(0,n_samples):
        x_cur=xi[n]
        for s in range(n_sections):
            x_new          = int(sos[s, 0] * x_cur                  + zi_slice[s, 0]) >> 28
            zi_slice[s, 0] =  sos[s, 1] * x_cur - sos[s, 4] * x_new + zi_slice[s, 1]
            zi_slice[s, 1] =  sos[s, 2] * x_cur - sos[s, 5] * x_new
            x_cur = x_new
        y[n]=x_cur

    print (y)
    return y.astype(float)/Q24



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
    plt.savefig('/tmp/filter.png')
    plt.show()

# calc nomralize sampling freq
fc_norm = fc/Fs()

#b, a = butterworth_filter(order=1, cutoff=fc_norm)

sos, b, a = ellipt_filter(order=4, cutoff=fc_norm, sa=stop_attn_db, rp=ripple_db)

print ('fCut_norm=', fc_norm, ' fc=', fc, ' Hz')
print (' b=',b,' a=',a)
print (' sos=',sos)

bqv = "vector = {32'd0,  32'd0,  32'd0,  32'd0,  32'd0,  32'd0,  32'd0,  32'd0,  32'd0,  32'd0  "

if 0: # AB type
	gxsm.set("dsp-SPMC-LCK-BQ-COEF-BA00", str(b[0]))
	gxsm.set("dsp-SPMC-LCK-BQ-COEF-BA01", str(b[1]))
	if b.size>2:
		gxsm.set("dsp-SPMC-LCK-BQ-COEF-BA02", str(b[2]))

	gxsm.set("dsp-SPMC-LCK-BQ-COEF-BA03", str(a[0]))
	gxsm.set("dsp-SPMC-LCK-BQ-COEF-BA04", str(a[1]))
	if a.size>2:
		gxsm.set("dsp-SPMC-LCK-BQ-COEF-BA05", str(a[2]))

cQ = (1<<28)-1
if 1: # SOS ellipt cascaded BiQ type
	for s in range (0,2):
		bqp = "};"
		for i in range (0,6):
			gxsm.set("dsp-SPMC-LCK-BQ{}-COEF-BA0{}".format(s+1,i), '0.1234')
			gxsm.set("dsp-SPMC-LCK-BQ{}-COEF-BA0{}".format(s+1,i), '0')
			gxsm.set("dsp-SPMC-LCK-BQ{}-COEF-BA0{}".format(s+1,i), str(sos[s][i]))
			bqp = ", {}32'd{:d} {}".format('-' if sos[s][i] < 0 else ' ', int(round(cQ*abs(sos[s][i]))), bqp)
			#bqp = ", {}32'd{:d} {}".format('-' if sos[s][i] < 0 else ' ', int(round(cQ*abs(sos[s][i])))-cQ, bqp) ## CHECK
			
		print ('SOS Bq [{:d}] Stage {}: '.format (cQ, s+1), bqv + bqp)

#plot_frequency_response(b, a, fc_norm)

columns, labels, units = gxsm.get_probe_event(0,-1)  # get last

print (labels)
plot_vpdata([4,1,2,3])
#plot_vpdata([4,0,1])

FS_data = 1000/(columns[4][101]-columns[4][100])

print ('FS data:', FS_data)

fc_norm = fc/FS_data
sos, b, a = ellipt_filter(order=4, cutoff=fc_norm, sa=stop_attn_db, rp=ripple_db)


#filteredS = sosfilt(sos, columns[4])
fsig, z0, z1 = run_sosfilt(sos, columns[1])
print ('SOS:', fsig)

print (columns, labels)

plt.plot(columns[4], fsig, label='SOS')
#plt.plot(columns[4], z0[0], label='SOSz00')
#plt.plot(columns[4], z1[0], label='SOSz01')
#plt.plot(columns[4], z0[1], label='SOSz10')
#plt.plot(columns[4], z1[1], label='SOSz11')
plt.legend()
plt.grid()
plt.show()
plt.savefig('/tmp/filter.png')


#columns[5] = run_sosfilt_Q24(sos, columns[4])
#labels[5]  = 'SOS[4] Q24'

#plot_vpdata([5,1,2,3,4,6])

