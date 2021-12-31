#!/usr/bin/python3 -Es

## * script to update GXSM and SRanger project files
## * via SVN or apt-get
## *
## * Copyright (C) 2009, 2010, 2011, 2014 Thorsten Wagner / ventiotec (R) Dolega und Wagner GbR
## * Copyright (C) 2020 Thorsten Wagner / ventiotec (R) Wagner und Muenchow GbR
## *
## * Author: Thorsten Wagner <linux@ventiotec.com>
## * WWW Home: http://sranger.sf.net / http://www.ventiotec.com
## *
## * This program is free software; you can redistribute it and/or modify
## * it under the terms of the GNU General Public License as published by
## * the Free Software Foundation; either version 2 of the License, or
## * (at your option) any later version.
## *
## * This program is distributed in the hope that it will be useful,
## * but WITHOUT ANY WARRANTY; without even the implied warranty of
## * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
## * GNU General Public License for more details.
## *
## * You should have received a copy of the GNU General Public License
## * along with this program; if not, write to the Free Software
## * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.


###############################################################################
#
#      Please adapt the following settings to your own needs
#
###############################################################################

## (home) directory containing the source codes in the subfolders Gxsm-3.0 and SRanger 
import os	# use os because python IO is bugy
home_dir = os.environ["HOME"]

import os
import re
import lsb_release

###############################################################################
# declarations and imports
###############################################################################


version = "2.0 (31.10.2020)"

import gi
gi.require_version("Gtk", "3.0")
from gi.repository import Gtk, GLib

import array

wins = {}

def delete_event(win, event=None):
	win.hide()
	# don't destroy window -- just leave it hidden
	return True

# update Gxsm via SVN
def svn_gxsm_update():
	add_event("Updating Gxsm-3.0 source code.")
	add_event("Please watch also the terminal!")
	add_event("You may have to make inputs there.")	
	os.chdir(home_dir)
	if os.path.exists("gxsm3-svn"):
	  add_event("Update sourcecode")
	  os.system("cd gxsm3-svn; svn update; cd ..")
	else:
	  add_event("Checkout sourcecode")
	  os.system("svn checkout https://svn.code.sf.net/p/gxsm/svn/trunk/Gxsm-3.0 gxsm3-svn")
	add_event("Compiling and packing Gxsm-3.0.")
	distinfo = lsb_release.get_distro_information()
	os.chdir("gxsm3-svn")
	if os.path.exists("debian/control_"+distinfo.get('ID', 'n/a').lower()+distinfo.get('RELEASE', 'n/a')):
		add_event("proper control file found!\n")
		os.system("cp debian/control_"+distinfo.get('ID', 'n/a').lower()+distinfo.get('RELEASE', 'n/a')+" debian/control")
		os.system("./autogen.sh; make")
		os.system("debuild -b -uc -us -I")
		add_event("Installing Gxsm-3.0 via dpkg.")
		os.system("sudo dpkg -i ../gxsm3_*.deb")
		add_event("Gxsm installation done.")
	else:
		add_event("No suitable control file found")
		add_event("for "+distinfo.get('ID', 'n/a').lower()+distinfo.get('RELEASE', 'n/a')+"\n")
	return

def svn_sranger_update():
	add_event("Updating SRanger source code.")
	add_event("Please watch also the terminal!")
	add_event("You may have to make inputs there.")
	os.chdir(home_dir)
	if os.path.exists("sranger-svn"):
	  add_event("Update sourcecode")
	  os.system("cd sranger-svn; svn update; cd ..")
	else:
	  add_event("Checkout sourcecode")
	  os.system("svn checkout https://svn.code.sf.net/p/sranger/svn/trunk/SRanger sranger-svn")
	os.chdir(home_dir)	
	os.chdir("sranger-svn/SR_STD_SP2/kernel-module")	
	add_event("Compiling SRanger kernel modul.")	
	os.system("debuild -b -uc -us -I")
	add_event("Done. Packing via debuild.")	
	os.system("sudo dpkg -i ../sranger-modules-std*.deb")
	add_event("Kernel modul for SR-STD/SP2 should be ready to use.")	
	event_list.set_label("")
	add_event("Updating SRanger MK2/MK3Pro kernel modul.")
	add_event("Please watch also the terminal!")
	add_event("You may have to make inputs there.")
	os.chdir(home_dir)
	os.chdir("sranger-svn/SR_MK2_MK3Pro/kernel-module")
	add_event("Compiling SRanger kernel modul.")	
	os.system("debuild -b -uc -us -I")
	add_event("Done. Installing via dpkg.")	
	os.system("sudo dpkg -i ../sranger-modules-mk23*.deb")
	add_event("Kernel modul for SR-MK2/MK3Pro should be ready to use.")	
	return

def apt_gxsm_update():
	add_event("Updating Gxsm-3.0 from apt repositry.")
	check_sources_list()
	os.system("sudo apt install -y --allow-unauthenticated gxsm3")
	add_event("Gxsm update done via apt-get.")
	return

def apt_kernel_update():
	add_event("Updating SRanger kernel modules\n from apt repositry.")
	check_sources_list()
	os.system("sudo apt-get -y --allow-unauthenticated install sranger-modules-mk23-dkms sranger-modules-std-dkms")
	add_event("SRanger kernel update done via apt-get.")
	return

def check_sources_list():
	# Check if sources.list exists
	os.system("sudo apt-add-repositry ppa:totto/gxsm")	
	os.system("sudo apt-get update")
	add_event("Updating cache of apt-get.")
	return

def svn_button(button_svn):
	global button_svn_handler
	button_svn.disconnect(button_svn_handler)
	button_svn.set_label("CANCEL")
	button_svn_handler = button_svn.connect("clicked", destroy)
	event_list.set_label("")	
	svn_gxsm_update()
	event_list.set_label("")
	svn_sranger_update()
	button_svn.disconnect(button_svn_handler)
	button_svn.set_label("SVN")
	button_svn_handler = button_svn.connect("clicked", svn_button)
	return
	
def apt_button(button_apt):
	global button_apt_handler	
	button_apt.disconnect(button_apt_handler)
	button_apt.set_label("CANCEL")
	button_apt_handler = button_apt.connect("clicked", destroy)
	event_list.set_label("")
	apt_gxsm_update()
	event_list.set_label("")
	apt_kernel_update()
	button_apt.disconnect(button_apt_handler)
	button_apt.set_label("APT")
	button_apt_handler = button_apt.connect("clicked", apt_button)
	return	

def cancel_button(button):
	Gtk.main_quit()

def destroy(*args):
	os.chdir(home_dir)
	Gtk.main_quit()


# adds a new entry to the event_list
def add_event(text):
	event_list.set_label(event_list.get_label() + text + "\n")
	while Gtk.events_pending():
      		Gtk.main_iteration_do(False) 
	return


def create_main_window():
	global event_list
	global button_apt_handler
	global button_svn_handler
	global button_cancel_handler
	global button_kernel_handler
	global win
	win = Gtk.Window(title="update Gxsm3 and SRanger tools")
	win.set_size_request(300, 320)
	win.connect("destroy", destroy)
	win.connect("delete_event", destroy)
	
	box1 = Gtk.VBox()
	win.add(box1)
	box1.show()
	scrolled_window = Gtk.ScrolledWindow()
	scrolled_window.set_policy(Gtk.PolicyType.AUTOMATIC,Gtk.PolicyType.AUTOMATIC)
	box1.pack_start(scrolled_window,True, True, 0)
	scrolled_window.show()
	
	box2 = Gtk.VBox()
	box2.set_border_width(5)
	box1.pack_start(box2, True, True, 0)
	box2.show()
	
	event_list = Gtk.Label(label="Welcome to the\nGxsm-SRanger-Updater\nVersion " + version + "\n")
	distinfo = lsb_release.get_distro_information()
	add_event("running on " + distinfo.get('ID', 'n/a') +" "+ distinfo.get('RELEASE', 'n/a')+"\n")
	add_event("You can update the Gxsm-3.0 and")
	add_event("the SRanger package via")	
	add_event("SVN giving you the latest")
	add_event("development version or")
	add_event("via apt-get giving you a more")
	add_event("stable version.")
	box2.pack_start(event_list,True,True,0)
	event_list.show()

	separator = Gtk.HSeparator()
	box1.pack_start(separator,True,True,0)
	separator.show()
	box2 = Gtk.VBox(spacing=10)
	box2.set_border_width(5)
	box1.pack_start(box2,True,True,0)
	box2.show()
	
	# Button for update via SVN
	button_svn = Gtk.Button.new_with_mnemonic("_SVN")
	button_svn_handler = button_svn.connect("clicked", svn_button)
	button_svn.set_can_default(True)
	box2.pack_start(button_svn,True,True,0)
	button_svn.show()
	
	# Button for update via apt-get
	button_apt = Gtk.Button.new_with_mnemonic("_APT")
	button_apt_handler = button_apt.connect("clicked", apt_button)
	button_apt.set_can_default(True)
	box2.pack_start(button_apt,True,True,0)
	button_apt.show()

	# Button to cancel
	button_cancel = Gtk.Button.new_with_mnemonic("ABORT")
	button_cancel_handler = button_cancel.connect("clicked", cancel_button)
	button_cancel.set_can_default(True)
	button_svn.grab_default()
	box2.pack_start(button_cancel,True,True,0)
	button_cancel.show()
	
	win.show()

create_main_window()
Gtk.main()
print ("Bye bye!")

