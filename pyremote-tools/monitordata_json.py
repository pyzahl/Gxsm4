#!/usr/bin/python3

import numpy as np
import matplotlib.pyplot as plt
import math
import time
from matplotlib import ticker
import json

plt.figure(figsize=(8, 4))

for i in range(0,220,2):
    with open('RPSPM-RINGBUFFER-DATA_{:04d}.json'.format(i), 'r') as file:
        dict = json.load(file)

    #print (dict['signals'])

    t = np.array(dict['signals']['SIGNAL_MON_FIFO_T']['value'])
    x = np.array(dict['signals']['SIGNAL_MON_FIFO_X']['value'])
    y = np.array(dict['signals']['SIGNAL_MON_FIFO_Y']['value'])
    in3 = np.array(dict['signals']['SIGNAL_MON_FIFO_IN3']['value'])

    #print (t)

    plt.plot(t, x, 'x')
    plt.plot(t, y, 'x')
    plt.plot(t, in3, 'x')

plt.title('MONITORS')
plt.xlabel('time in sec')
plt.ylabel('Y')
plt.legend()
plt.grid()
plt.show()
