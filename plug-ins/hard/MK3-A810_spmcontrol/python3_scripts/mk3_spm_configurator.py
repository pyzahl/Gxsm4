#!/usr/bin/env python3

## * Python User Control for
## * Configuration and SPM Approach Control tool for the FB_spm DSP program/GXSM2 Mk3-Pro/A810 based
## * for the Signal Ranger STD/SP2 DSP board
## * 
## * Copyright (C) 2010-13 Percy Zahl
## *
## * Author: Percy Zahl <zahl@users.sf.net>
## * WWW Home: http://sranger.sf.net
## *
## * This program is free software; you can redistribute it and/or modify
## * it under the terms of the GNU General Public License as published by
## * the Free Software Foundation; either version 2 of the License, or
## * (at your option) any later version.
## *
## * This program is distributed in the hope that it will be useful,
## * but WITHOUT ANY WARRANTY; without even the implied warranty of
## * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## * GNU General Public License for more details.
## *
## * You should have received a copy of the GNU General Public License
## * along with this program; if not, write to the Free Software
## * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.

#### get numpy, scipy:
#### sudo apt-get install python-numpy python-scipy python-matplotlib ipython ipython-notebook python-pandas python-sympy python-nose

import gi
gi.require_version("Gtk", "3.0")
from gi.repository import Gtk, GLib

import time

import struct
import array

from numpy import *

import math
import re

import pickle
import pydot
#import xdot
from scipy.optimize import leastsq

from mk3_spmcontol_class import *
from meterwidget import *
from scopewidget import *

import datetime

OSZISCALE = 0.66

# timeouts [ms]
timeout_update_patchrack              = 3000
timeout_update_patchmenu              = 3000
timeout_update_A810_readings          =  120
timeout_update_signal_monitor_menu    = 2000
timeout_update_signal_monitor_reading =  500
timeout_update_meter_readings         =   88
timeout_update_process_list           =  100

timeout_min_recorder          =   80
timeout_min_tunescope         =   20
timeout_tunescope_default     =  150

timeout_DSP_status_reading        =  100
timeout_DSP_signal_lookup_reading = 2100


## Si Diode Curve 10 -- i.e for DT-400
## [ T(K), Volts, dV/dT (mV/K) ]
curve_10 = [
  [  1.40, 1.69812, -13.1 ],
  [  1.60, 1.69521, -15.9 ],
  [  1.80, 1.69177, -18.4 ],
  [  2.00, 1.68786, -20.7 ],
  [  2.20, 1.68352, -22.7 ],
  [  2.40, 1.67880, -24.4 ],
  [  2.60, 1.67376, -25.9 ],
  [  2.80, 1.66845, -27.1 ],
  [  3.00, 1.66292, -28.1 ],
  [  3.20, 1.65721, -29.0 ],
  [  3.40, 1.65134, -29.8 ],
  [  3.60, 1.64529, -30.7 ],
  [  3.80, 1.63905, -31.6 ],
  [  4.00, 1.63263, -32.7 ],
  [  4.20, 1.62602, -33.6 ],
  [  4.40, 1.61920, -34.6 ],
  [  4.60, 1.61220, -35.4 ],
  [  4.80, 1.60506, -36.0 ],
  [  5.00, 1.59782, -36.5 ],
  [  5.50, 1.57928, -37.6 ],
  [  6.00, 1.56027, -38.4 ],
  [  6.50, 1.54097, -38.7 ],
  [  7.00, 1.52166, -38.4 ],
  [  7.50, 1.50272, -37.3 ],
  [  8.00, 1.48443, -35.8 ],
  [  8.50, 1.46700, -34.0 ],
  [  9.00, 1.45048, -32.1 ],
  [  9.50, 1.43488, -30.3 ],
  [  10.0, 1.42013, -28.7 ],
  [  10.5, 1.40615, -27.2 ],
  [  11.0, 1.39287, -25.9 ],
  [  11.5, 1.38021, -24.8 ],
  [  12.0, 1.36809, -23.7 ],
  [  12.5, 1.35647, -22.8 ],
  [  13.0, 1.34530, -21.9 ],
  [  13.5, 1.33453, -21.2 ],
  [  14.0, 1.32412, -20.5 ],
  [  14.5, 1.31403, -19.9 ],
  [  15.0, 1.30422, -19.4 ],
  [  15.5, 1.29464, -18.9 ],
  [  16.0, 1.28527, -18.6 ],
  [  16.5, 1.27607, -18.2 ],
  [  17.0, 1.26702, -18.0 ],
  [  17.5, 1.25810, -17.7 ],
  [  18.0, 1.24928, -17.6 ],
  [  18.5, 1.24053, -17.4 ],
  [  19.0, 1.23184, -17.4 ],
  [  19.5, 1.22314, -17.4 ],
  [  20.0, 1.21440, -17.6 ],
  [  21.0, 1.19645, -18.5 ],
  [  22.0, 1.17705, -20.6 ],
  [  23.0, 1.15558, -21.7 ],
  [  24.0, 1.13598, -15.9 ],
  [  25.0, 1.12463, -7.72 ],
  [  26.0, 1.11896, -4.34 ],
  [  27.0, 1.11517, -3.34 ],
  [  28.0, 1.11212, -2.82 ],
  [  29.0, 1.10945, -2.53 ],
  [  30.0, 1.10702, -2.34 ],
  [  32.0, 1.10263, -2.08 ],
  [  34.0, 1.09864, -1.92 ],
  [  36.0, 1.09490, -1.83 ],
  [  38.0, 1.09131, -1.77 ],
  [  40.0, 1.08781, -1.74 ],
  [  42.0, 1.08436, -1.72 ],
  [  44.0, 1.08093, -1.72 ],
  [  46.0, 1.07748, -1.73 ],
  [  48.0, 1.07402, -1.74 ],
  [  50.0, 1.07053, -1.75 ],
  [  52.0, 1.06700, -1.77 ],
  [  54.0, 1.06346, -1.78 ],
  [  56.0, 1.05988, -1.79 ],
  [  58.0, 1.05629, -1.80 ],
  [  60.0, 1.05267, -1.81 ],
  [  65.0, 1.04353, -1.84 ],
  [  70.0, 1.03425, -1.87 ],
  [  75.0, 1.02482, -1.91 ],
  [  77.35, 1.02032, -1.92 ],
  [  80.0, 1.01525, -1.93 ],
  [  85.0, 1.00552, -1.96 ],
  [  90.0, 0.99565, -1.99 ],
  [  95.0, 0.98564, -2.02 ],
  [  100.0, 0.97550, -2.04 ],
  [  110.0, 0.95487, -2.08 ],
  [  120.0, 0.93383, -2.12 ],
  [  130.0, 0.91243, -2.16 ],
  [  140.0, 0.89072, -2.19 ],
  [  150.0, 0.86873, -2.21 ],
  [  160.0, 0.84650, -2.24 ],
  [  170.0, 0.82404, -2.26 ],
  [  180.0, 0.80138, -2.28 ],
  [  190.0, 0.77855, -2.29 ],
  [  200.0, 0.75554, -2.31 ],
  [  210.0, 0.73238, -2.32 ],
  [  220.0, 0.70908, -2.34 ],
  [  230.0, 0.68564, -2.35 ],
  [  240.0, 0.66208, -2.36 ],
  [  250.0, 0.63841, -2.37 ],
  [  260.0, 0.61465, -2.38 ],
  [  270.0, 0.59080, -2.39 ],
  [  273.15, 0.58327, -2.39 ],
  [  280.0, 0.56690, -2.39 ],
  [  290.0, 0.54294, -2.40 ],
  [  300.0, 0.51892, -2.40 ],
  [  305.0, 0.50688, -2.41 ],
  [  310.0, 0.49484, -2.41 ],
  [  320.0, 0.47069, -2.42 ],
  [  330.0, 0.44647, -2.42 ],
  [  340.0, 0.42221, -2.43 ],
  [  350.0, 0.39783, -2.44 ],
  [  360.0, 0.37337, -2.45 ],
  [  370.0, 0.34881, -2.46 ],
  [  380.0, 0.32416, -2.47 ],
  [  390.0, 0.29941, -2.48 ],
  [  400.0, 0.27456, -2.49 ],
  [  410.0, 0.24963, -2.50 ],
  [  420.0, 0.22463, -2.50 ],
  [  430.0, 0.19961, -2.50 ],
  [  440.0, 0.17464, -2.49 ],
  [  450.0, 0.14985, -2.46 ],
  [  460.0, 0.12547, -2.41 ],
  [  470.0, 0.10191, -2.30 ],
  [  475.0, 0.09062, -2.22 ]
]


## [ T(K), Volts, dV/dT (mV/K) ]
def v2k(v):
    for dtv in curve_10:
        dv = v-dtv[1]
        if dv >= 0.0:
            return dtv[0] + dv*1000./dtv[2]
    return 0.0

#np_v2k = np.vectorize(v2k)


# class MyDotWindow(xdot.DotWindow):
#
#         def __init__(self):
#                 xdot.DotWindow.__init__(self)
#                 self.widget.connect('clicked', self.on_url_clicked)
#
#         def on_url_clicked(self, widget, url, event):
#                 dialog = Gtk.MessageDialog(
#                         parent = self, 
#                         buttons = Gtk.BUTTONS_OK,
#                         message_format="%s clicked" % url)
#                 dialog.connect('response', lambda dialog, response: dialog.destroy())
#                 dialog.run()
#                 return True

class Visualize():
        def __init__(self, parent, shownullsignals=0):
                # http://www.graphviz.org/Documentation.php
                graph = pydot.Dot(graph_type='digraph', compound='true', rankdir="LR", overlap="false") #, splines="ortho")

                moduleg = {}
                signaln = {}
                inputn = {}
                node_id = 0

                moduleg ["NodeTypes"] = pydot.Cluster(graph_name="Legend",
                                                      style="filled",
                                                      color="lightgrey",
                                                      label = "Node Types:")
                c = moduleg ["NodeTypes"]
                graph.add_subgraph (c)
                c.add_node ( pydot.Node( "Process Flow", style="filled", fillcolor="lightskyblue") )
                c.add_node ( pydot.Node( "Signal Source", style="filled", fillcolor="green") )
                c.add_node ( pydot.Node( "Modul Input", style="filled", fillcolor="gold") )
                c.add_node ( pydot.Node( "Modul Input=0", style="filled", fillcolor="grey") )
                c.add_node ( pydot.Node( "DISABLED (NULL Ptr)", style="filled", fillcolor="grey97") )
                c.add_node ( pydot.Node( "Signal Manipulation", style="filled", fillcolor="cyan") )
                c.add_node ( pydot.Node( "Unmanaged Node (PLL, ..)", style="filled", fillcolor="purple") )
                c.add_node ( pydot.Node( "Error Signal not found", style="filled", fillcolor="red") )

                mod_conf =  parent.mk3spm.get_module_configuration_list()
                for mod in mod_conf.keys():
                        moduleg [mod] = pydot.Cluster(graph_name=mod,
                                                      style="filled",
                                                      color="lightgrey",
                                                      label = mod)
                        c = moduleg [mod]
                        print ("AddSubgraph: "+mod)
                        graph.add_subgraph (c)
                        signaln [mod] = pydot.Node( mod, style="filled", fillcolor="lightskyblue")
                        c.add_node( signaln [mod] )

                        record = ""
                        fi=1
                        for signal in parent.mk3spm.get_signal_lookup_list ():
                                if signal[SIG_ADR] > 0:
                                        if signal[SIG_GRP] == mod:
                                                if re.search ("In\ [0-7]", signal[SIG_NAME]):
                                                        if record == "":
                                                                record = "<f0> "+re.escape (signal[SIG_NAME])
                                                        else:
                                                                record = record + "|<f%d> "%fi + re.escape (signal[SIG_NAME])
                                                                fi=fi+1
                                                        print ("added to record: " + signal[SIG_NAME] + " ==> " + record)
                                                signaln [signal[SIG_NAME]] = pydot.Node( signal[SIG_NAME], style="filled", fillcolor="green")
                                                c.add_node( signaln [signal[SIG_NAME]] )
#                        if record != "":
#                                signaln ["record_IN"] = pydot.Node ("record_IN", label=record, shape="record", style="filled", fillcolor="green")
#                                c.add_node( signaln ["record_IN"] )

                for mod in mod_conf.keys():
                        c = moduleg [mod]
                        for mod_inp in mod_conf[mod]:
                                [signal, data, offset] = parent.mk3spm.query_module_signal_input(mod_inp[0])
                                # Null-Signal -- omit node connections, shade node
                                if signal[SIG_VAR] == "analog.vnull":
                                        nodecolor="grey"
                                else:
                                        nodecolor=mod_inp[4]
                                print ("INPUT NODE: ", mod_inp)
                                if mod_inp[2] >= 0 and signal[SIG_INDEX] != -1:
                                        inputn [mod_inp[1]] = pydot.Node( mod_inp[1], style="filled", fillcolor=nodecolor)
                                        c.add_node( inputn [mod_inp[1]] )
                                        print (signal[SIG_NAME]," ==> ",mod_inp[1],"     (",mod_inp, signal, data, ")")
                                        if signal[SIG_VAR] != "analog.vnull" or shownullsignals:
                                                if nodecolor == "cyan":
                                                        sigdir="back"
                                                        sigcol="magenta"
                                                else:
                                                        sigdir="forward"
                                                        sigcol="red"
                                                edge = pydot.Edge(signaln [signal[SIG_NAME]], inputn [mod_inp[1]], color=sigcol, dir=sigdir)
                                                graph.add_edge(edge)
                                                if re.search ("In\ [0-7]", signal[SIG_NAME]):
                                                        signal_f = "f%s"%re.sub("In ","",signal[SIG_NAME])
                                                        edge = pydot.Edge("record_IN:f0", inputn [mod_inp[1]], color=sigcol, dir=sigdir)

                                else:
                                        if mod_inp[3] == 1:
                                                print ("DISABLED [p=0] ==> ",mod_inp[1],"  (",mod_inp, signal, data, ")")
                                                inputn [mod_inp[1]] = pydot.Node( mod_inp[1], style="filled", fillcolor="grey97")
                                        else:
                                                if mod_inp[4] == "purple":
                                                        print ("UNMANAGED NODE (PLL, etc.) ==> ", mod_inp[1],"  (",mod_inp, signal, data, ")")
                                                        inputn [mod_inp[1]] = pydot.Node( mod_inp[1], style="filled", fillcolor=mod_inp[4])
                                                else:
                                                        print ("ERROR: ",mod_inp, signal, data, " ** can not find signal.")
                                                        inputn [mod_inp[1]] = pydot.Node( mod_inp[1], style="filled", fillcolor="red")
                                        c.add_node( inputn [mod_inp[1]] )

                # processing order
                edge = pydot.Edge(signaln ["DSP"], signaln ["Analog_IN"], arrowhead="empty", color="green", weight="1000")
                graph.add_edge(edge)        
                edge = pydot.Edge(signaln ["Analog_IN"], signaln ["PAC"], arrowhead="empty", color="green", weight="1000")
                graph.add_edge(edge)        
                edge = pydot.Edge(signaln ["PAC"], signaln ["Scope"], arrowhead="empty", color="green", weight="1000")
                graph.add_edge(edge)        
                edge = pydot.Edge(signaln ["PAC"], signaln ["Mixer"], arrowhead="empty", color="green", weight="1000")
                graph.add_edge(edge)        
                edge = pydot.Edge(signaln ["Mixer"], signaln ["RMS"], arrowhead="empty", color="green", weight="1000")
                graph.add_edge(edge)        
                edge = pydot.Edge(signaln ["Mixer"], signaln ["DBGX_Mixer"], arrowhead="empty", color="green", weight="1000")
                graph.add_edge(edge)        
                edge = pydot.Edge(signaln ["RMS"], signaln ["Counter"], arrowhead="empty", color="green", weight="1000")
                graph.add_edge(edge)        
                edge = pydot.Edge(signaln ["Counter"], signaln ["Control"], arrowhead="empty", color="green", weight="1000")
                graph.add_edge(edge)        
                edge = pydot.Edge(signaln ["Control"], signaln ["VP"], arrowhead="empty", color="green", weight="1000")
                graph.add_edge(edge)        
                edge = pydot.Edge(signaln ["VP"], signaln ["Scan"], arrowhead="empty", color="green", weight="1000")
                graph.add_edge(edge)        
                edge = pydot.Edge(signaln ["VP"], signaln ["LockIn"], arrowhead="empty", color="green", weight="1000")
                graph.add_edge(edge)        
                edge = pydot.Edge(signaln ["Scan"], signaln ["Z_Servo"], arrowhead="empty", color="green", weight="1000")
                graph.add_edge(edge)        
                edge = pydot.Edge(signaln ["Z_Servo"], signaln ["M_Servo"], arrowhead="empty", color="green", weight="1000")
                graph.add_edge(edge)        
                edge = pydot.Edge(signaln ["M_Servo"], signaln ["Analog_OUT"], arrowhead="empty", color="green", weight="1000")
                graph.add_edge(edge)        
                edge = pydot.Edge(signaln ["Analog_OUT"], signaln ["Coarse"], arrowhead="empty", color="green", weight="1000")
                graph.add_edge(edge)        

                # add fixed relations
                edge = pydot.Edge(signaln ["Bias Adjust"], signaln ["VP Bias"], arrowhead="open")
                graph.add_edge(edge)        

                ANALOG_RELATIONS = {
                        "DIFF_IN0":"In 0",
                        "DIFF_IN1":"In 1",
                        "DIFF_IN2":"In 2",
                        "DIFF_IN3":"In 3"
                        }
                for inp in ANALOG_RELATIONS.keys():
                        edge = pydot.Edge(inputn [inp], signaln [ANALOG_RELATIONS[inp]], arrowhead="invdot", weight="1000")
                        graph.add_edge(edge)        

                # PAC / PLL assignment 
                edge = pydot.Edge(signaln ["In 4"], inputn ["PLL_INPUT"], arrowhead="empty", color="purple")
                graph.add_edge(edge)        
                edge = pydot.Edge(signaln ["PLL Exci Signal"], inputn ["OUTMIX_CH7"], arrowhead="empty", color="purple")
                graph.add_edge(edge)        

                MIXER_RELATIONS = {
                        "MIXER0":["MIX 0", "MIX0 qfac15 LP", "IIR32 0"],
                        "MIXER1":["MIX 1", "IIR32 1"],
                        "MIXER2":["MIX 2", "IIR32 2"],
                        "MIXER3":["MIX 3", "IIR32 3"]
                        }

#                for inp in MIXER_RELATIONS.keys():
#                        for s in MIXER_RELATIONS[inp]:
#                                print ("processing MIXER_REL: ", s, inp)
#                                edge = pydot.Edge(inputn [inp], signaln [s], arrowhead="diamond", weight="2000")
#                                graph.add_edge(edge)        
#                edge = pydot.Edge(signaln ["MIX 0"], signaln ["MIX out delta"], arrowhead="open", weight="100")
#                graph.add_edge(edge)        
#                edge = pydot.Edge(signaln ["MIX 1"], signaln ["MIX out delta"], arrowhead="open", weight="100")
#                graph.add_edge(edge)        
#                edge = pydot.Edge(signaln ["MIX 2"], signaln ["MIX out delta"], arrowhead="open", weight="100")
#                graph.add_edge(edge)        
#                edge = pydot.Edge(signaln ["MIX 3"], signaln ["MIX out delta"], arrowhead="open", weight="100")
#                graph.add_edge(edge)        

                edge = pydot.Edge(inputn ["ANALOG_AVG_INPUT"], signaln ["signal AVG-256"], arrowhead="invdot", weight="1000")
                graph.add_edge(edge)        
                edge = pydot.Edge(signaln ["signal AVG-256"], signaln ["signal RMS-256"], arrowhead="open", weight="100")
                graph.add_edge(edge)        

                OUTMIX_RELATIONS = {
                        "OUTMIX_CH0":["OUTMIX_CH0_ADD_A","OUTMIX_CH0_SUB_B","OUTMIX_CH0_SMAC_A","OUTMIX_CH0_SMAC_B","Out 0"],
                        "OUTMIX_CH1":["OUTMIX_CH1_ADD_A","OUTMIX_CH1_SUB_B","OUTMIX_CH1_SMAC_A","OUTMIX_CH1_SMAC_B","Out 1"],
                        "OUTMIX_CH2":["OUTMIX_CH2_ADD_A","OUTMIX_CH2_SUB_B","OUTMIX_CH2_SMAC_A","OUTMIX_CH2_SMAC_B","Out 2"],
                        "OUTMIX_CH3":["OUTMIX_CH3_ADD_A","OUTMIX_CH3_SUB_B","OUTMIX_CH3_SMAC_A","OUTMIX_CH3_SMAC_B","Out 3"],
                        "OUTMIX_CH4":["OUTMIX_CH4_ADD_A","OUTMIX_CH4_SUB_B","OUTMIX_CH4_SMAC_A","OUTMIX_CH4_SMAC_B","Out 4"],
                        "OUTMIX_CH5":["OUTMIX_CH5_ADD_A","OUTMIX_CH5_SUB_B","OUTMIX_CH5_SMAC_A","OUTMIX_CH5_SMAC_B","Out 5"],
                        "OUTMIX_CH6":["OUTMIX_CH6_ADD_A","OUTMIX_CH6_SUB_B","OUTMIX_CH6_SMAC_A","OUTMIX_CH6_SMAC_B","Out 6"],
                        "OUTMIX_CH7":["OUTMIX_CH7_ADD_A","OUTMIX_CH7_SUB_B","OUTMIX_CH7_SMAC_A","OUTMIX_CH7_SMAC_B","Out 7"],
                        "OUTMIX_CH0_SMAC_A":["OUTMIX_CH0_ADD_A"],
                        "OUTMIX_CH0_SMAC_B":["OUTMIX_CH0_SUB_B"],
                        "OUTMIX_CH1_SMAC_A":["OUTMIX_CH1_ADD_A"],
                        "OUTMIX_CH1_SMAC_B":["OUTMIX_CH1_SUB_B"],
                        "OUTMIX_CH2_SMAC_A":["OUTMIX_CH2_ADD_A"],
                        "OUTMIX_CH2_SMAC_B":["OUTMIX_CH2_SUB_B"],
                        "OUTMIX_CH3_SMAC_A":["OUTMIX_CH3_ADD_A"],
                        "OUTMIX_CH3_SMAC_B":["OUTMIX_CH3_SUB_B"],
                        "OUTMIX_CH4_SMAC_A":["OUTMIX_CH4_ADD_A"],
                        "OUTMIX_CH4_SMAC_B":["OUTMIX_CH4_SUB_B"],
                        "OUTMIX_CH5_SMAC_A":["OUTMIX_CH5_ADD_A"],
                        "OUTMIX_CH5_SMAC_B":["OUTMIX_CH5_SUB_B"],
                        "OUTMIX_CH6_SMAC_A":["OUTMIX_CH6_ADD_A"],
                        "OUTMIX_CH6_SMAC_B":["OUTMIX_CH6_SUB_B"],
                        "OUTMIX_CH7_SMAC_A":["OUTMIX_CH7_ADD_A"],
                        "OUTMIX_CH7_SMAC_B":["OUTMIX_CH7_SUB_B"],
                        "OUTMIX_CH8":["OUTMIX_CH8_ADD_A","Wave Out 8"],
                        "OUTMIX_CH9":["OUTMIX_CH9_ADD_A","Wave Out 9"]
                        }
                for inp in OUTMIX_RELATIONS.keys():
                        for inp2 in OUTMIX_RELATIONS[inp]:
                                if re.search ("SMAC", inp):
                                        edge = pydot.Edge(inputn [inp], inputn [inp2], arrowhead="invdot", color="blue", weight="3000")
                                else:
                                        if re.search ("Out", inp2):
                                                edge = pydot.Edge(inputn [inp], signaln [inp2], weight="4000")
                                        else:
                                                edge = pydot.Edge(inputn [inp], inputn [inp2], arrowhead="obox", weight="2000")
                                graph.add_edge(edge)        

                LOCKIN_RELATIONS = {
                        "LOCKIN_A":["LockIn A-0","LockIn A-1st","LockIn A-2nd","LockIn Ref"],
                        "LOCKIN_B":["LockIn B-0","LockIn B-1st","LockIn B-2nd","LockIn Ref"]
                        }
                for inp in LOCKIN_RELATIONS.keys():
                        for inp2 in LOCKIN_RELATIONS[inp]:
                                edge = pydot.Edge(inputn [inp], signaln [inp2], arrowhead="ediamond", weight="2000")
                                graph.add_edge(edge)        


                SERVO_RELATIONS = {
                        "M_SERVO":["M Servo", "M Servo Neg"],
                        "Z_SERVO":["Z Servo", "Z Servo Neg"]
                        }
                for inp in SERVO_RELATIONS.keys():
                        for s in SERVO_RELATIONS[inp]:
                                edge = pydot.Edge(inputn [inp], signaln [s], arrowhead="dot", weight="1000")
                                graph.add_edge(edge)        

        #        graph.write_png('signal_graph.png')

                if os.path.exists("/usr/share/gxsm3/pixmaps"):
                # use directory /usr/share/gxsm3/pixmaps
                        graph.write_svg('/usr/share/gxsm3/pixmaps/signal_graph.svg')
                        graph.write_dot('/usr/share/gxsm3/pixmaps/signal_graph.dot')
                        os.system("xdot /usr/share/gxsm3/pixmaps/signal_graph.dot&")
                        # os.system("eog /usr/share/gxsm3/pixmaps/signal_graph.svg &")
                elif os.path.exists("/usr/share/gxsm/pixmaps"):
                # use directory /usr/share/gxsm/pixmaps
                        graph.write_svg('/usr/share/gxsm/pixmaps/signal_graph.svg')
                        graph.write_dot('/usr/share/gxsm/pixmaps/signal_graph.dot')
                        os.system("xdot /usr/share/gxsm/pixmaps/signal_graph.dot&")
                else:
                # use working directory, possibly /gxsm-svn/Gxsm-2.0/plug-ins/hard/MK3-A810_spmcontrol/python_scripts
                        graph.write_svg('signal_graph.svg')
                        graph.write_dot('signal_graph.dot')
                        os.system("xdot signal_graph.dot&")        
                        # os.system("eog signal.svg &")

        #        dotwindow = MyDotWindow()
        #        dotwindow.set_dotcode(graph.dot)

                if 0:
                        png_str = graph.create_png(prog='dot')

                # treat the dot output string as an image file
                        sio = StringIO()
                        sio.write(png_str)
                        sio.seek(0)
                        img = mpimg.imread(sio)

                # plot the image
                        imgplot = plt.imshow(img, aspect='equal')
                # plt.show(block=False)
                        plt.show()



class SignalScope():

    def __init__(self, parent, mode="", single_shot=0, blen=5000):

        [Xsignal, Xdata, Offset] = parent.mk3spm.query_module_signal_input(DSP_SIGNAL_SCOPE_SIGNAL1_INPUT_ID)
        [Ysignal, Ydata, Offset] = parent.mk3spm.query_module_signal_input(DSP_SIGNAL_SCOPE_SIGNAL2_INPUT_ID)

        label = "Oscilloscope/Recorder" + mode
        name=label
        self.ss = single_shot
        self.block_length = blen
        self.restart_func = self.nop
        self.trigger = 0
        self.trigger_level = 0
        if name not in parent.wins:
                win = Gtk.Window()
                parent.wins[name] = win
                v = Gtk.VBox(spacing=0)

                scope = Oscilloscope( Gtk.Label(), v, "XT", label, OSZISCALE)
                scope.scope.set_wide (True)
                scope.show()
                scope.set_chinfo([Xsignal[SIG_NAME], Ysignal[SIG_NAME]])
                win.add(v)
                if not self.ss:
                        self.block_length = 512
                        parent.mk3spm.set_recorder (self.block_length, self.trigger, self.trigger_level)

                table = Gtk.Table(n_rows=4, n_columns=2)
                v.pack_start (table, True, True, 0)
                table.show()

                tr=0
                lab = Gtk.Label(label="# Samples:")
                lab.show ()
                table.attach(lab, 2, 3, tr, tr+1)
                Samples = Gtk.Entry()
                Samples.set_text("%d"%self.block_length)
                table.attach(Samples, 2, 3, tr+1, tr+2)
                Samples.show()

                [signal,data,offset] = parent.mk3spm.query_module_signal_input(DSP_SIGNAL_SCOPE_SIGNAL1_INPUT_ID)
                combo = parent.make_signal_selector (DSP_SIGNAL_SCOPE_SIGNAL1_INPUT_ID, signal, "CH1: ", parent.global_vector_index)
#                lab = Gtk.Label (label="CH1-scale:")
                combo.show ()
                table.attach(combo, 0, 1, tr, tr+1)
                Xscale = Gtk.Entry()
                Xscale.set_text("1.")
                table.attach(Xscale, 0, 1, tr+1, tr+2)
                Xscale.show()

                [signal,data,offset] = parent.mk3spm.query_module_signal_input(DSP_SIGNAL_SCOPE_SIGNAL2_INPUT_ID)
                combo = parent.make_signal_selector (DSP_SIGNAL_SCOPE_SIGNAL2_INPUT_ID, signal, "CH2: ", parent.global_vector_index)
#                lab = Gtk.Label( label="CH2-scale:")
                combo.show ()
                table.attach(combo, 1, 2, tr, tr+1)
                Yscale = Gtk.Entry()
                Yscale.set_text("1.")
                table.attach(Yscale, 1, 2, tr+1, tr+2)
                Yscale.show()

                tr = tr+2
                labx0 = Gtk.Label( label="X-off:")
                labx0.show ()
                table.attach(labx0, 0, 1, tr, tr+1)
                Xoff = Gtk.Entry()
                Xoff.set_text("0")
                table.attach(Xoff, 0, 1, tr+1, tr+2)
                Xoff.show()
                laby0 = Gtk.Label( label="Y-off:")
                laby0.show ()
                table.attach(laby0, 1, 2, tr, tr+1)
                Yoff = Gtk.Entry()
                Yoff.set_text("0")
                table.attach(Yoff, 1, 2, tr+1, tr+2)
                Yoff.show()

                self.xdc = 0.
                self.ydc = 0.

                def update_scope(set_data, Xsignal, Ysignal, xs, ys, x0, y0, num, x0l, y0l):
                        blck = parent.mk3spm.check_recorder_ready ()
                        if blck == -1:
                                n = self.block_length
                                [xd, yd] = parent.mk3spm.read_recorder (n)
                                if not self.run:
                                        if n < 128:
                                                print ("CH1:")
                                                print (xd)
                                                print ("CH2:")
                                                print (yd)
                                        else:
                                                save("mk3_recorder_xdata", xd)
                                                save("mk3_recorder_ydata", yd)
                                                scope.set_flash ("Saved: mk3_recorder_[xy]data")

                                # auto subsample if big
                                nss = n
                                nraw = n
                                if n > 8192:
                                        ### CHECK DOWN SAMPLING METHODE BELOW, SEAMS TO DROP POINTS EVEN IF MULTIPE OF 2
                                        ss = int(n/8192)
                                        end =  ss * int(len(xd)/ss)
                                        nss = (int)(n/ss)
                                        xd = mean(xd[:end].reshape(-1, ss), 1)
                                        yd = mean(yd[:end].reshape(-1, ss), 1)
                                        scope.set_info(["sub sampling: %d"%n + " by %d"%ss + " [nss=%d"%nss + ",end=%d]"%end,
                                                        "T = %g ms"%(n/150.),
                                                        mode])
                                        scope.set_subsample_factor(ss)
                                else:
                                        scope.set_info([
                                                        "T = %g ms"%(n/150.),
                                                        mode])
                                        scope.set_subsample_factor(1)

                                # change number samples?
                                try:
                                        self.block_length = int(num())
                                        if self.block_length < 64:
                                                self.block_length = 64
                                        if self.block_length > 999999:
                                                print ("MAX BLOCK LEN is 999999")
                                                self.block_length = 1024
                                except ValueError:
                                        self.block_length = 512

                                if self.block_length != n or self.ss:
                                        self.run = False
                                        self.run_button.set_label("RESTART")

                                if not self.ss:
                                        parent.mk3spm.set_recorder (self.block_length, self.trigger, self.trigger_level)
                                #                                v = value * signal[SIG_D2U]
                                #                                maxv = (1<<31)*math.fabs(signal[SIG_D2U])
                                try:
                                        xscale_div = float(xs())
                                except ValueError:
                                        xscale_div = 1

                                try:
                                        yscale_div = float(ys())
                                except ValueError:
                                        yscale_div = 1

                                n = nss
                                try:
                                        self.xoff = float(x0())
                                        for i in range (0, n, 8):
                                                self.xdc = 0.9 * self.xdc + 0.1 * xd[i] * Xsignal[SIG_D2U]
                                        x0l("X-DC = %g"%self.xdc)
                                except ValueError:
                                        for i in range (0, n, 8):
                                                self.xoff = 0.9 * self.xoff + 0.1 * xd[i] * Xsignal[SIG_D2U]
                                        x0l("X-off: %g"%self.xoff)

                                try:
                                        self.yoff = float(y0())
                                        for i in range (0, n, 8):
                                                self.ydc = 0.9 * self.ydc + 0.1 * yd[i] * Ysignal[SIG_D2U]
                                        y0l("Y-DC = %g"%self.ydc)
                                except ValueError:
                                        for i in range (0, n, 8):
                                                self.yoff = 0.9 * self.yoff + 0.1 * yd[i] * Ysignal[SIG_D2U]
                                        y0l("Y-off: %g"%self.yoff)

                                if math.fabs(xscale_div) > 0:
                                        xd = -(xd * Xsignal[SIG_D2U] - self.xoff) / xscale_div
                                else:
                                        xd = xd * 0. # ZERO/OFF
                                        
                                self.trigger_level = self.xoff / Xsignal[SIG_D2U]

                                if math.fabs(yscale_div) > 0:
                                        yd = -(yd * Ysignal[SIG_D2U] - self.yoff) / yscale_div
                                else:
                                        yd = yd * 0. # ZERO/OFF

                                scope.set_scale ( { 
                                                "CH1:": "%g"%xscale_div + " %s"%Xsignal[SIG_UNIT],
                                                "CH2:": "%g"%yscale_div + " %s"%Xsignal[SIG_UNIT],
                                                "Timebase:": "%g ms"%(nraw/150./20.) 
                                                })
                                
                                scope.set_data (xd, yd)

                                if self.mode > 1:
                                        self.run_button.set_label("SINGLE")
                        else:
                                scope.set_info(["waiting for trigger or data [%d]"%blck])
                                scope.queue_draw()


                        return self.run

                def stop_update_scope (win, event=None):
                        print ("STOP, hide.")
                        win.hide()
                        self.run = False
                        return True

                def toggle_run_recorder (b):
                        if self.run:
                                self.run = False
                                self.run_button.set_label("RUN")
                        else:
                                self.restart_func ()

                                [Xsignal, Xdata, Offset] = parent.mk3spm.query_module_signal_input(DSP_SIGNAL_SCOPE_SIGNAL1_INPUT_ID)
                                [Ysignal, Ydata, Offset] = parent.mk3spm.query_module_signal_input(DSP_SIGNAL_SCOPE_SIGNAL2_INPUT_ID)
                                scope.set_chinfo([Xsignal[SIG_NAME], Ysignal[SIG_NAME]])

                                period = int(2.*self.block_length/150.)
                                if period < timeout_min_recorder:
                                        period = timeout_min_recorder

                                if self.mode < 2: 
                                        self.run_button.set_label("STOP")
                                        self.run = True
                                else:
                                        self.run_button.set_label("ARMED")
                                        parent.mk3spm.set_recorder (self.block_length, self.trigger, self.trigger_level)
                                        self.run = False

                                GLib.timeout_add (period, update_scope, scope, Xsignal, Ysignal, Xscale.get_text, Yscale.get_text, Xoff.get_text, Yoff.get_text, Samples.get_text, labx0.set_text, laby0.set_text)

                def toggle_trigger (b):
                        if self.trigger == 0:
                                self.trigger = 1
                                self.trigger_button.set_label("TRIGGER POS")
                        else:
                                if self.trigger > 0:
                                        self.trigger = -1
                                        self.trigger_button.set_label("TRIGGER NEG")
                                else:
                                        self.trigger = 0
                                        self.trigger_button.set_label("TRIGGER OFF")
                        print (self.trigger, self.trigger_level)

                def toggle_mode (b):
                        if self.mode == 0:
                                self.mode = 1
                                self.ss = 0
                                self.mode_button.set_label("T-Auto")
                        else:
                                if self.mode == 1:
                                        self.mode = 2
                                        self.ss = 0
                                        self.mode_button.set_label("T-Normal")
                                else:
                                        if self.mode == 2:
                                                self.mode = 3
                                                self.ss = 1
                                                self.mode_button.set_label("T-Single")
                                        else:
                                                self.mode = 0
                                                self.ss = 0
                                                self.mode_button.set_label("T-Free")

                self.run_button = Gtk.Button(label="STOP")
                self.run_button.connect("clicked", toggle_run_recorder)
                self.hb = Gtk.HBox()
                self.hb.pack_start (self.run_button, True, True, 0)
                self.mode_button = Gtk.Button(label="M: Free")
                self.mode=0 # 0 free, 1 auto, 2 nommal, 3 single
                self.mode_button.connect("clicked", toggle_mode)
                self.hb.pack_start (self.mode_button, True, True, 0)
                self.mode_button.show ()
                table.attach(self.hb, 2, 3, tr, tr+1)
                tr = tr+1

                self.trigger_button = Gtk.Button(label="TRIGGER-OFF")
                self.trigger_button.connect("clicked", toggle_trigger)
                self.trigger_button.show ()
                table.attach(self.trigger_button, 2, 3, tr, tr+1)

                self.run = False
                win.connect("delete_event", stop_update_scope)
                toggle_run_recorder (self.run_button)
                
        parent.wins[name].show_all()

    def set_restart_callback (self, func):
        self.restart_func = func
                        
    def nop (self):
        return 0
                        
# try 1..2 deg or 0.5V as value
class StepResponse():
        def __init__(self, parent, value=0.05, mode="Amp"):
                self.parent = parent
                self.mk3spm = parent.mk3spm
                self.wins = parent.wins
                self.mode = mode

                self.value = value
                self.setup (value)
                self.scope = SignalScope(parent, " Step Response: "+mode, 1)
                self.scope.set_restart_callback (self.setup)

        def setup(self, value=0.):
                if value != 0.:
                        self.value = value
                if self.mode == "Amp":
                        # Ampl step
                        s1 = self.parent.mk3spm.lookup_signal_by_name ("PLL Res Amp LP")
                        s2 = self.parent.mk3spm.lookup_signal_by_name ("PLL Exci Amp LP")
                        self.parent.mk3spm.change_signal_input (0, s1 , DSP_SIGNAL_SCOPE_SIGNAL1_INPUT_ID)
                        self.parent.mk3spm.change_signal_input (0, s2 , DSP_SIGNAL_SCOPE_SIGNAL2_INPUT_ID)
                        self.parent.mk3spm. set_PLL_amplitude_step (self.value)

                else:
                        # Phase Step
                        s1 = self.parent.mk3spm.lookup_signal_by_name ("PLL Exci Frq LP")
                        s2 = self.parent.mk3spm.lookup_signal_by_name ("PLL Res Ph LP")
                        self.parent.mk3spm.change_signal_input (0, s1 , DSP_SIGNAL_SCOPE_SIGNAL1_INPUT_ID)
                        self.parent.mk3spm.change_signal_input (0, s2 , DSP_SIGNAL_SCOPE_SIGNAL2_INPUT_ID)
                        self.parent.mk3spm. set_PLL_phase_step (self.value)

class TuneScope():

    def __init__(self, parent, fc=30760.0, span=100., Fres=0.1, int_ms = 100.):
        Xsignal = parent.mk3spm.lookup_signal_by_name("PLL Res Amp LP")
        Ysignal = parent.mk3spm.lookup_signal_by_name("PLL Res Ph LP")
        label = "Tune Scope -- X: " + Xsignal[SIG_NAME] + "  Y: " + Ysignal[SIG_NAME]
        name  = "Tune Scope"
        if name not in wins:
                win = Gtk.Window()
                parent.wins[name] = win
                v = Gtk.VBox()

                self.Fc       = fc
                self.Fspan    = span
                self.Fstep    = Fres
                self.interval = int_ms
                self.pos      = -10
                self.points   = int (2 * round (self.Fspan/2./self.Fstep) + 1)
                self.ResAmp   = ones (self.points)
                self.ResAmp2F = ones (self.points)
                self.ResPhase = zeros (self.points)
                self.ResPhasePAC = zeros (self.points)
                self.ResPhase2F = zeros (self.points)
                self.Fit      = zeros (self.points)
                self.Freq     = zeros (self.points)
                self.volumeSine  = 0.3
                self.mode2f = 0
                self.phase_prev1 = 0.
                self.phase_prev2 = 0.
                self.phase_center = 0.
                self.phase_center2 = 0.
                
                def Frequency (position):
                        return self.Fc - self.Fspan/2. + position * self.Fstep

                self.FSineHz  = Frequency (0.)

                scope = Oscilloscope( Gtk.Label(), v, "XT", label, OSZISCALE)
                scope.set_scale ( { Xsignal[SIG_UNIT]: "mV", Ysignal[SIG_UNIT]: "deg", "Freq." : "Hz" })
                scope.set_chinfo(["Res Ampl LP","Res Phase LP","Model","Res Ampl 2F","Res Phase 2F"])
                win.add(v)
                scope.scope.set_wide (True)
                scope.scope.set_dBX (True)
                scope.scope.set_dBZ (True)

                table = Gtk.Table(n_rows=4, n_columns=2)
                table.set_row_spacings(5)
                table.set_col_spacings(5)
                v.pack_start (table, True, True, 0)

                tr=0
                lab = Gtk.Label(label="Ampl scale: V/div or 0 dB")
                table.attach(lab, 0, 1, tr, tr+1)
                self.Xscale = Gtk.Entry()
                self.Xscale.set_text("0.02")
                table.attach(self.Xscale, 0, 1, tr+1, tr+2)
                lab = Gtk.Label(label="Phase scale: deg/Div")
                table.attach(lab, 1, 2, tr, tr+1)

                self.Yscale = Gtk.Entry()
                self.Yscale.set_text("20.")
                table.attach(self.Yscale, 1, 2, tr+1, tr+2)
                lab = Gtk.Label(label="Fc [Hz]:")
                table.attach(lab, 2, 3, tr, tr+1)
                self.Fcenter = Gtk.Entry()
                self.Fcenter.set_text("%g"%self.Fc)
                table.attach(self.Fcenter, 2, 3, tr+1, tr+2)

                lab = Gtk.Label(label="Span [Hz]:")
                table.attach(lab, 3, 4, tr, tr+1)
                self.FreqSpan = Gtk.Entry()
                self.FreqSpan.set_text("%g"%self.Fspan)
                table.attach(self.FreqSpan, 3, 4, tr+1, tr+2)


                tr = tr+2
                self.labx0 = Gtk.Label(label="Amp off:")
                table.attach(self.labx0, 0, 1, tr, tr+1)
                self.Xoff = Gtk.Entry()
                self.Xoff.set_text("0")
                table.attach(self.Xoff, 0, 1, tr+1, tr+2)

                self.laby0 = Gtk.Label( label="Phase off:")
                table.attach(self.laby0, 1, 2, tr, tr+1)
                self.Yoff = Gtk.Entry()
                self.Yoff.set_text("0") ## 180
                table.attach(self.Yoff, 1, 2, tr+1, tr+2)

                lab = Gtk.Label(label="Volume Sine [V]:")
                table.attach(lab, 2, 3, tr, tr+1)
                self.Vs = Gtk.Entry()
                self.Vs.set_text("%g"%self.volumeSine)
                table.attach(self.Vs, 2, 3, tr+1, tr+2)

                lab = Gtk.Label(label="Interval [ms]:")
                table.attach(lab, 3, 4, tr, tr+1)
                self.Il = Gtk.Entry()
                self.Il.set_text("%d"%self.interval)
                table.attach(self.Il, 3, 4, tr+1, tr+2)

                self.M2F = Gtk.CheckButton ("Mode 1F+2F")
                table.attach(self.M2F, 0, 1, tr+2, tr+3)

                lab = Gtk.Label(label="Freq. Res. [Hz]:")
                table.attach(lab, 1, 2, tr+2, tr+3)
                self.FreqRes = Gtk.Entry()
                self.FreqRes.set_text("%g"%self.Fstep)
                table.attach(self.FreqRes, 2, 3, tr+2, tr+3)

                self.xdc = 0.
                self.ydc = 0.

                self.fitresults = { "A":0., "f0": 32768., "Q": 0. }

                self.pk_data_fname = "data_last_tune_result_3_7"

                try:
                        with open (self.pk_data_fname, 'rb') as f:
                                self.last_tune = pickle.load (f)
                except IOError:
                        self.last_tune  = { "f0":30760, "p0": 0.0 } # default on no previous results data
                        
                print (self.last_tune)
                
                self.fitinfo = ["", "", ""]
                self.resmodel = ""

                def run_fit(iA0, if0, iQ):
                        class Parameter:
                            def __init__(self, initialvalue, name):
                                self.value = initialvalue
                                self.name=name
                            def set(self, value):
                                self.value = value
                            def __call__(self):
                                return self.value

                        def fit(function, parameters, x, data, u):
                                def fitfun(params):
                                        for i,p in enumerate(parameters):
                                                p.set(params[i])
                                        return (data - function(x))/u

                                if x is None: x = arange(data.shape[0])
                                if u is None: u = ones(data.shape[0],"float")
                                p = [param() for param in parameters]
                                return leastsq(fitfun, p, full_output=1)

                        # define function to be fitted
                        def resonance(f):
                                A=1000.
                                return (A/A0())/sqrt(1.0+Q()**2*(f/w0()-w0()/f)**2)

                        self.resmodel = "Model:  A(f) = (1000/A0) / sqrt (1 + Q^2 * (f/f0 - f0/f)^2)"

                        # read data
                        ## freq, vr, dvr=load('lcr.dat', unpack=True)
                        freq = self.Freq + self.Fstep*0.5         ## actual adjusted frequency, aligned for fit ??? not exactly sure why.
                        vr   = self.ResAmp 
                        dvr  = self.ResAmp/100.  ## error est. 1%

                        # the fit parameters: some starting values
                        A0=Parameter(iA0,"A")
                        w0=Parameter(if0,"f0")
                        Q=Parameter(iQ,"Q")

                        p=[A0,w0,Q]

                        # for theory plot we need some frequencies
                        freqs=linspace(self.Fc - self.Fspan/2, self.Fc + self.Fspan/2, 200)
                        initialplot=resonance(freqs)

#                        self.Fit = initialplot
                        
                        # uncertainty calculation
#                        v0=10.0
#                        uvr=sqrt(dvr*dvr+vr*vr*0.0025)/v0
                        v0=1.
                        uvr=sqrt(dvr*dvr+vr*vr*0.0025)/v0

                        # do fit using Levenberg-Marquardt
#                        p2,cov,info,mesg,success=fit(resonance, p, freq, vr/v0, uvr)
                        p2,cov,info,mesg,success=fit(resonance, p, freq, vr, None)

                        if success==1:
                                print ("Converged")
                        else:
                                self.fitinfo[0] = "Fit not converged."
                                print ("Not converged")
                                print (mesg)

                        # calculate final chi square
                        chisq=sum(info["fvec"]*info["fvec"])

                        dof=len(freq)-len(p)
                        # chisq, sqrt(chisq/dof) agrees with gnuplot
                        print ("Converged with chi squared ",chisq)
                        print ("degrees of freedom, dof ", dof)
                        print ("RMS of residuals (i.e. sqrt(chisq/dof)) ", sqrt(chisq/dof))
                        print ("Reduced chisq (i.e. variance of residuals) ", chisq/dof)
                        print ()
                        
                        # uncertainties are calculated as per gnuplot, "fixing" the result
                        # for non unit values of the reduced chisq.
                        # values at min match gnuplot
                        print ("Fitted parameters at minimum, with 68% C.I.:")
                        for i,pmin in enumerate(p2):
                                self.fitinfo[i]  = "%-10s %12f +/- %10f"%(p[i].name,pmin,sqrt(cov[i,i])*sqrt(chisq/dof))
                                self.fitresults[p[i].name] = pmin
                                print (self.fitinfo[i])
                        print ()

                        self.last_tune["f0"] = self.fitresults["f0"]
                        self.last_tune["p0"] = self.ResPhasePAC[int(round((self.fitresults["f0"]-self.Fc-self.Fspan/2)/self.Fstep))]
                        print (self.last_tune)
                        
                        with open (self.pk_data_fname, 'wb') as f:
                                pickle.dump (self.last_tune, f, pickle.HIGHEST_PROTOCOL)


                        
                        print ("Correlation matrix")
                        # correlation matrix close to gnuplot
                        print ("               ")
                        for i in range(len(p)):
                            print ("%-10s"%(p[i].name,))
                        print ()
                        for i in range(len(p2)):
                            print ("%10s"%p[i].name)
                            for j in range(i+1):
                                print ("%10f"%(cov[i,j]/sqrt(cov[i,i]*cov[j,j]),))
                            print ()

                        finalplot=resonance(freqs)
                        self.Fit = finalplot
#                        print ("----------------- Final:")
#                        print (self.Fit)


                        save("mk3_tune_Freq", self.Freq)
                        save("mk3_tune_Fit", self.Fit)
                        save("mk3_tune_ResAmp", self.ResAmp)
                        save("mk3_tune_ResPhase", self.ResPhase)
                        save("mk3_tune_ResPhase2F", self.ResPhase2F)
                        save("mk3_tune_ResAmp2F", self.ResAmp2F)
                        scope.set_flash ("Saved: mk3_tune_*")


                self.peakvalue = 0
                self.peakindex = 0

                def update_tune(set_data, Xsignal, Ysignal):
                        Filter64Out = parent.mk3spm.read_PLL_Filter64Out ()
                        cur_a = Filter64Out[iii_PLL_F64_ResAmpLP] * Xsignal[SIG_D2U]
                        cur_ph_pac = Filter64Out[iii_PLL_F64_ResPhaseLP] * Ysignal[SIG_D2U]
                        cur_ph = cur_ph_pac
                        #cur_ph = cur_ph_pac - self.last_tune["p0"] # center with last phase a res, then unwrap
                        print (self.pos, Filter64Out, cur_a, cur_ph, self.phase_prev1, self.points)

                        if self.pos < 0:
                                self.pre_ph = cur_ph_pac
                        cur_ph = cur_ph - math.floor(((cur_ph - self.pre_ph)/360.0)+0.5)*360.;
                        self.pre_ph = cur_ph_pac
                        
#                        # init phase 29681.6
#                        if self.pos < 0:
#                                while cur_ph <= -180:
#                                        cur_ph += 360
#                                while cur_ph >= 180:
#                                        cur_ph -= 360
#
#                                if self.mode2f == 0:
#                                        self.phase_prev1 = cur_ph
#                                else:
#                                        self.phase_prev2 = cur_ph
#
#                        # full phase unwrap
#                      if self.mode2f == 0:
#                             pre_ph = self.phase_center
#                      else:
#                             pre_ph = self.phase_center2
#                    
#                      # P_UnWrapped(i)=P(i)-Floor(((P(i)-P(i-1))/2Pi)+0.5)*2Pi
#
#                        if cur_ph - pre_ph > 180.:
#                              cur_ph = cur_ph - 180. # 360
#                                print ("Ph UnWrap pos=", cur_ph)
#                        if cur_ph - pre_ph < -180.:
#                               cur_ph = cur_ph + 180. # 360
#                                print ("Ph UnWrap neg=", cur_ph)

                        

                                
                        if self.pos >= 0 and self.pos < self.points:
                                self.Freq[self.pos]     = self.FSineHz
                                if self.mode2f == 0:
                                        self.ResAmp[self.pos]   = cur_a
                                        self.ResPhase[self.pos] = cur_ph
                                        self.ResPhasePAC[self.pos] = cur_ph_pac
                                        self.phase_prev1 = cur_ph
                                
                                        if self.peakvalue < cur_a:
                                                self.peakvalue = cur_a
                                                self.peakindex = self.pos
                                else:
                                        self.ResAmp2F[self.pos]   = cur_a
                                        self.ResPhase2F[self.pos] = cur_ph
                                        self.phase_prev2 = cur_ph
                        else:
                                self.peakvalue = 0

                        if self.M2F.get_active ():
                                if self.mode2f == 1:
                                        self.pos      = self.pos+1
                                        self.mode2f = 0
                                else:
                                        self.mode2f = 1
                        else:
                                self.pos      = self.pos+1
                                self.mode2f = 0
                                
                                
                        if self.pos >= self.points:
                                self.run = False
                                self.run_button.set_label("RESTART")
                                if self.peakvalue > 0.:
                                        run_fit (1000./self.peakvalue, Frequency (self.peakindex), 20000.)
                                self.mode2f = 1
                                parent.mk3spm.adjust_PLL_sine (self.volumeSine, self.fitresults["f0"], self.mode2f)
                                time.sleep (0.2)
                                cur_ph = Filter64Out[iii_PLL_F64_ResPhaseLP] * Ysignal[SIG_D2U]
                                while cur_ph <= -180:
                                        cur_ph += 360
                                while cur_ph >= 180:
                                        cur_ph -= 360
                                self.phase_center2 = cur_ph
                                self.mode2f = 0
                                parent.mk3spm.adjust_PLL_sine (self.volumeSine, self.fitresults["f0"], self.mode2f)
                                time.sleep (0.2)
                                cur_ph = Filter64Out[iii_PLL_F64_ResPhaseLP] * Ysignal[SIG_D2U]

                                while cur_ph <= -180:
                                        cur_ph += 360
                                while cur_ph >= 180:
                                        cur_ph -= 360
                                self.phase_center = cur_ph
                                self.pos = -10
                        else:
                                self.Fstep   = self.Fspan/(self.points-1)
                                self.FSineHz = Frequency (self.pos)
                                parent.mk3spm.adjust_PLL_sine (self.volumeSine, self.FSineHz, self.mode2f)

                        if self.peakindex > 0 and self.peakindex < self.points:
                                scope.set_info(["tuning at %.3f Hz"%self.FSineHz + " [%d]"%self.pos,
                                                "cur peak at: %.3f Hz"%Frequency (self.peakindex),
                                                "Res Amp: %g V"%cur_a + " [ %g V]"%self.peakvalue,
                                                "Phase: %3.1f deg"%cur_ph_pac + " [ %3.1f deg]"%self.last_tune["p0"],
                                                "Vol. Sine: %.3f V"%self.volumeSine,"","","","","",
                                                self.resmodel,
                                                self.fitinfo[0],self.fitinfo[1],self.fitinfo[2]
                                                ])
                        else:
                                scope.set_info(["tuning at %.3f Hz"%self.FSineHz + " [%d]"%self.pos,
                                                "cur peak at: --",
                                                "Res Amp: %g V"%cur_a,
                                                "Phase: %3.1f deg"%self.phase_prev1,
                                                "Vol. Sine: %.3f V"%self.volumeSine,"","","","","",
                                                self.resmodel,
                                                self.fitinfo[0],self.fitinfo[1],self.fitinfo[2]
                                                ])

                        n = self.pos

                        try:
                                self.Fc = float(self.Fcenter.get_text())
                                if self.Fc < 10.:
                                        self.Fc=32766.
                                if self.Fc > 75000.:
                                        self.Fc=32766.
                        except ValueError:
                                self.Fc=32766.

                        try:
                                self.volumeSine = float(self.Vs.get_text())
                                if self.volumeSine < 0.:
                                        self.volumeSine=0.
                                if self.volumeSine > 10.:
                                        self.volumeSine=1.
                        except ValueError:
                                self.volumeSine=1.

                        tmp = 1.
                        try:
                                xscale_div = float(self.Xscale.get_text())
                                if xscale_div == 0.:
                                        xscale_div = tmp
                        except ValueError:
                                xscale_div = tmp
                                
                        tmp = 1.
                        try:
                                yscale_div = float(self.Yscale.get_text())
                                if yscale_div == 0.:
                                        yscale_div = tmp
                        except ValueError:
                                yscale_div = tmp

                        try:
                                self.xoff = float(self.Xoff.get_text())
                                for i in range (0, n, 8):
                                        self.xdc = 0.9 * self.xdc + 0.1 * self.ResAmp[i]
                                self.labx0.set_text ("X-DC = %g"%self.xdc)
                        except ValueError:
                                for i in range (0, n, 8):
                                        self.xoff = 0.9 * self.xoff + 0.1 * self.ResAmp[i]
                                self.labx0.set_text ("Amp Avg=%g"%self.xoff)

                        try:
                                self.yoff = float(self.Yoff.get_text())
                                for i in range (0, n, 8):
                                        self.ydc = 0.9 * self.ydc + 0.1 * self.ResPhase[i]
                                self.laby0.set_text ("Phase Avg=%g"%self.ydc)
                        except ValueError:
                                for i in range (0, n, 8):
                                        self.yoff = 0.9 * self.yoff + 0.1 * self.ResPhase[i]
                                self.laby0.set_text ("Y-off: %g"%self.yoff)

                        xd = -(self.ResAmp - self.xoff) / xscale_div
                        fd = -(self.Fit - self.xoff) / xscale_div
                        yd = -(self.ResPhase - self.yoff) / yscale_div

                        ud = -(self.ResAmp2F - self.xoff) / xscale_div
                        vd = -(self.ResPhase2F - self.yoff) / yscale_div

                        scope.set_scale ( { 
                                        "CH1: (Ampl.)":"%.3f V"%xscale_div, 
                                        "CH2: (Phase)":"%.1f deg"%yscale_div, 
                                        "Freq.: ": "%g Hz"%(self.Fspan/20.)
                                        })
                        if scope.get_wide ():
                                x = 40.*self.pos/self.points - 20 + 0.1
                        else:
                                x = 20.*self.pos/self.points - 10 + 0.1
                        y = -(cur_a- self.xoff) / xscale_div + 0.25
#                        scope.set_data (xd, yd, fd, array([[x,y],[x, y-0.5]]))
                        scope.set_data_with_uv (xd, yd, fd, ud, vd, array([[x,y],[x, y-0.5]]))


                        return self.run

                def stop_update_tune (win, event=None):
                        print ("STOP, hide.")
                        win.hide()
                        self.run = False
                        return True

                def toggle_run_tune (b):
                        if self.run:
                                self.run = False
                                self.run_button.set_label("RUN TUNE")
                                run_fit (1000./self.peakvalue, Frequency (self.peakindex), 20000.)
                        else:
                                self.run = True
                                self.run_button.set_label("STOP TUNE")
                                try:
                                        tmp = float(self.FreqSpan.get_text())
                                        if tmp != self.Fspan:
                                                if tmp > 0.1 and tmp < 75000:
                                                        self.Fspan = tmp
                                except ValueError:
                                        print ("invalid entry")
                                try:
                                        tmp = float(self.FreqRes.get_text())
                                        if tmp != self.Fstep:
                                                if tmp > 0.0001 and tmp < 100.:
                                                        self.Fstep=tmp
                                                else:
                                                        print ("invalid Fstep entry")
                                except ValueError:
                                        print ("invalid Fstep entry")

                                self.points   = int (2 * round (self.Fspan/2./self.Fstep) + 1)
                                print ("# points: ", self.points)
                                self.ResAmp   = ones (self.points)
                                self.ResPhase2F = zeros (self.points)
                                self.ResAmp2F   = ones (self.points)
                                self.ResPhase = zeros (self.points)
                                self.ResPhasePAC = zeros (self.points)
                                self.Fit      = zeros (self.points)
                                self.Freq     = zeros (self.points)
                                self.mode2f   = 0
                                self.pos      = -10
                                self.interval = int (self.Il.get_text())
                                self.fitinfo = ["","",""]
                                self.resmodel = ""
                                if self.interval < timeout_min_tunescope or self.interval > 30000:
                                        self.interval = timeout_tunescope_default
                                GLib.timeout_add (self.interval, update_tune, scope, Xsignal, Ysignal)


                self.run_button = Gtk.Button("STOP TUNE")
                self.run_button.connect("clicked", toggle_run_tune)
                table.attach(self.run_button, 3, 4, tr+2, tr+3)

                self.run = False
                win.connect("delete_event", stop_update_tune)
                toggle_run_tune (self.run_button)
                
        parent.wins[name].show_all()




class SignalPlotter():
        # X: 7=time, plotting Monitor Taps 20,21,0,1
    def __init__(self, parent, length = 300., taps=[7,20,21,0,1], samplesperpage=2000):
        self.monitor_taps = taps
        [value, uv1, Xsignal] = parent.mk3spm.get_monitor_signal (self.monitor_taps[1])
        [value, uv2, Ysignal] = parent.mk3spm.get_monitor_signal (self.monitor_taps[2])
        [value, uv3, Usignal] = parent.mk3spm.get_monitor_signal (self.monitor_taps[3])
        [value, uv4, Vsignal] = parent.mk3spm.get_monitor_signal (self.monitor_taps[4])

        label = "Signal Plotter -- Monitor Indexes 20,21,0,1"
        name  = "Signal Plotter"

        if name not in wins:
                win = Gtk.Window()
                parent.wins[name] = win
                v = Gtk.HBox()

                self.pos      = 0
                self.span     = length
                self.points   = samplesperpage
                self.dt       = self.span/self.points
                self.Time = zeros (self.points)
                self.Tap1 = zeros (self.points)
                self.Tap2 = zeros (self.points)
                self.Tap3 = zeros (self.points)
                self.Tap4 = zeros (self.points)

                self.t0 = parent.mk3spm.get_monitor_data (self.monitor_taps[0])

                scope = Oscilloscope( Gtk.Label(), v, "XT", label, OSZISCALE)
                scope.set_scale ( { Xsignal[SIG_UNIT]: "mV", Ysignal[SIG_UNIT]: "deg", "time" : "s" })
                scope.set_chinfo(["MON1","MON2","MON3","MON4"])
                #scope.set_info(["to select CH1..4 taps","select Signals via Signal Monitor for:"," t, CH1..4 as Mon" + str(self.monitor_taps)])
                scope.set_flash ("configure CH1..4 via Signal Monitor: t, CH1..4:=" + str(self.monitor_taps))

                win.add(v)

                table = Gtk.Table(n_rows=4, n_columns=2)
                table.set_row_spacings(5)
                table.set_col_spacings(5)
                v.pack_start (table, True, True, 0)

                tr=0
                c=0
                lab = Gtk.Label( label="CH1: %s"%Xsignal[SIG_NAME] + ", Scale in %s/div"%Xsignal[SIG_UNIT])

                table.attach(lab, c, c+1, tr, tr+1)
                tr=tr+1
                self.M1scale = Gtk.Entry()
                self.M1scale.set_text("1.0")
                table.attach(self.M1scale, c, c+1, tr, tr+1)
                tr=tr+1

                lab = Gtk.Label( label="CH2: %s"%Ysignal[SIG_NAME] + ", Scale in %s/div"%Ysignal[SIG_UNIT])
                table.attach(lab, c, c+1, tr, tr+1)
                tr=tr+1
                self.M2scale = Gtk.Entry()
                self.M2scale.set_text("1.0")
                table.attach(self.M2scale, c, c+1, tr, tr+1)
                tr=tr+1

                lab = Gtk.Label( label="CH3: %s"%Usignal[SIG_NAME] + ", Scale in %s/div"%Usignal[SIG_UNIT])
                table.attach(lab, c, c+1, tr, tr+1)
                tr=tr+1
                self.M3scale = Gtk.Entry()
                self.M3scale.set_text("1.0")
                table.attach(self.M3scale, c, c+1, tr, tr+1)
                tr=tr+1
                
                lab = Gtk.Label( label="CH4: %s"%Vsignal[SIG_NAME] + ", Scale in %s/div"%Vsignal[SIG_UNIT])
                table.attach(lab, c, c+1, tr, tr+1)
                tr=tr+1
                self.M4scale = Gtk.Entry()
                self.M4scale.set_text("1.0")
                table.attach(self.M4scale, c, c+1, tr, tr+1)
                tr=tr+1

                tr = 0
                c = 1
                self.labx0 = Gtk.Label( label="Offset in %s/div"%Xsignal[SIG_UNIT])
                table.attach(self.labx0, c, c+1, tr, tr+1)
                tr=tr+1
                self.M1off = Gtk.Entry()
                self.M1off.set_text("0")
                table.attach(self.M1off, c, c+1, tr, tr+1)
                tr=tr+1

                self.laby0 = Gtk.Label( label="Offset in %s/div"%Ysignal[SIG_UNIT])
                table.attach(self.laby0, c, c+1, tr, tr+1)
                tr=tr+1
                self.M2off = Gtk.Entry()
                self.M2off.set_text("0")
                table.attach(self.M2off, c, c+1, tr, tr+1)
                tr=tr+1

                self.laby0 = Gtk.Label( label="Offset in %s/div"%Usignal[SIG_UNIT])
                table.attach(self.laby0, c, c+1, tr, tr+1)
                tr=tr+1
                self.M3off = Gtk.Entry()
                self.M3off.set_text("0")
                table.attach(self.M3off, c, c+1, tr, tr+1)
                tr=tr+1

                self.laby0 = Gtk.Label( label="Offset in %s/div"%Vsignal[SIG_UNIT])
                table.attach(self.laby0, c, c+1, tr, tr+1)
                tr=tr+1
                self.M4off = Gtk.Entry()
                self.M4off.set_text("0")
                table.attach(self.M4off, c, c+1, tr, tr+1)
                tr=tr+1
                
                c=0
                lab = Gtk.Label( label="Page length in s \n per "+str(self.points)+" samples")
                table.attach(lab, c, c+1, tr, tr+1)
                tr=tr+1
                self.Il = Gtk.Entry()
                self.Il.set_text("%d"%self.span)
                table.attach(self.Il, c, c+1, tr, tr+1)

                self.xdc = 0.
                self.ydc = 0.

                def update_plotter():

                        if self.pos >= self.points:
                                #self.run = False
                                #self.run_button.set_label("RESTART")
                                self.pos = 0
                                                          
                                save ("plotter_t-"+str(self.t0), self.Time)
                                save ("plotter_t1-"+str(self.t0), self.Tap1)
                                save ("plotter_t2-"+str(self.t0), self.Tap2)
                                save ("plotter_t3-"+str(self.t0), self.Tap3)
                                save ("plotter_t4-"+str(self.t0), self.Tap4)
                                scope.set_flash ("Saved: plotter_t#-"+str(self.t0))
                                # auto loop
                                self.t0 = parent.mk3spm.get_monitor_data (self.monitor_taps[0])
                                
                        n = self.pos
                        self.pos = self.pos+1

                        t = parent.mk3spm.get_monitor_data (self.monitor_taps[0])
                        t = (t-self.t0) / 150000.
                        self.Time[n] = t
                        
                        [value, uv1, signal1] = parent.mk3spm.get_monitor_signal (self.monitor_taps[1])
                        self.Tap1[n] = uv1
                        [value, uv2, signal2] = parent.mk3spm.get_monitor_signal (self.monitor_taps[2])
                        self.Tap2[n] = uv2
                        [value, uv3, signal3] = parent.mk3spm.get_monitor_signal (self.monitor_taps[3])
                        self.Tap3[n] = uv3
                        [value, uv4, signal4] = parent.mk3spm.get_monitor_signal (self.monitor_taps[4])
                        self.Tap4[n] = uv4

                        if n == 0:
                            print ("plotter data signals:")
                            print (signal1, signal2, signal3)
                            scope.set_chinfo([signal1[SIG_NAME],signal2[SIG_NAME],signal3[SIG_NAME],signal4[SIG_NAME]])

                        print (n, t, uv1, uv2, uv3, uv4)
                        
                        try:
                                m1scale_div = float(self.M1scale.get_text())
                        except ValueError:
                                m1scale_div = 1
                                
                        try:
                                m2scale_div = float(self.M2scale.get_text())
                        except ValueError:
                                m2scale_div = 1

                        try:
                                m3scale_div = float(self.M3scale.get_text())
                        except ValueError:
                                m3scale_div = 1

                        try:
                                m4scale_div = float(self.M4scale.get_text())
                        except ValueError:
                                m4scale_div = 1

                        try:
                                m1off = float(self.M1off.get_text())
                        except ValueError:
                                m1off = 0
                                
                        try:
                                m2off = float(self.M2off.get_text())
                        except ValueError:
                                m2off = 0
                                
                        try:
                                m3off = float(self.M3off.get_text())
                        except ValueError:
                                m3off = 0
                                
                        try:
                                m4off = float(self.M4off.get_text())
                        except ValueError:
                                m4off = 0
                                
                        scope.set_scale ( { 
                                        "XY1: (Tap1)":"%g mV"%m1scale_div, 
                                        "XY2: (Tap2)":"%g mV"%m2scale_div, 
                                        "time: ": "%g s/div"%(self.span/20.)
                                        })

                        ### def set_data (self, Xd, Yd, Zd=zeros(0), XYd=[zeros(0), zeros(0)]):

                        #  if t > self.span:
                        #    td = 20. * (self.Time-t+self.span)/self.span
                        # else:
                        td = -10. + 20. * self.Time/self.span
                        t1 = -(self.Tap1 - m1off) / m1scale_div
                        t2 = -(self.Tap2 - m2off) / m2scale_div 
                        t3 = -(self.Tap3 - m3off) / m3scale_div 
                        t4 = -(self.Tap4 - m4off) / m4scale_div 
                        #scope.set_data (zeros(0), zeros(0), zeros(0), XYd=[td, t1])
                        #scope.set_data (t1, t2, zeros(0), XYd=[td, t1])
                        scope.set_data_with_uv (t1, t2, t3, t4)

                        return self.run

                def stop_update_plotter (win, event=None):
                        print ("STOP, hide.")
                        win.hide()
                        self.run = False
                        save ("plotter_t-"+str(self.t0), self.Time)
                        save ("plotter_t1-"+str(self.t0), self.Tap1)
                        save ("plotter_t2-"+str(self.t0), self.Tap2)
                        save ("plotter_t3-"+str(self.t0), self.Tap3)
                        save ("plotter_t4-"+str(self.t0), self.Tap4)
                        scope.set_flash ("Saved: plotter_t#-"+str(self.t0))
                        return True

                def toggle_run_plotter (b):
                        if self.run:
                                self.run = False
                                self.run_button.set_label("RUN PLOTTER")
                                save ("plotter_t-"+str(self.t0), self.Time)
                                save ("plotter_t1-"+str(self.t0), self.Tap1)
                                save ("plotter_t2-"+str(self.t0), self.Tap2)
                                save ("plotter_t3-"+str(self.t0), self.Tap3)
                                save ("plotter_t4-"+str(self.t0), self.Tap4)
                                scope.set_flash ("Saved: plotter_t#-"+str(self.t0))
                        else:
                                self.run = True
                                self.run_button.set_label("STOP PLOTTER")
                                self.t0 = parent.mk3spm.get_monitor_data (self.monitor_taps[0])
                                self.pos      = 0
                                self.span = float (self.Il.get_text())
                                self.dt   = self.span/self.points
                                self.Time = zeros (self.points)
                                self.Tap1 = zeros (self.points)
                                self.Tap2 = zeros (self.points)
                                self.Tap3 = zeros (self.points)
                                self.Tap4 = zeros (self.points)
                                GLib.timeout_add (int (self.dt*1000.), update_plotter)

                self.run_button = Gtk.Button("STOP TUNE")
                self.run_button.connect("clicked", toggle_run_plotter)
                table.attach(self.run_button, 1, 2, tr, tr+1)

                self.run = False
                win.connect("delete_event", stop_update_plotter)
                toggle_run_plotter (self.run_button)
                
        parent.wins[name].show_all()


class RecorderDeci():
        # X: 7=time, plotting Monitor Taps 20,21,0,1
    def __init__(self, parent):
        label = "Decimating Contineous Recorder Signal 0"
        name  = "S0 Deci Recorder"

        if name not in wins:
                win = Gtk.Window()
                parent.wins[name] = win
                v = Gtk.HBox()

                [Xsignal, Xdata, OffsetX] = parent.mk3spm.query_module_signal_input(DSP_SIGNAL_SCOPE_SIGNAL1_INPUT_ID)
                [Ysignal, Ydata, OffsetY] = parent.mk3spm.query_module_signal_input(DSP_SIGNAL_SCOPE_SIGNAL2_INPUT_ID)

                print (Ysignal)
                print (Xsignal)
        
                scope = Oscilloscope( Gtk.Label(), v, "XT", label, OSZISCALE)
                scope.set_subsample_factor(256)
                scope.scope.set_wide (True)
                scope.show()
                #scope.set_chinfo([Xsignal[SIG_NAME], "9.81*Integral of "+Xsignal[SIG_NAME]])
                scope.set_chinfo([Xsignal[SIG_NAME], Ysignal[SIG_NAME]])
                #scope.set_scale ( { signalV[SIG_UNIT]: "V", "Temp": "K" })

                win.add(v)

                table = Gtk.Table(n_rows=8, n_columns=2)
                v.pack_start (table, True, True, 0)
                tr=0
                c=0
                lab = Gtk.Label( label="CH1: %s"%Xsignal[SIG_NAME] + ", Scale in %s/div"%Xsignal[SIG_UNIT])

                table.attach(lab, c, c+1, tr, tr+1)
                tr=tr+1
                self.M1scale = Gtk.Entry()
                self.M1scale.set_text("0.1")
                table.attach(self.M1scale, c, c+1, tr, tr+1)
                self.M1ac = Gtk.CheckButton(label="AC")
                table.attach(self.M1ac, c+1, c+2, tr, tr+1)
                tr=tr+1

                lab = Gtk.Label( label="CH2: %s"%Ysignal[SIG_NAME] + ", Scale in %s/div"%Ysignal[SIG_UNIT])
                table.attach(lab, c, c+1, tr, tr+1)
                tr=tr+1
                self.M2scale = Gtk.Entry()
                self.M2scale.set_text("1.0")
                table.attach(self.M2scale, c, c+1, tr, tr+1)
                self.M2ac = Gtk.CheckButton(label="AC")
                table.attach(self.M2ac, c+1, c+2, tr, tr+1)
                tr=tr+1

                lab = Gtk.Label( label="Threshold")
                table.attach(lab, c, c+1, tr, tr+1)
                tr=tr+1
                self.T1 = Gtk.Entry()
                self.T1.set_text("0.0")
                table.attach(self.T1, c, c+1, tr, tr+1)
                tr=tr+1

                self.lastevent = parent.mk3spm.read_recorder_deci (4097, "", True)
                self.logfile = 'mk3_S0_py_recorder_deci256.log'
                self.logcount = 5
                self.count = 100
                
                def update_recorder():
                        try:
                                m1scale_div = float(self.M1scale.get_text())
                        except ValueError:
                                m1scale_div = 1.0
                                
                        try:
                                m2scale_div = float(self.M2scale.get_text())
                        except ValueError:
                                m2scale_div = 1.0

                        try:
                                m1th = float(self.T1.get_text())
                        except ValueError:
                                m1th = 0.0

                        #dscale = 256.*32768./10. 
                                
                        rec  = parent.mk3spm.read_recorder_deci_ch (4097, 0, self.logfile)*Xsignal[SIG_D2U]
                        rec1 = parent.mk3spm.read_recorder_deci_ch (4097, 1, self.logfile)*Ysignal[SIG_D2U]


                        if self.M1ac.get_active():
                            rec = rec - average(rec)
                        if self.M2ac.get_active():
                            rec1 = rec1 - average(rec1)
                        
                        scope.set_data (rec/m1scale_div, rec1/m2scale_div)

                        ma = rec.max()
                        mi = rec.min()
                        scope.set_info(["max: "+str(ma), "min: "+str(mi)])

                        if True: # disable velocity calc and th recording
                            return self.run
                        
                        vel = zeros(4097)
                        if True:
                            vint=0.0
                            dt=256.0/150000.0
                            om = sum(rec, axis=0)/4097.0
                            i=0
                            for a in rec:
                                om = om*0.99+0.01*a
                                vint = vint + 9.81*(a-om)*dt
                                vel[i] = vint
                                i=i+1

                        scope.set_data (rec/m1scale_div, vel/m2scale_div)

                        if m1th >= 0.0:
                                ma = rec.max()
                                mi = rec.min()
                                scope.set_info(["max: "+str(ma), "min: "+str(mi)])
                                if abs (ma) > m1th or abs (mi) > m1th:
                                        self.logfile = 'mk3_S0_py_recorder_deci256.log'
                                        if m1th > 0.0 and self.logcount == 0:
                                                with open(self.logfile, "a") as recorder:
                                                        recorder.write("### Threashold Triggered at: " + str(datetime.datetime.now()) + " max: "+str(ma) + " min: "+str(mi) + "\n")
                                                        self.count = 0
                                                        self.lastevent = rec
                                        self.logcount = 25*5
                                        self.count = self.count + 1
                                        if self.count == 24:
                                                self.lastevent = rec
                                        scope.set_flash ("Threashold Detected, Recording... " + str(self.count))
                                else:
                                        if self.logcount > 0:
                                                self.logcount = self.logcount - 1
                                                scope.set_flash ("Threashold Detected, Recording... " + str(self.logcount))
                                                scope.set_data_with_uv (rec/m1scale_div, vel/m2scale_div, zeros(0), self.lastevent/m1scale_div, self.lastevent/m2scale_div)
                                        else:
                                                if self.logfile != '':
                                                        self.logfile = ''
                                                        scope.set_data_with_uv (rec/m1scale_div, vel/m2scale_div, zeros(0), zeros(0), zeros(0))
                        
                        
                        return self.run

                def stop_recorder (win, event=None):
                        print ("STOP, hide.")
                        win.hide()
                        self.run = False
                        return True

                def toggle_run_recorder (b):
                        if self.run:
                                self.run = False
                                self.run_button.set_label("RUN REC")
                                scope.set_flash ("Stopped.")
                        else:
                                self.run = True
                                self.run_button.set_label("STOP REC")
                                scope.set_flash ("Starting...")
                                GLib.timeout_add (200, update_recorder)

                self.run_button = Gtk.Button("STOP REC")
                self.run_button.connect("clicked", toggle_run_recorder)
                table.attach(self.run_button, 0, 1, tr, tr+1)

                self.run = False
                win.connect("delete_event", stop_recorder)
                toggle_run_recorder (self.run_button)
                
        parent.wins[name].show_all()


class DiodeTPlotter():
        # X: 7=time, plotting Monitor Taps 20,21,0,1
    def __init__(self, parent, length = 3600., taps=[7,1], samplesperpage=3600):
        self.monitor_taps = taps

        [value, uv, signalV] = parent.mk3spm.get_monitor_signal (self.monitor_taps[1])

        label = "Diode Temp Plotter -- Monitor Indexes 1 (MIX IN 1)"
        name  = "Diode Temp Plotter"

        if name not in wins:
                win = Gtk.Window()
                parent.wins[name] = win
                v = Gtk.HBox()

                self.pos      = 0
                self.span     = length
                self.points   = samplesperpage
                self.dt       = self.span/self.points
                self.Time = zeros (self.points)
                self.Tap4 = zeros (self.points)

                self.t0 = parent.mk3spm.get_monitor_data (self.monitor_taps[0])

                scope = Oscilloscope( Gtk.Label(), v, "XT", label, OSZISCALE)
                scope.set_scale ( { signalV[SIG_UNIT]: "V", "Temp": "K" })
                scope.set_chinfo(["TEMP","MONV"])
                scope.set_flash ("configure CH4 via Signal Monitor: t, CH4:=" + str(self.monitor_taps[1]))

                win.add(v)

                table = Gtk.Table(n_rows=4, n_columns=2)
                v.pack_start (table, True, True, 0)

                tr=0
                c=0

                lab = Gtk.Label( label="CH1: Diode T in K")
                table.attach(lab, c, c+1, tr, tr+1)
                tr=tr+1
                self.M3scale = Gtk.Entry()
                self.M3scale.set_text("1.0")
                table.attach(self.M3scale, c, c+1, tr, tr+1)
                tr=tr+1
                
                lab = Gtk.Label( label="CH2: %s"%signalV[SIG_NAME] + ", Scale in %s/div"%signalV[SIG_UNIT])
                table.attach(lab, c, c+1, tr, tr+1)
                tr=tr+1
                self.M4scale = Gtk.Entry()
                self.M4scale.set_text("0.1")
                table.attach(self.M4scale, c, c+1, tr, tr+1)
                tr=tr+1

                tr = 0
                c = 1
                self.laby0 = Gtk.Label( label="Offset in K")
                table.attach(self.laby0, c, c+1, tr, tr+1)
                tr=tr+1
                self.M3off = Gtk.Entry()
                self.M3off.set_text("0")
                table.attach(self.M3off, c, c+1, tr, tr+1)
                tr=tr+1

                self.laby0 = Gtk.Label( label="Offset in V, 1.40K = 1.69812V")
                table.attach(self.laby0, c, c+1, tr, tr+1)
                tr=tr+1
                self.M4off = Gtk.Entry()
                self.M4off.set_text("1.69812")
                table.attach(self.M4off, c, c+1, tr, tr+1)
                tr=tr+1
                
                c=0
                lab = Gtk.Label( label="Page length in s \n per "+str(self.points)+" samples")
                table.attach(lab, c, c+1, tr, tr+1)
                tr=tr+1
                self.Il = Gtk.Entry()
                self.Il.set_text("%d"%self.span)
                table.attach(self.Il, c, c+1, tr, tr+1)

                self.xdc = 0.
                self.ydc = 0.

                def update_plotter():

                        if self.pos >= self.points:
                                #self.run = False
                                #self.run_button.set_label("RESTART")
                                self.pos = 0
                                                          
                                save ("plotter_dt-"+str(self.t0), self.Time)
                                save ("plotter_dv-"+str(self.t0), self.TapV)
                                save ("plotter_dk-"+str(self.t0), self.TapK)
                                scope.set_flash ("Saved: plotter_t#-"+str(self.t0))
                                # auto loop
                                self.t0 = parent.mk3spm.get_monitor_data (self.monitor_taps[0])
                                
                        n = self.pos
                        self.pos = self.pos+1

                        t = parent.mk3spm.get_monitor_data (self.monitor_taps[0])
                        t = (t-self.t0) / 150000.
                        self.Time[n] = t
                        
                        [value, uv, signalV] = parent.mk3spm.get_monitor_signal_median (self.monitor_taps[1])
                        self.TapV[n] = uv
                        self.TapK[n] = v2k(uv)

                        if n == 0:
                            print ("plotter data signals:")
                            print (signalV)
                            scope.set_chinfo(["Diode Temp in K", "Diode Volts"])

                        print (n, t, uv, v2k(uv))
                        
                        try:
                                m3scale_div = float(self.M3scale.get_text())
                        except ValueError:
                                m3scale_div = 1

                        try:
                                m4scale_div = float(self.M4scale.get_text())
                        except ValueError:
                                m4scale_div = 1

                        try:
                                m3off = float(self.M3off.get_text())
                        except ValueError:
                                m3off = 0
                                
                        try:
                                m4off = float(self.M4off.get_text())
                        except ValueError:
                                m4off = 0
                                
                        scope.set_scale ( { 
                                        "Temp"   : "[ {:.3f} K ]  {:g} K".format(v2k(uv),m3scale_div), 
                                        "DVolt:" : "[ {:.4f} V ]  {:g}".format(uv,m4scale_div), 
                                        "time: " : "{:g} s".format(self.span/20.)
                                        })

                        ### def set_data (self, Xd, Yd, Zd=zeros(0), XYd=[zeros(0), zeros(0)]):

                        #  if t > self.span:
                        #    td = 20. * (self.Time-t+self.span)/self.span
                        # else:
                        td = -10. + 20. * self.Time/self.span
                        tk = -(self.TapK - m3off) / m3scale_div 
                        tv = -(self.TapV - m4off) / m4scale_div 
                        #scope.set_data (zeros(0), zeros(0), zeros(0), XYd=[td, t1])
                        #scope.set_data (t1, t2, zeros(0), XYd=[td, t1])
                        scope.set_data_with_uv (tk, tv, zeros(0), zeros(0))

                        return self.run

                def stop_update_plotter (win, event=None):
                        print ("STOP, hide.")
                        win.hide()
                        self.run = False
                        save ("plotter_dt-"+str(self.t0), self.Time)
                        save ("plotter_dv-"+str(self.t0), self.TapV)
                        save ("plotter_dk-"+str(self.t0), self.TapK)
                        scope.set_flash ("Saved: plotter_dt#-"+str(self.t0))
                        return True

                def toggle_run_plotter (b):
                        if self.run:
                                self.run = False
                                self.run_button.set_label("RUN PLOTTER")
                                save ("plotter_dt-"+str(self.t0), self.Time)
                                save ("plotter_dv-"+str(self.t0), self.TapV)
                                save ("plotter_dk-"+str(self.t0), self.TapK)
                                scope.set_flash ("Saved: plotter_dt#-"+str(self.t0))
                        else:
                                self.run = True
                                self.run_button.set_label("STOP PLOTTER")
                                self.t0 = parent.mk3spm.get_monitor_data (self.monitor_taps[0])
                                self.pos      = 0
                                self.span = float (self.Il.get_text())
                                self.dt   = self.span/self.points
                                self.Time = zeros (self.points)
                                [value, uv, signalV] = parent.mk3spm.get_monitor_signal (self.monitor_taps[1])
                                self.TapV = zeros (self.points)+uv
                                self.TapK = zeros (self.points)+v2k(uv)
                                GLib.timeout_add (int (self.dt*1000.), update_plotter)

                self.run_button = Gtk.Button(label="STOP TUNE")
                self.run_button.connect("clicked", toggle_run_plotter)
                table.attach(self.run_button, 1, 2, tr, tr+1)

                self.run = False
                win.connect("delete_event", stop_update_plotter)
                toggle_run_plotter (self.run_button)
                
        parent.wins[name].show_all()



################################################################################
# CONFIGURATOR MAIN MENU/WINDOW
################################################################################

class Mk3_Configurator:
    def __init__(self):
        self.mk3spm = SPMcontrol (0)
        self.mk3spm.read_configurations ()
        self.vector_index = 0

        buttons = {
                   "1 810 AD/DA Monitor": self.create_A810_ADDA_monitor,
                   "2 SPM Signal Monitor": self.create_signal_monitor,
                   "3 SPM Signal Patch Rack": self.create_signal_patch_rack,
                   "4 SPM Signal+DSP Manager": self.create_signal_manager,
                   "5 SPM Create Signal Graph": self.create_dotviz_graph,
                   "6 SPM Signal Oscilloscope": self.create_oscilloscope_app,
                   "6pSPM Signal Plotter": self.create_signalplotter_app,
                   "6kSPM Diode Temp Plotter": self.create_diodetplotter_app,
                   "6zSPM Deci Stream Recorder": self.create_recorder_app,
                   "7 PLL: Osc. Tune App": self.create_PLL_tune_app,
                   "8 PLL: Ampl. Step App": self.create_amp_step_app,
                   "9 PLL: Phase Step App": self.create_phase_step_app,
##                   "GPIO Control": create_coolrunner_app,
##                   "Rate Meter": create_ratemeter_app,
##                   "FPGA Slider Control": create_coolrunner_slider_control_app,
##                   "FPGA LVDT Stage Control": create_coolrunner_lvdt_stage_control_app,
##                   "SR MK3 DSP Info": create_info,
##                   "SR MK3 DSP SPM Settings": create_settings_edit,
##                   "SR MK3 DSP Reset Peak Load": reset_dsp_load_peak,
#                   "SR MK3 read HR mask": self.mk3spm.read_hr,
#                   "SR MK3 set HR0": self.mk3spm.set_hr_0,
#                   "SR MK3 set HR1": self.mk3spm.set_hr_1,
#                   "SR MK3 set HR1slow": self.mk3spm.set_hr_1slow,
#                   "SR MK3 set HR1slow2": self.mk3spm.set_hr_1slow2,
#                   "SR MK3 set HRs2": self.mk3spm.set_hr_s2,
#                    "SR MK3 HARD RESET": self.mk3spm.issue_mk3_hard_reset,
#                    "SR MK3 read GPIO settings": self.mk3spm.read_gpio,
#                    "SR MK3 write GPIO settings": self.mk3spm.write_gpio,
#                    "SR MK3 CLR DSP GP53,54,55": self.mk3spm.clr_dsp_gpio,
#                    "SR MK3 SET DSP GP53,54,55": self.mk3spm.set_dsp_gpio,
#                    "SR TEST GPIO PULSE": self.mk3spm.execute_pulse_cb,

##                   "SR MK3 read D-FIFO128": readFIFO128,
##                   "SR MK3 read D-FIFO-R": readFIFO,
##                   "SR MK3 read PD-FIFO-R": readPFIFO,
                   "U SCO Config": self.sco_config,
                   "X Vector Index Selector": self.create_index_selector_matrix,
                   "Y User Values": self.create_user_values,
#                   "Z SCAN DBG": self.print_dbg,
                 }
        win = Gtk.Window()
        win.set_name("main window")
        win.connect("destroy", self.destroy)
        win.connect("delete_event", self.destroy)

        hb = Gtk.HeaderBar() 
        hb.set_show_close_button(True) 
        hb.props.title = "MK3-SPM"
        win.set_titlebar(hb) 
        hb.set_subtitle("Configurator Apps") 

        box1 = Gtk.VBox()
        win.add(box1)
        scrolled_window = Gtk.ScrolledWindow()
        scrolled_window.set_border_width(5)
        box1.pack_start(scrolled_window, True, True, 0)
        box2 = Gtk.VBox()
        box2.set_border_width(5)
        scrolled_window.add(box2)

        lab = Gtk.Label(label="SR dev:" + self.mk3spm.sr_path ())
        box2.pack_start(lab, True, True, 0)
        lab = Gtk.Label(label=self.mk3spm.get_spm_code_version_info())
        box2.pack_start(lab, True, True, 0)

        k = buttons.keys()
        for i in sorted(k):
                button = Gtk.Button(label=i)

                if buttons[i]:
                        button.connect ("clicked", buttons[i])
                else:
                        button.set_sensitive (False)
                box2.pack_start (button, True, True, 0)

        separator = Gtk.HSeparator()
        box1.pack_start (separator, False, True, 0)

        box2 = Gtk.VBox()
        box2.set_border_width (5)
        box1.pack_start (box2, False, True, 0)
        button = Gtk.Button(label='Close')
        button.connect ("clicked", self.do_exit)
        box2.pack_start (button, True, True, 0)
        win.show_all ()
        win.set_size_request(1,600)

        self.wins = {}

    def create_index_selector_matrix(self, b):
                name="Index Selector Matrix"
                if name not in wins:
                        win = Gtk.Window()
                        self.wins[name] = win
                        win.connect("delete_event", self.delete_event)

                        box1 = Gtk.VBox()
                        win.add(box1)

                        box2 = Gtk.VBox()
                        box2.set_border_width(5)
                        box1.pack_start(box2, True, True, 0)

                        separator = Gtk.HSeparator()
                        box1.pack_start (separator, False, True, 0)

                        # 8x8 matrix selector
                        table = Gtk.Table(n_rows=8, n_columns=8)
                        box1.pack_start(table, False, True, 0)
                        
                        for i in range (0,8):
                                for j in range (0,8):
                                        ij = 8*i+j
                                        button = Gtk.Button ("%d"%ij)
                                        button.connect ("clicked", self.do_set_index, ij)
                                        table.attach (button, i, i+1, j, j+1)

                        self.offsetindicator = Gtk.Label("Global Vector Index is [%d]"%self.vector_index)
                        box1.pack_start(self.offsetindicator, False, True, 0)

                self.wins[name].show_all()

    def do_set_user_input(self, b, i, u_gettext):
            self.mk3spm.write_signal_by_name ("user signal array", float(u_gettext ()), i)
                
    def create_user_values(self, b):
                name="User Values Array"
                if name not in wins:
                        win = Gtk.Window()
                        self.wins[name] = win
                        win.connect("delete_event", self.delete_event)

                        box1 = Gtk.VBox()
                        win.add(box1)

                        box2 = Gtk.VBox()
                        box2.set_border_width(5)
                        box1.pack_start(box2, True, True, 0)

                        separator = Gtk.HSeparator()
                        box1.pack_start (separator, False, True, 0)

                        # 8x8 matrix selector
                        table = Gtk.Table (n_rows=8, n_columns=8)
                        table.set_row_spacings(1)
                        table.set_col_spacings(1)
                        box1.pack_start(table, False, True, 0)
                        
                        for j in range (0,2):
                                for i in range (0,16):
                                        ij = 16*j+i
                                        ui = Gtk.Entry()
                                        [u,uv, sig] = self.mk3spm.read_signal_by_name ("user signal array", ij)
                                        ui.set_text("%f %s"%(uv, sig[SIG_UNIT]))
                                        table.attach (ui, 2*j+1, 2*j+2, i, i+1)

                                        button = Gtk.Button ("Set User Value [%d]"%ij)
                                        button.connect ("clicked", self.do_set_user_input, ij, ui.get_text)
                                        table.attach (button, 2*j+0, 2*j+1, i, i+1)

                self.wins[name].show_all()

    def print_dbg(self, b):
            def update ():
                    self.mk3spm.check_dsp_scan ()
                    return True
            
            GLib.timeout_add(200, update)

    def do_set_index(self, b, ij):
            self.vector_index = ij
            self.offsetindicator.set_text("Global Vector Index is [%d]"%self.vector_index)

    def delete_event(self, win, event=None):
            win.hide()
            # don't destroy window -- just leave it hidden
            return True

    def do_exit (self, button):
            Gtk.main_quit()

    def destroy (self, *args):
            Gtk.main_quit()
        
    # update DSP setting parameter
    def int_param_update(self, _adj):
        param_address = _adj.get_data("int_param_address")
        param = int (_adj.get_data("int_param_multiplier")*_adj.value)
        self.mk3spm.write_parameter (param_address, struct.pack ("<l", param), 1) 
        
    def make_menu_item(self, signal_store, name, callback, value, identifier, func=lambda:0):
        ##item = Gtk.MenuItem(name)
        print ("MK OPT MENU ITEM:", name, value, identifier)
        if (name != 'DISABLED'):
            signal_store.append([name, value[0]])
        else:
            signal_store.append([name, -1])
        ##item.connect("activate", callback, value, identifier, func)
        ##item.show()
        ##return item
        return None

    def adjust_signal_callback(self, opt, SIG_LIST, _input_id):
        pos=opt.get_active()
        print ("SIGNAL POS:",pos)
        # prefix+"S%02d:"%signal[SIG_INDEX]+signal[SIG_NAME], self.mk3spm.change_signal_input, signal, _input_id, self.global_vector_index
        ## "DISABLED", self.mk3spm.disable_signal_input, 0, _input_id, self.global_vector_index)
        # --> connect("activate", callback, value, identifier, func)

        if pos>0:
            signal = SIG_LIST[pos-1]
            print ("Adjust Input", _input_id, " to signal:", signal)
            if signal[SIG_INDEX] >= 0:
                self.mk3spm.change_signal_input (0, signal, _input_id, self.global_vector_index)
            else:
                # "DISABLED"
                self.mk3spm.disable_signal_input (0, 0, _input_id, self.global_vector_index)
        else:
            print ("Can't do that.")
            
    def toggle_flag(self, w, mask):
            self.mk3spm.adjust_statemachine_flag(mask, w.get_active())


    ##### configurator tools
    def create_dotviz_graph (self, _button):
            vis = Visualize (self)
            
    def create_oscilloscope_app (self, _button):
            kao = SignalScope (self)

    def create_signalplotter_app (self, _button):
            ##### SignalPlotter __init__ (self, parent, length = 300., taps=[7,20,21,0,1], samplesperpage=2000):

            ## may customize defaults using this line:
            ## sigplotter = SignalPlotter (self, 300, [7,20,21,0,1], 2000)
            sigplotter = SignalPlotter (self)

    def create_diodetplotter_app (self, _button):
            dtplotter = DiodeTPlotter (self)
        
    def create_recorder_app (self, _button):
            decirecorder = RecorderDeci (self)
        
    def create_PLL_tune_app (self, _button):
            tune = TuneScope (self)

    def create_amp_step_app (self, _button):
            as_scope = StepResponse(self, 0.05, "Amp")

    def create_phase_step_app (self, _button):
            ph_scope = StepResponse(self, 1., "Phase")

    def scoset (self, button):
            print (self.s_offset.get_text())
            print (self.fsens.get_text())
            sc   = round (self.sco_s_offsetQ * float(self.s_offset.get_text()) )
            freq = float (self.fsens.get_text())
            fsv  = round (freq*self.sco_sensQ)    # CPN31 * 2pi freq / 150000.
            print (sc, fsv)
            scodata = self.mk3spm.read_o(i_sco1, self.scoid * struct.calcsize(fmt_sco1), fmt_sco1)
            if scodata[11] == 0:
                    # (Sin - Sc) * fsv
                    self.mk3spm.write_o(i_sco1, self.scoid * struct.calcsize(fmt_sco1), struct.pack("<ll", sc, fsv))
            else:
                    CPN31 = 2147483647
                    deltasRe = round (CPN31 * math.cos (2.*math.pi*freq/150000.))
                    deltasIm = round (CPN31 * math.sin (2.*math.pi*freq/150000.))

                    print ("deltasRe/Im=")
                    print (deltasRe, deltasIm)
#                    deltasIm = round (freq*89953.58465492555767605362)   # CPN31 * 2pi f / 150000. 
#                    tmp=deltasIm*deltasIm
#                    print (tmp)
#                    tmp=long(tmp)>>32
#                    print (tmp)
#                    deltasRe = CPN31 - tmp
#                    print ("deltasRe/Im approx=")
#                    print (deltasRe, deltasIm)

                    self.mk3spm.write_o(i_sco1, self.scoid * struct.calcsize(fmt_sco1), struct.pack("<llll", sc, fsv, deltasRe, deltasIm))
                    self.mk3spm.write_o(i_sco1, self.scoid * struct.calcsize(fmt_sco1) + 4*4, struct.pack("<ll", 2147483647, 0)) # reset
                    print ("SCO=")

            scodata = self.mk3spm.read_o(i_sco1, self.scoid * struct.calcsize(fmt_sco1), fmt_sco1)
            print (scodata)

    def scodbg (self, button):
            scodata = self.mk3spm.read_o(i_sco1, self.scoid * struct.calcsize(fmt_sco1), fmt_sco1)
            print (scodata)

    def scoman (self, button):
            scodata = self.mk3spm.read_o(i_sco1, self.scoid * struct.calcsize(fmt_sco1), fmt_sco1)
            if scodata[11] > 0:
                    self.mk3spm.write_o(i_sco1, self.scoid * struct.calcsize(fmt_sco1) + 4*11, struct.pack("<l", 0)) # auto generator mode
            else:
                    self.mk3spm.write_o(i_sco1, self.scoid * struct.calcsize(fmt_sco1) + 4*11, struct.pack("<l", 1)) # manual generator mode
            scodata = self.mk3spm.read_o(i_sco1, self.scoid * struct.calcsize(fmt_sco1), fmt_sco1)
            print (scodata)

    def sco_read (self, button, id=0):
            self.scoid=id
            [c, fsv] = self.mk3spm.read_o(i_sco1, self.scoid * struct.calcsize(fmt_sco1), "<ll")
            self.s_offset.set_text(str(c/self.sco_s_offsetQ))
            self.fsens.set_text(str(fsv/self.sco_sensQ))
            self.scoidlab.set_label('SCO '+str(id+1))
            
    def sco_config (self, _button):
            name="SCO Config"
            if name not in wins:
                    win = Gtk.Window()
                    self.wins[name] = win
                    win.connect("delete_event", self.delete_event)
                    
                    grid = Gtk.VBox()
                    win.add(grid)

                    self.scoidlab = Gtk.Label( label="SCO ?")
                    grid.add(self.scoidlab)

                    self.sco_sensQ     = 89953.58465492555767605362/1024.
                    self.sco_s_offsetQ = 2147483648./10.
                    
                    [c, fsv] = self.mk3spm.read(i_sco1, "<ll")
                    
                    self.cl0 = Gtk.Label( label="Sig Offset [V]:")
                    grid.add(self.cl0)
                    self.s_offset = Gtk.Entry()
                    self.s_offset.set_text(str(c/self.sco_s_offsetQ))
                    grid.add (self.s_offset)

                    self.fs0 = Gtk.Label( label="F Sens [Hz/V]:")
                    grid.add(self.fs0)
                    self.fsens = Gtk.Entry()
                    self.fsens.set_text(str(fsv/self.sco_sensQ))
                    grid.add (self.fsens)

                    self.sco_read (0)
                    
                    bset = Gtk.Button(stock='set')
                    bset.connect("clicked", self.scoset)
                    grid.add(bset)

                    bsco1 = Gtk.Button(stock='Select SCO 1')
                    bsco1.connect("clicked", self.sco_read, 0)
                    grid.add(bsco1)
                    bsco2 = Gtk.Button(stock='Select SCO 2')
                    bsco2.connect("clicked", self.sco_read, 1)
                    grid.add(bsco2)

                    bdbg = Gtk.Button(stock='debug')
                    bdbg.connect("clicked", self.scodbg)
                    grid.add(bdbg)

                    bman = Gtk.Button(stock='toggle man/auto')
                    bman.connect("clicked", self.scoman)
                    grid.add(bman)

                    button = Gtk.Button(stock='gtk-close')
                    button.connect("clicked", lambda w: win.hide())
                    grid.add(button)

                    self.wins[name].show_all()

                    
                    
    # Vector index or signal variable offset from signal head pointer
    def global_vector_index (self):
            return self.vector_index

    def make_signal_selector (self, _input_id, sig, prefix, flag_nullptr_ok=0):
                signal_store = Gtk.ListStore(str, int)
                #opt = Gtk.OptionMenu()
                #menu = Gtk.Menu()
                i = 0
                i_actual_signal = -1 # for anything not OK
                item = self.make_menu_item(signal_store, "???", self.mk3spm.change_signal_input, sig, -1)
                SIG_LIST = self.mk3spm.get_signal_lookup_list()
                for signal in SIG_LIST:
                    #print (signal)
                    if signal[SIG_ADR] > 0:
                        item = self.make_menu_item(signal_store, prefix+"S%02d:"%signal[SIG_INDEX]+signal[SIG_NAME], self.mk3spm.change_signal_input, signal, _input_id, self.global_vector_index)
                        #menu.append(item)
                        if signal[SIG_ADR] == sig[SIG_ADR]:
                            i_actual_signal=i
                        i=i+1
                if flag_nullptr_ok:
                    item = self.make_menu_item(signal_store, "DISABLED", self.mk3spm.disable_signal_input, 0, _input_id, self.global_vector_index)
                    #menu.append(item)

                if i_actual_signal == -1:
                        i_actual_signal = i

                opt = Gtk.ComboBox.new_with_model(signal_store)
                opt.connect("changed", self.adjust_signal_callback, SIG_LIST, _input_id)
                renderer_text = Gtk.CellRendererText()
                opt.pack_start(renderer_text, True)
                opt.add_attribute(renderer_text, "text", 0)
                opt.set_active(i_actual_signal+1)

                ##print ("ACTIVATE:",i_actual_signal, SIG_LIST[i_actual_signal])

                #menu.set_active(i_actual_signal+1)
                #opt.set_menu(menu)
                opt.show()
                return opt # [opt, menu]

    def update_signal_menu_from_signal (self, combo, tap):
            [v, uv, signal] = self.mk3spm.get_monitor_signal (tap)
            if signal[SIG_INDEX] <= 0:
                    combo.set_active(0)
                    return 1
            combo.set_active(signal[SIG_INDEX]+1)
            return 1

    # build offset editor dialog
    def create_A810_ADDA_monitor(self, _button):
                name="A810 AD/DA Monitor"
                if name not in wins:
                        win = Gtk.Window()
                        self.wins[name] = win
                        win.connect("delete_event", self.delete_event)

                        box1 = Gtk.VBox()
                        win.add(box1)

                        box2 = Gtk.VBox()
                        box2.set_border_width(5)
                        box1.pack_start(box2, True, True, 0)

                        table = Gtk.Table (n_rows=6, n_columns=17)
                        box2.pack_start(table, False, False, 0)

                        r=0
                        lab = Gtk.Label( label="A810 Channels")
                        table.attach (lab, 0, 1, r, r+1)

                        lab = Gtk.Label( label="AD-IN Reading")
                        lab.set_alignment(1.0, 0.5)
                        table.attach (lab, 2, 3, 0, 1)

                        lab = Gtk.Label( label="DA-OUT Reading")
                        lab.set_alignment(1.0, 0.5)
                        table.attach (lab, 6, 7, r, r+1)

                        level_max = 10000. # mV
                        level_fac = 0.305185095 # mV/Bit

                        r = r+1
                        separator = Gtk.HSeparator()
                        separator.show()
                        table.attach (separator, 0, 7, r, r+1)

                        r = r+1
                        # create table for CH0..7 in/out reading, hook tabs
                        for tap in range(0,8):
                                lab = Gtk.Label( label="CH-"+str(tap))
                                table.attach (lab, 0, 1, r, r+1)

                                labin = Gtk.Label( label="+####.# mV")
                                labin.set_alignment(1.0, 0.5)
                                table.attach (labin, 2, 3, r, r+1)

                                labout = Gtk.Label( label="+####.# mV")
                                labout.set_alignment(1.0, 0.5)
                                table.attach (labout, 6, 7, r, r+1)

                                GLib.timeout_add(timeout_update_A810_readings, self.A810_reading_update, labin.set_text, labout.set_text, tap)
                                r = r+1

                        separator = Gtk.HSeparator()
                        box1.pack_start(separator, False, True, 0)
                        separator.show()

                        box2 = Gtk.VBox()
                        box2.set_border_width(10)
                        box1.pack_start(box2, False, True, 0)

                        button = Gtk.Button(stock='gtk-close')
                        button.connect("clicked", lambda w: win.hide())
                        box2.pack_start(button, True, True, 0)

                self.wins[name].show_all()

    # LOG "VU" Meter
    def create_meterVU(self, w, tap, fsk):
            [value, uv, signal] = self.mk3spm.get_monitor_signal (tap)
            label = signal[SIG_NAME]
            unit  = signal[SIG_UNIT]
            name="meterVU-"+label
            if name not in wins:
                    win = Gtk.Window()
                    self.wins[name] = win
                    win.connect("delete_event", self.delete_event)
                    v = Gtk.VBox()
                    instr = Instrument( Gtk.Label(), v, "VU", label)
                    instr.show()
                    win.add(v)

                    def update_meter(inst, _tap, signal, fm):
#                            [value, v, signal] = self.mk3spm.get_monitor_signal (_tap)
                            value = self.mk3spm.get_monitor_data (_tap)


                            if signal[SIG_VAR] == 'analog.rms_signal':
                                    try:
                                            float (fm ())
                                            maxv = math.fabs( float (fm ()))
                                    except ValueError:
                                            maxv = (1<<31)*math.fabs(math.sqrt(signal[SIG_D2U]))
                            
                                    v = math.sqrt(math.fabs(value) * signal[SIG_D2U])
                            else:
                                    try:
                                            float (fm ())
                                            maxv = math.fabs( float (fm ()))
                                    except ValueError:
                                            maxv = (1<<31)*math.fabs(signal[SIG_D2U])
                            
                                    v = value * signal[SIG_D2U]

                            maxdb = 20.*math.log10(maxv)
                            # _labsv("%+06.2f " %v+signal[SIG_UNIT])

                            if v >= 0:
                                    p="+"
                            else:
                                    p="-"
                            db = -maxdb + 20.*math.log10(math.fabs(v+0.001))

                            inst.set_reading_auto_vu (db, p)
                            inst.set_range(arange(0,maxv/10*11,maxv/10))

                            return True

                    GLib.timeout_add (timeout_update_meter_readings, update_meter, instr, tap, signal, fsk)
            self.wins[name].show_all()

    # Linear Meter
    def create_meterLinear(self, w, tap, fsk):
            [value, uv, signal] = self.mk3spm.get_monitor_signal (tap)
            label = signal[SIG_NAME]
            unit  = signal[SIG_UNIT]
            name="meterLinear-"+label
            if name not in wins:
                    win = Gtk.Window()
                    self.wins[name] = win
                    win.connect("delete_event", self.delete_event)
                    v = Gtk.VBox()
                    try:
                            float (fsk ())
                            maxv = float (fsk ())
                    except ValueError:
                            maxv = (1<<31)*math.fabs(signal[SIG_D2U])

                    instr = Instrument( Gtk.Label(), v, "Volt", label, unit)
                    instr.set_range(arange(0,maxv/10*11,maxv/10))
                    instr.show()
                    win.add(v)

                    def update_meter(inst, _tap, signal, fm):
#                            [value, v, signal] = self.mk3spm.get_monitor_signal (_tap)
                            value = self.mk3spm.get_monitor_data (_tap)
                            v = value * signal[SIG_D2U]
                            try:
                                    float (fm ())
                                    maxv = float (fm ())
                            except ValueError:
                                    maxv = (1<<31)*math.fabs(signal[SIG_D2U])
                            
                            if v >= 0:
                                    p="+"
                            else:
                                    p="-"
                            inst.set_reading_auto_vu (v, p)
                            inst.set_range(arange(0,maxv/10*11,maxv/10))
                            return True

                    GLib.timeout_add (timeout_update_meter_readings, update_meter, instr, tap, signal, fsk)
            self.wins[name].show_all()

#    def update_signal_menu_from_signal (self, menu, tap):
#            [v, uv, signal] = self.mk3spm.get_monitor_signal(tap)
#            if signal[SIG_INDEX] <= 0:
#                    menu.set_active(0)
#                    return 1
#            menu.set_active(signal[SIG_INDEX]+1)
#            return 1

    def create_signal_monitor(self, _button):
                name="SPM Signal Monitor"
                if name not in wins:
                        win = Gtk.Window()
                        self.wins[name] = win
                        win.connect("delete_event", self.delete_event)

                        box1 = Gtk.VBox()
                        win.add(box1)

                        box2 = Gtk.VBox()
                        box2.set_border_width(5)
                        box1.pack_start(box2, True, True, 0)

                        table = Gtk.Table (n_rows=11, n_columns=37)
                        box2.pack_start(table, False, False, 0)

                        r=0
                        lab = Gtk.Label( label="Signal to Monitor")
                        table.attach (lab, 0, 1, r, r+1)

                        lab = Gtk.Label( label="DSP Signal Variable name")
                        table.attach (lab, 1, 2, r, r+1)

                        lab = Gtk.Label( label="DSP Sig Reading")
                        lab.set_alignment(1.0, 0.5)
                        table.attach (lab, 2, 3, r, r+1)

                        lab = Gtk.Label( label="Value")
                        lab.set_alignment(1.0, 0.5)
                        table.attach (lab, 6, 7, r, r+1)

                        lab = Gtk.Label( label="dB")
                        lab.set_alignment(1.0, 0.5)
                        table.attach (lab, 7, 8, r, r+1)
                        lab = Gtk.Label( label="V")
                        lab.set_alignment(1.0, 0.5)
                        table.attach (lab, 8, 9, r, r+1)

                        lab = Gtk.Label( label="Galvo Range")
                        lab.set_alignment(1.0, 0.5)
                        table.attach (lab, 9, 10, r, r+1)

                        r = r+1
                        separator = Gtk.HSeparator()
                        separator.show()
                        table.attach (separator, 0, 7, r, r+1)

                        r = r+1
                        # create full signal monitor
                        for tap in range(0,num_monitor_signals):
                                [value, uv, signal] = self.mk3spm.get_monitor_signal (tap)
                                if signal[SIG_INDEX] < 0:
                                        break
                                combo = self.make_signal_selector (DSP_SIGNAL_MONITOR_INPUT_BASE_ID+tap, signal, "Mon[%02d]: "%tap)
                                table.attach (combo, 0, 1, r, r+1)

                                GLib.timeout_add (timeout_update_signal_monitor_menu, self.update_signal_menu_from_signal, combo, tap)

                                lab2 = Gtk.Label( label=signal[SIG_VAR])
                                lab2.set_alignment(-1.0, 0.5)
                                table.attach (lab2, 1, 2, r, r+1)

                                labv = Gtk.Label( label=str(value))
                                labv.set_alignment(1.0, 0.5)
                                table.attach (labv, 2, 3, r, r+1)

                                labsv = Gtk.Label( label="+####.# mV")
                                labsv.set_alignment(1.0, 0.5)
                                table.attach (labsv, 6, 7, r, r+1)
                                GLib.timeout_add (timeout_update_signal_monitor_reading, self.signal_reading_update, lab2.set_text, labv.set_text, labsv.set_text, tap)

                                full_scale = Gtk.Entry()
                                full_scale.set_text("auto")
                                table.attach (full_scale, 9, 10, r, r+1)

                                image = Gtk.Image()            
                                if os.path.isfile("meter-icondB.png"):
                                        imagefile="meter-icondB.png"
                                elif os.path.isfile("/usr/share/gxsm3/pixmaps/meter-icondB.png"):
                                        imagefile="/usr/share/gxsm3/pixmaps/meter-icondB.png"
                                else:
                                        imagefile="/usr/share/gxsm/pixmaps/meter-icondB.png"
                                image.set_from_file(imagefile)
                                image.show()
                                button = Gtk.Button()
                                button.add(image)
                                button.connect("clicked", self.create_meterVU, tap, full_scale.get_text)
                                table.attach (button, 7, 8, r, r+1)

                                image = Gtk.Image()
                                if os.path.isfile("meter-iconV.png"):
                                        imagefile="meter-iconV.png"
                                elif os.path.isfile("/usr/share/gxsm3/pixmaps/meter-iconV.png"):
                                        imagefile="/usr/share/gxsm3/pixmaps/meter-iconV.png"
                                else:
                                        imagefile="/usr/share/gxsm/pixmaps/meter-iconV.png"
                                image.set_from_file(imagefile)
                                image.show()
                                button = Gtk.Button()
                                button.add(image)
                                button.connect("clicked", self.create_meterLinear, tap, full_scale.get_text)
                                table.attach (button, 8, 9, r, r+1)

                                

                                r = r+1

                        separator = Gtk.HSeparator()
                        box1.pack_start(separator, False, True, 0)
                        separator.show()

                        box2 = Gtk.VBox()
                        box2.set_border_width(10)
                        box1.pack_start(box2, False, True, 0)

                        button = Gtk.Button(stock='gtk-close')
                        button.connect("clicked", lambda w: win.hide())
                        box2.pack_start(button, True, True, 0)

                self.wins[name].show_all()

    def update_signal_menu_from_mod_inp(self, _lab, mod_inp):
                [signal, data, offset] = self.mk3spm.query_module_signal_input(mod_inp)
                if offset > 0:
                        _lab (" == "+signal[SIG_VAR]+" [%d] ==> "%(offset))
                else:
                        _lab (" == "+signal[SIG_VAR]+" ==> ")
                return 1

    # Build MK3-A810 FB-SPMCONTROL Signal Patch Rack
    def create_signal_patch_rack(self, _button):
                name="SPM Patch Rack"
                if name not in wins:
                        win = Gtk.Window()
                        self.wins[name] = win
                        win.connect("delete_event", self.delete_event)
                        win.set_size_request(800, 500)

                        box1 = Gtk.VBox()
                        win.add(box1)
                        scrolled_window = Gtk.ScrolledWindow()
                        scrolled_window.set_border_width(3)
                        box1.pack_start(scrolled_window, True, True, 0)
                        scrolled_window.show()
                        box2 = Gtk.VBox()
                        box2.set_border_width(3)
                        scrolled_window.add_with_viewport(box2)
                        box1.pack_start(box2, True, True, 0)

                        table = Gtk.Table (n_rows=6, n_columns=17)
                        box2.pack_start(table, False, False, 0)

                        r=0
                        lab = Gtk.Label( label="SIGNAL")
                        table.attach (lab, 0, 1, r, r+1)

                        lab = Gtk.Label( label="--- variable --->")
                        table.attach (lab, 1, 2, r, r+1)

                        lab = Gtk.Label( label="MODULE INPUT")
                        lab.set_alignment(0.5, 0.5)
                        table.attach (lab, 2, 3, r, r+1)

                        r = r+1
                        separator = Gtk.HSeparator()
                        separator.show()
                        table.attach (separator, 0, 7, r, r+1)

                        r = r+1
                        # create full signal monitor
                        for mod in DSP_MODULE_INPUT_ID_CATEGORIZED.keys():
                                for mod_inp in DSP_MODULE_INPUT_ID_CATEGORIZED[mod]:
                                        if mod_inp[0] > 0: # INPUT_ID_VALID
                                                [signal, data, offset] = self.mk3spm.query_module_signal_input(mod_inp[0])
                                                if mod_inp[2] >= 0: # address valid, zero allowed in special cases if mod_inp[3] == 1!
                                                        # lab = Gtk.Label( label=signal[SIG_NAME])
                                                        # lab.set_alignment(-1.0, 0.5)        
                                                        # table.attach (lab1, 0, 1, r, r+1)
                                                        
                                                        prefix = "Sig: "
                                                        combo = self.make_signal_selector (mod_inp[0], signal, prefix, mod_inp[3])
                                                        lab.set_alignment(-1.0, 0.5)
                                                        table.attach (combo, 0, 1, r, r+1)

                                                        lab = Gtk.Label( label=" == "+signal[SIG_VAR]+" ==> ")
                                                        #                                        lab = Gtk.Label( label=" ===> ")
                                                        lab.set_alignment(-1.0, 0.5)        
                                                        table.attach (lab, 1, 2, r, r+1)
                                                        GLib.timeout_add (timeout_update_patchrack, self.update_signal_menu_from_mod_inp, lab.set_text, mod_inp[0])

                                                        lab = Gtk.Label( label=mod_inp[1])
                                                        lab.set_alignment(-1.0, 0.5)        
                                                        table.attach (lab, 2, 3, r, r+1)

                                                        r = r+1

                        separator = Gtk.HSeparator()
                        box1.pack_start(separator, False, True, 0)
                        separator.show()

                        box2 = Gtk.VBox()
                        box2.set_border_width(10)
                        box1.pack_start(box2, False, True, 0)

                        button = Gtk.Button(label='Close')
                        button.connect("clicked", lambda w: win.hide())
                        box2.pack_start(button, True, True, 0)

                self.wins[name].show_all()

    # Build MK3-A810 FB-SPMCONTROL Signal Manger / FLASH SUPPORT
    def create_signal_manager(self, _button, flashdbg=0):
                name="SPM Signal and DSP Core Manager"
                if name not in wins:
                        win = Gtk.Window()
                        self.wins[name] = win
                        win.connect("delete_event", self.delete_event)
                        win.set_size_request(800, 450)
                        self.dsp_man_win=win
                        
                        scrolled_window = Gtk.ScrolledWindow()
                        scrolled_window.set_border_width(5)
                        win.add(scrolled_window)
                        
                        box1 = Gtk.VBox()
                        scrolled_window.add_with_viewport(box1)

                        box2 = Gtk.VBox()
                        box2.set_border_width(10)
                        box1.pack_start(box2, False, True, 0)

                        button = Gtk.Button("REVERT TO POWER UP DEFAULTS")
                        button.connect("clicked", self.mk3spm.disable_signal_input, 0, 0, 0) # REVERT TO POWER-UP-DEFAULT
                        box2.pack_start(button, True, True, 0)

                        hb = Gtk.HBox()
                        box2.pack_start(hb, True, True, 0)
                        button = Gtk.Button("SAVE DSP-CONFIG TO FILE")
                        button.connect("clicked",  self.mk3spm.read_and_save_actual_module_configuration, "mk3_signal_configuration.pkl") # store to file
                        hb.pack_start(button, True, True, 0)

                        button = Gtk.Button("LOAD DSP-CONFIG FROM FILE")
                        button.connect("clicked",  self.mk3spm.load_and_write_actual_module_configuration, "mk3_signal_configuration.pkl") # restore from file
                        hb.pack_start(button, True, True, 0)

                        
                        hb = Gtk.HBox()
                        box2.pack_start(hb, True, True, 0)
                        button = Gtk.Button("STORE TO FLASH")
                        button.connect("clicked", self.mk3spm.manage_signal_configuration, DSP_SIGNAL_TABLE_STORE_TO_FLASH_ID)
                        hb.pack_start(button, True, True, 0)
                        
                        button = Gtk.Button("RESTORE FROM FLASH")
                        button.connect("clicked", self.mk3spm.manage_signal_configuration, DSP_SIGNAL_TABLE_RESTORE_FROM_FLASH_ID)
                        hb.pack_start(button, True, True, 0)
                        
                        button = Gtk.Button("ERASE (INVALIDATE) FLASH TABLE")
                        button.connect("clicked", self.mk3spm.manage_signal_configuration, DSP_SIGNAL_TABLE_ERASE_FLASH_ID)
                        hb.pack_start(button, True, True, 0)
                        
                        if flashdbg:
                                separator = Gtk.HSeparator()
                                box1.pack_start(separator, False, True, 0)
                                separator.show()
                                
                                lab = Gtk.Label(label="-- FLASH debug actions enabled --")
                                box1.pack_start(lab, False, True, 0)
                                lab.show()
                                
                                self.mk3spm.flash_dump(0, 2)
                                lab = Gtk.Label(label=self.mk3spm.flash_dump(0, 2))
                                box1.pack_start(lab, False, True, 0)
                                lab.show()
                                
                                hb = Gtk.HBox()
                                box2.pack_start(hb, True, True, 0)
                                data = Gtk.Entry()
                                data.set_text("0")
                                hb.pack_start(data, True, True, 0)
                                
                                button = Gtk.Button("SEEK")
                                button.connect("clicked", self.mk3spm.manage_signal_configuration, DSP_SIGNAL_TABLE_SEEKRW_FLASH_ID, data.get_text)
                                hb.pack_start(button, True, True, 0)
                                
                                button = Gtk.Button("READ")
                                button.connect("clicked", self.mk3spm.manage_signal_configuration, DSP_SIGNAL_TABLE_READ_FLASH_ID)
                                hb.pack_start(button, True, True, 0)
                                
                                button = Gtk.Button("WRITE")
                                button.connect("clicked", self.mk3spm.manage_signal_configuration, DSP_SIGNAL_TABLE_WRITE_FLASH_ID, data.get_text)
                                hb.pack_start(button, True, True, 0)
                                
                                button = Gtk.Button("TEST0")
                                button.connect("clicked", self.mk3spm.manage_signal_configuration, DSP_SIGNAL_TABLE_TEST0_FLASH_ID)
                                hb.pack_start(button, True, True, 0)
                                
                                button = Gtk.Button("TEST1")
                                button.connect("clicked", self.mk3spm.manage_signal_configuration, DSP_SIGNAL_TABLE_TEST1_FLASH_ID)
                                hb.pack_start(button, True, True, 0)
                                
                                button = Gtk.Button("DUMP")
                                button.connect("clicked", self.mk3spm.flash_dump)
                                hb.pack_start(button, True, True, 0)
                                

                        hb = Gtk.HBox()
                        box2.pack_start(hb, True, True, 0)
                        data =  Gtk.Label( label="DSP CORE MANAGER:")
                        hb.pack_start(data, True, True, 0)
                        button = Gtk.Button("CLR RTL-PEAK")
                        button.connect("clicked", self.mk3spm.reset_task_control_peak)
                        hb.pack_start(button, True, True, 0)
                        button = Gtk.Button("CLR N EXEC")
                        button.connect("clicked", self.mk3spm.reset_task_control_nexec)
                        hb.pack_start(button, True, True, 0)
                        button = Gtk.Button("REST CLOCK")
                        button.connect("clicked", self.mk3spm.reset_dsp_clock)
                        hb.pack_start(button, True, True, 0)
                        button = Gtk.Button("STATE?")
                        button.connect("clicked", self.mk3spm.print_statemachine_status)
                        hb.pack_start(button, True, True, 0)
                        button = Gtk.Button("590MHz")
                        button.connect("clicked", self.mk3spm.dsp_adjust_speed, 590)
                        hb.pack_start(button, True, True, 0)
                        button = Gtk.Button("688MHz")
                        def confirm_688(dummy):
                                label = Gtk.Label("Confirm DSP reconfiguration to 688 MHz.\nWARNING\nUse on own risc.")
                                dialog = Gtk.Dialog(title="DSP Clock Reconfiguration", transient_for=self.dsp_man_win, flags=0)
                                dialog.add_buttons (Gtk.STOCK_CANCEL, Gtk.ResponseType.CANCEL, Gtk.STOCK_OK, Gtk.ResponseType.OK)
                                dialog.vbox.pack_start(label, True, True, 0)
                                label.show()
                                checkbox = Gtk.CheckButton("I agree.")
                                dialog.action_area.pack_end(checkbox, True, True, 0)
                                checkbox.show()
                                response = dialog.run()
                                dialog.destroy()
                                
                                print (response)
                                if response == Gtk.ResponseType.OK and checkbox.get_active():
                                        self.mk3spm.dsp_adjust_speed(0, 688)

                        button.connect("clicked", confirm_688)
                        hb.pack_start(button, True, True, 0)

                        hb = Gtk.HBox()
                        box2.pack_start(hb, True, True, 0)
                        lbl =  Gtk.Label( label="DP MAX Tuning: ")
                        hb.pack_start(lbl, True, True, 0)
                        button = Gtk.Button("Default")
                        button.connect("clicked", self.mk3spm.set_dsp_rt_DP_max_default)
                        hb.pack_start(button, True, True, 0)
                        button = Gtk.Button("3800")
                        button.connect("clicked", self.mk3spm.set_dsp_rt_DP_max_hi1)
                        hb.pack_start(button, True, True, 0)
                        button = Gtk.Button("4000")
                        button.connect("clicked", self.mk3spm.set_dsp_rt_DP_max_hi2)
                        hb.pack_start(button, True, True, 0)
                        button = Gtk.Button("4200!")
                        button.connect("clicked", self.mk3spm.set_dsp_rt_DP_max_hi3)
                        hb.pack_start(button, True, True, 0)
                        button = Gtk.Button("4400!!")
                        button.connect("clicked", self.mk3spm.set_dsp_rt_DP_max_hi4)
                        hb.pack_start(button, True, True, 0)

                        
                                
                        def confirm_experimental(dummy, func, param1, param2=None):
                                label = Gtk.Label("Confirm Experimental DSP code.\nWARNING\nUse on own risc. Not for production yet.")
                                dialog = Gtk.Dialog(title="DSP Experimental Feature", transient_for=self.dsp_man_win, flags=0)
                                dialog.add_buttons (Gtk.STOCK_CANCEL, Gtk.ResponseType.CANCEL, Gtk.STOCK_OK, Gtk.ResponseType.OK)
                                dialog.vbox.pack_start(label, True, True, 0)
                                label.show()
                                checkbox = Gtk.CheckButton("I agree.")
                                dialog.action_area.pack_end(checkbox, True, True, 0)
                                checkbox.show()
                                response = dialog.run()
                                dialog.destroy()
                                
                                if response == Gtk.ResponseType.OK and checkbox.get_active():
                                        func(dummy, param1, param2)
                                else:
                                        print ("Function not activated, please confirm via checkbox.")
                                        

                        # McBSP (FPGA <-> MK3 hi speed seria link communication)
                        hb = Gtk.HBox()
                        box2.pack_start(hb, True, True, 0)
                        lbl =  Gtk.Label( label="DSP McBSP MHz:")
                        hb.pack_start(lbl, True, True, 0)
                        button = Gtk.Button("ON")
                        button.connect("clicked", confirm_experimental, self.mk3spm.dsp_enable_McBSP, 1)
                        hb.pack_start(button, True, True, 0)
                        button = Gtk.Button("RST")
                        button.connect("clicked", self.mk3spm.dsp_reset_McBSP, 0)
                        hb.pack_start(button, True, True, 0)

                        lbl =  Gtk.Label( label="Debug:")
                        hb.pack_start(lbl, True, True, 0)
                        button = Gtk.Button("SCHEDULE@END")
                        button.connect("clicked", self.mk3spm.dsp_enable_McBSP, 3)
                        hb.pack_start(button, True, True, 0)
                        button = Gtk.Button("OFF")
                        button.connect("clicked", self.mk3spm.dsp_enable_McBSP, 4)
                        hb.pack_start(button, True, True, 0)
                        button = Gtk.Button("L1")
                        button.connect("clicked", self.mk3spm.dsp_enable_McBSP, 5)
                        hb.pack_start(button, True, True, 0)
                        button = Gtk.Button("L2")
                        button.connect("clicked", self.mk3spm.dsp_enable_McBSP, 6)
                        hb.pack_start(button, True, True, 0)
                        button = Gtk.Button("L3")
                        button.connect("clicked", self.mk3spm.dsp_enable_McBSP, 7)
                        hb.pack_start(button, True, True, 0)

#                        hb = Gtk.HBox()
#                        box2.pack_start(hb, True, True, 0)
#                        lbl =  Gtk.Label( label="DSP McBSP mode:")
#                        hb.pack_start(lbl, True, True, 0)
#                        button = Gtk.Button("N=1")
#                        button.connect("clicked", self.mk3spm.dsp_configure_McBSP_N, 1)
#                        hb.pack_start(button, True, True, 0)
#                        button = Gtk.Button("N=2")
#                        button.connect("clicked", self.mk3spm.dsp_configure_McBSP_N, 2)
#                        hb.pack_start(button, True, True, 0)
#                        button = Gtk.Button("N=3")
#                        button.connect("clicked", self.mk3spm.dsp_configure_McBSP_N, 3)
#                        hb.pack_start(button, True, True, 0)
#                        button = Gtk.Button("N=4")
#                        button.connect("clicked", self.mk3spm.dsp_configure_McBSP_N, 4)
#                        hb.pack_start(button, True, True, 0)
#                        button = Gtk.Button("N=5")
#                        button.connect("clicked", self.mk3spm.dsp_configure_McBSP_N, 5)
#                        hb.pack_start(button, True, True, 0)
#                        button = Gtk.Button("N=6")
#                        button.connect("clicked", self.mk3spm.dsp_configure_McBSP_N, 6)
#                        hb.pack_start(button, True, True, 0)
#                        button = Gtk.Button("N=7")
#                        button.connect("clicked", self.mk3spm.dsp_configure_McBSP_N, 7)
#                        hb.pack_start(button, True, True, 0)
#                        button = Gtk.Button("N=8")
#                        button.connect("clicked", self.mk3spm.dsp_configure_McBSP_N, 8)
#                        hb.pack_start(button, True, True, 0)

                        hb = Gtk.HBox()
                        box2.pack_start(hb, True, True, 0)
                        lbl =  Gtk.Label( label="DSP McBSP MHz: ")
                        hb.pack_start(lbl, True, True, 0)
                        button = Gtk.Button("0.5")
                        button.connect("clicked", confirm_experimental, self.mk3spm.dsp_configure_McBSP_M, 0, lbl)
                        hb.pack_start(button, True, True, 0)
                        button = Gtk.Button("1")
                        button.connect("clicked", confirm_experimental, self.mk3spm.dsp_configure_McBSP_M, 1, lbl)
                        hb.pack_start(button, True, True, 0)
                        button = Gtk.Button("2")
                        button.connect("clicked", confirm_experimental, self.mk3spm.dsp_configure_McBSP_M, 2, lbl)
                        hb.pack_start(button, True, True, 0)
                        button = Gtk.Button("4")
                        button.connect("clicked", confirm_experimental, self.mk3spm.dsp_configure_McBSP_M, 3, lbl)
                        hb.pack_start(button, True, True, 0)
                        button = Gtk.Button("5")
                        button.connect("clicked", confirm_experimental, self.mk3spm.dsp_configure_McBSP_M, 4, lbl)
                        hb.pack_start(button, True, True, 0)
                        button = Gtk.Button("6.25")
                        button.connect("clicked", confirm_experimental, self.mk3spm.dsp_configure_McBSP_M, 5, lbl)
                        hb.pack_start(button, True, True, 0)
                        button = Gtk.Button("8.33")
                        button.connect("clicked", confirm_experimental, self.mk3spm.dsp_configure_McBSP_M, 6, lbl)
                        hb.pack_start(button, True, True, 0)
                        button = Gtk.Button("10")
                        button.connect("clicked", confirm_experimental, self.mk3spm.dsp_configure_McBSP_M, 7, lbl)
                        hb.pack_start(button, True, True, 0)
                        button = Gtk.Button("12.5")
                        button.connect("clicked", confirm_experimental, self.mk3spm.dsp_configure_McBSP_M, 8, lbl)
                        hb.pack_start(button, True, True, 0)
                        button = Gtk.Button("14.28")
                        button.connect("clicked", confirm_experimental, self.mk3spm.dsp_configure_McBSP_M, 9, lbl)
                        hb.pack_start(button, True, True, 0)
                        button = Gtk.Button("16.6")
                        button.connect("clicked", confirm_experimental, self.mk3spm.dsp_configure_McBSP_M, 10, lbl)
                        hb.pack_start(button, True, True, 0)
                        button = Gtk.Button("20")
                        button.connect("clicked", confirm_experimental, self.mk3spm.dsp_configure_McBSP_M, 11, lbl)
                        hb.pack_start(button, True, True, 0)

                        hb = Gtk.HBox()
                        box2.pack_start(hb, True, True, 0)
                        lbl =  Gtk.Label( label="DSP McBSP MHz x 688/590 if turbo: ")
                        hb.pack_start(lbl, True, True, 0)
                        button = Gtk.Button("25 * OK")
                        button.connect("clicked", confirm_experimental, self.mk3spm.dsp_configure_McBSP_M, 12, lbl)
                        hb.pack_start(button, True, True, 0)
                        #button = Gtk.Button("33 1:2 clk ratio not good")
                        #button.connect("clicked", confirm_experimental, self.mk3spm.dsp_configure_McBSP_M, 13, lbl)
                        #hb.pack_start(button, True, True, 0)
                        button = Gtk.Button("50 ** FPGA LoopBack for CLKR,FRMR required")
                        button.connect("clicked", confirm_experimental, self.mk3spm.dsp_configure_McBSP_M, 14, lbl)
                        hb.pack_start(button, True, True, 0)
                        button = Gtk.Button("100 !!")
                        button.connect("clicked", confirm_experimental, self.mk3spm.dsp_configure_McBSP_M, 15, lbl)
                        hb.pack_start(button, True, True, 0)
                        
                        button = Gtk.Button(label='Close')
                        button.connect("clicked", lambda w: win.hide())
                        box2.pack_start(button, True, True, 0)

                        separator = Gtk.HSeparator()
                        box2.pack_start (separator, False, True, 0)

                        self.load=0.0
                        labtimestructs = Gtk.Label( label="DSP TIME")
                        labtimestructs.set_alignment(0.0, 0.5)
                        box2.pack_start (labtimestructs, False, True, 0)
                        GLib.timeout_add (timeout_update_process_list, self.rt_dsprtosinfo_update, labtimestructs)
                                    
                        table = Gtk.Table (n_rows=7, n_columns=20)
                        box2.pack_start(table, False, False, 0)

                        r=0
                        c=0
                        hdrs=["PID","DSPTIME","TASK-TIME","PEAK","MISSED","FLAGS","Name of RT Task"]
                        for h in hdrs:
                                labh = Gtk.Label( label=h)
                                if c == 0 or c == 6:
                                        labh.set_alignment(0.0, 0.5)
                                else:
                                        if c == 5:
                                                labh.set_alignment(0.5, 0.5)
                                        else:
                                                labh.set_alignment(1.0, 0.5)
                                table.attach (labh, c, c+1, r, r+1)
                                c=c+1
                        for pid in range(0,NUM_RT_TASKS):
                                r=r+1
                                c=0
                                labpid = Gtk.Label( label="RT{:03d}".format(pid))
                                labpid.set_alignment(0.0, 0.5)
                                table.attach (labpid, c, c+1, r, r+1)
                                c=c+1
                                labdsp_t = Gtk.Label( label="0")
                                labdsp_t.set_alignment(1.0, 0.5)
                                table.attach (labdsp_t, c, c+1, r, r+1)
                                c=c+1
                                labtask_t = Gtk.Label( label="0")
                                labtask_t.set_alignment(1.0, 0.5)
                                table.attach (labtask_t, c, c+1, r, r+1)
                                c=c+1
                                labtask_m = Gtk.Label( label="0")
                                labtask_m.set_alignment(1.0, 0.5)
                                table.attach (labtask_m, c, c+1, r, r+1)
                                c=c+1
                                labmissed = Gtk.Label( label="0")
                                labmissed.set_alignment(1.0, 0.5)
                                table.attach (labmissed, c, c+1, r, r+1)
                                c=c+1
                                labflag = Gtk.Label( label="0")
                                labflag.set_alignment(0.5, 0.5)
                                table.attach (labflag, c, c+1, r, r+1)
                                c=c+1
                                lab=Gtk.Label( label=dsp_rt_process_name[pid])
                                lab.set_alignment(0.0, 0.5)
                                table.attach(lab, c, c+1, r, r+1)
                                c=c+1
                                if pid >= 0:
                                        cfg=Gtk.Button("cfg")
                                        cfg.connect("clicked", self.configure_rt, pid)
                                        table.attach(cfg, c, c+1, r, r+1)

                                GLib.timeout_add (timeout_update_process_list, self.rt_process_list_update, labdsp_t, labtask_t, labtask_m, labmissed, labflag, labpid, pid)

                        r=r+2
                        c=0
                        hdrs=["PID","TIME NEXT","INTERVAL", "Timer Info", "NUM EXEC", "FLAGS", "Name of Idle Task"]
                        for h in hdrs:
                                labh = Gtk.Label( label=h)
                                if c == 0 or c == 6:
                                        labh.set_alignment(0.0, 0.5)
                                else:
                                        if c == 5:
                                                labh.set_alignment(0.5, 0.5)
                                        else:
                                                labh.set_alignment(1.0, 0.5)
                                table.attach (labh, c, c+1, r, r+1)
                                c=c+1

                        for pid in range(0,NUM_ID_TASKS):
                                r=r+1
                                c=0
                                labpid = Gtk.Label(label="ID{:03d}".format(pid+1))
                                labpid.set_alignment(0.0, 0.5)
                                table.attach (labpid, c, c+1, r, r+1)
                                c=c+1
                                labtn = Gtk.Label( label="0")
                                labtn.set_alignment(1.0, 0.5)
                                table.attach (labtn, c, c+1, r, r+1)
                                c=c+1
                                labtask_ti = Gtk.Label(label="0")
                                labtask_ti.set_alignment(1.0, 0.5)
                                table.attach (labtask_ti, c, c+1, r, r+1)
                                c=c+1
                                lab_tmr_frq = Gtk.Label(label="-")
                                lab_tmr_frq.set_alignment(1.0, 0.5)
                                table.attach (lab_tmr_frq, c, c+1, r, r+1)
                                c=c+1
                                labtask_n = Gtk.Label(label="0")
                                labtask_n.set_alignment(1.0, 0.5)
                                table.attach (labtask_n, c, c+1, r, r+1)
                                c=c+1
                                labtask_f = Gtk.Label(label="0")
                                labtask_f.set_alignment(0.5, 0.5)
                                table.attach (labtask_f, c, c+1, r, r+1)
                                c=c+1
                                lab=Gtk.Label(label=dsp_id_process_name[pid])
                                lab.set_alignment(0.0, 0.5)
                                table.attach(lab, c, c+1, r, r+1)
                                c=c+1
                                cfg=Gtk.Button("cfg")
                                cfg.connect("clicked", self.configure_id, pid)
                                table.attach(cfg, c, c+1, r, r+1)

                                GLib.timeout_add (timeout_update_process_list, self.id_process_list_update, labpid, labtn, labtask_ti, lab_tmr_frq, labtask_n, labtask_f, pid)
                        
                self.wins[name].show_all()

    def rt_process_list_update(self, _dsp_t, _task_t, _task_m, _missed, _flag, _labpid, pid):
            flags = self.mk3spm.get_task_control_entry(ii_statemachine_rt_task_control, ii_statemachine_rt_task_control_flags, pid)
            missed_last = int(_missed.get_text())
            missed = self.mk3spm.get_task_control_entry(ii_statemachine_rt_task_control, ii_statemachine_rt_task_control_missed, pid)

            if flags == 0:
                    _labpid.set_markup("<span color='grey'>RT{:03d}</span>".format(pid))
                    _flag.set_markup("<span color='grey'>INACTIVE</span>")
            else:
                    if missed_last != missed:
                            _labpid.set_markup("<span color='red'>RT{:03d}</span>".format(pid))
                    else:
                            _labpid.set_markup("<span color='blue'>RT{:03d}</span>".format(pid))
                    IER=" *<span color='violet'>"        
                    if flags & 0x00100000:
                        IER=IER+"K"        
                    if flags & 0x00200000:
                        IER=IER+"A"        
                    if flags & 0x00400000:
                        IER=IER+"M"        
                    IER=IER+"</span>"        
                    if flags & 0x10:
                            _flag.set_markup("<span color='blue'>ALWAYS</span>"+IER)
                    if flags & 0x20:
                            _flag.set_markup("<span color='green'>on ODD CLK</span>"+IER)
                    if flags & 0x40:
                            _flag.set_markup("<span color='green'>on EVEN CLK</span>"+IER)
                            
            _dsp_t.set_text("{:10d}".format(self.mk3spm.get_task_control_entry(ii_statemachine_rt_task_control, ii_statemachine_rt_task_control_time, pid)))
            _task_t.set_text("{:10d}".format(self.mk3spm.get_task_control_entry_task_time (ii_statemachine_rt_task_control, pid)))
            _task_m.set_text("{:10d}".format(self.mk3spm.get_task_control_entry_peak_time (ii_statemachine_rt_task_control, pid)))
            _missed.set_text("{:10d}".format(missed))
            return 1

    def configure_rt(self, bnt, pid):
            pname  = Gtk.Label(dsp_rt_process_name[pid])
            flags = self.mk3spm.get_task_control_entry(ii_statemachine_rt_task_control, ii_statemachine_rt_task_control_flags, pid)
            label = Gtk.Label(label="Set Flags:")
            dialog = Gtk.Dialog(title="Adjust RT{:03d} Task".format(pid), transient_for=self.dsp_man_win, flags=0)
            dialog.add_buttons (Gtk.STOCK_CANCEL, Gtk.ResponseType.CANCEL, Gtk.STOCK_OK, Gtk.ResponseType.OK)
            
            dialog.vbox.pack_start(pname, True, True, 0)
            pname.show()
            hb = Gtk.HBox()
            dialog.vbox.pack_end(hb, True, True, 0)
            hb.show()
            hb.pack_start(label, True, True, 0)
            label.show()
            cb_active = Gtk.RadioButton.new_with_label_from_widget(None, label="Active")
            hb.pack_end(cb_active, True, True, 0)
            cb_active.show()
            cb_odd = Gtk.RadioButton.new_with_label_from_widget(cb_active,  label="Odd")
            hb.pack_end(cb_odd, True, True, 0)
            cb_odd.show()
            cb_even = Gtk.RadioButton.new_with_label_from_widget(cb_active,  label="Even")
            hb.pack_end(cb_even, True, True, 0)
            cb_even.show()
            cb_sleep = Gtk.RadioButton.new_with_label_from_widget(cb_active,  label="Sleep")
            hb.pack_end(cb_sleep, True, True, 0)
            cb_sleep.show()

            if flags & 0x10:
                    cb_active.set_active(True)
            elif flags & 0x20:
                    cb_odd.set_active(True)
            elif flags & 0x40:
                    cb_even.set_active(True)
            else:
                    cb_sleep.set_active(True)
                    
            label = Gtk.Label("Set Protect from (IRQ):")
            hb = Gtk.HBox()
            dialog.vbox.pack_end(hb, True, True, 0)
            hb.show()
            hb.pack_start(label, True, True, 0)
            label.show()
            cb_kernel = Gtk.CheckButton("Kernel")
            hb.pack_end(cb_kernel, True, True, 0)
            cb_kernel.show()
            cb_A810 = Gtk.CheckButton("A810")
            hb.pack_end(cb_A810, True, True, 0)
            cb_A810.show()
            cb_McBSP = Gtk.CheckButton("McBSP")
            hb.pack_end(cb_McBSP, True, True, 0)
            cb_McBSP.show()

            if flags & 0x00100000:
                    cb_kernel.set_active(True)
            if flags & 0x00200000:
                    cb_A810.set_active(True)
            if flags & 0x00400000:
                    cb_McBSP.set_active(True)

            response = dialog.run()
            dialog.destroy()
            
            if response == Gtk.ResponseType.OK: #Gtk.RESPONSE_ACCEPT:
                    ier_flags="none"
                    if cb_kernel.get_active():
                            ier_flags=ier_flags+' kernel'
                    if cb_A810.get_active():
                            ier_flags=ier_flags+' a810'
                    if cb_McBSP.get_active():
                            ier_flags=ier_flags+' mcbsp'
                    if cb_active.get_active():
                            self.mk3spm.configure_rt_task(pid, "active", ier_flags)
                    elif cb_odd.get_active():
                            self.mk3spm.configure_rt_task(pid, "odd", ier_flags)
                    elif cb_even.get_active():
                            self.mk3spm.configure_rt_task(pid, "even", ier_flags)
                    elif cb_sleep.get_active():
                            self.mk3spm.configure_rt_task(pid, "sleep", ier_flags)
                    
    def id_process_list_update(self, _labpid, _tn, _ti, _tmr_frq, _ne, _flags, pid):
            flags = self.mk3spm.get_task_control_entry(ii_statemachine_id_task_control, ii_statemachine_id_task_control_flags, pid)
            if flags == 0:
                    _labpid.set_markup("<span color='grey'>ID{:03d}</span>".format(pid+1))
                    _flags.set_markup("<span color='grey'>INACTIVE</span>")
            else:
                    if flags & 0x10:
                            _labpid.set_markup("<span color='blue'>ID{:03d}</span>".format(pid+1))
                            _flags.set_markup("<span color='blue'>ALWAYS</span>")
                    elif flags & 0x20:
                            _labpid.set_markup("<span color='green'>ID{:03d}</span>".format(pid+1))
                            _flags.set_markup("<span color='green'>TIMER</span>")
                    elif flags & 0x40:
                            _labpid.set_markup("<span color='yellow'>ID{:03d}</span>".format(pid+1))
                            _flags.set_markup("<span color='yellow'>CLOCK</span>")
                    else:
                            _labpid.set_markup("<span color='red'>ID{:03d}</span>".format(pid+1))
                            _flags.set_markup("<span color='red'>???</span>")
                            
            _tn.set_text("{:10d}".format(self.mk3spm.get_task_control_entry(ii_statemachine_id_task_control, ii_statemachine_id_task_control_time_next, pid)))
            interval = self.mk3spm.get_task_control_entry(ii_statemachine_id_task_control, ii_statemachine_id_task_control_interval, pid)
            _ti.set_text("{:10d}".format(interval))
            if interval > 0:
                    _tmr_frq.set_text("{:g}Hz".format(150000./interval)+" {:g}ms".format(interval/150.))
            else:
                    _tmr_frq.set_text("-")
            _ne.set_text("{:10d}".format(self.mk3spm.get_task_control_entry(ii_statemachine_id_task_control, ii_statemachine_id_task_control_n_exec, pid)))
            return 1

    def configure_id(self, btn, pid):
            pname  = Gtk.Label(dsp_id_process_name[pid])
            flags = self.mk3spm.get_task_control_entry(ii_statemachine_id_task_control, ii_statemachine_id_task_control_flags, pid)
            label = Gtk.Label("Set flags:")
            dialog = Gtk.Dialog(title="Adjust ID{:03d} Task".format(pid+1), transient_for=self.dsp_man_win, flags=0)
            dialog.add_buttons (Gtk.STOCK_CANCEL, Gtk.ResponseType.CANCEL, Gtk.STOCK_OK, Gtk.ResponseType.OK)

            dialog.vbox.pack_start(pname, True, True, 0)
            pname.show()
            hb = Gtk.HBox()
            dialog.vbox.pack_end(hb, True, True, 0)
            hb.show()
            hb.pack_start(label, True, True, 0)
            label.show()
            cb_always = Gtk.RadioButton.new_with_label_from_widget(None, label="Always")
            hb.pack_end(cb_always, True, True, 0)
            cb_always.show()
            cb_timer = Gtk.RadioButton.new_with_label_from_widget(cb_always, label="Timer")
            hb.pack_end(cb_timer, True, True, 0)
            cb_timer.show()
            cb_clock = Gtk.RadioButton.new_with_label_from_widget(cb_always, label="Clock")
            hb.pack_end(cb_clock, True, True, 0)
            cb_clock.show()
            cb_sleep = Gtk.RadioButton.new_with_label_from_widget(cb_always, label="Sleep")
            hb.pack_end(cb_sleep, True, True, 0)
            cb_sleep.show()

            if flags & 0x10:
                    cb_always.set_active(True)
            elif flags & 0x20:
                    cb_timer.set_active(True)
            elif flags & 0x40:
                    cb_clock.set_active(True)
            else:
                    cb_sleep.set_active(True)
                    
            response = dialog.run()
            dialog.destroy()
            
            if response == Gtk.ResponseType.OK:
                    if cb_always.get_active():
                            self.mk3spm.configure_id_task(pid, "always")
                    elif cb_timer.get_active():
                            self.mk3spm.configure_id_task(pid, "timer")
                    elif cb_clock.get_active():
                            self.mk3spm.configure_id_task(pid, "clock")
                    elif cb_sleep.get_active():
                            self.mk3spm.configure_id_task(pid, "sleep")

    def rtc_get(self, param):
            return self.mk3spm.get_rtos_parameter (param)
        
    def rt_dsprtosinfo_update(self, labtimestructs):
            self.load = 0.9*self.load + 0.1*self.rtc_get (ii_statemachine_DataProcessTime)/self.rtc_get (ii_statemachine_DataProcessReentryTime)
            d = int((self.rtc_get (ii_statemachine_BLK_count_minutes)-1)/(24*60))
            h = int((self.rtc_get (ii_statemachine_BLK_count_minutes)-1)/60 - 24*d);
            m = int((self.rtc_get (ii_statemachine_BLK_count_minutes)-1) - 24*60*d - h*60)
            s = 59 - self.rtc_get (ii_statemachine_DSP_seconds)
            labtimestructs.set_text("DSP TIME STRUCTS: DP TICKS {:10d}   {:10d}s   {:10d}m   {:02d}s\n"
                                    .format(self.rtc_get (ii_statemachine_DSP_time),
                                            self.rtc_get (ii_statemachine_BLK_count_seconds),
                                            self.rtc_get (ii_statemachine_BLK_count_minutes)-1,
                                            self.rtc_get (ii_statemachine_DSP_seconds))
                                    +"DSP STATUS up  {:d} days {:d}:{:02d}:{:02d}, Average Load: {:g}\n".format(d,h,m,s, self.load)
                                    +"DP Reentry Time:  {:10d}  Earliest: {:10d}  Latest:  {:10d}\n"
                                    .format(
                                            self.rtc_get (ii_statemachine_DataProcessReentryTime),
                                            self.rtc_get (ii_statemachine_DataProcessReentryPeak & 0xffff),
                                            self.rtc_get (ii_statemachine_DataProcessReentryPeak >> 16))
                                    +"DP Time: {:10d}  Max: {:10d}  Idle Max: {:10d} Min: {:10d}\n"
                                    .format(
                                            self.mk3spm.get_task_control_entry_task_time(ii_statemachine_rt_task_control, 0),
                                            self.mk3spm.get_task_control_entry_peak_time(ii_statemachine_rt_task_control, 0),
                                            self.rtc_get (ii_statemachine_IdleTime_Peak >> 16),
                                            self.rtc_get (ii_statemachine_IdleTime_Peak & 0xffff))
                                    +"DP Continue Time Limit: {:10d}".format(self.rtc_get (ii_statemachine_DP_max_time_until_abort))
            )
            return 1


    # updates the right side of the offset dialog
    def A810_reading_update(self, _labin, _labout, tap):
            tmp = self.mk3spm.get_ADDA ()

            level_fac = 0.305185095 # mV/Bit
            Vin  = level_fac * tmp[0][tap]
            Vout = level_fac * tmp[1][tap]

            _labin("%+06.3f mV" %Vin)
            _labout("%+06.3f mV" %Vout)

            return 1

    # updates the right side of the offset dialog
    def signal_reading_update(self, _sig_var, _labv, _labsv, tap):
            [value, uv, signal] = self.mk3spm.get_monitor_signal (tap)
            if signal[SIG_INDEX] <= 0:
                    _labv("N/A")
                    _labsv("+####.## mV")
                    return 1

            _sig_var(signal[SIG_VAR])
            _labv(str(value))

            if (signal[SIG_UNIT] == "X"):
                    _labsv("0x%08X " %value)
            else:
                    _labsv("%+06.4f " %uv+signal[SIG_UNIT])
            return 1



    ##### fire up default background updates then run main gtk loop
    def main(self):
            GLib.timeout_add (timeout_DSP_status_reading, self.mk3spm.get_status)        
            GLib.timeout_add (timeout_DSP_signal_lookup_reading, self.mk3spm.read_signal_lookup, 1)

            Gtk.main()


########################################

print ( __name__ )
if __name__ == "__main__":
        mk3 = Mk3_Configurator ()
        mk3.main ()

## END
