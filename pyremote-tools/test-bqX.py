#!/usr/bin/python3
import numpy as np
import matplotlib.pyplot as plt
from scipy.signal import butter, freqz, ellip, iirnotch, tf2sos
import math
import time
import matplotlib as mpl
mpl.use('Agg')
from matplotlib import ticker
mpl.pyplot.close('all')

# Set Filter F-Cut for
# Elliptical filter 4th order
fc = 0.5*float(gxsm.get("dsp-SPMC-LCK-FREQ"))
stop_attn_db = 40
ripple_db=1

LCK_DEC = float(gxsm.get("dsp-LCK-DECII-MONITOR"))  # LockIn signal decimation

AD463FS = 125e6/59/LCK_DEC

print ('DECII#', LCK_DEC, 'ADC-Fs', AD463FS, 'Hz', ' -> FS', AD463FS/LCK_DEC, ' Hz')

##Configure Lock-In: FNorm: 529.661 DDS Freq= 2000 Hz DECII2=1 DCF_IIR2=11 Mode=0 *** Gains: gin: 1, gout: 0
##Configure DC-FILTER: dc=-0.0142334, dc-tau=1 s

#GVP: 0.02s 20000
#GVP: 0.02s 20000

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
    plt.title('VPDATA')
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
	fss = AD463FS
	print ('Lck Freq: {} Hz -- Lck internal subsampling: {} Hz'.format(freq, fss))
	return fss

print ('Fs LockIn is ', Fs (), ' Hz')

def run_sosfilt(sos, x):
    n_samples = x.shape[0]
    n_sections = sos.shape[0]
    zx = np.zeros(n_sections)
    zy = np.zeros(n_sections)

    #print (n_samples, n_sections)
    #print (sos.shape)

    y = np.zeros(n_samples)

    ys = np.zeros((n_sections,n_samples))
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
            ys[s, n] = x_new
            z0[s,n] = zi_slice[s, 0]
            z1[s,n] = zi_slice[s, 1]
        y[n]=x_cur
    return y, z0, z1, ys


def run_sosfilt_Q24(sos, x):
    sens=1
    QS = 1<<24  #24
    QC = 1<<28 	#28
    QSC = float(QS) * QC
    n_samples = x.shape[0]
    n_sections = sos.shape[0]
    zx = np.zeros(n_sections)
    zy = np.zeros(n_sections)

    ys = np.zeros((n_sections,n_samples))
    z0 = np.zeros((n_sections,n_samples))
    z1 = np.zeros((n_sections,n_samples))

    #print (n_samples, n_sections)
    #print (sos.shape)

    sos = sos*QC
    sos = sos.astype(np.int64)

    print ('SOS_QS:', sos)
    
    xi = (x*QS*sens).astype(np.int64)

    print ('X', xi[0:20])
    
    y = np.zeros(n_samples, type(np.int64))
    
    zi_slice = np.zeros((n_sections,2), type(np.int64))
    for n in range(0,n_samples):
        x_cur=xi[n]
        for s in range(n_sections):
            x_new          = int(sos[s, 0] * x_cur                     + zi_slice[s, 0]) / QC
            zi_slice[s, 0] =     sos[s, 1] * x_cur - sos[s, 4] * x_new + zi_slice[s, 1]
            zi_slice[s, 1] =     sos[s, 2] * x_cur - sos[s, 5] * x_new
            x_cur = x_new
            ys[s, n] = x_new
            z0[s,n] = zi_slice[s, 0]
            z1[s,n] = zi_slice[s, 1]
        y[n]=x_cur
        #y[n]=ys[0,n]

    print ('Y:', y[0:20], ' Yf:', y.astype(float)[0:20]/QS/sens)
    return y.astype(float)/QS/sens, z0.astype(float)/QSC/sens, z1.astype(float)/QSC/sens, ys.astype(float)/QSC/sens



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

def notch_filter(w0, Q, fs):
    b, a = iirnotch(w0, Q, fs=fs)
    sos = tf2sos(b, a)
    print ('a,b: ', a, b)
    print ('sos: ', sos)
    return sos, b, a

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
    plt.savefig('/tmp/fresponse.png')
    plt.show()

# calc nomralize sampling freq
fc_norm = fc/Fs()

#b, a = butterworth_filter(order=1, cutoff=fc_norm)

sos, b, a = ellipt_filter(order=4, cutoff=fc_norm, sa=stop_attn_db, rp=ripple_db)

#sos, b, a = notch_filter(2000, 30, Fs())

print ('*****************************************')
print ('fCut_norm=', fc_norm, ' fc=', fc, ' Hz',  ' FS:', Fs())
print ('*****************************************')
print (' b=',b,' a=',a)
#print (' sos=',sos)

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
			
		#print ('SOS Bq [{:d}] Stage {}:\n'.format (cQ, s+1), bqv + bqp)

plot_frequency_response(b, a, fc_norm)

if 1:

	print ('Getting last vpdata set from master scan DATA ********')
	columns, labels, units, xy = gxsm.get_probe_event(0,-1)  # get last
	vpdata  = dict (zip (labels, columns))
	vpunits = dict (zip (labels, units))

	#vpdata['08-LockIn-Mag'][0] = 0
	#vpdata['14-LockIn-Mag-pass'][0] = 0

	print ('XY:', xy)
	print ('vpunits:', vpunits)

	print(vpdata['08-LockIn-Mag'][0:20])
	print(vpdata['14-LockIn-Mag-pass'][0:20])
	print(vpdata['08-LockIn-Mag'][-20:])
	print(vpdata['14-LockIn-Mag-pass'][-20:])

	vpdata['Time-Mon'][-1] = vpdata['Time-Mon'][-3]
	vpdata['Time-Mon'][-2] = vpdata['Time-Mon'][-3]

	print(vpdata['Time-Mon'][0:20])
	print(vpdata['Time-Mon'][-20:])



	FS_data = 1000/(vpdata['Time-Mon'][101]-vpdata['Time-Mon'][100])


	fc_norm = fc/FS_data
	print ('*** DATA ********************************')
	print ('FS data:', FS_data, ' Hz   Fc_norm:', fc_norm, ' actual sample dt=', vpdata['Time-Mon'][101]-vpdata['Time-Mon'][100],' ms')
	print ('*****************************************')
	sos, b, a = ellipt_filter(order=4, cutoff=fc_norm, sa=stop_attn_db, rp=ripple_db)


	#filteredS = sosfilt(sos, vpdata['14-LockIn-Mag-pass'])
	
	# Floatingpoint
	fsigf, zf0, zf1, ysf = run_sosfilt(sos, vpdata['14-LockIn-Mag-pass'])

	# Integer Q
	fsigQ, z0, z1, ys = run_sosfilt_Q24(sos, vpdata['14-LockIn-Mag-pass'])
	#print ('SOS:', fsig)

	plt.figure(figsize=(12, 4))
	plt.title('Filter Sim and Data Fc {} (Normed: {} Hz)'.format(fc, fc_norm))
	plt.xlabel('time in ms')
	plt.ylabel('signal in V')

	plt.plot (vpdata['Time-Mon'], vpdata['14-LockIn-Mag-pass'], alpha=0.3, label='Mag-pass')
	plt.plot (vpdata['Time-Mon'], vpdata['08-LockIn-Mag'], alpha=0.3, label='Mag-BQ')
	if 1: # Floating Point Calc
		plt.plot (vpdata['Time-Mon'], fsigf, label='Mag-SOS')
		plt.plot (vpdata['Time-Mon'], ysf[0], label='Mag-SOSs0')
		plt.plot (vpdata['Time-Mon'], zf0[0], label='fZ0S0')
		plt.plot (vpdata['Time-Mon'], zf1[0], label='fZ1S0')
		plt.plot (vpdata['Time-Mon'], zf0[1], label='fZ0S1')
		plt.plot (vpdata['Time-Mon'], zf1[1], label='fZ1S1')
	if 1: # Int Calc
		plt.plot (vpdata['Time-Mon'], fsigQ, alpha=0.4, linewidth=10, label='Mag-SOS-Q')
		plt.plot (vpdata['Time-Mon'], ys[0], label='Mag-SOSs0-Q')
		plt.plot (vpdata['Time-Mon'], z0[0], label='Z0S0')
		plt.plot (vpdata['Time-Mon'], z1[0], label='Z1S0')
		plt.plot (vpdata['Time-Mon'], z0[1], label='Z0S1')
		plt.plot (vpdata['Time-Mon'], z1[1], label='Z1S1')
	#plt.ylim (0.0, 1.)
	#plt.xlim (0.0, 5.)
	plt.legend()
	plt.grid()
	#plt.show()
	plt.savefig('/tmp/filter.png')

