# **ATTENTION:**

**THIS IS A BETA VERSION AND WORK IN PROGRESS of Gxsm4 ported to GTK4 from Gxsm-3.0 on SF (gxsm.sf.net).**
There are minor gtk4 related issues and shortcomings at this time
and this version is for evaluation and future migration readyness.
See details of pending / not functional parts below in section 3.
(C) PyZahl 2021-12-31

# 1. General Information

GXSM -- Gnome X Scanning Microscopy: A multi-channel image and vector-probe data acquisition and visualization system designed for SPM techniques (STM,AFM..). A plug-in interface allows any user add-on data-processing and special hardware and instrument support.

Based on several hardware options it supports a commercially available DSP hardware (see https://www.softdb.com/scanning-probe-microscopy/) and provided also open source code for all the low level signal processing tasks and instrument controls in a most flexible and adaptable manner.

All latest stable software is available now via GIT:
(Gxsm4 Beta currently) https://github.com/pyzahl/Gxsm4

or Live Demo/Install CD (with GXSM3):
http://www.ventiotec.de/linux/GXSM-Linux.iso

GXSM Web Site: http://gxsm.sf.net

# 2. Installation

Gxsm4 requires GTK4, GtkSourceView5, libfftw, libnetcdf, libquicktime, ... Therefore, please install a recent linux distribution like debian or ubuntu (>= 22.04 LTS). To run GXSM4 with Wayland as window manager, you have two alternative to tweak your linux: i) In Ubuntu 22.04 Wayland is the default window manager if you are not using an nvidia gpu. To deactivate Wayland support, please add/enalbe as root in /etc/gdm3/custom.conf the line. 

Native development system is currently Debain 12 or Debian-testing:

apt-get install jed emacs meson cmake libgtk-4-dev libsoup-gnome2.4-dev gsettings-desktop-schemas-dev libglew-dev 
apt-get install libnetcdf-c++4-dev ncview libnetcdf-dev libnetcdf-cxx-legacy-dev libglm-dev
apt-get install libfftw3-dev libgsl-dev libgtksourceview-5-dev python3-dev libpython3-all-dev python3-numpy
apt-get install libopencv-*-dev libquicktime-dev

add local lib path below to /etc/ld.so.conf.d/local.conf

/usr/local/lib/x86_64-linux-gnu

and run ldconfig.

Install
apt-get install dconf-editor

Disable /org/gnome/gxsm4 Splash if running Wayland! (Splan window can not be positioned/displayed and freezes gxsm4 at start)

```
WaylandEnable=false
```
ii) Alternatively, deactivate the splash screen during GXSM4 startup. Open the dconf-editor and navigate to org/gnome/gxsm4. Here change the entry "splash" to off.

To obtain a copy of the source code, please run in a terminal:  
``` 
 $ git clone https://github.com/pyzahl/Gxsm4 gxsm4-git
``` 
## a) New build tool: Meson buildsystem -- work in progress:

First create your "builddir" in the project root folder.
Then run in the folder Gxsm4-git
``` 
 $ meson builddir
 $ cd builddir
 $ meson compile
 $ meson install
```
Here, "meson compile, meson install" calls ninja. You can simply call "ninja install" to do it all.

To uninstall call in the buildir
``` 
ninja uninstall
```

## b) New package tool: Flatpak -- work in progress:

Install flatpak and flathub
``` 
 $ sudo apt install flatpak flatpak-builder -y
 $ sudo flatpak remote-add --if-not-exists flathub https://flathub.org/repo/flathub.flatpakrepo
```
Install the gnome sdk within your flatpak enviroment:
``` 
 $ flatpak install flathub org.gnome.Sdk//41 org.gnome.Platform//41
```

Now install and run GXSM4 (assuming that the json-file/source is in the folder gxsm4-git
``` 
 $ mkdir flatpak_builddir
 $ flatpak-builder --user --install --force-clean flatpak_builddir gxsm4-git/org.gnome.gxsm4.json
 $ flatpak run org.gnome.Gxsm4
```
 
## 3. To-Do-List

- Testing testing testing

## 4. Pending and known issues:

- Window Position Management....  gtk4 does not provide any hands on that any more -- big convenience and usability issue 🙁
		- X11: works again via native X11 calls...
		- Wayland: likely never a solution ever as Wayland dose not give access at all to window's positioning. But please educate me if I am wrong.
		- 
- GL3D Scan View Mode: currently disabled. Port pending.

- All ENTRIES: added configuration menu option not yet attached and not accessible. Need to figure out how to add a custom menu entry to gtk_popovers.
             It is possible to manually edit the properties via the dconf-editor to get started.
	     Some thing seams slow here at build and update, or has post idel latency.

- known GTK4 shortcomings so far noted: 
  - Long popup/down selection lists, items out of "screen limits" are not sccessible, no/missing scrolling feature, etc.. No idea yet how to make this work right.
  - Rendering in cairo fall back mode (when using X11 export via ssh -X for example) is very slow -- some where around a magnitude (10x) slow! What makes remote work nearly impossible. However, varies a lot by "fetaure" used. Menu pop ups are very slow, take long to appear. GUI initial  build (many entries, etc.) takes a long time.
  - press/release signals not available for simple button widget. Work around assigning handlers does not work as expected. Work for a canvas "home made" button. Non perfec tbu tworkable workaround currently used: Arrow icons on button widget accept press and release events. (Needed for Mover Controls: "fire wave signal on DSP when pressed" direction buttons.

- Pending back/forward sync or porting from gxsm3: idle callbacks for Tip Move and related vs. blocking or singel shot. Address pending minor random but rare move issue with initiating a scan.
- Pending odd behavior for object move/edit in some situations. (Workaround: remove all, start over)
- Pending: some hot keys are non functional

- DSP-CONTROL windows A, B -- initial TAB Drag to empty secondary window impossible (or hard??) to find a hook area to drop off. Work around for now:
Manually hack config via dconf-editor, then further DnD is easy and as usual again:

  - Set for example
/org/gnome/gxsm4/hwi/sranger-mk23/window-01-tabs = 'aclefghkmn------------------------------'
and
/org/gnome/gxsm4/hwi/sranger-mk23/window-02-tabs = 'dijo----------------------------'
Then start gxsm4 again.

 - Save Profile as Drawing=pdf -> crash


## FYI:
----
I disabled warning messages in configure.ac (CFLAG -w) to not get flooded by a silly issue I cannot fix and added -fpermissive.
This is needed as I get an error by the C++ compiler per default now when I xor  GTK_FLAGS  like A | B ... as this foces a typ conversion to int...
Oh well some crap.

much more ... to be figured out ans tested ....


## 4. How to report bugs

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


## 5. Patches

Patches should also be submitted to https://github.com/pyzahl/Gxsm4/. 
If the patch fixes an existing bug, add the patch as an attachment to 
that bug report.

Otherwise, enter a new bug report that describes the patch, and attach
the patch to that bug report.

Please create patches with the git format-patch command.

If you are interested in helping us to develop gedit, please see the 
file 'AUTHOR' for contact information and/or send a message to the gedit
mailing list. See also the file 'HACKING' for more detailed information.


  *The gxsm team.*
