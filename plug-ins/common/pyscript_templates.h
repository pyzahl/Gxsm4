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



/*
const gchar *template_name = R"V0G0N(
...py script ...
)V0G0N";
*/
