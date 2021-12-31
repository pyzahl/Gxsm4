#!/usr/bin/env python3

## GXSM NetCDF San Image Viwer and Tag Tool with GXSM remote inspection and control socket client functionality
## https://keras.io/getting-started/faq/#how-can-i-run-keras-on-gpu
## nccopy -k nc4 testV3.nc testV4.nc
## F**edUp libs:  percy@ltncafm:~/SVN/Gxsm-3.0-spmc$ exec env LD_LIBRARY_PATH=/home/percy/anaconda3/pkgs/cudatoolkit-10.0.130-0/lib  /usr/bin/python3.7 ./gxsm-scan-image-viewer-sok-client.py 
## https://cs.stanford.edu/people/karpathy/rcnn/
##https://www.kaggle.com/kmader/keras-rcnn-based-overview-wip#Overview

from __future__ import absolute_import, division, print_function, unicode_literals

# Importing the Keras libraries and packages
#import datetime
import glob2

import tensorflow as tf
from tensorflow.keras import backend as K
from tensorflow.keras.models import Sequential, load_model, model_from_json 
from tensorflow.keras.optimizers import RMSprop
from tensorflow.keras.layers import LSTM, Dense, RepeatVector, Masking, TimeDistributed, Conv2D, MaxPooling2D, Flatten, Activation, Dropout
from tensorflow.keras.utils import plot_model, to_categorical
from tensorflow.python.framework import ops
ops.reset_default_graph()
from  tensorflow.keras.preprocessing.image import ImageDataGenerator

import sys
import os		# use os because python IO is bugy
import time
import threading
import re
import socket
import json
import random

import gi
gi.require_version('Gtk', '3.0')
from gi.repository import GLib, Gio, Gtk, Gdk, GObject, GdkPixbuf
#import cairo

from netCDF4 import Dataset, chartostring, stringtochar, stringtoarr
import struct
import array
import math

import numpy as np
import matplotlib.cm as cm
import matplotlib.pyplot as plt
import matplotlib.cbook as cbook
from matplotlib.path import Path
from matplotlib.patches import PathPatch
from matplotlib.backends.backend_gtk3agg import (FigureCanvasGTK3Agg as FigureCanvas)
from matplotlib.figure import Figure
from matplotlib.backends.backend_gtk3cairo import FigureCanvasGTK3Cairo as FigureCanvas
from matplotlib.backends.backend_gtk3 import NavigationToolbar2GTK3 as NavigationToolbar
from matplotlib.patches import Rectangle
from matplotlib import cm

from PIL import Image

## GXSM AI: Brain -- KERAS
DATAPATH = 'DATA'

class brain:
    def __init__(self, num_labels=10, patch_shape=64):
        print ("GXSM Brain: Initializing Keras Classifier...")
        self.num_labels = num_labels

        # Initialising the CNN
        #self.classifier = Sequential()

        # Step 1 - Convolution
        #self.classifier.add(Conv2D(32, (3, 3), input_shape = (patch_shape, patch_shape, 1), activation = 'relu'))

        # Step 2 - Pooling
        #self.classifier.add(MaxPooling2D(pool_size = (2, 2)))
        # Adding a second convolutional layer
        #self.classifier.add(Conv2D(32, (3, 3), activation = 'relu'))
        #self.classifier.add(MaxPooling2D(pool_size = (2, 2)))

        # Step 3 - Flattening
        #self.classifier.add(Flatten())

        # Step 4 - Full connection
        #self.classifier.add(Dense(units = 128, activation = 'relu'))
        #self.classifier.add(Dense(units = 1, activation = 'sigmoid'))

        # Compiling the CNN
        #self.classifier.compile(optimizer = 'adam', loss = 'categorical_crossentropy', metrics = ['accuracy'])
        #self.classifier.compile(loss='categorical_crossentropy',
        #                        optimizer=opt,
        #                        metrics=['accuracy'])

        self.classifier = Sequential()
        self.classifier.add(Conv2D(32, (3, 3), padding='same', input_shape = (patch_shape, patch_shape, 1)))    # input_shape=x_train.shape[1:]
        self.classifier.add(Activation('relu'))
        self.classifier.add(Conv2D(32, (3, 3)))
        self.classifier.add(Activation('relu'))
        self.classifier.add(MaxPooling2D(pool_size=(2, 2)))
        self.classifier.add(Dropout(0.25))

        self.classifier.add(Conv2D(64, (3, 3), padding='same'))
        self.classifier.add(Activation('relu'))
        self.classifier.add(Conv2D(64, (3, 3)))
        self.classifier.add(Activation('relu'))
        self.classifier.add(MaxPooling2D(pool_size=(2, 2)))
        self.classifier.add(Dropout(0.25))

        self.classifier.add(Flatten())
        self.classifier.add(Dense(512))
        self.classifier.add(Activation('relu'))
        self.classifier.add(Dropout(0.5))
        self.classifier.add(Dense(self.num_labels))
        self.classifier.add(Activation('softmax'))

        # initiate RMSprop optimizer
        opt = RMSprop(learning_rate=0.0001, decay=1e-6)

        # Let's train the model using RMSprop
        self.classifier.compile(loss='categorical_crossentropy',
                                optimizer=opt,
                                metrics=['accuracy'])
        
        self.set_count = [0,0]

        self.patch_shape = patch_shape
        
        self.training_data   = { 'dataarr': [], 'labelarr': [], 'tag': [] }
        self.validation_data = { 'dataarr': [], 'labelarr': [], 'tag': [] }
        self.prediction_data = { 'dataarr': [], 'labelarr': [], 'tag': [] }
        
        self.datapath = DATAPATH
        
    def init_training(self):
        print ("GXSM Brain: Preparing Training and Validation Sets...")
        self.train_datagen = ImageDataGenerator (rescale = 1./20.,
                                                 rotation_range=60,
                                                 shear_range = 0.2,
                                                 zoom_range = 0.2,
                                                 horizontal_flip = True,
                                                 vertical_flip = True)
        self.validation_datagen = ImageDataGenerator (rescale = 1./20)

        # flow(x, y=None, batch_size=32, shuffle=True, sample_weight=None, seed=None, save_to_dir=None, save_prefix='', save_format='png', subset=None)

        #print (self.training_data['dataarr'])
        #print (self.training_data['labelarr'])
        print ('T Data: ', np.shape(np.array(self.training_data['dataarr'])))
        print ('T Labels: ',np.shape(np.array(self.training_data['labelarr'])))

        self.training_one_hot_labels = to_categorical(np.array(self.training_data['labelarr']), self.num_labels)
        print (self.training_one_hot_labels)
        self.training_set = self.train_datagen.flow (np.array(self.training_data['dataarr']), self.training_one_hot_labels, # X (img:[[xn,yn,chn]...] , Y [(labels)...])
                                                     batch_size = 32)
        
        print ('V Data: ', np.shape(np.array(self.validation_data['dataarr'])))
        print ('V Labels: ',np.shape(np.array(self.validation_data['labelarr'])))

        self.validation_one_hot_labels = to_categorical(np.array(self.validation_data['labelarr']), self.num_labels)
        print (self.validation_one_hot_labels)
        self.validation_set = self.validation_datagen.flow (np.array(self.validation_data['dataarr']), self.validation_one_hot_labels, # X (img:[[xn,yn,chn]...] , Y [(labels)...])
                                                            batch_size = 32)

    def start_training(self):
        self.init_training()
        print ("GXSM Brain: In Training...")
        self.classifier.fit_generator (self.training_set,
                                       steps_per_epoch = 2000, #8000
                                       epochs = 5, #25
                                       validation_data = self.validation_set,
                                       validation_steps = 10) #100
        self.store_brain ()

    def store_datasets(self):
        print (self.training_data)
        print (self.validation_data)
        return

    def load_datasets(self):
        return
        
    def store_brain(self):
        self.classifier.save("STM_brain0.hd5") 

    def load_brain(self):
        self.classifier = load_model("STM_brain0.hd5")

    def do_prediction(self):
        #test_image = image.load_img(casefile, target_size = (64, 64))
        #test_image = image.img_to_array(test_image)
        #test_image = np.expand_dims(test_image, axis = 0)
        #test_image = np.expand_dims(data, axis = 0)
        #result = self.classifier.predict (self.prediction_data)
        #self.training_set.class_indices
        #print (result)

        # clear
        for tag in self.prediction_data['tag']:
            key = '0'
            tag['value'] = key
            tag['color'] = CAT_COLORS[key]
            tag['description'] = CAT_DESCRIPTIONS[key]
            ##'region': { 'xy':[0.,0.,0.,0.],'ij':[0,0,0,0]},
            tag['event-at'] = [tag['region']['xy'][0]+tag['region']['xy'][2]/2, tag['region']['xy'][1]+tag['region']['xy'][3]/2]

        print (self.prediction_data['tag'])

        self.prediction_datagen = ImageDataGenerator (rescale = 1./20)
        self.prediction_one_hot_labels = to_categorical(np.array(self.prediction_data['labelarr']), self.num_labels)
        print (self.prediction_one_hot_labels)
        self.prediction_set = self.prediction_datagen.flow (np.array(self.prediction_data['dataarr']), self.prediction_one_hot_labels, # X (img:[[xn,yn,chn]...] , Y [(labels)...])
                                                            batch_size = 32)
        print ("Running prediction...")
        self.result = self.classifier.predict_generator (self.prediction_set)
        print (self.result)
        # update labels
        for v, tag in zip (self.result, self.prediction_data['tag']):
            key = str(np.argmax(v))
            print ('{} => {} => key:"{}"'.format(tag, v, key))
            tag['value'] = key
            tag['predictions'] = v
            tag['color'] = CAT_COLORS[key]
            tag['description'] = CAT_DESCRIPTIONS[key]
            print (tag)
        return self.prediction_data['tag']

    def add_to_set(self, patch, label, dataset, name):
        if label['value'] >= self.num_labels:
            return

        im = Image.fromarray(patch, 'F')
        data = np.array(im.resize ((self.patch_shape, self.patch_shape)))  # assume square shape
        
        dataset['dataarr'].append (np.transpose([data]))
        dataset['labelarr'].append (int(label['value']))
        dataset['tag'].append (label)
        #print (dataset)
        
        ### create gallery for visual indspection only, not used for data input
        im = Image.fromarray(data*64, 'F')
        im = im.convert('L')
        #im.show()
        
        fdir=self.datapath+'/'+name
        if not os.path.isdir (fdir):
            os.mkdir (fdir)
        if not os.path.isdir ("{}/{}".format(fdir, label['description'])):
            os.mkdir ("{}/{}".format(fdir,label['description']))
        im.save ("{}/{}/tr{}_{}.png".format(fdir, label['description'], label['value'], self.set_count[0]))
        self.set_count[0] = self.set_count[0] + 1

    def add_to_training_set(self, patch, label):
        self.add_to_set (patch, label, self.training_data, "training-set")
        
    def add_to_validation_set(self, patch, label):
        self.add_to_set (patch, label, self.validation_data, "validation-set")

    def add_to_prediction_set(self, patch, label):
        self.add_to_set (patch, label, self.prediction_data, "prediction-set")

    def clear_prediction_set(self):
        self.prediction_data = { 'dataarr': [], 'labelarr': [], 'tag': [] }
        print (self.prediction_data)
        
###################################################################
## from gxsm xsm.C / gxsm units mechanism "id" :
## unit symbol, ascii simple, scaling to base, formatting options
###################################################################
gxsm_units = {
        "AA": [ u"\u00C5",   "Ang",    1e0, ".1f", ".3f" ],
        "nm": [ "nm",  "nm",     10e0, ".1f", ".3f" ],
        "um": [ u"\u00B5m",  "um",     10e3, ".1f", ".3f" ],
        "mm": [ "mm",  "mm",     10e6, ".1f", ".3f" ],
        "BZ": [ "%BZ", "BZ",     1e0, ".1f", ".2f" ],
        "sec": ["\"",  "\"",      1e0, ".1f", ".2f" ],
        "V": [  "V",   "V",       1e0, ".2f", ".3f" ],
        "mV": [ "mV",  "mV",      1e-3, ".2f", ".3f" ],
        "V": [  "*V", "V",      1e0, ".2f", ".3f" ],
        "*dV": [ "*dV","dV",     1e0, ".2f", ".3f" ],
        "*ddV": [ "*ddV","ddV",  1e0, ".2f", ".3f" ],
        "*V2": [ "V2", "V2",       1e0, ".2f", ".3f" ],
        "1": [  " ",   " ",       1e0, ".3f", ".4f" ],
        "0": [  " ",   " ",       1e0, ".3f", ".4f" ],
        "B": [  "Bool",   "Bool", 1e0, ".3f", ".4f" ],
        "X": [  "X",   "X",       1e0, ".3f", ".4f" ],
        "xV": [  "xV",   "xV",    1e0, ".3f", ".4f" ],
        "deg": [ u"\u00B0", "deg",       1e0, ".3f", ".4f" ],
        "Amp": [ "A",  "A",       1e9, "g", "g" ],
        "nA": [ "nA",  "nA",      1e0, ".2f", ".3f" ],
        "pA": [ "pA",  "pA",      1e-3, ".1f", ".2f" ],
        "nN": [ "nN",  "nN",      1e0, ".2f", ".3f" ],
        "Hz": [ "Hz",  "Hz",      1e0, ".2f", ".3f" ],
        "mHz": [ "mHz",  "mHz",   1e-3, ".2f", ".3f" ],
        "K": [  "K",   "K",       1e0, ".2f", ".3f" ],
        "amu": ["amu", "amu",     1e0, ".1f", ".2f" ],
        "CPS": ["Cps", "Cps",     1e0, ".1f", ".2f" ],
        "CNT": ["CNT", "CNT",     1e0, ".1f", ".2f" ],
        "Int": ["Int", "Int",     1e0, ".1f", ".2f" ],
        "A/s": ["A/s", "A/s",     1e0, ".2f", ".3f" ],
        "s": ["s", "s",           1e0, ".2f", ".3f" ],
        "ms": ["ms", "ms",        1e0, ".2f", ".3f" ],
}


############################################################
# Socket Client
############################################################

HOST = '127.0.0.1'  # The server's hostname or IP address
PORT = 65432        # The port used by the server


class SocketClient:
    def __init__(self, host, port):
        self.sok = None
        #with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as client:
        #   self.sok = client
        try:
            self.sok=socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.sok.connect ((host, port))
            self.send_as_json ({'echo': [{'message': 'Hello GXSM3! Establishing Socket Link.'}]})
            data = self.receive_json ()
            print('Received: ', data)
        except:
            pass
            
    def __del__(self):
        if not self.sok:
            print ('No connection')
            raise Exception('You have to connect first before receiving data')
        self.sok.close()

    def send_as_json(self, data):
        if not self.sok:
            print ('No connection')
            raise Exception('You have to connect first before receiving data')
        try:
            serialized = json.dumps(data)
        except (TypeError, ValueError):
            raise Exception('You can only send JSON-serializable data')
        
        print('Sending JSON: N={} D={}'.format(len(serialized.encode('utf-8')),serialized))

        # send the length of the serialized data first
        sd = '{}\n{}'.format(len(serialized), serialized)
        # send the serialized data
        self.sok.sendall(sd.encode('utf-8'))
            
    def request_start_scan(self):
        self.send_as_json({'action': ['start-scan']})
        return self.receive()

    def request_stop_scan(self):
        self.send_as_json({'action': ['stop-scan']})
        return self.receive()

    def request_autosave(self):
        self.send_as_json({'action': ['autosave']})
        return self.receive()

    def request_autoupdate(self):
        self.send_as_json({'action': ['autoupdate']})
        return self.receive()

    def request_action(self, id):
        self.send_as_json({'action': [{'id':id}]})
        return self.receive()

    def request_action_v(self, id, value):
        self.send_as_json({'action': [{'id':id, 'value':value}]})
        return self.receive()

    def request_set_parameter(self, id, value):
        self.send_as_json({'command': [{'set': id, 'value': value}]})
        return self.receive()

    def request_get_parameter(self, id):
        self.send_as_json({'command': [{'get': id}]})
        return self.receive()

    def request_gets_parameter(self, id):
        self.send_as_json({'command': [{'gets': id}]})
        return self.receive()

    def request_query_info(self, x):
        self.send_as_json({'command': [{'query': x}]})
        return self.receive()

    def request_query_info_args(self, x, i=0,j=0,k=0):
        self.send_as_json({'command': [{'query': x, 'args': [i,j,k]}]})
        return self.receive()

    def receive(self):
        if not self.sok:
            print ('No connection')
            raise Exception('You have to connect first before receiving data')
        return self.receive_json()

    def receive_json(self):
        #print ('receive_json...\n')
        if not self.sok:
            print ('No connection')
            raise Exception('You have to connect first before receiving data')
        # try simple assume one message
        try:
            data = self.sok.recv (1024)
            if data:
                #print ('Got Data: {}'.format(data))
                count, jsdata = data.split(b'\n')
                #print ('N={} D={}'.format(count,jsdata))
                try:
                    deserialized = json.loads(jsdata)
                    print ('Received JSON: N={} D={}'.format(count,deserialized))
                    return deserialized
                except (TypeError, ValueError):
                    deserialized = json.loads({'JSON-Deserialize-Error'})
                    raise Exception('Data received was not in JSON format')
                    return deserialized
            else:
                pass
        except:
            pass
        
    def receive_json_long(self, socket):
        # read the length of the data, letter by letter until we reach EOL
        length_str = b''
        print ('Waiting for response.\n')
        char = self.sok.recv(1)
        while char != '\n':
            length_str += char
            char = self.sok.recv(1)
        total = int(length_str)
        #print('receiving json bytes # ', total, ' [', length_str, ']\n')
        # use a memoryview to receive the data chunk by chunk efficiently
        view = memoryview(bytearray(total))
        next_offset = 0
        while total - next_offset > 0:
            recv_size = self.sok.recv_into(view[next_offset:], total - next_offset)
            next_offset += recv_size
        try:
            deserialized = json.loads(view.tobytes())
        except (TypeError, ValueError):
            raise Exception('Data received was not in JSON format')

        print('received JSON: {}\n', deserialized)
        
        return deserialized
    
    def recv_and_close(self):
        data = self.receive()
        self.close()
        return data
    
    def close(self):
        if self.s:
            self.sok.close()
            self.sok = None

############################################################
# END SOCKET
############################################################


CAT_COLORS = {
    '0': 'grey',
    '1': 'aquamarine',
    '2': 'cyan',
    '3': 'lawngreen',
    '4': 'orange',
    '5': 'fuchsia',
    '6': 'gold',
    '7': 'yellow',
    '8': 'magenta',
    '9': 'deeppink',
    '10': 'violet',
    '11': 'deepskyblue',
    '20': 'red',
    '21': 'green',
    '22': 'blue',
    '23': 'cyan',
    '24': 'magenta',
    '30': 'red',
    '31': 'green',
    '32': 'blue',
    '33': 'cyan',
    '34': 'magenta',
    '50': 'red',
    '51': 'green',
    '52': 'blue',
    '53': 'cyan',
    '54': 'magenta',
    '90': 'bisque',
    '91': 'orange',
    '92': 'coral',
    '94': 'tomato',
    '99': 'red',
}


CAT_DESCRIPTIONS = {
    '0':  '0: Unknown',
    '1':  '1: Flat',
    '2':  '2: Lattice',
    '3':  '3: Step',
    '4':  '4: Step Bunch',
    '5':  '5: Island',
    '6':  '6: Adsorbate Small Molecule',
    '7':  '7: Adsorbate Large Molecule Cluster',
    '8':  '8: Tiny Blob',
    '9':  '9: Blob',
    '10': '10: Large Blob',
    '11': '11: Step Pinning',
    '20': '20: Ideal Metal Tip',
    '21': '21: Good Metal Tip',
    '22': '22: Blunt Metal Tip',
    '23': '23: Bad Tip or Surface',
    '24': '24: Double or Multi Tip',
    '30': '30: Molecule on Tip Fuzzy',
    '31': '31: Molecule on Tip OK',
    '32': '32: CO Tip ideal',
    '33': '33: CO Tip non ideal',
    '34': '34: --',
    '50': '50: Feature X',
    '51': '51: Feature Y',
    '52': '52: Feature Z',
    '53': '53: --',
    '54': '54: --',
    '90': '90: Tip Functionalized',
    '91': '91: Tip Change',
    '92': '92: Major Tip or Z Change',
    '93': '93: --',
    '94': '94: --',
    '99': '99: Danger',
}


#### Application stuff

APP_TITLE="GXSM NetCDF Scan Data Viewer and Image Tagging Tool with GXSM Socket Client Control and AI Support"

MENU_XML="""
<?xml version="1.0" encoding="UTF-8"?>
<interface>
  <menu id="app-menu">
    <section>
      <item>
        <attribute name="action">app.connect</attribute>
        <attribute name="label" translatable="yes">Connect Gxsm</attribute>
      </item>
      <item>
        <attribute name="action">app.open</attribute>
        <attribute name="label" translatable="yes">Open NetCDF</attribute>
      </item>
      <item>
        <attribute name="action">app.folder</attribute>
        <attribute name="label" translatable="yes">Set Folder</attribute>
      </item>
      <item>
        <attribute name="action">app.statistics</attribute>
        <attribute name="label" translatable="yes">Folder Statistics</attribute>
      </item>
      <item>
        <attribute name="action">app.list</attribute>
        <attribute name="label" translatable="yes">List NetCDF</attribute>
      </item>
    </section>
    <section>
      <item>
        <attribute name="action">win.maximize</attribute>
        <attribute name="label" translatable="yes">Maximize</attribute>
      </item>
    </section>
    <section>
      <item>
        <attribute name="action">app.about</attribute>
        <attribute name="label" translatable="yes">_About</attribute>
      </item>
      <item>
        <attribute name="action">app.quit</attribute>
        <attribute name="label" translatable="yes">_Quit</attribute>
        <attribute name="accel">&lt;Primary&gt;q</attribute>
    </item>
    </section>
  </menu>
  <menu id="app-menubar">
    <submenu id="app-file-menu">
      <attribute name="label" translatable="yes">_File</attribute>
      <section>
	<attribute name="id">file-section</attribute>
        <item>
          <attribute name="action">app.connect</attribute>
          <attribute name="label" translatable="yes">Connect Gxsm</attribute>
        </item>
        <item>
          <attribute name="action">app.open</attribute>
          <attribute name="label" translatable="yes">Open NetCDF</attribute>
        </item>
        <item>
          <attribute name="action">app.folder</attribute>
          <attribute name="label" translatable="yes">Set Folder</attribute>
        </item>
        <item>
          <attribute name="action">app.statistics</attribute>
          <attribute name="label" translatable="yes">Folder Statistics</attribute>
        </item>
        <item>
          <attribute name="action">app.list</attribute>
          <attribute name="label" translatable="yes">List NetCDF</attribute>
        </item>
      </section>
      <section>
        <item>
          <attribute name="action">app.about</attribute>
          <attribute name="label" translatable="yes">_About</attribute>
        </item>
        <item>
          <attribute name="action">app.quit</attribute>
          <attribute name="label" translatable="yes">_Quit</attribute>
          <attribute name="accel">&lt;Primary&gt;q</attribute>
        </item>
      </section>
    </submenu>
    <submenu id="app-action-menu">
      <attribute name="label" translatable="yes">Action</attribute>
      <section>
	<attribute name="id">action-section</attribute>
        <item>
          <attribute name="action">win.change_action</attribute>
          <attribute name="target">Tagging</attribute>
          <attribute name="label" translatable="yes">tagging</attribute>
        </item>
        <item>
          <attribute name="action">win.change_action</attribute>
          <attribute name="target">Scaling</attribute>
          <attribute name="label" translatable="yes">scaling</attribute>
        </item>
      </section>
    </submenu>
    <submenu id="app-tagging-menu">
      <attribute name="label" translatable="yes">Tags</attribute>
      <section>
	<attribute name="id">tags-section</attribute>
        <item>
          <attribute name="action">win.change_taglabel</attribute>
          <attribute name="target">0: Unknown</attribute>
          <attribute name="label" translatable="yes">Unkown</attribute>
        </item>
        <item>
          <attribute name="action">win.change_taglabel</attribute>
          <attribute name="target">1: Flat</attribute>
          <attribute name="label" translatable="yes">Flat</attribute>
        </item>
        <item>
          <attribute name="action">win.change_taglabel</attribute>
          <attribute name="target">2: Lattice</attribute>
          <attribute name="label" translatable="yes">Lattice</attribute>
        </item>
        <item>
          <attribute name="action">win.change_taglabel</attribute>
          <attribute name="target">3: Step</attribute>
          <attribute name="label" translatable="yes">Step</attribute>
        </item>
        <item>
          <attribute name="action">win.change_taglabel</attribute>
          <attribute name="target">4: Step Bunch</attribute>
          <attribute name="label" translatable="yes">Step Bunch</attribute>
        </item>
        <item>
          <attribute name="action">win.change_taglabel</attribute>
          <attribute name="target">5: Island</attribute>
          <attribute name="label" translatable="yes">Island</attribute>
        </item>
        <item>
          <attribute name="action">win.change_taglabel</attribute>
          <attribute name="target">6: Adsorbate Small Molecule</attribute>
          <attribute name="label" translatable="yes">Adsorbate Small Molecule</attribute>
        </item>
        <item>
          <attribute name="action">win.change_taglabel</attribute>
          <attribute name="target">7: Adsorbate Large Molecule Cluster</attribute>
          <attribute name="label" translatable="yes">Adsorbate Large Molecule Cluster</attribute>
        </item>
        <item>
          <attribute name="action">win.change_taglabel</attribute>
          <attribute name="target">8: Tiny Blob</attribute>
          <attribute name="label" translatable="yes">Tiny Blob</attribute>
        </item>
        <item>
          <attribute name="action">win.change_taglabel</attribute>
          <attribute name="target">9: Blob</attribute>
          <attribute name="label" translatable="yes">Blob</attribute>
        </item>
        <item>
          <attribute name="action">win.change_taglabel</attribute>
          <attribute name="target">10: Large Blob</attribute>
          <attribute name="label" translatable="yes">Large Blob</attribute>
        </item>
        <item>
          <attribute name="action">win.change_taglabel</attribute>
          <attribute name="target">11: Step Pinning</attribute>
          <attribute name="label" translatable="yes">Step Pinning</attribute>
        </item>
        <item>
          <attribute name="action">win.change_taglabel</attribute>
          <attribute name="target">20: Ideal Metal Tip</attribute>
          <attribute name="label" translatable="yes">Ideal Metal Tip</attribute>
        </item>
        <item>
          <attribute name="action">win.change_taglabel</attribute>
          <attribute name="target">21: Good Metal Tip</attribute>
          <attribute name="label" translatable="yes">Good Metal Tip</attribute>
        </item>
        <item>
          <attribute name="action">win.change_taglabel</attribute>
          <attribute name="target">22: Blunt Metal Tip</attribute>
          <attribute name="label" translatable="yes">Blunt Metal Tip</attribute>
        </item>
        <item>
          <attribute name="action">win.change_taglabel</attribute>
          <attribute name="target">23: Bad Tip or Surface</attribute>
          <attribute name="label" translatable="yes">Bad Tip or Surface</attribute>
        </item>
        <item>
          <attribute name="action">win.change_taglabel</attribute>
          <attribute name="target">24: Double or Multi Tip</attribute>
          <attribute name="label" translatable="yes">Double or Multi Tip</attribute>
        </item>
        <item>
          <attribute name="action">win.change_taglabel</attribute>
          <attribute name="target">30: Molecule on Tip Fuzzy</attribute>
          <attribute name="label" translatable="yes">Molecule on Tip Fuzzy</attribute>
        </item>
        <item>
          <attribute name="action">win.change_taglabel</attribute>
          <attribute name="target">31: Molecule on Tip OK</attribute>
          <attribute name="label" translatable="yes">Molecule on Tip OK</attribute>
        </item>
        <item>
          <attribute name="action">win.change_taglabel</attribute>
          <attribute name="target">32: CO Tip ideal</attribute>
          <attribute name="label" translatable="yes">CO Tip ideal</attribute>
        </item>
        <item>
          <attribute name="action">win.change_taglabel</attribute>
          <attribute name="target">33: CO Tip non ideal</attribute>
          <attribute name="label" translatable="yes">CO Tip non ideal</attribute>
        </item>
        <item>
          <attribute name="action">win.change_taglabel</attribute>
          <attribute name="target">50: Feature X</attribute>
          <attribute name="label" translatable="yes">Feature X</attribute>
        </item>
        <item>
          <attribute name="action">win.change_taglabel</attribute>
          <attribute name="target">51: Feature Y</attribute>
          <attribute name="label" translatable="yes">Feature Y</attribute>
        </item>
        <item>
          <attribute name="action">win.change_taglabel</attribute>
          <attribute name="target">52: Feature Z</attribute>
          <attribute name="label" translatable="yes">Feature Z</attribute>
        </item>
        <item>
          <attribute name="action">win.change_taglabel</attribute>
          <attribute name="target">90: Tip Functionalized</attribute>
          <attribute name="label" translatable="yes">Tip Functionalized</attribute>
        </item>
        <item>
          <attribute name="action">win.change_taglabel</attribute>
          <attribute name="target">91: Tip Change</attribute>
          <attribute name="label" translatable="yes">Tip Change</attribute>
        </item>
        <item>
          <attribute name="action">win.change_taglabel</attribute>
          <attribute name="target">92: Major Tip or Z Change</attribute>
          <attribute name="label" translatable="yes">Major Tip or Z Change</attribute>
        </item>
        <item>
          <attribute name="action">win.change_taglabel</attribute>
          <attribute name="target">99: Danger</attribute>
          <attribute name="label" translatable="yes">Danger</attribute>
        </item>
      </section>
      <section>
	<attribute name="id">tag-dest-section</attribute>
        <item>
          <attribute name="action">win.change_tagdest</attribute>
          <attribute name="target">RegionTags</attribute>
          <attribute name="label" translatable="yes">Use Region Tags</attribute>
        </item>
        <item>
          <attribute name="action">win.change_tagdest</attribute>
          <attribute name="target">AITags</attribute>
          <attribute name="label" translatable="yes">Use AI Tags</attribute>
        </item>
        <item>
          <attribute name="action">win.change_tagdest</attribute>
          <attribute name="target">TrainingTags</attribute>
          <attribute name="label" translatable="yes">Use Training Tags</attribute>
        </item>
        <item>
          <attribute name="action">win.change_tagdest</attribute>
          <attribute name="target">ValidationTags</attribute>
          <attribute name="label" translatable="yes">Use Validation Tags</attribute>
        </item>
        <item>
          <attribute name="action">win.tags-clear</attribute>
          <attribute name="label" translatable="yes">Clear Tags</attribute>
        </item>
        <item>
          <attribute name="action">win.tags-store</attribute>
          <attribute name="label" translatable="yes">Store Tags</attribute>
        </item>
        <item>
          <attribute name="action">win.tags-load</attribute>
          <attribute name="label" translatable="yes">Load Tags</attribute>
        </item>
      </section>
    </submenu>
    <submenu id="app-ai-menu">
      <attribute name="label" translatable="yes">AI-Keras</attribute>
      <section>
	<attribute name="id">ai-section</attribute>
        <item>
          <attribute name="action">win.ai_auto_add_folder_to_training_and_validation</attribute>
          <attribute name="label" translatable="yes">Auto Folder to T+V</attribute>
        </item>
        <item>
          <attribute name="action">win.ai_add_to_training_set</attribute>
          <attribute name="label" translatable="yes">Add to Training</attribute>
        </item>
        <item>
          <attribute name="action">win.ai_add_to_validation_set</attribute>
          <attribute name="label" translatable="yes">Add to Validation</attribute>
        </item>
        <item>
          <attribute name="action">win.ai_add_to_prediction_set</attribute>
          <attribute name="label" translatable="yes">Add to Prediction</attribute>
        </item>
        <item>
          <attribute name="action">win.ai_store_data</attribute>
          <attribute name="label" translatable="yes">Store Data</attribute>
        </item>
        <item>
          <attribute name="action">win.ai_load_data</attribute>
          <attribute name="label" translatable="yes">Load Data</attribute>
        </item>
        <item>
          <attribute name="action">win.ai_train</attribute>
          <attribute name="label" translatable="yes">Run Training</attribute>
        </item>
        <item>
          <attribute name="action">win.ai_evaluate</attribute>
          <attribute name="label" translatable="yes">Run Evaluation</attribute>
        </item>
        <item>
          <attribute name="action">win.ai_store</attribute>
          <attribute name="label" translatable="yes">Store Brain</attribute>
        </item>
        <item>
          <attribute name="action">win.ai_load</attribute>
          <attribute name="label" translatable="yes">Load Brain</attribute>
        </item>
        <item>
          <attribute name="action">win.ai_clear_prediction_set</attribute>
          <attribute name="label" translatable="yes">Clear Prediction Set</attribute>
        </item>
      </section>
    </submenu>
  </menu>
</interface>
"""

############################################################
# Main Window and Application
############################################################

class AppWindow(Gtk.ApplicationWindow):

    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)

        self.subN=1
        self.subM=1
        self.gxsm = None
        self.cdf_image_data_filename = ''
        self.connect("destroy", self.quit)
        self.rootgrp = None
        self.tag_labels = []
        self.NCFile_tags = { 'RegionTags': [], 'AITags': [], 'TrainingTags': [], 'ValidationTags': [] }
        self.click_action = 'Tagging'
        self.AI = None
        
        # This will be in the windows group and have the "win" prefix
        max_action = Gio.SimpleAction.new_stateful("maximize", None,
                                           GLib.Variant.new_boolean(False))
        max_action.connect("change-state", self.on_maximize_toggle)
        self.add_action(max_action)

        # Keep it in sync with the actual state
        self.connect("notify::is-maximized",
                            lambda obj, pspec: max_action.set_state(
                                               GLib.Variant.new_boolean(obj.props.is_maximized)))
        grid = Gtk.Grid()
        self.grid=grid
        self.add(grid)

        ## FILE SELECTION
        
        y=0
        button1 = Gtk.Button(label="Open NetCDF")
        button1.connect("clicked", self.on_file_clicked)
        grid.attach(button1, 0, y, 2, 1)
        y=y+1
        #button2 = Gtk.Button(label="Set Folder")
        #button2.connect("clicked", self.on_folder_clicked)
        #grid.attach(button2, 0, y, 2, 1)
        #y=y+1
        #button3 = Gtk.Button(label="List NetCDF")
        #button3.connect("clicked", self.list_CDF_clicked)
        #grid.attach(button3, 0, y, 2, 1)
        #y=y+1
        button4 = Gtk.Button(label="Reload")
        button4.connect("clicked", self.reload_clicked)
        grid.attach(button4, 0, y, 2, 1)
        y=y+1
        button5 = Gtk.Button(label="Next #")
        button5.connect("clicked", self.next_clicked)
        grid.attach(button5, 1, y, 1, 1)
        button6 = Gtk.Button(label="Prev #")
        button6.connect("clicked", self.prev_clicked)
        grid.attach(button6, 0, y, 1, 1)
        y=y+1
        button5 = Gtk.Button(label="Next")
        button5.connect("clicked", self.next_in_list_clicked)
        grid.attach(button5, 1, y, 1, 1)
        button6 = Gtk.Button(label="Prev")
        button6.connect("clicked", self.prev_in_list_clicked)
        grid.attach(button6, 0, y, 1, 1)
        y=y+1

        ## PROCESSING OPTIONS
        
        buttonQ = Gtk.Button(label="P: Quick")
        buttonQ.connect("clicked", self.process_quick_clicked)
        grid.attach(buttonQ, 0, y, 2, 1)
        y=y+1
        buttonQ = Gtk.Button(label="P: Plane")
        buttonQ.connect("clicked", self.process_plane_clicked)
        grid.attach(buttonQ, 0, y, 2, 1)
        y=y+1
        buttonQ = Gtk.Button(label="P: Grad^2")
        buttonQ.connect("clicked", self.process_grad2_clicked)
        grid.attach(buttonQ, 0, y, 1, 1)
        buttonQ = Gtk.Button(label="P: Grad^2 M1")
        buttonQ.connect("clicked", self.process_grad2m1_clicked)
        grid.attach(buttonQ, 1, y, 1, 1)
        y=y+1

        ## REGIONS SETUP

        button2x2 = Gtk.Button(label="2x2")
        button2x2.connect("clicked", self.subdivide2x2_clicked)
        grid.attach(button2x2, 0, y, 1, 1)
        button2x2ai = Gtk.Button(label="...AI")
        button2x2ai.connect("clicked", self.subdivide2x2ai_clicked)
        grid.attach(button2x2ai, 1, y, 1, 1)
        y=y+1
        button3x3 = Gtk.Button(label="3x3")
        button3x3.connect("clicked", self.subdivide3x3_clicked)
        grid.attach(button3x3, 0, y, 1, 1)
        button3x3ai = Gtk.Button(label="...AI")
        button3x3ai.connect("clicked", self.subdivide3x3ai_clicked)
        grid.attach(button3x3ai, 1, y, 1, 1)
        y=y+1
        button4x4 = Gtk.Button(label="4x4")
        button4x4.connect("clicked", self.subdivide4x4_clicked)
        grid.attach(button4x4, 0, y, 1, 1)
        button4x4ai = Gtk.Button(label="...AI")
        button4x4ai.connect("clicked", self.subdivide4x4ai_clicked)
        grid.attach(button4x4ai, 1, y, 1, 1)
        y=y+1
        button5x5 = Gtk.Button(label="5x5")
        button5x5.connect("clicked", self.subdivide5x5_clicked)
        grid.attach(button5x5, 0, y, 1, 1)
        button5x5ai = Gtk.Button(label="...AI")
        button5x5ai.connect("clicked", self.subdivide5x5ai_clicked)
        grid.attach(button5x5ai, 1, y, 1, 1)
        y=y+1


        ## IMAGEPARAMETERS
        
        l = Gtk.Label(label="Image Parameters:")
        grid.attach(l, 0, y, 2, 1)
        y=y+1
        l = Gtk.Label(label="Bias")
        grid.attach(l, 0, y, 1, 1)
        self.bias = Gtk.Entry()
        grid.attach(self.bias, 1, y, 1, 1)
        y=y+1
        l = Gtk.Label(label="Current")
        grid.attach(l, 0, y, 1, 1)
        self.current = Gtk.Entry()
        grid.attach(self.current, 1, y, 1, 1)
        y=y+1
        l = Gtk.Label(label="Nx")
        grid.attach(l, 0, y, 1, 1)
        self.Nx = Gtk.Entry()
        grid.attach(self.Nx, 1, y, 1, 1)
        y=y+1
        l = Gtk.Label(label="Ny")
        grid.attach(l, 0, y, 1, 1)
        self.Ny = Gtk.Entry()
        grid.attach(self.Ny, 1, y, 1, 1)
        y=y+1
        l = Gtk.Label(label="XRange")
        grid.attach(l, 0, y, 1, 1)
        self.Xrange = Gtk.Entry()
        grid.attach(self.Xrange, 1, y, 1, 1)
        y=y+1
        l = Gtk.Label(label="YRange")
        grid.attach(l, 0, y, 1, 1)
        self.Yrange = Gtk.Entry()
        grid.attach(self.Yrange, 1, y, 1, 1)
        y=y+1
        l = Gtk.Label(label="dx")
        grid.attach(l, 0, y, 1, 1)
        self.dX = Gtk.Entry()
        grid.attach(self.dX, 1, y, 1, 1)
        y=y+1
        l = Gtk.Label(label="dy")
        grid.attach(l, 0, y, 1, 1)
        self.dY = Gtk.Entry()
        grid.attach(self.dY, 1, y, 1, 1)
        y=y+1
        l = Gtk.Label(label="dz")
        grid.attach(l, 0, y, 1, 1)
        self.dZ = Gtk.Entry()
        grid.attach(self.dZ, 1, y, 1, 1)
        y=y+1
        l = Gtk.Label(label="ZR")
        grid.attach(l, 0, y, 1, 1)
        self.ZR = Gtk.Entry()
        grid.attach(self.ZR, 1, y, 1, 1)

        # Menu Actions Mode
        self.act_variant = GLib.Variant.new_string("Tagging")
        act_action = Gio.SimpleAction.new_stateful("change_action", self.act_variant.get_type(), self.act_variant)
        act_action.connect("change-state", self.on_change_action_state)
        self.add_action(act_action)

        self.act_label = Gtk.Label(label=self.act_variant.get_string()) #,  margin=30)
        y=y+1
        grid.attach(Gtk.Label(label='Action:'),0,y,1,1)
        grid.attach(self.act_label,1,y,1,1)

        # Menu: TAGGING
        self.taglabel_variant = GLib.Variant.new_string("*0: Unknown*")
        lbl_action = Gio.SimpleAction.new_stateful("change_taglabel", self.taglabel_variant.get_type(), self.taglabel_variant)
        lbl_action.connect("change-state", self.on_change_taglabel_state)
        self.add_action(lbl_action)
        self.taglabel = "0: Unknown"

        # Menu Tagging Mode
        self.tact_variant = GLib.Variant.new_string("RegionTags")
        tact_action = Gio.SimpleAction.new_stateful("change_tagdest", self.tact_variant.get_type(), self.tact_variant)
        tact_action.connect("change-state", self.on_change_tagdest_state)
        self.add_action(tact_action)
        self.tact_label = Gtk.Label(label=self.tact_variant.get_string())
        self.tact_label_group = self.tact_variant.get_string()
        y=y+1
        grid.attach(Gtk.Label(label='Tag Dest:'),0,y,1,1)
        grid.attach(self.tact_label,1,y,1,1)

        y=y+1
        grid.attach(Gtk.Label(label='Work Folder:'),0,y,1,1)
        self.work_folder_w = Gtk.Label(label='.')
        grid.attach(self.work_folder_w,1,y,1,1)

        ## TAG Actions
        #########################################################
        action = Gio.SimpleAction.new("tags-clear", None)
        action.connect("activate", self.on_tags_clear)
        self.add_action(action)

        action = Gio.SimpleAction.new("tags-store", None)
        action.connect("activate", self.on_tags_store)
        self.add_action(action)

        action = Gio.SimpleAction.new("tags-load", None)
        action.connect("activate", self.on_tags_load)
        self.add_action(action)

        
        ## AI-Keras Actions
        #########################################################
        action = Gio.SimpleAction.new("ai_auto_add_folder_to_training_and_validation", None)
        action.connect("activate", self.on_ai_auto_add_folder_to_training_and_validation)
        self.add_action(action)

        action = Gio.SimpleAction.new("ai_add_to_training_set", None)
        action.connect("activate", self.on_ai_add_to_training)
        self.add_action(action)

        action = Gio.SimpleAction.new("ai_add_to_validation_set", None)
        action.connect("activate", self.on_ai_add_to_validation)
        self.add_action(action)

        action = Gio.SimpleAction.new("ai_add_to_prediction_set", None)
        action.connect("activate", self.on_ai_add_to_prediction)
        self.add_action(action)

        action = Gio.SimpleAction.new("ai_clear_prediction_set", None)
        action.connect("activate", self.on_ai_clear_prediction_set)
        self.add_action(action)

        action = Gio.SimpleAction.new("ai_train", None)
        action.connect("activate", self.on_ai_train)
        self.add_action(action)

        action = Gio.SimpleAction.new("ai_evaluate", None)
        action.connect("activate", self.on_ai_evaluate)
        self.add_action(action)

        action = Gio.SimpleAction.new("ai_store_data", None)
        action.connect("activate", self.on_ai_store_data)
        self.add_action(action)

        action = Gio.SimpleAction.new("ai_load_data", None)
        action.connect("activate", self.on_ai_load_data)
        self.add_action(action)     

        action = Gio.SimpleAction.new("ai_store", None)
        action.connect("activate", self.on_ai_store)
        self.add_action(action)

        action = Gio.SimpleAction.new("ai_load", None)
        action.connect("activate", self.on_ai_load)
        self.add_action(action)     
        ########################################################
        
        y=y+1
        grid.attach(Gtk.Label(label='Categories:'),0,y,2,1)
        self.numhistorytaglabels=12
        self.start_taglabels=7
        self.taglabelbutton = []
        for i in range(0,self.numhistorytaglabels):
            self.taglabelbutton.append(None)
            lbl = CAT_DESCRIPTIONS[str(i)]
            bgc = CAT_COLORS[str(i)]
            if i==0:
                self.taglabelbutton[i] = Gtk.RadioButton.new_with_label_from_widget (None, label=lbl)
            else:
                self.taglabelbutton[i] = Gtk.RadioButton.new_with_label_from_widget (self.taglabelbutton[i-1], label=lbl)  #self.taglabel_variant.get_string())
            self.taglabelbutton[i].connect("clicked", self.taglabel_clicked)
            for child in self.taglabelbutton[i].get_children():
                if i == 0:
                    child.set_label('<big><b><span background="{}"> *{}* </span></b></big>'.format(bgc,lbl))
                else:
                    child.set_label('<big><span background="{}"> *{}* </span></big>'.format(bgc,lbl))
                child.set_use_markup(True)
            y=y+1
            grid.attach(self.taglabelbutton[i],0,y,2,1)

        ## COLOR...

        #y=96
        y=y+1
        buttonCM = Gtk.Button(label="Color Map Swap")
        buttonCM.connect("clicked", self.color_map_swap_clicked)
        grid.attach(buttonCM, 0, y, 2, 1)
        y=y+1
        buttonC = Gtk.Button(label="Clear")
        buttonC.connect("clicked", self.clear_clicked)
        grid.attach(buttonC, 0, y, 2, 1)
        y=y+1
        self.positionC = Gtk.Label(label="MouseC-XY")
        grid.attach(self.positionC, 0, y, 2, 1)
        y=y+1
        self.positionM = Gtk.Label(label="MouseM-XY")
        grid.attach(self.positionM, 0, y, 2, 1)

        #ax = plt.Axes(fig,[0,0,1,1])
    
        self.fig = Figure(figsize=(6, 6), dpi=100)
        self.axy = self.fig.add_subplot(111)
        self.axline = self.fig.add_subplot(111)
        self.canvas = FigureCanvas(self.fig)  # a Gtk.DrawingArea
        self.canvas.set_size_request(600, 600)
        self.grid.attach(self.canvas, 3,1, 100,100)
        self.cbar = None
        self.im   = None
        self.colormap = cm.RdYlGn
        #self.colormap = cm.Greys  #magma
        #cmaps['Sequential'] = [
        #    'Greys', 'Purples', 'Blues', 'Greens', 'Oranges', 'Reds',
        #    'YlOrBr', 'YlOrRd', 'OrRd', 'PuRd', 'RdPu', 'BuPu',
        #    'GnBu', 'PuBu', 'YlGnBu', 'PuBuGn', 'BuGn', 'YlGn']

        #cmaps['Perceptually Uniform Sequential'] = [
        #    'viridis', 'plasma', 'inferno', 'magma', 'cividis']

        #cmaps['Sequential (2)'] = [
        #    'binary', 'gist_yarg', 'gist_gray', 'gray', 'bone', 'pink',
        #    'spring', 'summer', 'autumn', 'winter', 'cool', 'Wistia',
        #    'hot', 'afmhot', 'gist_heat', 'copper']

        # Create toolbar
        self.toolbar = NavigationToolbar (self.canvas, self)
        self.grid.attach(self.toolbar, 2,0,100,1)

        self.fig.canvas.mpl_connect('button_press_event', self.on_mouse_click_press)
        self.fig.canvas.mpl_connect('button_release_event', self.on_mouse_click_release)
        self.fig.canvas.mpl_connect('motion_notify_event', self.on_mouse_move)

        c=110
        y=0

        #self.buttonG1 = Gtk.Button(label="Connect GXSM")
        #self.buttonG1.set_tooltip_text("Connect to Gxsm3 socket server at {}:{}\nMust have the gxsm-sok-server.py in the gxsm remote console running.".format(HOST,PORT))
        #self.buttonG1.connect("clicked", self.connect_clicked)
        #grid.attach(self.buttonG1, c, y, 2, 1)
        
        self.show_all()

    def quit(self, button):
        if self.rootgrp:
            self.rootgrp.close()
        return
        

    def connect_clicked(self, widget):
        
        if self.gxsm != None:
            return

        #self.buttonG1.hide()
        
        grid=self.grid
        c=110
        y=1

        buttonG1 = Gtk.Button(label="GXSM ! Live")
        buttonG1.connect("clicked", self.live_clicked)
        grid.attach(buttonG1, c, y, 2, 1)
        y=y+1
        buttonG2 = Gtk.Button(label="GXSM ! Start")
        buttonG2.connect("clicked", self.start_clicked)
        grid.attach(buttonG2, c, y, 2, 1)
        y=y+1
        buttonG3 = Gtk.Button(label="GXSM ! Stop")
        buttonG3.connect("clicked", self.stop_clicked)
        grid.attach(buttonG3, c, y, 2, 1)
        y=y+1
        buttonG4 = Gtk.Button(label="GXSM ! Auto Save")
        buttonG4.connect("clicked", self.autosave_clicked)
        grid.attach(buttonG4, c, y, 2, 1)
        y=y+1
        buttonG5 = Gtk.Button(label="GXSM ! Auto Update")
        buttonG5.connect("clicked", self.autoupdate_clicked)
        grid.attach(buttonG5, c, y, 2, 1)
        y=y+1

        #generated:
        #for h in gxsm.list_refnames ():
	#    print ("'{}':\t['{}', {}, None, False],".format(h, h, gxsm.get(h))) 
        
        self.GXSM_params = {
            'RangeX':	['Range X', 1200.0, None, True],
            'RangeY':	['Range Y', 1200.0, None, True],
            'PointsX':	['Points X', 400.0, None, True],
            'PointsY':	['Points Y', 400.0, None, True],
            'StepsX':	['Steps X', 3.007518796992481, None, False],
            'StepsY':	['Steps Y', 3.007518796992481, None, False],
            'OffsetX':	['Offset X', -337.1, None, True],
            'OffsetY':	['Offset Y', 0.0, None, True],
            'ScanX':	['Scan X', 16.5, None, True],
            'ScanY':	['Scan Y', 0.0, None, True],
            'Rotation':	['Rotation', 0.0, None, True],
            'LayerSelect':	['LayerSelect', 0.0, None, False],
            'Layers':	['Layers', 1.0, None, False],
            'TimeSelect':	['TimeSelect', 0.0, None, False],
            'Time':	['Time', 1.0, None, False],
            'dsp-LCK-AC-Z-Amp':	['dsp-LCK-AC-Z-Amp', 0.0, None, False],
            'dsp-LCK-AC-Bias-Amp':	['dsp-LCK-AC-Bias-Amp', 0.399921, None, False],
            'dsp-fbs-bias3':	['Bias3', 0.0, None, False],
            'dsp-fbs-bias2':	['Bias2', 0.0, None, False],
            'dsp-fbs-bias1':	['Bias1', 0.0, None, False],
            'dsp-fbs-bias':	['Bias',  1.0, None, True],

            'dsp-fbs-mx0-current-level':	['Current-level', 0.0, None, True],
            'dsp-fbs-mx0-current-gain':	['dsp-fbs-mx0-current-gain', 0.5, None, False],
            'dsp-fbs-mx0-current-set':	['Current-set', 0.004, None, True],

            'dsp-adv-dsp-zpos-ref':	['Zpos-ref', 0.0, None, True],

            'dsp-fbs-scan-speed-move':	['Speed move', 1818.3, None, True],
            'dsp-fbs-scan-speed-scan':	['Speed scan', 802.5215805471124, None, True],

            'dsp-adv-scan-slope-y':	['Slope-y', -0.03013, None, True],
            'dsp-adv-scan-slope-x':	['Slope-x', -0.02528, None, True],

            'dsp-adv-scan-xs2nd-z-offset':	['dsp-adv-scan-xs2nd-z-offset', 0.0, None, False],
            'dsp-adv-scan-dyn-zoom':	['dsp-adv-scan-dyn-zoom', 1.0, None, False],
            'dsp-adv-scan-fwd-slow-down-2nd':	['dsp-adv-scan-fwd-slow-down-2nd', 10.0, None, False],
            'dsp-adv-scan-pre-pts':	['dsp-adv-scan-pre-pts', 0.0, None, False],
            'dsp-adv-scan-fwd-slow-down':	['dsp-adv-scan-fwd-slow-down', 1.0, None, False],
            'dsp-adv-scan-fast-return':	['dsp-adv-scan-fast-return', 1.0, None, False],
            'dsp-adv-scan-rasterb':	['dsp-adv-scan-rasterb', 11.0, None, False],
            'dsp-adv-scan-raster':	['dsp-adv-scan-raster', 11.0, None, False],
            'dsp-fbs-ci':	['CI', 120.18209408194234, None, True],
            'dsp-fbs-cp':	['CP', 149.50652503793626, None, True],
            'dsp-fbs-mx3-level':	['dsp-fbs-mx3-level', 0.0, None, False],
            'dsp-fbs-mx3-gain':	['dsp-fbs-mx3-gain', 0.5, None, False],
            'dsp-fbs-mx3-set':	['dsp-fbs-mx3-set', 0.0, None, False],
            'dsp-fbs-mx2-level':	['dsp-fbs-mx2-level', 0.0, None, False],
            'dsp-fbs-mx2-gain':	['dsp-fbs-mx2-gain', 5.0, None, False],
            'dsp-fbs-mx2-set':	['dsp-fbs-mx2-set', -0.1, None, False],
            'dsp-fbs-mx1-freq-level':	['dsp-fbs-mx1-freq-level', 0.0, None, False],
            'dsp-fbs-mx1-freq-gain':	['dsp-fbs-mx1-freq-gain', -0.499, None, False],
            'dsp-fbs-mx1-freq-set':	['dsp-fbs-mx1-freq-set', 0.0, None, False],
            'dsp-fbs-motor':	['Motor', 0.0, None, False],
        }

        if self.gxsm == None:
            self.gxsm = SocketClient(HOST, PORT)

        l = Gtk.Label(label="GXSM Parameters:")
        grid.attach(l, c, y, 2, 1)
        y=y+1

        for id in self.GXSM_params.keys():
            if self.GXSM_params[id][3]:
                l = Gtk.Label(label=self.GXSM_params[id][0])
                grid.attach(l, c, y, 1, 1)
                self.GXSM_params[id][2] = Gtk.Entry()
                self.GXSM_params[id][2].connect("focus_out_event", self.gxsmparam_changed3) ## only after out of focus
                #self.GXSM_params[id][2].connect("changed", self.gxsmparam_changed) ## immediate update
                grid.attach(self.GXSM_params[id][2], c+1, y, 1, 1)
                ret = self.gxsm.request_gets_parameter(id)
                #print(ret['result'][0]['value'])
                self.GXSM_params[id][2].set_text(str(ret['result'][0]['value']))
                y=y+1
        
        y=y+1
        buttonA = Gtk.Button(label="GXSM ! GetParam")
        buttonA.connect("clicked", self.getparam_clicked)
        grid.attach(buttonA, c, y, 2, 1)
        y=y+1
        buttonB = Gtk.Button(label="GXSM ! SetParam")
        buttonB.connect("clicked", self.setparam_clicked)
        grid.attach(buttonB, c, y, 2, 1)

        self.show_all()
        
        #def onclick(event):
        #    print('%s click: button=%d, x=%d, y=%d, xdata=%f, ydata=%f' %
        #          ('double' if event.dblclick else 'single', event.button,
        #           event.x, event.y, event.xdata, event.ydata))


    def on_mouse_move(self, event):
        if event.xdata and event.ydata:
            self.positionM.set_text ('MM: ({0:5.1f}, {1:5.1f})'.format(event.xdata, event.ydata))
        else:
            self.positionM.set_text ('MM: N/A')

    def on_mouse_click_press(self, event):
        if event.xdata and event.ydata:
            self.positionC.set_text ('MCP: ({0:5.1f}, {1:5.1f})'.format(event.xdata, event.ydata))
            current_label = self.taglabel
            label = {'color': CAT_COLORS[current_label.split(':')[0]],
                     'description': current_label,
                     'value':int(current_label.split(':')[0]),
                     'predictions': [],
                     'alpha': 0.25,
                     'region': { 'xy':[0.,0.,0.,0.],'ij':[0,0,0,0]},
                     'event-at': [int(event.xdata*10)/10., int(event.ydata*10)/10.],
                     'M': self.subM,
                     'N': self.subN
            }

            if self.click_action == 'Tagging':
                self.tag(label)
                self.tag_labels.append (label)
                self.add_tag_to_netcdf (label, self.tact_label_group)

            elif self.click_action == 'Scaling':
                self.rect={'p1':[0,0], 'p2':[0,0]}
                self.do_scale (event.xdata, event.ydata, 'p1')
        else:
            self.positionC.set_text ('MC: N/A')

    def on_mouse_click_release(self, event):
        if event.xdata and event.ydata:
            self.positionC.set_text ('MCR: ({0:5.1f}, {1:5.1f})'.format(event.xdata, event.ydata))
            if self.click_action == 'Tagging':
                print('*')
            elif self.click_action == 'Scaling':
                self.do_scale (event.xdata, event.ydata, 'p2')
        else:
            self.positionC.set_text ('MC: N/A')

    ## addes tag to NCFile
    def store_tags_to_netcdf (self, tags, nc_group='RegionTags'):
        #print ('NCFile_tags=',self.NCFile_tags['RegionTags'])
        if self.rootgrp:
            if nc_group in self.rootgrp.groups:
                tagsvar = self.rootgrp['/RegionTags/JsonImageTags']
                print ('NCFile RegionTags found:\n', tagsvar[:], '\n')
            else:
                print ('Creating RegionTags group.\n')
                self.rootgrp.createGroup     (nc_group)
                self.rootgrp.createDimension ('NumTags',64) # max 64 tags
                tagsvar = self.rootgrp.createVariable  ('/{}/JsonImageTags'.format(nc_group), str, 'NumTags')
            print ('Updating Region Tags:\n')
            i=len (tags)
            if i > 0:
                tagsvar[:len(tags)] = np.array(tags)
            for j in range(i,64):
                tagsvar[j:j+1] = np.array([''])
            print ('/{}/JsonImageTag ==> {}'.format(nc_group, tagsvar[:]))

    ## addes tag to NCFile
    def add_tag_to_netcdf (self, label, nc_group='RegionTags'):
        jstr = json.dumps (label)
        self.NCFile_tags[nc_group].append (jstr)
        self.store_tags_to_netcdf (self.NCFile_tags[nc_group], nc_group)
            
    def on_file_clicked(self, widget):
        dialog = Gtk.FileChooserDialog(action=Gtk.FileChooserAction.OPEN)
        dialog.add_buttons(Gtk.STOCK_CANCEL, Gtk.ResponseType.CANCEL,
			   Gtk.STOCK_OPEN, Gtk.ResponseType.OK)
        #dialog.set_transient_for(self.main_widget)
        self.add_filters(dialog)
        #dialog.modal = True
        response = dialog.run()
        try:
            if response == Gtk.ResponseType.OK:
                print("Open clicked")
                print("File selected: " + dialog.get_filename())
                self.cdf_image_data_filename = dialog.get_filename()
                self.load_CDF ()
        finally:
            dialog.destroy()

    def add_filters(self, dialog):
        filter_py = Gtk.FileFilter()
        filter_py.set_name("Unidata NetCDF (GXSM)")
        filter_py.add_mime_type("application/x-netcdf")
        dialog.add_filter(filter_py)

        filter_any = Gtk.FileFilter()
        filter_any.set_name("Any files")
        filter_any.add_pattern("*")
        dialog.add_filter(filter_any)

    def on_folder_clicked(self, widget):
        dialog = Gtk.FileChooserDialog("Please choose a folder", self,
                                       Gtk.FileChooserAction.SELECT_FOLDER,
                                       (Gtk.STOCK_CANCEL, Gtk.ResponseType.CANCEL,
                                        "Select", Gtk.ResponseType.OK))
        #dialog.set_default_size(600, 400)

        response = dialog.run()
        if response == Gtk.ResponseType.OK:
            self.work_folder = dialog.get_filename()
            self.work_folder_w.set_text ('...{}'.format(self.work_folder[-25:]))
            print("Folder selected: {}".format(self.work_folder))
            self.file_list = glob2.glob('{}/*.nc'.format(self.work_folder))
            self.file_list_iterator = iter(self.file_list)
            print ('{}\n#files: {}'.format(self.file_list, len(self.file_list)))
            
        elif response == Gtk.ResponseType.CANCEL:
            print("Cancel clicked")

        dialog.destroy()

    def on_folder_statistics(self):
        fcount=0
        pcount=[]
        for fn in self.file_list:
            fcount = fcount+1
            tmprootgrp = Dataset (fn, "r")
            print ('[{}] {} ..... Dims: {}, {} Ang @ {}, {}px'.format(fcount, fn,
                                                                        tmprootgrp['rangex'][0], tmprootgrp['rangey'][0],
                                                                        len(tmprootgrp['dimx']), len(tmprootgrp['dimy'])))
            nc_group='RegionTags'
            if nc_group in tmprootgrp.groups:
                tagsvar = tmprootgrp['/{}/JsonImageTags'.format(nc_group)]
                #print ('NCFile RegionTags found:\n', tagsvar[:], '\n')
                #print("Loading Tags... ", nc_group)
                for jstr in tagsvar:
                    if len(jstr) > 2:
                        label = json.loads (jstr)
                        #print (' {} [{}]'.format(label['description'], label['value']))
                        pcount.append(label['value'])
                print (' Label Counts: {}'.format(np.histogram (pcount, bins=range(0,100))[0]))

        print (pcount)
        print (' Label Counts: {}'.format(np.histogram (pcount, bins=range(0,100))))

                        
    def reload_clicked(self, widget):
        if os.path.isfile(self.cdf_image_data_filename):
            self.load_CDF ()
        else:
            print ('Sorry.')

    def color_map_swap_clicked(self, widget):
        #self.colormap = cm.RdYlGn
        self.colormap = cm.gray #Greys
        #self.colormap = cm.magma
        #cmaps['Sequential'] = [
        #    'Greys', 'Purples', 'Blues', 'Greens', 'Oranges', 'Reds',
        #    'YlOrBr', 'YlOrRd', 'OrRd', 'PuRd', 'RdPu', 'BuPu',
        #    'GnBu', 'PuBu', 'YlGnBu', 'PuBuGn', 'BuGn', 'YlGn']

        #cmaps['Perceptually Uniform Sequential'] = [
        #    'viridis', 'plasma', 'inferno', 'magma', 'cividis']

        #cmaps['Sequential (2)'] = [
        #    'binary', 'gist_yarg', 'gist_gray', 'gray', 'bone', 'pink',
        #    'spring', 'summer', 'autumn', 'winter', 'cool', 'Wistia',
        #    'hot', 'afmhot', 'gist_heat', 'copper']

            
    def next_clicked(self, widget):
        nnn = re.findall('[0-9]+',self.cdf_image_data_filename)
        tmp = self.cdf_image_data_filename.split(nnn[-1])
        fn = tmp[0]+'{0:03d}'.format(int(nnn[-1])+1)+tmp[1]
        print('Trying to load: {}'.format(fn))
        if os.path.isfile (fn):
            self.last_cdf_image_data_filename = self.cdf_image_data_filename
            self.cdf_image_data_filename = fn
            self.load_CDF ()

    def prev_clicked(self, widget):
        nnn = re.findall('[0-9]+',self.cdf_image_data_filename)
        tmp = self.cdf_image_data_filename.split(nnn[-1])
        fn = tmp[0]+'{0:03d}'.format(int(nnn[-1])-1)+tmp[1]
        if os.path.isfile (fn):
            self.last_cdf_image_data_filename = self.cdf_image_data_filename
            self.cdf_image_data_filename = fn
            self.load_CDF ()

    def next_in_list_clicked(self, widget):
        try:
            fn = next(self.file_list_iterator)
            if os.path.isfile (fn):
                self.last_cdf_image_data_filename = self.cdf_image_data_filename
                self.cdf_image_data_filename = fn
                self.load_CDF ()
        except:
            print ("END OF FILE LIST. RESTETTING TO START.")
            self.file_list_iterator = iter(self.file_list)
                
    def prev_in_list_clicked(self, widget):
        self.cdf_image_data_filename =  self.last_cdf_image_data_filename
        self.load_CDF ()
    
    def live_clicked(self, widget):
        if self.gxsm == None:
            self.gxsm = SocketClient(HOST, PORT)
        self.gxsm.request_autoupdate()
        JSONfn = self.gxsm.request_query_info_args('chfname', 0)
        ##{"result": [{"query": "chfname", "value": "/mnt/XX/BNLBox2T/Ilya-P38804/20191106-Bi2Se3/Bi2Se3-C60-322-M-Xp-Topo.nc"}]}
        fn = JSONfn['result'][0]['value']
        if os.path.isfile(fn):
            self.cdf_image_data_filename = fn
            self.load_CDF ()
        JSONfn = self.gxsm.request_query_info_args('y_current', 0)
        yi = JSONfn['result'][0]['value']
        print ("Current CH[0]: {}  AT Y-index={}".format(fn, yi))
        
    def start_clicked(self, widget):
        if self.gxsm == None:
            self.gxsm = SocketClient(HOST, PORT)
        ret = self.gxsm.request_start_scan()
        print(ret)
        
    def stop_clicked(self, widget):
        if self.gxsm == None:
            self.gxsm = SocketClient(HOST, PORT)
        ret = self.gxsm.request_stop_scan()
        print(ret)
        
    def autosave_clicked(self, widget):
        if self.gxsm == None:
            self.gxsm = SocketClient(HOST, PORT)
        ret=self.gxsm.request_autosave()
        print(ret)
        
    def autoupdate_clicked(self, widget):
        if self.gxsm == None:
            self.gxsm = SocketClient(HOST, PORT)
        ret=self.gxsm.request_autoupdate()
        print(ret)
        
    def getparam_clicked(self, widget):
        if self.gxsm == None:
            self.gxsm = SocketClient(HOST, PORT)

        for id in self.GXSM_params.keys():
            if self.GXSM_params[id][3]:
                ret = self.gxsm.request_gets_parameter(id)
                self.GXSM_params[id][2].set_text(str(ret['result'][0]['value']))
        
    def setparam_clicked(self, widget):
        if self.gxsm == None:
            self.gxsm = SocketClient(HOST, PORT)

        for id in self.GXSM_params.keys():
            if self.GXSM_params[id][3]:
                etxt = self.GXSM_params[id][2].get_text()
                #print (etxt.lstrip().split(' '))
                #print (etxt.lstrip().split(' ')[0])
                #print (str(float(etxt.lstrip().split(' ')[0])))
                ret = self.gxsm.request_set_parameter(id, str(float(etxt.lstrip().split(' ')[0])))
        
    def gxsmparam_changed3(self, widget, dum):
        self.gxsmparam_changed(widget)
        
    def gxsmparam_changed(self, widget):
        etxt = widget.get_text()
        for id in self.GXSM_params.keys():
            if self.GXSM_params[id][2] == widget:
                ret = self.gxsm.request_set_parameter(id, str(float(etxt.lstrip().split(' ')[0])))

    def clear_tags(self, nc_group):
        for p in self.axy.patches[:]:
            p.remove()
        for t in self.axy.texts[:]:
            t.remove()
        self.fig.canvas.draw()
        self.NCFile_tags[nc_group] = []
        self.tag_labels = []
        
    def clear_clicked(self, widget):
        self.clear_tags (self.tact_label_group)
        
    def process_quick_clicked(self, widget):
        x = range (0,self.nx)
        for y in self.ImageData[:]:
            A = np.vstack([x, np.ones(len(x))]).T
            m,c = np.linalg.lstsq(A, y)[0]
            for i in x:
                y[i] = y[i] - ( m*i + c )

        self.update_image(self.ImageData.max(), self.ImageData.min())

    def process_grad2(self):
        grad = np.gradient (self.ImageData)
        hx = np.histogram (grad[0], bins=256)
        hy = np.histogram (grad[1], bins=256)
        mx = hx[1][np.argmax(hx[0])]
        my = hy[1][np.argmax(hy[0])]
        self.ImageData = (grad[0]-mx)**2+(grad[1]-my)**2
        
    def process_grad2_clicked(self, widget):
        self.process_grad2()
        self.update_image(self.ImageData[2:-2,2:-2].max(), self.ImageData[2:-2,2:-2].min())
        
    def process_grad2m1_clicked(self, widget):
        self.process_grad2()
        gmax = self.ImageData[2:-2,2:-2].max()
        if gmax > 1.0:
            gmax = 1.0
        self.update_image (gmax, 0.0)
        
    def process_plane_clicked(self, widget):
        bg = self.plane_max_likely (self.ImageData)
        self.ImageData = self.ImageData + bg
        self.ImageData = self.ImageData - self.ImageData.min()
        self.update_image(self.ImageData.max(), self.ImageData.min())
        
    def plane_max_likely(self, patch):
        grad = np.gradient (patch)
        hx = np.histogram (grad[0], bins=256)
        hy = np.histogram (grad[1], bins=256)
        #print ('hx=',hx)
        #print ('hy=',hy)
        mx = hx[1][np.argmax(hx[0])]
        my = hy[1][np.argmax(hy[0])]
        print ('MaxProbGrad X at {} = {}'.format(np.argmax(hx[0]), hx[1][np.argmax(hx[0])]))
        print ('MaxProbGrad Y at {} = {}'.format(np.argmax(hy[0]), hy[1][np.argmax(hy[0])]))
        bg = np.empty(patch.shape)
        rz=0.0
        rx=0.0
        for row in bg[:]:  #self.ImageData[:]:  #patch:
            z=rz
            for i in range(0,patch.shape[1]):
                row[i] = z
                z=z+mx
            rz=rz+my
        s=bg.max() - bg.min()
        print ('bg scale: {} = {} - {}'.format(s, bg.max(), bg.min()))
        return bg

        
    def add_0tags_predict(self, n,m):
        if not self.AI:
            self.AI = brain() # init brain class (Keras)
        self.clear_clicked (None)
        self.subdivide_NxN (n,m)
        for x in np.arange(self.xr/n/2,self.xr*(n+0.5)/n, self.xr/n):
            for y in np.arange(self.yr/m/2,self.yr*(m+0.5)/m, self.yr/m):
                label = {'color': CAT_COLORS['0'],
                         'description': CAT_DESCRIPTIONS['0'],
                         'value':int(0),
                         'predictions': [],
                         'alpha': 0.25,
                         'region': { 'xy':[0.,0.,0.,0.],'ij':[0,0,0,0]},
                         'event-at': [x,y],
                         'M': m,
                         'N': n
                }
                self.tag_labels.append (label)
                self.tag(label)
        self.AI.clear_prediction_set ()
        self.on_ai_add_to_prediction (None, None)
        self.on_ai_evaluate (None, None)
        
    def subdivide2x2_clicked(self, widget):
        self.subdivide_NxN(2,2)
    def subdivide3x3_clicked(self, widget):
        self.subdivide_NxN(3,3)
    def subdivide4x4_clicked(self, widget):
        self.subdivide_NxN(4,4)
    def subdivide5x5_clicked(self, widget):
        self.subdivide_NxN(5,5)

    def subdivide2x2ai_clicked(self, widget):
        self.add_0tags_predict (2,2)
    def subdivide3x3ai_clicked(self, widget):
        self.add_0tags_predict (3,3)
    def subdivide4x4ai_clicked(self, widget):
        self.add_0tags_predict (4,4)
    def subdivide5x5ai_clicked(self, widget):
        self.add_0tags_predict (5,5)
            

    def subdivide_NxN(self, N,M):
        self.subN=N
        self.subM=M
        #self.axy.cla ()
        self.ImageData = self.ImageData - self.ImageData.min()
        for p in self.axy.patches[:]:
            p.remove()
        
        for i,j in list(np.ndindex(N,M)):

            i0 = int (i*self.nx/N+1)
            j0 = int (j*self.ny/M+1)
            iN = int (self.nx/N-2)
            jN = int (self.ny/M-2)

            x0  = i*self.xr/N*1.01
            y0  = j*self.yr/M*1.01
            xw  = self.xr/N*0.98
            yw  = self.yr/M*0.98

            #ImPatch = self.ImageData[j0:j0+jN , i0:i0+iN]
            #vmin = ImPatch.min()
            ## self.ImageData[j0:j0+jN , i0:i0+iN] =  self.ImageData[j0:j0+jN , i0:i0+iN] - vmin
            
            self.add_color_patch (N,M, i,j, 'Grey', 0.2)

        self.update_image(self.ImageData.max(), self.ImageData.min())

    def tag(self, label, update=True):
        x,y = label['event-at']
        i = int(label['N']*x/self.xr)
        j = int(label['M']*y/self.yr)
        label['region'] = self.add_color_patch(label['N'], label['M'], i,j, label['color'], label['alpha'])
        print (len(label['predictions']), label['value'])
        if len(label['predictions'])-1 >= int(label['value']):
            self.axy.text(x,y, '{0} ({1:.2f})'.format(label['value'], label['predictions'][int(label['value'])]))
        else:
            self.axy.text(x,y, '{}'.format(label['value']))
        if update:
            self.fig.canvas.draw()
        
    def add_color_patch(self, N,M, i,j, color, alpha=0.1):
        x0  = i*self.xr/N*1.01
        y0  = j*self.yr/M*1.01
        xw  = self.xr/N*0.98
        yw  = self.yr/M*0.98
        self.axy.add_patch(Rectangle((x0,y0), xw, yw, facecolor=color, alpha=alpha))

        i0 = int (i*self.nx/N+1)
        j0 = int (j*self.ny/M+1)
        iN = int (self.nx/N-2)
        jN = int (self.ny/M-2)
        
        return { 'xy':[x0,y0,xw,yw], 'ij':[i0,self.ny-j0-jN,iN,jN] }
        
    def do_scale(self, x,y, pn):
        self.rect[pn][0] = int(self.nx*x/self.xr)
        self.rect[pn][1] = self.ny-int(self.ny*y/self.yr)
        if pn == 'p2':
            tmp = self.rect['p1'][1]
            self.rect['p1'][1] = min(self.rect['p1'][1], self.rect['p2'][1])
            self.rect['p2'][1] = max(tmp, self.rect['p2'][1])
            tmp = self.rect['p1'][0]
            self.rect['p1'][0] = min(self.rect['p1'][0],self.rect['p2'][0])
            self.rect['p2'][0] = max(tmp,self.rect['p2'][0])
            print ('Rect:', self.rect)
            if self.rect['p2'][1] - self.rect['p1'][1] > 0 and self.rect['p2'][0]-self.rect['p1'][0] > 0:
                ImRefRect = self.ImageData[self.rect['p1'][1]:self.rect['p2'][1], self.rect['p1'][0]:self.rect['p2'][0]]
                print (ImRefRect.max(), ImRefRect.min())
                self.update_image (ImRefRect.max(), ImRefRect.min())
            
    def load_CDF(self):
        print("NetCDF File: ", self.cdf_image_data_filename)
        if self.rootgrp:
            self.rootgrp.close()
        self.tag_labels = []
        
        self.rootgrp = Dataset (self.cdf_image_data_filename, "r+")
        print(self.rootgrp['FloatField'])

        self.ImageData = self.rootgrp['FloatField'][0][0][:][:]
        print (self.ImageData)
        self.Xlookup = self.rootgrp['dimx'][:]
        self.Ylookup = self.rootgrp['dimy'][:]
        print('Bias    = ', self.rootgrp['sranger_mk2_hwi_bias'][0], self.rootgrp['sranger_mk2_hwi_bias'].var_unit)
        self.ub = self.rootgrp['sranger_mk2_hwi_bias'][0]
        self.bias.set_text ('{0:.4f} '.format(self.rootgrp['sranger_mk2_hwi_bias'][0])+self.rootgrp['sranger_mk2_hwi_bias'].var_unit)

        if 'sranger_mk2_hwi_mix0_set_point' in self.rootgrp.variables:
            print('Current = ', self.rootgrp['sranger_mk2_hwi_mix0_set_point'][0], self.rootgrp['sranger_mk2_hwi_mix0_set_point'].var_unit)
            self.cur = self.rootgrp['sranger_mk2_hwi_mix0_set_point'][0]
            self.current.set_text ('{0:.4f} '.format(self.rootgrp['sranger_mk2_hwi_mix0_set_point'][0])+self.rootgrp['sranger_mk2_hwi_mix0_set_point'].var_unit)

        if 'sranger_mk2_hwi_mix0_current_set_point' in self.rootgrp.variables:
            print('Current (old name) = ', self.rootgrp['sranger_mk2_hwi_mix0_current_set_point'][0], self.rootgrp['sranger_mk2_hwi_mix0_current_set_point'].var_unit)
            self.cur = self.rootgrp['sranger_mk2_hwi_mix0_current_set_point'][0]
            self.current.set_text ('{0:.4f} '.format(self.rootgrp['sranger_mk2_hwi_mix0_current_set_point'][0])+self.rootgrp['sranger_mk2_hwi_mix0_current_set_point'].var_unit)

            
        print('X Range = ', self.rootgrp['rangex'][0], self.rootgrp['rangex'].var_unit)
        self.xr = self.rootgrp['rangex'][0]
        self.Xrange.set_text ('{0:.1f} '.format(self.rootgrp['rangex'][0])+self.rootgrp['rangex'].var_unit)
        
        print('Y Range = ', self.rootgrp['rangey'][0], self.rootgrp['rangey'].var_unit)
        self.yr = self.rootgrp['rangey'][0]
        self.Yrange.set_text ('{0:.1f} '.format(self.rootgrp['rangey'][0])+self.rootgrp['rangey'].var_unit)

        print('Nx      = ', len(self.rootgrp['dimx']))
        self.nx = len(self.rootgrp['dimx'])
        self.Nx.set_text ('{} px'.format(self.nx))
        print('Ny      = ', len(self.rootgrp['dimy']))
        self.ny = len(self.rootgrp['dimy'])
        self.Ny.set_text ('{} px'.format(self.ny))

        print('dx      = ', self.rootgrp['dx'][0], self.rootgrp['dx'].var_unit)
        self.dx = self.rootgrp['dx'][0]
        self.dX.set_text ('{0:.4f} '.format(self.rootgrp['dx'][0])+gxsm_units[self.rootgrp['dx'].unit][0])
        print('dy      = ', self.rootgrp['dy'][0], gxsm_units[self.rootgrp['dy'].unit][0])
        self.dy = self.rootgrp['dy'][0]
        self.dY.set_text ('{0:.4f} '.format(self.rootgrp['dy'][0])+gxsm_units[self.rootgrp['dy'].unit][0])
        print('dz      = ', self.rootgrp['dz'][0], gxsm_units[self.rootgrp['dz'].unit][0])
        self.dz = self.rootgrp['dz'][0]
        self.dZ.set_text ('{0:.4f} '.format(self.rootgrp['dz'][0])+gxsm_units[self.rootgrp['dz'].unit][0])
        self.ZR.set_text ('{0:.2f} '.format(self.ImageData.max()-self.ImageData.min())+gxsm_units[self.rootgrp['dz'].unit][0])

        self.ImageData = self.ImageData * self.dz # scale to unit
        
        for p in self.axy.patches[:]:
            p.remove()
        
        # create/update figure and image
        self.update_image(self.ImageData.max(), self.ImageData.min())
        self.axy.set_title('...'+self.cdf_image_data_filename[-40:]) # -45 for 800px
        self.axy.set_xlabel('X in '+gxsm_units[self.rootgrp['dx'].unit][0])
        self.axy.set_ylabel('Y in '+gxsm_units[self.rootgrp['dy'].unit][0])
        if self.cbar:
            self.cbar.remove () #fig.delaxes(self.figure.axes[1])
        self.cbar = self.fig.colorbar(self.im, extend='both', shrink=0.9, ax=self.axy)
        self.cbar.set_label('Z in '+gxsm_units[self.rootgrp['dz'].unit][0])
        self.fig.canvas.draw()
        print ('NCLoad tags from group {}'.format (self.tact_label_group))
        self.load_tags_from_netcdf (self.tact_label_group)

    def load_tags_from_netcdf (self, nc_group='RegionTags'):
        self.clear_tags (nc_group)
        if nc_group in self.rootgrp.groups:
            #print (self.rootgrp.groups['RegionTags'])
            tagsvar = self.rootgrp['/{}/JsonImageTags'.format(nc_group)]
            #print ('NCFile RegionTags found:\n', tagsvar[:], '\n')
            print("Loading Tags... ", nc_group)
            for jstr in tagsvar:
                if len(jstr) > 2:
                    self.NCFile_tags[nc_group].append (jstr)
                    label = json.loads (jstr)
                    self.tag_labels.append (label)
                    print (label)
                    self.tag (label, False)
        self.fig.canvas.draw()



    def update_image(self, vmax, vmin):
    
        self.im=self.axy.imshow(self.ImageData, interpolation='bilinear', cmap=self.colormap,
                           origin='upper', extent=[0, self.xr, 0, self.yr],
                           vmax=vmax, vmin=vmin)
        if self.cbar:
            self.cbar.remove () #fig.delaxes(self.figure.axes[1])
        self.cbar = self.fig.colorbar(self.im, extend='both', shrink=0.9, ax=self.axy)
        self.cbar.set_label('Z in '+gxsm_units[self.rootgrp['dz'].unit][0])
        self.fig.canvas.draw()

        
    def list_CDF_clicked(self, widget):
        print("NetCDF File: ", self.cdf_image_data_filename)
        self.rootgrp = Dataset (self.cdf_image_data_filename, "r")
        print(self.rootgrp.data_model)
        print(self.rootgrp.groups)
        print(self.rootgrp.dimensions)
        print(self.rootgrp.variables)

        print('========================================')
        print(self.rootgrp['FloatField'])

        #print(self.rootgrp['dimx'])
        #print(self.rootgrp['dimx'][:])
        self.Xlookup = self.rootgrp['dimx'][:]

        #print(self.rootgrp['dimy'])
        #print(self.rootgrp['dimy'][:])
        self.Ylookup = self.rootgrp['dimy'][:]
        
        #print(self.rootgrp['sranger_mk2_hwi_bias'])
        print('Bias = ', self.rootgrp['sranger_mk2_hwi_bias'][0], gxsm_units[self.rootgrp['sranger_mk2_hwi_bias'].var_unit][0])

        if 'sranger_mk2_hwi_mix0_set_point' in self.rootgrp.variables:
            #print(self.rootgrp['sranger_mk2_hwi_mix0_set_point'])
            print('Current = ', self.rootgrp['sranger_mk2_hwi_mix0_set_point'][0], gxsm_units[self.rootgrp['sranger_mk2_hwi_mix0_set_point'].var_unit][0])

        if 'sranger_mk2_hwi_mix0_current_set_point' in self.rootgrp.variables:
            print('Current (old name) = ', self.rootgrp['sranger_mk2_hwi_mix0_current_set_point'][0], self.rootgrp['sranger_mk2_hwi_mix0_current_set_point'].var_unit)
        

        #print(self.rootgrp['rangex'])
        print('X Range = ', self.rootgrp['rangex'][0], gxsm_units[self.rootgrp['rangex'].unit][0])
        #print(self.rootgrp['rangey'])
        print('Y Range = ', self.rootgrp['rangey'][0], gxsm_units[self.rootgrp['rangey'].unit][0])
        #print(self.rootgrp['dz'])
        print('dz = ', self.rootgrp['dz'][0], gxsm_units[self.rootgrp['dz'].unit][0])

        if 'RegionTags' in self.rootgrp.groups:
            print (self.rootgrp.groups['RegionTags'])
            print (chartostring(self.rootgrp['/RegionTags/JsonImageTag0'][:]))

        #self.rootgrp.close()

        
    def on_change_action_state(self, action, value):
        action.set_state(value)
        self.click_action = value.get_string()
        self.act_label.set_text(value.get_string())

    def taglabel_clicked(self, widget):
        for tagchild in widget.get_children():
            self.taglabel = tagchild.get_label().split('*')[1]
            print (self.taglabel)
        
    def on_change_taglabel_state(self, action, value):
        action.set_state(value)
        #self.taglabelbutton.set_text(value.get_string())
        for i in range(self.numhistorytaglabels-1, self.start_taglabels, -1):
            for child in self.taglabelbutton[i].get_children():
                for cp in self.taglabelbutton[i-1].get_children():
                    child.set_label(cp.get_label())
                    child.set_use_markup(True)

        for child in self.taglabelbutton[self.start_taglabels].get_children():
            child.set_label('<big><b><span background="{}"> *{}* </span></b></big>'.format(CAT_COLORS[value.get_string().split(':')[0]], value.get_string()))
            child.set_use_markup(True)
        self.taglabelbutton[self.start_taglabels].set_active (True)
        self.taglabel = value.get_string()
        print (self.taglabel)
            
    def on_change_tagdest_state(self, action, value):
        action.set_state (value)
        self.tact_label.set_text (value.get_string())
        self.tact_label_group = value.get_string()
            
    def on_maximize_toggle(self, action, value):
        action.set_state(value)
        if value.get_boolean():
            self.maximize()
        else:
            self.unmaximize()

    ## TAGS -- Managing
    def on_tags_clear(self, action, param):
        self.clear_clicked (None)

    def on_tags_store(self, action, param):
        nc_group=self.tact_label_group
        NCFile_tags = []
        for label in self.tag_labels:
            tmp = label
            tmp['predictions'] = [] # remove predictions ndarray, not JSON complatible: TypeError: Object of type ndarray is not JSON serializable
            jstr = json.dumps (label)
            NCFile_tags.append (jstr)
        print (NCFile_tags)
        self.store_tags_to_netcdf (NCFile_tags, nc_group)
        
    def on_tags_load(self, action, param):
        self.load_tags_from_netcdf (self.tact_label_group)
    
    ## GXSM AI/BRAIN -- KERAS -- Data Managing
    def on_ai_auto_add_folder_to_training_and_validation(self, action, param):
        if not self.AI:
            self.AI = brain() # init brain class (Keras)

        for fn in self.file_list:
            if os.path.isfile (fn):
                self.cdf_image_data_filename = fn
                self.load_CDF ()
                print ('pre-processing image grad2')
                self.process_grad2 ()
                self.ImageData = self.ImageData*30.
                self.update_image(self.ImageData[2:-2,2:-2].max(), self.ImageData[2:-2,2:-2].min())
                for label in self.tag_labels:
                    print ("preparing: {}".format(label))
                    ij = label['region']['ij']
                    ImPatch = self.ImageData[ij[1] : ij[1]+ij[3], ij[0] : ij[0]+ij[2]]
                    vmin = ImPatch.min()
                    patch = ImPatch - vmin
                    if random.randint(1,100) > 66:
                        self.AI.add_to_training_set (patch, label)
                    else:
                        self.AI.add_to_validation_set (patch, label)

    def on_ai_add_to_training(self, action, param):
        if not self.AI:
            self.AI = brain() # init brain class (Keras)

        for label in self.tag_labels:
            print ("preparing: {}".format(label))
            ij = label['region']['ij']
            ImPatch = self.ImageData[ij[1] : ij[1]+ij[3], ij[0] : ij[0]+ij[2]]
            vmin = ImPatch.min()
            patch = ImPatch - vmin                         
            self.AI.add_to_training_set (patch, label)

    def on_ai_add_to_validation(self, action, param):
        if not self.AI:
            self.AI = brain() # init brain class (Keras)

        for label in self.tag_labels:
            print ("preparing: {}".format(label))
            ij = label['region']['ij']
            ImPatch = self.ImageData[ij[1] : ij[1]+ij[3], ij[0] : ij[0]+ij[2]]
            vmin = ImPatch.min()
            patch = ImPatch - vmin                         
            self.AI.add_to_validation_set (patch, label)

    def on_ai_add_to_prediction(self, action, param):
        if not self.AI:
            self.AI = brain() # init brain class (Keras)

        for label in self.tag_labels:
            print ("preparing: {}".format(label))
            ij = label['region']['ij']
            ImPatch = self.ImageData[ij[1] : ij[1]+ij[3], ij[0] : ij[0]+ij[2]]
            vmin = ImPatch.min()
            patch = ImPatch - vmin                         
            self.AI.add_to_prediction_set (patch, label)

    def on_ai_clear_prediction_set(self, action, param):
        self.AI.clear_prediction_set ()
        
    def on_ai_train(self, action, param):
        if not self.AI:
            print ("Please add training and validation sets first!")
            return
        self.AI.start_training ()
        
    def on_ai_evaluate(self, action, param):
        if not self.AI:
            print ("Please load brain first!")
            return
        labels = self.AI.do_prediction ()
        self.clear_clicked (None)
        for label in labels:
            self.tag_labels.append (label)
            self.tag (label)
        
    def on_ai_store_data(self, action, param):
        if not self.AI:
            print ("Nothing to store at this time!")
            return
        self.AI.store_datasets ()

    def on_ai_load_data(self, action, param):
        if not self.AI:
            self.AI = brain () # init brain class (Keras)
        self.AI.load_datasets ()

    def on_ai_store(self, action, param):
        if not self.AI:
            print ("Nothing to store at this time!")
            return
        self.AI.store_brain ()

    def on_ai_load(self, action, param):
        if not self.AI:
            self.AI = brain () # init brain class (Keras)
        self.AI.load_brain ()

############################################################
# G Application Core
############################################################

            
class Application(Gtk.Application):

    def __init__(self, *args, **kwargs):
        super().__init__(*args, application_id="org.gnome.gxsm-sok-ai-app",
                         flags=Gio.ApplicationFlags.HANDLES_COMMAND_LINE,
                         **kwargs)
        self.window = None

        self.add_main_option("test", ord("t"), GLib.OptionFlags.NONE,
                             GLib.OptionArg.NONE, "Command line test", None)

    def do_startup(self):
        Gtk.Application.do_startup(self)

        ## File Menu Actions
        
        action = Gio.SimpleAction.new("connect", None)
        action.connect("activate", self.on_connect_gxsm)
        self.add_action(action)

        action = Gio.SimpleAction.new("open", None)
        action.connect("activate", self.on_open_netcdf)
        self.add_action(action)

        action = Gio.SimpleAction.new("folder", None)
        action.connect("activate", self.on_set_folder)
        self.add_action(action)

        action = Gio.SimpleAction.new("statistics", None)
        action.connect("activate", self.on_folder_statistics)
        self.add_action(action)

        action = Gio.SimpleAction.new("list", None)
        action.connect("activate", self.on_list_netcdf)
        self.add_action(action)

        action = Gio.SimpleAction.new("about", None)
        action.connect("activate", self.on_about)
        self.add_action(action)

        action = Gio.SimpleAction.new("quit", None)
        action.connect("activate", self.on_quit)
        self.add_action(action)
        
        builder = Gtk.Builder.new_from_string(MENU_XML, -1)
        self.set_app_menu(builder.get_object("app-menu"))
        self.set_menubar(builder.get_object("app-menubar"))
        
    def do_activate(self):
        # We only allow a single window and raise any existing ones
        if not self.window:
            # Windows are associated with the application
            # when the last one is closed the application shuts down
            self.window = AppWindow(application=self, title=APP_TITLE)

        self.window.present()

    def do_command_line(self, command_line):
        options = command_line.get_options_dict()
        # convert GVariantDict -> GVariant -> dict
        options = options.end().unpack()

        if "test" in options:
            # This is printed on the main instance
            print("Test argument recieved: %s" % options["test"])

        self.activate()
        return 0


    def on_connect_gxsm(self, action, param):
        self.window.connect_clicked(None)
        
    def on_open_netcdf(self, action, param):
        self.window.on_file_clicked(None)

    def on_set_folder(self, action, param):
        self.window.on_folder_clicked(None)

    def on_folder_statistics(self, action, param):
        self.window.on_folder_statistics()

    def on_list_netcdf(self, action, param):
        self.window.list_CDF_clicked(None)
    
    def on_about(self, action, param):
        about_dialog = Gtk.AboutDialog(transient_for=self.window, modal=True,
                                       program_name="gxsm-scan-image-viewer.py: Gxsm3 Remote AI Client Tag Tool",
                                       authors=["Percy Zahl"],
                                       #documenters = ["--"],
                                       copyright="GPL",
                                       website="http://www.gxsm.sf.net",
                                       logo=GdkPixbuf.Pixbuf.new_from_file_at_scale("./gxsm3-icon.svg", 200,200, True),
                                       )
        about_dialog.set_copyright(
            "Copyright \xc2\xa9 2019 Percy Zahl.\n"
            "This is a experimental GXSM3 Remote Control Client,\n"
            "a Gxsm Scan Data NetCDF4 Image Viewer\n"
            "and Region Tagging/Image Categorization Tool\n"
            "...with future explorative AI Client functionality."
        )
        about_dialog.present()

    def on_quit(self, action, param):
        self.quit()


        
if __name__ == "__main__":
    print("Num Tensorflow GPUs Available: ", len(tf.config.experimental.list_physical_devices('GPU')))
    app = Application()
    app.run(sys.argv)
