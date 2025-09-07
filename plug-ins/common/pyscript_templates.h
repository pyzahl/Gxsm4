/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 8 c-style: "K&R" -*- */

/* Gnome gxsm - Gnome X Scanning Microscopy
 * universal STM/AFM/SARLS/SPALEED/... controlling and
 * data analysis software
 *
 * Gxsm Plugin Name: pyremote.C
 * ========================================
 *
 * Copyright (C) 1999 The Free Software Foundation
 *
 * Authors: Percy Zahl <zahl@fkp.uni-hannover.de>
 * additional features: Andreas Klust <klust@fkp.uni-hannover.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 */

const gchar *template_demo_script = R"V0G0N(
###########################################################################
# Python Gxsm4 remote example script using library gxsm4-lib-scan.
# Before the works you must have the libraries Utils and Scan in place in ~/.gxsm4/pyaction/, for that:
# Please visit via the remote console Menu->Libraries->Lib Utils and Lib Scan, and save each.
# From then on those files are under your control. 
# If you update or want to make sure to get the latest provided version simply delete those files.
# Then come back here and try out this script!

# 1. Import gxsm4-lib-scan.py from ~/.gxsm4/pyaction/, this also includes gxsm4-lib-utils.py and commonly used python libs for you!

#GXSM_USE_LIBRARY <gxsm4-lib-scan>

# 2. create your scan base object:

scan = scan_control()

# 3. use it! Here simply it executes a scan for every bias voltage given in the list:

scan.run_set ([-0.6, -0.4, -0.3])

)V0G0N";

const gchar *template_help = R"V0G0N(
############################################################################
#
# PLEASE EXECUTE ME TO PRINT THE GXSM4 PYTHON REMOTE COMMAND HELP LIST !!
#
############################################################################
S='***'
s='--------------------------------------------------------------------------------'
print (S)
print ('(1) Gxsm4 python remote console -- gxsm.help on build in commands')
print (s)
for h in gxsm.help ():
	print (h)
print ('*')

print (s)
print ('(2) Gxsm4 python remote console -- help on reference names')
print ('  used for gxsm.set and get, gets commands.')
print ('  Hint: hover the pointer over any get/set enabled Gxsm entry to see it`s ref-name!')
print ('  Example: gxsm.set ("dsp-fbs-bias", "1.0")')
print (s)

for h in gxsm.list_refnames ():
	print ('{} \t=>\t {}'.format(h, gxsm.get(h))) 
print ('*')

print (s)
print ('(3) Gxsm4 python remote console -- help on action names used for gxsm.action command')
print ('  Hint: hover the pointer over any Gxsm Action enabled Button to see it`s action-name!')
print ('  Example: gxsm.action ("DSP_CMD_GOTO_Z0")')
print (s)
	
for h in gxsm.list_actions ():
	print (h)
print ('*')
)V0G0N";


const gchar *template_data_access = R"V0G0N(
# pyremote example script for data acess/manipulation/analysis

import os
import array
import numpy
import math

homedir = os.environ['HOME']
basedir=homedir+"/mydatafolder"
print ("Loading Scans for Analysis...")
# CH0 : load scan
gxsm.load (0,basedir+"/"+"mydatafileU001.nc")
# CH1
gxsm.load (1,basedir+"/"+"mydatafileV001.nc")

# fetch dimensions
dims=gxsm.get_dimensions(0)
print (dims)
geo=gxsm.get_geometry(0)
print (geo)
diffs_u=gxsm.get_differentials(0)
print (diffs_u)
diffs_v=gxsm.get_differentials(1)
print (diffs_v)

# create empty scratch array
m = numpy.zeros((dims[1],dims[0]), dtype=float)

print ("calculating...")

# do some fancy math now

gxsm.progress ("Fancy Py Calculus in Porgress", -1) # Init GXSM Progress Display
# go over all pixels
for y in range (0,dims[1]):
    gxsm.progress ("y="+str(y), float(y)/dims[1]) # Update Progress
    for x in range (0, dims[0]):
        m[y][x]=0
        min=1000.
        # find minimul of all data layer in both scans...
        for v in range (0, dims[2]):
            u=gxsm.get_data_pkt (0, x, y, v, 0)*diffs_u[2]
            v=gxsm.get_data_pkt (1, x, y, v, 0)*diffs_v[2]
            if min > u:
                min = u
            if min > v:
                min = v
        m[y][x]=min

gxsm.progress ("Fancy Calc done", 2.) # Close Progress Info

# convert 2d array into single memory2d block
n = numpy.ravel(m) # make 1-d
mem2d = array.array('f', n.astype(float)) 

# CH2 : activate ch 2 and create scan with resulting image data from memory2d
gxsm.chmodea (2)
gxsm.createscanf (2,dims[0],dims[1], geo[0], geo[1], mem2d)

# be nice and auto update/autodisp
gxsm.chmodea (2)
gxsm.direct ()
gxsm.autodisplay ()

print ("update displays completed.")
)V0G0N";


const gchar *template_data_cfextract_simple = R"V0G0N(
# Real world working on data script. 
# You need to adjust input files, z range, settings below.
import os
import array
import numpy
import math

# use simulation file
homedir = os.environ['HOME']
basedir=homedir+"/BNLBox/Data/Pohl/5TTPO1Au-Simulation"
print ("Loading Scans for Analysis...")
# CH0 : load pre processed frequency shift data (const height force maps) in ch 0
gxsm.load (0,basedir+"/"+"2TTPO1PQAu1-sim-df-inlayer.nc")
#gxsm.load (0,basedir+"/"+"1TTPO-sim-df-inlayer.nc")
#gxsm.load (0,basedir+"/"+"1TTPO-sim-df-fzlim-inlayer.nc")

# fetch dimensions
dims=gxsm.get_dimensions(0)
print (dims)
geo=gxsm.get_geometry(0)
print (geo)
diffs_f=gxsm.get_differentials(0)
print (diffs_f)

# create empty scratch array
m = numpy.zeros((dims[1],dims[0]), dtype=float)

print ("calculating...")

vmax=dims[2]

# adjust Z range:
zdata=numpy.arange (19.5,8.5, -0.05)
print (zdata)
dz=-0.05

flv = 35.0 # Hz, frq. level to lock to
rhs = 1    # use righ hand side of f-min
lhs = 0   # use left hand side of f-min

interlevel=8 # interpolation level
zdata_fine=numpy.arange (zdata[0], zdata[zdata.size-1], -(zdata[0]-zdata[1])/interlevel)
#print (zdata_fine)

gxsm.progress ("CdF Image Extract", -1) # Init GXSM Progress Display
# primitiv slow test run
for y in range (0,dims[1]):
	gxsm.progress ("y="+str(y), float(y)/dims[1]) # Update Progress
	for x in range (0, dims[0]):
		m[y][x]=0
		zmin=1000.
		minz=1000.
		minf=1000.
		minv=0
		# Locate Minimum in df curve
		for v in range (0, vmax):
			df=gxsm.get_data_pkt (0, x, y, v, 0)*diffs_f[2]
			z=zdata[v]
			if zmin > z:
				zmin=z
			if minf > df:
				minf=df
				minz=z
				minv=v
			if df > flv+5:
				break
		m[y][x]=minz
		# find df at RHS, use interpol, replace z
		if rhs:
			m[y][x]=zmin
			for v in range (interlevel*minv, interlevel*vmax):
				vi=float(v)/interlevel
				df=gxsm.get_data_pkt (0, x, y, vi, 0)*diffs_f[2]
				z=zdata_fine[v]
				if df > flv:		
					m[y][x]=z
					break
		# find df at LHS, use interpol, replace z
		if lhs:# or m[y][x]==zmin:
			m[y][x]=zmin
			for v in range (interlevel*vmax, 0, -1):
				vi=float(v)/interlevel
				df=gxsm.get_data_pkt (0, x, y, vi, 0)*diffs_f[2]
				z=zdata_fine[v]
				if df > flv:		
					m[y][x]=z
					break
						
gxsm.progress ("CdF Image Extract", 2.) # Close Progress Info

# convert 2d array into single memory2d block
n = numpy.ravel(m) # make 1-d
mem2d = array.array('f', n.astype(float)) 

# CH2 : activate ch 2 and create scan with resulting image data from memory2d
gxsm.chmodea (2)
gxsm.createscanf (2,dims[0],dims[1],1, geo[0], geo[1], mem2d)
gxsm.add_layerinformation ("@ "+str(flv)+" Hz",10)

# be nice and auto update/autodisp
gxsm.chmodea (2)
gxsm.direct ()
gxsm.autodisplay ()

print ("update displays completed.")
)V0G0N";

const gchar *template_data_cfextract_data = R"V0G0N(
# Real world working on data script. 
# You need to adjust input files and settings below.
import os
import array
import numpy
import math

homedir = os.environ['HOME']
basedir=homedir+"/BNLBox/Data/Pohl/20170612-Au-TTPO-exorted-data"
print ("Loading Scans for Analysis...")
# CH0 : load pre processed frequency shift data (const height force maps) in ch 0
gxsm.load (0,basedir+"/"+"Au111-TTPO-V3004-3025-Xp-PLL-Exci-Frq-dirftcorrected-cropped-LOG12-f0-inlayer.nc")
# CH1 : load pre processed matching topo data (actual const height value, may deviate from const number if compliance mode is used) in ch 1
gxsm.load (1,basedir+"/"+"Au111-TTPO-V3004-3025-Xp-PLL-Topo-dirftcorrected-cropped317-inlayer.nc")

# fetch dimensions
dims=gxsm.get_dimensions(0)
print (dims)
geo=gxsm.get_geometry(0)
print (geo)
diffs_f=gxsm.get_differentials(0)
print (diffs_f)
diffs_z=gxsm.get_differentials(1)
print (diffs_z)

# create empty scratch array
m = numpy.zeros((dims[1],dims[0]), dtype=float)

print ("calculating...")

flv = -1.0 # Hz, frq. level to lock to
rhs = 1    # use righ hand side of f-min
lhs = 0   # use left hand side of f-min

interlevel=8 # interpolation level

gxsm.progress ("CdF Image Extract", -1) # Init GXSM Progress Display
# primitiv slow test run
for y in range (0,dims[1]):
	gxsm.progress ("y="+str(y), float(y)/dims[1]) # Update Progress
	for x in range (0, dims[0]):
		m[y][x]=0
		zmin=1000.
		minz=1000.
		minf=1000.
		minv=0
		# Locate Minimum in df curve
		for v in range (0, dims[2]):
			df=gxsm.get_data_pkt (0, x, y, v, 0)*diffs_f[2]
			z=gxsm.get_data_pkt (1, x, y, v, 0)*diffs_z[2]
			if zmin > z:
				zmin=z
			if minf > df:
				minf=df
				minz=z
				minv=v
		m[y][x]=minz
		# find df at RHS, use interpol, replace z
		if rhs:
			m[y][x]=zmin
			for v in range (interlevel*minv, interlevel*dims[2]):
				vi=float(v)/interlevel
				df=gxsm.get_data_pkt (0, x, y, vi, 0)*diffs_f[2]
				z=gxsm.get_data_pkt (1, x, y, vi, 0)*diffs_z[2]
				if df > flv:		
					m[y][x]=z
					break
		# find df at LHS, use interpol, replace z
		if lhs:
			m[y][x]=zmin
			for v in range (interlevel*dims[2], 0, -1):
				vi=float(v)/interlevel
				df=gxsm.get_data_pkt (0, x, y, vi, 0)*diffs_f[2]
				z=gxsm.get_data_pkt (1, x, y, vi, 0)*diffs_z[2]
				if df > flv:		
					m[y][x]=z
					break
						
gxsm.progress ("CdF Image Extract", 2.) # Close Progress Info

# convert 2d array into single memory2d block
n = numpy.ravel(m) # make 1-d
mem2d = array.array('f', n.astype(float)) 

# CH2 : activate ch 2 and create scan with resulting image data from memory2d
gxsm.chmodea (2)
gxsm.createscanf (2,dims[0],dims[1],1, geo[0], geo[1], mem2d)
gxsm.add_layerinformation ("@ "+str(flv)+" Hz",10)

# be nice and auto update/autodisp
gxsm.chmodea (2)
gxsm.direct ()
gxsm.autodisplay ()

print ("update displays completed.")
)V0G0N";


const gchar *template_animate = R"V0G0N(
# Set a channel to Surface 3D view mode, remote will only apply to the current active GL Scen Setup/Control.

ti=0 # start at t-initial = 0
tf=200 # assume dt = 1 about 1/10s

phii=-120.0
phif=70.0

fovi=60.0
fovf=20.0
for t in range(ti, tf,1):
	gxsm.set("rotationz",str(phii+(phif-phii)*t/tf)) # adjust Z rotation (or any of the 3D view settings)
	gxsm.set("fov",str(fovi+(fovf-fovi)*t/tf)) # adjust Z rotation (or any of the 3D view settings)
	gxsm.set("animationindex",str(t+1)) # this will save the frame on every new index value!
	gxsm.sleep(0.01) # give GXSM a moment to update displays...
	#gxsm.sleep(1) # give GXSM a moment to update displays...
)V0G0N";


const gchar *template_movie_drawing_export = R"V0G0N(
# Save Drawing (many layers/times) as png image file series (via Cairo)

ch=0 # scratch channel used
output_filebasename = "/tmp/test" # export file path and file name base, a count is added

#gxsm.load (ch, "my_multilayer_or_time_data_file.nc") # or pre load manually in channel 1 and leave this commented out

[nx,ny,nv,nt]=gxsm.get_dimensions (ch)
for time in range (0,nt):
        # gxsm.set ("TimeSelect", str(time)) ## will also work as a generic example using entry set controls
	for layer in range (0,nv):
		print (time, layer);
		# gxsm.set ("LayerSelect", str(layer)) ## will also work as a generic example using entry set controls
		gxsm.set_view_indices (ch, time, layer) ## build in directly set both for ch is more efficient
		# gxsm.autodisplay () ## if you like, uncomment this, else manually set high/low limitis in viewcontrol to keep fixed
		gxsm.sleep(0.1) # give gxsm a moment to update
		gxsm.save_drawing (ch, time, layer, output_filebasename+'_T%d'%time+'_L%d.png'%layer )
)V0G0N";

const gchar *template_watchdog = R"V0G0N(
# Watch dog script. Watching via RTQuery system parameters:
# for example dF and if abs(dF) > limit DSP_CMD_STOPALL is issued (cancel auto approch, etc.)

# Watch dog script. Watching via RTQuery system parameters:
# for example dF and if abs(dF) > limit DSP_CMD_STOPALL is issued (cancel auto approch, etc.)

def z0_goto(set_z0=0, speed=400):
        gxsm.set ("dspmover-z0-goto","%g"%set_z0)
        gxsm.set ("dspmover-z0-speed","%g"%speed)
        gxsm.action ("DSP_CMD_GOTO_Z0")
        svec=gxsm.rtquery ("o")  ## in HV volts
        z0 = svec[0]
        print ("Retract ** Offset Z0 = ", z0)
        gxsm.logev ('Remote Z0 Retract')
        gxsm.sleep (30)
#       
#       while abs(z0-set_z0) > 1.:
#               gxsm.action ("DSP_CMD_GOTO_Z0")
#               gxsm.sleep (200)
#               svec=gxsm.rtquery ("o")
#               z0 = svec[0]
#               svec=gxsm.rtquery ("z")
#               print ("Z0=", z0, svec)
        return z0

# Coarse Z0/Offset Tools/Approach custom
def xxz0_retract():
        z0_goto(-160)

def z0_retract():
        gxsm.set ("dspmover-z0-speed","1000")
        svec=gxsm.rtquery ("o")  ## in HV volts
        z0 = svec[0]
        print ("Retarct ** Offset Z0 = ", z0)
        gxsm.logev ('Remote Z0 Retract')
        while z0 > -160.:
                gxsm.action ("DSP_CMD_GOTO_Z0")
                gxsm.sleep (10)
                svec=gxsm.rtquery ("o")
                z0 = svec[0]
                svec=gxsm.rtquery ("z")
                print ("Z0=", z0, svec)

def z_in_range_check ():
        svec=gxsm.rtquery ("z") ## in HV Volts
        print ("ZXYS=", svec)
        return svec[0] > -15

def z0_approach():
        gxsm.set ("dspmover-z0-speed","600")
        svec=gxsm.rtquery ("o") ## in HV Volts
        z0 = svec[0]
        print ("Approach ** Offset Z0 = ", z0)
        gxsm.logev ('Remote Z0 Approach')
        while z0 < 180. and not z_in_range_check ():
                gxsm.action ("DSP_CMD_AUTOCENTER_Z0")
                gxsm.sleep (10)
                svec=gxsm.rtquery ("o")
                z0 = svec[0]
                svec=gxsm.rtquery ("z")
                print ("Z0=", z0, svec)
                sc = gxsm.get ("script-control")
                if sc < 1:
                        break
        return svec[0]

def autoapproach_via_z0():
        gxsm.set ("dspmover-z0-goto","-1600")
        count=0
        gxsm.logev ('Remote Auto Approach start')
        while not z_in_range_check():
                z0_retract ()
                gxsm.sleep (20)
                gxsm.logev ('Remote Auto Approach XP-Auto Steps')
                gxsm.action ("DSP_CMD_MOV-ZP_Auto")
                gxsm.sleep (20)
                zs=z0_approach ()
                count=count+1
                gxsm.logev ('Remote Auto Approach #' + str(count) + ' ZS=' + str (zs))
                sc = gxsm.get ("script-control")
                if sc < 1:
                        break
                
        gxsm.set ("dspmover-z0-speed","400")
        gxsm.logev('Remote Auto Approach completed')


def watch_dog_run(limit=25):
        gxsm.action ("DSP_CMD_AUTOAPP")
        df=0.
        while abs(df) < limit: 
                gxsm.sleep (10)
                fvec=gxsm.rtquery ("f")
                df = fvec[0]
                print ("dF=",df)
                gxsm.logev("Watchdog dF=%gHz"%df)
                sc = gxsm.get ("script-control")
                if sc < 1:
                        break

        gxsm.action ("DSP_CMD_STOPALL")
        gxsm.logev("Watchdog DSP_CMD_STOPALL ACTION as of dF=%gHz"%df)
        return df

def watch_dog_df_autoapp():
        df_abort = 25
        df=0.
        z0_goto (300,400)
        svec=gxsm.rtquery ("o") ## in HV Volts
        z0 = svec[0]
        print ("Approach ** Offset Z0 = ", z0)
        gxsm.logev ('Remote Z0 Approach')
        while z0 < 180. and not z_in_range_check () and abs(df) < df_abort:
                df = watch_dog_run (df_abort)
                gxsm.sleep (200)
                sc = gxsm.get ("script-control")
                if sc < 1:
                        break

        return svec[0]

def test_rt_param_query ():
        gxsm.sleep (200)
        fvec=gxsm.rtquery ("f")
        df = fvec[0]
        print ("dF=",df)
        zvec=gxsm.rtquery ("z")
        print ('Zvec=', zvec)
        zvec=gxsm.rtquery ("z0")
        print ('Z0vec=', zvec)
        gxsm.logev("Watchdog Check dF=%gHz" %df + " zvec=%g A" %zvec[0])
        print ("Watchdog Abort")


#autoapproach_via_z0()

watch_dog_df_autoapp()

## FINISH DEFAULTS

print ("Finished/Aborted.")

gxsm.set ("dspmover-z0-goto","-1600")
gxsm.set ("dspmover-z0-speed","400")


)V0G0N";

const gchar *template_gxsm_sok_server = R"V0G0N(
# Load scan support object from library
# GXSM_USE_LIBRARY <gxsm4-lib-scan>

## GXSM PyRemote socket server script

import selectors
import socket
import sys
import os
import fcntl
import json

## CONNECTION CONFIG

HOST = '127.0.0.1'  # Standard loopback interface address (localhost)
PORT = 65432        # Port to listen on (non-privileged ports are > 1023)

##

sys.stdout.write ("************************************************************\n")
sys.stdout.write ("* GXSM Py Socket Server is listening on "+HOST+":"+str(PORT)+"\n")
sc=gxsm.get ("script-control")
sys.stdout.write ('* Script Control is set to {} currently.\n'.format(int(sc)))
sys.stdout.write ('* Set Script Control >  0 to keep server alife! 0 will exit.\n')
sys.stdout.write ('* Set Script Control == 1 for idle markings...\n')
sys.stdout.write ('* Set Script Control == 2 for silence.\n')
sys.stdout.write ('* Set Script Control >  2 minial sleep, WARNIGN: GUI may be sluggish.\n')
sys.stdout.write ('* Set Script Control == 4 fast (recommended for new threadded pyremote).\n')
sys.stdout.write ("************************************************************\n\n")

gxsm.set ("script-control", '4')

sys.stdout.write ('*** Set Script Control ==  0 to terminate server. ***\n\n')

## message processing

def process_message(jmsg):
    #print ('processing JSON:\n', jmsg)
    for cmd in jmsg.keys():
        #print('Request = {}'.format(cmd))
        if cmd == 'command':
            for k in jmsg['command'][0].keys():
                if k == 'set':
                    ## {'command': [{'set': id, 'value': value}]}

                    #print('gxsm.set ({}, {})'.format(jmsg['command'][0]['set'], jmsg['command'][0]['value']))
                    gxsm.set(jmsg['command'][0]['set'], jmsg['command'][0]['value'])
                    return {'result': [{'set':jmsg['command'][0]}]}

                elif k == 'gets':
                    ## {'command': [{'get': id}]}

                    #print('gxsm.gets ({})'.format(jmsg['command'][0]['gets']))
                    value=gxsm.gets(jmsg['command'][0]['gets'])
                    #print(value)
                    return {'result': [{'gets':jmsg['command'][0]['gets'], 'value':value}]}


                elif k == 'get':
                    ## {'command': [{'get': id}]}

                    #print('gxsm.get ({})'.format(jmsg['command'][0]['get']))
                    value=gxsm.get(jmsg['command'][0]['get'])
                    #print(value)
                    return {'result': [{'get':jmsg['command'][0]['get'], 'value':value}]}

                elif k == 'query':
                    ## {'command': [{'query': x, 'args': [ch,...]}]}
                    
                    if (jmsg['command'][0]['query'] == 'chfname'):
                        value=gxsm.chfname(jmsg['command'][0]['args'][0])
                        return {'result': [{'query':jmsg['command'][0]['query'], 'value':value}]}

                    elif (jmsg['command'][0]['query'] == 'y_current'):
                        value=gxsm.y_current()
                        return {'result': [{'query':jmsg['command'][0]['query'], 'value':value}]}

                    else:
                        return {'result': 'invalid query command'}
                else:
                    return {'result': 'invalid command'}

        elif cmd == 'action':
            if (jmsg['action'][0] == 'start-scan'):
                gxsm.startscan ()
                return {'result': 'ok'}
            elif (jmsg['action'][0] == 'stop-scan'):
                gxsm.stopscan ()
                return {'result': 'ok'}
            elif (jmsg['action'][0] == 'autosave'):
                value = gxsm.autosave ()
                return {'result': 'ok', 'ret':value}
            elif (jmsg['action'][0] == 'autoupdate'):
                value = gxsm.autoupdate ()
                return {'result': 'ok', 'ret':value}
            else:
                return {'result': 'invalid action'}
        elif cmd == 'echo':
            return {'result': [{'echo': jmsg['echo'][0]['message']}]}
        else: return {'result': 'invalid request'}
            
## socket server 
    
# set sys.stdin non-blocking
def set_input_nonblocking():
    orig_fl = fcntl.fcntl (sys.stdin, fcntl.F_GETFL)
    fcntl.fcntl (sys.stdin, fcntl.F_SETFL, orig_fl | os.O_NONBLOCK)

def create_socket (server_addr, max_conn):
    sys.stdout.write ("*** GXSM Py Socket Server is now listening on "+HOST+":"+str(PORT)+"\n")
    server = socket.socket (socket.AF_INET, socket.SOCK_STREAM)
    server.setsockopt (socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    server.setblocking (False)
    server.bind (server_addr)
    server.listen (max_conn)
    return server

def try_connect (server, server_addr):
    try:
        server.connect (server_addr)
        return True
    except:
        pass
    return False

def read_direct(conn, mask):
    global keep_alive
    try:
        client_address = conn.getpeername ()
        data = conn.recv (1024)
        #print ('Got {} from {}'.format(data, client_address))
        try:
            serialized = json.dumps(process_message (data))
        except (TypeError, ValueError):
            serialized = json.dumps({'JSON-Error'})
            raise Exception('You can only send JSON-serializable data')
        ret = '{}\n{}'.format(len(serialized), serialized)
        sys.stdout.write ('Return JSON for {} => {}\n'.format(data,ret))
        conn.sendall (ret.encode('utf-8'))
        if not data:
            keep_alive = True
    except:
        pass
      
def read(conn, mask):
    global keep_alive
    try:
        client_address = conn.getpeername ()
        jdata = receive_json(conn)
        if jdata:
            #print ('Got {} from {}'.format(jdata, client_address))
            ret=process_message (jdata)
            sys.stdout.write ('Return JSON for {} => {}\n'.format(jdata,ret))
            send_as_json(conn, ret)
        if not jdata:
            keep_alive = True
    except:
        pass
            
def send_as_json(conn, data):
    try:
        serialized = json.dumps(data)
    except (TypeError, ValueError):
        print ('JSON-SEND-Error')
        serialized = json.dumps({'JSON-SEND-Error'})
        raise Exception('You can only send JSON-serializable data')
    # send the length of the serialized data first
    sd = '{}\n{}'.format(len(serialized), serialized)
    sys.stdout.write ('Sending JSON: {}\n'.format(sd))
    conn.sendall (sd.encode('utf-8'))

def receive_json(conn):
    try:
        # try simple assume one message
        data = conn.recv (1024)
        if data:
            count, jsdata = data.split(b'\n')
            #print ('Got Data N={} D={}'.format(count,jsdata))
            try:
                deserialized = json.loads(jsdata)
            except (TypeError, ValueError):
                print('EE JSON-Deserialize-Error\n')
                deserialized = json.loads({'JSON-Deserialize-Error'})
                raise Exception('Data received was not in JSON format')
            return deserialized
    except:
        pass

def receive_json_long(conn):
    # read the length of the data, letter by letter until we reach EOL
    length_str = b''
    char = conn.recv(1)
    while char != '\n':
        length_str += char
        char = conn.recv(1)
    total = int(length_str)
    # use a memoryview to receive the data chunk by chunk efficiently
    view = memoryview(bytearray(total))
    next_offset = 0
    while total - next_offset > 0:
        recv_size = conn.recv_into(view[next_offset:], total - next_offset)
        next_offset += recv_size
        try:
            deserialized = json.loads(view.tobytes())
        except (TypeError, ValueError):
            raise Exception('Data received was not in JSON format')
    return deserialized
    
def accept(sock, mask):
    new_conn, addr = sock.accept ()
    new_conn.setblocking (False)
    print ('Accepting connection from {}'.format(addr))
    m_selector.register(new_conn, selectors.EVENT_READ, read)

def quit ():
    global keep_alive
    print ('Exiting...')
    keep_alive = False

def from_keyboard(arg1, arg2):
    line = arg1.read()
    if line == 'quit\n':
        quit()
    else:
        print('User input: {}'.format(line))

## MAIN ##

keep_alive = True

m_selector = selectors.DefaultSelector ()

set_input_nonblocking()
m_selector.register (sys.stdin, selectors.EVENT_READ, from_keyboard)

sys.stdout.write ("*** GXSM Py Socket Server is (re)starting...\n")
# listen to port 10000, at most 10 connections

server_addr = (HOST, PORT)
server = create_socket (server_addr, 10)
m_selector.register (server, selectors.EVENT_READ, accept)

while keep_alive:
    sc=gxsm.get ("script-control")
    if sc == 1:
        sys.stdout.write ('.')
        # sys.stdout.flush ()

    for key, mask in m_selector.select(0):
        callback = key.data
        callback (key.fileobj, mask)

    if sc > 3: # sc=4
        gxsm.sleep (0.01) # 1ms
    elif sc > 2: # sc=3
        gxsm.sleep (0.1) # 10ms
    else: # sc=2
        gxsm.sleep (1) # 100ms

    if sc == 0:
        sys.stdout.write ('\nScript Control is 0:  Closing server down now.\n')
        quit ()

sys.stdout.write ("*** GXSM Py Socket Server connection shutting down...\n")
m_selector.unregister (server)

# close connection
server.shutdown (socket.SHUT_RDWR)
server.close ()

# unregister events
m_selector.unregister (sys.stdin)

#  close select
m_selector.close ()

sys.stdout.write ("*** GXSM Py Socket Server Finished. ***\n")
)V0G0N";


const gchar *template_thv5_autoapp_script = R"V0G0N(
# Createc THV5 Autoapp Script
import time
import requests


class THV5():
#!/usr/bin/env python3

### #!/usr/bin/env python

## * Python User Control for
## * Createc HV5
## * 
## * Copyright (C) 2013 Percy Zahl
## *
## * Author: Percy Zahl 
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

import faulthandler; faulthandler.enable()

version = "1.0.0"

import gi
gi.require_version('Gtk', '4.0')

from gi.repository import Gtk, Gdk, GLib

import cairo
import os                # use os because python IO is bugy
import time
import fcntl
from threading import Timer

#import GtkExtra
import struct
import array

import math

import requests
import numpy as np

from multiprocessing import shared_memory
from multiprocessing.resource_tracker import unregister

from meterwidget import *

METER_SCALE = 0.66

FALSE = 0
TRUE  = -1

# Createc HV Control

SWAP_CXY=1

# RPSPMC XYZ Monitors via GXSM4 
global gxsm_shm

global rpspmc

rpspmc = {
        'bias': 0.0,
        'current': 0.0,
        'gvp' : { 'x':0.0, 'y':0.0, 'z': 0.0, 'u': 0.0, 'a': 0.0, 'b': 0.0, 'am':0.0, 'fm':0.0 },
        'pac' : { 'dds_freq': 0.0, 'ampl': 0.0, 'exec':0.0, 'phase': 0.0, 'freq': 0.0, 'dfreq': 0.0, 'dfreq_ctrl': 0.0 }
        }

global CHV5_configuration
global CHV5_driftcomp
global CHV5_monitor
global CHV5_coarse
global CHV5_gain_list
global CHV5_gains

CHV5_configuration = {
        'gain':  [2,2,3],
        'filter': [0,0,0],
        'bw': [0,0,0],
        'target': [0.,0.,0.]
}

CHV5_monitor = {
        'monitor': [0.0,0.0,0.0],
        'monitor_min': [0.0,0.0,0.0],
        'monitor_max': [0.0,0.0,0.0],
        }

CHV5_gain_list = [3,6,12,24]
CHV5_gains = [12., 12., 24.]


CHV5_coarse = {
        'steps': [10,10,5],
        'volts': [100.0,100.0,75.0],
        'period': [500,500,500]
        }

CHV5_driftcomp = [ 0., 0., 0. ]

DIGVfacM = 1. #200./32767.
DIGVfacO = 1. #181.81818/32767.

scaleM = [ DIGVfacM, DIGVfacM, DIGVfacM ]
scaleO = [ DIGVfacO, DIGVfacO, DIGVfacO ]
unit  = [ "V", "V", "V", "dB" ]

updaterate = 88        # update time watching the SRanger in ms

GXSM_Link_status = FALSE

setup_list = []
control_list = []

# open shared memort to gxsm4 to RPSPMC's monitors

try:
        gxsm_shm = shared_memory.SharedMemory(name='gxsm4rpspmc_monitors')
        unregister(gxsm_shm._name, 'shared_memory')

        print (gxsm_shm)
        xyz=np.ndarray((9,), dtype=np.double, buffer=gxsm_shm.buf).reshape((3,3)).T  # X Mi Ma, Y Mi Ma, Z Mi Ma
        print (xyz)
except FileNotFoundError:
        print ("SharedMemory(name='gxsm4rpspmc_monitors') not available. Please start gxsm4 and connect RPSPMC.")

               
#    // Sets HV gain parameters and filter settings
#    // @param gainx,gainy,gainz - Gain values for each axis (1-255)
#    // @param filter - Enable/disable primary filter XY
#    // @param filter2nd - Enable/disable secondary filter Z
#    // @param coarseboard - Board number for coarse voltage control
#    // @param coarsevoltage - Voltage setting for coarse control (0-255)
#    // @return 0 if successful
#    function SetHVGain(gainx, gainy, gainz: Integer; filter, filter2nd: Boolean;
#      coarseboard, coarsevoltage: Integer): Integer;
#
#    // Controls coarse movement for specified channel
#    // @param channel - Channel number (1-3 for X,Y,Z)
#    // @param direction - Movement direction (-1 or 1)
#    // @param burstcount - Number of steps to move
#    // @param period - Time period between steps
#    // @param start - Start flag (1 to start movement)
#    // @return 0 if successful
#    function CoarseMove(channel, direction, burstcount, period, start: Integer): Integer;

class THV5():
        def __init__(self, ip):
                self.besocke = False
                self.ip = ip

        def request_stmafm(self, data):
                #idhttp1.Get(ipstring + 'stmafm?' + command, ResponseStream);
                print (self.ip + 'stmafm?' + data)
                #return requests.get(self.ip + 'stmafm?' + data)

        def request(self, data):
                #idhttp1.Get(ipstring + 'stmafm?' + command, ResponseStream);
                print (self.ip + data)
                return requests.get(self.ip + data)


        def request_coarse(self, data):
                #idhttp1.Get(ipstring + 'stmafm?' + command, ResponseStream);
                print (self.ip + 'coarse?' + data)
                #return requests.get(self.ip + 'stmafm?' + data)

                #http://192.168.40.10/coarse?v0=150&p0=500&a0=Z&c0=33&bs=0

        #http://192.168.40.10/gain?g0=12&l0=ON&e0=OFF&g1=12&l1=ON&e1=OFF&g2=12&l2=ON&e2=ON&g3=3&l3=OFF&e3=OFF
        # X 12x, lowpass on, endfilter off
        # Y 12x, lowpass on, endfilter off
        # Z 12x, lowpass on, endfilter on
        # AUX 3 off off
                
        # gain: 3,6,12,24  filters: 'ON','OFF' 
        def SetTHVGain(self, gainx, gainy, gainz):
                return self.request ('gain?gain?g0={}&l0=ON&e0=OFF&g1={}&l1=ON&e1=OFF&g2={}&l2=ON&e2=ON&g3=3&l3=OFF&e3=OFF'.format(gainx, gainy, gainz))
                #return self.request ('gain?f1={}&f2={}&g0={}&g1=&g2={}&v{}={}'.format(filter, filter2nd, gainx, gainy, gainz, coarseboard, coarsevoltage))


        # http://192.168.40.10/coarse?v0=150&p0=500&a0=Z&c0=10000
        # http://192.168.40.10/coarse?v0=150&p0=500&a0=Z&c0=33&bs=0     150V 500 us Z 33 steps   START
        # http://192.168.40.10/coarse?v0=150&p0=500&a0=X&c0=33&bs=0
        # http://192.168.40.10/coarse?v0=150&p0=500&a0=Y&c0=33&bs=0
        #
        # http://192.168.40.10/coarse?v0=150&p0=500&a0=Y&c0=33          SAVE
        #
        # 
        
        def SetTHVCoarseMove(self, channel, direction, burstcount, period, voltage, start):
                c=['X','Y','Z']
                ## http://192.168.40.10/coarse?v0=15&p0=500&a0=Z&c0=1&bs=0
                if start:
                        return self.request ('coarse?v={}&p0={}&a0={}&c0={}&bs=0'.format(int(voltage), int(period), c[channel], burstcount*direction))
                else:
                        return self.request ('coarse?v={}&p0={}&a0={}&c0={}&bs=0'.format(0, 0, c[channel], 0))

        #return self.request ('k0={}&c0={}&p0={}&bb=0'.format(channel, burstcount*direction, period))
        #return self.request ('k0={}&c0={}&p0={}'.format(channel, burstcount*direction, period))

        # Prizm
        #if start:
        #        return self.request ('a0={}&c0={}&p0={}&bs=0'.format(channel, burstcount*direction, period))
        #else:
        #        return self.request ('a0={}&c0={}&p0={}'.format(channel, burstcount*direction, period))

        def THVReset (self):
                return self.request ('fd=1')


        def THVStatus (self):
                settings = self.request ('')
                print (settings)
                return settings.split('&')
        
                        
class gxsm_link():
        def __init__(self, oc):
                print ("Gxsm Link Init.")

        def offset_adjust (self, start, count):
                global Z0_invert_status
                ticks = time.time()
                if cound >= 0:
                        t = Timer(self.interval - (ticks-start-count*self.interval), self.offset_adjust, [start, count+1])
                        t.start()
                v = [0.,0.,0.]
                print ("Number of ticks since 12:00am, January 1, 1970:", ticks, " #", count, v)
            
        def start (self):
                self.count = 0
                self.interval = 0.100 # interval in sec
                self.t = Timer(self.interval, self.offset_adjust, (time.time(), 0)) # start over at full second, for testing only
                self.t.start()
        
        def stop (self):
                self.count = -1

        def enable_gain_control (self):
                self.gxsm_gain_control_link = True;

        def disable_gain_control (self):
                self.gxsm_gain_control_link = False;

                
        def status (self):
                return 0



def delete_event(win, event=None):
        global GXSM_Link_status
        print ("Link: ", GXSM_Link_status)
        if GXSM_Link_status:
                print ("GXSM Link is active, please disable Link to quit")
                idlg = Gtk.MessageDialog(
                        flags=0,
                        message_type=Gtk.MessageType.INFO,
                        buttons=Gtk.ButtonsType.OK,
                        text="Warning: GXSM Link is active!\nLink will go done on SPD Control termination.")
                idlg.run()
                idlg.destroy()
                return False
        else:
                win.hide()
                Gtk.main_quit()
        return True

def make_menu_item(name, _object, callback, value, identifier):
        item = Gtk.MenuItem(name)
        item.connect("activate", callback, value, identifier)
        item.show()
        return item

def read_back():
        global CHV5_configuration
        
def _X_write_back_():
        global CHV5_configuration

## show only current
global QPAmpl
QPAmpl = [0.,0.,0.]
def update_CHV5_monitor(_c1set, _c2set, _c3set, _cqp):
        global CHV5_configuration
        global CHV5_monitor
        global rpspmc
        global QPAmpl

        _c1set (scaleM[0] * CHV5_monitor['monitor'][0], scaleM[0] * CHV5_monitor['monitor_min'][0], scaleM[0] * CHV5_monitor['monitor_max'][0])
        _c2set (scaleM[1] * CHV5_monitor['monitor'][1], scaleM[1] * CHV5_monitor['monitor_min'][1], scaleM[1] * CHV5_monitor['monitor_max'][1])
        _c3set (scaleM[2] * CHV5_monitor['monitor'][2], scaleM[2] * CHV5_monitor['monitor_min'][2], scaleM[2] * CHV5_monitor['monitor_max'][2])

        ## print (rpspmc)
        QPAmpl[0] = -100+10*rpspmc['pac']['ampl']
        if QPAmpl[0] > 0.0:
                QPAmpl[0] = 20*(math.log10(rpspmc['pac']['ampl'])-1)
        #rpspmc['pac']['dfreq']rpspmc['pac']['dfreq']
        _cqp (QPAmpl[0], 5*(rpspmc['pac']['dfreq']-1))
        #_cqp (0, -10, -100)  # 0 -> 1dB   10 -> +20dB    -10 -> 0dB   -100 -> -20dB
        #mu = 0.02
        #QPAmpl[2] = QPAmpl[0] if QPAmpl[2] < QPAmpl[0] else (1-mu)*QPAmpl[2]+mu*QPAmpl[0]
        #QPAmpl[1] = QPAmpl[0] if QPAmpl[1] > QPAmpl[0] else (1-mu)*QPAmpl[2]+mu*QPAmpl[0]
        
        return 1

## show min/max at 1Hz
def update_CHV5_monitor_full(_c1set, _c2set, _c3set):
        global CHV5_configuration
        global CHV5_monitor

        spantag  = "<span size=\"24000\" font_family=\"monospace\">"   # .. color=\"#ff0000\"
        spantag2 = "<span size=\"10000\" font_family=\"monospace\">"   # .. color=\"#ff0000\"
                
        _c1set (spantag + "<b>%8.3f " %(scaleM[0]*CHV5_monitor[ii_monitor_X]) + unit[0] + "</b></span>\n"
                + spantag2
                + "%8.3f " %(scaleM[0]*CHV5_monitor[ii_monitor_Xmin]) + unit[0]
                + "%8.3f " %(scaleM[0]*CHV5_monitor[ii_monitor_Xmax]) + unit[0]
                +" </span>")
        _c2set (spantag + "<b>%8.3f " %(scaleM[1]*CHV5_monitor[ii_monitor_Y]) + unit[1] + "</b></span>\n"
                + spantag2
                + "%8.3f " %(scaleM[0]*CHV5_monitor[ii_monitor_Ymin]) + unit[0]
                + "%8.3f " %(scaleM[0]*CHV5_monitor[ii_monitor_Ymax]) + unit[0]
                +" </span>")
        _c3set (spantag + "<b>%8.3f " %(scaleM[2]*CHV5_monitor[ii_monitor_Z]) + unit[2] + "</b></span>\n"
                + spantag2
                + "%8.3f " %(scaleM[0]*CHV5_monitor[ii_monitor_Zmin]) + unit[0]
                + "%8.3f " %(scaleM[0]*CHV5_monitor[ii_monitor_Zmax]) + unit[0]
                +" </span>")
        
        return 1


def hv1_adjust(_object, _value, _identifier):
        global CHV5_gain_list
        global CHV5_gains
        value = _value
        index = _identifier
        #if index >= ii_config_preset_X0 and index < ii_config_dum:

        CHV5_gains[index] = CHV5_gain_list[value]

        print ('HV-adjust: ', index, value, ', CHV5 Gains: ', CHV5_gains, ' ... may toggle to enforce up-to-now.')

        read_back ()
        return 1


        
class offset_vector():
    def __init__(self, etv, gsv):
        self.etvec = etv
        self.gainselectmenuvec = gsv

    def update_display(self):
        for i in range (0,3):
            self.etvec[i].set_text("%12.3f" %(scaleO[i]*CHV5_configuration['target'][i]))
        return False

    def write_t_vector(self, button):
        global CHV5_configuration
        tX = [0,0,0]
        for i in range (0,3):
            tX[i] = int (float (self.etvec[i].get_text ()) / scaleO[i]);
            if tX[i] > 32766:
                    tX[i] = 32766
            if tX[i] < -32766:
                    tX[i] = -32766

#        print (tX)

        ##os.write (sr.fileno(), struct.pack ("<hhh", tX[0], tX[1], tX[2]))
        
        read_back ()
        GLib.idle_add (self.update_display)

    def write_t_vector_var(self, tvec):
        global CHV5_configuration
        tX = [0,0,0]
        for i in range (0,3):
            tX[i] = int (tvec[i] / scaleO[i]);
            if tX[i] > 32766:
                    tX[i] = 32766
            if tX[i] < -32766:
                    tX[i] = -32766
            
#        print (tX)

        ##os.write (sr.fileno(), struct.pack ("<hhh", tX[0], tX[1], tX[2]))
        
        read_back ()
        GLib.idle_add (self.update_display)

class drift_compensation():
    def __init__(self, edv, oc):
        self.count = -1
        self.edvec = edv
        self.offset_ctrl = oc

    def update (self):
        self.tvd = [0.,0.,0.] # differential offset vector (drift rate in 1 / sec)
        for i in range (0,3):
            self.tvd[i] = float (self.edvec[i].get_text ())

        read_back ()
        self.tvi = [0.,0.,0.,0.] # initial offset vector
        for i in range (0,3):
            self.tvi[i] = scaleO[i] * CHV5_configuration['target'][i]
        self.tvi[3] = time.time() # initial time

        self.offset_ctrl.write_t_vector_var(self.tvi)

    def offset_adjust (self, start, count):
        if self.count >= 0:
            self.count = count
            ticks = time.time()
            t = Timer(self.interval - (ticks-start-count*self.interval), self.offset_adjust, [start, count+1])
            t.start()
            v = [0.,0.,0.]
            for i in range (0,3):
                v[i] = self.tvi[i] + (ticks - self.tvi[3]) * self.tvd[i]
            self.offset_ctrl.write_t_vector_var(v)
#            print ("Number of ticks since 12:00am, January 1, 1970:", ticks, " #", count, v)
        
    def start (self):
        self.update ()
        self.count = 0
        self.interval = 0.05 # interval in sec
        self.t = Timer(self.interval, self.offset_adjust, (time.time(), 0)) # start over at full second, for testing only
        self.t.start()
        
    def stop (self):
        self.count = -1


def toggle_configure_widgets (w):
        print ("CONFIG STATES:")
        if w.get_active():
                print ("ACTIVE")
        if w.get_inconsistent ():
                print ("INCONSISTENT")
        if w.get_active():
                if w.get_inconsistent ():
                        w.set_inconsistent (False)
                else:
                        w.set_inconsistent (True)
        if w.get_active():
                if w.get_inconsistent ():
                        for w in control_list:
                                w.hide ()
                else:
                        for w in control_list:
                                w.show ()
                for w in setup_list:
                        w.show ()
        else:
                for w in setup_list:
                        w.hide ()
                for w in control_list:
                        w.hide ()
        #win.resize(1, 1)

                        
def toggle_driftcompensation (w, dc):
        if w.get_active():
            dc.start ()
            print ("Drift Compensation ON")
        else:
            dc.stop ()
            print ("Drift Compensation OFF")
        
def toggle_gxsm_link (w, gl):
        global GXSM_Link_status
        if w.get_active():
                gl.start ()
                GXSM_Link_status = TRUE
                print ("GXSM Link enabled")
        else:
                gl.stop ()
                GXSM_Link_status = FALSE
                print ("GXSM Link disabled")
        
def toggle_gxsm_gain_link (w, gl):
        global GXSM_Link_status
        if w.get_active():
                gl.enable_gain_control ()
        else:
                gl.disable_gain_control ()
        
def toggle_Z0_invert (w):
        global Z0_invert_status
        if w.get_active():
                Z0_invert_status = TRUE
                print ("Z0 invert ACTIVE")
        else:
                Z0_invert_status = FALSE
                print ("Z0 normal")



        
def do_emergency(button,w,gl):
        print ("Emergency Stop Action:")
        global GXSM_Link_status
        if w.get_active():
                gl.stop ()
                GXSM_Link_status = FALSE
                print ("GXSM Link enabled")
                w.set_active(False)
        #goto_presets()

def on_gain_changed(combo, ii, self):
        tree_iter = combo.get_active_iter()
        if tree_iter is not None:
                model = combo.get_model()
                g = model[tree_iter][0]
                position = combo.get_active()
                print("Gain {}: {} [{}]".format(ii,g,position))
                hv1_adjust (None, position, ii)
        self.set_gains()

# besides print label, same code:                
def on_bw_changed(combo, ii):
        tree_iter = combo.get_active_iter()
        if tree_iter is not None:
                model = combo.get_model()
                g = model[tree_iter][0]
                position = combo.get_active()
                print("BW {}: {} [{}]".format(ii,g,position))
                #hv1_adjust (None, position, ii)


def get_status():
        global CHV5_monitor
        global gxsm_shm
        global CHV5_gains

        global rpspmc
        
        try:

                # XYZ MAX MIN (3x3)
                #memcpy  (shm_ptr, spmc_signals.xyz_meter, sizeof(spmc_signals.xyz_meter));
                #           0,1,2,  3,4,5,  6,7,8,
                
                # Monitors: Bias, reg, set,   GPVU,A,B,AM,FM, MUX, Signal (Current), AD463x[2], XYZ, XYZ0, XYZS
                #           10,    11,  12,    13, 14,15,16,17, 18, 19,               20, 21,    22,  23,   24
                #memcpy  (shm_ptr+sizeof(spmc_signals.xyz_meter), &spmc_parameters.bias_monitor, 21*sizeof(double));

                # PAC-PLL Monitors: dc-offset, exec_ampl_mon, dds_freq_mon, dds_dfre, volume_mon, phase_mon, control_dfreq_mon
                #memcpy  (shm_ptr+sizeof(spmc_signals.xyz_meter)+21*sizeof(double), &pacpll_parameters.dc_offset, 6*sizeof(double));
                #                   40,        41,            42,           43,        44,          45,        46
                
                gxsm_shares=np.ndarray((50), dtype=np.double, buffer=gxsm_shm.buf) # flat array all shares 
                xyz=np.ndarray((9,), dtype=np.double, buffer=gxsm_shm.buf).reshape((3,3)).T  # X Mi Ma, Y Mi Ma, Z Mi Ma
                CHV5_monitor['monitor']=xyz[0] * CHV5_gains
                CHV5_monitor['monitor_max']=xyz[1] * CHV5_gains
                CHV5_monitor['monitor_min']=xyz[2] * CHV5_gains

                #print ('CHV5 Gains: ', CHV5_gains, ' ... may toggle to enforce up-to-now.')
                #print (CHV5_monitor)
                
                rpspmc['bias']      = gxsm_shares[10]
                rpspmc['current']   = gxsm_shares[19]
                rpspmc['gvp']['u']  = gxsm_shares[13]
                rpspmc['pac']['ampl']   = gxsm_shares[44]
                rpspmc['pac']['dfreq']   = gxsm_shares[43]
                #print (rpspmc)
                
        except NameError:
                try:
                        gxsm_shm = shared_memory.SharedMemory(name='gxsm4rpspmc_monitors')
                        unregister(gxsm_shm._name, 'shared_memory')
                        print (gxsm_shm)
                        xyz=np.ndarray((9,), dtype=np.double, buffer=gxsm_shm.buf).reshape((3,3)).T  # X Mi Ma, Y Mi Ma, Z Mi Ma
                        print (xyz)
                except FileNotFoundError:
                        print ("SharedMemory(name='gxsm4rpspmc_monitors') not available. Please start gxsm4 and connect RPSPMC.")
        
        return 1

def do_exit(button):
        Gtk.main_quit()

def destroy(*args):
        Gtk.main_quit()

def locate_spm_control_dsp():
        print ("need to create a MK3 base support class.........")


class HV5App(Gtk.Application):
        def __init__(self, *args, **kwargs):
                super().__init__(*args, application_id="com.createc.hv5app", **kwargs)

                self.thv = THV5('http://192.168.40.10/')
                get_status()
                
                self.window = None


        def start_coarse(self, button, a,b,c, dir, es, ev, ep):
                axis = 0
                steps = 0
                volts = 0
                period = 500
                d=1
                if abs(dir) > 0:
                        axis = abs(dir)-1
                        if dir < 0:
                                d=-1
                        steps = int(es[axis].get_text())
                        volts = float(ev[axis].get_text())
                        period = float(ep[axis].get_text())
                print ('do coarse START', dir, steps, volts, period)
                self.thv.SetTHVCoarseMove(axis, d, steps, period, volts, True)
                
        def stop_coarse(self, button, a, dir, es, ev, ep):
                print ('do coarse STOP', dir)
                axis = 0
                if abs(dir) > 0:
                        axis = abs(dir)-1
                self.thv.SetTHVCoarseMove(axis, 1, 0, 0, 0, False)


        def set_gains(self):
                global CHV5_gains
                self.thv.SetTHVGain(CHV5_gains[0], CHV5_gains[1], CHV5_gains[2])

                
        def do_activate(self):
                global CHV5_configuration
                global CHV5_monitor
                global CHV5_gains

                if not self.window:
                        self.window = Gtk.ApplicationWindow(application=self, title="CHV5")
                        name = "Createc HV Control"

                        print ('do_activate: CHV5 Gains: ', CHV5_gains, ' ... may toggle to enforce up-to-now.')
                        
                        hb = Gtk.HeaderBar() 
                        #hb.set_show_close_button(True) 
                        #hb.props.title = "CHV5"
                        self.window.set_titlebar(hb) 
                        #hb.set_subtitle("Createc Piezo Drive") 
                        
                        grid = Gtk.Grid()
                        self.window.set_child (grid)

                        tr=1

                        maxvqp = 10
                        v = Gtk.Box(orientation=Gtk.Orientation.VERTICAL)
                        cqp = Instrument( Gtk.Label(), v, "VU", "QP", unit[3], widget_scale=METER_SCALE)
                        cqp.set_range(arange(0,maxvqp/10*11,maxvqp/10))
                        grid.attach(v, 0,tr, 1,1)

                        
                        maxv = 200
                        v = Gtk.Box(orientation=Gtk.Orientation.VERTICAL)
                        c1 = Instrument( Gtk.Label(), v, "Volt", "X-Axis", unit[0], widget_scale=METER_SCALE)
                        c1.set_range(arange(0,maxv/10*11,maxv/10))
                        grid.attach(v, 1,tr, 1,1)
                        
                        v = Gtk.Box(orientation=Gtk.Orientation.VERTICAL)
                        c2 = Instrument( Gtk.Label(), v, "Volt", "Y-Axis", unit[1], widget_scale=METER_SCALE)
                        c2.set_range(arange(0,maxv/10*11,maxv/10))
                        grid.attach(v, 2,tr, 1,1)
                        
                        v = Gtk.Box(orientation=Gtk.Orientation.VERTICAL)
                        c3 = Instrument( Gtk.Label(), v, "Volt", "Z-Axis", unit[2], widget_scale=METER_SCALE)
                        c3.set_range(arange(0,maxv/10*11,maxv/10))
                        grid.attach(v, 3,tr, 1,1)
                        tr=tr+1
                        
                        GLib.timeout_add (updaterate, update_CHV5_monitor, c1.set_reading_lohi, c2.set_reading_lohi, c3.set_reading_lohi, cqp.set_reading_auto_vu_extra)
                        separator = Gtk.Separator(orientation=Gtk.Orientation.HORIZONTAL)
                        grid.attach(separator, 0, tr, 5, 1)
                        tr=tr+1
                        
                        
                        # Offset Adjusts
                        tr = tr+1
                        lab = Gtk.Label(label="Offsets:")
                        control_list.append (lab)
                        grid.attach(lab, 0, tr, 1, 1)
                        
                        eo = []
                        
                        for i in range(0,3):
                                eo.append (Gtk.Entry())
                                eo[i].set_text("%12.3f" %(scaleO[i]*CHV5_configuration['target'][i]))
                                eo[i].set_width_chars(8)
                                control_list.append (eo[i])
                                grid.attach(eo[i], 1+i, tr, 1, 1)

                        oabutton = Gtk.Button() #stock='gtk-apply')
                        control_list.append (oabutton)
                        grid.attach(oabutton, 4, tr, 1, 1)
                        
                        # Drift Compensation Setup
                        tr = tr+1
                        lab = Gtk.Label(label="Drift Comp.:")
                        setup_list.append (lab)
                        grid.attach(lab, 0, tr, 1, 1)
                        
                        ed = []
                    
                        for i in range(0,3):
                                ed.append (Gtk.Entry())
                                ed[i].set_text("%12.3f " %(scaleO[i]*CHV5_driftcomp[i]) ) # + unit[i] + "/s")
                                ed[i].set_width_chars(8)
                                grid.attach(ed[i], 1+i, tr, 1, 1)
                                setup_list.append (ed[i])

                        lab = Gtk.Label(label=unit[0] + "/s")
                        setup_list.append (lab)
                        grid.attach(lab, 4, tr, 1, 1)

                        # GAINs
                        tr = tr+1
                        lab = Gtk.Label(label="Gains:")
                        control_list.append (lab)
                        grid.attach(lab, 0, tr, 1, 1)
                        
                        gain_store = Gtk.ListStore(str)
                        chan = ["gain_X", "gain_Y", "gain_Z"]
                        gain = [" 3x", " 6x", " 12x", " 24x"]
                        for g in gain:
                                gain_store.append([g])
                                gain_select = []
                        for ci in range(0,3):
                                opt = Gtk.ComboBox.new_with_model(gain_store)
                                opt.connect("changed", on_gain_changed, ci, self)
                                renderer_text = Gtk.CellRendererText()
                                opt.pack_start(renderer_text, True)
                                opt.add_attribute(renderer_text, "text", 0)
                                opt.set_active(CHV5_configuration['gain'][ci])
                                gain_select.append(opt.set_active)
                                #control_list.append (opt)
                                grid.attach(opt, 1+ci, tr, 1, 1)

                        ci=3

                        # BWs
                        tr = tr+1
                        lab = Gtk.Label(label="Bandwidth:")
                        setup_list.append (lab)
                        grid.attach(lab, 0, tr, 1, 1)
                        chan = ["bw_X", "bw_Y", "bw_Z"]
                        bw_store = Gtk.ListStore(str)
                        bw   = ["50 kHz", "10 kHz", "1 kHz"]
                        for b in bw:
                                bw_store.append([b])
                                
                        for ci in range(0,3):
                                opt = Gtk.ComboBox.new_with_model(bw_store)
                                opt.connect("changed", on_bw_changed, ci)
                                renderer_text = Gtk.CellRendererText()
                                opt.pack_start(renderer_text, True)
                                opt.add_attribute(renderer_text, "text", 0)
                                opt.set_active(CHV5_configuration['bw'][ci])
                                setup_list.append (opt)
                                grid.attach(opt, 1+ci, tr, 1, 1)

                        tr = tr+1
                        separator = Gtk.Separator(orientation=Gtk.Orientation.HORIZONTAL)
                        grid.attach(separator, 0, tr, 5, 1)
                        tr = tr+1
                        
                        # Link controls
                        
                        offset_control = offset_vector(eo, gain_select)
                        oabutton.connect("clicked", offset_control.write_t_vector)
                        dc_control = drift_compensation (ed, offset_control)
                        GxsmLink = gxsm_link(offset_control)
                        
                        # Closing ---
                        
                        hbox = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL)
                        #grid.attach(hbox, 0, tr, 5, 1)
                        hb.pack_end(hbox)
                        
                        cbl = check_button = Gtk.CheckButton(label="GXSM Link")
                        button = Gtk.Button.new_with_mnemonic(label='Stop')
                        button.connect("clicked", do_emergency, cbl, GxsmLink)
                        #Label=button.get_children()[0]
                        #Label=Label.get_children()[0].get_children()[1]
                        #Label=Label.set_label('STOP')
                        control_list.append (button)
                        grid.attach(button, 4, 1, 1, 1)
                        
                        cbc = check_button = Gtk.CheckButton(label="Config")
                        check_button.set_active(False)
                        check_button.connect('toggled', toggle_configure_widgets)
                        hbox.append(check_button)
                        
                        dc_check_button = Gtk.CheckButton(label="Drift Comp.")
                        dc_check_button.set_active(False)
                        dc_check_button.connect('toggled', toggle_driftcompensation, dc_control)
                        setup_list.append (dc_check_button)
                        hbox.append(dc_check_button)
                        
                        check_button=cbl
                        check_button.set_active(False)
                        check_button.connect('toggled', toggle_gxsm_link, GxsmLink)
                        hbox.append(check_button)
                        #setup_list.append (check_button)
                        check_button.set_sensitive (GxsmLink.status ())        
                        
                        check_button = Gtk.CheckButton(label="GXSM Gains")
                        check_button.set_active(False)
                        check_button.connect('toggled', toggle_gxsm_gain_link, GxsmLink)
                        hbox.append(check_button)
                        #setup_list.append (check_button)
                        check_button.set_sensitive (GxsmLink.status ())
                        
                        check_button = Gtk.CheckButton(label="Z0 inv.")
                        check_button.set_active(True)
                        check_button.connect('toggled', toggle_Z0_invert)
                        setup_list.append (check_button)
                        hbox.append(check_button)
                        check_button.set_sensitive (GxsmLink.status ())        
                        
                        toggle_configure_widgets(cbc)
                        
                        GLib.timeout_add(updaterate, get_status)        

                        #### Coarse Controller

                        grid_chv5c = Gtk.Grid()
                        grid.attach(grid_chv5c, 10, 0, 10, 5)

                        if SWAP_CXY:
                                DX=-2
                                DY=-1
                                axis = [1,0,2]
                        else:
                                DX=1
                                DY=2
                                axis = [0,1,2]

                        lxyz = ['X','Y','Z']
                                
                        es = [Gtk.Entry(), Gtk.Entry(), Gtk.Entry()] ## Entry Steps
                        ev = [Gtk.Entry(), Gtk.Entry(), Gtk.Entry()] ## Entry Volts
                        ep = [Gtk.Entry(), Gtk.Entry(), Gtk.Entry()] ## Entro Periods
                        grid_chv5c.attach(Gtk.Label(label='Ax'), 16, 3, 1, 1)
                        grid_chv5c.attach(Gtk.Label(label='# Steps'), 17, 3, 1, 1)
                        grid_chv5c.attach(Gtk.Label(label='Amplitude in V'), 18, 3, 1, 1)
                        grid_chv5c.attach(Gtk.Label(label='Period in µs'), 19, 3, 1, 1)
                        for r in range(0,3):
                                i = axis[r]
                                grid_chv5c.attach(Gtk.Label(label=lxyz[i]), 16, 5+r, 1, 1)
                                es[i].set_text("{}".format(CHV5_coarse['steps'][i]))
                                es[i].set_width_chars(8)
                                #control_list.append (eo[i])
                                grid_chv5c.attach(es[i], 17, 5+r, 1, 1)
                                ev[i].set_text("{}".format(CHV5_coarse['volts'][i]))
                                ev[i].set_width_chars(8)
                                #control_list.append (evo[i])
                                grid_chv5c.attach(ev[i], 18, 5+r, 1, 1)
                                ep[i].set_text("{}".format(CHV5_coarse['period'][i]))
                                ep[i].set_width_chars(8)
                                #control_list.append (ep[i])
                                grid_chv5c.attach(ep[i], 19, 5+r, 1, 1)

                        button = Gtk.Button.new_from_icon_name("media-playback-stop")
                        click_gesture = Gtk.GestureClick()
                        click_gesture.connect("pressed", self.start_coarse, 0, es, ev, ep)
                        click_gesture.connect("end",  self.stop_coarse, 0, es, ev, ep)
                        button.get_child().add_controller(click_gesture)
                        #control_list.append (button)
                        grid_chv5c.attach(button, 11, 6, 1, 1)

                        button = Gtk.Button.new_from_icon_name("arrow-left")
                        click_gesture = Gtk.GestureClick()
                                
                        click_gesture.connect("pressed",  self.start_coarse, -DX, es, ev, ep)
                        click_gesture.connect("end",  self.stop_coarse, -DX, es, ev, ep)
                        button.get_child().add_controller(click_gesture)
                        #control_list.append (button)
                        grid_chv5c.attach(button, 10, 6, 1, 1)

                        button = Gtk.Button.new_from_icon_name("arrow-right")
                        click_gesture = Gtk.GestureClick()
                        click_gesture.connect("pressed",  self.start_coarse, DX, es, ev, ep)
                        click_gesture.connect("end",  self.stop_coarse, DX, es, ev, ep)
                        button.get_child().add_controller(click_gesture)
                        #control_list.append (button)
                        grid_chv5c.attach(button, 12, 6, 1, 1)

                        button = Gtk.Button.new_from_icon_name("arrow-up")
                        click_gesture = Gtk.GestureClick()
                        click_gesture.connect("pressed",  self.start_coarse, DY, es, ev, ep)
                        click_gesture.connect("end",  self.stop_coarse, DY, es, ev, ep)
                        button.get_child().add_controller(click_gesture)
                        #control_list.append (button)
                        grid_chv5c.attach(button, 11, 5, 1, 1)

                        button = Gtk.Button.new_from_icon_name("arrow-down")
                        click_gesture = Gtk.GestureClick()
                        click_gesture.connect("pressed",  self.start_coarse, -DY, es, ev, ep)
                        click_gesture.connect("end",  self.stop_coarse, -DY, es, ev, ep)
                        button.get_child().add_controller(click_gesture)
                        #control_list.append (button)
                        grid_chv5c.attach(button, 11, 7, 1, 1)
                        

                        button = Gtk.Button.new_from_icon_name("arrow-up")
                        click_gesture = Gtk.GestureClick()
                        click_gesture.connect("pressed",  self.start_coarse, 3, es, ev, ep)
                        click_gesture.connect("end",  self.stop_coarse, 3, es, ev, ep)
                        button.get_child().add_controller(click_gesture)
                        #control_list.append (button)
                        grid_chv5c.attach(button, 15, 5, 1, 1)

                        button = Gtk.Button.new_from_icon_name("arrow-down")
                        click_gesture = Gtk.GestureClick()
                        click_gesture.connect("pressed",  self.start_coarse, -3, es, ev, ep)
                        click_gesture.connect("end",  self.stop_coarse, -3, es, ev, ep)
                        button.get_child().add_controller(click_gesture)
                        #control_list.append (button)
                        grid_chv5c.attach(button, 15, 7, 1, 1)

                        ###########

                        self.window.present()
                        
                        
                        
if __name__ == "__main__":
        app = HV5App()
        app.run()


#gxsm_shm.close()

)V0G0N";


const gchar *template_gvp_save_restore_script = R"V0G0N(
# Save/Load current GVP to/from JSON file
import json

filename = 'gvp_vector_program_xxx.json'
gvp_vectors = []

load = True

#gxsm.get('dsp-gvp-du00')

if load:
	with open(filename, 'r') as file:
        	gvp_vectors = json.load(file)
	for v in gvp_vectors:
		for key in v:
			print (key, v[key])
			gxsm.set(key, str(v[key]))

else:

	for eckey in gxsm.list_refnames ():
		if eckey.startswith('dsp-gvp'):
			print ('{} \t=>\t {}'.format(eckey, gxsm.get(eckey))) 
			gvp_vectors.append ({eckey: gxsm.get(eckey)})
			
	#print (gvp_vectors)

	with open(filename, 'w') as file:
		json.dump(gvp_vectors, file)

)V0G0N";

/*
const gchar *template_name = R"V0G0N(
...py script ...
)V0G0N";
*/
