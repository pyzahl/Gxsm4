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
 * Large segments of code were borrowewd from gwyddion's
 * (http://gwyddion.net/) pygwy. These were originally authored by
 * David Necas (Yeti), Petr Klapetek and Jan Horak
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
 * All "% OptPlugInXXX" tags are optional
 * --------------------------------------------------------------------------------
% BeginPlugInDocuSection
% PlugInDocuCaption: Python remote control
% PlugInName: pyremote
% PlugInAuthor: Stefan Schr\"oder
% PlugInAuthorEmail: stefan_fkp@users.sf.net
% PlugInMenuPath: Tools/Pyremote Console

% PlugInDescription
 This plugin is an interface to an embedded Python
 Interpreter.

% PlugInUsage
 Choose Pyremote Console from the \GxsmMenu{Tools} menu to enter a
 python console. A default command script is loaded when the console
 is started for the first time, but is not executed automatically. In
 the appendix you will find a tutorial with examples and tips and
 tricks.

% OptPlugInSection: Reference

The following script shows you all commands that the gxsm-module supports:

\begin{alltt}
print dir(gxsm)
\end{alltt}

The result will look like this:

\begin{alltt}
['__doc__', '__name__', 'autodisplay', 'chmodea',
'chmodem', 'chmoden', 'chmodeno', 'chmodex',
'chview1d', 'chview2d', 'chview3d', 'da0', 'direct',
'echo', 'export', 'import', 'load', 'log',
'logev', 'quick', 'save', 'saveas',
'scaninit', 'scanline', 'scanupdate', 'scanylookup',
'set', 'sleep', 'startscan', 'stopscan', 'unitbz',
'unitev', 'units', 'unitvolt', 'waitscan', 'createscan']
\end{alltt}


The following list shows a brief explanation of the commands, together with
the signature (that is the type of arguments).

'()' equals no argument. E.g. \verb+startscan()+

'(N)' equals one Integer arument. E.g. \verb+chview1d(2)+

'(X)' equals one Float argument. No example.

'(S)' equals a string. Often numbers are evaluated as strings first. Like
in \verb+set("RangeX", "234.12")+

'(S,N)' equals two parameters. E.g. \verb+gnuexport("myfilename.nc", 1)+

\begin{tabular}{ll} \hline
Scan operation\\
\texttt{startscan() }   &       Start a scan.\\
\texttt{stopscan() }    &       Stop scanning.\\
\texttt{waitscan}       &       is commented out in app\_remote.C\\
\texttt{initscan() }    &       only initialize.\\
\texttt{scanupdate() }  &       Set hardware parameters on DSP.\\
\texttt{setylookup(N,X)}&       ?\\
\texttt{scanline}       &       Not implemented.\\ \hline
File operation\\
\texttt{save() }  &       Save all.\\
\texttt{saveas(S,N) }    &       Save channel N with filename S.\\
\texttt{load(S,N)   }    &       Load file S to channel N.\\
\texttt{import(S,N) } &       Import file S to channel N.\\
\texttt{export(S,N) } &       Export channel N to file S.\\ \hline
Channel operation\\
\texttt{chmodea(N)}      &       Set channel(N) as active.\\
\texttt{chmodex(N)}      &       Set channel(N) to X.\\
\texttt{chmodem(N)}      &       Set channel(N) to Math.\\
\texttt{chmoden(N),N}    &       Set channel(N) to mode(N).\\
\texttt{chmodeno(N)}     &       Set channel(N) to mode 'No'.\\
\texttt{chview1d(N)}     &       View channel(N) in 1d-mode.\\
\texttt{chview2d(N)}     &       View channel(N) in 2d-mode.\\
\texttt{chview3d(N)}     &       View channel(N) in 3d-mode.\\ \hline
Views\\
\texttt{autodisplay()}  &       Autodisplay.\\
\texttt{quick()}        &       Set active display to quickview.\\
\texttt{direct()}       &       Set active display to directview.\\
\texttt{log()}          &       Set active display to logview.\\ \hline
Units\\
\texttt{unitbz() }      &       Set units to BZ.\\
\texttt{unitvolt()}     &       Set units to Volt.\\
\texttt{unitev()}       &       Set units to eV.\\
\texttt{units()}        &       Set units to S.\\ \hline
Others\\
\texttt{createscan(N,N,N,N,A) }  &  Create scan from array.\\
\texttt{list() }  &  Get list of known parameters for get/set.\\
\texttt{set(S,S)}       &  Set parameter to value.\\
\texttt{get(S)}         &  Get parameter, returns floating point value in current user unit .\\
\texttt{gets(S)}         &  Get parameter, returns string with user unit.\\
\texttt{action(S)}         &  Initiate Action (S): trigger menu actions and button-press events (refer to GUI tooltips in buttons and menu action list below).\\
\texttt{rtquery(S)}     &  Ask current HwI to run RTQuery with parameter S, return vector of three values depening on query.\\
\texttt{y\_current()}    &  Ask current HwI to run RTQuery what shall return the actual scanline of a scan in progress, undefined return otherwise.\\
\texttt{echo(S)  }  &       Print S to console.\\
\texttt{logev(S) }  &       Print S to logfile.\\
\texttt{sleep(N) }  &       Sleep N/10 seconds.\\
\texttt{add\_layerinformation(S,N)   }  &       Add Layerinformation string S to active scan, layer N.\\
\texttt{da0(X)   }  &       Set Analog Output channel 0 to X Volt. (not implemented).\\
\end{tabular}

% OptPlugInSubSection: The set-command

The set command can modify the following parameters:

\begin{tabular}{ll}
\texttt{ACAmp} & \texttt{ACFrq} \\
\texttt{ACPhase} & \texttt{} \\
\texttt{CPShigh} & \texttt{CPSlow} \\
\texttt{Counter} & \texttt{Energy} \\
\texttt{Gatetime} & \texttt{Layers} \\
\texttt{LengthX} & \texttt{LengthY} \\
\texttt{Offset00X} & \texttt{Offset00Y} \\
\texttt{OffsetX} & \texttt{OffsetY} \\
\texttt{PointsX} & \texttt{PointsY} \\
\texttt{RangeX} & \texttt{RangeY} \\
\texttt{Rotation} & \texttt{} \\
\texttt{StepsX} & \texttt{StepsY} \\
\texttt{SubSmp} & \texttt{VOffsetZ} \\
\texttt{VRangeZ} & \texttt{ValueEnd} \\
\texttt{ValueStart} & \texttt{nAvg} \\
\end{tabular}

These parameters are case-sensitive.
To help the python remote programmer to figure out the correct
set-names of all remote enabled entry fields a nifty option
was added to the Help menu to show tooltips with the correct "remote set name"
if the mouse is hovering over the entry.

% OptPlugInSubSection: The get-command

The \texttt{get()} command can retrieve the value of the remote control parameters.
While \texttt{get()} retrieves the internal value as a floating points number,
\texttt{gets()} reads the actual string from the text entry including units.
The list of remote control accessible parameters can be retrieved with \texttt{list()}.

\begin{alltt}
print "OffsetX = ", gxsm.get("OffsetX")
gxsm.set("OffsetX", "12.0")
print "Now OffsetX = ", gxsm.get("OffsetX")

for i in gxsm.list():
    print i, " ", gxsm.get(i), " as string: ", gxsm.gets(i)
\end{alltt}

On my machine (without hardware attached) this prints:

\begin{alltt}
OffsetX =  0.0
Now OffsetX =  12.0
Counter   0.0  as string:  00000
VOffsetZ   0.0  as string:  0 nm
VRangeZ   500.0  as string:  500 nm
Rotation   1.92285320764e-304  as string:  1.92285e-304 °
TimeSelect   0.0  as string:  0
Time   1.0  as string:  1
LayerSelect   0.0  as string:  0
Layers   1.0  as string:  1
OffsetY   0.0  as string:  0.0 nm
OffsetX   12.0  as string:  12.0 nm
PointsY   1000.0  as string:  1000
PointsX   1000.0  as string:  1000
StepsY   0.519863986969  as string:  0.52 nm
StepsX   0.519863986969  as string:  0.52 nm
RangeY   64.9830993652  as string:  65.0 nm
RangeX   64.9830993652  as string:  65.0 nm
\end{alltt}

All entry fields with assigned id can now be queried.


% OptPlugInSubSection: Creating new scans

Pyremote can create new images from scratch using the
\verb+createscan+ command. Its arguments are
pixels in x-direction, pixels in y-direction,
range in x-direction (in Angstrom),
range in y-direction (in Angstrom) and finally
a flat, numeric array that must contain
as many numbers as needed to fill the matrix.

This example creates a new scan employing sine to
show some pretty landscape.

\begin{alltt}
import array   # for array
import numpy # for fromfunction
import math    # for sin

def dist(x,y):
   return ((numpy.sin((x-50)/15.0) + numpy.sin((y-50)/15.0))*100)

m = numpy.fromfunction(dist, (100,100))
n = numpy.ravel(m) # make 1-d
p = n.tolist()       # convert to list

examplearray = array.array('l', p) #
gxsm.createscan(100, 100, 10000, 10000, examplearray)
\end{alltt}


\GxsmScreenShot{GxsmPI_pyremote01}{An autogenerated image.}

This command can be easily extended to create an importer for arbitrary
file formats via python. The scripts directory contains an elaborate
example how to use this facility to import the file format employed
by Nanonis.


% OptPlugInSubSection: Menupath and Plugins

Any plugin, that has a menuentry can be
executed via the
\GxsmTT{menupath}-action command. Several of them, however, open a dialog and ask
for a specific parameter, e.g. the diff-PI in \GxsmMenu{Math/Filter1D}.
This can become annoying, when you want to batch process a greater number
of files. To execute a PI non-interactively it is possible to
call a plugin from scripts with default parameters and no user interaction.

The \GxsmTT{diff}-PI can be called like this:

\begin{alltt}
print "Welcome to Python."
gxsm.logev('my logentry')
gxsm.startscan()
gxsm.action('diff_PI')
\end{alltt}

The \GxsmTT{diff}- and \GxsmTT{smooth}-function are, at the time of this
writing, the only Math-PI, that have such an 'action'-callback. Others
will follow. See \GxsmFile{diff.C} to find out, how to extend your
favourite PI with action-capabilities.

The action-command can execute the following PI:

\begin{tabular}{ll}
\GxsmTT{diff\_PI} & kernel-size set to 5+1\\
\GxsmTT{smooth\_PI} & kernel-size set to 5+1\\
\GxsmTT{print\_PI} & defaults are read from gconf\\
\end{tabular}

GXSM4 Menu Action Table Information -- remote menu action/math/... call via action key, see table below for list, example:

\begin{alltt}
gxsm.signal_emit("math-filter1d-section-Koehler")
\end{alltt}

\begin{tabular}{l|l||l}
Section ID & Menu Entry & Action Key\\
\hline\hline
math-filter2d-section & Stat Diff & math-filter2d-section-Stat-Diff\\
math-convert-section & to float & math-convert-section-to-float\\
math-arithmetic-section & Z Rescale & math-arithmetic-section-Z-Rescale\\
math-statistics-section & Add Trail & math-statistics-section-Add-Trail\\
math-transformations-section & OctoCorr & math-transformations-section-OctoCorr\\
math-arithmetic-section & Mul X & math-arithmetic-section-Mul-X\\
math-filter2d-section & Edge & math-filter2d-section-Edge\\
math-arithmetic-section & Max & math-arithmetic-section-Max\\
math-statistics-section & Stepcounter & math-statistics-section-Stepcounter\\
math-convert-section & to double & math-convert-section-to-double\\
math-filter1d-section & Koehler & math-filter1d-section-Koehler\\
math-statistics-section & Histogram & math-statistics-section-Histogram\\
math-background-section & Line: 2nd order & math-background-section-Line--2nd-order\\
math-filter2d-section & Despike & math-filter2d-section-Despike\\
math-transformations-section & Auto Align & math-transformations-section-Auto-Align\\
math-background-section & Plane Regression & math-background-section-Plane-Regression\\
math-statistics-section & Cross Correlation & math-statistics-section-Cross-Correlation\\
math-arithmetic-section & Z Usr Rescale & math-arithmetic-section-Z-Usr-Rescale\\
math-filter1d-section & Diff & math-filter1d-section-Diff\\
math-filter2d-section & T derive & math-filter2d-section-T-derive\\
math-background-section & Pass CC & math-background-section-Pass-CC\\
math-statistics-section & Auto Correlation & math-statistics-section-Auto-Correlation\\
math-transformations-section & Rotate 90deg & math-transformations-section-Rotate-90deg\\
math-arithmetic-section & Invert & math-arithmetic-section-Invert\\
math-background-section & Plane max prop & math-background-section-Plane-max-prop\\
math-convert-section & U to float & math-convert-section-U-to-float\\
math-transformations-section & Movie Concat & math-transformations-section-Movie-Concat\\
math-transformations-section & Shear Y & math-transformations-section-Shear-Y\\
math-statistics-section & NN-distribution & math-statistics-section-NN-distribution\\
math-background-section & Waterlevel & math-background-section-Waterlevel\\
math-transformations-section & Quench Scan & math-transformations-section-Quench-Scan\\
math-arithmetic-section & Div X & math-arithmetic-section-Div-X\\
math-statistics-section & Vacancy Line Analysis & math-statistics-section-Vacancy-Line-Analysis\\
math-transformations-section & Shear X & math-transformations-section-Shear-X\\
math-convert-section & to complex & math-convert-section-to-complex\\
math-convert-section & make test & math-convert-section-make-test\\
math-misc-section & Spectrocut & math-misc-section-Spectrocut\\
math-arithmetic-section & Log & math-arithmetic-section-Log\\
math-statistics-section & Average X Profile & math-statistics-section-Average-X-Profile\\
math-filter2d-section & Lineinterpol & math-filter2d-section-Lineinterpol\\
math-background-section & Z drift correct & math-background-section-Z-drift-correct\\
math-background-section & Line Regression & math-background-section-Line-Regression\\
math-statistics-section & SPALEED Simkz & math-statistics-section-SPALEED-Simkz\\
math-convert-section & to byte & math-convert-section-to-byte\\
math-statistics-section & Slope Abs & math-statistics-section-Slope-Abs\\
math-filter2d-section & Small Convol & math-filter2d-section-Small-Convol\\
math-convert-section & to long & math-convert-section-to-long\\
math-transformations-section & Multi Dim Transpose & math-transformations-section-Multi-Dim-Transpose\\
math-arithmetic-section & Sub X & math-arithmetic-section-Sub-X\\
math-background-section & Stop CC & math-background-section-Stop-CC\\
math-filter2d-section & FT 2D & math-filter2d-section-FT-2D\\
math-convert-section & to short & math-convert-section-to-short\\
math-transformations-section & Volume Transform & math-transformations-section-Volume-Transform\\
math-background-section & Gamma & math-background-section-Gamma\\
math-background-section & Plane 3 Points & math-background-section-Plane-3-Points\\
math-transformations-section & Affine & math-transformations-section-Affine\\
math-misc-section & Shape & math-misc-section-Shape\\
math-background-section & Sub Const & math-background-section-Sub-Const\\
math-transformations-section & Flip Diagonal & math-transformations-section-Flip-Diagonal\\
math-background-section & Timescale FFT & math-background-section-Timescale-FFT\\
math-misc-section & Layersmooth & math-misc-section-Layersmooth\\
math-filter1d-section & Lin stat diff & math-filter1d-section-Lin-stat-diff\\
math-filter2d-section & Smooth & math-filter2d-section-Smooth\\
math-transformations-section & Manual Drift Fix/Align & math-transformations-section-Manual-Drift-Fix-Align\\
math-statistics-section & Polar Histogramm & math-statistics-section-Polar-Histogramm\\
math-statistics-section & feature match & math-statistics-section-feature-match\\
math-statistics-section & Angular Analysis & math-statistics-section-Angular-Analysis\\
math-transformations-section & Merge V & math-transformations-section-Merge-V\\
math-filter2d-section & Local height & math-filter2d-section-Local-height\\
math-filter2d-section & IFT 2D & math-filter2d-section-IFT-2D\\
math-statistics-section & Baseinfo & math-statistics-section-Baseinfo\\
math-transformations-section & Scale Scan & math-transformations-section-Scale-Scan\\
math-filter2d-section & Curvature & math-filter2d-section-Curvature\\
math-arithmetic-section & Add X & math-arithmetic-section-Add-X\\
math-transformations-section & Mirror X & math-transformations-section-Mirror-X\\
math-transformations-section & Merge H & math-transformations-section-Merge-H\\
math-statistics-section & feature recenter & math-statistics-section-feature-recenter\\
math-transformations-section & Shift-Area & math-transformations-section-Shift-Area\\
math-misc-section & Workfuncextract & math-misc-section-Workfuncextract\\
math-filter1d-section & t dom filter & math-filter1d-section-t-dom-filter\\
math-statistics-section & SPALEED Sim. & math-statistics-section-SPALEED-Sim.\\
math-statistics-section & Slope Dir & math-statistics-section-Slope-Dir\\
math-transformations-section & Reverse Layers & math-transformations-section-Reverse-Layers\\
math-filter1d-section & Despike & math-filter1d-section-Despike\\
math-background-section & Rm Line Shifts & math-background-section-Rm-Line-Shifts\\
math-transformations-section & Rotate & math-transformations-section-Rotate\\
math-transformations-section & Mirror Y & math-transformations-section-Mirror-Y\\
math-arithmetic-section & Z Limiter & math-arithmetic-section-Z-Limiter\\
math-probe-section & AFM Mechanical Simulation & math-probe-section-AFM-Mechanical-Simulation\\
math-probe-section & Image Extract & math-probe-section-Image-Extract\\
\hline
file-import-section & PNG & file-import-section-PNG\\
file-export-section & PNG & file-export-section-PNG\\
file-import-section & WIP & file-import-section-WIP\\
file-export-section & WIP & file-export-section-WIP\\
file-import-section & primitive auto & file-import-section-primitive-auto\\
file-export-section & primitive auto & file-export-section-primitive-auto\\
file-import-section & UKSOFT & file-import-section-UKSOFT\\
file-export-section & UKSOFT & file-export-section-UKSOFT\\
file-import-section & PsiHDF & file-import-section-PsiHDF\\
file-export-section & PsiHDF & file-export-section-PsiHDF\\
file-import-section & WSxM & file-import-section-WSxM\\
file-export-section & WSxM & file-export-section-WSxM\\
file-import-section & RHK-200 & file-import-section-RHK-200\\
file-import-section & RHK SPM32 & file-import-section-RHK-SPM32\\
file-export-section & RHK SPM32 & file-export-section-RHK-SPM32\\
file-import-section & Nano Scope & file-import-section-Nano-Scope\\
file-import-section & Omicron Scala & file-import-section-Omicron-Scala\\
file-import-section & UK2k & file-import-section-UK2k\\
file-import-section & Vis5D & file-import-section-Vis5D\\
file-export-section & Vis5D & file-export-section-Vis5D\\
file-import-section & G-dat & file-import-section-G-dat\\
file-export-section & G-dat & file-export-section-G-dat\\
file-import-section & SDF & file-import-section-SDF\\
file-import-section & GME Dat & file-import-section-GME-Dat\\
file-export-section & GME Dat & file-export-section-GME-Dat\\
file-import-section & ASCII & file-import-section-ASCII\\
file-export-section & ASCII & file-export-section-ASCII\\
file-import-section & SPA4-d2d & file-import-section-SPA4-d2d\\
file-export-section & SPA4-d2d & file-export-section-SPA4-d2d\\
file-import-section & Quicktime & file-import-section-Quicktime\\
file-export-section & Quicktime & file-export-section-Quicktime\\
\end{tabular}


% OptPlugInSubSection: DSP-Control

The DSP-Control is the heart of SPM activity. The following parameters
can be set with \GxsmTT{set}. (DSP2 commands are available in Gxsm 2 only)

\GxsmNote{Manual Hacker notes: list of DSP/DSP2 is depricated. All entry fields with hover-over entry id is now remote capable.}

\begin{tabular}{ll}
\GxsmTT{DSP\_CI} & \GxsmTT{DSP2\_CI} \\
\GxsmTT{DSP\_CP} & \GxsmTT{DSP2\_CP} \\
\GxsmTT{DSP\_CS} & \GxsmTT{DSP2\_CS} \\
\GxsmTT{DSP\_I} & \GxsmTT{DSP2\_I} \\
\GxsmTT{DSP\_MoveLoops} & \GxsmTT{DSP2\_MoveLoops} \\
\GxsmTT{DSP\_MoveSpd} & \GxsmTT{DSP2\_MoveSpd} \\
\GxsmTT{DSP\_NAvg} & \GxsmTT{DSP2\_NAvg} \\
\GxsmTT{DSP\_Pre} & \GxsmTT{DSP2\_Pre} \\
\GxsmTT{DSP\_ScanLoops} & \GxsmTT{DSP2\_ScanLoops} \\
\GxsmTT{DSP\_ScanSpd} & \GxsmTT{DSP2\_ScanSpd} \\
\GxsmTT{DSP\_SetPoint} & \GxsmTT{DSP2\_SetPoint} \\
\GxsmTT{DSP\_U} & \GxsmTT{DSP2\_U} \\
\end{tabular}

\GxsmNote{Manual Hacker notes: VP exectutes via hover-over ExecuteID and action command.}



% OptPlugInSubSection: Peakfinder

Another plugin allows remote control. The plugin-functions are commonly
executed by a call of the \GxsmTT{action}-command. It is
\GxsmFile{Peakfinder}:

DSP Peak Find Plugin Commandset for the SPA-LEED peak finder:\\

\begin{tabular}{lllll}
\multicolumn{5}{c}{Commands Plugin \filename{DSP Peak Find}:}\\ \\ \hline
Cmd & Arg. & \multicolumn{2}{l}{Values} & Description\\ \hline
\hline
action & DSPPeakFind\_XY0\_1 &&& Get fitted XY Position\\
action & DSPPeakFind\_OffsetFromMain\_1 &&& Get Offset from Main\\
action & DSPPeakFind\_OffsetToMain\_1 &&& Put Offset to Main\\
action & DSPPeakFind\_EfromMain\_1 &&& Get Energy from Main\\
action & DSPPeakFind\_RunPF\_1 &&& Run Peak Finder\\
\hline
action & DSPPeakFind\_XXX\_N &&& run action XXX (see above)\\
       &                    &&& on PF Folder N\\
\end{tabular}

The call is equivalent to the example above.

% OptPlugInSubSection: Peakfinder

% OptPlugInConfig

The plugin can be configured in the preferences. The default script
that will be loaded when the console is enetred for the first time
is defined in the path-tab in item \GxsmFile{PyremoteFile}.
The name must be a qualified python module name. A module name
is not a filename! Thus \verb+remote.py+ is not a valid entry,
but \verb+remote+ (the default) is. The module is found by searching the
directories listed in the environment variable PYTHONPATH. If no file is
defined or a file matching the name cannot be found, a warning will be issued.

The module with GXSM internal commands
is called \GxsmFile{gxsm}.

To find the Python-script \GxsmFile{remote.py}, the environment-variable
PYTHONPATH is evaluated. If it is not expliticly declared, GXSM will set
PYTHONPATH to your current working directory. This is equivalent to the
following call:

\begin{alltt}
$export PYTHONPATH='.'
$gxsm4
\end{alltt}

Thus, the script in your current working directory will be found.

If you want to put your script somewhere else than into the
current directory, modify the environment variable
\GxsmFile{PYTHONPATH}. Python
will look into all directories, that are stored there.

\begin{alltt}
$export PYTHONPATH='/some/obscure/path/'
$gxsm
\end{alltt}

Or you can link it from somewhere else. Or you can create a one line script,
that executes another script or several scripts. Do whatever you like.


% OptPlugInFiles
 Python precompiles your remote.py to remote.pyc. You can safely remove the
file remote.pyc file at any time, Python will regenerate it upon start of
the interpreter.

% OptPlugInRefs
See the appendix for more information. Don't know Python? Visit
\GxsmTT{python.org}.

% OptPlugInKnownBugs

The error handling is only basic. Your script may run if you give
wrong parameters but not deliver the wanted results. You can crash
Gxsm or even X! E.g. by selecting an illegal channel. Remember that channel
counting in the scripts begins with 0. Gxsm's channel numbering begins with 1.

The embedded functions return $-1$ as error value. It's a good idea
to attach \texttt{print} to critical commands to check this.

The \verb+remote_echo+ command is implemented via debug printing.
Using Pythons \texttt{print} is recommended.

The view functions \GxsmFile{quick}, \GxsmFile{direct}, \GxsmFile{log}
change the viewmode, but not the button in the main window, don't be
confused.

The waitscan and da0 function are not yet implemented and likely will never
be.

The library detection during compilation is amateurish. Needs work.

Python will check for the
right type of your arguments. Remember, that all values in \GxsmTT{set} are strings
and have to be quoted. Additionaly care for the case sensitivity.

If you you want to pause script execution, use the embedded sleep command
\GxsmTT{gxsm.sleep()} and not \GxsmTT{time.sleep()}, because the function from
the time library will freeze GXSM totally during the sleep.
(This is not a bug, it's a feature.)

% OptPlugInNotes
TODO: Add more action-handlers in Math-PI.
% and clean up inconsistent use of spaces and tabs.

% OptPlugInHints
If you write a particularly interesting remote-script, please give it back
to the community. The GXSM-Forums always welcome input.

% EndPlugInDocuSection
 * --------------------------------------------------------------------------------
 */

#include <gtk/gtk.h>
#include <gtksourceview/gtksource.h>

#include <Python.h>
#define NPY_NO_DEPRECATED_API NPY_1_7_API_VERSION
#include "numpy/arrayobject.h"
#include "numpy/ndarraytypes.h"
#include "numpy/ndarrayobject.h"
#include "numpy/arrayobject.h"

#include <sys/types.h>
#include <signal.h>

#include "glib/gstdio.h"

#include "config.h"
#include "glbvars.h"
#include "plugin.h"
#include "gnome-res.h"
#include "action_id.h"
#include "xsmtypes.h"



#include "gapp_service.h"
#include "xsm.h"
#include "unit.h"
#include "pcs.h"

#include "gxsm_app.h"
#include "gxsm_window.h"
#include "app_view.h"
#include "surface.h"

#include "pyremote.h"

#include "pyscript_templates.h"
#include "pyscript_templates_script_libs.h"

// number of script control EC's -- but must manually match schemata in .xml files!
#define NUM_SCV 10

// Plugin Prototypes
static void pyremote_init( void );
static void pyremote_about( void );
static void pyremote_configure( void );
static void pyremote_cleanup( void );
static void pyremote_run(GtkWidget *w, void *data);

 // Fill in the GxsmPlugin Description here
GxsmPlugin pyremote_pi = {
	  NULL,                   // filled in and used by Gxsm, don't touch !
	  NULL,                   // filled in and used by Gxsm, don't touch !
	  0,                      // filled in and used by Gxsm, don't touch !
	  NULL,                   // The Gxsm-App Class Ref.pointer (called "gapp" in Gxsm) is
	  // filled in here by Gxsm on Plugin load,
	  // just after init() is called !!!
	  // ----------------------------------------------------------------------
	  // Plugins Name, CodeStly is like: Name-M1S[ND]|M2S-BG|F1D|F2D|ST|TR|Misc
	  (char *)"Pyremote",
	  NULL,
	  // Description, is shown by PluginViewer (Plugin: listplugin, Tools->Plugin Details)
	  (char *)"Remote control",
	  // Author(s)
	  (char *) "Stefan Schroeder",
	  // Menupath to position where it is appended to
	  (char *)"tools-section",
	  // Menuentry
	  N_("Pyremote Console"),
	  // help text shown on menu
	  N_("Python Remote Control Console"),
	  // more info...
	  (char *)"See Manual.",
	  NULL,          // error msg, plugin may put error status msg here later
	  NULL,          // Plugin Status, managed by Gxsm, plugin may manipulate it too
	  // init-function pointer, can be "NULL",
	  // called if present at plugin load
	  pyremote_init,
	  // query-function pointer, can be "NULL",
	  // called if present after plugin init to let plugin manage it install itself
	  NULL, // query should be "NULL" for Gxsm-Math-Plugin !!!
	  // about-function, can be "NULL"
	  // can be called by "Plugin Details"
	  pyremote_about,
	  // configure-function, can be "NULL"
	  // can be called by "Plugin Details"
	  pyremote_configure,
	  // run-function, can be "NULL", if non-Zero and no query defined,
	  // it is called on menupath->"plugin"
	  pyremote_run, // run should be "NULL" for Gxsm-Math-Plugin !!!
	  // cleanup-function, can be "NULL"
	  // called if present at plugin removal
	  NULL, // direct menu entry callback1 or NULL
	  NULL, // direct menu entry callback2 or NULL

	  pyremote_cleanup
};

class py_gxsm_console;

// GXSM PY REMOTE GUI/CONSOLE CLASS
py_gxsm_console *py_gxsm_remote_console = NULL;

// Text used in Aboutbox, please update!!a
static const char *about_text = N_("Gxsm Plugin\n\n"
                                   "Python Remote Control.");

// Symbol "get_gxsm_plugin_info" is resolved by dlsym from Gxsm, used to get Plugin's info!!
GxsmPlugin *get_gxsm_plugin_info ( void ){
	pyremote_pi.description = g_strdup_printf(N_("Gxsm pyremote plugin %s"), VERSION);
	return &pyremote_pi;
}

// 5.) Start here with the plugins code, vars def., etc.... here.
// ----------------------------------------------------------------------
//
// TODO:
// More error-handling
// Cannot return int in run
// fktname editable in preferences.
// Add numeric interface (LOW)
// Add image interface (LOW)
// Add i/o possibility (LOW)

// about-Function
static void pyremote_about(void)
{
	const gchar *authors[] = { pyremote_pi.authors, NULL};
	gtk_show_about_dialog (NULL, 
			       "program-name",  pyremote_pi.name,
			       "version", VERSION,
			       "license", GTK_LICENSE_GPL_3_0,
			       "comments", about_text,
			       "authors", authors,
			       NULL
			       );
}

// configure-Function
static void pyremote_configure(void)
{
	if(pyremote_pi.app){
		pyremote_pi.app->message("Pyremote Plugin Configuration");
	}
}

typedef struct {
        GList *plugins;
        PyObject *module;
        PyObject *dict;
        PyObject *main_module;
} PyGxsmModuleInfo;

static PyGxsmModuleInfo py_gxsm_module;


class py_gxsm_console;
typedef struct {
        const gchar *cmd;
        int mode;
        PyObject *dictionary;
        PyObject *ret;
        py_gxsm_console *pygc;
        PyThreadState *py_state_save;
} PyRunThreadData;


static GMutex g_list_mutex;

static GMutex mutex;
#define WAIT_JOIN_MAIN {gboolean tmp; do{ g_usleep (10000); g_mutex_lock (&mutex); tmp=idle_data.wait_join; g_mutex_unlock (&mutex); }while(tmp);}
#define UNSET_WAIT_JOIN_MAIN g_mutex_lock (&mutex); idle_data->wait_join=false; g_mutex_unlock (&mutex)


typedef struct {
        remote_args ra;
        const gchar *string;
        PyObject *self;
        PyObject *args;
        gint ret;
        gboolean wait_join;
        double vec[4];
        gint64 i64;
        gchar  c;
} IDLE_from_thread_data;


class py_gxsm_console : public AppBase{
public:
	py_gxsm_console (Gxsm4app *app):AppBase(app){
                script_filename = NULL;
                gui_ready = false;
                user_script_running = 0;
                action_script_running = 0;
                run_data.cmd = NULL;
                run_data.mode = 0;
                run_data.dictionary = NULL;
                run_data.ret  = NULL;
                message_list  = NULL;
                initialize ();
                g_mutex_init (&mutex);
        };
	virtual ~py_gxsm_console ();
        
        virtual void AppWindowInit(const gchar *title, const gchar *sub_title=NULL);
	
        void initialize(void);
        PyObject* run_string(const char *cmd, int type, PyObject *g, PyObject *l);
        void show_stderr(const gchar *str);
        void initialize_stderr_redirect(PyObject *d);
        void destroy_environment(PyObject *d, gboolean show_errors);
        PyObject* create_environment(const gchar *filename, gboolean show_errors);

        static void PyRun_GAsyncReadyCallback (GObject *source_object,
                                               GAsyncResult *res,
                                               gpointer user_data);

        static void PyRun_GTaskThreadFunc (GTask *task,
                                           gpointer source_object,
                                           gpointer task_data,
                                           GCancellable *cancellable);
        
        static gpointer PyRun_GThreadFunc (gpointer data);
        
        const char* run_command(const gchar *cmd, int mode);

        void push_message_async (const gchar *msg){
                g_mutex_lock (&g_list_mutex);
                if (msg)
                        message_list = g_slist_prepend (message_list, g_strdup(msg));
                else
                        message_list = g_slist_prepend (message_list, NULL); // push self terminate IDLE task mark
                g_mutex_unlock (&g_list_mutex);
                g_idle_add (pop_message_list_to_console, this); // keeps running and watching for async console data to display
        }

        static gboolean pop_message_list_to_console (gpointer user_data){
                py_gxsm_console *pygc = (py_gxsm_console*) user_data;

                g_mutex_lock (&g_list_mutex);
                if (!pygc->message_list){
                        g_mutex_unlock (&g_list_mutex);
                        return G_SOURCE_REMOVE;
                }
                GSList* last = g_slist_last (pygc->message_list);
                if (!last){
                        g_mutex_unlock (&g_list_mutex);
                        return G_SOURCE_REMOVE;
                }
                if (last -> data)  {
                        pygc->append (last -> data);
                        g_free (last -> data);
                        pygc->message_list = g_slist_delete_link (pygc->message_list, last);
                        g_mutex_unlock (&g_list_mutex);
                        return G_SOURCE_REMOVE;
                } else { // NULL data mark found
                        pygc->message_list = g_slist_delete_link (pygc->message_list, last);
                        g_mutex_unlock (&g_list_mutex);
                        pygc->append ("--END IDLE--");
                        return G_SOURCE_REMOVE; // finish IDLE task
                }
        }

        void append (const gchar *msg);

        gchar *pre_parse_script (const gchar *script, int *n_lines=NULL, int r=0); // parse script for gxsm lib include statements

        static void open_file_callback_exec (GtkDialog *dialog,  int response, gpointer user_data);
        static void open_file_callback (GSimpleAction *action, GVariant *parameter, gpointer user_data);
        static void open_action_script_callback (GSimpleAction *action, GVariant *parameter, gpointer user_data);
        static void save_file_callback (GSimpleAction *action, GVariant *parameter, gpointer user_data);
        static void save_file_as_callback_exec (GtkDialog *dialog,  int response, gpointer user_data);
        static void save_file_as_callback (GSimpleAction *action, GVariant *parameter, gpointer user_data);
        static void configure_callback (GSimpleAction *action, GVariant *parameter, gpointer user_data);

        static void run_file (GtkButton *btn, gpointer user_data);
        static void kill (GtkToggleButton *btn, gpointer user_data);

        void create_gui(void);
        void run();

        static void command_execute(GtkEntry *entry, gpointer user_data);
        static void clear_output(GtkButton *btn, gpointer user_data);
        // static gboolean check_func(PyObject *m, gchar *name, gchar *filename);


        gchar *get_gxsm_script (const gchar *name){
                gchar *tmp_script = NULL;
                gchar* path = g_strconcat (g_get_home_dir (), "/.gxsm4/pyaction", NULL);
                gchar* tmp_script_filename = g_strconcat (path, "/", name, ".py", NULL);
                g_free (path);
                if (g_file_test (tmp_script_filename, G_FILE_TEST_EXISTS)){
                        GError *err = NULL;
                        if (!g_file_get_contents(tmp_script_filename,
                                                 &tmp_script,
                                                 NULL,
                                                 &err)) {
                                gchar *message = g_strdup_printf("Cannot read content of file "
                                                                 "'%s': %s",
                                                                 tmp_script_filename,
                                                                 err->message);
                                append (message);
                                main_get_gapp()->warning (message);
                                g_free(message);
                        }
                } else {
                        gchar *message = g_strdup_printf("Action script/library %s not yet defined.\nPlease define action script using the python console.", tmp_script_filename);
                        g_message ("%s", message);
                        append(message);
                        main_get_gapp()->warning (message);
                        g_free(message);
                }
                g_free (tmp_script_filename);
                return tmp_script;
        };
        
        void run_action_script (const gchar *name){
                gchar *tmp_script = get_gxsm_script (name);
                if (tmp_script){
                        gchar *tmp = g_strdup_printf ("%s #jobs[%d]+1", N_("\n>>> Executing action script: "), action_script_running);
                        append (tmp);
                        g_free (tmp);
                        append (name);
                        append ("\n");
                        action_script_running++;
                        const gchar *output = run_command(tmp_script, Py_file_input);
                        --action_script_running;
                        g_free (tmp_script);
                        append (output);
                        append (N_("\n<<< Action script finished: "));
                        append (name);
                        append ("\n");
                }
        };
        
        void set_script_filename (const gchar *name = NULL){
                if (name){
                        if (script_filename)
                                g_free (script_filename);
                        if (strstr (name, ".py")){
                                script_filename = g_strdup (name);
                        } else {
                                gchar* path = g_strconcat (g_get_home_dir (), "/.gxsm4/pyaction", NULL);
                                // attempt to create folder if not exiting
                                if (!g_file_test (path, G_FILE_TEST_IS_DIR)){
                                        g_message ("creating action script folder %s", path);
                                        g_mkdir_with_parents (path, 0777);
                                }
                                script_filename = g_strconcat (path, "/", name, ".py", NULL);
                                g_free (path);
                                if (!g_file_test (script_filename, G_FILE_TEST_EXISTS)){
                                        if (strstr (script_filename, "gxsm4-script")){
                                                // make sample
                                                std::ofstream example_file;
                                                example_file.open(script_filename);
                                                example_file << "# Example python gxsm4 script file " << script_filename << " was created.\n"
                                                        "# this is the gxsm developer test and work script - see the Gxsm4 manual for more information\n";
                                                example_file << template_demo_script;
                                                example_file.close();
                                        } else if (strstr (script_filename, "gxsm4-help")){
                                                // make sample
                                                std::ofstream example_file;
                                                example_file.open(script_filename);
                                                example_file << "# Gxsm Python Script file " << script_filename << " was created.\n";
                                                example_file << template_help;
                                                example_file.close();
                                        } else if (strstr (script_filename, "gxsm4-data-access-template")){
                                                // make sample
                                                std::ofstream example_file;
                                                example_file.open(script_filename);
                                                example_file << "# Gxsm Python Script file " << script_filename << " was created.\n";
                                                example_file << template_data_access;
                                                example_file.close();
                                        } else if (strstr (script_filename, "gxsm4-control-watchdog")){
                                                // make sample
                                                std::ofstream example_file;
                                                example_file.open(script_filename);
                                                example_file << "# Gxsm Python Script file " << script_filename << " was created.\n";
                                                example_file << template_watchdog;
                                                example_file.close();
                                        } else if (strstr (script_filename, "gxsm4-control-gxsm-sok-server")){
                                                // make sample
                                                std::ofstream example_file;
                                                example_file.open(script_filename);
                                                example_file << "# Gxsm Python Script file " << script_filename << " was created.\n";
                                                example_file << template_gxsm_sok_server;
                                                example_file.close();
                                        } else if (strstr (script_filename, "gxsm4-data-cfextract-simple")){
                                                // make sample
                                                std::ofstream example_file;
                                                example_file.open(script_filename);
                                                example_file << "# Gxsm Python Script file " << script_filename << " was created.\n";
                                                example_file << template_data_cfextract_simple;
                                                example_file.close();
                                        } else if (strstr (script_filename, "gxsm4-data-cfextract-data")){
                                                // make sample
                                                std::ofstream example_file;
                                                example_file.open(script_filename);
                                                example_file << "# Gxsm Python Script file " << script_filename << " was created.\n";
                                                example_file << template_data_cfextract_data;
                                                example_file.close();
                                        } else if (strstr (script_filename, "gxsm4-3d-animate")){
                                                // make sample
                                                std::ofstream example_file;
                                                example_file.open(script_filename);
                                                example_file << "# Gxsm Python Surface3D/GL Animation Example script file " << script_filename << " was created.\n\n";
                                                example_file << template_animate;
                                                example_file.close();
                                        } else if (strstr (script_filename, "gxsm4-movie-export")){
                                                // make sample
                                                std::ofstream example_file;
                                                example_file.open(script_filename);
                                                example_file << "# Gxsm Python Script for Multi layer/time series Drawing Export " << script_filename << " was created.\n\n";
                                                example_file << template_movie_drawing_export;
                                                example_file.close();
                                        } else if (strstr (script_filename, "gxsm4-lib-utils")){
                                                // make sample
                                                std::ofstream example_file;
                                                example_file.open(script_filename);
                                                example_file << "# Gxsm Python Script Library: " << script_filename << " was created.\n\n";
                                                example_file << template_library_utils;
                                                example_file.close();
                                        } else if (strstr (script_filename, "gxsm4-lib-control")){
                                                // make sample
                                                std::ofstream example_file;
                                                example_file.open(script_filename);
                                                example_file << "# Gxsm Python Script Library: " << script_filename << " was created.\n\n";
                                                example_file << template_library_control;
                                                example_file.close();
                                        } else if (strstr (script_filename, "gxsm4-lib-scan")){
                                                // make sample
                                                std::ofstream example_file;
                                                example_file.open(script_filename);
                                                example_file << "# Gxsm Python Script Library: " << script_filename << " was created.\n\n";
                                                example_file << template_library_scan;
                                                example_file.close();
                                        } else if (strstr (script_filename, "gxsm4-lib-probe")){
                                                // make sample
                                                std::ofstream example_file;
                                                example_file.open(script_filename);
                                                example_file << "# Gxsm Python Script Library: " << script_filename << " was created.\n\n";
                                                example_file << template_library_probe;
                                                example_file.close();
                                        } else if (strstr (script_filename, "gxsm4-lib-analysis")){
                                                // make sample
                                                std::ofstream example_file;
                                                example_file.open(script_filename);
                                                example_file << "# Gxsm Python Script Library: " << script_filename << " was created.\n\n";
                                                example_file << template_library_analysis;
                                                example_file.close();
                                        } else {
                                                // make sample
                                                std::ofstream example_file;
                                                example_file.open(script_filename);
                                                example_file << "# Example python action script file " << script_filename << " was created.\n"
                                                        "# - see the Gxsm4 manual for more information\n"
                                                        "gxsm.set (\"dsp-fbs-bias\",\"0.1\") # set Bias to 0.1V\n"
                                                        "gxsm.set (\"dsp-fbs-mx0-current-set\",\"0.01\") # Set Current Setpoint to 0.01nA\n"
                                                        "# gxsm.sleep (2)  # sleep for 2/10s\n";
                                                example_file.close();
                                        }
                                }
                        }
                }
                if (script_filename)
                        SetTitle (pyremote_pi.help, script_filename);
                else
                        SetTitle (pyremote_pi.help, "no valid file name");
        };
        void write_example_file (void);

        void fix_eols_to_unix (gchar *text);


        gboolean set_sc_label (const gchar *id, const gchar *l){
                // *id = g_strdup_printf ("py-sc%02d",i+1);
                if (strlen(id) < 7) return false;
                int i = atoi(&id[5]);
                if (i>0 && i <= NUM_SCV){
                        gtk_label_set_text (GTK_LABEL(sc_label[i-1]) ,l);
                        return true;
                } else return false;
        };
        
private:
        PyRunThreadData run_data;
        GSList *message_list;
        gboolean gui_ready;
        gint user_script_running;
        gint action_script_running;
        
        GSettings *gsettings;
        GtkWidget *file_menu;

        //PyGxsmModuleInfo py_gxsm_module;
        const char *example_filename = "gxsm_pyremote_example.py";

        // Console GUI elemets
        PyObject *std_err;
        PyObject *dictionary;
        GtkWidget *console_output;
        GtkTextMark *console_mark_end;
        GtkWidget *console_file_content;
        gchar *script_filename;
        gboolean query_filename;
        gboolean fail;
        gdouble exec_value;
        gdouble sc_value[NUM_SCV];
        GtkWidget *sc_label[NUM_SCV];
};



///////////////////////////////////////////////////////////////
// BLOCK I -- generic, help, get/set data, actions
///////////////////////////////////////////////////////////////

/* stolen from app_remote.C */
static void Check_ec(Gtk_EntryControl* ec, remote_args* ra){
	ec->CheckRemoteCmd (ra); // only reading PCS is thread safe!
};

static void Check_conf(GnomeResPreferences* grp, remote_args* ra){
        if (grp && ra)
                gnome_res_check_remote_command (grp, ra);
};

static void CbAction_ra(remote_action_cb* ra, gpointer arglist){
	if(ra->cmd && ((gchar**)arglist)[1])
		if(! strcmp(((gchar**)arglist)[1], ra->cmd)){
			if (ra->data)
				(*ra->RemoteCb) (ra->widget, ra->data);
			else
				(*ra->RemoteCb) (ra->widget, arglist);
			// see above and pcs.h
		}
};

static PyObject* remote_help(PyObject *self, PyObject *args);

/* This function will build and return a python tuple
   that contains all the objects (string name) that
   you can 'set' and 'get'.

   Example output of 'print gxsm.list()':

   ('Counter', 'VOffsetZ', 'VRangeZ', 'Rotation',
   'TimeSelect', 'Time', 'LayerSelect', 'Layers',
   'OffsetY', 'OffsetX', 'PointsY', 'PointsX',
   'StepsY', 'StepsX', 'RangeY', 'RangeX')

   when no hardware is attached.

*/
static PyObject* remote_listr(PyObject *self, PyObject *args)
{
	int slen = g_slist_length( main_get_gapp()->RemoteEntryList ); // How many entries?

	// This will be our return object with as many slots as input list has:
	PyObject *ret = PyTuple_New(slen);
	GSList* tmp = main_get_gapp()->RemoteEntryList;
	for (int n=0; n<slen; n++){
                Gtk_EntryControl* ec = (Gtk_EntryControl*)tmp->data; // Look at data item in GSList.
                PyTuple_SetItem(ret, n, PyUnicode_FromString(ec->get_refname())); // Add Refname to Return-list
                tmp = g_slist_next(tmp);
        }

	return ret;
}

static PyObject* remote_lista(PyObject *self, PyObject *args)
{
	int slen = g_slist_length( main_get_gapp()->RemoteActionList ); // How many entries?

	// This will be our return object with as many slots as input list has:
	PyObject *ret = PyTuple_New(slen);
	GSList* tmp = main_get_gapp()->RemoteActionList;
	for (int n=0; n<slen; n++){
                remote_action_cb* ra = (remote_action_cb*)tmp->data; // Look at data item in GSList.
                PyTuple_SetItem(ret, n, PyUnicode_FromString(ra->cmd)); // Add Refname to Return-list
                tmp = g_slist_next(tmp);
        }

	return ret;
}

static PyObject* remote_gets(PyObject *self, PyObject *args)
{
	PI_DEBUG(DBG_L2, "pyremote: Getting as string ");
	gchar *parameter;

	if (!PyArg_ParseTuple(args, "s", &parameter))
		return Py_BuildValue("i", -1);

	int parameterlen = strlen(parameter);
	int slen = g_slist_length( main_get_gapp()->RemoteEntryList );

	gchar *ret = NULL;

	GSList* tmp = main_get_gapp()->RemoteEntryList;
	for (int n=0; n<slen; n++)
		{
			Gtk_EntryControl* ec = (Gtk_EntryControl*)tmp->data;

			if (strncmp(parameter, ec->get_refname(), parameterlen) == 0)
				{
					ret = g_strdup(ec->Get_UsrString());
				}
			tmp = g_slist_next(tmp);
		}

	if (ret == NULL) // If the parameter doesn't exist.
		{
			ret = g_strdup("ERROR");
		}

	return Py_BuildValue("s", ret);
}


// Getting value in current user unit as plain double number -- could also/option get as string with unit
static PyObject* remote_get(PyObject *self, PyObject *args)
{
	PI_DEBUG(DBG_L2, "pyremote: Getting ");
	gchar *parameter;
	remote_args ra;
	ra.qvalue = 0.;

	if (!PyArg_ParseTuple(args, "s", &parameter))
		return Py_BuildValue("i", -1);

	PI_DEBUG(DBG_L2, parameter << " query" );

	ra.qvalue = 0.;
	gchar *list[] = {(gchar *)"get", parameter, NULL};
	ra.arglist = list;

	g_slist_foreach(main_get_gapp()->RemoteEntryList, (GFunc) Check_ec, (gpointer)&ra);
	PI_DEBUG(DBG_L2, parameter << " query result: " << ra.qvalue );

        if (ra.qstr)
                return Py_BuildValue("s", ra.qstr);
        else
                return Py_BuildValue("f", ra.qvalue);
}

static gboolean main_context_set_entry_from_thread (gpointer user_data){
        IDLE_from_thread_data *idle_data = (IDLE_from_thread_data *) user_data;
        // NOT THREAD SAFE GUI OPERATION TRIGGER HERE
        
	PI_DEBUG_GM (DBG_L2, "pyremote: main_context_set_entry_from_thread start %s %s %s", idle_data->ra.arglist[0], idle_data->ra.arglist[1], idle_data->ra.arglist[2] );
        // check PCS entries
	g_slist_foreach (main_get_gapp()->RemoteEntryList, (GFunc) Check_ec, (gpointer)&(idle_data->ra));

        // check current active/open CONFIGURE elements
	g_slist_foreach (main_get_gapp()->RemoteConfigureList, (GFunc) Check_conf, (gpointer)&(idle_data->ra));

	PI_DEBUG_GM (DBG_L2, "pyremote: main_context_set_entry_from_thread end");
        UNSET_WAIT_JOIN_MAIN;
        
        return G_SOURCE_REMOVE;
}

static PyObject* remote_set(PyObject *self, PyObject *args)
{
	PI_DEBUG(DBG_L2, "pyremote: Setting ");
	gchar *parameter, *value;
        IDLE_from_thread_data idle_data;
 
	if (!PyArg_ParseTuple(args, "ss", &parameter, &value))
		return Py_BuildValue("i", -1);

	PI_DEBUG_GM (DBG_L2, "%s to %s", parameter, value );

	idle_data.ra.qvalue = 0.;
	gchar *list[] = { (char *)"set", parameter, value, NULL };
	idle_data.ra.arglist = list;
	idle_data.wait_join = true;

	PI_DEBUG_GM (DBG_L2, "IDLE START" );
        g_idle_add (main_context_set_entry_from_thread, (gpointer)&idle_data);
	PI_DEBUG_GM (DBG_L2, "IDLE WAIT JOIN" );
        WAIT_JOIN_MAIN;
	PI_DEBUG_GM (DBG_L2, "IDLE DONE" );

	return Py_BuildValue("i", 0);
}

static gboolean main_context_action_from_thread (gpointer user_data){
        IDLE_from_thread_data *idle_data = (IDLE_from_thread_data *) user_data;
        // NOT THREAD SAFE GUI OPERATION TRIGGER HERE
	gchar *parameter;
        gchar *value;
        idle_data->ret = -1;

	if (!PyArg_ParseTuple(idle_data->args, "s|s", &parameter, &value)){
                UNSET_WAIT_JOIN_MAIN;
                return G_SOURCE_REMOVE;
        }

	PI_DEBUG(DBG_L2, "pyremote Action ** idle cb: value:" << parameter << ", " << value );

	gchar *list[] = {(char *)"action", parameter, value, NULL};
	g_slist_foreach(gapp->RemoteActionList, (GFunc) CbAction_ra, (gpointer)list);
        idle_data->ret = 0;

        UNSET_WAIT_JOIN_MAIN;
        return G_SOURCE_REMOVE;
}

static PyObject* remote_action(PyObject *self, PyObject *args)
{
	PI_DEBUG(DBG_L2, "pyremote: Action ") ;
        IDLE_from_thread_data idle_data;
        idle_data.self = self;
        idle_data.args = args;
        idle_data.wait_join = true;
        g_idle_add (main_context_action_from_thread, (gpointer)&idle_data);
        WAIT_JOIN_MAIN;
        return Py_BuildValue("i", idle_data.ret);
}


static gboolean main_context_idle_rtquery (gpointer user_data){
        IDLE_from_thread_data *idle_data = (IDLE_from_thread_data *) user_data;
        // NOT THREAD SAFE GUI OPERATIONS

	double u,v,w;
	gint64 ret = gapp->xsm->hardware->RTQuery ( idle_data->string, u,v,w);
        idle_data->vec[0]=u;
        idle_data->vec[1]=v;
        idle_data->vec[2]=w;
        idle_data->i64=ret;
        idle_data->ret = 0;

        UNSET_WAIT_JOIN_MAIN;
        return G_SOURCE_REMOVE;
}

static gint64 idle_rtquery(const gchar *m, double &x, double &y, double &z)
{
	PI_DEBUG(DBG_L2, "IDLE RTQuery ") ;
        IDLE_from_thread_data idle_data;
        idle_data.string = m;
        idle_data.wait_join = true;
        g_idle_add (main_context_idle_rtquery, (gpointer)&idle_data);
        WAIT_JOIN_MAIN;

        x=idle_data.vec[0];
        y=idle_data.vec[1];
        z=idle_data.vec[2];
        return idle_data.i64;
}



// asks HwI via RTQuery for real time watches -- depends on HwI and it's capabilities/availabel options
/* Hardware realtime monitoring -- all optional */
/* default properties are
 * "X" -> current realtime tip position in X, inclusive rotation and offset
 * "Y" -> current realtime tip position in Y, inclusive rotation and offset
 * "Z" -> current realtime tip position in Z
 * "xy" -> X and Y
 * "zxy" -> Z, X, Y [mk2/3]
 * "o" -> Z, X, Y-Offset [mk2/3]
 * "f" -> feedback watch: f0, I, Irms as read on PanView [mk2/3]
 * "s" -> status bits [FB,SC,VP,MV,(PAC)], [DSP load], [DSP load peak]  [mk2/3]
 * "i" -> GPIO watch -- speudo real time, may be chached by GXSM: out, in, dir  [mk2/3]
 * "U" -> current bias
 */

static gboolean main_context_rtquery_from_thread (gpointer user_data){
        IDLE_from_thread_data *idle_data = (IDLE_from_thread_data *) user_data;
        // NOT THREAD SAFE GUI OPERATION TRIGGER HERE
	gchar *parameter=NULL;
        idle_data->ret = -1;

	if (!PyArg_ParseTuple(idle_data->args, "s", &parameter)){
                UNSET_WAIT_JOIN_MAIN;
                return G_SOURCE_REMOVE;
        }

	double u,v,w;
	gint64 ret = gapp->xsm->hardware->RTQuery (parameter, u,v,w);
        idle_data->c=parameter[0];
        idle_data->vec[0]=u;
        idle_data->vec[1]=v;
        idle_data->vec[2]=w;
        idle_data->i64=ret;
        idle_data->ret = 0;

        UNSET_WAIT_JOIN_MAIN;
        return G_SOURCE_REMOVE;
}

static PyObject* remote_rtquery(PyObject *self, PyObject *args)
{
	PI_DEBUG(DBG_L2, "pyremote: RTQuery ") ;
        IDLE_from_thread_data idle_data;
        idle_data.self = self;
        idle_data.args = args;
        idle_data.wait_join = true;
        g_idle_add (main_context_rtquery_from_thread, (gpointer)&idle_data);
        WAIT_JOIN_MAIN;

        if (idle_data.c == 'm'){
                static char uu[10] = { 0,0,0,0, 0,0,0,0, 0,0};
                strncpy ((char*) uu, (char*)&idle_data.i64, 8);
                return Py_BuildValue("fffs", idle_data.vec[0], idle_data.vec[1], idle_data.vec[2], uu);
        } else
                return Py_BuildValue("fff",  idle_data.vec[0], idle_data.vec[1], idle_data.vec[2]);
}

// asks HwI via RTQuery for real time watches -- depends on HwI and it's capabilities/availabel options
static PyObject* remote_y_current(PyObject *self, PyObject *args)
{
	PI_DEBUG(DBG_L2, "pyremote: y_current ") ;
	//gchar *parameter;

	gint y = main_get_gapp()->xsm->hardware->RTQuery ();

	return Py_BuildValue("i", y);
}

static gboolean main_context_remote_moveto_scan_xy_from_thread (gpointer user_data){
        IDLE_from_thread_data *idle_data = (IDLE_from_thread_data *) user_data;
        // NOT THREAD SAFE GUI OPERATION TRIGGER HERE

        main_get_gapp()->xsm->hardware->MovetoXY
                (R2INT(main_get_gapp()->xsm->Inst->XA2Dig(main_get_gapp()->xsm->data.s.sx)),
                 R2INT(main_get_gapp()->xsm->Inst->YA2Dig(main_get_gapp()->xsm->data.s.sy)));
        
        main_get_gapp()->spm_update_all ();

        UNSET_WAIT_JOIN_MAIN;
        return G_SOURCE_REMOVE;
}

static PyObject* remote_moveto_scan_xy(PyObject *self, PyObject *args)
{
	PI_DEBUG(DBG_L2, "pyremote: Moveto Scan XY ");
	//remote_args ra;
	double x, y;
        
	if (!PyArg_ParseTuple(args, "dd", &x, &y))
		return Py_BuildValue("i", -1);

	PI_DEBUG(DBG_L2, x << ", " << y );

        if (x >= -main_get_gapp()->xsm->data.s.rx/2 && x <= main_get_gapp()->xsm->data.s.rx/2 &&
            y >= -main_get_gapp()->xsm->data.s.ry/2 && y <= main_get_gapp()->xsm->data.s.ry/2){
        
                main_get_gapp()->xsm->data.s.sx = x;
                main_get_gapp()->xsm->data.s.sy = y;

                IDLE_from_thread_data idle_data;
                idle_data.wait_join = true;
                g_idle_add (main_context_remote_moveto_scan_xy_from_thread, (gpointer)&idle_data);
                WAIT_JOIN_MAIN;
        
                return Py_BuildValue("i", 0);
        } else {
                g_warning ("PyRemote: Set Scan XY: requested position (%g, %g) Ang out of current scan range.", x,y);
                return Py_BuildValue("i", -1);
        }
}


///////////////////////////////////////////////////////////////
// BLOCK II -- scan actions
///////////////////////////////////////////////////////////////

static gboolean main_context_emit_toolbar_action_from_thread (gpointer user_data){
        IDLE_from_thread_data *idle_data = (IDLE_from_thread_data *) user_data;
        // NOT THREAD SAFE GUI OPERATION TRIGGER HERE

	PI_DEBUG_GM (DBG_L2, "pyremote: main_context_emit_toolbar_action  >%s<", idle_data->string);
        main_get_gapp()->signal_emit_toolbar_action (idle_data->string);

        UNSET_WAIT_JOIN_MAIN;
        return G_SOURCE_REMOVE;
}

static PyObject* remote_startscan(PyObject *self, PyObject *args)
{
	PI_DEBUG_GM (DBG_L2, "pyremote: Starting scan");
        IDLE_from_thread_data idle_data;
        idle_data.string = "Toolbar_Scan_Start";
        idle_data.wait_join = true;
        g_idle_add (main_context_emit_toolbar_action_from_thread, (gpointer)&idle_data);
	PI_DEBUG_GM (DBG_L2, "pyremote: startscan idle job initiated");
        WAIT_JOIN_MAIN;
	PI_DEBUG_GM (DBG_L2, "pyremote: startscan idle job completed");
	return Py_BuildValue("i", 0);
}

static gboolean main_context_createscan_from_thread (gpointer user_data){
        IDLE_from_thread_data *idle_data = (IDLE_from_thread_data *) user_data;

        // NOT THREAD SAFE GUI OPERATION TRIGGER HERE
        long ch;
	long sizex, sizey, sizev;
	double rangex, rangey;
	PyObject *obj;
        long append=0;

        idle_data->ret = -1;
        
        sizev=1; // try xyv
	if(!PyArg_ParseTuple (idle_data->args, "llllddOl", &ch, &sizex, &sizey, &sizev, &rangex, &rangey, &obj, &append)){
                UNSET_WAIT_JOIN_MAIN;
                return G_SOURCE_REMOVE;
        }
        g_message ("Create Scan: %ld x %ld [x %ld], size %g x %g Ang from python array, append=%ld",sizex, sizey, sizev, rangex, rangey, append);

        Py_buffer view;
        gboolean rf=false;
        if (PyObject_CheckBuffer (obj)){
                if (PyObject_GetBuffer (obj, &view, PyBUF_SIMPLE)){
                        UNSET_WAIT_JOIN_MAIN;
                        return G_SOURCE_REMOVE;
                }
                rf=true;
        }
        if ( (long unsigned int)(view.len / sizeof(long)) != (long unsigned int)(sizex*sizey*sizev) ){
                g_message ("Create Scan: ERROR array len=%ld does not match nx x ny=%ld", view.len / sizeof(long), sizex*sizey);
                UNSET_WAIT_JOIN_MAIN;
                return G_SOURCE_REMOVE;
        }
        g_message ("Create Scan: array len=%ld OK.", view.len / sizeof(long));
        

	//Scan *dst;
	//main_get_gapp()->xsm->ActivateFreeChannel();
	//dst = main_get_gapp()->xsm->GetActiveScan();
	Scan *dst = main_get_gapp()->xsm->GetScanChannel (ch);

        if (dst){
                g_message ("Resize");
                dst->mem2d->Resize (sizex, sizey, sizev, ZD_FLOAT);

                dst->data.s.nx = sizex;
                dst->data.s.ny = sizey;
                dst->data.s.nvalues = sizev;
                dst->data.s.ntimes = 1;
                
                dst->data.s.dx = rangex/(sizex-1);
                dst->data.s.dy = rangey/(sizey-1);
                dst->data.s.dz = 1;
                dst->data.s.rx = rangex;
                dst->data.s.ry = rangey;

                dst->data.s.x0 = 0.;
                dst->data.s.y0 = 0.;
                dst->data.s.alpha = 0.;

                dst->data.ui.SetUser ("User");

                gchar *tmp=g_strconcat ("PyCreate ",
                                        NULL);
                dst->data.ui.SetComment (tmp);
                g_free (tmp);

                g_message ("Convert Data");
                /*Read*/

                long *buf = (long*)view.buf;

                long stridex = sizex;
                long strideyx = stridex*sizey;
                
                for(gint v=0; v<dst->mem2d->GetNv(); v++)
                        for(gint i=0; i<dst->mem2d->GetNy(); i++)
                                for(gint j=0; j<dst->mem2d->GetNx(); j++)
                                        dst->mem2d->data->Z( (double)buf[j+stridex*i+strideyx*v], j, i, v);
                dst->data.orgmode = SCAN_ORG_CENTER;
                dst->mem2d->data->MkXLookup (-dst->data.s.rx/2., dst->data.s.rx/2.);
                dst->mem2d->data->MkYLookup (-dst->data.s.ry/2., dst->data.s.ry/2.);
                dst->mem2d->data->MkVLookup (0, dst->data.s.nvalues-1);

                if (append > 0){
                        // append in time
                        //dst->GetDataSet(data);
                        
                        double t=0., t0=0.;
                        if (!dst->TimeList) // reference to the first frame/image loaded
                                t0 = (double)dst->data.s.tStart;
                        
                        t = (double)dst->data.s.tStart - t0;
                        dst->mem2d->add_layer_information (new LayerInformation ("name", 0., "PyScanCreate"));
                        dst->mem2d->add_layer_information (new LayerInformation ("t",t, "%.2f s"));
                        dst->mem2d->data->update_ranges (0);
                        dst->append_current_to_time_elements (-1, t);
                } else
                        dst->free_time_elements ();
                
                main_get_gapp()->spm_update_all();
                dst->draw();

                if (rf)
                        PyBuffer_Release (&view);

                idle_data->ret = 0;
                UNSET_WAIT_JOIN_MAIN;
                return G_SOURCE_REMOVE;
        }

        if (rf)
                PyBuffer_Release (&view);
        
        UNSET_WAIT_JOIN_MAIN;
        return G_SOURCE_REMOVE;
}

static PyObject* remote_createscan(PyObject *self, PyObject *args)
{
	PI_DEBUG_GM (DBG_L2, "pyremote: Creating scan");
        IDLE_from_thread_data idle_data;
        idle_data.self = self;
        idle_data.args = args;
        idle_data.wait_join = true;

        g_idle_add (main_context_createscan_from_thread, (gpointer)&idle_data);
        WAIT_JOIN_MAIN;

        return Py_BuildValue("i", idle_data.ret);
}

///////////////////////////////////////////////////////////////
static gboolean main_context_createscanf_from_thread (gpointer user_data){
        IDLE_from_thread_data *idle_data = (IDLE_from_thread_data *) user_data;

        // NOT THREAD SAFE GUI OPERATION TRIGGER HERE

	PyObject *obj;

        long ch;
	long sizex, sizey, sizev;
	double rangex, rangey;
        long append=0;

        idle_data->ret = -1;

        sizev=1; // try xyv
	if(!PyArg_ParseTuple (idle_data->args, "llllddOl", &ch, &sizex, &sizey, &sizev, &rangex, &rangey, &obj, &append)){
                UNSET_WAIT_JOIN_MAIN;
                return G_SOURCE_REMOVE;
        }
        g_message ("Create Scan Float: %ld x %ld [x %ld], size %g x %g Ang from python array, append=%ld",sizex, sizey, sizev, rangex, rangey, append);

        Py_buffer view;
        gboolean rf=false;

        if (PyObject_CheckBuffer (obj)){
                if (PyObject_GetBuffer (obj, &view, PyBUF_SIMPLE)){
                        UNSET_WAIT_JOIN_MAIN;
                        return G_SOURCE_REMOVE;
                }
                rf=true;
        }
	if ( (long unsigned int)(view.len / sizeof(float)) != (long unsigned int)(sizex*sizey*sizev) ) {
                g_message ("Create Scan: ERROR array len=%ld does not match nx x ny=%ld", view.len / sizeof(float), sizex*sizey);
                UNSET_WAIT_JOIN_MAIN;
                return G_SOURCE_REMOVE;
	}
        g_message ("Create Scan: array len=%ld OK.", view.len / sizeof(float));
        
	//if (PyObject_AsWriteBuffer(the_array, (void **) &pbuf, (Py_ssize_t*)&blen))
	//	return Py_BuildValue("i", -1);
	//Scan *dst;
	//main_get_gapp()->xsm->ActivateFreeChannel();
	//dst = main_get_gapp()->xsm->GetActiveScan();

	Scan *dst = main_get_gapp()->xsm->GetScanChannel (ch);
        if (dst){
        
                dst->mem2d->Resize (sizex, sizey, sizev, ZD_FLOAT);

                dst->data.s.nx = sizex;
                dst->data.s.ny = sizey;
                dst->data.s.nvalues = sizev;
                dst->data.s.ntimes = 1;

                dst->data.s.dx = rangex/(sizex-1);
                dst->data.s.dy = rangey/(sizey-1);
                dst->data.s.dz = 1;
                dst->data.s.rx = rangex;
                dst->data.s.ry = rangey;

                dst->data.s.x0 = 0.;
                dst->data.s.y0 = 0.;

                dst->data.ui.SetUser("User");

                gchar *tmp = g_strconcat("PyCreate ", NULL);
                dst->data.ui.SetComment(tmp);
                g_free(tmp);

                /*Read */
                float *buf = (float*)view.buf;
                long stridex = sizex;
                long strideyx = stridex*sizey;
                for(gint v=0; v<dst->mem2d->GetNv(); v++)
                        for (gint i = 0; i < dst->mem2d->GetNy(); i++)
                                for (gint j = 0; j < dst->mem2d->GetNx(); j++)
                                        dst->mem2d->data->Z(buf[j + stridex * i + strideyx * v], j, i, v);

                dst->data.orgmode = SCAN_ORG_CENTER;
                dst->mem2d->data->MkXLookup(-dst->data.s.rx / 2.,
                                            dst->data.s.rx / 2.);
                dst->mem2d->data->MkYLookup(-dst->data.s.ry / 2.,
                                            dst->data.s.ry / 2.);
                dst->mem2d->data->MkVLookup (0, dst->data.s.nvalues-1);

                if (append > 0){
                        // append in time
                        //dst->GetDataSet(data);
                        
                        double t=0., t0=0.;
                        if (!dst->TimeList) // reference to the first frame/image loaded
                                t0 = (double)dst->data.s.tStart;
                        
                        t = (double)dst->data.s.tStart - t0;
                        dst->mem2d->add_layer_information (new LayerInformation ("name", 0., "PyScanCreate"));
                        dst->mem2d->add_layer_information (new LayerInformation ("t",t, "%.2f s"));
                        dst->mem2d->data->update_ranges (0);
                        dst->append_current_to_time_elements (-1, t);
                } else
                        dst->free_time_elements ();

                main_get_gapp()->spm_update_all();
                dst->draw();
                dst = NULL;

                if (rf)
                        PyBuffer_Release (&view);

                idle_data->ret = 0;
                UNSET_WAIT_JOIN_MAIN;
                return G_SOURCE_REMOVE;
        }
        
        if (rf)
                PyBuffer_Release (&view);

        UNSET_WAIT_JOIN_MAIN;
        return G_SOURCE_REMOVE;
}


static PyObject *remote_createscanf(PyObject * self, PyObject * args)
{
	PI_DEBUG(DBG_L2, "pyremote: Creating scanf");
        IDLE_from_thread_data idle_data;
        idle_data.self = self;
        idle_data.args = args;
        idle_data.wait_join = true;

        g_idle_add (main_context_createscanf_from_thread, (gpointer)&idle_data);
        WAIT_JOIN_MAIN;

        return Py_BuildValue("i", idle_data.ret);
}

static PyObject* remote_set_scan_unit(PyObject *self, PyObject *args)
{
	PI_DEBUG(DBG_L2, "pyremote: set scan zunit ");
	//remote_args ra;
        int ch;
	gchar *udim, *unitid, *ulabel;

	if (!PyArg_ParseTuple(args, "lsss", &ch, &udim, &unitid, &ulabel))
		return Py_BuildValue("i", -1);

	Scan *dst = main_get_gapp()->xsm->GetScanChannel (ch);
        if (dst){
       
                UnitObj *u = main_get_gapp()->xsm->MakeUnit (unitid, ulabel);
                g_message ("Set Scan Unit %c [%s] in %s", udim[0], u->Label(), u->Symbol());
                switch (udim[0]){
                case 'x': case 'X':
                        dst->data.SetXUnit(u); break;
                case 'y': case 'Y':
                        dst->data.SetYUnit(u); break;
                case 'z': case 'Z':
                        dst->data.SetZUnit(u); break;
                case 'l': case 'L': case 'v': case 'V':
                        dst->data.SetVUnit(u); break;
                case 't': case 'T':
                        dst->data.SetTimeUnit(u); break;
                default:
                        g_message ("Invalid Dimension Id given.");
                        break;
                }
                delete u;
        }
        else {
                g_message ("Invalid channel %d given.", ch);
                return Py_BuildValue("i", -1);
        }
	return Py_BuildValue("i", 0);
}

static PyObject* remote_set_scan_lookup(PyObject *self, PyObject *args)
{
	PI_DEBUG(DBG_L2, "pyremote: set scan lookup ");
	//remote_args ra;
        int ch;
	gchar *udim;
        double start, end;

	if (!PyArg_ParseTuple(args, "lsdd", &ch, &udim, &start, &end))
		return Py_BuildValue("i", -1);

	Scan *dst = main_get_gapp()->xsm->GetScanChannel (ch);
        if (dst){
       
                switch (udim[0]){
                case 'x': case 'X':
                        dst->mem2d->data->MkXLookup(start, end); break;
                case 'y': case 'Y':
                        dst->mem2d->data->MkYLookup(start, end); break;
                case 'l': case 'L': case 'v': case 'V':
                        dst->mem2d->data->MkVLookup(start, end); break;
                default:
                        g_message ("Invalid Dimension Id given.");
                        break;
                }
        }
        else
                return Py_BuildValue("i", -1);

	return Py_BuildValue("i", 0);
}


static PyObject* remote_getgeometry(PyObject *self, PyObject *args)
{
	PI_DEBUG(DBG_L2, "pyremote:getgeometry");

	long ch;

	if (!PyArg_ParseTuple (args, "l", &ch))
		return Py_BuildValue("ddddd", 0., 0., 0., 0., 0.);

	Scan *src = main_get_gapp()->xsm->GetScanChannel (ch);
        if (src)
                return Py_BuildValue("ddddd", src->data.s.rx, src->data.s.ry, src->data.s.x0, src->data.s.y0, src->data.s.alpha);
        else
		return Py_BuildValue("ddddd", 0., 0., 0., 0., 0.);
}

static PyObject* remote_getdifferentials(PyObject *self, PyObject *args)
{
	PI_DEBUG(DBG_L2, "pyremote:getdifferentials");

	long ch;

	if (!PyArg_ParseTuple (args, "l", &ch))
		return Py_BuildValue("dddd", 0., 0., 0., 0.);

	Scan *src = main_get_gapp()->xsm->GetScanChannel (ch);
        if (src)
                return Py_BuildValue("dddd", src->data.s.dx, src->data.s.dy, src->data.s.dz, src->data.s.dl);
        else
		return Py_BuildValue("dddd", 0., 0., 0., 0.);
}

static PyObject* remote_getdimensions(PyObject *self, PyObject *args)
{
	PI_DEBUG(DBG_L2, "pyremote:getdimensions");

	long ch;

	if (!PyArg_ParseTuple (args, "l", &ch))
		return Py_BuildValue("llll", -1, -1, -1, -1);

	Scan *src = main_get_gapp()->xsm->GetScanChannel (ch);
        if (src)
                return Py_BuildValue("llll", src->mem2d->GetNx (), src->mem2d->GetNy (), src->mem2d->GetNv (), src->number_of_time_elements ());
        else
		return Py_BuildValue("llll", -1, -1, -1, -1);
}

static PyObject* remote_getdatapkt(PyObject *self, PyObject *args)
{
	PI_DEBUG(DBG_L2, "pyremote:getdatapkt");

	long ch;
        double x, y, v, t;
        
	if (!PyArg_ParseTuple (args, "ldddd", &ch, &x, &y, &v, &t))
		return Py_BuildValue("d", 0.);

	Scan *src = main_get_gapp()->xsm->GetScanChannel (ch);
        if (src)
                if (t > 0.)
                        return Py_BuildValue("d", src->mem2d->GetDataPktInterpol (x,y,v, src, (int)(t)));
                else
                        return Py_BuildValue("d", src->mem2d->GetDataPktInterpol (x,y,v));
        else
		return Py_BuildValue("d", 0.);
}

static PyObject* remote_putdatapkt(PyObject *self, PyObject *args)
{
	PI_DEBUG(DBG_L2, "pyremote: putdatapkt");
	long ch;
        long x, y, v, t;
        double value;
        
	if (!PyArg_ParseTuple (args, "dlllll", &value, &ch, &x, &y, &v, &t))
		return Py_BuildValue("i", -1);

	Scan *dst = main_get_gapp()->xsm->GetScanChannel (ch);
        if (dst){
                dst->mem2d->PutDataPkt (value, x,y,v);
                return Py_BuildValue("i", 0);
        } else
		return Py_BuildValue("i", -1);
}

//{"get_slice", remote_getslice, METH_VARARGS, "Get Slice/Image: [nx,ny,array]=gxsm.get_slice (ch, v, t)"},

static PyObject* remote_getslice(PyObject *self, PyObject *args)
{
	PI_DEBUG(DBG_L2, "pyremote: getslice");
	long ch,v,t,yi,yn;

	//PyObject *obj;
        
	if (!PyArg_ParseTuple (args, "lllll", &ch, &v, &t, &yi, &yn))
		return Py_BuildValue("i", -1);

	Scan *src =gapp->xsm->GetScanChannel (ch);
        if (src && (yi+yn) <= src->mem2d->GetNy()){
                g_message ("remote_getslice from mem2d scan data in (dz scaled to unit) CH%d, Ys=%d Yf=%d", (int)ch, (int)yi, (int)(yi+yn));

                // PyObject* PyArray_SimpleNewFromData(int nd, npy_intp const* dims, int typenum, void* data);
                // PyObject *PyArray_FromDimsAndData(int n_dimensions, int dimensions[n_dimensions], int item_type, char *data);
                npy_intp dims[2]; 
                dims[0] = yn;
                dims[1] = src->mem2d->GetNx ();
                g_message ("Creating PyArray: shape %d , %d", dims[0], dims[1]);
                double *darr2 = (double*) malloc(sizeof(double) * dims[0]*dims[1]);
                double *dp=darr2;
                int yf = yi+yn;

                for (int y=yi; y<yf; ++y)
                        for (int x=0; x<dims[1]; ++x)
                                *dp++ = src->mem2d->data->Z(x,y)*src->data.s.dz;
                
                PyObject* pyarr = PyArray_SimpleNewFromData(2, dims, NPY_DOUBLE, (void*)darr2);
                PyArray_ENABLEFLAGS((PyArrayObject*) pyarr, NPY_ARRAY_OWNDATA);
                return Py_BuildValue("O", pyarr); // Python code will receive the array as numpy array.
        } else
		return Py_BuildValue("i", -1);

	return Py_BuildValue("i", 0);
}

static PyObject* remote_get_x_lookup(PyObject *self, PyObject *args)
{
	PI_DEBUG(DBG_L2, "pyremote:get_x_lookup");

	long ch, i;

	if (!PyArg_ParseTuple (args, "ll", &ch, &i))
		return Py_BuildValue("d", 0.);

	Scan *src = main_get_gapp()->xsm->GetScanChannel (ch);
        if (src)
                return Py_BuildValue("d", src->mem2d->data->GetXLookup(i));
        else
		return Py_BuildValue("d", 0.);
}

static PyObject* remote_get_y_lookup(PyObject *self, PyObject *args)
{
	PI_DEBUG(DBG_L2, "pyremote:get_x_lookup");

	long ch, i;

	if (!PyArg_ParseTuple (args, "ll", &ch, &i))
		return Py_BuildValue("d", 0.);

	Scan *src = main_get_gapp()->xsm->GetScanChannel (ch);
        if (src)
                return Py_BuildValue("d", src->mem2d->data->GetYLookup(i));
        else
		return Py_BuildValue("d", 0.);
}

static PyObject* remote_get_v_lookup(PyObject *self, PyObject *args)
{
	PI_DEBUG(DBG_L2, "pyremote:get_x_lookup");

	long ch, i;

	if (!PyArg_ParseTuple (args, "ll", &ch, &i))
		return Py_BuildValue("d", 0.);

	Scan *src = main_get_gapp()->xsm->GetScanChannel (ch);
        if (src)
                return Py_BuildValue("d", src->mem2d->data->GetVLookup(i));
        else
		return Py_BuildValue("d", 0.);
}


static PyObject* remote_set_x_lookup(PyObject *self, PyObject *args)
{
	PI_DEBUG(DBG_L2, "pyremote:get_x_lookup");

	long ch, i;
        double v;

	if (!PyArg_ParseTuple (args, "lld", &ch, &i, &v))
		return Py_BuildValue("d", 0.);

	Scan *src = main_get_gapp()->xsm->GetScanChannel (ch);
        if (src){
                src->mem2d->data->SetXLookup(i, v);
                return Py_BuildValue("d", src->mem2d->data->GetXLookup(i));
        } else
		return Py_BuildValue("d", 0.);
}

static PyObject* remote_set_y_lookup(PyObject *self, PyObject *args)
{
	PI_DEBUG(DBG_L2, "pyremote:get_x_lookup");

	long ch, i;
        double v;

	if (!PyArg_ParseTuple (args, "lld", &ch, &i, &v))
		return Py_BuildValue("d", 0.);

	Scan *src = main_get_gapp()->xsm->GetScanChannel (ch);
        if (src){
                src->mem2d->data->SetYLookup(i, v);
                return Py_BuildValue("d", src->mem2d->data->GetYLookup(i));
        } else
		return Py_BuildValue("d", 0.);
}

static PyObject* remote_set_v_lookup(PyObject *self, PyObject *args)
{
	PI_DEBUG(DBG_L2, "pyremote:get_x_lookup");

	long ch, i;
        double v;

	if (!PyArg_ParseTuple (args, "lld", &ch, &i, &v))
		return Py_BuildValue("d", 0.);

	Scan *src = main_get_gapp()->xsm->GetScanChannel (ch);
        if (src){
                src->mem2d->data->SetVLookup(i, v);
                return Py_BuildValue("d", src->mem2d->data->GetVLookup(i));
        } else
		return Py_BuildValue("d", 0.);
}


static PyObject* remote_getobject(PyObject *self, PyObject *args)
{
	PI_DEBUG(DBG_L2, "pyremote:getobject");

	long ch, nth;
        
	if (!PyArg_ParseTuple (args, "ll", &ch, &nth))
		return Py_BuildValue("s", "Invalid Parameters. [ll]: ch, nth");

	Scan *src = main_get_gapp()->xsm->GetScanChannel (ch);
        if (src){
                int n_obj = src->number_of_object ();
                if (nth < n_obj){
                        scan_object_data *obj_data = src->get_object_data (nth);
                        double xy[6] = {0.,0., 0.,0., 0.,0.};
                        if (obj_data){
                                obj_data->get_xy_i_pixel (0, xy[0], xy[1]);
                                if (obj_data->get_num_points () > 1){
                                        obj_data->get_xy_i_pixel (1, xy[2], xy[3]);
                                        if (obj_data->get_num_points () > 2){
                                                obj_data->get_xy_i_pixel (2, xy[4], xy[5]);
                                                return Py_BuildValue("sdddddd", obj_data->get_name (), xy[0], xy[1], xy[2], xy[3], xy[4], xy[5]);
                                        } else
                                                return Py_BuildValue("sdddd", obj_data->get_name (), xy[0], xy[1], xy[2], xy[3]);
                                } else
                                        return Py_BuildValue("sdd", obj_data->get_name (), xy[0], xy[1]);
                        }
                        else
                                return Py_BuildValue("sdd", "None", 0, 0);                        
                }
        }
        return Py_BuildValue("s", "None");
}



static gboolean main_context_getobject_action_from_thread (gpointer user_data){
        IDLE_from_thread_data *idle_data = (IDLE_from_thread_data *) user_data;
        // NOT THREAD SAFE GUI OPERATION TRIGGER HERE
	long ch;
        gchar *objnameid;
        gchar *action;
        idle_data->ret = -1;
        
	if (!PyArg_ParseTuple (idle_data->args, "lss", &ch, &objnameid, &action)){
                UNSET_WAIT_JOIN_MAIN;
                return G_SOURCE_REMOVE;
        }

	Scan *src =gapp->xsm->GetScanChannel (ch);
        if (src){
                int n_obj = src->number_of_object ();
                for (int i=0; i < n_obj; ++i){
                        scan_object_data *obj_data = src->get_object_data (i);
                        if (obj_data){
                                if (!strcmp (action, "REMOVE-ALL")){
                                        if (!strncmp (obj_data->get_name (), objnameid, strlen(objnameid))){ // part match?
                                                ViewControl *vc = src->view->Get_ViewControl ();
                                                vc->remove_object ((VObject *)obj_data, vc);
                                                n_obj = src->number_of_object ();
                                                --i;
                                                idle_data->ret = 0;
                                                continue;
                                        }
                                }
                                if (!strcmp (obj_data->get_name (), objnameid)){ // match?
                                        if (!strcmp (action, "GET-COORDS")){
                                                obj_data->SetUpScan ();
                                                idle_data->ret = 0;
                                        }
                                        else if (!strcmp (action, "SET-OFFSET")){
                                                obj_data->set_offset ();
                                                idle_data->ret = 0;
                                        }
                                        else if (!strncmp (action, "SET-LABEL-TO:",13)){
                                                obj_data->set_object_label (&action[13]);
                                                idle_data->ret = 0;
                                        }
                                        else if (!strcmp (action, "REMOVE")){
                                                ViewControl *vc = src->view->Get_ViewControl ();
                                                vc->remove_object ((VObject *)obj_data, vc);
                                                idle_data->ret = 0;
                                        }
                                        UNSET_WAIT_JOIN_MAIN;
                                        return G_SOURCE_REMOVE;
                                }
                        }
                }
        }
        UNSET_WAIT_JOIN_MAIN;
        return G_SOURCE_REMOVE;
}

static PyObject* remote_getobject_action(PyObject *self, PyObject *args)
{
	PI_DEBUG(DBG_L2, "pyremote:getobject_getcoords_setup_scan");
        IDLE_from_thread_data idle_data;
        idle_data.self = self;
        idle_data.args = args;
        idle_data.wait_join = true;
        g_idle_add (main_context_getobject_action_from_thread, (gpointer)&idle_data);
        WAIT_JOIN_MAIN;
        if (idle_data.ret)
                return Py_BuildValue("s", "Invalid Parameters. [lls]: ch, objname-id, action=[GET_COORDS, SET-OFFSET, SET-LABEL-TO:{LABEL}, REMOVE]");
        else 
                return Py_BuildValue("s", "OK");

}

static gboolean main_context_addmobject_from_thread (gpointer user_data){
        IDLE_from_thread_data *idle_data = (IDLE_from_thread_data *) user_data;
        // NOT THREAD SAFE GUI OPERATION TRIGGER HERE
	long ch,grp,x,y;
        double size = 1.0;
        gchar *id;
        idle_data->ret = -1;
        
	if (!PyArg_ParseTuple (idle_data->args, "lsllld", &ch, &id, &grp, &x, &y, &size)){
		//return Py_BuildValue("s", "Invalid Parameters. [ll]: ch, nth");
                UNSET_WAIT_JOIN_MAIN;
                return G_SOURCE_REMOVE;
        }

	const gchar *marker_group[] = { 
		"*Marker:red", "*Marker:green", "*Marker:blue", "*Marker:yellow", "*Marker:cyan", "*Marker:magenta",  
		NULL };
	PI_DEBUG(DBG_L2, "pyremote:putobject");
      
	Scan *src =gapp->xsm->GetScanChannel (ch);
        if (!strncmp(id,"Rectangle",9)){
                VObject *vo;
                double xy[4];
                gfloat c[4] = { 1.,0.,0.,1.};
                int spc[2][2] = {{0,0},{0,0}};
                int sp00[2] = {1,1};
                src->Pixel2World ((int)round(x-size/2), (int)round(y-size/2), xy[0], xy[1]);
                src->Pixel2World ((int)round(x+size/2), (int)round(y+size/2), xy[2], xy[3]);
                (src->view->Get_ViewControl ())->AddObject (vo = new VObRectangle ((src->view->Get_ViewControl ())->canvas, xy, FALSE, VOBJ_COORD_ABSOLUT, id, 1.0));
                vo->set_obj_name (id);
                vo->set_custom_label_font ("Sans Bold 12");
                vo->set_custom_label_color (c);
                if (grp>0){
                        gfloat fillcolor[4];
                        gfloat outlinecolor[4];
                        for(int j=0; j<4; j++){
                                int sh = 24-(8*j);
                                fillcolor[j] = outlinecolor[j] = (gfloat)((grp&(0xff << sh) >> sh)) / 256.;
                        }
                        outlinecolor[3] = 0.0;
                        vo->set_color_to_custom (fillcolor, outlinecolor);
                }
                vo->set_on_spacetime  (sp00[0] ? FALSE:TRUE, spc[0]);
                vo->set_off_spacetime (sp00[1] ? FALSE:TRUE, spc[1]);
                vo->show_label (true);
                vo->lock_object (true);
                vo->remake_node_markers ();
        } else if (!strncmp(id,"Point",5)){
                VObject *vo;
                double xy[2];
                gfloat c[4] = { 1.,0.,0.,1.};
                int spc[2][2] = {{0,0},{0,0}};
                int sp00[2] = {1,1};
                src->Pixel2World ((int)round(x), (int)round(y), xy[0], xy[1]);
                (src->view->Get_ViewControl ())->AddObject (vo = new VObPoint ((src->view->Get_ViewControl ())->canvas, xy, FALSE, VOBJ_COORD_ABSOLUT, id, 1.0));
                vo->set_obj_name (id);
                vo->set_custom_label_font ("Sans Bold 12");
                vo->set_custom_label_color (c);
                if (grp>0){
                        gfloat fillcolor[4];
                        gfloat outlinecolor[4];
                        for(int j=0; j<4; j++){
                                int sh = 24-(8*j);
                                fillcolor[j] = outlinecolor[j] = (gfloat)((grp&(0xff << sh) >> sh)) / 256.;
                        }
                        outlinecolor[3] = 0.0;
                        vo->set_color_to_custom (fillcolor, outlinecolor);
                }
                vo->set_on_spacetime  (sp00[0] ? FALSE:TRUE, spc[0]);
                vo->set_off_spacetime (sp00[1] ? FALSE:TRUE, spc[1]);
                vo->show_label (true);
                vo->lock_object (true);
                vo->remake_node_markers ();
        } else if (grp == -1 || !strncmp(id,"xy",2)){
                if (size < 0. || size > 10.) size = 1.0;
                grp = 5;
                VObject *vo;
                double xy[2];
                gfloat c[4] = { 1.,0.,0.,1.};
                int spc[2][2] = {{0,0},{0,0}};
                int sp00[2] = {1,1};
                double px,py,pz;
                // gapp->xsm->hardware->RTQuery ("P", px, py, pz); // get Tip Position in pixels
                idle_rtquery ("P", px, py, pz); // get Tip Position in pixels
                src->Pixel2World ((int)round(px), (int)round(py), xy[0], xy[1]);
                gchar *lab = g_strdup_printf ("M%s XYZ=%g,%g,%g",id, px,py,pz);
                (src->view->Get_ViewControl ())->AddObject (vo = new VObPoint ((src->view->Get_ViewControl ())->canvas, xy, FALSE, VOBJ_COORD_ABSOLUT, lab, size));
                g_free (lab);
                vo->set_obj_name (marker_group[grp]);
                vo->set_custom_label_font ("Sans Bold 12");
                vo->set_custom_label_color (c);
                vo->set_on_spacetime  (sp00[0] ? FALSE:TRUE, spc[0]);
                vo->set_off_spacetime (sp00[1] ? FALSE:TRUE, spc[1]);
                vo->show_label (true);
                vo->lock_object (true);
                vo->remake_node_markers ();
        } else {
                if (size < 0. || size > 10.) size = 1.0;

                if (grp < 0 || grp > 6) grp=0; // silently set 0 if out of range
        
                if (src->view->Get_ViewControl ()){
                        VObject *vo;
                        double xy[2];
                        gfloat c[4] = { 1.,0.,0.,1.};
                        int spc[2][2] = {{0,0},{0,0}};
                        int sp00[2] = {1,1};
                        src->Pixel2World ((int)round(x), (int)round(y), xy[0], xy[1]);
                        gchar *lab = g_strdup_printf ("M%s",id);
                        (src->view->Get_ViewControl ())->AddObject (vo = new VObPoint ((src->view->Get_ViewControl ())->canvas, xy, FALSE, VOBJ_COORD_ABSOLUT, lab, size));
                        g_free (lab);
                        vo->set_obj_name (marker_group[grp]);
                        vo->set_custom_label_font ("Sans Bold 12");
                        vo->set_custom_label_color (c);
                        vo->set_on_spacetime  (sp00[0] ? FALSE:TRUE, spc[0]);
                        vo->set_off_spacetime (sp00[1] ? FALSE:TRUE, spc[1]);
                        vo->show_label (true);
                        vo->lock_object (true);
                        vo->remake_node_markers ();
                }
        }
        idle_data->ret = 0;
       
        UNSET_WAIT_JOIN_MAIN;
        return G_SOURCE_REMOVE;
}


static PyObject* remote_addmobject(PyObject *self, PyObject *args)
{
        IDLE_from_thread_data idle_data;
        idle_data.self = self;
        idle_data.args = args;
        idle_data.wait_join = true;
        g_idle_add (main_context_addmobject_from_thread, (gpointer)&idle_data);
        WAIT_JOIN_MAIN;
        if (idle_data.ret)
                return Py_BuildValue("s", "Invalid Parameters. [ll]: ch, nth");
        else 
                return Py_BuildValue("s", "OK");
}

static PyObject* remote_stopscan(PyObject *self, PyObject *args)
{
	PI_DEBUG(DBG_L2, "pyremote: Stopping scan");
        IDLE_from_thread_data idle_data;
        idle_data.string = "Toolbar_Scan_Stop";
        idle_data.wait_join = true;
        g_idle_add (main_context_emit_toolbar_action_from_thread, (gpointer)&idle_data);
        WAIT_JOIN_MAIN;
	return Py_BuildValue("i", 0);
}

static PyObject* remote_waitscan(PyObject *self, PyObject *args)
{
        double x,y,z;
        long block = 0;
	PI_DEBUG_GM (DBG_L2, "pyremote: wait scan");
	if (PyArg_ParseTuple (args, "l", &block)){
                g_usleep(50000);
                if(idle_rtquery ("W",x,y,z) ){
                        if (block){
                                PI_DEBUG_GM (DBG_L2, "pyremote: wait scan (block=%d)-- blocking until ready.", (int) block);
                                while(idle_rtquery ("W",x,y,z) ){
                                        PI_DEBUG_GM (DBG_L2, "pyremote: wait scan blocking, line = %d",gapp->xsm->hardware->RTQuery () );
                                        g_usleep(100000);
                                }
                        }
                        return Py_BuildValue("i",gapp->xsm->hardware->RTQuery () ); // return current y_index of scan
                }else
                        return Py_BuildValue("i", -1); // no scan in progress
        } else {
                PI_DEBUG_GM (DBG_L2, "pyremote: wait scan -- default: blocking until ready.");
                while(idle_rtquery ("W",x,y,z) ){
                        g_usleep(100000);
                        PI_DEBUG_GM (DBG_L2, "pyremote: wait scan, default: block, line = %d",gapp->xsm->hardware->RTQuery () );
                }
        }
        return Py_BuildValue("i", -1); // no scan in progress
}

static PyObject* remote_scaninit(PyObject *self, PyObject *args)
{
	PI_DEBUG_GM (DBG_L2, "pyremote: Initializing scan");
        IDLE_from_thread_data idle_data;
        idle_data.string = "Toolbar_Scan_Init";
        idle_data.wait_join = true;
        g_idle_add (main_context_emit_toolbar_action_from_thread, (gpointer)&idle_data);
        WAIT_JOIN_MAIN;
	return Py_BuildValue("i", 0);
}

static PyObject* remote_scanupdate(PyObject *self, PyObject *args)
{
	PI_DEBUG(DBG_L2, "pyremote: Updating scan (hardware)");
        IDLE_from_thread_data idle_data;
        idle_data.string = "Toolbar_Scan_UpdateParam";
        idle_data.wait_join = true;
        g_idle_add (main_context_emit_toolbar_action_from_thread, (gpointer)&idle_data);
        WAIT_JOIN_MAIN;
	return Py_BuildValue("i", 0);
}

static PyObject* remote_scanylookup(PyObject *self, PyObject *args)
{
	PI_DEBUG(DBG_L2, "pyremote: Scanylookup");
	int value1 = 0;
	double value2 = 0.0;
	if (!PyArg_ParseTuple(args, "ld", &value1, &value2))
		return Py_BuildValue("i", -1);
	PI_DEBUG(DBG_L2,  value1 << " and " << value2 );
	if(value1 && value2){
		gchar *cmd = NULL;
		cmd = g_strdup_printf ("2 %d %g", value1, value2);
		main_get_gapp()->PutPluginData (cmd);
		//main_get_gapp()->signal_emit_toolbar_action ("Toolbar_Scan_SetYLookup");
                IDLE_from_thread_data idle_data;
                idle_data.string = "Toolbar_Scan_SetYLookup";
                idle_data.wait_join = true;
                g_idle_add (main_context_emit_toolbar_action_from_thread, (gpointer)&idle_data);
                WAIT_JOIN_MAIN;
		g_free (cmd);
	}
	return Py_BuildValue("i", 0);
}

static PyObject* remote_scanline(PyObject *self, PyObject *args)
{
	PI_DEBUG(DBG_L2, "pyremote: Scan line");
	int value1 = 0, value2 = 0, value3 = 0;
	if (!PyArg_ParseTuple(args, "lll", &value1, &value2, &value3))
		return Py_BuildValue("i", -1);
	PI_DEBUG(DBG_L2,  value1 << " and " << value2 << " and " << value3);
	PI_DEBUG(DBG_L2, "pyremote: Warning toolbar NYI");
	if(value1){
		gchar *cmd = NULL;
		if(value2 && value3){
			cmd = g_strdup_printf ("3 %d %d %d",
					       value1,
					       value2,
					       value3);
			main_get_gapp()->PutPluginData (cmd);
			//main_get_gapp()->signal_emit_toolbar_action ("Toolbar_Scan_Partial_Line");
                        IDLE_from_thread_data idle_data;
                        idle_data.string = "Toolbar_Scan_Partial_Line";
                        idle_data.wait_join = true;
                        g_idle_add (main_context_emit_toolbar_action_from_thread, (gpointer)&idle_data);
                        WAIT_JOIN_MAIN;
		}
		else{
			cmd = g_strdup_printf ("d %d",
					       value1);
			main_get_gapp()->PutPluginData (cmd);
			//main_get_gapp()->signal_emit_toolbar_action ("Toolbar_Scan_Line");
                        IDLE_from_thread_data idle_data;
                        idle_data.string = "Toolbar_Scan_Line";
                        idle_data.wait_join = true;
                        g_idle_add (main_context_emit_toolbar_action_from_thread, (gpointer)&idle_data);
                        WAIT_JOIN_MAIN;
		}
		g_free (cmd);
	}
	return Py_BuildValue("i", 0);
}

///////////////////////////////////////////////////////////////
// BLOCK III  -- file IO
///////////////////////////////////////////////////////////////

#if 0 // TEMPLATE
static gboolean main_context_TEMPLATE_from_thread (gpointer user_data){
        IDLE_from_thread_data *idle_data = (IDLE_from_thread_data *) user_data;
        // NOT THREAD SAFE GUI OPERATION TRIGGER HERE
	long channel = 0;
        idle_data->ret = -1;
        
	if (!PyArg_ParseTuple(idle_data->args, "l", &channel)){
                UNSET_WAIT_JOIN_MAIN;
                return G_SOURCE_REMOVE;
        }
        main_get_gapp()->xsm->ActivateChannel ((int)channel);
        idle_data->ret = 0;

        UNSET_WAIT_JOIN_MAIN;
        return G_SOURCE_REMOVE;
}
{
        IDLE_from_thread_data idle_data;
        idle_data.string = "Toolbar_Scan_Partial_Line";
        idle_data.self = self;
        idle_data.args = args;
        idle_data.wait_join = true;
        g_idle_add (main_context_TEMPLATE_from_thread, (gpointer)&idle_data);
        WAIT_JOIN_MAIN;
	return Py_BuildValue("i", idle_data.ret);
}
#endif

static gboolean main_context_autosave_from_thread (gpointer user_data){
        IDLE_from_thread_data *idle_data = (IDLE_from_thread_data *) user_data;
        // NOT THREAD SAFE GUI OPERATION TRIGGER HERE
        main_get_gapp()->enter_thread_safe_no_gui_mode();
        main_get_gapp()->auto_save_scans ();
        main_get_gapp()->exit_thread_safe_no_gui_mode();
        
        UNSET_WAIT_JOIN_MAIN;
        return G_SOURCE_REMOVE;
}

static PyObject* remote_autosave(PyObject *self, PyObject *args)
{
	PI_DEBUG(DBG_L2, "pyremote: Save All/Update");
        IDLE_from_thread_data idle_data;
        idle_data.wait_join = true;
        g_idle_add (main_context_autosave_from_thread, (gpointer)&idle_data);
        WAIT_JOIN_MAIN;
	return Py_BuildValue("i", (long)main_get_gapp()->xsm->hardware->RTQuery ());
}


static gboolean main_context_autoupdate_from_thread (gpointer user_data){
        IDLE_from_thread_data *idle_data = (IDLE_from_thread_data *) user_data;
        // NOT THREAD SAFE GUI OPERATION TRIGGER HERE
        main_get_gapp()->enter_thread_safe_no_gui_mode();
        main_get_gapp()->auto_update_scans ();
        main_get_gapp()->exit_thread_safe_no_gui_mode();
        
        UNSET_WAIT_JOIN_MAIN;
        return G_SOURCE_REMOVE;
}

static PyObject* remote_autoupdate(PyObject *self, PyObject *args)
{
	PI_DEBUG(DBG_L2, "pyremote: Save All/Update");
        IDLE_from_thread_data idle_data;
        idle_data.wait_join = true;
        g_idle_add (main_context_autoupdate_from_thread, (gpointer)&idle_data);
        WAIT_JOIN_MAIN;
	return Py_BuildValue("i", (long)main_get_gapp()->xsm->hardware->RTQuery ());
}


static gboolean main_context_save_from_thread (gpointer user_data){
        IDLE_from_thread_data *idle_data = (IDLE_from_thread_data *) user_data;
        // NOT THREAD SAFE GUI OPERATION TRIGGER HERE
	main_get_gapp()->xsm->save(MANUAL_SAVE_AS, NULL, -1, TRUE);
        
        UNSET_WAIT_JOIN_MAIN;
        return G_SOURCE_REMOVE;
}

static PyObject* remote_save(PyObject *self, PyObject *args)
{
	PI_DEBUG(DBG_L2, "pyremote: Save");
        IDLE_from_thread_data idle_data;
        idle_data.wait_join = true;
        g_idle_add (main_context_save_from_thread, (gpointer)&idle_data);
        WAIT_JOIN_MAIN;
	return Py_BuildValue("i", 0);
}


static gboolean main_context_saveas_from_thread (gpointer user_data){
        IDLE_from_thread_data *idle_data = (IDLE_from_thread_data *) user_data;
        // NOT THREAD SAFE GUI OPERATION TRIGGER HERE
	gchar* fname = NULL;
	long channel = 0;
        idle_data->ret = -1;
        
	if (!PyArg_ParseTuple(idle_data->args, "ls", &channel, &fname)){
                UNSET_WAIT_JOIN_MAIN;
                return G_SOURCE_REMOVE;
        }
        
	if (fname){
		main_get_gapp()->xsm->save (MANUAL_SAVE_AS, fname, channel, TRUE);
		//main_get_gapp()->xsm->save(TRUE, fname, channel);
                idle_data->ret = 0;
	}

        UNSET_WAIT_JOIN_MAIN;
        return G_SOURCE_REMOVE;
}

static PyObject* remote_saveas(PyObject *self, PyObject *args)
{
	PI_DEBUG(DBG_L2, "pyremote: Save As ");
        IDLE_from_thread_data idle_data;
        idle_data.self = self;
        idle_data.args = args;
        idle_data.wait_join = true;
        g_idle_add (main_context_saveas_from_thread, (gpointer)&idle_data);
        WAIT_JOIN_MAIN;
	return Py_BuildValue("i", idle_data.ret);
}

static gboolean main_context_load_from_thread (gpointer user_data){
        IDLE_from_thread_data *idle_data = (IDLE_from_thread_data *) user_data;
        // NOT THREAD SAFE GUI OPERATION TRIGGER HERE
	gchar* fname = NULL;
	long channel = 0;
        idle_data->ret = -1;
        
	if (!PyArg_ParseTuple(idle_data->args, "ls", &channel, &fname)){
                UNSET_WAIT_JOIN_MAIN;
                return G_SOURCE_REMOVE;
        }
        
	if (fname){
		main_get_gapp()->xsm->ActivateChannel (channel);
		main_get_gapp()->xsm->load (fname);
                idle_data->ret = 0;
	}

        UNSET_WAIT_JOIN_MAIN;
        return G_SOURCE_REMOVE;
}

static PyObject* remote_load(PyObject *self, PyObject *args)
{
	PI_DEBUG(DBG_L2, "pyremote: Loading ");
        IDLE_from_thread_data idle_data;
        idle_data.self = self;
        idle_data.args = args;
        idle_data.wait_join = true;
        g_idle_add (main_context_load_from_thread, (gpointer)&idle_data);
        WAIT_JOIN_MAIN;
	return Py_BuildValue("i", idle_data.ret);
}



static gboolean main_context_import_from_thread (gpointer user_data){
        IDLE_from_thread_data *idle_data = (IDLE_from_thread_data *) user_data;
        // NOT THREAD SAFE GUI OPERATION TRIGGER HERE
	gchar* fname = NULL;
	long channel = 0;
        idle_data->ret = -1;
        
	if (!PyArg_ParseTuple(idle_data->args, "ls", &channel, &fname)){
                UNSET_WAIT_JOIN_MAIN;
                return G_SOURCE_REMOVE;
        }
        
	if (fname){
		main_get_gapp()->xsm->ActivateChannel (channel);
		main_get_gapp()->xsm->load (fname);
                idle_data->ret = 0;
	}

        UNSET_WAIT_JOIN_MAIN;
        return G_SOURCE_REMOVE;
}


static PyObject* remote_import(PyObject *self, PyObject *args)
{
	PI_DEBUG(DBG_L2, "pyremote: Importing ");
        IDLE_from_thread_data idle_data;
        idle_data.self = self;
        idle_data.args = args;
        idle_data.wait_join = true;
        g_idle_add (main_context_import_from_thread, (gpointer)&idle_data);
        WAIT_JOIN_MAIN;
	return Py_BuildValue("i", idle_data.ret);
}

static gboolean main_context_export_from_thread (gpointer user_data){
        IDLE_from_thread_data *idle_data = (IDLE_from_thread_data *) user_data;
        // NOT THREAD SAFE GUI OPERATION TRIGGER HERE
	gchar* fname = NULL;
	long channel = 0;
        idle_data->ret = -1;
        
	if (!PyArg_ParseTuple(idle_data->args, "ls", &channel, &fname)){
                UNSET_WAIT_JOIN_MAIN;
                return G_SOURCE_REMOVE;
        }
        
	if (fname){
		main_get_gapp()->xsm->ActivateChannel (channel);
		main_get_gapp()->xsm->gnuexport (fname);
                idle_data->ret = 0;
	}

        UNSET_WAIT_JOIN_MAIN;
        return G_SOURCE_REMOVE;
}

static PyObject* remote_export(PyObject *self, PyObject *args)
{
	PI_DEBUG(DBG_L2, "pyremote: Exporting ");
        IDLE_from_thread_data idle_data;
        idle_data.self = self;
        idle_data.args = args;
        idle_data.wait_join = true;
        g_idle_add (main_context_export_from_thread, (gpointer)&idle_data);
        WAIT_JOIN_MAIN;
	return Py_BuildValue("i", idle_data.ret);
}

static gboolean main_context_save_drawing_from_thread (gpointer user_data){
        IDLE_from_thread_data *idle_data = (IDLE_from_thread_data *) user_data;
        // NOT THREAD SAFE GUI OPERATION TRIGGER HERE
	gchar* fname = NULL;
	long channel = 0;
        long time_index = 0;
        long layer_index = 0;

        idle_data->ret = -1;
        
 	if (!PyArg_ParseTuple(idle_data->args, "llls", &channel, &time_index, &layer_index, &fname)){
                UNSET_WAIT_JOIN_MAIN;
                return G_SOURCE_REMOVE;
        }
        
	if (fname){
		main_get_gapp()->xsm->ActivateChannel (channel);
                ViewControl* vc =main_get_gapp()->xsm->GetActiveScan()->view->Get_ViewControl();

                if (!vc){
                        UNSET_WAIT_JOIN_MAIN;
                        return G_SOURCE_REMOVE;
                }
                
                main_get_gapp()->xsm->data.display.vlayer = layer_index;
                main_get_gapp()->xsm->data.display.vframe = time_index;
                App::spm_select_layer (NULL, gapp);
                App::spm_select_time (NULL, gapp);
                
                main_get_gapp()->xsm->GetActiveScan()->mem2d_time_element (time_index)->SetLayer (layer_index);
                vc->view_file_save_drawing (fname);
                idle_data->ret = 0;
	}

        UNSET_WAIT_JOIN_MAIN;
        return G_SOURCE_REMOVE;
}

static PyObject* remote_save_drawing (PyObject *self, PyObject *args)
{
	PI_DEBUG(DBG_L2, "pyremote: save drawing ");
        IDLE_from_thread_data idle_data;
        idle_data.self = self;
        idle_data.args = args;
        idle_data.wait_join = true;
        g_idle_add (main_context_save_drawing_from_thread, (gpointer)&idle_data);
        WAIT_JOIN_MAIN;
	return Py_BuildValue("i", idle_data.ret);
}

///////////////////////////////////////////////////////////////
// BLOCK IV
///////////////////////////////////////////////////////////////

static gboolean main_context_set_view_indices_from_thread (gpointer user_data){
        IDLE_from_thread_data *idle_data = (IDLE_from_thread_data *) user_data;
        // NOT THREAD SAFE GUI OPERATION TRIGGER HERE
	long channel = 0;
        long time_index = 0;
        long layer_index = 0;
        idle_data->ret = -1;
        
	if (!PyArg_ParseTuple(idle_data->args, "lll", &channel, &time_index, &layer_index)){
                UNSET_WAIT_JOIN_MAIN;
                return G_SOURCE_REMOVE;
        }
        
        main_get_gapp()->xsm->ActivateChannel (channel);
        main_get_gapp()->xsm->data.display.vlayer = layer_index;
        main_get_gapp()->xsm->data.display.vframe = time_index;
        App::spm_select_layer (NULL, gapp);
        App::spm_select_time (NULL, gapp);
        main_get_gapp()->xsm->GetActiveScan()->mem2d_time_element (time_index)->SetLayer (layer_index);
        idle_data->ret = 0;

        UNSET_WAIT_JOIN_MAIN;
        return G_SOURCE_REMOVE;
}



static PyObject* remote_set_view_indices (PyObject *self, PyObject *args)
{
	PI_DEBUG(DBG_L2, "pyremote: save drawing ");
        IDLE_from_thread_data idle_data;
        idle_data.self = self;
        idle_data.args = args;
        idle_data.wait_join = true;
        g_idle_add (main_context_set_view_indices_from_thread, (gpointer)&idle_data);
        WAIT_JOIN_MAIN;
	return Py_BuildValue("i", idle_data.ret);
}



static gboolean main_context_autodisplay_from_thread (gpointer user_data){
        IDLE_from_thread_data *idle_data = (IDLE_from_thread_data *) user_data;
        // NOT THREAD SAFE GUI OPERATION TRIGGER HERE
        main_get_gapp()->enter_thread_safe_no_gui_mode();
        main_get_gapp()->xsm->ActiveScan->auto_display();
        main_get_gapp()->exit_thread_safe_no_gui_mode();
        
        UNSET_WAIT_JOIN_MAIN;
        return G_SOURCE_REMOVE;
}


static PyObject* remote_autodisplay(PyObject *self, PyObject *args)
{
	PI_DEBUG(DBG_L2, "pyremote: Autodisplay");
        if (!main_get_gapp()->xsm->ActiveScan)
                return Py_BuildValue("i", -1);

        IDLE_from_thread_data idle_data;
        idle_data.wait_join = true;
        g_idle_add (main_context_autodisplay_from_thread, (gpointer)&idle_data);
        WAIT_JOIN_MAIN;
        return Py_BuildValue("i", 0);
}

static PyObject* remote_chfname(PyObject *self, PyObject *args)
{
	PI_DEBUG(DBG_L2, "pyremote: Chfname ");
	long channel = 0;
	if (!PyArg_ParseTuple(args, "l", &channel))
		return Py_BuildValue("i", -1);
        int ch=channel;
        if (ch >= 100) ch -= 100;
        if (gapp->xsm->GetScanChannel(ch)){
                const gchar *tmp = gapp->xsm->GetScanChannel (ch)->storage_manager.get_filename();
                if (channel >= 100)
                        return Py_BuildValue ("s", tmp ? tmp : gapp->xsm->GetScanChannel (ch)->data.ui.originalname);
                else
                        return Py_BuildValue ("s", tmp ? tmp : gapp->xsm->GetScanChannel (ch)->data.ui.name);
        } else
                return Py_BuildValue ("s", "EE: invalid channel");
}


static gboolean main_context_chmodea_from_thread (gpointer user_data){
        IDLE_from_thread_data *idle_data = (IDLE_from_thread_data *) user_data;
        // NOT THREAD SAFE GUI OPERATION TRIGGER HERE
	long channel = 0;
        idle_data->ret = -1;
        
	if (!PyArg_ParseTuple(idle_data->args, "l", &channel)){
                UNSET_WAIT_JOIN_MAIN;
                return G_SOURCE_REMOVE;
        }
        main_get_gapp()->xsm->ActivateChannel ((int)channel);
        idle_data->ret = 0;

        UNSET_WAIT_JOIN_MAIN;
        return G_SOURCE_REMOVE;
}

static PyObject* remote_chmodea(PyObject *self, PyObject *args)
{
	PI_DEBUG(DBG_L2, "pyremote: Chmode a ");
        IDLE_from_thread_data idle_data;
        idle_data.self = self;
        idle_data.args = args;
        idle_data.wait_join = true;
        g_idle_add (main_context_chmodea_from_thread, (gpointer)&idle_data);
        WAIT_JOIN_MAIN;
	return Py_BuildValue("i", idle_data.ret);
}

static gboolean main_context_chmodex_from_thread (gpointer user_data){
        IDLE_from_thread_data *idle_data = (IDLE_from_thread_data *) user_data;
        // NOT THREAD SAFE GUI OPERATION TRIGGER HERE
	long channel = 0;
        idle_data->ret = -1;
        
	if (!PyArg_ParseTuple(idle_data->args, "l", &channel)){
                UNSET_WAIT_JOIN_MAIN;
                return G_SOURCE_REMOVE;
        }
        main_get_gapp()->xsm->SetMode ((int)channel, ID_CH_M_X);
        idle_data->ret = 0;

        UNSET_WAIT_JOIN_MAIN;
        return G_SOURCE_REMOVE;
}

static PyObject* remote_chmodex(PyObject *self, PyObject *args)
{
	PI_DEBUG(DBG_L2, "pyremote: Chmode x ");
        IDLE_from_thread_data idle_data;
        idle_data.self = self;
        idle_data.args = args;
        idle_data.wait_join = true;
        g_idle_add (main_context_chmodex_from_thread, (gpointer)&idle_data);
        WAIT_JOIN_MAIN;
	return Py_BuildValue("i", idle_data.ret);
}

static gboolean main_context_chmodem_from_thread (gpointer user_data){
        IDLE_from_thread_data *idle_data = (IDLE_from_thread_data *) user_data;
        // NOT THREAD SAFE GUI OPERATION TRIGGER HERE
	long channel = 0;
        idle_data->ret = -1;
        
	if (!PyArg_ParseTuple(idle_data->args, "l", &channel)){
                UNSET_WAIT_JOIN_MAIN;
                return G_SOURCE_REMOVE;
        }
        main_get_gapp()->xsm->SetMode ((int)channel, ID_CH_M_MATH);
        idle_data->ret = 0;

        UNSET_WAIT_JOIN_MAIN;
        return G_SOURCE_REMOVE;
}

static PyObject* remote_chmodem(PyObject *self, PyObject *args)
{
	PI_DEBUG(DBG_L2, "pyremote: Chmode m ");
        IDLE_from_thread_data idle_data;
        idle_data.self = self;
        idle_data.args = args;
        idle_data.wait_join = true;
        g_idle_add (main_context_chmodem_from_thread, (gpointer)&idle_data);
        WAIT_JOIN_MAIN;
	return Py_BuildValue("i", idle_data.ret);
}

static gboolean main_context_chmoden_from_thread (gpointer user_data){
        IDLE_from_thread_data *idle_data = (IDLE_from_thread_data *) user_data;
        // NOT THREAD SAFE GUI OPERATION TRIGGER HERE
	long channel = 0;
        long mode = 0;
        idle_data->ret = -1;

        if (!PyArg_ParseTuple(idle_data->args, "ll", &channel, &mode)){
                UNSET_WAIT_JOIN_MAIN;
                return G_SOURCE_REMOVE;
        }
        idle_data->ret = 0;
        main_get_gapp()->xsm->SetMode ((int)channel, ID_CH_M_X+mode);
        UNSET_WAIT_JOIN_MAIN;
        return G_SOURCE_REMOVE;
}

static PyObject* remote_chmoden(PyObject *self, PyObject *args)
{
	PI_DEBUG(DBG_L2, "pyremote: Chmode n ");
        IDLE_from_thread_data idle_data;
        idle_data.self = self;
        idle_data.args = args;
        idle_data.wait_join = true;
        g_idle_add (main_context_chmoden_from_thread, (gpointer)&idle_data);
        WAIT_JOIN_MAIN;
	return Py_BuildValue("i", idle_data.ret);
}

static PyObject* remote_chmodeno(PyObject *self, PyObject *args)
{
	PI_DEBUG(DBG_L2, "pyremote: Chmode no -- not available");
	return Py_BuildValue("i", 0);
#if 0 // not thread safe, may trigger GUI / dialog 
	long channel = 0;
	if (!PyArg_ParseTuple(args, "l", &channel))
		return Py_BuildValue("i", -1);
	return Py_BuildValue ("i",main_get_gapp()->xsm->SetView ((int)channel, ID_CH_V_NO));
#endif
}

static PyObject* remote_chview1d(PyObject *self, PyObject *args)
{
	PI_DEBUG(DBG_L2, "pyremote: Chmode 1d -- not available");
	return Py_BuildValue("i", 0);
#if 0 // not thread safe, may trigger GUI / dialog 
	PI_DEBUG(DBG_L2, "pyremote: Chview 1d.");
	long channel = 0;
	if (!PyArg_ParseTuple(args, "l", &channel))
		return Py_BuildValue("i", -1);
	return Py_BuildValue ("i",main_get_gapp()->xsm->SetView (channel, ID_CH_V_PROFILE));
#endif
}

static PyObject* remote_chview2d(PyObject *self, PyObject *args)
{
	PI_DEBUG(DBG_L2, "pyremote: Chview 2d -- not avialable");
	return Py_BuildValue("i", 0);
#if 0 // not thread safe, may trigger GUI / dialog 
	long channel = 0;
	if (!PyArg_ParseTuple(args, "l", &channel))
		return Py_BuildValue("i", -1);
	return Py_BuildValue ("i",main_get_gapp()->xsm->SetView ((int)channel, ID_CH_V_GREY));
#endif
}

static PyObject* remote_chview3d(PyObject *self, PyObject *args)
{
	PI_DEBUG(DBG_L2, "pyremote: Chview 3d -- not available");
	return Py_BuildValue("i", 0);
#if 0 // not thread safe, may trigger GUI / dialog 
	long channel = 0;
	if (!PyArg_ParseTuple(args, "l", &channel))
		return Py_BuildValue("i", -1);
	return Py_BuildValue ("i",main_get_gapp()->xsm->SetView ((int)channel, ID_CH_V_SURFACE));
#endif
}

static gboolean main_context_setvm_from_thread (gpointer user_data){
        IDLE_from_thread_data *idle_data = (IDLE_from_thread_data *) user_data;
        // NOT THREAD SAFE GUI OPERATION TRIGGER HERE
	main_get_gapp()->xsm->SetVM (idle_data->ret);
        idle_data->ret = 0;
        UNSET_WAIT_JOIN_MAIN;
        return G_SOURCE_REMOVE;
}


static PyObject* remote_quick(PyObject *self, PyObject *args)
{
	PI_DEBUG(DBG_L2, "pyremote: Quick");
        IDLE_from_thread_data idle_data;
        idle_data.self = self;
        idle_data.args = args;
        idle_data.ret = SCAN_V_QUICK; // mode here
        idle_data.wait_join = true;
        g_idle_add (main_context_setvm_from_thread, (gpointer)&idle_data);
        WAIT_JOIN_MAIN;
	return Py_BuildValue("i", idle_data.ret);
}

static PyObject* remote_direct(PyObject *self, PyObject *args)
{
	PI_DEBUG(DBG_L2, "pyremote: Direkt");
        IDLE_from_thread_data idle_data;
        idle_data.self = self;
        idle_data.args = args;
        idle_data.ret = SCAN_V_DIRECT; // mode here
        idle_data.wait_join = true;
        g_idle_add (main_context_setvm_from_thread, (gpointer)&idle_data);
        WAIT_JOIN_MAIN;
	return Py_BuildValue("i", idle_data.ret);
}

static PyObject* remote_log(PyObject *self, PyObject *args)
{
	PI_DEBUG(DBG_L2, "pyremote: Log");
        IDLE_from_thread_data idle_data;
        idle_data.self = self;
        idle_data.args = args;
        idle_data.ret = SCAN_V_LOG; // mode here
        idle_data.wait_join = true;
        g_idle_add (main_context_setvm_from_thread, (gpointer)&idle_data);
        WAIT_JOIN_MAIN;
	return Py_BuildValue("i", idle_data.ret);
}

static gboolean main_context_math_crop_from_thread (gpointer user_data){
        IDLE_from_thread_data *idle_data = (IDLE_from_thread_data *) user_data;
        // NOT THREAD SAFE GUI OPERATION TRIGGER HERE
	long chsrc = 0;
	long chdst = 1;
        idle_data->ret = -1;

	if (!PyArg_ParseTuple(idle_data->args, "ll", &chsrc, &chdst)){
                UNSET_WAIT_JOIN_MAIN;
                return G_SOURCE_REMOVE;
        }

        if (!main_get_gapp()->xsm->GetScanChannel (chdst))
                main_get_gapp()->xsm->ActivateChannel (chdst);

        if (chsrc != chdst && main_get_gapp()->xsm->GetScanChannel(chsrc) && main_get_gapp()->xsm->GetScanChannel(chdst)){
                if (CropScan (main_get_gapp()->xsm->GetScanChannel (chsrc), main_get_gapp()->xsm->GetScanChannel (chdst)) == MATH_OK)
                    idle_data->ret = 0;

                main_get_gapp()->enter_thread_safe_no_gui_mode();
                main_get_gapp()->xsm->ActivateChannel (chdst);
                main_get_gapp()->xsm->ActiveScan->auto_display();
                main_get_gapp()->exit_thread_safe_no_gui_mode();
        }
        
        UNSET_WAIT_JOIN_MAIN;
        return G_SOURCE_REMOVE;
}

static PyObject* remote_crop(PyObject *self, PyObject *args)
{
	PI_DEBUG(DBG_L2, "pyremote: Crop");
        IDLE_from_thread_data idle_data;
        idle_data.self = self;
        idle_data.args = args;
        idle_data.wait_join = true;
        g_idle_add (main_context_math_crop_from_thread, (gpointer)&idle_data);
        WAIT_JOIN_MAIN;
	return Py_BuildValue("i", idle_data.ret);
}



///////////////////////////////////////////////////////////////
// BLOCK V
// unitbz .   DONE
// unitvolt .  DONE
// unitev .  DONE
// units .  DONE
///////////////////////////////////////////////////////////////

static PyObject* remote_unitbz(PyObject *self, PyObject *args)
{
	PI_DEBUG(DBG_L2, "pyremote: unitbz");
	main_get_gapp()->xsm->SetModeFlg(MODE_BZUNIT);
	main_get_gapp()->xsm->ClrModeFlg(MODE_VOLTUNIT);
	return Py_BuildValue("i", 0);
}
static PyObject* remote_unitvolt(PyObject *self, PyObject *args)
{
	PI_DEBUG(DBG_L2, "pyremote: unitvolt");
	main_get_gapp()->xsm->SetModeFlg(MODE_VOLTUNIT);
	main_get_gapp()->xsm->ClrModeFlg(MODE_BZUNIT);
	return Py_BuildValue("i", 0);
}
static PyObject* remote_unitev(PyObject *self, PyObject *args)
{
	PI_DEBUG(DBG_L2, "pyremote: unitev");
	main_get_gapp()->xsm->SetModeFlg(MODE_ENERGY_EV);
	main_get_gapp()->xsm->ClrModeFlg(MODE_ENERGY_S);
	return Py_BuildValue("i", 0);
}
static PyObject* remote_units(PyObject *self, PyObject *args)
{
	PI_DEBUG(DBG_L2, "pyremote: units");
	main_get_gapp()->xsm->SetModeFlg(MODE_ENERGY_S);
	main_get_gapp()->xsm->ClrModeFlg(MODE_ENERGY_EV);
	return Py_BuildValue("i", 0);
}

///////////////////////////////////////////////////////////////
// BLOCK VI
// echo S      DONE
// logev S      DONE
// da0 X      DONE, commented out
// signal S      DONE
// more actions by plugins S  NYI
///////////////////////////////////////////////////////////////

static PyObject* remote_echo(PyObject *self, PyObject *args)
{

	PI_DEBUG(DBG_L2, "pyremote: Echo.");
	gchar* line1;
	if (!PyArg_ParseTuple(args, "s", &line1))
		return Py_BuildValue("i", -1);
	/*Change the Debuglevel to: print always.*/
        g_message ("%s", line1);
	return Py_BuildValue("i", 0);
}


static gboolean main_context_logev_from_thread (gpointer user_data){
        IDLE_from_thread_data *idle_data = (IDLE_from_thread_data *) user_data;
        // NOT THREAD SAFE GUI OPERATION TRIGGER HERE
	gchar* zeile;
	if (!PyArg_ParseTuple(idle_data->args, "s", &zeile)){
                idle_data->ret = -1;
                UNSET_WAIT_JOIN_MAIN;
                return G_SOURCE_REMOVE;
        }
	if(zeile){
		main_get_gapp()->monitorcontrol->LogEvent((char *)"RemoteLogEv", zeile);
	}else{
		main_get_gapp()->monitorcontrol->LogEvent((char *)"RemoteLogEv", (char *)"--");
	}
        idle_data->ret = 0;
        UNSET_WAIT_JOIN_MAIN;
        return G_SOURCE_REMOVE;
}

static PyObject* remote_logev(PyObject *self, PyObject *args)
{
	PI_DEBUG(DBG_L2, "pyremote: Log ev.");
        IDLE_from_thread_data idle_data;
        idle_data.self = self;
        idle_data.args = args;
        idle_data.ret = SCAN_V_LOG; // mode here
        idle_data.wait_join = true;
        g_idle_add (main_context_logev_from_thread, (gpointer)&idle_data);
        WAIT_JOIN_MAIN;
	return Py_BuildValue("i", idle_data.ret);
}


static gboolean main_context_progress_info_from_thread (gpointer user_data){
        IDLE_from_thread_data *idle_data = (IDLE_from_thread_data *) user_data;
        // NOT THREAD SAFE GUI OPERATION TRIGGER HERE
        double d;
	gchar* info;
        idle_data->ret = -1;

	if (!PyArg_ParseTuple(idle_data->args, "sd", &info, &d)){
                UNSET_WAIT_JOIN_MAIN;
                return G_SOURCE_REMOVE;
        }

	if(info){
                if (d < 0.)
                        main_get_gapp()->progress_info_new (info, 1);
                else {
                        main_get_gapp()->progress_info_set_bar_fraction (d, 1);
                        main_get_gapp()->progress_info_set_bar_text (info, 1);
                }
                if (d > 1.){
                       main_get_gapp()->progress_info_close ();
                }
                idle_data->ret = 0;
	}
        
        UNSET_WAIT_JOIN_MAIN;
        return G_SOURCE_REMOVE;
}

static PyObject* remote_progress_info(PyObject *self, PyObject *args)
{
	PI_DEBUG(DBG_L2, "pyremote: progress_info");
        IDLE_from_thread_data idle_data;
        idle_data.self = self;
        idle_data.args = args;
        idle_data.wait_join = true;
        g_idle_add (main_context_progress_info_from_thread, (gpointer)&idle_data);
        WAIT_JOIN_MAIN;
	return Py_BuildValue("i", idle_data.ret);
}


static gboolean main_context_add_layer_information_from_thread (gpointer user_data){
        IDLE_from_thread_data *idle_data = (IDLE_from_thread_data *) user_data;
        // NOT THREAD SAFE GUI OPERATION TRIGGER HERE
	gchar* info;
	long layer = 0;

	if (!PyArg_ParseTuple(idle_data->args, "sl", &info, &layer)){
                idle_data->ret = -1;
                UNSET_WAIT_JOIN_MAIN;
                return G_SOURCE_REMOVE;
        }

        PI_DEBUG(DBG_L2, info << " to layer info, lv=" << layer );
        if (main_get_gapp()->xsm->ActiveScan)
                if(info && layer>=0 && layer<main_get_gapp()->xsm->GetActiveScan() -> mem2d->GetNv())
                       main_get_gapp()->xsm->GetActiveScan() -> mem2d->add_layer_information ((int)layer, new LayerInformation (info));

        idle_data->ret = 0;
        UNSET_WAIT_JOIN_MAIN;
        return G_SOURCE_REMOVE;
}

static PyObject* remote_add_layer_information(PyObject *self, PyObject *args)
{
	PI_DEBUG(DBG_L2, "pyremote: add_layer_information to active scan channel");
        IDLE_from_thread_data idle_data;
        idle_data.self = self;
        idle_data.args = args;
        idle_data.wait_join = true;
        g_idle_add (main_context_add_layer_information_from_thread, (gpointer)&idle_data);
        WAIT_JOIN_MAIN;
	return Py_BuildValue("i", idle_data.ret);
}

static PyObject* remote_da0(PyObject *self, PyObject *args)
{

	PI_DEBUG(DBG_L2, "pyremote: da0 ");
	double channel = 0;
	if (!PyArg_ParseTuple(args, "d", &channel))
		return Py_BuildValue("i", -1);
	if (channel){
		PI_DEBUG(DBG_L2, "Commented out.");
		//main_get_gapp()->xsm->hardware->SetAnalog("da-name", channel);
	}
	return Py_BuildValue("i", 0);
}


static gboolean main_context_signal_emit_from_thread (gpointer user_data){
        IDLE_from_thread_data *idle_data = (IDLE_from_thread_data *) user_data;
        // NOT THREAD SAFE GUI OPERATION TRIGGER HERE
	gchar *action;
        idle_data->ret = -1;

	if (!PyArg_ParseTuple(idle_data->args, "s", &action)){
                UNSET_WAIT_JOIN_MAIN;
                return G_SOURCE_REMOVE;
        }

	PI_DEBUG_GM (DBG_L2, "pyremote::remote_signal_emit (calling g_action_group_activate_action): %s", action);

        GActionMap *gm = G_ACTION_MAP (g_application_get_default ());
        //        g_message ("pyremote::remote_signal_emit get g_action_map: %s", gm ? "OK":"??");

        GAction *ga = g_action_map_lookup_action (gm, action);
        //        GAction *ga = g_action_map_lookup_action (G_ACTION_MAP (main_get_gapp()->getApp ()), action);
        //	g_message ("pyremote::remote_signal_emit g_action_map_lookup_action: %s -> %s", action, ga?"OK":"??");

        if (ga){
                g_action_activate (ga, NULL);
                idle_data->ret = 0;
        } else {
                PI_DEBUG_GP_WARNING (DBG_L2, "==> action unknown: %s", action);
        }

        UNSET_WAIT_JOIN_MAIN;
        return G_SOURCE_REMOVE;
}

static PyObject* remote_signal_emit(PyObject *self, PyObject *args)
{
        IDLE_from_thread_data idle_data;
        idle_data.self = self;
        idle_data.args = args;
        idle_data.wait_join = true;
        g_idle_add (main_context_signal_emit_from_thread, (gpointer)&idle_data);
        WAIT_JOIN_MAIN;
	return Py_BuildValue("i", idle_data.ret);
}

#if 0
/* Taken from somewhere*/
static gboolean busy_sleep;
gint ret_false(gpointer r)
{
        // FIX-ME GTK4 ??? what is it at all
	//gtk_main_quit();
        *((int *)r) = FALSE;
	return FALSE;
}

void sleep_ms(int ms)
{
	if (busy_sleep) return;          /* Don't allow more than 1 sleep_ms */
	busy_sleep=TRUE;
        int wait=TRUE;
	g_timeout_add(ms,(GSourceFunc)ret_false, &wait); /* Start time-out function*/
        while (wait)
                while(g_main_context_pending (NULL)) g_main_context_iteration (NULL, FALSE);

        //gtk_main();                             /* wait */
	busy_sleep=FALSE;
}
#endif

static PyObject* remote_sleep(PyObject *self, PyObject *args)
{
	PI_DEBUG(DBG_L2, "pyremote: Sleep ");
	double d;
	if (!PyArg_ParseTuple(args, "d", &d))
		return Py_BuildValue("i", -1);
	if (d>0.){ // d in 1/10s
                g_usleep ((useconds_t)round(d*1e5)); // now in a thread and can simply sleep here!
		// sleep_ms((int)(round(d*100)));
	}
	return Py_BuildValue("i", 0);
}


static gboolean main_context_set_sc_label_from_thread (gpointer user_data){
        IDLE_from_thread_data *idle_data = (IDLE_from_thread_data *) user_data;
	gchar *id, *label;

	if (!PyArg_ParseTuple(idle_data->args, "ss", &id, &label)){
                UNSET_WAIT_JOIN_MAIN;
                return G_SOURCE_REMOVE;
        }
        if (py_gxsm_remote_console)
                py_gxsm_remote_console->set_sc_label (id, label);
        
        UNSET_WAIT_JOIN_MAIN;
        return G_SOURCE_REMOVE;
}

static PyObject* remote_set_sc_label(PyObject *self, PyObject *args)
{
        IDLE_from_thread_data idle_data;
        idle_data.self = self;
        idle_data.args = args;
        idle_data.wait_join = true;
        g_idle_add (main_context_set_sc_label_from_thread, (gpointer)&idle_data);
        WAIT_JOIN_MAIN;
	return Py_BuildValue("i", idle_data.ret);
}

///////////////////////////////////////////////////////////
/*
PyMethodDef
Structure used to describe a method of an extension type. This structure has four fields:

Field	C Type	Meaning
ml_name	char *	name of the method
ml_meth	PyCFunction	pointer to the C implementation
ml_flags	int	flag bits indicating how the call should be constructed
ml_doc	char *	points to the contents of the docstring
*/

static PyMethodDef GxsmPyMethods[] = {
	// BLOCK I
	{"help", remote_help, METH_VARARGS, "List Gxsm methods: print gxsm.help ()"},
	{"set", remote_set, METH_VARARGS, "Set Gxsm entry value, see list_refnames: gxsm.set ('refname','value as string')"},
	{"get", remote_get, METH_VARARGS, "Get Gxsm entry as value, see list_refnames. gxsm.get ('refname')"},
	{"gets", remote_gets, METH_VARARGS, "Get Gxsm entry as string. gxsm.gets ('refname')"},
	{"list_refnames", remote_listr, METH_VARARGS, "List all available Gxsm entry refnames (Better: pointer hover over Gxsm-Entry to see tooltip with ref-name). print gxsm.list_refnames ()"},
	{"action", remote_action, METH_VARARGS, "Trigger Gxsm action (menu action or button signal), see list_actions: gxsm.action('action')"},
	{"list_actions", remote_lista, METH_VARARGS, "List all available Gxsm actions (Better: pointer hover over Gxsm-Button to see tooltip with action-name): print gxsm.list_actions ()"},
	{"rtquery", remote_rtquery, METH_VARARGS, "Gxsm hardware Real-Time-Query: svec[3] = gxsm.rtquery('X|Y|Z|xy|zxy|o|f|s|i|U') "},
        {"y_current", remote_y_current, METH_VARARGS, "RTQuery Current Scanline."},
	{"moveto_scan_xy", remote_moveto_scan_xy, METH_VARARGS, "Set tip position to Scan-XY: gxsm.moveto_scan_xy (x,y)"},

	// BLOCK II
	{"createscan", remote_createscan, METH_VARARGS, "Create Scan int: gxsm.createscan (ch,nx,ny,nv pixels, rx,ry in A, array.array('l', [...]), append)"},
	{"createscanf", remote_createscanf, METH_VARARGS, "Create Scan float: gxsm.createscan (ch,nx,ny,nv pixels, rx,ry in A, array.array('f', [...]), append)"},
	{"set_scan_unit", remote_set_scan_unit, METH_VARARGS, "Set Scan X,Y,Z,L Dim Unit: gxsm.set_scan_unit (ch,'X|Y|Z|L|T','UnitId string','Label string')"},
	{"set_scan_lookup", remote_set_scan_lookup, METH_VARARGS, "Set Scan Lookup for Dim: gxsm.set_scan_lookup (ch,'X|Y|L',start,end)"},
	//{"set_scan_lookup_i", remote_set_scan_llookup, METH_VARARGS, "Set Scan Lookup for Dim: gxsm.set_scan_lookup_i (ch,'X|Y|L',start,end)"},

	{"get_geometry", remote_getgeometry, METH_VARARGS, "Get Scan Geometry: [rx,ry,x0,y0,alpha]=gxsm.get_geometry (ch)"},
	{"get_differentials", remote_getdifferentials, METH_VARARGS, "Get Scan Scaling: [dx,dy,dz,dl]=gxsm.get_differentials (ch)"},
	{"get_dimensions", remote_getdimensions, METH_VARARGS, "Get Scan Dimensions: [nx,ny,nv,nt]=gxsm.get_dimensions (ch)"},
	{"get_data_pkt", remote_getdatapkt, METH_VARARGS, "Get Data Value at point: value=gxsm.get_data_pkt (ch, x, y, v, t)"},
	{"put_data_pkt", remote_putdatapkt, METH_VARARGS, "Put Data Value to point: gxsm.put_data_pkt (value, ch, x, y, v, t)"},
	{"get_slice", remote_getslice, METH_VARARGS, "Get Image Data Slice (Lines) from Scan in channel ch, yi ... yi+yn: [nx,ny,array]=gxsm.get_slice (ch, v, t, yi, yn)"},
	{"get_x_lookup", remote_get_x_lookup, METH_VARARGS, "Get Scan Data index to world mapping: x=gxsm.get_x_lookup (ch, i)"},
	{"get_y_lookup", remote_get_y_lookup, METH_VARARGS, "Get Scan Data index to world mapping: y=gxsm.get_y_lookup (ch, i)"},
	{"get_v_lookup", remote_get_v_lookup, METH_VARARGS, "Get Scan Data index to world mapping: v=gxsm.get_v_lookup (ch, i)"},
	{"set_x_lookup", remote_set_x_lookup, METH_VARARGS, "Set Scan Data index to world mapping: x=gxsm.get_x_lookup (ch, i, v)"},
	{"set_y_lookup", remote_set_y_lookup, METH_VARARGS, "Set Scan Data index to world mapping: y=gxsm.get_y_lookup (ch, i, v)"},
	{"set_v_lookup", remote_set_v_lookup, METH_VARARGS, "Set Scan Data index to world mapping: v=gxsm.get_v_lookup (ch, i, v)"},
	{"get_object", remote_getobject, METH_VARARGS, "Get Object Coordinates: [type, x,y,..]=gxsm.get_object (ch, n)"},
	{"add_marker_object", remote_addmobject, METH_VARARGS, "Put Marker/Rectangle Object at pixel coordinates or current tip pos (id='xy'|grp=-1, 'Rectangle[id]|grp=-2, 'Point[id]'): gxsm.add_marker_object (ch, label=str|'xy'|'Rectangle-id', mgrp=0..5|-1, x=ix,y=iy, size)"},
        {"marker_getobject_action", remote_getobject_action, METH_VARARGS, "Marker/Rectangle Object Action: gxsm.marker_getobject_action (ch, objnameid, action='REMOVE'|'REMOVE-ALL'|'GET_COORDS'|'SET-OFFSET')"},
	{"startscan", remote_startscan, METH_VARARGS, "Start Scan."},
	{"stopscan", remote_stopscan, METH_VARARGS, "Stop Scan."},
	{"waitscan", remote_waitscan, METH_VARARGS, "Wait Scan. ret=gxsm.waitscan(blocking=true). ret=-1: no scan in progress, else current line index."},
	{"scaninit", remote_scaninit, METH_VARARGS, "Scaninit."},
	{"scanupdate", remote_scanupdate, METH_VARARGS, "Scanupdate."},
	{"scanylookup", remote_scanylookup, METH_VARARGS, "Scanylookup."},
	{"scanline", remote_scanline, METH_VARARGS, "Scan line."},

	// BLOCK III
	{"autosave", remote_autosave, METH_VARARGS, "Save: Auto Save Scans. gxsm.autosave (). Returns current scanline y index and file name(s) if scanning."},
	{"autoupdate", remote_autoupdate, METH_VARARGS, "Save: Auto Update Scans. gxsm.autoupdate (). Returns current scanline y index and file name(s) if scanning."},
	{"save", remote_save, METH_VARARGS, "Save: Auto Save Scans: gxsm.save ()"},
	{"saveas", remote_saveas, METH_VARARGS, "Save File As: gxsm.saveas (ch, 'path/fname.nc')"},
	{"load", remote_load, METH_VARARGS, "Load File: gxsm.load (ch, 'path/fname.nc')"},
	{"export", remote_export, METH_VARARGS, "Export scan: gxsm.export (ch, 'path/fname.nc')"},
	{"import", remote_import, METH_VARARGS, "Import scan: gxsm.import (ch, 'path/fname.nc')"},
	{"save_drawing", remote_save_drawing, METH_VARARGS, "Save Drawing to file: gxsm.save_drawing (ch, time, layer, 'path/fname.png|pdf|svg')"},

	// BLOCK IV
	{"set_view_indices", remote_set_view_indices, METH_VARARGS, "Set Ch view time and layer indices: gxsm.set_view_indices (ch, time, layer)"},
	{"autodisplay", remote_autodisplay, METH_VARARGS, "Autodisplay active channel: gxsm.autodisplay ()"},
	{"chfname", remote_chfname, METH_VARARGS, "Get Ch Filename: filename = gxsm.chfname (ch)"},
	{"chmodea", remote_chmodea, METH_VARARGS, "Set Ch Mode to A: gxsm.chmodea (ch)"},
	{"chmodex", remote_chmodex, METH_VARARGS, "Set Ch Mode to X: gxsm.chmodex (ch)"},
	{"chmodem", remote_chmodem, METH_VARARGS, "Set Ch Mode to MATH: gxsm.chmodem (ch)"},
	{"chmoden", remote_chmoden, METH_VARARGS, "Set Ch Mode to Data Channel <X+N>: gxsm.chmoden (ch,n)"},
	{"chmodeno", remote_chmodeno, METH_VARARGS, "Set View Mode to No: gxsm.chmodeno (ch)"},
	{"chview1d", remote_chview1d, METH_VARARGS, "Set View Mode to 1D: gxsm.chmode1d (ch)"},
	{"chview2d", remote_chview2d, METH_VARARGS, "Set View Mode to 2D: gxsm.chmode2d (ch)"},
	{"chview3d", remote_chview3d, METH_VARARGS, "Set View Mode to 3D: gxsm.chmode3d (ch)"},
	{"quick", remote_quick, METH_VARARGS, "Quick."},
	{"direct", remote_direct, METH_VARARGS, "Direct."},
	{"log", remote_log, METH_VARARGS, "Log."},
	{"crop", remote_crop, METH_VARARGS, "Crop (ch-src, ch-dst)"},

	// BLOCK V
	{"unitbz", remote_unitbz, METH_VARARGS, "UnitBZ."},
	{"unitvolt", remote_unitvolt, METH_VARARGS, "UnitVolt."},
	{"unitev", remote_unitev, METH_VARARGS, "UniteV."},
	{"units", remote_units, METH_VARARGS, "UnitS."},

	// BLOCK VI
	{"echo", remote_echo, METH_VARARGS, "Echo string to terminal. gxsm.echo('hello gxsm to terminal') "},
	{"logev", remote_logev, METH_VARARGS, "Write string to Gxsm system log file and log monitor: gxsm.logev ('hello gxsm to logfile/monitor') "},
	{"progress", remote_progress_info, METH_VARARGS, "Show/update gxsm progress info. fraction<0 init, 0..1 progress, >1 close: gxsm.progress ('Calculating...', fraction) "},
	{"add_layerinformation", remote_add_layer_information, METH_VARARGS, "Add Layerinformation to active scan. gxsm.add_layerinformation('Text',ch)"}, 
	{"da0", remote_da0, METH_VARARGS, "Da0. -- N/A for SRanger"},
	{"signal_emit", remote_signal_emit, METH_VARARGS, "Action-String. "},
	{"sleep", remote_sleep, METH_VARARGS, "Sleep N/10s: gxsm.sleep (N) "},
	{"set_sc_label", remote_set_sc_label, METH_VARARGS, "Set PyRemote SC label: gxsm.set_sc_label (id [1..8],'value as string')"},

	{NULL, NULL, 0, NULL}
};

static PyObject* remote_help(PyObject *self, PyObject *args)
{
        gint entries;
	for (entries=0; GxsmPyMethods[entries].ml_name != NULL; entries++);
	PyObject *ret = PyTuple_New(entries);
	for (int n=0; n < entries; n++){
                gchar *tmp = g_strdup_printf ("gxsm.%s : %s", GxsmPyMethods[n].ml_name, GxsmPyMethods[n].ml_doc);
                PyTuple_SetItem(ret, n, PyUnicode_FromString (tmp)); // Add Refname to Return-list
                g_free (tmp);
        }

	return ret;
}



int ok_button_callback( GtkWidget *widget, gpointer data)
{
	//    cout << getpid() << endl;
	kill (getpid(), SIGINT);
	//    cout << "pressed" <<endl;
	return 0;
}

py_gxsm_console::~py_gxsm_console (){
        PI_DEBUG_GM(DBG_L2, "Pyremote Plugin: destructor. Calls: Py_FinalizeEx()");
        Py_FinalizeEx();
}


/* Python strout/err redirection helper module */

static PyObject* redirection_stdoutredirect(PyObject *self, PyObject *args)
{
        const char *string;
        if(!PyArg_ParseTuple(args, "s", &string))
                return NULL;

        g_print ("%s", string);
        if (py_gxsm_remote_console)
                py_gxsm_remote_console->push_message_async (string);

        Py_INCREF(Py_None);
        return Py_None;
}

static PyMethodDef RedirectionMethods[] = {
                                           {"stdoutredirect", redirection_stdoutredirect, METH_VARARGS,
                                            "stdout redirection helper"},
                                           {NULL, NULL, 0, NULL}
};

static struct PyModuleDef redirection_module_def = {
                                       PyModuleDef_HEAD_INIT,
                                       "redirection",     /* m_name */
                                       "GXSM Python Console STDI Redirection Module",  /* m_doc */
                                       -1,                  /* m_size */
                                       RedirectionMethods,  /* m_methods */
                                       NULL,                /* m_reload */
                                       NULL,                /* m_traverse */
                                       NULL,                /* m_clear */
                                       NULL,                /* m_free */
};


static struct PyModuleDef gxsm_module_def = {
                                       PyModuleDef_HEAD_INIT,
                                       "gxsm",     /* m_name */
                                       "GXSM Python Remote Module",  /* m_doc */
                                       -1,                  /* m_size */
                                       GxsmPyMethods,       /* m_methods */
                                       NULL,                /* m_reload */
                                       NULL,                /* m_traverse */
                                       NULL,                /* m_clear */
                                       NULL,                /* m_free */
};

static PyObject* PyInit_Gxsm(void)
{
        PI_DEBUG_GM (DBG_L1, "** PyInit_Gxsm => PyModuleCreate gxsm\n");
        // g_print ("** PyInit_Gxsm => PyModuleCreate gxsm\n");
        return PyModule_Create (&gxsm_module_def);
}

static PyObject* PyInit_Redirection(void)
{
        PI_DEBUG_GM (DBG_L1, "** PyInit_Redirection => PyModuleCreate redirection\n");
        // g_print ("** PyInit_Redirection => PyModuleCreate redirection\n");
        return PyModule_Create (&redirection_module_def);
}


void py_gxsm_console::initialize(void)
{
	PI_DEBUG_GM (DBG_L1, "pyremote Plugin :: py_gxsm_console::initialize **");

	if (!Py_IsInitialized()) {
		PI_DEBUG_GM (DBG_L1, "** Initializing Python interpreter, loading gxsm module and stdout redirection helper **");
                PI_DEBUG_GM (DBG_L1, "pyremote Plugin :: initialize -- PyImport_Append");
                // g_print ("pyremote Plugin :: initialize -- PyImport_Append\n");
                PyImport_AppendInittab ("gxsm", &PyInit_Gxsm);
                PyImport_AppendInittab ("redirection", &PyInit_Redirection);

                PI_DEBUG_GM (DBG_L2, "pyremote Plugin :: initialize --  PyInitializeEx(0)");
                // g_print ("pyremote Plugin :: initialize -- PyInitializeEx(0)\n");
		// Do not register signal handlers -- i.e. do not "crash" gxsm on errors!
                Py_InitializeEx (0);

                //import_array(); // returns a value
                if (_import_array() < 0) {
                        PI_DEBUG_GM (DBG_L1, "pyremote Plugin :: initialize -- ImportModule gxsm, import array failed.");
                        PyErr_Print(); PyErr_SetString(PyExc_ImportError, "numpy.core.multiarray failed to import");
                }
                
                //if (!PyEval_ThreadsInitialized())
                //        PyEval_InitThreads();
                //PyEval_InitThreads(); obsolete in 3.7
                //PyEval_ReleaseLock();

		PI_DEBUG_GM (DBG_L2, "pyremote Plugin :: initialize -- ImportModule gxsm");
                // g_print ("pyremote Plugin :: initialize -- ImportModule gxsm\n");
                py_gxsm_module.module = PyImport_ImportModule("gxsm");
                PyImport_ImportModule("redirection");

                PI_DEBUG_GM (DBG_L2, "pyremote Plugin :: initialize -- AddModule main\n");
                // g_print ("pyremote Plugin :: initialize -- AddModule main\n");
		py_gxsm_module.main_module = PyImport_AddModule("__main__");
                
		PI_DEBUG_GM (DBG_L2, "Get dict");
                // g_print ("pyremote Plugin :: initialize -- GetDict");
		py_gxsm_module.dict = PyModule_GetDict (py_gxsm_module.module);

                //PyGILState_STATE py_state = PyGILState_Ensure();
                //PyGILState_Release (py_state);

        } else {
		g_message ("Python interpreter already initialized.");
	}
}

PyObject* py_gxsm_console::run_string(const char *cmd, int type, PyObject *g, PyObject *l) {
        push_message_async ("\n<<< Python interpreter started. <<<\n");
        push_message_async (cmd);

        PyGILState_STATE py_state = PyGILState_Ensure();
	PyObject *ret = PyRun_String(cmd, type, g, l);
        PyGILState_Release (py_state);

        push_message_async (ret ?
                            "\n<<< Python interpreter finished processing string. <<<\n" :
                            "\n<<< Python interpreter completed with Exception/Error. <<<\n");
        push_message_async (NULL); // terminate IDLE push task
	if (!ret) {
		g_message ("Python interpreter completed with Exception/Error.");
		PyErr_Print();
	}
	return ret;
}


void py_gxsm_console::show_stderr(const gchar *str)
{
        GtkDialogFlags flags =  (GtkDialogFlags) (GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT);
        GtkWidget *dialog = gtk_dialog_new_with_buttons (N_("Python interpreter result"),
                                                         window,
                                                         flags,
                                                         _("_CLOSE"), GTK_RESPONSE_CLOSE,
                                                         NULL);

	GtkWidget *text = gtk_text_view_new ();
	GtkWidget *scroll = gtk_scrolled_window_new ();
	gtk_scrolled_window_set_child (GTK_SCROLLED_WINDOW (scroll), text);
	gtk_text_view_set_editable (GTK_TEXT_VIEW(text), FALSE);
	gtk_text_buffer_set_text (gtk_text_view_get_buffer(GTK_TEXT_VIEW(text)),
                                  str,
                                  -1);
        gtk_box_append (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))), scroll);

        gtk_widget_show (dialog);

        int response = GTK_RESPONSE_NONE;
        g_signal_connect (G_OBJECT (dialog), "response", G_CALLBACK (GnomeAppService::on_dialog_response_to_user_data), &response);
                        
        // FIX-ME GTK4 ??
        // wait here on response
        while (response == GTK_RESPONSE_NONE)
                while(g_main_context_pending (NULL)) g_main_context_iteration (NULL, FALSE);
}

void py_gxsm_console::initialize_stderr_redirect(PyObject *d)
{
        // new redirection of stdout/err capture
        run_string ("import redirection\n"
                    "import sys\n"
                    "class StdoutCatcher:\n"
                    "    def write(self, stuff):\n"
                    "        redirection.stdoutredirect(stuff)\n"
                    "    def flush(self):\n"
                    "        redirection.stdoutredirect('\\n')\n"
                    "class StderrCatcher:\n"
                    "    def write(self, stuff):\n"
                    "        redirection.stdoutredirect(stuff)\n"
                    "    def flush(self):\n"
                    "        redirection.stdoutredirect('\\n')\n"
                    "sys.stdout = StdoutCatcher()\n"
                    "sys.stderr = StderrCatcher()\n",
		   Py_file_input,
		   d,
		   d);
}

PyObject *py_gxsm_console::create_environment(const gchar *filename, gboolean show_errors) {
	PyObject *d, *plugin_filename;
	wchar_t *argv[1];
	argv[0] = NULL;

	d = PyDict_Copy (PyModule_GetDict (py_gxsm_module.main_module));
	// set __file__ variable for clearer error reporting
	plugin_filename = Py_BuildValue("s", filename);
	PyDict_SetItemString(d, "__file__", plugin_filename);
	PySys_SetArgv(0, argv);

        initialize_stderr_redirect(d);

#if 0
	// redirect stderr and stdout of python script to temporary file
	if (show_errors) {
		PI_DEBUG (DBG_L4,  "showing errors");
		initialize_stderr_redirect(d);
	} else {
		PI_DEBUG (DBG_L4,  "NOT showing errors");
        }
#endif
	return d;
}

void py_gxsm_console::destroy_environment(PyObject *d, gboolean show_errors) {
	// show content of temporary file which contains stderr and stdout of python
	// script and close it
	PyDict_Clear(d);
	Py_DECREF(d);
}

void py_gxsm_console::clear_output(GtkButton *btn, gpointer user_data)
{
	py_gxsm_console *pygc = (py_gxsm_console *)user_data;
	GtkTextBuffer *console_buf;
	GtkTextIter start_iter, end_iter;
	GtkTextView *textview;

	textview = GTK_TEXT_VIEW (pygc->console_output);
	console_buf = gtk_text_view_get_buffer(textview);
	gtk_text_buffer_get_bounds(console_buf, &start_iter, &end_iter);
	gtk_text_buffer_delete(console_buf, &start_iter, &end_iter);
}


/*
 * killing the interpreter
 * see http://stackoverflow.com/questions/1420957/stopping-embedded-python
 * for possibly adding an exit flag.
 */
void py_gxsm_console::kill(GtkToggleButton *btn, gpointer user_data)
{
	py_gxsm_console *pygc = (py_gxsm_console *)user_data;

        if (pygc->user_script_running > 0){
                pygc->append (N_("\n*** SCRIPT KILL: Setting PyErr Interrupt to abort script.\n"));
        
                //Py_AddPendingCall(-1);
                PI_DEBUG_GM (DBG_L2,  "trying to kill interpreter");
                PyErr_SetInterrupt();
#if 0
                PyGILState_STATE state = PyGILState_Ensure();    
                int r = Py_AddPendingCall(&Stop, NULL); // inject our Stop routine
                PyErr_SetInterrupt ();
                g_message ("Py_AddPendingCall -> %d", r);
                PyGILState_Release(state);

                //PyErr_SetInterruptEx (SIGINT);
                //PyErr_CheckSignals();
                //PyErr_SetString(PyExc_KeyboardInterrupt, "Abort");
#endif           
        } else {
                pygc->append (N_("\n*** SCRIPT KILL: No user script is currently running.\n"));
        }
}

#if 0
void py_gxsm_console::PyRun_GTaskThreadFunc (GTask *task,
                                             gpointer source_object,
                                             gpointer task_data,
                                             GCancellable *cancellable){
        PyRunThreadData *s = (PyRunThreadData*) task_data;
        PI_DEBUG_GM (DBG_L2, "pyremote Plugin :: py_gxsm_console::PyRun_GTaskThreadFunc");

        PyGILState_STATE py_state = PyGILState_Ensure();
        s->ret = PyRun_String(s->cmd,
                              s->mode,
                              s->dictionary,
                              s->dictionary);
        PyGILState_Release (py_state);
        PyEval_RestoreThread(s->py_state_save);

        g_free (s->cmd);
        s->cmd = NULL;
        PI_DEBUG_GM (DBG_L2, "pyremote Plugin :: py_gxsm_console::PyRun_GTaskThreadFunc done");
}
#endif

gpointer py_gxsm_console::PyRun_GThreadFunc (gpointer data){
        PyRunThreadData *s = (PyRunThreadData*) data;
        PI_DEBUG_GM (DBG_L2, "pyremote Plugin :: py_gxsm_console::PyRun_GThreadFunc");

        PyGILState_STATE py_state = PyGILState_Ensure();
        s->ret = PyRun_String(s->cmd,
                              s->mode,
                              s->dictionary,
                              s->dictionary);
        g_free (s->cmd);
        s->cmd = NULL;
        PI_DEBUG_GM (DBG_L2, "pyremote Plugin :: py_gxsm_console::PyRun_GThreadFunc PyRun completed");
        if (!s->ret) PyErr_Print();
        --s->pygc->user_script_running;
        s->pygc->push_message_async (s->ret ?
                                    "\n<<< PyRun user script (as thread) finished. <<<\n" :
                                    "\n<<< PyRun user script (as thread) run raised an exeption. <<<\n");
        s->pygc->push_message_async (NULL); // terminate IDLE push task
        PI_DEBUG_GM (DBG_L2, "pyremote Plugin :: py_gxsm_console::PyRun_GThreadFunc finished.");

        PyGILState_Release (py_state);
        PyEval_RestoreThread(s->py_state_save);

        return NULL;
}

void py_gxsm_console::PyRun_GAsyncReadyCallback (GObject *source_object,
                                                 GAsyncResult *res,
                                                 gpointer user_data){
        PI_DEBUG_GM (DBG_L2, "pyremote Plugin :: py_gxsm_console::PyRun_GAsyncReadyCallback");
	py_gxsm_console *pygc = (py_gxsm_console *)user_data;

        PyGILState_STATE py_state = PyGILState_Ensure();
        if (!pygc->run_data.ret) PyErr_Print();
        PyGILState_Release (py_state);

        --pygc->user_script_running;
        pygc->push_message_async (pygc->run_data.ret ?
                                  "\n<<< PyRun user script (as thread) finished. <<<\n" :
                                  "\n<<< PyRun user script (as thread) run raised an exeption. <<<\n");
        pygc->push_message_async (NULL); // terminate IDLE push task
        PI_DEBUG_GM (DBG_L2, "pyremote Plugin :: py_gxsm_console::PyRun_GAsyncReadyCallback done");
}


const gchar* py_gxsm_console::run_command(const gchar *cmd, int mode)
{
	if (!cmd) {
		g_warning("No command.");
		return NULL;
	}

        PyErr_Clear(); // clear any previous error or interrupts set

        if (!run_data.cmd){
                PI_DEBUG_GM (DBG_L2, "pyremote Plugin :: py_gxsm_console::run_command *** starting console IDLE message pop job.");
                run_data.cmd = g_strdup (cmd);
                run_data.mode = mode;
                run_data.dictionary = dictionary;
                run_data.ret  = NULL;
                run_data.pygc = this;
#if 1
                run_data.py_state_save = PyEval_SaveThread();
                g_thread_new (NULL, PyRun_GThreadFunc, &run_data);
#else
                GTask *pyrun_task = g_task_new (NULL,
                                                NULL,
                                                PyRun_GAsyncReadyCallback, this);
                g_task_set_task_data (pyrun_task, &run_data, NULL);
                g_task_run_in_thread (pyrun_task, PyRun_GTaskThreadFunc);
#endif
                PI_DEBUG_GM (DBG_L2, "pyremote Plugin :: py_gxsm_console::run_command thread fired up");
                return NULL;
        } else {
                return "Busy";
        }
}

void py_gxsm_console::append (const gchar *msg)
{
	if (!msg) return;
        GtkTextBuffer *text_buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (console_output));
        GtkTextIter text_iter_end;
        gtk_text_buffer_get_end_iter (text_buffer, &text_iter_end);
        gtk_text_buffer_insert (text_buffer, &text_iter_end, msg, -1);
        gtk_text_view_scroll_to_mark (GTK_TEXT_VIEW (console_output),
                                      console_mark_end,
                                      0., FALSE, 0., 0.);
}

// simple parser to include script library.
// As this seams not possible with embedded python via "use ..." as gxsm.Fucntions() are not availble in external libraries.
// So this simple approach.
gchar *py_gxsm_console::pre_parse_script (const gchar *script, int *n_lines, int r){
        gchar *tmp;
        gchar *parsed_script = g_strdup_printf ("### parsed script. Level: %d\n", r);
        gchar **lines = NULL;
        gchar *to_parse = g_strdup (script);
        int i=0;
                
        // Max recursion = 10
        if (r > 10){
                gchar *message = g_strdup_printf("Pasing Error: Potential dead-loop of recursive inclusion detected:\n"
                                                 "Include Level > 10.\n"
                                                 );
                g_warning ("%s", message);
                append(message);
                main_get_gapp()->warning (message);
                g_free(message);

                g_free (parsed_script);
                return to_parse;
        }

        do {
                ++i;
                if (lines) g_strfreev (lines);
                lines = g_strsplit (to_parse, "\n", 2);
                g_free (to_parse);
                to_parse = NULL;
                //g_print ("%05d: %s\n", i, lines[0]);

                if (g_strrstr(lines[0], "#GXSM_USE_LIBRARY")){
                        gchar *a = g_strrstr(lines[0], "<");
                        gchar *b = g_strrstr(lines[0], ">");
                        if (a && b){
                                *b='\0';
                                gchar *name=a+1;
                                //g_print ("Including Library <%s>\n", name);
                                gchar *lib_script = get_gxsm_script (name);
                                if (lib_script){
                                        int n_included=0;
                                        gchar *rtmp = pre_parse_script (lib_script, &n_included, r+1); // recursively parse
                                        g_free (lib_script);
                                        n_included += 8; // incl. comments below
                                        gchar *stat = g_strdup_printf ("Lines inlucded from <%s> at line %d: %d, Include Level: %d\n",
                                                                       name, i, n_included, r+1);
                                
                                        tmp = g_strconcat (parsed_script, "\n\n",
                                                           "### BEGIN GXSM LIBRARY SCRIPT <", name, ">\n\n",
                                                           rtmp ? rtmp : "## PARSING ERROR: LIB-SCRIPT NOT FOUND", "\n",
                                                           "### ", stat,
                                                           "### END GXSM LIBRARY SCRIPT <", name, ">\n\n",
                                                           NULL);
                                        append (stat);
                                        g_message ("%s", stat);

                                        i += n_included;

                                        g_free (stat);
                                        g_free (parsed_script);
                                        g_free (rtmp);
                                        *b='>';
                                        parsed_script = tmp;
                                } else {
                                        gchar *message = g_strdup_printf("Action script/library parser error at line %d:\n"
                                                                         "%s\n"
                                                                         "Include file <%s> not found.",
                                                                         i,
                                                                         lines[0],
                                                                         name
                                                                         );
                                        g_message ("%s", message);
                                        append(message);
                                        main_get_gapp()->warning (message);
                                        g_free(message);
                                }
                        } else {
                                g_warning ("Pasing Error here: %s", lines[0]);
                                gchar *message = g_strdup_printf("Action script/library parser syntax error at line %d:\n"
                                                                 "%s\n"
                                                                 "Gxsm Library script include example statement:\n"
                                                                 "#GXSM_USE_LIBRARY <gxsm4-lib-utils>\n",
                                                                 i,
                                                                 lines[0]);
                                g_message ("%s", message);
                                append(message);
                                main_get_gapp()->warning (message);
                                g_free(message);
                        }
                } else {
                        tmp = g_strconcat (parsed_script, "\n", lines[0], NULL);
                        g_free (parsed_script);
                        parsed_script = tmp;
                }
                        
                if (lines[1])
                        to_parse = g_strdup (lines[1]);
        } while (lines[0] && lines[1]);

        g_strfreev (lines);
        if (to_parse)
                g_free (to_parse);

        //g_print ("PARSED-SCRIPT\n");
        //g_print (parsed_script);

        if (n_lines)
                *n_lines = i;
        
        return parsed_script;
}

void py_gxsm_console::run_file(GtkButton *btn, gpointer user_data)
{
	py_gxsm_console *pygc = (py_gxsm_console *)user_data;
	GtkTextView *textview;
	GtkTextBuffer *console_file_buf;
	GtkTextIter start_iter, end_iter;
	const gchar *output;
        gchar *script, *parsed_script;

	textview = GTK_TEXT_VIEW(pygc->console_file_content);
	console_file_buf = gtk_text_view_get_buffer(textview);

        //gchar *tmp = g_strdup_printf ("%s #jobs[%d]+1\n", N_("\n>>> Checking for user script execution... \n"), pygc->user_script_running);
        //pygc->append (tmp);
        //g_free (tmp);

        if (pygc->user_script_running > 0){
                pygc->append (N_("\n*** STOP -- User script is currently running. No recursive execution allowed for this console.\n"));
        } else {
                gtk_text_buffer_get_bounds(console_file_buf, &start_iter, &end_iter);
                script = gtk_text_buffer_get_text(console_file_buf,
                                                  &start_iter, &end_iter, FALSE);
                parsed_script = pygc->pre_parse_script (script);
                g_free (script);
                script = parsed_script;
                
                pygc->push_message_async (N_("\n>>> Executing parsed script >>>\n"));
                pygc->user_script_running++;
                output = pygc->run_command (script, Py_file_input);
                g_free(script);
        }
}

void py_gxsm_console::fix_eols_to_unix(gchar *text)
{
	gchar *p = strchr(text, '\r');
	guint i, j;

	/* Unix */
	if (!p)
		return;

	/* Mac */
	if (p[1] != '\n') {
		do {
			*p = '\n';
		} while ((p = strchr(p+1, '\r')));

		return;
	}

	/* MS-DOS */
	for (i = 0, j = 0; text[i]; i++) {
		if (text[i] != '\r') {
			text[j] = text[i];
			j++;
		}
	}
	text[j] = '\0';
}

void py_gxsm_console::open_file_callback_exec (GtkDialog *dialog,  int response, gpointer user_data)
{
	py_gxsm_console *pygc = (py_gxsm_console *)user_data;
        if (!pygc-> query_filename || response == GTK_RESPONSE_ACCEPT){
                gchar *file_content;
                GError *err = NULL;

                if (pygc->query_filename) {
                        GtkFileChooser *chooser = GTK_FILE_CHOOSER (dialog);
                        g_autoptr(GFile) file = gtk_file_chooser_get_file (chooser);
                        gchar *tmp_file_name = g_file_get_parse_name (file);
                        pygc->set_script_filename (tmp_file_name);
                        g_free (tmp_file_name);
                        gtk_window_destroy (GTK_WINDOW (dialog));
                }
                else {
                        /* this is a bit of a kludge, so i want to ensure that using the set
                           filename is chosen explicitly each time*/
                        pygc->query_filename = true;
                }
                if (!g_file_get_contents(pygc->script_filename,
                                         &file_content,
                                         NULL,
                                         &err)) {
                        gchar *message = g_strdup_printf("Cannot read content of file "
                                                         "'%s': %s",
                                                         pygc->script_filename,
                                                         err->message);
                        g_clear_error(&err);
                        pygc->append(message);
                        pygc->fail = true;
                        g_free(message);
                        pygc->set_script_filename (NULL);
                }
                else {
                        GtkTextBuffer *console_file_buf;
                        GtkTextView *textview;
                        pygc->fix_eols_to_unix(file_content);

                        // read string which contain last command output
                        textview = GTK_TEXT_VIEW (pygc->console_file_content);
                        console_file_buf = gtk_text_view_get_buffer (textview);
                        // append input line
                        gtk_text_buffer_set_text(console_file_buf, file_content, -1);
                        g_free (file_content);
                        pygc->fail = false;
                }
        }
}

void py_gxsm_console::open_file_callback (GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
	py_gxsm_console *pygc = (py_gxsm_console *)user_data;

        if (pygc->query_filename) {
                GtkWidget *chooser = gtk_file_chooser_dialog_new(_("Open Python script"),
                                                                 pygc->window,
                                                                 GTK_FILE_CHOOSER_ACTION_OPEN,
                                                                 _("_Cancel"), GTK_RESPONSE_CANCEL,
                                                                 _("_Open"), GTK_RESPONSE_ACCEPT,
                                                                 NULL);
                if (pygc->script_filename){
                        GFile *gf=g_file_new_for_path (pygc->script_filename);
                        gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (chooser), gf, NULL);
                }
        
                GtkFileFilter *filter = gtk_file_filter_new();
                gtk_file_filter_add_mime_type(filter, "text/x-python");
                gtk_file_filter_set_name (filter, "Python");
                gtk_file_filter_add_pattern(filter, "*.py");
                gtk_file_chooser_set_filter (GTK_FILE_CHOOSER(chooser), filter);

                gtk_widget_show (chooser);
                g_signal_connect (chooser, "response",
                                  G_CALLBACK (py_gxsm_console::open_file_callback_exec), pygc);
        } else {
                open_file_callback_exec (NULL, GTK_RESPONSE_ACCEPT, pygc);
        }
}

void py_gxsm_console::open_action_script_callback (GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
	py_gxsm_console *pygc = (py_gxsm_console *)user_data;
	GtkTextBuffer *console_file_buf;
	GtkTextView *textview;
        GVariant *old_state=NULL, *new_state=NULL;
        gchar *file_content;
        GError *err = NULL;

        old_state = g_action_get_state (G_ACTION (action));
        new_state = g_variant_new_string (g_variant_get_string (parameter, NULL));
                
        PI_DEBUG_GM (DBG_L1, "py_gxsm_console open_file action %s activated, state changes from %s to %s\n",
                      g_action_get_name (G_ACTION (action)),
                      g_variant_get_string (old_state, NULL),
                      g_variant_get_string (new_state, NULL));

        pygc->set_script_filename (g_variant_get_string (new_state, NULL));
        if (!g_file_get_contents(pygc->script_filename,
                                 &file_content,
                                 NULL,
                                 &err)) {
                gchar *message = g_strdup_printf("Cannot read content of file "
                                                 "'%s': %s",
                                                 pygc->script_filename,
                                                 err->message);
                g_clear_error(&err);
                pygc->append(message);
                pygc->fail = true;
                g_free(message);
                pygc->set_script_filename (NULL);
        }
        else {
                pygc->fix_eols_to_unix(file_content);

                // read string which contain last command output
                textview = GTK_TEXT_VIEW(pygc->console_file_content);
                console_file_buf = gtk_text_view_get_buffer(textview);
                // append input line
                gtk_text_buffer_set_text(console_file_buf, file_content, -1);
                g_free(file_content);
                pygc->fail = false;
        }
        g_simple_action_set_state (action, new_state);
        g_variant_unref (old_state);
}

void py_gxsm_console::save_file_callback (GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
	py_gxsm_console *pygc = (py_gxsm_console *)user_data;
	GtkTextView *textview;
	GtkTextBuffer *buf;
	GtkTextIter start_iter, end_iter;
	gchar *script;
	FILE *f;

	if (pygc->script_filename == NULL) {
		pygc->save_file_as_callback (NULL, NULL, user_data);
	}
	else {
		textview = GTK_TEXT_VIEW (pygc->console_file_content);
		buf = gtk_text_view_get_buffer(textview);
		gtk_text_buffer_get_bounds(buf, &start_iter, &end_iter);
		script = gtk_text_buffer_get_text(buf, &start_iter, &end_iter, FALSE);
		f = g_fopen (pygc->script_filename, "wb");
		if (f) {
			fwrite(script, 1, strlen(script), f);
			fclose(f);
                        pygc->set_script_filename ();
		}
		else {
			gchar *message = g_strdup_printf("Cannot open file '%s': %s",
							 pygc->script_filename,
							 g_strerror(errno));
			pygc->append(message);
			g_free(message);
                        pygc->set_script_filename ("invalid file");
		}
		g_free(script);
	}
}

void py_gxsm_console::save_file_as_callback_exec (GtkDialog *dialog,  int response, gpointer user_data)
{
	py_gxsm_console *pygc = (py_gxsm_console *)user_data;
        if (response == GTK_RESPONSE_ACCEPT){
                GtkFileChooser *chooser = GTK_FILE_CHOOSER (dialog);
                g_autoptr(GFile) file = gtk_file_chooser_get_file (chooser);
                gchar *tmp_file_name = g_file_get_parse_name (file);
                pygc->set_script_filename (tmp_file_name);
                g_free (tmp_file_name);
		pygc->save_file_callback (NULL, NULL, user_data);
	}
        gtk_window_destroy (GTK_WINDOW (dialog));
}

void py_gxsm_console::save_file_as_callback (GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
	py_gxsm_console *pygc = (py_gxsm_console *)user_data;
	GtkWidget *chooser = gtk_file_chooser_dialog_new (_("Save Script as"),
                                                          NULL,
                                                          GTK_FILE_CHOOSER_ACTION_SAVE,
                                                          _("_Cancel"), GTK_RESPONSE_CANCEL,
                                                          _("_Save"), GTK_RESPONSE_ACCEPT,
                                                          NULL);

	GtkFileFilter *filter = gtk_file_filter_new();
        gtk_file_filter_add_mime_type(filter, "text/x-python");
        gtk_file_filter_set_name (filter, "Python");
        gtk_file_filter_add_pattern(filter, "*.py");
        gtk_file_chooser_set_filter (GTK_FILE_CHOOSER(chooser), filter);

        // FIX-ME GTK4
	//gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER (chooser), TRUE);
	//gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER (chooser), default_folder_for_saving);
	gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER (chooser), "Untitled document");

        gtk_widget_show (chooser);
        g_signal_connect (chooser, "response", G_CALLBACK (py_gxsm_console::save_file_as_callback_exec), pygc);
}

void py_gxsm_console::configure_callback (GSimpleAction *action, GVariant *parameter, 
                                          gpointer user_data){
        //py_gxsm_console *pygc = (py_gxsm_console *) user_data;
        GVariant *old_state, *new_state;

        old_state = g_action_get_state (G_ACTION (action));
        new_state = g_variant_new_boolean (!g_variant_get_boolean (old_state));
                
        g_print ("Toggle action %s activated, state changes from %d to %d\n",
                 g_action_get_name (G_ACTION (action)),
                 g_variant_get_boolean (old_state),
                 g_variant_get_boolean (new_state));

        g_simple_action_set_state (action, new_state);
        g_variant_unref (old_state);
        
	if (g_variant_get_boolean (new_state)){
		;
	}
}

static GActionEntry win_py_gxsm_action_entries[] = {
	{ "pyfile-open", py_gxsm_console::open_file_callback, NULL, NULL, NULL },
	{ "pyfile-save", py_gxsm_console::save_file_callback, NULL, NULL, NULL },
	{ "pyfile-save-as", py_gxsm_console::save_file_as_callback, NULL, NULL, NULL },

	{ "pyfile-action-use", py_gxsm_console::open_action_script_callback, "s", "'sf1'", NULL },

	{ "pyremote-configure", py_gxsm_console::configure_callback, NULL, NULL, NULL },
};

void py_gxsm_console::AppWindowInit(const gchar *title, const gchar *sub_title){

        PI_DEBUG_GM (DBG_L2, "pyremote Plugin :: AppWindoInit() -- building Console AppWindow.");

        //        SET_PCS_GROUP("plugin_libpyremote");
        //        gsettings = g_settings_new (GXSM_RES_BASE_PATH_DOT".plugin.common.libpyremote");

        app_window = gxsm4_app_window_new (GXSM4_APP (main_get_gapp()->get_application ()));
        window = GTK_WINDOW (app_window);

        header_bar = gtk_header_bar_new ();
        gtk_widget_show (header_bar);

        g_action_map_add_action_entries (G_ACTION_MAP (app_window),
                                         win_py_gxsm_action_entries, G_N_ELEMENTS (win_py_gxsm_action_entries),
                                         this);

        // create window PopUp menu  ---------------------------------------------------------------------
        file_menu = gtk_popover_menu_new_from_model (G_MENU_MODEL (main_get_gapp()->get_plugin_pyremote_file_menu ()));
        //g_assert (GTK_IS_MENU (file_menu));

        // attach popup file menu button --------------------------------
        GtkWidget *header_menu_button = gtk_menu_button_new ();
        gtk_menu_button_set_icon_name (GTK_MENU_BUTTON (header_menu_button), "document-open-symbolic");
        gtk_menu_button_set_popover (GTK_MENU_BUTTON (header_menu_button), file_menu);
        gtk_header_bar_pack_end (GTK_HEADER_BAR (header_bar), header_menu_button);
        gtk_widget_show (header_menu_button);

        // attach execute action buttons --------------------------------
        GtkWidget *header_action_button = gtk_button_new ();
        gtk_button_set_icon_name (GTK_BUTTON (header_action_button), "system-run-symbolic");
        gtk_header_bar_pack_end (GTK_HEADER_BAR (header_bar), header_action_button);
        gtk_widget_show (header_action_button);
	g_signal_connect (header_action_button, "clicked", G_CALLBACK(py_gxsm_console::run_file), this);
	gtk_widget_set_tooltip_text (header_action_button, N_("Execute Script"));

        header_action_button = gtk_button_new ();
        gtk_button_set_icon_name (GTK_BUTTON (header_action_button), "edit-clear-all-symbolic");
        gtk_header_bar_pack_end (GTK_HEADER_BAR (header_bar), header_action_button);
        gtk_widget_show (header_action_button);
        g_signal_connect (header_action_button, "clicked", G_CALLBACK(py_gxsm_console::clear_output), this);
	gtk_widget_set_tooltip_text (header_action_button, N_("Clear Console Output"));

        header_action_button = gtk_button_new ();
        gtk_button_set_icon_name (GTK_BUTTON (header_action_button), "system-shutdown-symbolic");
        gtk_header_bar_pack_end (GTK_HEADER_BAR (header_bar), header_action_button);
        gtk_widget_show (header_action_button);
	g_signal_connect (header_action_button, "clicked", G_CALLBACK(py_gxsm_console::kill), this);
	gtk_widget_set_tooltip_text (header_action_button, N_("Abort Script"));

        SetTitle (title, "no script file name.");
        gtk_window_set_titlebar (GTK_WINDOW (window), header_bar);

	v_grid = gtk_grid_new ();
        gtk_window_set_child (GTK_WINDOW (window), v_grid);
	g_object_set_data (G_OBJECT (window), "v_grid", v_grid);
	gtk_widget_show (v_grid);

	gtk_widget_show (GTK_WIDGET (window));

        PI_DEBUG_GM (DBG_L2, "pyremote Plugin :: AppWindoInit() -- building Console AppWindow -- calling GUI builder.");
        create_gui ();

        gui_ready = true;
        PI_DEBUG_GM(DBG_L2, "pyremote Plugin :: AppWindoInit() -- Console AppWindow ready.");

        set_window_geometry ("python-console");

        PI_DEBUG_GM(DBG_L2, "pyremote Plugin :: AppWindoInit() -- done.");
}




void py_gxsm_console::create_gui ()
{
	GtkWidget *console_scrolledwin, *file_scrolledwin, *vpaned, *hpaned_scpane, *frame, *sc_grid;
	GtkWidget *entry_input;

	GtkTextView *file_textview, *output_textview;
	//PangoFontDescription *font_desc;
        
	BuildParam *bp;
	BuildParam *bp_sc;
	UnitObj *null_unit;

	GtkSourceLanguageManager *manager;
	GtkSourceBuffer *sourcebuffer;
	GtkSourceLanguage *language;

        PI_DEBUG_GM(DBG_L2, "pyremote Plugin :: create_gui() -- building GUI elements.");

        sc_grid = gtk_grid_new ();

        bp = new BuildParam (v_grid, NULL, main_get_gapp()->RemoteEntryList);
        
	// create static structure;
	exec_value = 50.0; // mid value

	// window
        hpaned_scpane = gtk_paned_new (GTK_ORIENTATION_HORIZONTAL);
        gtk_widget_set_vexpand (hpaned_scpane, TRUE);
        gtk_widget_set_hexpand (hpaned_scpane, TRUE);
        bp->grid_add_widget (hpaned_scpane, 100);
	gtk_widget_show (hpaned_scpane);
        bp->grid_add_widget (hpaned_scpane, 100);

        
	vpaned = gtk_paned_new (GTK_ORIENTATION_VERTICAL);
        gtk_widget_set_hexpand (vpaned, TRUE);
        gtk_widget_set_vexpand (vpaned, TRUE);
	gtk_widget_show (vpaned);
        //bp->grid_add_widget (vpaned, 100);

	gtk_paned_set_start_child (GTK_PANED(hpaned_scpane), vpaned);
	gtk_paned_set_end_child (GTK_PANED(hpaned_scpane), sc_grid);

        bp->new_line ();

	file_scrolledwin = gtk_scrolled_window_new();
	gtk_paned_set_start_child (GTK_PANED(vpaned), file_scrolledwin);
	gtk_widget_show (file_scrolledwin);
        
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(file_scrolledwin),
				       GTK_POLICY_AUTOMATIC,
				       GTK_POLICY_AUTOMATIC);

	console_scrolledwin = gtk_scrolled_window_new();
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(console_scrolledwin),
				       GTK_POLICY_AUTOMATIC,
				       GTK_POLICY_AUTOMATIC);

	gtk_paned_set_end_child (GTK_PANED(vpaned), console_scrolledwin);
	gtk_widget_show (console_scrolledwin);

	// console output
	console_output = gtk_text_view_new();
	output_textview = GTK_TEXT_VIEW (console_output);
        /* create an auto-updating 'always at end' marker to scroll */
        GtkTextBuffer *text_buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (console_output));
        GtkTextIter text_iter_end;
        gtk_text_buffer_get_end_iter (text_buffer, &text_iter_end);

        console_mark_end = gtk_text_buffer_create_mark (text_buffer,
                                                        NULL,
                                                        &text_iter_end,
                                                        FALSE);

        gtk_scrolled_window_set_child (GTK_SCROLLED_WINDOW (console_scrolledwin), console_output);
        gtk_widget_show (console_output);
	gtk_text_view_set_editable(output_textview, FALSE);

	// source view file buffer
        sourcebuffer = gtk_source_buffer_new (NULL);
        console_file_content = gtk_source_view_new_with_buffer (sourcebuffer);
	file_textview = GTK_TEXT_VIEW(console_file_content);
	gtk_source_view_set_show_line_numbers(GTK_SOURCE_VIEW(file_textview), TRUE);
	gtk_source_view_set_auto_indent(GTK_SOURCE_VIEW(file_textview), TRUE);
	manager = gtk_source_language_manager_get_default();

	language = gtk_source_language_manager_get_language(manager, "pygwy");
	if (!language)
		language = gtk_source_language_manager_get_language(manager, "python");
	gtk_source_buffer_set_language(sourcebuffer, language);
	gtk_source_buffer_set_highlight_syntax(sourcebuffer, TRUE);

#if 0
	console_file_content = gtk_text_view_new();
	file_textview = GTK_TEXT_VIEW(console_file_content);
#endif

	// set font
	//font_desc = pango_font_description_from_string ("Monospace 8");
	//gtk_widget_override_font (console_file_content, font_desc);
        //gtk_widget_override_font (console_output, font_desc);
	//pango_font_description_free (font_desc);

	gtk_scrolled_window_set_child (GTK_SCROLLED_WINDOW (file_scrolledwin), console_file_content);
        gtk_widget_show (console_file_content);
	gtk_text_view_set_editable (file_textview, TRUE);

	frame = gtk_frame_new (N_("Command"));
	entry_input = gtk_entry_new ();
	gtk_frame_set_child (GTK_FRAME (frame), entry_input);
        gtk_widget_show (frame);
        gtk_widget_show (entry_input);
	gtk_entry_set_invisible_char (GTK_ENTRY(entry_input), 9679);
	gtk_widget_grab_focus (GTK_WIDGET(entry_input));
        bp->grid_add_widget (frame, 80);
        
	null_unit = new UnitObj(" "," ");
	bp->grid_add_ec ("Script Control", null_unit, &exec_value, 0.0, 100.0, "4g", 1., 10., "script-control");

        main_get_gapp()->RemoteEntryList = bp->get_remote_list_head ();

        bp_sc = new BuildParam (sc_grid, NULL, gapp->RemoteEntryList);
        for(int i=0; i<NUM_SCV; ++i){
                sc_value[i] = 0.0;
                gchar *l = g_strdup_printf ("SC%d",i+1);
                gchar *id = g_strdup_printf ("py-sc%02d",i+1);
                bp_sc->grid_add_ec (l, null_unit, &sc_value[i], -1e10, 1e10, "g", 1., 10., id);
                sc_label[i] = bp_sc->label;
                g_free (l);
                g_free (id);
                bp_sc->new_line ();
        }
        gapp->RemoteEntryList = bp_sc->get_remote_list_head ();
        
	g_signal_connect(entry_input, "activate",
			 G_CALLBACK(py_gxsm_console::command_execute), this);
        
	gtk_text_view_set_wrap_mode (output_textview, GTK_WRAP_WORD_CHAR);

        gtk_widget_show (v_grid);

        //gtk_paned_set_position (GTK_PANED(vpaned), 50);
	//gtk_paned_set_position (GTK_PANED(vpaned), -1); // 1:1 -- unset default
	gtk_paned_set_position (GTK_PANED(hpaned_scpane), 300);

        PI_DEBUG_GM(DBG_L2, "pyremote Plugin :: console_create_gui() -- building GUI elements.... completed.");
}


// Small idiotic function to create a file with some pyremote example commands if none is found.
void  py_gxsm_console::write_example_file(void)
{
	std::ofstream example_file;
	example_file.open(example_filename);
	example_file << "\n#Since no default file / script was found, here are some\n"
		"# things you can do with the python interface.\n\n"
                "# Please visit/install gxsm4-lib-utils.py, ... before executing this:\n"
                "# Simply visit Menu/Libraries/Lib Utils first.\n"
                "\n"
                "#GXSM_USE_LIBRARY <gxsm4-lib-scan>\n"
                "\n"
		"# You can also create the more extensive default/example tools collection: File->Use default.\n"
		"# - see the manual for more information\n"
                "# Load scan support object from library\n"
                "\n"
                "scan = scan_control()\n"
                "\n"
		"# Execute to try these\n"
		"c = gxsm.get(\"script-control\")\n"
		"print \"Control value: \", c\n"
		"if c < 50:\n"
		"    print \"control value below limit - aborting!\"\n"
		"    print \"you can set the value in the window above\"\n"
		"    # you could set Script_Control like any other variable - see below\n"
		"else:\n"
		"    gxsm.set (\"RangeX\",\"200\")\n"
		"    gxsm.set (\"RangeY\",\"200\")\n"
		"    gxsm.set (\"PointsX\",\"100\")\n"
		"    gxsm.set (\"PointsY\",\"100\")\n"
		"    gxsm.set (\"dsp-fbs-bias\",\"0.1\")\n"
		"    gxsm.set (\"dsp-fbs-mx0-current-set\",\"1.5\")\n"
		"    gxsm.set (\"dsp-fbs-cp\", \"25\")\n"
		"    gxsm.set (\"dsp-fbs-ci\", \"20\")\n"
		"    gxsm.set (\"OffsetX\", \"0\")\n"
		"    gxsm.set (\"OffsetY\", \"0\")\n"
		"#    gxsm.action (\"DSP_CMD_AUTOAPP\")\n"
		"    gxsm.sleep (10)\n"
                "\n"
		"    #gxsm.startscan ()\n"
                "\n"
                "    scan.run_set ([-0.6, -0.4, -0.3])  # simply executes a scan for every bias voltage given in the list.\n"
                "\n"
	        "    gxsm.moveto_scan_xy (100.,50.)\n";
	example_file.close();
}

void py_gxsm_console::run()
{
	PyObject *d;

	PI_DEBUG_GM(DBG_L2, "pyremote Plugin :: console_run()");

        // are we up already? just make sure to (re) present window, else create
        if (gui_ready){
                gtk_window_present (GTK_WINDOW(window));
		return;
	} else {
		AppWindowInit (pyremote_pi.help);
        }

        append("Welcome to the PyRemote Control for GXSM: Python Version is ");
        append(Py_GetVersion());
              
	script_filename = NULL;
        fail = false;
	// create new environment
	d = create_environment("__console__", FALSE); // was FALSE, want to see change in error message handling
	if (!d) {
		g_warning("Cannot create copy of Python dictionary.");
		return;
	}

	PI_DEBUG_GM(DBG_L2, "pyremote Plugin :: console_run() run_string to import gxsm module.");

	run_string("import gxsm\n",
        	   Py_file_input,
		   d,
		   d);

        dictionary = d;

	// try loading the default pyremote file
	script_filename = g_strdup_printf("%s.py", xsmres.PyremoteFile);
	query_filename = false;

	PI_DEBUG_GM(DBG_L1, "Pyremote console opening >%s< ", script_filename);
	open_file_callback (NULL, NULL, this);

	// put some small sommand example if no file is found
	if (fail) {
                PI_DEBUG(DBG_L1, "Pyremote console opening failed. Generating example.");
		query_filename = false;
		write_example_file();
		script_filename = g_strdup(example_filename);
		open_file_callback (NULL, NULL, this);
	}

        append("\n\n");
        append("WARNING: ================================================================================\n");
        append("WARNING: GXSM4 beta work in progress: embedded python running in it's own thread.\n");
        append("WARNING: pending compete testing of a few less used python functions and thead save validation. \n");
        append("WARNING: ================================================================================\n\n");
        
        PI_DEBUG_GM(DBG_L2, "pyremote Plugin :: console_run() -- startup finished and ready. Standing by.");
}

void py_gxsm_console::command_execute(GtkEntry *entry, gpointer user_data)
{
	py_gxsm_console *pygc = (py_gxsm_console *)user_data;
	gchar *input_line;
	const gchar *command;
	GString *output;

        input_line = g_strconcat(">>> ", gtk_entry_buffer_get_text (GTK_ENTRY_BUFFER (gtk_entry_get_buffer (GTK_ENTRY (entry)))), "\n", NULL);
	output = g_string_new(input_line);
	command = gtk_entry_buffer_get_text (GTK_ENTRY_BUFFER (gtk_entry_get_buffer (GTK_ENTRY (entry))));
	output = g_string_append(output,
				 pygc->run_command(command, Py_single_input));

	pygc->append((gchar *)output->str);
	g_string_free(output, TRUE);

	gtk_editable_select_region(GTK_EDITABLE(entry), 0, -1);
}


// -------------------------------------------------- GXSM PLUGIN INTERFACE

static void run_action_script_callback (gpointer data){
        const gchar *name = *(gchar**)data;
        py_gxsm_remote_console->run_action_script (name);
}

// init-Function
static void pyremote_init(void)
{
	/* Python will search for remote.py in the directories, defined
	   by PYTHONPATH. */
	PI_DEBUG_GM(DBG_L2, "pyremote Plugin Init");
	if (!getenv("PYTHONPATH")){
		PI_DEBUG_GM(DBG_L2, "pyremote: PYTHONPATH is not set.");
		PI_DEBUG_GM(DBG_L2, "pyremote: Setting to '.'");
		setenv("PYTHONPATH", ".", 0);
	}

	if (!py_gxsm_remote_console){
		py_gxsm_remote_console = new py_gxsm_console (main_get_gapp() -> get_app ());
	
                main_get_gapp()->ConnectPluginToRemoteAction (run_action_script_callback);
        }

        py_gxsm_remote_console->run();
}


// cleanup-Function
static void pyremote_cleanup(void)
{
	PI_DEBUG_GM(DBG_L2, "Pyremote Plugin Cleanup");
	if (py_gxsm_remote_console){
                PI_DEBUG(DBG_L3, "Pyremote Plugin: savinggeometry forced now.");
                py_gxsm_remote_console->SaveGeometry (); // some what needed and now it running the destruictor also. Weird.
                PI_DEBUG(DBG_L3, "Pyremote Plugin: closing up remote control console: delete py_gxsm_remote_console.");
		delete py_gxsm_remote_console;
        }
        py_gxsm_remote_console = NULL;
}

void pyremote_run( GtkWidget *w, void *data ){
	PI_DEBUG_GM(DBG_L2, "pyremote Plugin Run Console.");

        // check if we are created -- should be.
        if (!py_gxsm_remote_console)
		py_gxsm_remote_console = new py_gxsm_console (main_get_gapp() -> get_app ());

	PI_DEBUG_GM(DBG_L2, "pyremote Plugin Run: Console-Run");
	py_gxsm_remote_console->run();
}
