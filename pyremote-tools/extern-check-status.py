#!/usr/bin/env python3

### #!/usr/bin/env python

## * Python XYZ View and Coarse Template GUI
## * with demo Gxsm4+RPSPMC external actions
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

version = "1.0.0"

#import gi
#gi.require_version('Gtk', '4.0')
#from gi.repository import Gtk, Gdk, GLib
#import cairo
import os                # use os because python IO is bugy
import time
import array
import math
import numpy as np

import gxsm4process as gxsm4

gxsm = gxsm4.gxsm_process()
                        
if __name__ == "__main__":
        print ("*** MAIN ***")
        print('MainAutoSave:', gxsm.action("GETCHECK-MainAutoSave"))
        print('Scan Repeat :', gxsm.action("GETCHECK-SCAN-REPEAT"))
        print ("*** EXITING ***")



