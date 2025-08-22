import numpy as np

dfi=gxsm.rtquery ("f") # dFreq, I-avg, I-RMS!
print('dF, I: ', dfi)

mixer123=gxsm.rtquery ("M") # in Volts!!!
M=np.array(mixer123)

## for IN1..IN7 in Volts -- native use:
print('Mixer CH1,2,3 (Volts if IN0..7)', M)

## this is as values are expected analog mapped (our range 10.4V max and 16bit in 24.6bit format -- undone to revert to 32bit native, as for example Signal "Bias or Motor" are used for example
print('Mixer CH1,2,3 (Volts if internal DSP 32bit data)', M/256*1.04)  

mixer123=gxsm.rtquery ("m99") # update signal mapping table
for i in range(0,22):
	# "mNN" :              MonitorSignal[NN] -- Monitor Singals at index SigMon[NN]: [Scaled Signal in Unit, raw signal, scale, ret=unit-string] (NN = 00 ... 22), 99: reload signal mapping
	mon=gxsm.rtquery ("m{:02d}".format(i)) # as mapped via signal table
	print(i, mon)