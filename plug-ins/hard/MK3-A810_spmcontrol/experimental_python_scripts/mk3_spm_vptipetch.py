#!/usr/bin/env python

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

version = "3.0.0"

import pygtk
pygtk.require('2.0')

import gobject, gtk
import time

import struct
import array

from numpy import *

import math
import re

import pydot
#import xdot
from scipy.optimize import leastsq

from mk3_spmcontol_class import *
from meterwidget import *
from scopewidget import *

# timeouts [ms]
timeout_update_patchrack              = 3000
timeout_update_patchmenu              = 3000
timeout_update_A810_readings          =  120
timeout_update_signal_monitor_menu    = 5000
timeout_update_signal_monitor_reading =  500
timeout_update_meter_readings         =   88
timeout_update_addplot_reading        =   88

timeout_min_recorder          =   80
timeout_min_tunescope         =   20
timeout_tunescope_default     =  150

timeout_DSP_status_reading        =  100
timeout_DSP_signal_lookup_reading = 2100


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
	if not parent.wins.has_key(name):
		win = gobject.new(gtk.Window,
				  type=gtk.WINDOW_TOPLEVEL,
				  title=label,
				  allow_grow=True,
				  allow_shrink=True,
				  border_width=0)
		parent.wins[name] = win
		v = gobject.new(gtk.HBox(spacing=0))

		scope = Oscilloscope( gobject.new(gtk.Label), v, "XT", label)
		scope.show()
		scope.set_chinfo([Xsignal[SIG_NAME], Ysignal[SIG_NAME]])
		win.add(v)
		if not self.ss:
			self.block_length = 512
			parent.mk3spm.set_recorder (self.block_length, self.trigger, self.trigger_level)

		table = gtk.Table(4, 2)
		table.set_row_spacings(5)
		table.set_col_spacings(5)
		v.pack_start (table)
		table.show()

		tr=0
		lab = gobject.new(gtk.Label, label="# Samples:")
		lab.show ()
		table.attach(lab, 2, 3, tr, tr+1)
		Samples = gtk.Entry()
		Samples.set_text("%d"%self.block_length)
		table.attach(Samples, 2, 3, tr+1, tr+2)
		Samples.show()

                [signal,data,offset] = parent.mk3spm.query_module_signal_input(DSP_SIGNAL_SCOPE_SIGNAL1_INPUT_ID)
                [lab1, menu1] = parent.make_signal_selector (DSP_SIGNAL_SCOPE_SIGNAL1_INPUT_ID, signal, "CH1: ", parent.global_vector_index)
#                lab = gobject.new(gtk.Label, label="CH1-scale:")
                lab1.show ()
		table.attach(lab1, 0, 1, tr, tr+1)
		Xscale = gtk.Entry()
		Xscale.set_text("1.")
		table.attach(Xscale, 0, 1, tr+1, tr+2)
		Xscale.show()

                [signal,data,offset] = parent.mk3spm.query_module_signal_input(DSP_SIGNAL_SCOPE_SIGNAL2_INPUT_ID)
                [lab2, menu1] = parent.make_signal_selector (DSP_SIGNAL_SCOPE_SIGNAL2_INPUT_ID, signal, "CH2: ", parent.global_vector_index)
#                lab = gobject.new(gtk.Label, label="CH2-scale:")
		lab2.show ()
		table.attach(lab2, 1, 2, tr, tr+1)
		Yscale = gtk.Entry()
		Yscale.set_text("1.")
		table.attach(Yscale, 1, 2, tr+1, tr+2)
		Yscale.show()

		tr = tr+2
		labx0 = gobject.new(gtk.Label, label="X-off:")
		labx0.show ()
		table.attach(labx0, 0, 1, tr, tr+1)
		Xoff = gtk.Entry()
		Xoff.set_text("0")
		table.attach(Xoff, 0, 1, tr+1, tr+2)
		Xoff.show()
		laby0 = gobject.new(gtk.Label, label="Y-off:")
		laby0.show ()
		table.attach(laby0, 1, 2, tr, tr+1)
		Yoff = gtk.Entry()
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
                                                print "CH1:"
                                                print xd
                                                print "CH2:"
                                                print yd
                                        else:
                                                save("mk3_recorder_xdata", xd)
                                                save("mk3_recorder_ydata", yd)

				# auto subsample if big
				nss = n
				nraw = n
				if n > 4096:
					ss = int(n/2048)
					end =  ss * int(len(xd)/ss)
					nss = (int)(n/ss)
					xd = mean(xd[:end].reshape(-1, ss), 1)
					yd = mean(yd[:end].reshape(-1, ss), 1)
					scope.set_info(["sub sampling: %d"%n + " by %d"%ss,
							"T = %g ms"%(n/150.)
							])
				else:
					scope.set_info([
							"T = %g ms"%(n/150.)
							])

				# change number samples?
				try:
					self.block_length = int(num())
					if self.block_length < 64:
						self.block_length = 64
					if self.block_length > 999999:
						print "MAX BLOCK LEN is 999999"
						self.block_length = 1024
				except ValueError:
					self.block_length = 512

				if self.block_length != n or self.ss:
					self.run = gtk.FALSE
					self.run_button.set_label("RESTART")

				if not self.ss:
					parent.mk3spm.set_recorder (self.block_length, self.trigger, self.trigger_level)
				#				v = value * signal[SIG_D2U]
				#				maxv = (1<<31)*math.fabs(signal[SIG_D2U])
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
						"CH1:": "%.2f"%xscale_div + " %s"%Xsignal[SIG_UNIT],
						"CH2:": "%.1f"%yscale_div + " %s"%Xsignal[SIG_UNIT],
						"Timebase:": "%g ms"%(nraw/150./20.) 
						})
				
				scope.set_data (xd, yd)

				if self.mode > 1:
					self.run_button.set_label("SINGLE")
			else:
				scope.set_info(["waiting for trigger or data [%d]"%blck])
				scope.queue_draw()


			return self.run

		def set_restart_callback (func):
			self.restart_func = func
			
		def stop_update_scope (win, event=None):
			print "STOP, hide."
			win.hide()
			self.run = gtk.FALSE
			return True

		def toggle_run_recorder (b):
			if self.run:
				self.run = gtk.FALSE
				self.run_button.set_label("RUN")
			else:
				self.restart_func ()
				self.run = gtk.TRUE
				self.run_button.set_label("STOP")

                                [Xsignal, Xdata, Offset] = parent.mk3spm.query_module_signal_input(DSP_SIGNAL_SCOPE_SIGNAL1_INPUT_ID)
                                [Ysignal, Ydata, Offset] = parent.mk3spm.query_module_signal_input(DSP_SIGNAL_SCOPE_SIGNAL2_INPUT_ID)
                                scope.set_chinfo([Xsignal[SIG_NAME], Ysignal[SIG_NAME]])

				period = int(2.*self.block_length/150.)
				if period < timeout_min_recorder:
					period = timeout_min_recorder
				gobject.timeout_add (period, update_scope, scope, Xsignal, Ysignal, Xscale.get_text, Yscale.get_text, Xoff.get_text, Yoff.get_text, Samples.get_text, labx0.set_text, laby0.set_text)
				if self.mode < 2: 
					self.run = gtk.TRUE
				else:
					self.run_button.set_label("WAITING")
					self.run = gtk.FALSE


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
			print self.trigger, self.trigger_level

		def toggle_mode (b):
			if self.mode == 0:
				self.mode = 1
				self.mode_button.set_label("M: Auto")
			else:
				if self.mode == 1:
					self.mode = 2
					self.mode_button.set_label("M: Normal")
				else:
					if self.mode == 2:
						self.mode = 3
						self.mode_button.set_label("M: Single")
					else:
						self.mode = 0
						self.mode_button.set_label("M: Free")

		self.run_button = gtk.Button("STOP")
#		self.run_button.set_use_stock(gtk.TRUE)
		self.run_button.connect("clicked", toggle_run_recorder)
		self.hb = gobject.new(gtk.HBox())
		self.hb.pack_start (self.run_button)
		self.mode_button = gtk.Button("M: Free")
		self.mode=0 # 0 free, 1 auto, 2 nommal, 3 single
		self.mode_button.connect("clicked", toggle_mode)
		self.hb.pack_start (self.mode_button)
		self.mode_button.show ()
		table.attach(self.hb, 2, 3, tr, tr+1)
		tr = tr+1

		self.trigger_button = gtk.Button("TRIGGER-OFF")
		self.trigger_button.connect("clicked", toggle_trigger)
		self.trigger_button.show ()
		table.attach(self.trigger_button, 2, 3, tr, tr+1)

		self.run = gtk.FALSE
		win.connect("delete_event", stop_update_scope)
		toggle_run_recorder (self.run_button)
		
	parent.wins[name].show_all()

    def nop (self):
        return 0
			

class TuneScope():

    def __init__(self, parent, fc=32766., span=40., Fres=0.2, int_ms = 150.):
	Xsignal = parent.mk3spm.lookup_signal_by_name("PLL Res Amp LP")
	Ysignal = parent.mk3spm.lookup_signal_by_name("PLL Res Ph LP")
	label = "Tune Scope -- X: " + Xsignal[SIG_NAME] + "  Y: " + Ysignal[SIG_NAME]
	name  = "Tune Scope"
	if not parent.wins.has_key(name):
		win = gobject.new(gtk.Window,
				  type=gtk.WINDOW_TOPLEVEL,
				  title=label,
				  allow_grow=True,
				  allow_shrink=True,
				  border_width=0)
		parent.wins[name] = win
		v = gobject.new(gtk.VBox(spacing=0))

		self.Fc       = fc
		self.Fspan    = span
		self.Fstep    = Fres
		self.interval = int_ms
		self.pos      = -10
		self.points   = int (2 * round (self.Fspan/2./self.Fstep) + 1)
		self.ResAmp   = ones (self.points)
		self.ResAmp2F = ones (self.points)
		self.ResPhase = zeros (self.points)
		self.ResPhase2F = zeros (self.points)
		self.Fit      = zeros (self.points)
		self.Freq     = zeros (self.points)
		self.volumeSine  = 0.5
                self.mode2f = 0
                self.phase_prev1 = 0
                self.phase_prev2 = 0
                
		def Frequency (position):
			return self.Fc - self.Fspan/2. + position * self.Fstep

		self.FSineHz  = Frequency (0.)

		scope = Oscilloscope( gobject.new(gtk.Label), v, "XT", label)
		scope.set_scale ( { Xsignal[SIG_UNIT]: "mV", Ysignal[SIG_UNIT]: "deg", "Freq." : "Hz" })
		scope.set_chinfo(["Res Ampl LP","Res Phase LP","Model","Res Ampl 2F","Res Phase 2F"])
		win.add(v)

		table = gtk.Table(4, 2)
		table.set_row_spacings(5)
		table.set_col_spacings(5)
		v.pack_start (table)

		tr=0
		lab = gobject.new(gtk.Label, label="Ampl scale: mV/div")
		table.attach(lab, 0, 1, tr, tr+1)
		self.Xscale = gtk.Entry()
		self.Xscale.set_text("0.5")
		table.attach(self.Xscale, 0, 1, tr+1, tr+2)
		lab = gobject.new(gtk.Label, label="Phase scale: deg/Div")
		table.attach(lab, 1, 2, tr, tr+1)

		self.Yscale = gtk.Entry()
		self.Yscale.set_text("20.")
		table.attach(self.Yscale, 1, 2, tr+1, tr+2)
		lab = gobject.new(gtk.Label, label="Fc [Hz]:")
		table.attach(lab, 2, 3, tr, tr+1)
		self.Fcenter = gtk.Entry()
		self.Fcenter.set_text("%g"%self.Fc)
		table.attach(self.Fcenter, 2, 3, tr+1, tr+2)

		lab = gobject.new(gtk.Label, label="Span [Hz]:")
		table.attach(lab, 3, 4, tr, tr+1)
		self.FreqSpan = gtk.Entry()
		self.FreqSpan.set_text("%g"%self.Fspan)
		table.attach(self.FreqSpan, 3, 4, tr+1, tr+2)


		tr = tr+2
		self.labx0 = gobject.new(gtk.Label, label="Amp off:")
		table.attach(self.labx0, 0, 1, tr, tr+1)
		self.Xoff = gtk.Entry()
		self.Xoff.set_text("0")
		table.attach(self.Xoff, 0, 1, tr+1, tr+2)

		self.laby0 = gobject.new(gtk.Label, label="Phase off:")
		table.attach(self.laby0, 1, 2, tr, tr+1)
		self.Yoff = gtk.Entry()
		self.Yoff.set_text("0") ## 180
		table.attach(self.Yoff, 1, 2, tr+1, tr+2)

		lab = gobject.new(gtk.Label, label="Volume Sine [V]:")
		table.attach(lab, 2, 3, tr, tr+1)
		self.Vs = gtk.Entry()
		self.Vs.set_text("%g"%self.volumeSine)
		table.attach(self.Vs, 2, 3, tr+1, tr+2)

		lab = gobject.new(gtk.Label, label="Interval [ms]:")
		table.attach(lab, 3, 4, tr, tr+1)
		self.Il = gtk.Entry()
		self.Il.set_text("%d"%self.interval)
		table.attach(self.Il, 3, 4, tr+1, tr+2)

		self.M2F = gtk.CheckButton ("Mode 1F+2F")
		table.attach(self.M2F, 0, 1, tr+2, tr+3)

		lab = gobject.new(gtk.Label, label="Freq. Res. [Hz]:")
		table.attach(lab, 1, 2, tr+2, tr+3)
		self.FreqRes = gtk.Entry()
		self.FreqRes.set_text("%g"%self.Fstep)
		table.attach(self.FreqRes, 2, 3, tr+2, tr+3)

		self.xdc = 0.
		self.ydc = 0.

		self.fitresults = { "A":0., "f0": 32768., "Q": 0. }
		self.fitinfo = ["", "", ""]
		self.resmodel = ""

		def run_fit(iA0, fi0, iQ):
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

#			self.Fit = initialplot
			
			# uncertainty calculation
#			v0=10.0
#			uvr=sqrt(dvr*dvr+vr*vr*0.0025)/v0
			v0=1.
			uvr=sqrt(dvr*dvr+vr*vr*0.0025)/v0

			# do fit using Levenberg-Marquardt
#			p2,cov,info,mesg,success=fit(resonance, p, freq, vr/v0, uvr)
			p2,cov,info,mesg,success=fit(resonance, p, freq, vr, None)

			if success==1:
				print "Converged"
			else:
				self.fitinfo[0] = "Fit not converged."
				print "Not converged"
				print mesg

			# calculate final chi square
			chisq=sum(info["fvec"]*info["fvec"])

			dof=len(freq)-len(p)
			# chisq, sqrt(chisq/dof) agrees with gnuplot
			print "Converged with chi squared ",chisq
			print "degrees of freedom, dof ", dof
			print "RMS of residuals (i.e. sqrt(chisq/dof)) ", sqrt(chisq/dof)
			print "Reduced chisq (i.e. variance of residuals) ", chisq/dof
			print

			# uncertainties are calculated as per gnuplot, "fixing" the result
			# for non unit values of the reduced chisq.
			# values at min match gnuplot
			print "Fitted parameters at minimum, with 68% C.I.:"
			for i,pmin in enumerate(p2):
				self.fitinfo[i]  = "%-10s %12f +/- %10f"%(p[i].name,pmin,sqrt(cov[i,i])*sqrt(chisq/dof))
				self.fitresults[p[i].name] = pmin
				print self.fitinfo[i]
			print

			print "Correlation matrix"
			# correlation matrix close to gnuplot
			print "               ",
			for i in range(len(p)): print "%-10s"%(p[i].name,),
			print
			for i in range(len(p2)):
			    print "%10s"%p[i].name,
			    for j in range(i+1):
				print "%10f"%(cov[i,j]/sqrt(cov[i,i]*cov[j,j]),),
			    print

			finalplot=resonance(freqs)
			self.Fit = finalplot
#			print "----------------- Final:"
#			print self.Fit

                        save("mk3_tune_Freq", self.Freq)
                        save("mk3_tune_Fit", self.Fit)
                        save("mk3_tune_ResAmp", self.ResAmp)
                        save("mk3_tune_ResPhase", self.ResPhase)
                        save("mk3_tune_ResPhase2F", self.ResPhase2F)
                        save("mk3_tune_ResAmp2F", self.ResAmp2F)


		self.peakvalue = 0
		self.peakindex = 0

		def update_tune(set_data, Xsignal, Ysignal):
			Filter64Out = parent.mk3spm.read_PLL_Filter64Out ()
			cur_a = Filter64Out[iii_PLL_F64_ResAmpLP] * Xsignal[SIG_D2U]
			cur_ph = Filter64Out[iii_PLL_F64_ResPhaseLP] * Ysignal[SIG_D2U]
			print self.pos, Filter64Out, cur_a, cur_ph, self.points

			# full phase unwrap
                        if self.mode2f == 0:
                                pre_ph = self.phase_prev1
                        else:
                                pre_ph = self.phase_prev2
                        
                        # P_UnWrapped(i)=P(i)-Floor(((P(i)-P(i-1))/2Pi)+0.5)*2Pi

			if cur_ph - pre_ph > 180.:
                                cur_ph = cur_ph - 360.
			if cur_ph - pre_ph < -180.:
                                cur_ph = cur_ph + 360.

			if self.pos >= 0 and self.pos < self.points:
				self.Freq[self.pos]     = self.FSineHz
                                if self.mode2f == 0:
                                        self.ResAmp[self.pos]   = cur_a
                                        self.ResPhase[self.pos] = cur_ph
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
				self.run = gtk.FALSE
				self.run_button.set_label("RESTART")
				run_fit (1000./self.peakvalue, Frequency (self.peakindex), 20000.)
				parent.mk3spm.adjust_PLL_sine (self.volumeSine, self.fitresults["f0"], self.mode2f)
                                self.mode2f = 0
				self.pos = -10
			else:
				self.Fstep   = self.Fspan/(self.points-1)
				self.FSineHz = Frequency (self.pos)
				parent.mk3spm.adjust_PLL_sine (self.volumeSine, self.FSineHz, self.mode2f)

			if self.peakindex > 0 and self.peakindex < self.points:
				scope.set_info(["tuning at %.3f Hz"%self.FSineHz + " [%d]"%self.pos,
						"cur peak at: %.3f Hz"%Frequency (self.peakindex),
						"Res Amp: %g mV"%cur_a + " [ %g mV]"%self.peakvalue,
						"Phase: %3.1f deg"%cur_ph + " [ %3.1f deg]"%self.ResPhase[self.peakindex],
						"Vol. Sine: %.3f V"%self.volumeSine,"","","","","",
						self.resmodel,
						self.fitinfo[0],self.fitinfo[1],self.fitinfo[2]
						])
			else:
				scope.set_info(["tuning at %.3f Hz"%self.FSineHz + " [%d]"%self.pos,
						"cur peak at: --",
						"Res Amp: %g mV"%cur_a,
						"Phase: %3.1f deg"%cur_ph,
						"Vol. Sine: %.3f V"%self.volumeSine,"","","","","",
						self.resmodel,
						self.fitinfo[0],self.fitinfo[1],self.fitinfo[2]
						])

			n = self.pos

			try:
				self.Fc = float(self.Fcenter.get_text())
				if self.Fc < 1000.:
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

			try:
				xscale_div = float(self.Xscale.get_text())
			except ValueError:
				xscale_div = 1
				
			try:
				yscale_div = float(self.Yscale.get_text())
			except ValueError:
				yscale_div = 1

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
					"CH1: (Ampl.)":"%g mV"%xscale_div, 
					"CH2: (Phase)":"%g deg"%yscale_div, 
					"Freq.: ": "%g Hz/div"%(self.Fspan/20.)
					})
			x = 20.*self.pos/self.points - 10 + 0.1
			y = -(cur_a- self.xoff) / xscale_div + 0.25
#			scope.set_data (xd, yd, fd, array([[x,y],[x, y-0.5]]))
                        scope.set_data_with_uv (xd, yd, fd, ud, vd, array([[x,y],[x, y-0.5]]))


			return self.run

		def stop_update_tune (win, event=None):
			print "STOP, hide."
			win.hide()
			self.run = gtk.FALSE
			return True

		def toggle_run_tune (b):
			if self.run:
				self.run = gtk.FALSE
				self.run_button.set_label("RUN TUNE")
				run_fit (1000./self.peakvalue, Frequency (self.peakindex), 20000.)
			else:
				self.run = gtk.TRUE
				self.run_button.set_label("STOP TUNE")
				try:
					tmp = float(self.FreqSpan.get_text())
					if tmp != self.Fspan:
						if tmp > 1. and tmp < 75000:
							self.Fspan = tmp
				except ValueError:
					print "invalid entry"
				try:
					tmp = float(self.FreqRes.get_text())
					if tmp != self.Fstep:
						if tmp > 0.0001 and tmp < 100.:
							self.Fstep=tmp
							self.points   = int (2 * round (self.Fspan/2./self.Fstep) + 1)
							self.ResAmp   = ones (self.points)
							self.ResPhase2F = zeros (self.points)
							self.ResAmp2F   = ones (self.points)
							self.ResPhase = zeros (self.points)
							self.Fit      = zeros (self.points)
							self.Freq     = zeros (self.points)
                                                        self.mode2f   = 0
							self.pos      = -10
						else:
							print "invalid Fstep entry"
				except ValueError:
					print "invalid Fstep entry"

				self.interval = self.Il.get_text()
				self.fitinfo = ["","",""]
				self.resmodel = ""
				if self.interval < timeout_min_tunescope or self.interval > 10000:
					self.interval = timeout_tunescope_default
				gobject.timeout_add (int(self.interval), update_tune, scope, Xsignal, Ysignal)


		self.run_button = gtk.Button("STOP TUNE")
		self.run_button.connect("clicked", toggle_run_tune)
		table.attach(self.run_button, 3, 4, tr+2, tr+3)

		self.run = gtk.FALSE
		win.connect("delete_event", stop_update_tune)
		toggle_run_tune (self.run_button)
		
	parent.wins[name].show_all()


################################################################################

class SignalPlotter():

    def __init__(self, parent, length = 300., taps=[4,1,3,5,2]):
        self.monitor_taps = taps
        [value, uv1, Xsignal] = parent.mk3spm.get_monitor_signal (self.monitor_taps[1])
        [value, uv2, Ysignal] = parent.mk3spm.get_monitor_signal (self.monitor_taps[2])
        [value, uv3, Usignal] = parent.mk3spm.get_monitor_signal (self.monitor_taps[3])
        [value, uv4, Vsignal] = parent.mk3spm.get_monitor_signal (self.monitor_taps[4])

	label = "Signal Plotter -- DSP Etch Control Watch"
	name  = "Signal Plotter"

	if not parent.wins.has_key(name):
		win = gobject.new(gtk.Window,
				  type=gtk.WINDOW_TOPLEVEL,
				  title=label,
				  allow_grow=True,
				  allow_shrink=True,
				  border_width=0)
		parent.wins[name] = win
		v = gobject.new(gtk.HBox(spacing=0))

		self.pos      = 0
                self.span     = length
		self.points   = 2000
		self.dt       = self.span/self.points
		self.Time = zeros (self.points)
		self.Tap1 = zeros (self.points)
		self.Tap2 = zeros (self.points)
		self.Tap3 = zeros (self.points)
		self.Tap4 = zeros (self.points)

                self.t0 = parent.mk3spm.get_monitor_data (self.monitor_taps[0])

		scope = Oscilloscope( gobject.new(gtk.Label), v, "XT", label)
		scope.set_scale ( { Xsignal[SIG_UNIT]: "mV", Ysignal[SIG_UNIT]: "deg", "time" : "s" })
		scope.set_chinfo(["MON1","MON2","MON3","MON4"])
		win.add(v)

		table = gtk.Table(4, 2)
		table.set_row_spacings(5)
		table.set_col_spacings(5)
		v.pack_start (table)

		tr=0
		lab = gobject.new(gtk.Label, label="CH1 scale: V/div")
		table.attach(lab, 0, 1, tr, tr+1)
		self.M1scale = gtk.Entry()
		self.M1scale.set_text("0.5")
		table.attach(self.M1scale, 0, 1, tr+1, tr+2)

		lab = gobject.new(gtk.Label, label="CH2 scale: V/Div")
		table.attach(lab, 1, 2, tr, tr+1)
		self.M2scale = gtk.Entry()
		self.M2scale.set_text("0.5")
		table.attach(self.M2scale, 1, 2, tr+1, tr+2)

		lab = gobject.new(gtk.Label, label="Span [s]:")
		table.attach(lab, 3, 4, tr, tr+1)
		self.Span = gtk.Entry()
		self.Span.set_text("%g"%self.span)
		table.attach(self.Span, 3, 4, tr+1, tr+2)


		tr = tr+2
		self.labx0 = gobject.new(gtk.Label, label="CH1 off:")
		table.attach(self.labx0, 0, 1, tr, tr+1)
		self.M1off = gtk.Entry()
		self.M1off.set_text("0")
		table.attach(self.M1off, 0, 1, tr+1, tr+2)

		self.laby0 = gobject.new(gtk.Label, label="CH2 off:")
		table.attach(self.laby0, 1, 2, tr, tr+1)
		self.M2off = gtk.Entry()
		self.M2off.set_text("0")
		table.attach(self.M2off, 1, 2, tr+1, tr+2)

		lab = gobject.new(gtk.Label, label="Interval [s]:")
		table.attach(lab, 3, 4, tr, tr+1)
		self.Il = gtk.Entry()
		self.Il.set_text("%d"%self.span)
		table.attach(self.Il, 3, 4, tr+1, tr+2)

#		self.M2F = gtk.CheckButton ("Mode 1F+2F")
#		table.attach(self.M2F, 0, 1, tr+2, tr+3)

		self.xdc = 0.
		self.ydc = 0.

		def update_plotter():

                        if self.pos >= self.points:
                                #self.run = gtk.FALSE
				#self.run_button.set_label("RESTART")
				self.pos = 0
                                                          
                                save ("plotter_t-"+str(self.t0), self.Time)
                                save ("plotter_t1-"+str(self.t0), self.Tap1)
                                save ("plotter_t2-"+str(self.t0), self.Tap2)
                                save ("plotter_t3-"+str(self.t0), self.Tap3)
                                save ("plotter_t4-"+str(self.t0), self.Tap4)
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
                            print "plotter data signals:"
                            print signal1, signal2, signal3
                            scope.set_chinfo([signal1[SIG_NAME],signal2[SIG_NAME],signal3[SIG_NAME],signal4[SIG_NAME]])

                        print n, t, uv1, uv2, uv3, uv4
                        
			try:
				m1scale_div = float(self.M1scale.get_text())
			except ValueError:
				m1scale_div = 1
				
			try:
				m2scale_div = float(self.M2scale.get_text())
			except ValueError:
				m2scale_div = 1

			try:
				m1off = float(self.M1off.get_text())
			except ValueError:
				m1off = 0
				
			try:
				m2off = float(self.M2off.get_text())
			except ValueError:
				m2off = 0
                                
                        scope.set_scale ( { 
					"XY1: (Tap1)":"%g mV"%m1scale_div, 
					"XY2: (Tap2)":"%g mV"%m2scale_div, 
					"time: ": "%g s"%(self.span/20.)
					})

                        ### def set_data (self, Xd, Yd, Zd=zeros(0), XYd=[zeros(0), zeros(0)]):

                        #  if t > self.span:
                        #    td = 20. * (self.Time-t+self.span)/self.span
                        # else:
                        td = -10. + 20. * self.Time/self.span
                        t1 = -(self.Tap1 - m1off) / m1scale_div
                        t2 = -(self.Tap2 - m2off) / m2scale_div 
                        t3 = -(self.Tap3 - m2off) / m2scale_div 
                        t4 = -(self.Tap4 - m2off) / m2scale_div 
                        #scope.set_data (zeros(0), zeros(0), zeros(0), XYd=[td, t1])
                        #scope.set_data (t1, t2, zeros(0), XYd=[td, t1])
                        scope.set_data_with_uv (t1, t2, t3, t4)

			return self.run

		def stop_update_plotter (win, event=None):
			print "STOP, hide."
			win.hide()
			self.run = gtk.FALSE
                        save ("plotter_t-"+str(self.t0), self.Time)
                        save ("plotter_t1-"+str(self.t0), self.Tap1)
                        save ("plotter_t2-"+str(self.t0), self.Tap2)
                        save ("plotter_t3-"+str(self.t0), self.Tap3)
                        save ("plotter_t4-"+str(self.t0), self.Tap4)
			return True

		def toggle_run_plotter (b):
			if self.run:
				self.run = gtk.FALSE
				self.run_button.set_label("RUN PLOTTER")
                                save ("plotter_t-"+str(self.t0), self.Time)
                                save ("plotter_t1-"+str(self.t0), self.Tap1)
                                save ("plotter_t2-"+str(self.t0), self.Tap2)
                                save ("plotter_t3-"+str(self.t0), self.Tap3)
                                save ("plotter_t4-"+str(self.t0), self.Tap4)
			else:
				self.run = gtk.TRUE
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
				gobject.timeout_add (int (self.dt*1000.), update_plotter)


		self.run_button = gtk.Button("STOP TUNE")
		self.run_button.connect("clicked", toggle_run_plotter)
		table.attach(self.run_button, 3, 4, tr+2, tr+3)

		self.run = gtk.FALSE
		win.connect("delete_event", stop_update_plotter)
		toggle_run_plotter (self.run_button)
		
	parent.wins[name].show_all()




        
################################################################################
# CONFIGURATOR MAIN MENU/WINDOW
################################################################################

class Mk3_Tool_Menu:
    def __init__(self):
	self.mk3spm = SPMcontrol ()
	self.mk3spm.read_configurations ()
	self.vector_index = 0
	self.dsp_address = 0x10f05428+8    #### 10f05428   _probe . LIM_up

        
	buttons = {
		   "1 Signal Monitor": self.create_signal_monitor_app,
		   "2 SPM Signal Oscilloscope": self.create_oscilloscope_app,
		   "3 PLL: Osc. Tune App": self.create_PLL_tune_app,
		   "4 Start VP Etch App": self.create_VP_etch_app,
		   "5 Signal PLotter App": self.create_signal_plotter_app,
		 }
	win = gobject.new(gtk.Window,
		     type=gtk.WINDOW_TOPLEVEL,
		     title='SR MK3-Pro SPM Configurator '+version,
		     allow_grow=True,
		     allow_shrink=True,
		     border_width=5)
	win.set_name("main window")
	win.set_size_request(260, 440)
	win.connect("destroy", self.destroy)
	win.connect("delete_event", self.destroy)

	box1 = gobject.new(gtk.VBox(False, 0))
	win.add(box1)
	scrolled_window = gobject.new(gtk.ScrolledWindow())
	scrolled_window.set_border_width(5)
	scrolled_window.set_policy(gtk.POLICY_AUTOMATIC, gtk.POLICY_AUTOMATIC)
	box1.pack_start(scrolled_window)
	box2 = gobject.new(gtk.VBox())
	box2.set_border_width(5)
	scrolled_window.add_with_viewport(box2)

	lab = gtk.Label("SR dev:" + self.mk3spm.sr_path ())
	box2.pack_start(lab)
	lab = gtk.Label(self.mk3spm.get_spm_code_version_info())
	box2.pack_start(lab)

        self.ie = gtk.Entry()
        self.ie.set_text("0")
	box2.pack_start(self.ie)
        self.ie.show()
        
	k = buttons.keys()
	k.sort()
	for i in k:
		button = gobject.new (gtk.Button, label=i)

		if buttons[i]:
			button.connect ("clicked", buttons[i])
		else:
			button.set_sensitive (False)
		box2.pack_start (button)

	separator = gobject.new (gtk.HSeparator())
	box1.pack_start (separator, expand=False)

	box2 = gobject.new (gtk.VBox(spacing=10))
	box2.set_border_width (5)
	box1.pack_start (box2, expand=False)
	button = gtk.Button (stock='gtk-close')
	button.connect ("clicked", self.do_exit)
	button.set_flags (gtk.CAN_DEFAULT)
	box2.pack_start (button)
	button.grab_default ()
	win.show_all ()

	self.wins = {}


    def delete_event(self, win, event=None):
	    win.hide()
	    # don't destroy window -- just leave it hidden
	    return True

    def do_exit (self, button):
	    gtk.main_quit()

    def destroy (self, *args):
	    gtk.main_quit()
	
    # update DSP setting parameter
    def int_param_update(self, _adj):
	    param_address = _adj.get_data("int_param_address")
	    param = int (_adj.get_data("int_param_multiplier")*_adj.value)
	    self.mk3spm.write_parameter (param_address, struct.pack ("<l", param), 1) 

    def make_menu_item(self, name, callback, value, identifier, func=lambda:0):
	    item = gtk.MenuItem(name)
	    item.connect("activate", callback, value, identifier, func)
	    item.show()
	    return item

    def toggle_flag(self, w, mask):
	    self.mk3spm.adjust_statemachine_flag(mask, w.get_active())


    ##### configurator tools
	    
    def create_oscilloscope_app (self, _button):
        kao = SignalScope (self)
	
    def create_PLL_tune_app (self, _button):
        tune = TuneScope (self)

    def create_signal_plotter_app (self, _button):
        plotter = SignalPlotter (self)

    # Vector index or signal variable offset from signal head pointer
    def global_vector_index (self):
        return int (self.ie.get_text())
    #        return self.vector_index

    def global_vector_index_null (self):
        return 0
    def global_vector_index_one (self):
        return 1
    def global_vector_index_two (self):
        return 2

    # Sec 2,4,7 : VP_SEC_END_IN1 *8 --- +0..3: probe.src_input[0..3], +4,5: .counter[0,1], +6,7: src_input[0,1]_integrated
    def global_vector_index_sec_pos (self):
	    return 3*8+0
    def global_vector_index_sec_pos_int (self):
	    return 3*8+6
    def global_vector_index_sec_off (self):
	    return 5*8+0
    def global_vector_index_sec_off_int (self):
	    return 5*8+6
    def global_vector_index_sec_neg (self):
	    return 8*8+0
    def global_vector_index_sec_neg_int (self):
	    return 8*8+6

    def global_dsp_address (self):
	    return self.dsp_address


    def make_signal_selector (self, _input_id, sig, prefix, flag_nullptr_ok=0):
		opt = gtk.OptionMenu()
		menu = gtk.Menu()
		i = 0
		i_actual_signal = -1 # for anything not OK
		item = self.make_menu_item(prefix+"DSP VAR ADDRESS", self.mk3spm.change_signal_monitor_debug_pointer, sig, _input_id, self.global_dsp_address)
		menu.append(item)
		SIG_LIST = self.mk3spm.get_signal_lookup_list()
		for signal in SIG_LIST:
			if signal[SIG_ADR] > 0:
				item = self.make_menu_item(prefix+"S%02d:"%signal[SIG_INDEX]+signal[SIG_NAME], self.mk3spm.change_signal_input, signal, _input_id, self.global_vector_index)
				menu.append(item)
				if signal[SIG_ADR] == sig[SIG_ADR]:
					i_actual_signal=i
				i=i+1
		if flag_nullptr_ok:
			item = self.make_menu_item("DISABLED", self.mk3spm.disable_signal_input, [0,0,"DISABLED","DISABLED","0",0], _input_id)
			menu.append(item)

		if i_actual_signal == -1:
			i_actual_signal = i

		menu.set_active(i_actual_signal+1)
		opt.set_menu(menu)
		opt.show()
		return [opt, menu]

    def update_signal_menu_from_signal (self, menu, tap):
	    [v, uv, signal] = self.mk3spm.get_monitor_signal (tap)
	    if signal[SIG_INDEX] <= 0:
		    menu.set_active(0)
		    return 1
	    menu.set_active(signal[SIG_INDEX]+1)
	    return 1

    # LOG "VU" Meter
    def create_meterVU(self, w, tap, fsk):
	    [value, uv, signal] = self.mk3spm.get_monitor_signal (tap)
	    label = signal[SIG_NAME]
	    unit  = signal[SIG_UNIT]
	    name="meterVU-"+label
	    if not self.wins.has_key(name):
		    win = gobject.new(gtk.Window,
				      type=gtk.WINDOW_TOPLEVEL,
				      title=label,
				      allow_grow=True,
				      allow_shrink=True,
				      border_width=0)
		    self.wins[name] = win
		    win.connect("delete_event", self.delete_event)
		    v = gobject.new(gtk.VBox(spacing=0))
		    instr = Instrument( gobject.new(gtk.Label), v, "VU", label)
		    instr.show()
		    win.add(v)

		    def update_meter(inst, _tap, signal, fm):
#			    [value, v, signal] = self.mk3spm.get_monitor_signal (_tap)
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

			    return gtk.TRUE

		    gobject.timeout_add (timeout_update_meter_readings, update_meter, instr, tap, signal, fsk)
	    self.wins[name].show_all()

    # Linear Meter
    def create_meterLinear(self, w, tap, fsk):
	    [value, uv, signal] = self.mk3spm.get_monitor_signal (tap)
	    label = signal[SIG_NAME]
	    unit  = signal[SIG_UNIT]
	    name="meterLinear-"+label
	    if not self.wins.has_key(name):
		    win = gobject.new(gtk.Window,
				      type=gtk.WINDOW_TOPLEVEL,
				      title=label,
				      allow_grow=True,
				      allow_shrink=True,
				      border_width=0)
		    self.wins[name] = win
		    win.connect("delete_event", self.delete_event)
		    v = gobject.new(gtk.VBox(spacing=0))
		    try:
			    float (fsk ())
			    maxv = float (fsk ())
		    except ValueError:
			    maxv = (1<<31)*math.fabs(signal[SIG_D2U])

		    instr = Instrument( gobject.new(gtk.Label), v, "Volt", label, unit)
		    instr.set_range(arange(0,maxv/10*11,maxv/10))
		    instr.show()
		    win.add(v)

		    def update_meter(inst, _tap, signal, fm):
#			    [value, v, signal] = self.mk3spm.get_monitor_signal (_tap)
			    value = self.mk3spm.get_monitor_data (_tap)
			    v = value * signal[SIG_D2U]
# DEBUG VERBOSE PRINT METER VALUE
                            time_tics = self.mk3spm.get_monitor_data (4)
                            print  "%10.3f s, " %(time_tics/150000.) + "CH%d, " %_tap + " %8.5f V" %v
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
			    return gtk.TRUE

		    gobject.timeout_add (timeout_update_meter_readings, update_meter, instr, tap, signal, fsk)
	    self.wins[name].show_all()


    # Plotter
    def create_plotter(self, w, tap, fsk):
	    [value, uv, signal] = self.mk3spm.get_monitor_signal (tap)

            try:
                float (fsk ())
                maxv = float (fsk ())
            except ValueError:
                maxv = (1<<31)*math.fabs(signal[SIG_D2U])

            class Plotter(object):
                def __init__(self, parent, ax, maxv, maxt=60):
                    self.parent = parent
                    self.ax = ax
                    self.maxt = maxt
                    self.tdata = [0]
                    self.ydata = [0]
                    self.line = Line2D(self.tdata, self.ydata)
                    self.ax.add_line(self.line)
                    self.ax.set_ylim(-maxv, maxv)
                    self.ax.set_xlim(0, self.maxt)
                    self.t0 = parent.mk3spm.get_monitor_data (4)
                    
                def update(self, y):
                    lastt = self.tdata[-1]
                    if lastt > self.tdata[0] + self.maxt:  # reset the arrays
                        self.tdata = [self.tdata[-1]]
                        self.ydata = [self.ydata[-1]]
                        self.ax.set_xlim(self.tdata[0], self.tdata[0] + self.maxt)
                        self.ax.figure.canvas.draw()

                    t = self.parent.mk3spm.get_monitor_data (4) - self.t0
                    self.tdata.append(t)
                    self.ydata.append(y)
                    self.line.set_data(self.tdata, self.ydata)
                    return self.line,

            def emitter():
                yield self.mk3spm.get_monitor_data (tap) * signal[SIG_D2U]
                
            #tmp = 'Plotter: '+signal[SIG_NAME]+' Mon['+str(tap)+']'
            #tmp = signal[SIG_NAME] + '['+ signal[SIG_UNIT]+']'
            
            plotter = Plotter(self, ax, maxv, 100)


    def update_signal_menu_from_signal (self, menu, tap):
	    [v, uv, signal] = self.mk3spm.get_monitor_signal(tap)
	    if signal[SIG_INDEX] <= 0:
		    menu.set_active(0)
		    return 1
	    menu.set_active(signal[SIG_INDEX]+1)
	    return 1

    def create_signal_monitor_app(self, _button):
		name="SPM Signal Monitor"
		if not self.wins.has_key(name):
			win = gobject.new(gtk.Window,
					  type=gtk.WINDOW_TOPLEVEL,
					  title=name,
					  allow_grow=True,
					  allow_shrink=True,
					  border_width=2)
			self.wins[name] = win
			win.connect("delete_event", self.delete_event)

			box1 = gobject.new(gtk.VBox())
			win.add(box1)

			box2 = gobject.new(gtk.VBox(spacing=0))
			box2.set_border_width(5)
			box1.pack_start(box2)

			table = gtk.Table (12, 37)
			table.set_row_spacings(0)
			table.set_col_spacings(4)
			box2.pack_start(table, False, False, 0)

			r=0
			lab = gobject.new(gtk.Label, label="Signal to Monitor")
			table.attach (lab, 0, 1, r, r+1)

			lab = gobject.new(gtk.Label, label="DSP Signal Variable name")
			table.attach (lab, 1, 2, r, r+1)

			lab = gobject.new(gtk.Label, label="DSP Sig Reading")
			lab.set_alignment(1.0, 0.5)
			table.attach (lab, 2, 3, r, r+1, gtk.FILL | gtk.EXPAND )

			lab = gobject.new(gtk.Label, label="Value")
			lab.set_alignment(1.0, 0.5)
			table.attach (lab, 6, 7, r, r+1, gtk.FILL | gtk.EXPAND )

			lab = gobject.new(gtk.Label, label="dB")
			lab.set_alignment(1.0, 0.5)
			table.attach (lab, 7, 8, r, r+1, gtk.FILL | gtk.EXPAND )
			lab = gobject.new(gtk.Label, label="V")
			lab.set_alignment(1.0, 0.5)
			table.attach (lab, 8, 9, r, r+1, gtk.FILL | gtk.EXPAND )

			lab = gobject.new(gtk.Label, label="Galvo Range")
			lab.set_alignment(1.0, 0.5)
			table.attach (lab, 9, 10, r, r+1, gtk.FILL | gtk.EXPAND )

			r = r+1
			separator = gobject.new(gtk.HSeparator())
			separator.show()
			table.attach (separator, 0, 7, r, r+1, gtk.FILL | gtk.EXPAND )

			r = r+1
			# create full signal monitor
			for tap in range(0,num_monitor_signals):
				[value, uv, signal] = self.mk3spm.get_monitor_signal (tap)
#				if signal[SIG_INDEX] < 0:
#					break
				[lab1, menu1] = self.make_signal_selector (DSP_SIGNAL_MONITOR_INPUT_BASE_ID+tap, signal, "Mon[%02d]: "%tap)
				lab1.set_alignment(-1.0, 0.5)
				table.attach (lab1, 0, 1, r, r+1)

				gobject.timeout_add (timeout_update_signal_monitor_menu, self.update_signal_menu_from_signal, menu1, tap)

				lab2 = gobject.new(gtk.Label, label=signal[SIG_VAR])
				lab2.set_alignment(-1.0, 0.5)
				table.attach (lab2, 1, 2, r, r+1)

				labv = gobject.new(gtk.Label, label=str(value))
				labv.set_alignment(1.0, 0.5)
				table.attach (labv, 2, 3, r, r+1)

				labsv = gobject.new (gtk.Label, label="+####.# mV")
				labsv.set_alignment(1.0, 0.5)
				table.attach (labsv, 6, 7, r, r+1, gtk.FILL | gtk.EXPAND )
				gobject.timeout_add (timeout_update_signal_monitor_reading, self.signal_reading_update, lab2.set_text, labv.set_text, labsv.set_text, tap)

				full_scale = gtk.Entry()
				full_scale.set_text("auto")
				table.attach (full_scale, 10, 11, r, r+1, gtk.FILL | gtk.EXPAND )

				image = gtk.Image()
				if os.path.isfile("meter-icondB.png"):
	    				imagefile="meter-icondB.png"
	    			elif os.path.isfile("/usr/share/gxsm3/pixmaps/meter-icondB.png"):
					imagefile="/usr/share/gxsm3/pixmaps/meter-icondB.png"
            			else:
					imagefile="/usr/share/gxsm/pixmaps/meter-icondB.png"
				image.set_from_file(imagefile)
				image.show()
				button = gtk.Button()
				button.add(image)
				button.connect("clicked", self.create_meterVU, tap, full_scale.get_text)
				table.attach (button, 7, 8, r, r+1, gtk.FILL | gtk.EXPAND )

				image = gtk.Image()
				if os.path.isfile("meter-iconV.png"):
	    				imagefile="meter-iconV.png"
	    			elif os.path.isfile("/usr/share/gxsm3/pixmaps/meter-iconV.png"):
					imagefile="/usr/share/gxsm3/pixmaps/meter-iconV.png"
            			else:
					imagefile="/usr/share/gxsm/pixmaps/meter-iconV.png"
				image.set_from_file(imagefile)
				image.show()
				button = gtk.Button()
				button.add(image)
				button.connect("clicked", self.create_meterLinear, tap, full_scale.get_text)
				table.attach (button, 8, 9, r, r+1, gtk.FILL | gtk.EXPAND )

				image = gtk.Image()
				if os.path.isfile("plot-icon.png"):
	    				imagefile="plot-icon.png"
	    			elif os.path.isfile("/usr/share/gxsm3/pixmaps/plot-icon.png"):
					imagefile="/usr/share/gxsm3/pixmaps/plot-icon.png"
            			else:
					imagefile="/usr/share/gxsm/pixmaps/plot-icon.png"
				image.set_from_file(imagefile)
				image.show()
				button = gtk.Button()
				button.add(image)
				button.connect("clicked", self.create_plotter, tap, full_scale.get_text)
                                table.attach (button, 9, 10, r, r+1, gtk.FILL | gtk.EXPAND )

				r = r+1

			separator = gobject.new(gtk.HSeparator())
			box1.pack_start(separator, expand=False)
			separator.show()

			box2 = gobject.new(gtk.VBox(spacing=10))
			box2.set_border_width(10)
			box1.pack_start(box2, expand=False)

			button = gtk.Button(stock='gtk-close')
			button.connect("clicked", lambda w: win.hide())
			box2.pack_start(button)
			button.set_flags(gtk.CAN_DEFAULT)
			button.grab_default()

		self.wins[name].show_all()

    def update_signal_menu_from_mod_inp(self, _lab, mod_inp):
		[signal, data, offset] = self.mk3spm.query_module_signal_input(mod_inp)
		if offset > 0:
			_lab (" == "+signal[SIG_VAR]+" [%d] ==> "%(offset))
		else:
			_lab (" == "+signal[SIG_VAR]+" ==> ")
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
                    _sig_var("debug[%d] " %(tap-22) + signal[SIG_VAR])
                    _labv(str(value))
#		    _labsv("+####.## mV")
		    _labsv("0x%08X " %value)
		    return 1

	    _sig_var(signal[SIG_VAR])
	    _labv(str(value))

	    if (signal[SIG_UNIT] == "X"):
		    _labsv("0x%08X " %value)
	    else:
		    _labsv("%+06.4f " %uv+signal[SIG_UNIT])
	    return 1

        
#******** :new VP = ================================================================================================================================================================
#  Offset :V[idx] = (       n,  dnx,  0X srcs, 0X opti., p_fb, repetiti., i,j, next,final,  (    du    ,     dx    ,     dy    ,     dz    ,     dx0   ,     dy0   ,     dphi  )mV )
#0X000000 :V[000] = (      16,    0, 00000014, 00000001, 0000,         0, 0,0, +000, +001,  ( -241.4742,    -0.0000,    -0.0000,    -0.0000,    -0.0000,    -0.0000,    -0.0000)mV )
#0X000022 :V[001] = (      16,    0, 00000014, 00000001, 0000,         0, 0,0, +000, +001,  (   -0.0000,    -0.0000,    -0.0000,    -0.0000,    -0.0000,    -0.0000,    -0.0000)mV )
#0X000044 :V[002] = (     100,    0, 00000014, 00000241, 0000,         0, 0,0, +009, +009,  (   -0.0000,    -0.0000,    -0.0000,    -0.0000,    -0.0000,    -0.0000,    -0.0000)mV )
#0X000066 :V[003] = (      16,    0, 00000014, 00000001, 0000,         0, 0,0, +000, +001,  (  241.4742,    -0.0000,    -0.0000,    -0.0000,    -0.0000,    -0.0000,    -0.0000)mV )
#0X000088 :V[004] = (     100,    0, 00000014, 00000001, 0000,         0, 0,0, +000, +001,  (   -0.0000,    -0.0000,    -0.0000,    -0.0000,    -0.0000,    -0.0000,    -0.0000)mV )
#0X0000aa :V[005] = (      16,    0, 00000014, 00000001, 0000,         0, 0,0, +000, +001,  (  241.4742,    -0.0000,    -0.0000,    -0.0000,    -0.0000,    -0.0000,    -0.0000)mV )
#0X0000cc :V[006] = (      16,    0, 00000014, 00000001, 0000,         0, 0,0, +000, +001,  (   -0.0000,    -0.0000,    -0.0000,    -0.0000,    -0.0000,    -0.0000,    -0.0000)mV )
#0X0000ee :V[007] = (     100,    0, 00000014, 00000141, 0000,         0, 0,0, +006, +006,  (   -0.0000,    -0.0000,    -0.0000,    -0.0000,    -0.0000,    -0.0000,    -0.0000)mV )
#0X000110 :V[008] = (      16,    0, 00000014, 00000001, 0000,         0, 0,0, +000, +001,  ( -241.4742,    -0.0000,    -0.0000,    -0.0000,    -0.0000,    -0.0000,    -0.0000)mV )
#0X000132 :V[009] = (     100,    0, 00000014, 00000001, 0000,         2, 2,0, -009, +001,  (   -0.0000,    -0.0000,    -0.0000,    -0.0000,    -0.0000,    -0.0000,    -0.0000)mV )
#0X000154 :V[010] = (      76,    0, 00000000, 00000001, 0000,         0, 0,0, +000, +000,  (   -0.0000,    -0.0000,    -0.0000,    -0.0000,    -0.0000,    -0.0000,    -0.0000)mV )
#0X000176 :V[011] = (      40,    0, 00000014, 00000001, 0000,         0, 0,0, +000, +001,  (   96.5897,    -0.0000,    -0.0000,    -0.0000,    -0.0000,    -0.0000,    -0.0000)mV )
#0X000198 :V[012] = (      76,    0, 00000000, 00000001, 0000,         0, 0,0, +000, +000,  (   -0.0000,    -0.0000,    -0.0000,    -0.0000,    -0.0000,    -0.0000,    -0.0000)mV )
#0X0001ba :V[013] = (      40,    0, 00000014, 00000001, 0000,         0, 0,0, +000, +001,  (  -96.5897,    -0.0000,    -0.0000,    -0.0000,    -0.0000,    -0.0000,    -0.0000)mV )
#0X0001dc :V[014] = (      76,    0, 00000000, 00000001, 0000,         0, 0,0, +000, +000,  (   -0.0000,    -0.0000,    -0.0000,    -0.0000,    -0.0000,    -0.0000,    -0.0000)mV )
#...
#0X000440 :V[032] = (       0,    0, 00000000, 00000001, 0000,         0, 0,0, +000, +000,  (   -0.0000,    -0.0000,    -0.0000,    -0.0000,    -0.0000,    -0.0000,    -0.0000)mV )


    def stop_vpetch (self, _button):
        print "DSP: issuing STOP VP PROBE"
        self.mk3spm.write_stop_probe ()
        return                    
        
    def exec_probe_only (self, _button):
        print "DSP: issuing START VP PROBE"
        self.mk3spm.write_start_probe ()
        return                    

    def dump_vpv (self, button):
        self.mk3spm.dump_VP_vector (range (0, 20))
        
    def dump_vpfb (self, button):
        data = self.mk3spm.dump_VP_fifobuffer ()
        print data
        save("mk3_vpfifo_dump", data)
 
    def start_vpetch (self, _button, get_ncyclesmax, get_amplitude, get_d_level, get_freq, get_duty, get_settle, get_voffset, get_cur_lp_f, get_ref_lp_f, get_lev_up, get_lev_dn, get_dyn_active):
            print "Starting VP Tip Etch Cycle..."
                    
            # test write_VP_vector (self, index, n, ts, nrep, ptr_next, sources, options, d_vector, duration, make_end = 0)
            n_ramp   = 16

            nloops = int (get_ncyclesmax ())
            if nloops < 1 or nloops > 10000000:
                nloops = 1000
            
            n_settle = int (round (float (get_settle ()) / 0.01333333333333))
            if n_settle < 1:
                n_settle = 1

            period = 75000. / float (get_freq ())
            duty = float (get_duty())
            if duty > 1. or duty < 0.:
                duty = 0.5
            pon  = round (0.5 * period * duty)
            poff = round (0.5 * period * (1.-duty))
            
            n_pos    = int (pon)
            n_off1   = int (poff)
            n_neg    = int (pon)
            n_off2   = int (poff)
            n_fin    = 8

            # SRCS MASK:
            # probe.src_input[3]: 0x00004, Lck0: 00008, Lck1stA: 1000, 
            # probe.src_input[0]: 2000, ..[1]: 4000, ..[2]: 8000
            # Zmonitor: 0x00001, Umonitor: 0x00002
            # FB_IN_Processed[0]: 0x0010, ..[1]: 0020,  ..[2]: 0040, [3]: 0080
            # IN4: 0x0100,  IN5: 0x0200,  IN6: 0x0400,  IN7: 0x0800
            srcs     = 0x0040 | 0x0080 | 0x0020 | 0x0002
            # FB_HOLD = 0x001, LIM_UP = 0x100, LIM_DN = 0x200, LIM_EXIT = 0x040
            opt_ramp = 0x00000000
            opt_pos  = 0x00000240
            opt_neg  = 0x00000140

            v_dc = float (get_voffset ())
            if v_dc > 5. or v_dc < -5.:
                v_dc = 0.

            ampl     = 1000. * float (get_amplitude ())
            dc       = 1000. * v_dc
            if ampl < 0. or ampl > 6000.:
                ampl = 1000.
            
            d_vector_dc      = [ dc,         -0.0000,    -0.0000,    -0.0000,    -0.0000,    -0.0000,    -0.0000]
            d_vector_pos     = [ ampl,       -0.0000,    -0.0000,    -0.0000,    -0.0000,    -0.0000,    -0.0000]
            d_vector_neg     = [-ampl,       -0.0000,    -0.0000,    -0.0000,    -0.0000,    -0.0000,    -0.0000]
            d_vector_hold    = [ 0.00,       -0.0000,    -0.0000,    -0.0000,    -0.0000,    -0.0000,    -0.0000]
            d_vector_posfin  = [-ampl-dc,    -0.0000,    -0.0000,    -0.0000,    -0.0000,    -0.0000,    -0.0000]
            d_vector_negfin  = [ ampl-dc,    -0.0000,    -0.0000,    -0.0000,    -0.0000,    -0.0000,    -0.0000]
            ts = 0.
            loop_n = nloops

            vpc = 0
            duration = 0.
            duration = self.mk3spm.write_VP_vector (vpc, 100, 0., 0, 0, 0, 1, d_vector_dc, duration) # START WATCH and shift to DC level
            vpc = vpc+1
            #
            duration = self.mk3spm.write_VP_vector (vpc, n_ramp,   ts,      0,  0, srcs, opt_ramp, d_vector_pos,  duration)
            vpc = vpc+1
            duration = self.mk3spm.write_VP_vector (vpc, n_settle, ts,      0,  0, srcs, opt_ramp, d_vector_hold, duration)
            vpc = vpc+1
            duration = self.mk3spm.write_VP_vector (vpc, n_pos,    ts,      0,  9, srcs, opt_pos,  d_vector_hold, duration)
            vpc = vpc+1
            duration = self.mk3spm.write_VP_vector (vpc, n_ramp,   ts,      0,  0, srcs, opt_ramp, d_vector_neg,  duration)
            vpc = vpc+1
            duration = self.mk3spm.write_VP_vector (vpc, n_off1,   ts,      0,  0, srcs, opt_ramp, d_vector_hold, duration)
            vpc = vpc+1
            duration = self.mk3spm.write_VP_vector (vpc, n_ramp,   ts,      0,  0, srcs, opt_ramp, d_vector_neg,  duration)
            vpc = vpc+1
            duration = self.mk3spm.write_VP_vector (vpc, n_settle, ts,      0,  0, srcs, opt_ramp, d_vector_hold, duration)
            vpc = vpc+1
            duration = self.mk3spm.write_VP_vector (vpc, n_neg,    ts,      0,  6, srcs, opt_neg,  d_vector_hold, duration)
            vpc = vpc+1
            duration = self.mk3spm.write_VP_vector (vpc, n_ramp,   ts,      0,  0, srcs, opt_ramp, d_vector_pos,  duration)
            vpc = vpc+1
            duration = self.mk3spm.write_VP_vector (vpc, n_off2,   ts, loop_n, -9, srcs, opt_ramp, d_vector_hold, duration)
            vpc = vpc+1
            
            # VP program main routine end
            duration = self.mk3spm.write_VP_vector (vpc, 1, 0., 0, 0, 0, 1, d_vector_hold, duration, 1) # END
            vpc = vpc+1
            
            # limiter break from pos, entrance point
            duration = self.mk3spm.write_VP_vector (vpc, n_fin,   ts,      0,  0, srcs, opt_ramp, d_vector_posfin,  duration)
            vpc = vpc+1
            duration = self.mk3spm.write_VP_vector (vpc, 1, 0., 0, 0, 0, 1, d_vector_hold, duration, 1) # END
            vpc = vpc+1
        
            # limiter break from neg, entrance point
            duration = self.mk3spm.write_VP_vector (vpc, n_fin,   ts,      0,  0, srcs, opt_ramp, d_vector_negfin,  duration)
            vpc = vpc+1
            duration = self.mk3spm.write_VP_vector (vpc, 1, 0., 0, 0, 0, 1, d_vector_hold, duration, 1) # END
            vpc = vpc+1
            
            # END END END END.
            for i in range (vpc, vpc+10):
                duration = self.mk3spm.write_VP_vector (i, 0, 0., 0, 0, 0, 1, d_vector_hold, duration, 1) # END
            
            
            # test write_execute_probe (self, bias_i, motor_i, AC_amp, AC_frq, AC_phA, AC_phB, AC_avg_cycels, VP_lim_up, VP_lim_dn, noise_amp=0., start=1)
            level_volt = float (get_d_level ())
            if level_volt > 10. or level_volt < -10.:
                level_volt = 1.

            level_up = float (get_lev_up ())
            if level_up > 10. or level_up < -10.:
                level_up = 0.
            level_dn = float (get_lev_dn ())
            if level_dn > 10. or level_dn < -10.:
                level_dn = 0.

            if get_dyn_active ():
                # VECTOR PROBE LIMITER UP/DN INPUT := MIX 3 signal -- used as finish detection signal
                self.mk3spm.change_signal_input(self, self.mk3spm.lookup_signal_by_name ("MIX OUT 3"), DSP_SIGNAL_VECPROBE_LIMITER_UP_INPUT_ID, self.global_vector_index_null)
                self.mk3spm.change_signal_input(self, self.mk3spm.lookup_signal_by_name ("MIX OUT 2"), DSP_SIGNAL_VECPROBE_LIMITER_DN_INPUT_ID, self.global_vector_index_null) 
                print "configuring for DYNAMIC LEVEL via MIX2,3 OUT="
                print level_volt
            else:
                # VECTOR PROBE LIMITER UP/DN INPUT := USERARR SIGNAL FIXED
                self.mk3spm.change_signal_input(self, self.mk3spm.lookup_signal_by_name ("user signal array"), DSP_SIGNAL_VECPROBE_LIMITER_UP_INPUT_ID, self.global_vector_index_null)
                self.mk3spm.change_signal_input(self, self.mk3spm.lookup_signal_by_name ("user signal array"), DSP_SIGNAL_VECPROBE_LIMITER_DN_INPUT_ID, self.global_vector_index_one)
                print "configuring for static USER LEVELs up,dn="
                print level_up, level_dn


                # Q32s23dot8 factor
            lev_scale = 1./(DSP32Qs23dot8TO_Volt)
                
            usrarr_signal = self.mk3spm.lookup_signal_by_name ("user signal array")
            self.mk3spm.write_parameter (usrarr_signal[SIG_ADR], struct.pack ("<ll", int (round (level_up * lev_scale)), int ( round (level_dn * lev_scale))))

                # Mixer 2,3 enable LIN mode *** Bit0: 0=OFF, 1=ON (Lin mode), Bit1: 1=Log/0=Lin, Bit2: reserved (IIR/FULL), Bit3: 1=FUZZY mode, Bit4: negate input value
            self.mk3spm.write_o (i_feedback_mixer, 4*(2*4+2), struct.pack ("<ll", 0x0001, 0x0001))

                # Mixer 2,3 setpoints used to offset mixer result
            set_up = int (round ( level_volt * lev_scale))
            set_dn = int (round (-level_volt * lev_scale))
            self.mk3spm.write_o (i_feedback_mixer, 4*(3*4+2), struct.pack ("<ll", set_up, set_dn))

                # iir_ca_q15_n = 32767. * (exp (-2.*M_PI*IIR_f0_max[i]/75000.))
            #mix0_ca_q15 -- not touched or used here
            mix1_ca_q15 = int (round (32767. * (exp (-2.*math.pi*float (get_cur_lp_f ())/75000.))))
            mix2_ca_q15 = int (round (32767. * (exp (-2.*math.pi*float (get_ref_lp_f ())/75000.))))
            mix3_ca_q15 = int (round (32767. * (exp (-2.*math.pi*float (get_ref_lp_f ())/75000.))))
            print "MIX1,2,3 CA Q15="
            print mix1_ca_q15, mix2_ca_q15, mix3_ca_q15
            self.mk3spm.write_o (i_feedback_mixer, 4*(4*4+1), struct.pack ("<lll", mix1_ca_q15, mix2_ca_q15, mix3_ca_q15))
                
            print "DSP: issuing START VP PROBE"
            self.mk3spm.write_execute_probe (0., 0.,   0., 4000., 0., 0., 32)
            
        # LIMITER CONDITIONS DEF: if  limiter_input > level_up or limiter_input < level_dn : exit else: continue
        #    if (probe.vector->options & VP_LIMITER_UP)
	#	if (*probe.limiter_input > *probe.limiter_updn[0]){  UP
	#		++probe.lix;
	#		goto run_one_time_step_1; //=> hold or abort branch
	#	}
	#    if (probe.vector->options & VP_LIMITER_DN)
	#	if (*probe.limiter_input < *probe.limiter_updn[1]){  DN
        #	        ++probe.lix;
	#		goto run_one_time_step_1; //=> hold or abort branch
	#    }

            return                    
            

    def create_VP_etch_app(self, _button):

        name="VP Etch App"
        if not self.wins.has_key(name):
            win = gobject.new(gtk.Window,
                              type=gtk.WINDOW_TOPLEVEL,
                              title=name,
                              allow_grow=True,
                              allow_shrink=True,
                            border_width=2)
            self.wins[name] = win
            win.connect("delete_event", self.delete_event)

            # make default signal mapping for VP tip etch

            # MIX 3 input for low pass filtered current watch := IN 3 source signal
            self.mk3spm.change_signal_input(self, self.mk3spm.lookup_signal_by_name ("In 3"), DSP_SIGNAL_MIXER1_INPUT_ID, self.global_vector_index_null)
            # MIX 2 input for ref signal, very slow low pass filtered current watch := IN 3 source signal
            self.mk3spm.change_signal_input(self, self.mk3spm.lookup_signal_by_name ("VP SecV"), DSP_SIGNAL_MIXER2_INPUT_ID, self.global_vector_index_sec_pos) # _int
            self.mk3spm.change_signal_input(self, self.mk3spm.lookup_signal_by_name ("VP SecV"), DSP_SIGNAL_MIXER3_INPUT_ID, self.global_vector_index_sec_neg)
            
            # VECTOR PROBE0..3 INPUT := MIX 3.. signals -- used for section values/monitoring
            self.mk3spm.change_signal_input(self, self.mk3spm.lookup_signal_by_name ("MIX IN 1"), DSP_SIGNAL_VECPROBE0_INPUT_ID, self.global_vector_index_null)
            self.mk3spm.change_signal_input(self, self.mk3spm.lookup_signal_by_name ("MIX IN 2"), DSP_SIGNAL_VECPROBE1_INPUT_ID, self.global_vector_index_null)
            self.mk3spm.change_signal_input(self, self.mk3spm.lookup_signal_by_name ("MIX IN 3"), DSP_SIGNAL_VECPROBE2_INPUT_ID, self.global_vector_index_null)
            self.mk3spm.change_signal_input(self, self.mk3spm.lookup_signal_by_name ("MIX OUT 3"), DSP_SIGNAL_VECPROBE3_INPUT_ID, self.global_vector_index_null)
            # VECTOR PROBE LIMITER INPUT := MIX 3 signal -- used as finish detection signal
            self.mk3spm.change_signal_input(self, self.mk3spm.lookup_signal_by_name ("MIX IN 1"), DSP_SIGNAL_VECPROBE_LIMITER_INPUT_ID, self.global_vector_index_null)


            # VECTOR PROBE LIMITER UP/DN INPUT := MIX 3 signal -- used as finish detection signal
            self.mk3spm.change_signal_input(self, self.mk3spm.lookup_signal_by_name ("MIX OUT 3"), DSP_SIGNAL_VECPROBE_LIMITER_UP_INPUT_ID, self.global_vector_index_null)
            self.mk3spm.change_signal_input(self, self.mk3spm.lookup_signal_by_name ("MIX OUT 2"), DSP_SIGNAL_VECPROBE_LIMITER_DN_INPUT_ID, self.global_vector_index_null)

            # MONITOR 0 (display only) := MIX 3 signal
            self.mk3spm.change_signal_input(self, self.mk3spm.lookup_signal_by_name ("MIX IN 1"), DSP_SIGNAL_MONITOR_INPUT_BASE_ID+0, self.global_vector_index_null)
            # MONITOR 1 (display only) := SEC POS VALUE reading
            self.mk3spm.change_signal_input(self, self.mk3spm.lookup_signal_by_name ("VP SecV"), DSP_SIGNAL_MONITOR_INPUT_BASE_ID+1, self.global_vector_index_sec_pos) #_int)
            # MONITOR 2 (display only) := SEC OFF VALUE reading
            self.mk3spm.change_signal_input(self, self.mk3spm.lookup_signal_by_name ("VP SecV"), DSP_SIGNAL_MONITOR_INPUT_BASE_ID+2, self.global_vector_index_sec_off)
            # MONITOR 3 (display only) := SEC NEG VALUE reading
            self.mk3spm.change_signal_input(self, self.mk3spm.lookup_signal_by_name ("VP SecV"), DSP_SIGNAL_MONITOR_INPUT_BASE_ID+3, self.global_vector_index_sec_neg) #_int)
            # MONITOR 4 (display only) := DSP TIME
            self.mk3spm.change_signal_input(self, self.mk3spm.lookup_signal_by_name ("TIME TICKS"), DSP_SIGNAL_MONITOR_INPUT_BASE_ID+4, self.global_vector_index_null)
            # MONITOR 5 (display only) := PROBE Prcocess FLAG
            self.mk3spm.change_signal_input(self, self.mk3spm.lookup_signal_by_name ("Probe pflag"), DSP_SIGNAL_MONITOR_INPUT_BASE_ID+5, self.global_vector_index_null)
  
            
            box1 = gobject.new(gtk.VBox())
            win.add(box1)

            box2 = gobject.new(gtk.VBox(spacing=0))
            box2.set_border_width(5)
            box1.pack_start(box2)
        
            table = gtk.Table (11, 37)
            table.set_row_spacings(0)
            table.set_col_spacings(4)
            box2.pack_start(table, False, False, 0)
        
            r=0
            lab = gobject.new(gtk.Label, label="Output options: Use one of and hook up 'S46: VP Bias':")
            table.attach (lab, 0, 1, r, r+1)

            r=r+1
            [signal,data,offset] = self.mk3spm.query_module_signal_input(DSP_SIGNAL_OUTMIX_CH6_INPUT_ID)
            [lab1, menu1] = self.make_signal_selector (DSP_SIGNAL_OUTMIX_CH6_INPUT_ID, signal, "DRIVE OUTPUT (OUTMIX_CH6 <= VP Bias, default): ")
            lab1.show ()
            table.attach(lab1, 0, 1, r, r+1)

            r=r+1
            [signal,data,offset] = self.mk3spm.query_module_signal_input(DSP_SIGNAL_OUTMIX_CH7_INPUT_ID)
            [lab1, menu1] = self.make_signal_selector (DSP_SIGNAL_OUTMIX_CH7_INPUT_ID, signal, "DRIVE OUTPUT (OUTMIX_CH7): ")
            lab1.show ()
            table.attach(lab1, 0, 1, r, r+1)

            r=r+1
            [signal,data,offset] = self.mk3spm.query_module_signal_input(DSP_SIGNAL_OUTMIX_CH0_INPUT_ID)
            [lab1, menu1] = self.make_signal_selector (DSP_SIGNAL_OUTMIX_CH0_INPUT_ID, signal, "DRIVE OUTPUT (OUTMIX_CH0): ")
            lab1.show ()
            table.attach(lab1, 0, 1, r, r+1)

            r=r+1
            lab = gobject.new(gtk.Label, label="Setup Monitors:")
            table.attach (lab, 0, 1, r, r+1)

            r=r+1

              
            for tap in range(0,6):
                r=r+1
                [value, uv, signal] = self.mk3spm.get_monitor_signal (tap)
                if signal[SIG_INDEX] < 0:
                    break

                [lab1, menu1] = self.make_signal_selector (DSP_SIGNAL_MONITOR_INPUT_BASE_ID+tap, signal, "Mon[%02d]: "%tap)
                lab1.set_alignment(-1.0, 0.5)
                table.attach (lab1, 0, 1, r, r+1)

                gobject.timeout_add (timeout_update_signal_monitor_menu, self.update_signal_menu_from_signal, menu1, tap)

                lab2 = gobject.new(gtk.Label, label=signal[SIG_VAR])
                lab2.set_alignment(-1.0, 0.5)
                table.attach (lab2, 1, 2, r, r+1)
                
                labv = gobject.new(gtk.Label, label=str(value))
                labv.set_alignment(1.0, 0.5)
                table.attach (labv, 2, 3, r, r+1)
                
                labsv = gobject.new (gtk.Label, label="+####.# mV")
                labsv.set_alignment(1.0, 0.5)
                table.attach (labsv, 6, 7, r, r+1, gtk.FILL | gtk.EXPAND )
                gobject.timeout_add (timeout_update_signal_monitor_reading, self.signal_reading_update, lab2.set_text, labv.set_text, labsv.set_text, tap)
                full_scale = gtk.Entry()
                full_scale.set_text("auto")
                table.attach (full_scale, 10, 11, r, r+1, gtk.FILL | gtk.EXPAND )
                
                image = gtk.Image()
		if os.path.isfile("meter-icondB.png"):
	    		imagefile="meter-icondB.png"
	    	elif os.path.isfile("/usr/share/gxsm3/pixmaps/meter-icondB.png"):
			imagefile="/usr/share/gxsm3/pixmaps/meter-icondB.png"
            	else:
			imagefile="/usr/share/gxsm/pixmaps/meter-icondB.png"
                image.set_from_file(imagefile)
                image.show()
                button = gtk.Button()
                button.add(image)
                button.connect("clicked", self.create_meterVU, tap, full_scale.get_text)
                table.attach (button, 7, 8, r, r+1, gtk.FILL | gtk.EXPAND )

                image = gtk.Image()
		if os.path.isfile("meter-iconV.png"):
	    		imagefile="meter-iconV.png"
	    	elif os.path.isfile("/usr/share/gxsm3/pixmaps/meter-iconV.png"):
			imagefile="/usr/share/gxsm3/pixmaps/meter-iconV.png"
            	else:
			imagefile="/usr/share/gxsm/pixmaps/meter-iconV.png"
                image.set_from_file(imagefile)
                image.show()
                button = gtk.Button()
                button.add(image)
                button.connect("clicked", self.create_meterLinear, tap, full_scale.get_text)
                table.attach (button, 8, 9, r, r+1, gtk.FILL | gtk.EXPAND )
                
                image = gtk.Image()
		if os.path.isfile("plot-icon.png"):
	    		imagefile="plot-icon.png"
	    	elif os.path.isfile("/usr/share/gxsm3/pixmaps/plot-icon.png"):
			imagefile="/usr/share/gxsm3/pixmaps/plot-icon.png"
            	else:
			imagefile="/usr/share/gxsm/pixmaps/plot-icon.png"
                image.set_from_file(imagefile)
                image.show()
                button = gtk.Button()
                button.add(image)
                button.connect("clicked", self.create_plotter, tap, full_scale.get_text)
                table.attach (button, 9, 10, r, r+1, gtk.FILL | gtk.EXPAND )


            r=r+1
            lab = gobject.new(gtk.Label, label="Sensing Input (CHn) hooked to MIX1 Setup.\n NOTE: ENABLE MIXER CHANNELs 2+3 PROCESSING 'LINEAR'")
            table.attach (lab, 0, 1, r, r+1)

            r=r+1
            [signal,data,offset] = self.mk3spm.query_module_signal_input(DSP_SIGNAL_MIXER1_INPUT_ID)
            [lab1, menu1] = self.make_signal_selector (DSP_SIGNAL_MIXER1_INPUT_ID, signal, "SENSE INPUT (MIX1 <= CHn): ")
            lab1.show ()
            table.attach(lab1, 0, 1, r, r+1)

            r=r+1
            separator = gobject.new(gtk.HSeparator())
            table.attach(separator, 0, 4, r, r+1)
            separator.show()
                        
            tr = r+1
            lab = gobject.new(gtk.Label, label="N cycels max")
            lab.show ()
            table.attach(lab, 0, 1, tr, tr+1)
            e = gtk.Entry()
            e.set_text("10")
            table.attach(e, 1, 2, tr, tr+1)
            e.show()
            ncyclesmax = e

            lab = gobject.new(gtk.Label, label="Amplitude [V]")
            lab.show ()
            table.attach(lab, 2, 3, tr, tr+1)
            e = gtk.Entry()
            e.set_text("4.0")
            table.attach(e, 3, 4, tr, tr+1)
            e.show()
            amplitude=e
            
            tr = tr+1
            lab = gobject.new(gtk.Label, label="DC Voltage [V]")
            lab.show ()
            table.attach(lab, 0, 1, tr, tr+1)
            e = gtk.Entry()
            e.set_text("-0.5")
            table.attach(e, 1, 2, tr, tr+1)
            e.show()
            voffset=e

            lab = gobject.new(gtk.Label, label="current low pass [Hz]")
            lab.show ()
            table.attach(lab, 2, 3, tr, tr+1)
            e = gtk.Entry()
            e.set_text("1000.")
            table.attach(e, 3, 4, tr, tr+1)
            e.show()
            cur_lp_f=e
            
            tr = tr+1
            lab = gobject.new(gtk.Label, label="Freq. [Hz]")
            lab.show ()
            table.attach(lab, 0, 1, tr, tr+1)
            e = gtk.Entry()
            e.set_text("400")
            table.attach(e, 1, 2, tr, tr+1)
            e.show()
            freq=e

            lab = gobject.new(gtk.Label, label="Duty Cycle [0..1]")
            lab.show ()
            table.attach(lab, 2, 3, tr, tr+1)
            e = gtk.Entry()
            e.set_text("0.5")
            table.attach(e, 3, 4, tr, tr+1)
            e.show()
            duty=e

            tr = tr+1
            lab = gobject.new(gtk.Label, label="Dyn. Level [V]")
            lab.show ()
            table.attach(lab, 0, 1, tr, tr+1)
            e = gtk.Entry()
            e.set_text("1.0")
            table.attach(e, 1, 2, tr, tr+1)
            e.show()
            level=e
            
            lab = gobject.new(gtk.Label, label="Ref low pass [Hz]")
            lab.show ()
            table.attach(lab, 2, 3, tr, tr+1)
            e = gtk.Entry()
            e.set_text("100.")
            table.attach(e, 3, 4, tr, tr+1)
            e.show()
            ref_lp_f=e

            rbdyn = gtk.RadioButton(group=None, label="Dynamic Limit")
            rbdyn.show ()
            table.attach(rbdyn, 4, 5, tr, tr+1)
            rbdyn.set_active(True)

            tr = tr+1
            lab = gobject.new(gtk.Label, label="Fixed Level Up [V]")
            lab.show ()
            table.attach(lab, 0, 1, tr, tr+1)
            e = gtk.Entry()
            e.set_text("-0.02")
            table.attach(e, 1, 2, tr, tr+1)
            e.show()
            f_level_up=e
            
            lab = gobject.new(gtk.Label, label="Down [V]")
            lab.show ()
            table.attach(lab, 2, 3, tr, tr+1)
            e = gtk.Entry()
            e.set_text("-0.1")
            table.attach(e, 3, 4, tr, tr+1)
            e.show()
            f_level_dn=e

            rbstat = gtk.RadioButton(rbdyn, label="Static Limit")
            rbstat.show ()
            table.attach(rbstat, 4, 5, tr, tr+1)
            rbstat.set_active(True)
            
            tr = tr+1
            lab = gobject.new(gtk.Label, label="LIMITER CONDITION := if ( limiter_input > level_up  or  limiter_input < level_dn ):  exit  else: continue")
            lab.show ()
            table.attach(lab, 0, 5, tr, tr+1)

            tr = tr+1
            lab = gobject.new(gtk.Label, label="Settling time [ms] (0.01333ms / per sample)")
            lab.show ()
            table.attach(lab, 0, 1, tr, tr+1)
            e = gtk.Entry()
            e.set_text("0.2")
            table.attach(e, 1, 2, tr, tr+1)
            e.show()
            settle=e
            
            
            separator = gobject.new(gtk.HSeparator())
            box1.pack_start(separator, expand=False)
            separator.show()
                        
            button = gtk.Button(stock='gtk-execute')
            button.connect("clicked", self.start_vpetch, ncyclesmax.get_text, amplitude.get_text, level.get_text, freq.get_text, duty.get_text, settle.get_text,
                           voffset.get_text, cur_lp_f.get_text, ref_lp_f.get_text, f_level_up.get_text, f_level_dn.get_text, rbdyn.get_active) 
            box2.pack_start(button)

            button = gtk.Button(stock='gtk-stop')
            button.connect("clicked", self.stop_vpetch)
            box2.pack_start(button)

            button = gtk.Button('VP EXECUTE (debug mode)')
            button.connect("clicked", self.exec_probe_only)
            box2.pack_start(button)

            button = gtk.Button('VPV dump (debug mode)')
            button.connect("clicked", self.dump_vpv)
            box2.pack_start(button)

            button = gtk.Button('VP FIFO BUFFER dump (debug mode)')
            button.connect("clicked", self.dump_vpfb)
            box2.pack_start(button)

            separator = gobject.new(gtk.HSeparator())
            box1.pack_start(separator, expand=False)
            separator.show()
                        

            box2 = gobject.new(gtk.VBox(spacing=10))
            box2.set_border_width(10)
            box1.pack_start(box2, expand=False)

            button = gtk.Button(stock='gtk-close')
            button.connect("clicked", lambda w: win.hide())
            box2.pack_start(button)
            button.set_flags(gtk.CAN_DEFAULT)
            button.grab_default()

            self.wins[name].show_all()


##### fire up default background updates then run main gtk loop
    def main(self):
	    gobject.timeout_add (timeout_DSP_status_reading, self.mk3spm.get_status)	
	    gobject.timeout_add (timeout_DSP_signal_lookup_reading, self.mk3spm.read_signal_lookup, 1)

	    gtk.main()


########################################

print __name__
if __name__ == "__main__":
	mk3 = Mk3_Tool_Menu ()
	mk3.main ()

## END
