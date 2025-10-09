# Gnome X Scanning Microscopy (GXSM)
<img align="right" width="128" height="128" alt="gxsm4-icon128" src="https://github.com/user-attachments/assets/2a5863e2-8d5c-4d76-9812-4d5226c5149a" />
GXSM -- Gnome X Scanning Microscopy: A multi-channel multi-dimension but usually image based data acquisition and visualization system designed for SPM techniques (STM,AFM..).
Our General-Vector-Program/Probe engine allows for arbitrary complex multidimensional "imaging/probing/manipulations/..." of all kinds at a huge dynamic range of data and timescales down to a few Nanoseconds with out latest hardware platform.

A plug-in interface allows any user add-on data-processing and special hardware and instrument support.

Based on several hardware options it supports a commercially available DSP hardware (getting outdated, but still fully supported by Gxsm3 (https://github.com/pyzahl/Gxsm3) and Gxsm4. No further developments planned.) "Signal Ranger MK2 and MK3" (see https://www.softdb.com/scanning-probe-microscopy/) and provided also open source code for all the low level signal processing tasks and instrument controls in a most flexible and adaptable manner.

## Latest
### Get excited: RPSPMC for GXSM4 is here and fully functional!
RPSPMC for GXSM4 arrived from a vision reality made happen now! This is an all new, very modular (analog in/out modules) and most importamt complete all on one FPGA level SPM control with all it needs for teh most advanced SPM including PAC-PLL, LockIn, etc. and at warp speed/data throughput up to 400 MB/s and control down to a few ns time scale allowing for arbitratry function generation, frequency sweeps and more -- with ESR and more in mind.
And it's been depoyed for production use in the laboratory already since April 2025!
Dive into it starting here: https://github.com/pyzahl/Gxsm4/tree/master/plug-ins/hard/RPSPMC (#)
We are working hard on making the hardware availabe to our users community but it will take some time and for various complex reasons the RPSPMC GIT repository is currently set to (#) private to procect our highly sophistiacted logic designs.

The GXSM Web Site historically remains hosted on SF: http://gxsm.sf.net

## Table of contents

- [Gnome X Scanning Microscopy (GXSM)](#gnome-x-scanning-microscopy-gxsm)
  - [Latest](#latest)
  - [Table of contents](#table-of-contents)
  - [Introduction to GXSM](#introduction-to-gxsm)
  - [Installation](#installation)
    - [Requirements to build and install from GIT source](#requirements-to-build-and-install-from-git-source) 
    - [New build tool: Meson buildsystem](#new-build-tool-meson-buildsystem)
  - [Pending ports and known issues](#pending-ports-and-known-issues)
    - [Odds and FYI](#odds-and-fyi)
  - [GXSM community](#gxsm-community)
    - [How to report bugs](#how-to-report-bugs)
    - [Patches and contributions](#patches-and-contributions)
    
# Introduction to GXSM

Gxsm4 was derived from proting of Gxsm3 (gtk3) to gtk4. But now is continuing out mission on providing the Scanning Probe Microscopy (SPM) community with a modern, visual and powerfull unrestricted tool for both, instrumnet control and data analysis. Gxsm is offering als a fully integrated Python interpreter for simple to most demanding automatization tasks including deploying AI of any kind.


# Installation

## Requirements to build and install from GIT source

Gxsm4 requires GTK4, GtkSourceView5, libfftw, libnetcdf, libquicktime, ... Therefore, please install a recent linux distribution like debian or ubuntu (>= 22.04 LTS). To run GXSM4 with Wayland as window manager, you have two alternative to tweak your linux: i) In Ubuntu 22.04 Wayland is the default window manager if you are not using an nvidia gpu. To deactivate Wayland support, please add/enalbe as root in /etc/gdm3/custom.conf the line. 

Native development system is currently Debain 12, 13 (Trixie) or Debian-testing:

```
apt-get install jed emacs meson cmake libgtk-4-dev libsoup-gnome2.4-dev gsettings-desktop-schemas-dev libglew-dev 
apt-get install libnetcdf-c++4-dev ncview libnetcdf-dev libnetcdf-cxx-legacy-dev libglm-dev libjson-glib-dev
apt-get install libfftw3-dev libgsl-dev libgtksourceview-5-dev python3-dev libpython3-all-dev python3-numpy
apt-get install libopencv-*-dev libquicktime-dev
```

may need to add local lib path below to /etc/ld.so.conf.d/local.conf

/usr/local/lib/x86_64-linux-gnu

and run ldconfig.

Install also:
```
apt-get install dconf-editor
```

To obtain a copy of the source code, please run in a terminal:  
``` 
 $ git clone https://github.com/pyzahl/Gxsm4 gxsm4-git
``` 
## New build tool: Meson buildsystem

First create your "builddir" in the project root folder.
Then run in the folder Gxsm4-git
``` 
 $ meson builddir
 $ cd builddir
 $ ninja install
```

To uninstall call in the buildir
``` 
ninja uninstall
```

# Pending ports and known issues

- Window Position Management....  gtk4 dropped native access to window geometry controls -- a big convenience and usability issue for diverse multi control window applications 🙁
		- X11: works again via native X11 calls... (fully functional on X11)
		- Wayland: likely never a solution ever as the default Wayland Compositor dose not give access at all to window's positioning. But please educate me if I am wrong.
		  Gtk4 and Wayland Compositor does not allow Window Geometry managemnt from Gxsm.
          Solution: For convenient efficient work used X11 display backend. Or, hang in in tight:
  		- Hyprland (on Wayland): The more modern Wayland provides a better and snappier experience vs. aging X11. And there is hope: For those loving exploring experimental and cutting edge tools: the latest tweaks and developments of Gxsm4 provide now experimental support for Window Geometry managenemnt when using Hyprland on Wayland via hyprctl! See: https://hypr.land/ For Debian Tetsing/Trixie start your adventure here: https://github.com/JaKooLit/Debian-Hyprland
		
- GL3D Scan View Mode: currently disabled. Port pending.

- known GTK4 shortcomings so far noted:
  - on X11 onyl: Window focus to activiate keyboard accellerator is at random lagging. Requires expplicit Menu call to get attention?!?!
  - Rendering in cairo fall back mode (when using X11 export via ssh -X for example) can be slow.
  - press/release signals not available for simple button widget. Work around assigning handlers does not work as expected. Work for a canvas "home made" button. Non perfec tbu tworkable workaround currently used: Arrow icons on button widget accept press and release events. (Needed for Mover Controls: "fire wave signal on DSP when pressed" direction buttons.

- Pending back/forward sync or porting from gxsm3: idle callbacks for Tip Move and related vs. blocking or singel shot. Address pending minor random but rare move issue with initiating a scan.
- Pending odd behavior for object move/edit in some situations. (Workaround: remove all (F12), start over)

- SRANGER-MK23/ MULIT TAB MANAGEMENT: DSP-CONTROL windows A, B -- initial TAB Drag to empty secondary window impossible (or hard??) to find a hook area to drop off. Work around for now:
Manually hack config via dconf-editor, then further DnD is easy and as usual again:

  - Set for example
```  
/org/gnome/gxsm4/hwi/sranger-mk23/window-01-tabs = 'aclefghkmn------------------------------'
and
/org/gnome/gxsm4/hwi/sranger-mk23/window-02-tabs = 'dijo----------------------------'
```
Then start gxsm4 again.

 - Save Profile as Drawing=pdf -> crash

---
## Odds and FYI

I disabled warning messages in configure.ac (CFLAG -w) to not get flooded by a silly issue I cannot fix and added -fpermissive.
This is needed as I get an error by the C++ compiler per default now when I xor  GTK_FLAGS  like A | B ... as this foces a typ conversion to int...
Oh well some C stuff.

# GXSM Community

## How to report bugs

Bugs should be reported to the gitlab bug tracking system.
(https://github.com/pyzahl/Gxsm4/issues). You will need to create an
account for yourself.

In the bug report please include:

* Information about your system. For instance:

   - What version of gxsm4
   - What operating system and version
   - What version of the gtk, glib and gnome libraries

  And anything else you think is relevant.

* How to reproduce the bug. 

* If the bug was a crash, the exact text that was printed out when the
  crash occurred.

* Further information such as stack traces may be useful, but is not
  necessary. If you do send a stack trace, and the error is an X error,
  it will be more useful if the stack trace is produced running the test
  program with the --sync command line option.


## Patches and contributions

Patches should also be submitted to https://github.com/pyzahl/Gxsm4/. 
If the patch fixes an existing bug, add the patch as an attachment to 
that bug report.

Otherwise, enter a new bug report that describes the patch, and attach
the patch to that bug report.

Please create patches with the git format-patch command.

If you are interested in helping us to develop gedit, please see the 
file 'AUTHOR' for contact information and/or send a message to the gedit
mailing list. See also the file 'HACKING' for more detailed information.

---

  *The gxsm team.*
