/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 8 c-style: "K&R" -*- */

/* Gnome gxsm - Gnome X Scanning Microscopy
 * universal STM/AFM/SARLS/SPALEED/... controlling and
 * data analysis software
 *
 * Gxsm Hardware Interface Plugin Name: sranger_mk2_hwi.C
 * ===============================================
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


/* Please do not change the Begin/End lines of this comment section!
 * this is a LaTeX style section used for auto generation of the PlugIn Manual 
 * Chapter. Add a complete PlugIn documentation inbetween the Begin/End marks!
 * All "% PlugInXXX" commentary tags are mandatory
 * All "% OptPlugInXXX" tags are optional and can be removed or commented in
 * --------------------------------------------------------------------------------
% BeginPlugInDocuSection
% PlugInDocuCaption: Signal Ranger MK2/3-A810 Hardware Interface
% PlugInName: sranger_mk2_hwi
% PlugInAuthor: Percy Zahl
% PlugInAuthorEmail: zahl@users.sf.net
% PlugInMenuPath: Tools/SR-DSP Control

% PlugInDescription
\index{Signal Ranger MK2 HwI}
\index{Signal Ranger MK2/3 HwI}
This provides the SignalRanger-MK2 and -MK3-Pro (equipped with the A810
analog IO module) hardware interface (HwI) for GXSM.  It contains all
hardware close and specific settings and controls for feedback,
scanning, all kind of probing (spectroscopy and manipulations) and
coarse motion control including the auto approach
controller. Invisible for the user it interacts with the SRanger DSP,
manages all DSP parameters and data streaming for scan and probe. The
newer MK3-Pro (mark 3) DSP provides more computing power and a 32bit
architecture and is capable of running a software PLL on top of the
standart A810 architecture and a PLL optimized A810 module what is
fully compatible but provided a more precise (stabilized) frequency
reference for higher PAC/PLL precision.

\GxsmNote{THIS PLUGIN IS DERIVED FROM THE ORIGINAL SR MODULE AND IT
  STARTS WITH EXACTLY THE SAME FEATURES. THIS DOCUMENT SECTION IS
  STILL WRITING IN PROGRESS.  We assume by now that the SR-MK2 or MK3-Pro is
  equiped with the Analog-810 (MK2-A810 or MK3-PLL SPM Controller), this is 
  the new SPM dedicated analog module GXSM will supports best starting in 2009
  (official introduction: Feb-2009) on. Main differenes/upgrades are 75/150kHz
  loop, high stability and precision AD/DA, lowest possible loop delays
  (less than 5 samples total), no hardware FIR/IIR or oversampling any
  longer on any channel.}

\GxsmNote{The MK2-A810 AD/DA channel configuration is by now
  equivaltent to the old SRanger with excetion of the new 4-channel
  feedback soure mixer inputs.  SR-AIC0..7-in/out correspondes to
  MK2-A810 AD/DA0..7 (See Appendix, SRanger DSP).\\ In brief:\\
  DA0/1/2:
  X0/Y0/Z0, DA3/4/5: Xs/Ys/Zs, DA6/7: Bias, Motor.\\
  AD0: real time IIR enables channel, Mixer input 0 (default feedback
  in for Current-Signal), AD1,2,3: additional input channels usable
  for feedack source mixer, AD4..7: Auxilary.  }

\GxsmNote{Notes on the latest MK3-Pro-A810/PLL ``Snowy Janus Hack'':
Starting with this experimental SVN revision
https://sourceforge.net/p/gxsm/svn/HEAD/tree/branches/Gxsm-2x3-transition-sig
a totally real time and hot configurable so called signal
configuration allows for any in- and output configuration and thus all
there is a default assignment so far known from the MK2. Please refer
to the MK3 subsection for deviations from the MK2 interface and setup.}

%The \GxsmEntry{SR DSP Control} Dialog is divided into several sections:
%1) Feedback & Scan: This folder contains all necessary options regarding feedback and scan.
%3) Advanced: Different settings for advanced features of the Signal Ranger.
%4) STS: Parameter for dI/dV spectroscopy can be defined here.
%5) FZ: In this section information about force - distance curves can be set.
%6) LM: For manipulation of x, y, or z use this dialog.
%7) Lock-In: This slice contains the parameter set for the build in Lock-In.
%8) AX: Auxiliary
%9) Graphs: Choose sources and decide how to plot them.

\GxsmGraphic{SR-DSP-MK3-DSP-Block}{0.285}{Schematic diagram of the
    DSP code topology: Startup section configures the DSP system, sets
    up timers for data processing interrupt subroutine (ISR) and enters the
    never ending idle loop, which implements a state machine.}

\GxsmGraphic{SR-DSP-MK3-mixer}{0.33}{Schematic diagram of the new DSP feedback with four signal
    source mixer. The input signals are transformed as requested
    (TR$_i$: linear/logarithmic/fuzzy) and the error signal $\Delta_i$
    is computed for every channel $i$ using individual set points. The
    sum of previously with gain $G_i$ scaled delta signals is computed
    and feed into the feedback algorithm $FB$ as $\Delta$.}
%
\GxsmGraphic{SR-DSP-MK3-IIR-plot-q}{0.35}{Life IIR performance demonstration in Gxsm
    self-test configuration. A stair
    case like current input signal starting at $3.6\:$pA noise level
    assuming $1\:$V/nA (or a $\times 10^9$ gain)
    with logarithmic steps starting at $20\:\mbox{pA}$ and scaled by
    $\times 2$ for the following steps (ADC-0 (black)) is generated using
    the Gxsm "PL"-Vector Probe mode. At the same time the IIR-response 
    (via Z-mon (red)) and the real-time $q$ (blue) is recorded.
    IIR settings used:
    $f_{\mbox{\tiny min}} = 50\:\mbox{Hz}$, $f_{\mbox{\tiny max}} =
    20\:\mbox{kHz}$, $\mbox{I}_c = 500\:\mbox{pA}$.}
%
New: The A810 DSP code implements a high resolution (HR) mode for
the DAC. This increases the bit resolution of selected output
channels up to 3 bits. This is possible on DSP software level now
running sampling and data processing at full 150$\:$kHz but limiting
all other DSP tasks like scan and feedback to a reasonable fraction
of this. For all input channels an automatic scan speed depending
bandwidth adjustment (simple averaging) is used as before, but now
the gained resolution due to statistics is not any longer thrown
away (rounded off to integer), but the full 32 bit accumulated value
and normalization count is kept and transferred. A normalization to
the original 16 bit magnitude, but now as floating point number, is
done by the HwI as data post processing. For performance reasons and
future expansions the FIFO data stream consisting of a set of 32 bit
signals is now compressed using first order linear predictor and
custom encoded byte packed.

In particular the signal to noise ratio of small or noisy signals will
increase automatically with lowering the scan speed (given here in
pixels/s). All available input channels ADC$_i$ are managed in this
automatic scan speed matching bandwidth mode:
\[ \mbox{V} = \frac{1}{N} \sum_{n=0}^{N-1} \mbox{ADC}_{i}(n)\mbox{,} \quad N = \frac{75000\:\mbox{Hz}}{\mbox{Pixel}/\mbox{s}} \]
or in signal noise gain ($S_N$) terms
\[ S_N = 20 \log\left(\frac{1}{\sqrt{N}}\right) \]
is the gain in signal to noise ratio -- assuming statistical noise --
on top of the available 16$\:$bits (1$\:$bit RMS) of the
MK2-A810.

This now requires tp adjust the Gxsm preferences as indicated on Gxsm startup
if not setup correctly. Refer also to the sample configuration shown in the
appendix \ref{A810-gxsm-pref} of this HwI section.

Further the latest experimental release revises the feedback loop
configuration and allows up to four signals (provided on ADC0$\dots$3)
to be user configurable as feedback sources using linear or
logarithmic signal transformations and even can be mixed and weighted
in different ways via the gains $G_i$ as illustrated in Fig.~\ref{fig:SR-DSP-MK3-mixer}. It also includes a special ``FUZZY'' mix mode for
signal level depended enabling of a particular chanqnel only if its
signal is beyond a given level; only the ``amount beyond'' is used for
$\Delta_i$ computation.

This multi channel feedback mode allows for example contineous
transitions between STM and AFM or Dynamic Force Microscope (DFM)
operation modes. The ``FUZZY'' mode can be used in many ways, one may
be a kind of ``tip guard'' mechanism watching for special conditions,
i.e. watching the power dissipation signal commonly available from
Phase Locked Loop (PLL) controllers used for DFM.

For channel ADC$_0$ a real time self adaptive Infinite Respose (IIR)
filter which adjusts its frequency as function of the signal
magnitude is implemented -- assuming to be used for sampling the
tunnel current in STM mode.  The user selects a crossover current
I$_c$ and cut-off lower limit frequency $f_{\mbox{\tiny min}}$, also
the upper bandwidth can be limited to $f_{\mbox{\tiny max}}$. This
will then in real time limit the ADC$_0$ input bandwidth in dependence
of the signal magnitude $|\mbox{I}_n|$ according to:

% CODE snipets for ref.
% 	// IIR self adaptive filter parameters
% 	double ca,cb;
% 	double Ic = main_get_gapp()->xsm->Inst->VoltIn2Dig (main_get_gapp()->xsm->Inst->nAmpere2V (1e-3*IIR_I_crossover)); // given in pA
% 	dsp_feedback.ca_q15   = sranger_mk2_hwi_hardware->float_2_sranger_q15 (ca=exp(-2.*M_PI*IIR_f0_max/75000.));
% 	dsp_feedback.cb_Ic    = (DSP_LONG) (32767. * (cb = IIR_f0_min/IIR_f0_max * Ic));
% 	dsp_feedback.I_cross  = (int)Ic; 
% 	dsp_feedback.I_offset = 1+(int)(main_get_gapp()->xsm->Inst->VoltIn2Dig (main_get_gapp()->xsm->Inst->nAmpere2V (1e-3*LOG_I_offset))); // given in pA
% ... dataprocess...
% 	if (feedback.I_cross > 0){
% 		// IIR self adaptive
% 		feedback.I_fbw = AIC_IN(0);
% 		AbsIn = _abss(AIC_IN(0));
% 		feedback.q_factor15 = _lssub (Q15L, _smac (feedback.cb_Ic, AbsIn, Q15) / _sadd (AbsIn, feedback.I_cross));
% 		feedback.zxx = feedback.q_factor15;
% 		if (feedback.q_factor15 < feedback.ca_q15)
% 		      feedback.q_factor15 = feedback.ca_q15;
% 		feedback.I_iir = _smac ( _lsmpy ( _ssub (Q15, feedback.q_factor15), AIC_IN(0)),  feedback.q_factor15, _lsshl (feedback.I_iir, -16));
% 		AIC_IN(0) = _lsshl (feedback.I_iir, -16);
% 	}
% ------
% Ic, IIR_f0_min,max
% ca_q  = exp(-2pi IIR_f0_max / 75000)  ----> f0 = -ln(q) / 2pi * 75000Hz
% cb_Ic = IIR_f0_min / IIR_f0_max * Ic
% I_offset
% --> q    = (IIR_f0_min / IIR_f0_max * Ic + abs(I_n)) [=cb_Ic] / (abs(I_n) + I_c)
% --> if (q < ca) q=ca_q // limit to max BW
% --> ~I_n = q * ~I_n-1  +  (1-q) * I_n

\[ f_0(q) = -75000\:\mbox{Hz} \frac{\ln(q)}{2 \pi}, \quad 
q(\mbox{I}_n) = 1-\frac{\frac{f_{\mbox{\tiny min}}}{f_{\mbox{\tiny max}}} \mbox{I}_c + |\mbox{I}_n|}{\mbox{I}_c + |\mbox{I}_n|}
\]

This real time computed $q$ is limited to a $q_{\mbox{\tiny min}}$
matching the given $f_{\mbox{\tiny max}}$ before the bandwidth limited
(IIR filter) current signal $\tilde{\mbox{I}}_n$ which is recursively
computed on the DSP according to:

\[ \tilde{\mbox{I}}_n = q \tilde{\mbox{I}}_{n-1} + (1-q) \mbox{I}_n \]

Fig.~\ref{fig:SR-DSP-MK3-IIR-plot-q} illustrates the IIR filter response to a
logarithmic steepening stair case test input signal. This is a live
IIR performance demonstration utilizing the Gxsm Vector Probe Engine
itself and having Gxsm and the MK2-A810 set up in a
self-test configuration. The stair case like
current input signal is starting at about $3.6\:$pA noise level
assuming $1\:$V/nA (or a $\times 10^9$ gain). The logarithmic steps
start at $20\:\mbox{pA}$ and scale by $\times 2$ for the adjacent
steps (ADC-0 (black)). The test signal is generated using the Gxsm
"PL"-Vector Probe mode. At the same time the IIR-response (via Z-mon
(red)) and the real-time $q$ (blue) is recorded.

The IIR settings used for this demonstration are: $f_{\mbox{\tiny min}} =
50\:\mbox{Hz}$, $f_{\mbox{\tiny max}} = 20\:\mbox{kHz}$, $\mbox{I}_c =
500\:\mbox{pA}$.

Looking at the IIR signal (red) the filter effectivity is clearly
visible for small signals, also the fast "step-up" response is
demonstrated by comparing with the slower "step-down" response.
Also note the $q$ cut-off indicated as the q cut-off level at 20$\:$kHz.

For a low signal to noise ratio (given for tunneling currents in the
pA regime) the feedback stability can be gained by a combination of
IIR filtering and slower scanning.
% Typically small tunnel signals at high amplifier gains are more noisy
% due to statistics and the feedback (Z) stability can be regained by
% signal recovery techniques like IIR filtering using experiment and
% system adapted bandwidth limiting and slow scanning. 
This digital self adapting IIR filter implementation allows full
control of the frequency ranges and guarantees a fast tip response to
a sudden increase of the tunnel signal (up to full band width) as
needed to prevent the tip from crashing into step bunches or other
``edges''.


% OptPlugInSection: SR-DSP Feedback and Scan Control
\index{Signal Ranger MK2 Feedback}
\index{Signal Ranger MK2 Scan Control }
The Feedback \& Scan folder contains all necessary options regarding
feedback and scan.  Here the feedback parameters and tunneling/force
settings are adjusted. The second purpose of this control panel is the adjustment of
all scan parameters of the digital vector scan generator like speed.

\GxsmScreenShotW{SR-DSP-MK3-DSP-Feedback-Scan}{GXSM4 MK3-PLL/A810 DSP Control window: Feedback and Scan Control page with activated configuration mode and internal offset-adding}{0.9\textwidth}
\GxsmScreenShotW{SR-DSP-MK3-DSP-Advanced}{GXSM4 MK3-PLL/A810 DSP Control window: Advanced settings page with activated configuration mode}{0.9\textwidth}

\begin{description}

\item[Bias] Voltage that is applied to OUT6. The entered value will be
  divided by the \GxsmPref{InstSPM}{BiasGain} value. Without an
  additional amplification or attenuation the
  \GxsmEntry{Instrument/BiasGain} value has to be set to 1.

\item[Feedback Mixer Source: SCurrent] The signal from ADC0 is feed to
  the self adaptive FIR (if enabled in Advanced Settings) and the
  processed as any other Mixer Source Input. However, this channel is
  expected to be used for (STM) Current Setpoint as it is converted
  due to the setting of \GxsmPref{InstSPM}{nAmpere2Volt}.

\item[Feedback Mixer Source: SSetPoint, SAux2,3] Further Mixer input
  signals from ADC1, ADC2 and ADC3.

\item[Mixer Gains] For scaling of the error signal (SetPoint -
  Source), use about 0.5 at max. Bascially works as pre-scaling of CP
  and CI together if only one channel is used.

\item[CP, CI] Parameters for the feedback loop. CP is proportional to
  the difference between the set point and the current value at IN5.
  CI integrates this difference. Higher values for CP / CI mean a
  faster loop.

\item[MoveSpd, ScanSpd] MoveSpd is valid for movements without taking
  data (e.g. change xy-offset or moving to the starting point of an
  area scan). ScanSpd is valid if the DSP is taking data.
  PiezoDriveSetting: Usually a high voltage amplifier has different
  gains. A change of the gain can easily be mentioned in GXSM by
  activating the appropriate factor. A change of this values acts
  instantly. The available gain values can be defined in the
  \GxsmPref{InstSPM}{Analog/V1-V9}. After changing the preferences
  GXSM needs to be restarted to take effect. Remark: Piezo constants
  and other parameters have to be charged. Please use the
  \GxsmPref{InstSPM}{Instrument/PiezoAV} for this odd values.
\item[LDCdx,dy] Coefficients for Linear Drift Correction -- active if
  \GxsmEmph{Enable LDC} is checked. While checked, any manual Offset
  changes are prohibited (blocked) -- uncheck to perform offset
  changes. This is the speed and direction the tip is moving to
  correct for drift. Use the negative number found via
  \GxsmPopup{Scan}{View/Coordinates/Relative, check Time} and set a
  Globel Reference Point via Point-Object in a previous Scan to mark a
  feature.
\end{description}

\GxsmHint{Good conservative start values for the feedback loop gains
  CP and CI (on a MK2-A810 system, 32 bit internal handling) are 0.004 
  for both. Typically they can be increased up to 0.01, depending on the
  the system, tip and sample. \\
  The MK3Pro-A810 handels the feedback 
  internally with 64 bit. Therefore, the values for the feedback are 
  roughly a factor of 256 higher than for a MK2 based system. Good 
  standard values for the Mk3Pro-A810 based system will be between 20 
  and 40. A meaningfull maximum is 256. CP = 100 (MK3Pro-A810) results 
  in 1 mV change per 1 V error signal.\\
  In general CP can be about 150\% of CI to gain more 
  stability with same CI. The feedback transfer is 1:1 (error to output
  per loop) if all gains, CP and CI, are set to one in linear mode.}


% OptPlugInSection: Advanced Feedback and Probe Control

Advanced is a collection of different settings, to handle the advanced
capabilities of the Signal Ranger DSP. It is quite a mixture of
different tools.

\begin{description}
\item[FB Switch] This option enables or disables the feedback loop.
  Disabling will keep the current Z-position.

\item[Frq-Ref] must be set to 75000$\:$kHz and not changed at any time
  (guru mode only).

\item[IIR] ADC0 (Current) can be real time signal magnituden dependent
  IIR filtered. A self adjusting IIR algorithm is used. See
  introduction to this plugin for details.

\item[Automatic Raster Probe] In general spectra can easily be taken
  while doing an area scan. Activating this feature forces the DSP to
  take spectra every line with an intended misalignment. A value of 1
  means a spectrum will be taken every scan point. A value of 9 means
  a distance of 9 area scan points between two spectra. The driven
  spectrum depends on the probe control that is selected (STS, Z, PL).
  Single spectra of a Raster Probe Scan easily can be handled by using
  the \GxsmPopup{Events}{Show Probe} feature in the area plot. For a
  conversion into a layer based image please use the
  \GxsmMenu{Math/Probe/ImageExtract} plug-in.

\item[Show Expert Controls] This option hides or reveals some advanced
  options in different folders. In this help file controls will be
  mentioned as Expert Controls, if they are revealed by this option.

\item[DynZoom] With the Signal Ranger DSP it is possible to dynamical
  zoom while scanning -- side effects on offest as zooming (data point
  separation adjustment) occurrs in real time at the current tip
  position, guru mode only. Always set back to one at scan start.

\item[PrePts] Problems with piezo creep in x-direction can be reduced
  by scanning a larger distance. The DSP adds equivalent points at
  both ends of a line which will be ignored in the resulting scan
  data. The total scan size will get larger in x by a factor of $(1 +
  2 * PrePts / ScanPointsX)$.  Use the Pan-View plug-in to prevent
  trouble with the maximum scan size.
\end{description}



% OptPlugInSubSection: STS: IV-Type Spectroscopy

\index{Signal Ranger MK2/3 Spectroscopy}
\index{Signal Ranger MK2/3 Manipulation}
\index{Signal Ranger MK2/3 Vector Probe}

\GxsmScreenShotW{SR-DSP-MK3-DSP-Advanced-Raster}{Advanced setup with Raster enabled}{0.9\textwidth}
\GxsmScreenShotW{SR-DSP-MK3-DSP-STS}{STS tab of the MK3 DSP control}{0.9\textwidth}

This is the dialog for scanning tunneling spectroscopy (STS). Sources
can be chosen in the Graphs Folder (Please have a look at the Graphs
section for details).

\begin{description}
\item[IV-Start-End, \#IV] These are the start and the end values of
  the spectrum. Optional a repetition counter is shown (Expert Control
  option enabled). This forces the Signal Ranger to do the STS
  spectrum n times.

\item[IV-dz] Lowering of the tip can improve the signal to noise ratio
  especially at low voltages. IV-dz is the maximum lowering depth
  which is reached at 0V. The lowering depth is equal to zero if the
  the I/V curve meets the bias voltage. Lowering the tip means a
  decrease of the tunneling gap. An automatic correction of the
  resistance is implemented by means of several (\#) dI/dz spectra at
  the bias voltage (Expert Control option).

\item[Points] Number of points the spectrum will have. Please have a
  look at \GxsmEntry{Int} for additional informations.

\item[IV-Slope] Slope while collecting data.  

\item[Slope-Ramp] Slope while just moving.  

\item[Line-dXY, \#Pts] Only shown if Expert Control option is set.
  Only if \#Pts is set to more than one the complete procedure
  previously defined will be repeated on \#Pts points equally spread
  along the via Line-dXY defined vector starting at the current tip
  position. Markers will be placed into the active scan window once
  done if Auto Plot is selected or every time updated if Plot is hit.
  To find the propper Vector coordinates, use the Line Object and read
  the dxy vector from the status line as indiceted by d(dx, dy).
  IV-Line Slope is used for moving speed from point to point --
  feedback will be enabled and the recovery delay is applied before
  the next STS probe cycle starts. Enable ``Show Probe Events'' (Scan
  side pane, Probe Events), else nothing will be shown on the scan.
  See demo setup in appendix \ref{A810-sts-demo} to this section.

\item[Final-Delay] After running a spectrum this is the time the DSP
  waits to enable the feedback again.

\item[Recover] This is the time between two spectra where the feedback
  is activated for a readjustment of the distance (Expert Control
  option).

\item[Status] Gives information about the ongoing spectrum:

\item[Tp] Total time the probe needs.  

\item[dU] Maximum difference of the voltages that will be applied.

\item[dUs] Stepsize of the data points.  

\item[Feedback On] Decides whether the feedback will be on or off
  while taking a spectrum.

\item[Dual] When activated the DSP will take two spectra. One spectrum
  is running from Start to End directly followed by a spectrum from
  the End to the Start value.

\item[Int] When activated the DSP will average all fetched data
  between two points. It can easily be seen, that decreasing the
  values of IV-Slope or Points will increase the oversampling and
  therefore will improve the quality of the spectrum.See note ($*$)
  below.

\item[Ramp] This option forces the DSP to stream all data to the PC
  including the Slope-Ramps.

\item[Save] When activated, GXSM will save spectra automatically to
  your home directory.

\item[Plot] When activated, GXSM will automatically show/update the
  plots chosen in the Graphs dialog.
\end{description}


\GxsmNote{$*$ The sampling rate of the Signal Ranger is 22.1 kHz so
  the time between two points of a spectrum leads directly to the
  number of interim points that can be used for oversampling.
%
total time for the spectrum:
\[ ts = dU / IV-Slope \]
%
time per point:
\[ tp = ts / Points = dU / (IV-Slope * Points) \]
%
number of samples at one point:
\[ N = tp * 75000 Hz = 75000 Hz * dU / (IV-Slope * Points) \]
}


% OptPlugInSubSection: Vertical (Z) Manipulation
Manipulation in general is controlled or forced top motion in one or
more dimensions for any desired purpose.
This is the dialog for distance spectroscopy and forced Z/tip manipulation.

\GxsmScreenShotW{SR-DSP-MK3-DSP-Z}{GXSM SR DSP Control window, tab Z manipulation}{0.9\textwidth}

\begin{description}
\item[Z-Start-End] These are the start and the end values of the spectrum in
respect to the current position.  

\item[Points] Number of points the spectrum will have. Please have a look at
\GxsmEntry{Int} for additional informations.

\item[Z-Slope] Slope while collecting data.

\item[Slope-Ramp] Slope while just moving.

\item[Final-Delay] After running a spectrum this is the time the DSP waits
to enable the feedback again.

\item[Status] Gives information about the ongoing spectrum:

\item[Tp] Total time the probe needs. 
\end{description}

Informations about the check options can be found in STS.



% OptPlugInSubSection: Lateral Manipulation (LM)
With LM a lateral manipulation of the tip/sample is possible.
But also the Z-dimension can be manipulated at the same time if dZ set to a non zero value.

\begin{description}
\item[dxyz] Distance vector that will be covered.

\item[Points] While moving it is possible to collect data. Points defines the number of collected data points.

\item[LM-Slope] Speed of the tip/sample.

\item[Final-Delay] Timeout after lateral manipulation.

\item[Status] Gives information about the ongoing move.
\end{description}

Informations about the check options can be found in STS.




% OptPlugInSubSection: Pulse (PL) and Tip Forming or controlled crash Z manipulations
There are several possibilities to prepare or manipulate a tip or locale surface area. One is to dip the
tip into the sample in a controlled manner, another option is applying a charge pulse or both combined.

%\GxsmScreenShotW{SR-DSP-MK3-DSP-PL}{GXSM SR DSP Control window: PL mode setup}{0.9\textwidth}
%\GxsmScreenShot{A810-DSP-Control-PL-demo}{GXSM2 SR DSP Control window: PL mode - demo setup with PanView showing Z adjusted to 4.7$\:$\AA\ for testing.}
%\GxsmScreenShot{A810-DSP-Control-PL-plot-demo}{GXSM2 SR DSP Control window: PL mode - demo plot in self-test mode (Bias loop back to current)}
%\GxsmScreenShot{A810-DSP-Control-Graphs-PLdemo}{GXSM2 SR DSP Control window: PL mode - demo Graphs configuration}
%\GxsmScreenShot{A810-DSP-Control-PL-demo3x}{GXSM2 SR DSP Control window: PL mode - demo for repetions}
%\GxsmScreenShot{A810-DSP-Control-PL-plot-demo3x}{GXSM2 SR DSP Control window: PL mode - demo for repetitions}



\GxsmScreenShot{SR-DSP-MK3-ADV-configure}{SR-DSP-MK3-ADV-configure}
\GxsmScreenShot{SR-DSP-MK3-ADV-simple}{SR-DSP-MK3-ADV-simple}
\GxsmScreenShot{SR-DSP-MK3-DSP-Advanced}{SR-DSP-MK3-DSP-Advanced}
\GxsmScreenShot{SR-DSP-MK3-DSP-Advanced-Raster}{SR-DSP-MK3-DSP-Advanced-Raster}
\GxsmScreenShot{SR-DSP-MK3-DSP-AX}{SR-DSP-MK3-DSP-AX}
\GxsmScreenShot{SR-DSP-MK3-DSP-Feedback-Scan}{SR-DSP-MK3-DSP-Feedback-Scan}
\GxsmScreenShot{SR-DSP-MK3-DSP-Graphs}{SR-DSP-MK3-DSP-Graphs}
\GxsmScreenShot{SR-DSP-MK3-DSP-LockIn}{SR-DSP-MK3-DSP-LockIn}
\GxsmScreenShot{SR-DSP-MK3-DSP-PL}{SR-DSP-MK3-DSP-PL}
\GxsmScreenShot{SR-DSP-MK3-DSP-STS}{SR-DSP-MK3-DSP-STS}
\GxsmScreenShot{SR-DSP-MK3-DSP-Z}{SR-DSP-MK3-DSP-Z}
\GxsmScreenShot{SR-DSP-MK3-FBS-bias-log-configure}{SR-DSP-MK3-FBS-bias-log-configure}
\GxsmScreenShot{SR-DSP-MK3-FBS-configure}{SR-DSP-MK3-FBS-configure}
\GxsmScreenShot{SR-DSP-MK3-FBS-CZ-FUZZY-LOG-bias-log}{SR-DSP-MK3-FBS-CZ-FUZZY-LOG-bias-log}
\GxsmScreenShot{SR-DSP-MK3-FBS-simple}{SR-DSP-MK3-FBS-simple}
\GxsmScreenShot{SR-DSP-MK3-Graphs}{SR-DSP-MK3-Graphs}
\GxsmScreenShot{SR-DSP-MK3-LockIn-configure}{SR-DSP-MK3-LockIn-configure}
\GxsmScreenShot{SR-DSP-MK3-LockIn-simple}{SR-DSP-MK3-LockIn-simple}
\GxsmScreenShot{SR-DSP-MK3-Mover-config}{SR-DSP-MK3-Mover-config}
\GxsmScreenShot{SR-DSP-MK3-PAC-FB-Controllers}{SR-DSP-MK3-PAC-FB-Controllers}
\GxsmScreenShot{SR-DSP-MK3-PAC-Operation}{SR-DSP-MK3-PAC-Operation}
\GxsmScreenShot{SR-DSP-MK3-VP-configure}{SR-DSP-MK3-VP-configure}
\GxsmScreenShot{SR-DSP-MK3-VP-simple}{SR-DSP-MK3-VP-simple}



\clearpage

\begin{description}
\item[Duration] Determines the duration of the pulse.
\item[Slope] Slope to reach \GxsmEntry{Volts and dZ}.
\item[Volts] Applied voltage.
\item[dZ] Gap or Z change.


\item[Final Delay] Delay for relaxing the I/V-converter after pulsing.

\item[Repetitions] How many pulses are applied.

\item[Step,dZ] Option to shift up/down every consecutive puls (for multiple repetitions)

\item[Status] Gives information about the ongoing pulse. 
\end{description}

Informations about the check options can be found in STS.



% OptPlugInSubSection: Laser Pulse (LPC) user form

% OptPlugInSubSection: Slow Pulse (SP) user form

% OptPlugInSubSection: Time Spectrum (TS)

% OptPlugInSubSection: Tracking (TK) mode
\index{Signal Ranger MK2/3 Tracking}

Real time tracking of simple features like local maxima or minima in Topography (Z, feedback on), Current or other signal external sources on A810-IN1 or 2 (TK-ref option menu).
This modes generated a special vector tracking sequence moving the tip starting at current tip position in circle (TK-nodes polygon (TK-points are used as step size each segment, moving at TK-Speed) with radius TK-rad, if TK-rad2 is > 0 it will use a double circle testing pattern) like motion. While moving on this pattern it will recognize the extrema (min/max) and if the gradient relative to the tips initial origin is up or down (as specified via TK-mode) it will relocate the tip and start over TK-Reps times.


% OptPlugInSubSection: Control of the digital (DSP) Lock-In
\index{Signal Ranger MK2/3 LockIn}
The Lock-In folder provides all settings concerning the build in
digital Lock-In. The Lock-In and with it the bias modulation is
automatically turned on if any Lock-In data channel is requested,
either for probing/STS (in Graphs) or as a scan data source
(Channelselector) for imaging.

There are a total of five digital correlation sums computed:

Averaged Input Signal (LockIn0), 

Phase-A/1st order (LockIn1stA),
Phase-B/1st order (LockIn1stB),

Phase-A/2nd order (LockIn2ndA),
Phase-B/2nd order (LockIn2ndB).

\GxsmNote{Please always select LockIn0 for STS.}

\GxsmScreenShotW{SR-DSP-MK3-DSP-LockIn}{GXSM SR DSP Control window: Lock-In settings and enabled configuration mode.}{0.9\textwidth}

\begin{description}
\item[AC-Amplitude] The amplitude of the overlaid Lock-In AC voltage.
\item[AC-Frequency] The base frequency of the Lock-In. There are four fixed frequency choices.
\item[AC-Phase-AB] Phase for A and B signal, applied for both, 1st and 2nd order.
\item[AC-Avg-Cycles] This sets the length for averaging, i.e. the corresponding time-constant.
\end{description}

For adjustments purpose only are the following parameters and the execute function here,
not needed to run the Lock-In for all other modes. 
The special probe mode implemented in this section can actually sweep the phase of the Lock-In,
it is useful to figure out the correct phase to use:

\begin{description}
\item[span] Full phase span to sweep.
\item[pts] Number data points to acquire while phase sweep.
\item[slope] Phase ramp speed. 
\end{description}

The digital Lock-In is restricted to a fixed length of base period
(choices are 128, 64, 32, 16 samples/per period with a fixed sample
rate of 75000 samples/s) and a fixed number of 8 periods for computing
the correlation sum: The total number of periods used for correlation
of data can be increased by setting AC-Avg-Cycles greater than one,
then overlapping sections of the 8 period long base window is used for
building the correlation sum. Thus the total integration length (time constant ) is

\[ \tau = \frac{\text{AC-Ave-Cycels} \cdot 8}{\text{Frq}} \]
\[ \text{Frq} = \frac{75000 \:\text{Hz}}{M = 128, 64, 32, 16} \].

There for the following discrete frequencies are available: 585.9$\:$Hz, 1171.9$\:$Hz, 2343.7$\:$Hz, 4687.5$\:$Hz.


The four correlation sums for A/B/1st/2nd are always computed in
parallel on the DSP if the Lock-In is enabled -- regardless what data
is requested. The correlation length is given by:

\[ N = 128 \cdot \text{AC-Ave-Cycels} \cdot 8\]
\[ \omega = 2 \pi \cdot \text{Frq} \]

Lock-In data calculations and reference signal generation is all in digital regime on the DSP in real-time. 
The modulation is applied to the Bias voltage by default automatically only if the Lock-In is active:
\[ U_{\text{ref}} = \text{AC-Amp} \cdot \sin(\omega t) + \text{Bias}\]

Averaged signal and Lock-In output signals calculated:
\[ U_{\text{LockIn0}} = \sum_{i=0}^{N-1}{U_{in,i}} \]
\[ U_{\text{LockIn1stA}} = \frac{2 \pi}{N} \sum_{i=0}^{N-1}{\text{AC-Amp} \cdot U_{in,i} \cdot \sin(i \frac{2\pi}{M} + \text{Phase-A})  }    \]
\[ U_{\text{LockIn1stB}} = \frac{2 \pi}{N} \sum_{i=0}^{N-1}{\text{AC-Amp} \cdot U_{in,i} \cdot \sin(i \frac{2\pi}{M} + \text{Phase-B})  }    \]
\[ U_{\text{LockIn2ndA}} = \frac{2 \pi}{N} \sum_{i=0}^{N-1}{\text{AC-Amp} \cdot U_{in,i} \cdot \sin(2 i \frac{2\pi}{M} + \text{Phase-A})  }    \]
\[ U_{\text{LockIn2ndA}} = \frac{2 \pi}{N} \sum_{i=0}^{N-1}{\text{AC-Amp} \cdot U_{in,i} \cdot \sin(2 i \frac{2\pi}{M} + \text{Phase-B})  }    \]

\GxsmNote{Implemented in FB\_spm\_probe.c, run\_lockin() (C) by P.Zahl 2002-2007. }

\GxsmNote{All Lock-In data is raw summed in 32bit integer variables by the DSP, they are not normalized at this time and moved to \Gxsmx via FIFO.
\Gxsmx applies the normalization before plotting. }


Information about the check options can be found in STS.

\clearpage

% OptPlugInSubSection: Auxiliary Probe Control

\GxsmScreenShotW{SR-DSP-MK3-DSP-AX}{GXSM SR DSP Control window: AX (Auxiliary) settings}{0.9\textwidth}

This folder can be used for control and data acquisition from several
kind of simple instruments like a QMA or Auger/CMA.

\GxsmNote{Best is to setup a new user for this instrument and
  configure the Bias-Gain so the ``Voltage'' corresponds to what you
  need. As input you can select any channel, including Lock-In and
  Counter. Here the Gate-Time is used to auto set the V-slope to match
  V-range and points.}

\clearpage


% OptPlugInSubSection: Data Sources and Graphing Control
In the Graphs folder all available data channels are listed. If a
Source is activated, measured data will be transferred into the
buffer. Saving the buffer will automatically save all activated
sources.  Additionally it is possible to define a source as to be
displayed. 

\GxsmScreenShotW{SR-DSP-MK3-DSP-Graphs}{GXSM SR DSP Control window, Graphs page: Plot and Data sources setup.}{0.9\textwidth}

\GxsmHint{Beware: If a channel is not marked as a Source there will be no data
to be displayed even if X or Y is checked.}

\clearpage

% OptPlugInSection: SR-DSP Mover and Approach Control
\index{Signal Ranger MK2/3 Mover}
\index{Signal Ranger MK2/3 Auto Approach}
\index{Signal Ranger MK2/3 Coarse Motions}
\label{pi:A810-mover}
GXSM with the SRanger also provides signal generation for
slip-stick type slider/mover motions which are often used for coarse
positioning aud tip approach. Set
\GxsmPref{User}{User/SliderControlType} to \GxsmEntry{mover} to get
the most configurable Mover Control dialog. If set to
\GxsmEntry{slider} (default setting) the dialog will be simplified for
Z/approach only. The different tabs are only for users convenience to
store different speed/step values, the output will always occur as
configured on the \GxsmEntry{Config} folder.

\GxsmScreenShot{SR-DSP-MoverConfig}{SR-DSP-MoverConfig}

\GxsmScreenShot{SR-DSP-Mover-configure}{SR-DSP-Mover-configure}

\GxsmScreenShot{SR-DSP-Mover-XY-configure}{SR-DSP-Mover-XY-configure}

\GxsmScreenShot{SR-DSP-Mover-Z0-simple}{SR-DSP-Mover-Z0-simple}

 % \GxsmScreenShotDual{SR-DSP-Mover}{GXSM SR generic coarse mover controller}{SR-DSP-Mover-Auto}{Auto approach controller}

To configure the mover output signal type and channles select the ``Config'' tab. Select the ``Curve Mode'', normally a simple Sawtooth will do it.
Then select the output configuration meeting your needs best. The MK3 ``Signal Master'' allows fully custom
assignment of the generic ``X'' and ``Y'' mover actions to any available output channel. See below  Wave$[0,1]$ out on  and Fig.~\ref{fig:screenshot:SR-DSP-MK3-Mover-config}.

\GxsmScreenShot{SR-DSP-MK3-Mover-config}{GXSM SR DSP configuration of SRanger (MK2/3Pro) inertial mover driver wave generate engine.}

More complex wave forms are available such as:
\begin{description}
\item[Wave: Sawtooth] Just a simple linear ramp and then a jump back to the initial value.
\item[Wave: Sine] For testing and maybe some inch worm drive.
\item[Wave: Cyclo, Cyclo+, Cyclo-, Inv Cyclo+, Inv Cyclo-] A cycloidal wave function (see \\ http://dx.doi.org/10.1063/1.1141450) should provide an even more abrupt change of the motion direction than the jump of the sawtooth and therefore work with lower amplitudes (or at lower temperatures) better. Depending on the option choosen, the signal is just limited to positive or negative values.
\item[Wave: KOALA] Intendet to be used with a KOALA drive (see http://dx.doi.org/10.1063/1.3681444). This wave form will require signal at two output channels with a phase shift of $\pi$. The wave form is shown in Fig.~\ref{fig:SR-DSP-MK3-Mover-Koala-waveform}.
\GxsmGraphic{SR-DSP-MK3-Mover-Koala-waveform}{2}{Wave form used to operate a KOALA drive STM.}
\item[Wave: Besocke] Intendet to be used with a Besocke style STM (see https://doi.org/10.1016/0039-6028(87)90151-8): 3 piezos walk up and down a ramp. In this particular case, the piezos have three segments at their outer side (u, v, w). This coarse motion will require signals at three output channels. These signals vary for different directions of movement. The fundamental waveform is shown in Fig.~\ref{fig:SR-DSP-MK3-Mover-Besocke-waveform}. By an additional analog switch, which can be controlled by the GPIO ports, one can change between xy motion (translation) and z motion (rotation). The switch either routes to the equivalent segments of the piezo the same signal (rotation) or projections on the three segments according to the 120$^\circ$ rotation between the three piezos (translation).

The time delay between the points A and B and also between D and E is named `settling time t1', the time delay between B and C is `period of fall t2'. t2 should be shorter than the actual time for the scan head to fall back onto the ramp. If the z-jump corresponds to about $\Delta h= 1\,\mu$m, the period of fall can be estimated according to the uniformly accelerated fall to be $t2=\sqrt{\frac{\Delta h}{g}}\approx 0.44\,$ms. 

These delays are both variable in the interface. The slip-stick amplitude is given by the voltage difference between C to D. The amplitude of the z-jump is defined as relative ratio to this value.
\GxsmGraphic{SR-DSP-MK3-Mover-Besocke-waveform}{0.5}{Basic wave form used for Besocke drive STM.}
\item[Pulse: positive] Uses an analog channel to generate a simple on/off pulse similar to the GPIOs but you can controll the voltage range.
\end{description}

% OptPlugInSubSection: Autoapproach

Main idea of autoapproach is to extract and retract scanner with tip (and enabled feedback)
between each macro approach step to prevent destruction of the tip.

For correct use of wave outputs/coarse motion controls and autoapproach it is necessary to setup
the output configuration and if needed the GPIO ports, which may control macro approach motor.
In config tab is possible to set:

\begin{description}
\item[Wave(0,1) out on] select the actual physical output channels used to output Wave X/Y vector. The defaults are: OUTMIX8 to Ch3 (normally X-Scan) and 9 to Ch4 (normally Y-Scan).
You may alternatively also choose the pre set options below.
\item[GPIO Pon] Sets ports as logic 1 or 0 in point $e$ - at the beginning of each pulse (see Fig.~\ref{fig:SR-DSP-MK3-DSP-AutoApproach}).
\item[GPIO Poff] Sets ports in point $f$ -at the end of each pulse.
\item[GPIO Preset] Sets ports in point $g$ - at the end of each pulse set.
\item[GPIO XY-scan] Sets ports while scanning.
\item[GPIO tmp1,2] Sets ports while switching mode.
\item[GPIO direction] Sets ports as input or output pin. Logical 1 means output.
\item[GPIO delay] Sets time delay $d$.
\end{description}

\GxsmNote{All previous settings (except GPIO delay) must be set in hexadecimal format.}

Autoapproach is stopped, when measured signal reaches setpoint value. This value is set in SR DSP Control window (SCurrent value).

\GxsmGraphic{SR-DSP-MK3-DSP-AutoApproach}{0.5}{Autoapproach signals. $Z_s$ is scannig signal in Z axis.}

In all tabs there is possible to set number of pulses in each step (input field \GxsmEntry{Max. Steps}).
In the Fig.~\ref{fig:SR-DSP-MK3-DSP-AutoApproach}, it is marked with letter $a$. Length of each step ($c$) is determined by input field \GxsmEntry{Duration}.

%% OptPlugInSubSection: Stepper motor tab
%
%This tab is used for control macro approach, which can be realised by stepper motor. 
%Approach motor is controlled by pulses on GPIO ports (in the Fig.~\ref{fig:screenshot:stepperMotorPulses})
%
%There are used these three signals:
%
%\begin{description}
%\item[Enable] Turn on the stepper motor. If is set to 0, no current is flowing (it can rotate freely).
%\item[Direction] Sets direction of motor rotation. If is set to 1, it will rotate CCW.
%\item[Clock] Each pulse means one step.
%\end{description}
%
%\GxsmScreenShot{stepperMotorPulses}{Pulses used for control of stepper motor.}
%\GxsmScreenShot{SR-DSP-Mover-StepperMotor}{Stepper motor control tab.}
%It is also possible, besides the manual mode, use the automatic approach. 
%
%\GxsmHint{Beware:MK2-A810 operates with 3.3V logic!}
%
%\GxsmNote{For easy stepper motor control via MK2/MK3Pro-A810 use stepper motor controller L297.}
%
%\clearpage

% OptPlugInSection: SR-DSP Phase/Amplitude Convergent Detector (PAC) Control
\label{pi:A810-PAC}
GXSM with the SRanger MK3-Pro Phase/Amplitude Convergent Detector.... (PLL).


\clearpage

% PlugInUsage
Set the \GxsmPref{Hardware}{Card} to ''SRangerMK2:SPM''.

\GxsmNote{
The MK2 has to be programmed (Flashed) with the DSP software\\
\filename{/SRanger/TiCC-project-files/MK2-A810\_FB\_spmcontrol/FB\_spmcontrol.out}\\
using the manufacturer supplied tools before it can be used with \Gxsm.
Power cycle the MK2 after flashing and before plugging into the Linux system.
}

% OptPlugInSection: Setup -- Gxsm Configurations for the MK2-A810.
\label{A810-gxsm-pref}
Quick sample reference for the MK2-A810 specific \Gxsm preferences.
\GxsmScreenShotDual{A810-PrefHardware}{MK2-A810 Hardware selection and device path setup.}{A810-PrefInstSPM}{Instrument configuration}
\GxsmScreenShotDualN{A810-PrefDataAq-1}{Data Acquisition Channel configuration: Data Sources}{A810-PrefDataAq-2}{}
\GxsmScreenShotDualN{A810-PrefDataAq-3}{Data Acquisition Channel configuration, more.}{A810-PrefDataAq-4}{}
\clearpage

% OptPlugInSection: Sample STS IV,dIdV
\label{A810-sts-IV-demo}
Quick screen shot for a STS setup for taking IV and dIdV data at a point (current ti position).
Example taken with home build Besocke style STM at room temperature, real data here!
\GxsmScreenShotDual{A810-DSP-Control-STS-demo-IVdIdV}{STS Setup for IV and dIdV}{A810-DSP-Control-STS-LCK-demo-IVdIdV}{Lock-In configuration used}
\GxsmScreenShotDual{A810-DSP-Control-STS-Graphs-demo-IVdIdV}{Data channel selection and graphing configuration for this STS probe}{A810-DSP-Control-STS-demo-IVdIdV-data}{Plot of the resulting data taking.}
\clearpage

% OptPlugInSection: Sample STS along line demo setup
\label{A810-sts-demo}
Quick screen shot for a STS setup along a line. Gxsm in self test configuration.
\GxsmScreenShotDual{A810-DSP-Control-STS-demo}{STS Setup}{A810-DSP-Control-STS-demo-graph}{Plotted Graphs}
\GxsmScreenShot{A810-DSP-Control-STS-demo-plot}{Scan with markers}
\clearpage


% OptPlugInSection: Signal Ranger MK3-Pro-A810/PLL Additional Features Guide
\index{Signal Ranger MK3-Pro/PLL}
\index{Signal Ranger MK3}
\label{SR-Mark3}

% OptPlugInSubSection: Introduction to a Open Signal Scheme

Adding more real time flexibility and the need to access even
more data channels/signals -- to manage a huge expansion of the ``signal
space''. See also the development background story about all this up front
in the \Gxsm\ preface \ref{pre-signalrevolution}.

Signals on the DSP are simply speaking just variables. Therefore, 
it is easy to maniplate them by connecting them to different 
source channels (i.e. ADC0-ADC7) or output channels
(i.e. DAC0-DAC7). In this view, they are ``hot plug-able'' like a 
physical wire in your rack. They can be pulled from one 
connector and plug into another. But be aware, you have to know
what you do. Secure the tip before trying new connections! Got me?

\GxsmScreenShotS{signal-graph}{Signal Graph for typical STM operation, as created by the tool and graphviz from the actual life DSP configuration.}{0.13}

A typical STM configuration will look like the mostly default
configuration as shown in Fig.~\ref{fig:screenshot:signal-graph}. This life signal
visualization is provided via the configuration application. Use the
xdot viewer (the configuration application attempts to launch xdot for
you automatically, if not present nothing will happen and only a
signal\_graph.dot and .svg output files are created.  of better
viewing, zooming and signal highlighting.

\GxsmWarning{Please acknowledge and understand this message as you
work with the ``Signal Monitor'' and ``Patch Rack'' tools: Due to
the nature of a parallel execution of programs or multitasking on
the host side and the graceful management of multiple processes
talking to the DSP (MK3-Pro here) at the same time there can be
``race'' conditions occur under certain circumstances accessing the
same DSP resources from two or more processes at the same
time. While it's guaranteed every process is receiving one completed
data read/write access package of any size, so it can not be
guaranteed that per process consecutive requests are consecutive for
the DSP as any other processes request may be served at any time in
between. This poses normally and per design no problem as long as
different processes request/operate on independent DSP resources or
tasks.\\ Now you must understand in this special case of the signal
management the actual signal configuration of any signal can be
requested and in a consecutive request read back as result.  Both,
\Gxsmx and the SPM configuration script are making use of this at
times and it's as of now the users responsibility to not run the
script's ``Signal Monitor'' or ``Patch Rack'' at the same time as
\Gxsmx will make signal configuration read back requests. You must
terminate (or at least temporary stop) any other signal requesting
script/process at the following times when \Gxsmx makes signal read
back requests: When \Gxsmx start up, Scan Start and Probe
Start/Graphs Plot Refreshes. In general don't run any not necessary
scripts, but for sure you may want to run the script to make
adjustments at initial system configuration/setup or tests or just
to monitor signals via the Recorder (Signal Oscilloscope) or run a
frequency/PLL tune -- this all is perfectly fine. It is not
sufficient to close the ``Patch Rack'' or ``Monitor'' Window as this
is only hiding the window, not destroying the tasks behind it. The
``background task'' requesting signals are installed at 1st time
opening the window for the ``Monitor'' or ``Patch Rack''. In future
this problem may be controlled via spin-locks, but as of now this is
a not essential overhead of USB communications.}

\clearpage

% OptPlugInSubSection: Getting Started
Prerequisites: Having build and installed the latest code including a
updated DSP MK3. Locate the configuration application (python script)
in the \Gxsmx source code tree. Also you may want to have the dot file
viewer ``xdot'' installed for convenient automatic viewing of the
signal configuration with important signal highlighting ability. As
those graphs may have many possibly overlapping signals.



% OptPlugInSubSection: Signal Monitoring and Configuration
\index{Signal Ranger MK3 Signals}
The python application suite shown in Fig.~\ref{fig:screenshot:MK3-spm-configurator-apps}

\filename{plug-ins/hard/MK3-A810\_spmcontrol/python\_scripts/mk3\_spm\_configurator.py}

provides monitoring and configuration tools including the PLL tune
application and step response test tools not directly provided via the
\Gxsmx GUI itself.

\GxsmScreenShotS{MK3-spm-configurator-apps}{SPM Configurator Python Apps}{0.2}

The actual readings of the two banks of each 8 AD and DA converters
provided by the A810 expansion board can be monitored via the A810
AD/DA Monitor App as shown in Fig.~\ref{fig:screenshot:MK3-spm-configurator-A810-mon}.
\GxsmScreenShotS{MK3-spm-configurator-A810-mon}{A810 AD/DA
Monitor}{0.2}

A subset of all signals can be configured via the Signal-Monitor for
watching. In general for testing and debugging any signal can be
selected at any monitor slot as shown in
Fig.~\ref{fig:screenshot:MK3-spm-configurator-2-signal-mon}. But for normal operation
\Gxsmx and als the SmartPiezoDrive (see \ref{sec:SPD}) are
automatically assigning Signal for efficient monitoring use and
without in depth knowledge those shall not be altered after \Gxsmx
start up.  

{
\tiny
\ctable[
caption={MK3 DSP Signal Description},
label={tab:DSPsignals},
botcap, % caption below table
sideways % This rotates the table
]{|l|r|l|l|c|p{3cm}|l|p{5cm}|}{}{
\hline
Type & Dim & DSP Variable & Signal Name & Unit & Conversion Factor & Module & Signal Description \\ \hline \hline
SI32 & 1 & analog.in[0] &  In 0 &  V &  DSP32Qs15dot16TO\_Volt &  Analog\_IN &  ADC INPUT1 \\
SI32 & 1 & analog.in[1] &  In 1 &  V &  DSP32Qs15dot16TO\_Volt &  Analog\_IN &  ADC INPUT2 \\
SI32 & 1 & analog.in[2] &  In 2 &  V &  DSP32Qs15dot16TO\_Volt &  Analog\_IN &  ADC INPUT3 \\
SI32 & 1 & analog.in[3] &  In 3 &  V &  DSP32Qs15dot16TO\_Volt &  Analog\_IN &  ADC INPUT4 \\
SI32 & 1 & analog.in[4] &  In 4 &  V &  DSP32Qs15dot16TO\_Volt &  Analog\_IN &  ADC INPUT5 \\
SI32 & 1 & analog.in[5] &  In 5 &  V &  DSP32Qs15dot16TO\_Volt &  Analog\_IN &  ADC INPUT6 \\
SI32 & 1 & analog.in[6] &  In 6 &  V &  DSP32Qs15dot16TO\_Volt &  Analog\_IN &  ADC INPUT7 \\
SI32 & 1 & analog.in[7] &  In 7 &  V &  DSP32Qs15dot16TO\_Volt &  Analog\_IN &  ADC INPUT8 \\
SI32 & 1 & analog.counter[0] &  Counter 0 &  CNT &  1 &  Counter &  FPGA based Counter Channel 1 \\
SI32 & 1 & analog.counter[1] &  Counter 1 &  CNT &  1 &  Counter &  FPGA based Counter Channel 2 \\
SI32 & 1 & probe.LockIn\_0A &      LockIn A-0 &    *V &    DSP32Qs15dot16TO\_Volt &  LockIn &  LockIn A 0 (average over full periods) \\
SI32 & 1 & probe.LockIn\_1stA &    LockIn A-1st &  *dV &   DSP32Qs15dot16TO\_Volt &  LockIn &  LockIn A 1st order \\
SI32 & 1 & probe.LockIn\_2ndA &    LockIn A-2nd &  *ddV &  DSP32Qs15dot16TO\_Volt &  LockIn &  LockIn A 2nd oder \\
SI32 & 1 & probe.LockIn\_0B &      LockIn B-0 &    *V &    DSP32Qs15dot16TO\_Volt &  LockIn &  LockIn B 0 (average over full periods) \\
SI32 & 1 & probe.LockIn\_1stB &    LockIn B-1st &  *dV &   DSP32Qs15dot16TO\_Volt &  LockIn &  LockIn B 1st order \\
SI32 & 1 & probe.LockIn\_2ndB &    LockIn B-2nd &  *ddV &  DSP32Qs15dot16TO\_Volt &  LockIn &  LockIn B 2nd order \\
SI32 & 1 & probe.LockIn\_ref &     LockIn Ref &     V &    DSP32Qs15dot0TO\_Volt &   LockIn &  LockIn Reference Sinewave (Modulation) (Internal Reference Signal) \\
SI32 & 1 & probe.PRB\_ACPhaseA32 & LockIn Phase A & deg &  180./(2913*CPN(16)) &    LockIn &  DSP internal LockIn PhaseA32 watch \\
SI32 & 1 & InputFiltered &   PLL Res Out &      V &                   10./CPN(22) &  PAC &  Resonator Output Signal \\
SI32 & 1 & SineOut0 &        PLL Exci Signal &  V &                   10./CPN(22) &  PAC &  Excitation Signal \\
SI32 & 1 & phase &           PLL Res Ph &      deg &          180./(CPN(22)*M\_PI) &  PAC &  Resonator Phase (no LP filter) \\
SI32 & 1 & PI\_Phase\_Out &    PLL Exci Frq &     Hz &  (150000./(CPN(29)*2.*M\_PI)) &  PAC &  Excitation Freq. (no LP filter) \\
SI32 & 1 & amp\_estimation &  PLL Res Amp &      V &                   10./CPN(22) &  PAC &  Resonator Amp. (no LP filter) \\
SI32 & 1 & volumeSine &      PLL Exci Amp &     V &                   10./CPN(22) &  PAC &  Excitation Amp. (no LP filter) \\
SI32 & 1 & Filter64Out[0] &  PLL Exci Frq LP &  Hz &  (150000./(CPN(29)*2.*M\_PI)) &  PAC &  Excitation Freq. (with LP filter) \\
SI32 & 1 & Filter64Out[1] &  PLL Res Ph LP &   deg &        (180./(CPN(29)*M\_PI)) &  PAC &  Resonator Phase (with LP filter) \\
SI32 & 1 & Filter64Out[2] &  PLL Res Amp LP &   V &               (10./(CPN(29))) &  PAC &  Resonator Ampl. (with LP filter) \\
SI32 & 1 & Filter64Out[3] &  PLL Exci Amp LP &  V &               (10./(CPN(29))) &  PAC &  Excitation Ampl. (with LP filter) \\
SI32 & 1 & feedback\_mixer.FB\_IN\_processed[0] &  MIX IN 0 &  V &  DSP32Qs23dot8TO\_Volt &  Mixer &  Mixer Channel 0 processed input signal \\
SI32 & 1 & feedback\_mixer.FB\_IN\_processed[1] &  MIX IN 1 &  V &  DSP32Qs23dot8TO\_Volt &  Mixer &  Mixer Channel 1 processed input signal \\
SI32 & 1 & feedback\_mixer.FB\_IN\_processed[2] &  MIX IN 2 &  V &  DSP32Qs23dot8TO\_Volt &  Mixer &  Mixer Channel 2 processed input signal \\
SI32 & 1 & feedback\_mixer.FB\_IN\_processed[3] &  MIX IN 3 &  V &  DSP32Qs23dot8TO\_Volt &  Mixer &  Mixer Channel 3 processed input signal \\
SI32 & 1 & feedback\_mixer.channel[0] &  MIX OUT 0 &  V &  DSP32Qs23dot8TO\_Volt &  Mixer &  Mixer Channel 0 output signal \\
SI32 & 1 & feedback\_mixer.channel[1] &  MIX OUT 1 &  V &  DSP32Qs23dot8TO\_Volt &  Mixer &  Mixer Channel 1 output signal \\
SI32 & 1 & feedback\_mixer.channel[2] &  MIX OUT 2 &  V &  DSP32Qs23dot8TO\_Volt &  Mixer &  Mixer Channel 2 output signal \\
SI32 & 1 & feedback\_mixer.channel[3] &  MIX OUT 3 &  V &  DSP32Qs23dot8TO\_Volt &  Mixer &  Mixer Channel 3 output signal \\
SI32 & 1 & feedback\_mixer.delta &  MIX out delta &       V &  DSP32Qs23dot8TO\_Volt &  Mixer &  Mixer Processed Summed Error Signal (Delta) (Z-Servo Input normally) \\
SI32 & 1 & feedback\_mixer.q\_factor15 & MIX0 qfac15 LP &   Q &        (1/(CPN(15))) &  Mixer &  Mixer Channel 0 actuall life IIR cutoff watch: q LP fg; f in Hz via: (-log (qf15 / 32767.) / (2.*M\_PI/75000.)) \\
SI32 & 1 & analog.avg\_signal &   signal AVG-256 &  V &    DSP32Qs15dot0TO\_Volt/256. &  RMS &  Averaged signal from Analog AVG module \\
SI32 & 1 & analog.rms\_signal &   signal RMS-256 &  V$^2$ & (DSP32Qs15dot0TO\_Volt *DSP32Qs15dot0TO\_Volt/256.) &  RMS &  RMS signal from Analog AVG module \\
SI32 & 1 & z\_servo.control &     Z Servo &       V &  DSP32Qs15dot16TO\_Volt &  Z\_Servo &  Z-Servo output \\
SI32 & 1 & z\_servo.neg\_control & Z Servo Neg &   V &  DSP32Qs15dot16TO\_Volt &  Z\_Servo &  -Z-Servo output \\
SI32 & 1 & z\_servo.watch &       Z Servo Watch &  B &                   1 &  Z\_Servo &  Z-Servo status (boolean) \\
SI32 & 1 & m\_servo.control &     M Servo &       V &  DSP32Qs15dot16TO\_Volt &  M\_Servo &  M-Servo output \\
SI32 & 1 & m\_servo.neg\_control & M Servo Neg &   V &  DSP32Qs15dot16TO\_Volt &  M\_Servo &  -M-Servo output \\
SI32 & 1 & m\_servo.watch &       M Servo Watch &  B &                   1 &  M\_Servo &  M-Servo statuis (boolean) \\
\hline
}}

{
\tiny
\ctable[
caption={MK3 DSP Signal Description continued},
label={tab:DSPsignalscont},
botcap, % caption below table
sideways % This rotates the table
]{|l|r|l|l|c|p{3cm}|l|p{5cm}|}{}{
\hline
Type & Dim & DSP Variable & Signal Name & Unit & Conversion Factor & Module & Signal Description \\ \hline \hline
SI32 & 1 & probe.Upos &  VP Bias &   V &  DSP32Qs15dot16TO\_Volt &  VP &  Bias after VP manipulations \\
SI32 & 1 & probe.Zpos &  VP Z pos &  V &  DSP32Qs15dot16TO\_Volt &  VP &  temp Z offset generated by VP program \\
SI32 & 1 & scan.xyz\_vec[i\_X] &  X Scan &            V &  DSP32Qs15dot16TO\_Volt &  Scan &  Scan generator: X-Scan signal \\
SI32 & 1 & scan.xyz\_vec[i\_Y] &  Y Scan &            V &  DSP32Qs15dot16TO\_Volt &  Scan &  Scan generator: Y-Scan signal \\
SI32 & 1 & scan.xyz\_vec[i\_Z] &  Z Scan &            V &  DSP32Qs15dot16TO\_Volt &  Scan &  Scan generator: Z-Scan signal (**unused) \\
SI32 & 1 & scan.xy\_r\_vec[i\_X] &  X Scan Rot &       V &  DSP32Qs15dot16TO\_Volt &  Scan &  final X-Scan signal in rotated coordinates \\
SI32 & 1 & scan.xy\_r\_vec[i\_Y] &  Y Scan Rot &       V &  DSP32Qs15dot16TO\_Volt &  Scan &  final Y-Scan signal in rotated coordinates \\
SI32 & 1 & scan.z\_offset\_xyslope &  Z Offset from XY Slope &       V &  DSP32Qs15dot16TO\_Volt &  Scan &  Scan generator: Z-offset generated by slop compensation calculation (integrative) \\
SI32 & 1 & scan.xyz\_gain &      XYZ Scan Gain &     X &                      1 &  Scan &  XYZ Scan Gains: bitcoded -/8/8/8 (0..255)x -- 10x all: 0x000a0a0a \\
SI32 & 1 & move.xyz\_vec[i\_X] &  X Offset &          V &  DSP32Qs15dot16TO\_Volt &  Scan &  Offset Move generator: X-Offset signal \\
SI32 & 1 & move.xyz\_vec[i\_Y] &  Y Offset &          V &  DSP32Qs15dot16TO\_Volt &  Scan &  Offset Move generator: Y-Offset signal \\
SI32 & 1 & move.xyz\_vec[i\_Z] &  Z Offset &          V &  DSP32Qs15dot16TO\_Volt &  Scan &  Offset Move generator: Z-Offset signal \\
SI32 & 1 & move.xyz\_gain &      XYZ Offset Gains &  X &                      1 &  Scan &  XYZ Offset Gains: bitcoded -/8/8/8 (0..255)x -- not yet used and fixed set to 10x (0x000a0a0a) \\
SI32 & 1 & analog.wave[0] &  Wave X &  V &  DSP32Qs15dot0TO\_Volt &  Coarse &  Wave generator: Wave-X (coarse motions) \\
SI32 & 1 & analog.wave[1] &  Wave Y &  V &  DSP32Qs15dot0TO\_Volt &  Coarse &  Wave generator: Wave-Y (coarse motions) \\
SI32 & 1 & autoapp.count\_axis[0] &  Count Axis 0 &  1 &      1 &  Coarse &  Coarse Step Counter Axis 0 (X) \\
SI32 & 1 & autoapp.count\_axis[1] &  Count Axis 1 &  1 &      1 &  Coarse &  Coarse Step Counter Axis 1 (Y) \\
SI32 & 1 & autoapp.count\_axis[2] &  Count Axis 2 &  1 &      1 &  Coarse &  Coarse Step Counter Axis 2 (Z) \\
SI32 & 1 & analog.bias &         Bias &         V &  DSP32Qs15dot16TO\_Volt &  Control &  DSP Bias Voltage reference following smoothly the Bias Adjuster \\
SI32 & 1 & analog.bias\_adjust &  Bias Adjust &  V &  DSP32Qs15dot16TO\_Volt &  Control &  Bias Adjuster (Bias Voltage) setpoint given by user interface \\
SI32 & 1 & analog.motor &        Motor &        V &  DSP32Qs15dot16TO\_Volt &  Control &  Motor Voltage (auxillary output shared with PLL excitiation if PAC processing is enabled!) \\
SI32 & 1 & analog.noise &        Noise &         1 &                   1 &  Control &  White Noise Generator \\
SI32 & 1 & analog.vnull &        Null-Signal &   0 &                   1 &  Control &  Null Signal used to fix any module input at Zero \\
SI32 & 1 & probe.AC\_amp &        AC Ampl &      V &   DSP32Qs15dot0TO\_Volt &  Control &  AC Amplitude Control for Bias modulation \\
SI32 & 1 & probe.AC\_amp\_aux &    AC Ampl Aux &  V &   DSP32Qs15dot0TO\_Volt &  Control &  AC Amplitude Control (auxillary channel or as default used for Z modulation) \\
SI32 & 1 & probe.noise\_amp &     Noise Ampl &   V &   DSP32Qs15dot0TO\_Volt &  Control &  Noise Amplitiude Control \\
SI32 & 1 & probe.state &         LockIn State &   X &                  1 &  Control &  LockIn Status watch \\
SI32 & 1 & state.DSP\_time &  TIME TICKS &  s &  1./150000. &  DSP &  DSP TIME TICKS: real time DSP time based on 150kHz data processing loop for one tick. 32bit free running \\
SI32 & 1 & feedback\_mixer.iir\_signal[0] &  IIR32 0 &  V &  DSP32Qs15dot16TO\_Volt &  DBGX\_Mixer &  Mixer processed input tap 0 32bit \\
SI32 & 1 & feedback\_mixer.iir\_signal[1] &  IIR32 1 &  V &  DSP32Qs15dot16TO\_Volt &  DBGX\_Mixer &  Mixer processed input tap 1 32bit \\
SI32 & 1 & feedback\_mixer.iir\_signal[2] &  IIR32 2 &  V &  DSP32Qs15dot16TO\_Volt &  DBGX\_Mixer &  Mixer processed input tap 2 32bit \\
SI32 & 1 & feedback\_mixer.iir\_signal[3] &  IIR32 3 &  V &  DSP32Qs15dot16TO\_Volt &  DBGX\_Mixer &  Mixer processed input tap 3 32bit \\
\hline
}}

{
\tiny
\ctable[
caption={ MK3 DSP Signal Description continued. With \newline
{\small DSP32Qs15dot0TO\_Volt } := {$10/32767$}, \newline
{\small DSP32Qs23dot8TO\_Volt } := {$10/(32767 * 2^8)$}, \newline
{\small DSP32Qs15dot16TO\_Volt} := {$10/(32767 * 2^{16})$}\ and \newline
{\small CPN(N) } := $2^N-1$
},
label={tab:DSPsignalscontcont},
botcap, % caption below table
sideways % This rotates the table
]{|l|r|l|l|c|p{3cm}|l|p{5cm}|}{}{
\hline
Type & Dim & DSP Variable & Signal Name & Unit & Conversion Factor & Module & Signal Description \\ \hline \hline
SI32 & 1 & analog.out[0].s &  Out 0 &  V &  DSP32Qs15dot16TO\_Volt &  Analog\_OUT &  DAC OUTPUT 1 \\
SI32 & 1 & analog.out[1].s &  Out 1 &  V &  DSP32Qs15dot16TO\_Volt &  Analog\_OUT &  DAC OUTPUT 2 \\
SI32 & 1 & analog.out[2].s &  Out 2 &  V &  DSP32Qs15dot16TO\_Volt &  Analog\_OUT &  DAC OUTPUT 3 \\
SI32 & 1 & analog.out[3].s &  Out 3 &  V &  DSP32Qs15dot16TO\_Volt &  Analog\_OUT &  DAC OUTPUT 4 \\
SI32 & 1 & analog.out[4].s &  Out 4 &  V &  DSP32Qs15dot16TO\_Volt &  Analog\_OUT &  DAC OUTPUT 5 \\
SI32 & 1 & analog.out[5].s &  Out 5 &  V &  DSP32Qs15dot16TO\_Volt &  Analog\_OUT &  DAC OUTPUT 6 \\
SI32 & 1 & analog.out[6].s &  Out 6 &  V &  DSP32Qs15dot16TO\_Volt &  Analog\_OUT &  DAC OUTPUT 7 \\
SI32 & 1 & analog.out[7].s &  Out 7 &  V &  DSP32Qs15dot16TO\_Volt &  Analog\_OUT &  DAC OUTPUT 8 \\
SI32 & 1 & analog.out[8].s &  Wave Out 8 &  V &  DSP32Qs15dot0TO\_Volt &  Analog\_OUT &  VIRTUAL OUTPUT 8 (Wave X default) \\
SI32 & 1 & analog.out[9].s &  Wave Out 9 &  V &  DSP32Qs15dot0TO\_Volt &  Analog\_OUT &  VIRTUAL OUTPUT 9 (Wave Y default) \\
SI32 & 1 & state.mode &          State mode &       X &  1 &  Process\_Control &  DSP statmachine status \\
SI32 & 1 & move.pflg &           Move pflag &       X &  1 &  Process\_Control &  Offset Move generator process flag \\
SI32 & 1 & scan.pflg &           Scan pflag &       X &  1 &  Process\_Control &  Scan generator process flag \\
SI32 & 1 & probe.pflg &          Probe pflag &      X &  1 &  Process\_Control &  Vector Probe (VP) process flag \\
SI32 & 1 & autoapp.pflg &        AutoApp pflag &    X &  1 &  Process\_Control &  Auto Approach process flag \\
SI32 & 1 & CR\_generic\_io.pflg &  GenericIO pflag &  X &  1 &  Process\_Control &  Generic IO process flag \\
SI32 & 1 & CR\_out\_pulse.pflg &   IO Pulse pflag &   X &  1 &  Process\_Control &  IO pulse generator process flag \\
SI32 & 1 & probe.gpio\_data &     GPIO data &        X &  1 &  Process\_Control &  GPIO data-in is read via VP if GPIO READ option is enabled \\
VI32 & 64 &  VP\_sec\_end\_buffer[0] &  "VP SecV" &   "xV" &  DSP32Qs15dot16TO\_Volt &  "VP" &  "VP section data tranfer buffer vector [64 = 8X Sec + 8CH matrix] \\
\hline
}
}


{\small
\setcounter{LTchunksize}{25}
\begin{center}
\begin{longtable}{|lr|l|l|}
\caption[Module Inputs]{Module Input Definitons.} \\
%Input name and Block Start ID && Type & Input Description \\ \hline  
\endfirsthead
\endhead
\hline \multicolumn{4}{|r|}{{Continued on next page}} \\ \hline
\endfoot
\hline \hline
\endlastfoot
%
\hline Input name and Block Start ID && Type & Input Description \\ \hline  \hline
\hline \multicolumn{2}{|l|}{{DSP\_SIGNAL\_BASE\_BLOCK\_A\_ID = 0x1000}} & \multicolumn{2}{|l|}{Servo, Mixer, Inputstage, Scanmap}\\ \hline
DSP\_SIGNAL\_Z\_SERVO\_INPUT\_ID & & SI32 & Z Servo Delta \\ \hline
DSP\_SIGNAL\_M\_SERVO\_INPUT\_ID & & SI32 & Motor Servo Delta \\ \hline
DSP\_SIGNAL\_MIXER0\_INPUT\_ID  & & SI32 & Mixer CH 0 \\ \hline
DSP\_SIGNAL\_MIXER1\_INPUT\_ID  & & SI32 & Mixer CH 1 \\ \hline
DSP\_SIGNAL\_MIXER2\_INPUT\_ID  & & SI32 & Mixer CH 2 \\ \hline
DSP\_SIGNAL\_MIXER3\_INPUT\_ID  & & SI32 & Mixer CH 3 \\ \hline
DSP\_SIGNAL\_DIFF\_IN0\_ID & & SI32 & Differential IN 0 \\ \hline
DSP\_SIGNAL\_DIFF\_IN1\_ID & & SI32 & Differential IN 1 \\ \hline
DSP\_SIGNAL\_DIFF\_IN2\_ID & & SI32 & Differential IN 2 \\ \hline
DSP\_SIGNAL\_DIFF\_IN3\_ID & & SI32 & Differential IN 3 \\ \hline
DSP\_SIGNAL\_SCAN\_CHANNEL\_MAP0\_ID & & SI32 & Scan Channel Map 0 \\ \hline
DSP\_SIGNAL\_SCAN\_CHANNEL\_MAP1\_ID & & SI32 & Scan Channel Map 1 \\ \hline
DSP\_SIGNAL\_SCAN\_CHANNEL\_MAP2\_ID & & SI32 & Scan Channel Map 2 \\ \hline
DSP\_SIGNAL\_SCAN\_CHANNEL\_MAP3\_ID & & SI32 & Scan Channel Map 3 \\ \hline
%
\hline \multicolumn{2}{|l|}{{DSP\_SIGNAL\_BASE\_BLOCK\_B\_ID = 0x2000}} &  \multicolumn{2}{|l|}{Vector Probe/LockIn Module} \\ \hline
DSP\_SIGNAL\_LOCKIN\_A\_INPUT\_ID & & SI32 & Lock-In Ch-A \\ \hline
DSP\_SIGNAL\_LOCKIN\_B\_INPUT\_ID & & SI32 & Lock-In Ch-B \\ \hline
DSP\_SIGNAL\_VECPROBE0\_INPUT\_ID & & SI32 & VP Source Signal, 32bit, no oversampling \\ \hline
DSP\_SIGNAL\_VECPROBE1\_INPUT\_ID & & SI32 & VP Source Signal, 32bit, no oversampling \\ \hline
DSP\_SIGNAL\_VECPROBE2\_INPUT\_ID & & SI32 & VP Source Signal, 32bit, no oversampling \\ \hline
DSP\_SIGNAL\_VECPROBE3\_INPUT\_ID & & SI32 & VP Source Signal, 32bit, no oversampling \\ \hline
DSP\_SIGNAL\_VECPROBE0\_CONTROL\_ID & & SI32 & VP Control Signal (modified by VP) \\ \hline
DSP\_SIGNAL\_VECPROBE1\_CONTROL\_ID & & SI32 & VP Control Signal (modified by VP) \\ \hline
DSP\_SIGNAL\_VECPROBE2\_CONTROL\_ID & & SI32 & VP Control Signal (modified by VP) \\ \hline
DSP\_SIGNAL\_VECPROBE3\_CONTROL\_ID & & SI32 & VP Control Signal (modified by VP) \\ \hline
DSP\_SIGNAL\_VECPROBE\_TRIGGER\_SIGNAL\_ID & & SI32 & VP Trigger Control Signal Input \\ \hline
%
\hline \multicolumn{2}{|l|}{{DSP\_SIGNAL\_BASE\_BLOCK\_C\_ID = 0x3000}} &  \multicolumn{2}{|l|}{Output Signal Mixer Stage 8x + 2 Virtual} \\ \hline
DSP\_SIGNAL\_OUTMIX\_CH0\dots7\_INPUT\_ID & & SI32 & OUTPUT CH0\dots7 Source \\ \hline
DSP\_SIGNAL\_OUTMIX\_CH0\dots7\_ADD\_A\_INPUT\_ID & & SI32 & A Signal to add \\ \hline
DSP\_SIGNAL\_OUTMIX\_CH0\dots7\_SUB\_B\_INPUT\_ID & & SI32 & B Signal to subtract \\ \hline
DSP\_SIGNAL\_OUTMIX\_CH0\dots7\_SMAC\_A\_INPUT\_ID & & SI32 & if set: gain for A Signal \\ \hline
DSP\_SIGNAL\_OUTMIX\_CH0\dots7\_SMAC\_B\_INPUT\_ID & & SI32 & if set: gain for B Signal to add \\ \hline \hline
%
DSP\_SIGNAL\_OUTMIX\_CH8\_INPUT\_ID & & SI32 & Coarse Wave Signal selection \\ \hline
DSP\_SIGNAL\_OUTMIX\_CH8\_ADD\_A\_INPUT\_ID & & SI32 & Signal to add \\ \hline \hline
DSP\_SIGNAL\_OUTMIX\_CH9\_INPUT\_ID & & SI32 & Coarse Wave Signal selection \\ \hline
DSP\_SIGNAL\_OUTMIX\_CH9\_ADD\_A\_INPUT\_ID & & SI32 & Signal to add \\ \hline
%
\hline \multicolumn{2}{|l|}{{DSP\_SIGNAL\_BASE\_BLOCK\_D\_ID = 0x4000}} &  \multicolumn{2}{|l|}{Recorder and AVG/RMS Modules} \\ \hline
DSP\_SIGNAL\_ANALOG\_AVG\_INPUT\_ID & & SI32 & Source for AVG/RMS Module \\ \hline
DSP\_SIGNAL\_SCOPE\_SIGNAL1\_INPUT\_ID & & SI32 & Recorder/Scope Input 1 \\ \hline
DSP\_SIGNAL\_SCOPE\_SIGNAL2\_INPUT\_ID & & SI32 & Recorder/Scopt Input 2 \\ \hline
%
\end{longtable}
\end{center}
}


\GxsmWarning{Reassigning Monitoring signals while normal Gxsm
operation may result in wrong signal reading in the PanView and may
corrupt the Gxsm-SmartPiezoDrive Link.}

The Signal Monitor also allows to visualize Signals in a Galvanometer
style for a better visual on changing readings as shown in Figs.~
\ref{fig:screenshot:MK3-spm-configurator-2-signal-mon-meter} and
\ref{fig:screenshot:MK3-spm-configurator-2-signal-mon-meter-rangefix}.

\GxsmNote{The more gizmos you watch via Galvos or the scope, the more
CPU time and USB bandwidth is utilized due to rapid refresh rates.
This is normally no problem unless a high data throughput is requested
for scanning or spectroscopy. It is advisable for normal operation
to minimize or best not run any of the configurator scripts while
taking data.}

\GxsmScreenShotS{MK3-spm-configurator-2-signal-mon}{Signal Monitor}{0.2}

\GxsmScreenShotS{MK3-spm-configurator-2-signal-mon-meter}{Signal Monitor with Meter}{0.2}

\GxsmScreenShotS{MK3-spm-configurator-2-signal-mon-meter-rangefix}{Signal Monitor with Meter and full scale range override}{0.2}

\clearpage



% OptPlugInSubSection: Signal Patching / Configuration

To configure your own very custom SPM you may completely reroute any
of the ``red'' signals (or make use of generic unutilized signals and
inputs).  For this purpose use the Patch Rack application shown in
Figs.~\ref{fig:screenshot:MK3-spm-configurator-2-signal-patchrack1},
\ref{fig:screenshot:MK3-spm-configurator-2-signal-patchrack2}, and
\ref{fig:screenshot:MK3-spm-configurator-2-signal-patchrack3}.

Here you are the master and have to make sure thinks add up right. The
Signal Monitor will be of great use to verify the functions.

\GxsmNote{Note to author: Need to add notes on signal precisions, etc.}

\GxsmScreenShotS{MK3-spm-configurator-2-signal-patchrack1}{Signal Patchrack}{0.2}

\GxsmScreenShotS{MK3-spm-configurator-2-signal-patchrack2}{Signal Patchrack}{0.2}

\GxsmScreenShotS{MK3-spm-configurator-2-signal-patchrack3}{Signal Patchrack}{0.2}

When satisfied with the signal configuration the Signal Manager
application (Fig.~\ref{fig:screenshot:MK3-spm-configurator-2-signal-manager})
allows to store the current life configuration to the MK3's FLASH
memory. It will be automatically reloaded on any later DSP power-up.
However, DSP software upgrades may include a modified signal table, in
this case always the ``Gxsm-MK2-like'' build in default initialization
will be loaded and the (old) FLASH configuration will be ignored as
the configuration table includes a signal revision control mechanism
to prevent future problems with new or eventually non existent
signals.  The ``Gxsm-MK2-like'' default configuration can be reloaded
at any time:
\begin{description}
\item[REVERT TO POWER UP DEFAULTS] This actually loads the pre
defined ``Gxsm-MK2-like'' build in defaults configuration.
\item[STORE TO FLASH] This stores the current signal configuration to flash.
\item[RESTORE FROM FLASH] This attempts to restore a signal
configuration from flash if a valid flash table exists and the
revision is compatible with the DSP code revision. Reflashing the DSP
code does NOT alter the signal configuration table and depending on
what is new the configuration will be in most cases still be
usable. Only if the signal or input descriptions have changed it
will fail -- nothing will be changed in this case. You can just
watch the configuration via the Patch Rack or compare the Signal
Graphs.
\item[ERASE (INVALIDATE) FLASH TABLE] This simply deletes any stored
signal configuration and thus invalidates the table. It does not
change the current configuration, but the next DSP power cycle will
obviously end up with the default configuration.
\end{description}

\GxsmHint{Advise: keep a human readable printout of your
configuration. The list on the terminal, a few screen shots as shown
here and may be also the signal graph.}

\GxsmNote{Notes to author: It is surely possible to save configuration
to a file on disk for later restore, but this is not yet
implemented.}

\GxsmScreenShotS{MK3-spm-configurator-2-signal-manager}{Signal
Manager/Flash Configurator. Please note, here are additional FLASH
test/debugging controls visible which will be excluded per default
soon.}{0.2}
\clearpage



% OptPlugInSubSection: Vizualization of the current Signal Configuration

The ``Create Signal Graph'' application inspects the current DSP
configuration, checks for errors and generates via the GraphViz tool a
dot file and a svg presentation of the configuration as shown in
Fig.~\ref{fig:screenshot:signal-graph}. This app writes a dot and svg file. It
attempts to launch xdot for viewing. This fails if xdot is not installed.

\clearpage




% OptPlugInSubSection: Signal Oszilloscope
\index{Signal Ranger MK3 Signal Scope}

\GxsmScreenShotS{MK3-spm-configurator-6-scope}{Signal Oszilloscope}{0.2}

\clearpage



% OptPlugInSubSection: GXSM Signal Master Evolved and beyond
\index{Signal Selection in GXSM} Configuring and selecting signals for
aquisition withing GXSM is not highly
flexible. Fig.~\ref{fig:screenshot:GXSM-SIGNAL-MASTER-EVOLVED} highlights the
dependencies from where signals are taken and now are dynamically set
for given slots in the channelselector source and VPsig source pull
down menus.

-- drafting --

Now full signal propagation and management via Gxsm including proper
unit and scaling support derived from signal definition table. All life,
actual settings read back from DSP at Gxsm startup and refresh at scan
start (in case of external manipulations).  Mixer channel signal
selections are now reflected by signal name in Channelselector and
also automatically used as Scan title/file name.  New are also up to 4
fully free assignable signals (32bit res.) as scan-source signal and
can be selected via the Channelselector, also automatic label
propagation and automatic resources updates for channel assignment
including units and scaling.  Along with this also new is a on
pixel/sub-grid level real time (fast) VP execute with
"end-of-VP-section" data assignment to a special 8x8 signal matrix "VP
Sec Vec64". More later about this. In general you can manipulate for
example the bias while FB hold or on, etc... in pixel level and
acquire at end of every VP section a value and assign this to a scan
channel as data source.

\GxsmScreenShotS{GXSM-SIGNAL-MASTER-EVOLVED}{Signal Master
Evolved}{0.2}

\clearpage




% OptPlugInSubSection: PAC/PLL Support and Tuning with Gxsm -- Startup guidelines
\index{Signal Ranger MK3 PAC/PLL Tuning}

Please start reading and understanding first the PLL principle of
operation, find the PLL user manual here:
\url{http://www.softdb.com/_files/_dsp_division/SPM_PLL_UsersManual.pdf}

The \Gxsm\ GUI is pretty much identical beyond a little different look
and arrangement of same control elements.  And the operation is
identical as Gxsm uses this SoftdB PAC-library and their DSP level
application interface.

For Gxsm as of now the PAC/PLL is started on channels\footnote{Channel
count convention is here 0 \dots 7.}  IN-4 (signal input) and OUT-7
(excitation) via the library call ``StartPLL(4,7);''. This implies for
all further operations that channel OUT-7 is not any more available
for any other uses, but only when the PAC processing is enabled.

\GxsmNote{About the MK3-Pro/A810-PLL and the by SoftdB provided active
miniature test oscillator PCB with two identical BNC's for input and
output: The excitation ``exec. signal input to the quartz'' side
(goes to A810 OUT-7) can be identified looking at the PCB and
finding the BNC what is connected only via R10 to the quartz (big
black block) or the BNC just next to the ON/OFF switch. The other
end connect to IN-4.}

\GxsmNote{Simple alternative to the active oscillator: you can actually
just get a simple passive ``watch'' quartz with $32.768$ kHz and
hook it it directly inbetween OUT-7 and IN-4. Hint: just mount it
inside a convenient small BNC-BNC ``filter'' box. It will work nice
with just 0.5V excitaion!}

\begin{enumerate}
\item For PAC/PLL signal monitoring select the two PLL related signals of interest
(for example the Resonator Amplitude ``PLL ResAmp'' and Phase (Frequency equivalent) ``PLL ResPh'')
to watch with the scope via the Patch Rack and assign to ``SCOPE\_SIGNAL1/2\_INPUT''.

\GxsmScreenShotS{MK3-spm-configurator-2-signal-patchrack-PLL-scope}{Example
Signal Patch Configuration for PLL view}{0.2}

\item Then select the phase detector time constant ``TauPAC'' -- set it
to 20 $\mu$s or faster.  \textbf{Referring to the PLL manual ``4.1 Phase
Detector Time Cst (s)'':} This control adjusts the time constant
of the phase detector. We suggest keeping time constant to $20 \mu$s
(fast set-up), which allows a bandwidth of about $8$ kHz. Note that
the auto-adjustment functions for the PI gains of both controllers
(amplitude and phase) automatically\footnote{This is \textbf{not} the
case for the Gxsm control and to use the auto gain adjustments
you must set TauPAC manually to 20 $\mu$s.} set the time constant
to 20us.  This way, the bandwidth of the controller is only limited
by the PI gains and the LP filter.

Any further tweaking of the time constant will prevent the auto-set
via ``Q'' from resulting in stable control loop conditions, so you
have to manually tweak those.

\item Then turn on the PAC Processing by activating the check box. Or toggle
to restart with changed time constant!

\item Operational ranges setup. This defines the range mapping and
actual sensitivities for excitation frequency, resonator phase,
excitation and resonator amplitudes.  Set the phase range to $360$
\degree ($\pm180$ \degree) for tuning (see Fig.~\ref{fig:screenshot:MK3-Gxsm-PLL-Control-op}). 
Select reasonable ranges for amplitudes matching your system needs.
\GxsmScreenShotS{MK3-Gxsm-PLL-Control-op}{PAC/PLL initial operation
boundary setup example. Time constant set to 20 $\mu$s here, PAC
processing enabled.}{0.5}

\item Check signals, manually set a estimated frequency, play manually
and check for any initial response using the monitoring option to
just read the values. Leave both FB Controllers off for now. You
should see some with frequency changing resonator response
amplitude, make sure ranges are working for you, if not, readjust.
\GxsmScreenShotS{MK3-Gxsm-PLL-Control-phase0}{PAC/PLL -- initial
manual pre-check of signals and responses.}{0.2} \clearpage

\item Now open the tune App to initiate a default sweep.
\GxsmScreenShotS{MK3-spm-configurator-7-tuneapp-initial}{PAC/PLL
Tune App -- initial run with phase reference set to zero. Here a
passive watch quartz was used.}{0.2} \clearpage

\item You may now already adjust the ``Setpoint-Phase'' to the value
the tune app detected phase at resonance. This is important as the
Phase controller works always around phase zero. And you have to
compensate via this Setpoint the phase offset. This done a
consecutive tune sweep should align the phase accordingly.
\GxsmScreenShotS{MK3-spm-configurator-7-tuneapp-phase-set}{PAC/PLL
Tune App -- frequency sweep with phase reference value was set to
initial phase at resonance as needed for phase locked loop (PLL is
always operating around phase 0). Here a passive watch quartz was
used}{0.2} \clearpage

\item Use the oscilloscope app to inspect the signals for further fine
tuning.\footnote{To be resolved issue,
currently the scope max block size/count is about 2000. Hi-level
Python issue. The DSP has storage for up to 1e6-1 value
pairs.}
\GxsmScreenShotS{MK3-spm-configurator-6-scope-PACsig}{Oscilloscope app
for viewing PLL operation. Use AC coupling for both channels so that 
you can enlarge the signals in a convient way.}{0.2}

\item Re run the tune tool with phase set to initial detected value
(relative to the initial reference set to zero! Keep in mind any
further adjustments are relative.).
\GxsmScreenShotS{MK3-Gxsm-PLL-Control-phase-set}{PAC/PLL Feedback
settings with phase reference set to initial phase at resonance
(here -73.7$\degree$).}{0.2} \clearpage

\item Enter the Q value computed by the tune app fit here to auto-set
the amplitude controller gains. Usually a good starting point.
Remember, those are only good for a phase detector time constant of
$20 \mu$s or faster as otherwise you limit the bandwidth before the
controllers! Select a reasonable amplitude setpoint, make sure the
excitation range is sufficient and turn on the amplitude controller.
\GxsmScreenShotS{MK3-Gxsm-PLL-Control-PAC-Ampl-FB}{PAC/PLL --
Amplitude FB on}{0.2}

\item Assuming the amplitude controller works you may proceed and
engage the phase controller to finally close the phase locked loop.
Just select the desired bandwidth. Then engage the controller check
box.
\GxsmScreenShotS{MK3-Gxsm-PLL-Control-PAC-ampl-phase-FB}{PAC/PLL
fully operational with both controller engaged. Amplitude
stabilized. Phase fixed.}{0.2}

\item Step Response Test Tool -- not yet completed -- need to fix
related issue with the python problem with big buffer transfers.

\end{enumerate}

\clearpage

\GxsmScreenShotS{MK3-DSP-Control-FB}{DSP Control -- Mixer, Servo}{0.2}
\index{Signal Ranger MK3 Mixer, Z-Servo}

\GxsmScreenShotS{MK3-DSP-Control-FB-w-Mservo}{DSP Control with M-Servo input configured}{0.2}
\index{Signal Ranger MK3 M-Servo}

\clearpage

Few MK3-Pro related notes on LockIn updates:

\begin{itemize}
\item Reference Sinus table length is 512 samples, MK3 LockIn operated
on full rate of $150$ kHz. Available frequencies are: $\times$ 1, 2,
4, $\dots$ 32 (all 512 $\dots$ $\frac{512}{32}$ samples per period)
$\rightarrow$ 292.968 Hz, 585.938 Hz, 1171.875 Hz, 2343.75 Hz,
4687.5 Hz, 9375.0 Hz.
\item Do not run LockIn in ``run free'' mode for Phase adjusting sweep.
\item Amplitude setting is respected in real time.
\item To change frequency in ``run free'' mode you must toggle it
on/off after chaning to re initialize. Touch frq. entry to update on
actual set frequincy.
\end{itemize}

\GxsmScreenShotS{MK3-DSP-Control-LockIn}{DSP Control -- LockIn: Free run mode enabled.}{0.5}
\index{Signal Ranger MK3 LockIn}

\clearpage


\GxsmScreenShotS{MK3-DSP-Control-LM}{DSP VP-LM -- tip forming example procedure}{0.2}

\GxsmScreenShotS{MK3-DSP-Control-Graphs-LM}{Graphs example for VP-LM}{0.2}

\clearpage

% OptPlugInSection: Optional MK3-Pro-SmartPiezoDrive with Gxsm Link
\index{Signal Ranger MK3-Smart Piezo Drive}
\index{MK3-Smart Piezo Drive}
\index{Smart Piezo Drive}
\label{sec:SPD}
The Smart Piezo Drive Linux Frontend
(Fig.~\ref{fig:screenshot:MK3-SmartPiezoDriveControl-Link}) with Gxsm-DSP
Offset-Link capability is provided via this python application:

\filename{plug-ins/hard/MK3-A810\_spmcontrol/python\_scripts/mk3\_spd\_control.py}

It communicates with the SPM contol DSP and periodically reads and
updated it's offset settings of the Link is enabled. For this the last
4 Signal Monitor slots are utilized and these signals should not be
changed when the Link is up. It automatically is setting up the Link
and Signals at startup and expects those not to be changed later.

\GxsmWarning{Beware: Later reassigning any of the last 4 Signal
Monitor slots will result in wrong offset or gain settings.}

\GxsmScreenShotS{MK3-SmartPiezoDriveControl-Link}{MK3 based Smart
Piezo Drive (HV Amplifier) Control Panel with Gxsm-Link for digital
Offset control.}{0.3}
\clearpage

%% OptPlugInSources

%% OptPlugInDest

% OptPlugInNote
Special features and behaviors to be documented here!

% EndPlugInDocuSection
* -------------------------------------------------------------------------------- 
*/

#include <sys/ioctl.h>

#include "config.h"
#include "plugin.h"
#include "xsmhard.h"
#include "glbvars.h"

#include "sranger_mk23common_hwi.h"
#include "sranger_mk2_hwi_control.h"
#include "sranger_mk2_hwi.h"
#include "sranger_mk3_hwi.h"


// Define HwI PlugIn reference name here, this is what is listed later within "Preferenced Dialog"
// i.e. the string selected for "Hardware/Card"!
#define THIS_HWI_PLUGIN_NAME "SRangerMK2:SPM"
#define THIS_HWI_PREFIX      "SR-MK2/3_HwI"


// Plugin Prototypes
static void sranger_mk2_hwi_init( void );
static void sranger_mk2_hwi_about( void );
static void sranger_mk2_hwi_configure( void );
static void sranger_mk2_hwi_query( void );
static void sranger_mk2_hwi_cleanup( void );

static void DSPControl_show_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data);
static void DSPControl_show_usertabs_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data);
static void DSPMover_show_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data);
static void DSPPAC_show_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data);

// custom menu callback expansion to arbitrary long list
static PIMenuCallback sranger_mk2_hwi_mcallback_list[] = {
		DSPControl_show_callback,             // direct menu entry callback1
		DSPControl_show_usertabs_callback,    // direct menu entry callback2
		DSPMover_show_callback,               // direct menu entry callback3
		DSPPAC_show_callback,                 // direct menu entry callback4
		NULL  // NULL terminated list
};

static GxsmPluginMenuCallbackList sranger_mk2_hwi_menu_callback_list = {
	3,
	sranger_mk2_hwi_mcallback_list
};

GxsmPluginMenuCallbackList *get_gxsm_plugin_menu_callback_list ( void ) {
	return &sranger_mk2_hwi_menu_callback_list;
}

extern "C++" {
        
        DSPControlContainer *DSPControlContainerClass = NULL;
        DSPControl *DSPControlClass = NULL;
        DSPMoverControl *DSPMoverClass = NULL;
        DSPPACControl *DSPPACClass = NULL;
        DSPControlUserTabs *DSPControlUserTabsClass = NULL;
        sranger_common_hwi_dev *sranger_common_hwi = NULL; // instance of the HwI derived XSM_Hardware class

        // Fill in the GxsmPlugin Description here
        GxsmPlugin sranger_mk2_hwi_pi = {
                NULL,                   // filled in and used by Gxsm, don't touch !
                NULL,                   // filled in and used by Gxsm, don't touch !
                0,                      // filled in and used by Gxsm, don't touch !
                NULL,                   // The Gxsm-App Class Ref.pointer (called "gapp" in Gxsm) is 
                // filled in here by Gxsm on Plugin load, 
                // just after init() is called !!!
                // ----------------------------------------------------------------------
                // Plugins Name, CodeStly is like: Name-M1S|M2S-BG|F1D|F2D|ST|TR|Misc
                "sranger_mk2_hwi-"
                "HW-INT-1S-SHORT",
                // Plugin's Category - used to autodecide on Pluginloading or ignoring
                // In this case of Hardware-Interface-Plugin here is the interface-name required
                // this is the string selected for "Hardware/Card"!
                THIS_HWI_PLUGIN_NAME,
                // Description, is shown by PluginViewer (Plugin: listplugin, Tools->Plugin Details)
                g_strdup ("SRanger MK2/3 hardware interface."),
                // Author(s)
                "Percy Zahl",
                // Menupath to position where it is appendet to -- not used by HwI PIs
                "windows-section,windows-section,windows-section,windows-section",
                // Menuentry -- not used by HwI PIs
                N_("SPM Control,SPM Control Tabs..,Mover Control,PAC Control"),
                // help text shown on menu
                N_("This is the " THIS_HWI_PLUGIN_NAME " - GXSM Hardware Interface."),
                // more info...
                "N/A",
                NULL,          // error msg, plugin may put error status msg here later
                NULL,          // Plugin Status, managed by Gxsm, plugin may manipulate it too
                // init-function pointer, can be "NULL", 
                // called if present at plugin load
                sranger_mk2_hwi_init,  
                // query-function pointer, can be "NULL", 
                // called if present after plugin init to let plugin manage it install itself
                sranger_mk2_hwi_query, // query can be used (otherwise set to NULL) to install
                // additional control dialog in the GXSM menu
                // about-function, can be "NULL"
                // can be called by "Plugin Details"
                sranger_mk2_hwi_about,
                // configure-function, can be "NULL"
                // can be called by "Plugin Details"
                sranger_mk2_hwi_configure,
                // run-function, can be "NULL", if non-Zero and no query defined, 
                // it is called on menupath->"plugin"
                NULL, // run should be "NULL" for Gxsm-Math-Plugin !!!
                // cleanup-function, can be "NULL"
                // called if present at plugin removal
                NULL,   // direct menu entry callback1 or NULL
                NULL,   // direct menu entry callback2 or NULL
                sranger_mk2_hwi_cleanup // plugin cleanup callback or NULL
        };
}

// Text used in Aboutbox, please update!!
static const char *about_text = N_("GXSM sranger_mk2_hwi Plugin\n\n"
                                   "Signal Ranger MK2 Hardware Interface for SPM.");

/* Here we go... */

/*
 * PI global
 */

// #define PI_DEBUG(L, DBGTXT) std::cout << "** (" << __FILE__ << ": " << __FUNCTION__ << ") Gxsm-PI-DEBUG-MESSAGE **: " << std::endl << " - " << DBGTXT << std::endl

gchar *sranger_mk2_hwi_configure_string = NULL;   // name of the currently in GXSM configured HwI (Hardware/Card)

const gchar *DSPControl_menupath  = "windows-section";
const gchar *DSPControl_menuentry = N_("SR-DSP Control");
const gchar *DSPControl_menuhelp  = N_("open the SR-DSP control window");

const gchar *DSPControl_menuentry_user = N_("SR-DSP User Tabs");
const gchar *DSPControl_menuhelp_user  = N_("open the SR-DSP control window for user customization");

const gchar *DSPMover_menuentry = N_("SR-DSP Mover");
const gchar *DSPMover_menuhelp  = N_("open the SR-DSP mover control window");

const gchar *DSPPAC_menuentry = N_("SR-DSP PAC");
const gchar *DSPPAC_menuhelp  = N_("open the SR-DSP Phase/Amplitude Convergent Detector control window");


static void DSPControl_StartScan_callback ( gpointer );

static void DSPControl_SaveValues_callback ( gpointer );
static void DSPControl_LoadValues_callback ( gpointer );

/* 
 * PI essential members
 */

// Symbol "get_gxsm_plugin_info" is resolved by dlsym from Gxsm, used to get Plugin's info!! 
// Essential Plugin Function!!
GxsmPlugin *get_gxsm_plugin_info ( void ){ 
	sranger_mk2_hwi_pi.description = g_strdup_printf(N_("GXSM HwI " THIS_HWI_PREFIX " plugin %s"), VERSION);
	return &sranger_mk2_hwi_pi; 
}

// Symbol "get_gxsm_hwi_hardware_class" is resolved by dlsym from Gxsm for all HwI type PIs, 
// Essential Plugin Function!!
XSM_Hardware *get_gxsm_hwi_hardware_class ( void *data ) {
        gchar *tmp;
        PI_DEBUG_GM (DBG_L1, "XSM_Hardware *get_gxsm_hwi_hardware_class ** HWI-DEV-MK2/3: Initialization and DSP verification.");
        main_get_gapp()->monitorcontrol->LogEvent (THIS_HWI_PREFIX " XSM_Hardware *get_gxsm_hwi_hardware_class", "Init 1");
	PI_DEBUG_GM (DBG_L1, THIS_HWI_PREFIX "... XSM_Hardware *get_gxsm_hwi_hardware_class");
        PI_DEBUG_GM (DBG_L3, " -> SR-MK2/3 HardwareInterface                        * Init 1");
	sranger_mk2_hwi_configure_string = g_strdup ((gchar*)data);
	PI_DEBUG_GM (DBG_L3, " -> sranger_mk2/3_hwi HardwareInterface               * Init 2");
	// probe for MK2, then MK3
	sranger_common_hwi = new sranger_mk2_hwi_spm ();
	PI_DEBUG_GM (DBG_L3, " -> sranger_mk2/3_hwi HardwareInterface common        * Init 3");

	if (sranger_common_hwi){
                PI_DEBUG_GM (DBG_L1, " -> sranger_mk2/3_hwi HardwareInterface probing for MK2...");
                PI_DEBUG_GM (DBG_L1, " -> DSP Mark ID found: SR-MK%d", sranger_common_hwi -> get_mark_id());

                tmp = g_strdup_printf ("MK %d", sranger_common_hwi -> get_mark_id());
                main_get_gapp()->monitorcontrol->LogEvent ("DSP Mark ID found", tmp); g_free (tmp);
                
                if (sranger_common_hwi->get_mark_id () != 2){ // no MK2 found
			PI_DEBUG_GM (DBG_L1, "    ... as this not a SR-MK2 DSP, reassigning ...");
			delete sranger_common_hwi;
                        sranger_common_hwi = NULL;
			// probe for MK3
			PI_DEBUG_GM (DBG_L1, "    ... probing for SR-MK3 DSP and software details.");
			sranger_common_hwi = new sranger_mk3_hwi_spm ();
			if (!sranger_common_hwi){
				PI_DEBUG_GW (DBG_L1, " -> ERROR not a MK3 SPMCONTROL -- HwI common init failed.");
				// exit (0); // -- no force exit here -- testing
				return NULL;
                        }
			if (sranger_common_hwi){
				if (sranger_common_hwi->get_mark_id() != 3){ // no MK3 found
                                        PI_DEBUG_GW (DBG_L1, " -> SR-MARK3 test failed.");
					delete sranger_common_hwi;
					sranger_common_hwi = NULL;
					PI_DEBUG_GW (DBG_L1, " -> ERROR -- DSP auto detection failed, no MK3 or MK2 found.\n");
					g_warning (" HwI Initialization error: -> E01 -- DSP auto detection failed, no MK3 or MK2 found.\n"
                                                   " Make sure DSP is plugged in and powered. Check kernel module and permissions.\n");
                                        main_get_gapp()->monitorcontrol->LogEvent ("ERROR: DSP auto detection failed, no MK3 or MK2 found.", " ??? ");
					// exit (0); // -- no force exit here -- testing
					return NULL;
				}
			}
                        if (!sranger_common_hwi -> dsp_device_status()){
                                PI_DEBUG_GM (DBG_L1, "XSM_Hardware *get_gxsm_hwi_hardware_class ... HWI-DEV-MK2/3: Open HwI Device Failed.");
                                return NULL;
                        }
		}
                PI_DEBUG_GM (DBG_L3, "    MK2/3 SPMCONTROL PRESENT: verify OK.");
	} else {
		PI_DEBUG_GM (DBG_L1, " -> ERROR: no MK3 or MK2 found.");
                g_critical ("HWI-DEV-MK2/3: HwI Init failed with E03.");
		// exit (0); // -- no force exit here -- testing
		return NULL;
	}
        PI_DEBUG_GM (DBG_EVER, "HWI-DEV-MK2/3: auto probing succeeded: MK%d DSP SPMCONTROL identified and ready.", sranger_common_hwi -> get_mark_id ());
        main_get_gapp()->monitorcontrol->LogEvent ("HwI: probing succeeded.", "DSP System Ready.");
	return sranger_common_hwi;
}

// init-Function
static void sranger_mk2_hwi_init(void)
{
	PI_DEBUG_GM (DBG_L2, "sranger_mk2_hwi Plugin Init");
	sranger_common_hwi = NULL;
}

// about-Function
static void sranger_mk2_hwi_about(void)
{
	const gchar *authors[] = { sranger_mk2_hwi_pi.authors, NULL};
	gtk_show_about_dialog (NULL, 
			       "program-name",  sranger_mk2_hwi_pi.name,
			       "version", VERSION,
			       "license", GTK_LICENSE_GPL_3_0,
			       "comments", about_text,
			       "authors", authors,
			       NULL
			       );
}

// configure-Function
static void sranger_mk2_hwi_configure(void)
{
	PI_DEBUG_GM (DBG_L2, "sranger_mk2_hwi Plugin HwI-Configure");
	if(sranger_mk2_hwi_pi.app)
		sranger_mk2_hwi_pi.app->message("sranger_mk2_hwi Plugin Configuration");
}

// query-Function
static void sranger_mk2_hwi_query(void)
{
	PI_DEBUG_GM (DBG_L1, THIS_HWI_PREFIX "::sranger_mk2_hwi_query:: PAC check... ");
	if (sranger_common_hwi){ // probe for PAC lib/PLL capability
		if (sranger_common_hwi->check_pac() != -1){
			PI_DEBUG_GM (DBG_L1, " HwI and PLL cap. present, adding PAC control.");
			sranger_mk2_hwi_menu_callback_list.n = 4; // adjust to include PAC menu entry
		} else {
			PI_DEBUG_GM (DBG_L1, " HwI OK, no PLL capability detected.");
		}
	} else {
		PI_DEBUG_GM (DBG_L1, " no valid HwI.");
	}


//      Setup Control Windows
// ==================================================
	DSPControlContainerClass = new DSPControlContainer ( force_gxsm_defaults || debug_level > 10 ? 1:0, debug_level);
	DSPControlContainerClass->realize ();

	sranger_mk2_hwi_pi.status = g_strconcat(N_("Plugin query has attached "),
					   sranger_mk2_hwi_pi.name, 
					   N_(": " THIS_HWI_PREFIX "-Control is created."),
					   NULL);
}

DSPControlContainer::DSPControlContainer(int default_tabs, int debug){
	freeze_config = 0;
	dbg = debug;
        hwi_settings = g_settings_new (GXSM_RES_BASE_PATH_DOT".hwi.sranger-mk23");

	for (int i=0; i<NOTEBOOK_WINDOW_NUMBER; ++i){
		DSP_notebook[i] = NULL;
		if (default_tabs){
			gchar *winid = g_strdup_printf ("window-%02d-tabs", i);
                        g_settings_set_string (hwi_settings, winid, "------------------------------");
                        g_free (winid);
		}
	}
	for (int i=0; i<NOTEBOOK_TAB_NUMBER; ++i){
		DSP_notebook_tab[i] = NULL;
		DSP_notebook_tab_placed[i] = 0;
	}
}

void DSPControlContainer::realize (){
//	SR DSP PAC Control Window
// ==================================================

        PI_DEBUG_GM (DBG_L3, THIS_HWI_PREFIX"::sranger_mk2_hwi DSPControlContainer::Realize -- check and add PAC window");
        
	if (sranger_common_hwi) // probe for PAC lib/PLL capability
		if (sranger_common_hwi->check_pac() != -1){
			DSPPACClass = new DSPPACControl (main_get_gapp() -> get_app ());
		}

//	SR DSP Control Window
// ==================================================

        PI_DEBUG_GM (DBG_L4, THIS_HWI_PREFIX"::sranger_mk2_hwi DSPControlContainer::Realize -- add DSPControl window");
	DSPControlClass = new DSPControl (main_get_gapp() -> get_app ());
	sranger_mk2_hwi_pi.app->ConnectPluginToStartScanEvent (DSPControl_StartScan_callback);

        PI_DEBUG_GM (DBG_L4, THIS_HWI_PREFIX"::sranger_mk2_hwi DSPControlContainer::Realize ConnectPluginToCDFSaveEvent");	
// connect to GXSM nc-fileio
        
	sranger_mk2_hwi_pi.app->ConnectPluginToCDFSaveEvent (DSPControl_SaveValues_callback);
	sranger_mk2_hwi_pi.app->ConnectPluginToCDFLoadEvent (DSPControl_LoadValues_callback);

//      User Tabs Window (blanc)
// ==================================================
        PI_DEBUG_GM (DBG_L4, THIS_HWI_PREFIX"::sranger_mk2_hwi DSPControlContainer::Realize -- add DSPControl User Tabs window");
	DSPControlUserTabsClass = new DSPControlUserTabs (main_get_gapp() -> get_app ());

//	SR DSP Mover Control Window
// ==================================================
        PI_DEBUG_GM (DBG_L4, THIS_HWI_PREFIX"::sranger_mk2_hwi DSPControlContainer::Realize -- add DSPMover Control window");
	DSPMoverClass = new DSPMoverControl (main_get_gapp() -> get_app ());

	PI_DEBUG (DBG_L4, THIS_HWI_PREFIX "::sranger_mk2_hwi_query:: populate tabs now...\n");

        populate_tabs ();
}

DSPControlContainer::~DSPControlContainer(){
	freeze_config = 1;

	if( DSPControlUserTabsClass )
		delete DSPControlUserTabsClass ;
	DSPControlUserTabsClass = NULL;

	if( DSPControlClass )
		delete DSPControlClass ;
	DSPControlClass = NULL;

	if( DSPMoverClass )
		delete DSPMoverClass ;
	DSPMoverClass = NULL;

	if( DSPPACClass )
		delete DSPPACClass ;
	DSPPACClass = NULL;

        g_clear_object (&hwi_settings);
}


int DSPControlContainer::callback_tab_reorder (GtkNotebook *notebook, GtkWidget *child, guint page_num, DSPControlContainer *dspc){
	gint n_w, n_t;
	n_w = GPOINTER_TO_INT (g_object_get_data(G_OBJECT (notebook), "notebook-window"));
	n_t = GPOINTER_TO_INT (g_object_get_data(G_OBJECT (child), "notebook-tab-contents"));
	
	PI_DEBUG (DBG_L5, "Notebook DSPControl tab reordered: window=" << n_w << " tab=" << n_t << " page=" << page_num);

	if (dspc->freeze_config)
		return 0;

	gchar *x = NULL;
	gchar *winid = g_strdup_printf ("window-%02d-tabs", n_w);
        x = g_settings_get_string (dspc->hwi_settings, winid); // "------------------------------");
	GString *tabconfig = g_string_new (x);
	g_free (x);
        PI_DEBUG (DBG_L5, "Notebook DSPControl tab reordered: window=" << n_w << " tabs_i=" << tabconfig->str);
	gchar *c = tabconfig->str;
	gint i=0;
	gchar key = 'a'+(gchar)n_t;
	while (*c && *c != key) ++i, ++c;
	g_string_erase (tabconfig, i, 1);
	g_string_insert_c (tabconfig, page_num, key);

        g_settings_set_string (dspc->hwi_settings, winid, tabconfig->str);
        PI_DEBUG (DBG_L5, "Notebook DSPControl tab reordered: window=" << n_w << " tabs_f=" << tabconfig->str);
	g_free (winid);
	g_string_free (tabconfig, true);
	return 1;
}

int DSPControlContainer::callback_tab_added (GtkNotebook *notebook, GtkWidget *child, guint page_num, DSPControlContainer *dspc){
	gint n_w, n_t;
	n_w =  GPOINTER_TO_INT (g_object_get_data(G_OBJECT (notebook), "notebook-window"));
	n_t = GPOINTER_TO_INT (g_object_get_data(G_OBJECT (child), "notebook-tab-contents"));
	
        PI_DEBUG (DBG_L4, "Notebook DSPControl tab added: window=" << n_w << " tab=" << n_t << " page=" << page_num);

	if (dspc->freeze_config)
		return 0;

	gchar *x = NULL;
	gchar *winid = g_strdup_printf ("window-%02d-tabs", n_w);
        x = g_settings_get_string (dspc->hwi_settings, winid); // "------------------------------");
	GString *tabconfig = g_string_new (x);
	g_free (x);
        PI_DEBUG (DBG_L5, "Notebook DSPControl tab added: window=" << n_w << " tabs_i=" << tabconfig->str);
	gchar key = 'a'+(gchar)n_t;
	g_string_insert_c (tabconfig, page_num, key);

        g_settings_set_string (dspc->hwi_settings, winid, tabconfig->str);
        PI_DEBUG (DBG_L5, "Notebook DSPControl tab added: window=" << n_w << " tabs_f=" << tabconfig->str);
	g_free (winid);
	g_string_free (tabconfig, true);
	return 1;
}

int DSPControlContainer::callback_tab_removed (GtkNotebook *notebook, GtkWidget *child, guint page_num, DSPControlContainer *dspc){
	gint n_w, n_t;
	n_w =  GPOINTER_TO_INT (g_object_get_data(G_OBJECT (notebook), "notebook-window"));
	n_t = GPOINTER_TO_INT (g_object_get_data(G_OBJECT (child), "notebook-tab-contents"));
	
	PI_DEBUG (DBG_L4, "Notebook DSPControl tab removed: window=" << n_w << " tab=" << n_t << " page=" << page_num);

	if (dspc->freeze_config)
		return 0;

	gchar *x = NULL;
	gchar *winid = g_strdup_printf ("window-%02d-tabs", n_w);

        x = g_settings_get_string (dspc->hwi_settings, winid); // "------------------------------");
	GString *tabconfig = g_string_new (x);
	g_free (x);
	PI_DEBUG (DBG_L4, "Notebook DSPControl tab removed: window=" << n_w << " tabs_i=" << tabconfig->str);
	gchar *c = tabconfig->str;
	gint i=0;
	gchar key = 'a'+(gchar)n_t;
	while (*c && *c != key) ++i, ++c;
	g_string_erase (tabconfig, i, 1);

        g_settings_set_string (dspc->hwi_settings, winid, tabconfig->str);

	PI_DEBUG (DBG_L4, "Notebook DSPControl tab removed: window=" << n_w << " tabs_f=" << tabconfig->str);
	g_free (winid);
	g_string_free (tabconfig, true);
	return 1;
}


static void DSPControl_show_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data){
	if ( DSPControlClass )
		DSPControlClass->show();
}

static void DSPControl_show_usertabs_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data){
	if ( DSPControlUserTabsClass )
		DSPControlUserTabsClass->show();
}

static void DSPMover_show_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data){
	if ( DSPMoverClass )
		DSPMoverClass->show();
}

static void DSPPAC_show_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data){
	if ( DSPPACClass )
		DSPPACClass->show();
}

static void DSPControl_StartScan_callback( gpointer ){
	PI_DEBUG_GM (DBG_L1, THIS_HWI_PREFIX "::DSPControl_StartScan_callback");
	if ( DSPControlClass )
		DSPControlClass->update();
}


static void DSPControl_SaveValues_callback ( gpointer ncf ){
	PI_DEBUG (DBG_L4, "SR-HwI::SPControl_SaveValues_callback");
	if ( DSPControlClass )
		DSPControlClass->save_values (*(NcFile *) ncf);
}

static void DSPControl_LoadValues_callback ( gpointer ncf ){
	PI_DEBUG (DBG_L4, "SR-HwI::SPControl_LoadValues_callback");
	if ( DSPControlClass )
		DSPControlClass->load_values (*(NcFile *) ncf);
}

// cleanup-Function
static void sranger_mk2_hwi_cleanup(void)
{
	PI_DEBUG (DBG_L4, THIS_HWI_PREFIX "::sranger_mk2_hwi_cleanup -- Plugin Cleanup, -- Menu [disabled]");
#if 0
	gchar *mp = g_strconcat(DSPControl_menupath, DSPControl_menuentry, NULL);
	gnome_app_remove_menus (GNOME_APP( sranger_mk2_hwi_pi.app->getApp() ), mp, 1);
	g_free(mp);

	mp = g_strconcat(DSPControl_menupath, DSPMover_menuentry, NULL);
	gnome_app_remove_menus (GNOME_APP( sranger_mk2_hwi_pi.app->getApp() ), mp, 1);
	g_free(mp);

	mp = g_strconcat(DSPControl_menupath, DSPPAC_menuentry, NULL);
	gnome_app_remove_menus (GNOME_APP( sranger_mk2_hwi_pi.app->getApp() ), mp, 1);
	g_free(mp);
#endif

	// delete ... 

	if( DSPControlContainerClass ){
                PI_DEBUG (DBG_L4, THIS_HWI_PREFIX "::sranger_mk2_hwi_cleanup -- delete DSP Control-Container-Class.");
		delete DSPControlContainerClass ;
        }
	DSPControlContainerClass = NULL;

	if (sranger_common_hwi){
                PI_DEBUG (DBG_L4, THIS_HWI_PREFIX "::sranger_mk2_hwi_cleanup -- delete DSP Common-HwI-Class.");
		delete sranger_common_hwi;
        }
	sranger_common_hwi = NULL;

        PI_DEBUG (DBG_L4, THIS_HWI_PREFIX "::sranger_mk2_hwi_cleanup -- g_free sranger_mk2_hwi_configure_string Info.");
	g_free (sranger_mk2_hwi_configure_string);
	sranger_mk2_hwi_configure_string = NULL;
	
        PI_DEBUG (DBG_L4, THIS_HWI_PREFIX "::sranger_mk2_hwi_cleanup -- Done.");
}
