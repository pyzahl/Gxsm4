/* Gxsm - Gnome X Scanning Microscopy
 * universal STM/AFM/SARLS/SPALEED/... controlling and
 * data analysis software
 * 
 * Copyright (C) 1999,2000,2001,2002,2003 Percy Zahl
 *
 * Authors: Percy Zahl <zahl@users.sf.net>
 * additional features: Andreas Klust <klust@users.sf.net>
 * WWW Home: http://gxsm.sf.net
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

//
// Gnome Shell Extension Hack for Gxsm Window Management under Wayland
// install (copy) files to ~/.local/share/gnome-shell/extensions/gxsm-wm@gxsm4.gnome.org
// re-Login to Wayland
// Enable Extension 'Gxsm WM' using Gnome Extension Manage
//

import Gio from 'gi://Gio';
import Meta from 'gi://Meta';
import { Extension } from 'resource:///org/gnome/shell/extensions/extension.js';
const GLib = imports.gi.GLib;


// busctl --user tree org.gnome.Shell 
// busctl --user call org.gnome.Shell /org/gnome/Shell/Extensions/GxsmWmExtension org.gnome.Shell.Extensions.GxsmWm GetGeoAction s "'Gxsm4'"
// 
// └─ /org/gnome/Shell/Extensions/GxsmWmExtension

// Set Window Geometry: SetGeoAction (wClass, title, x,y,w,h,op); use op=0
// busctl --user call org.gnome.Shell /org/gnome/Shell/Extensions/GxsmWmExtension org.gnome.Shell.Extensions.GxsmWm SetGeoAction ssiiiii -- "gxsm4" "Gxsm4" 100 100 500 600 0

// Query Window Geometry: GetGeoAction (wClass, title);
// busctl --user call org.gnome.Shell /org/gnome/Shell/Extensions/GxsmWmExtension org.gnome.Shell.Extensions.GxsmWm GetGeoAction ss "gxsm4" "Gxsm4"                
// => 's "[100, 100, 500, 600]"' or 's "WNA"' (Window not available)

// Query all Windows GetGeoAction ('*', '');
// busctl --user call org.gnome.Shell /org/gnome/Shell/Extensions/GxsmWmExtension org.gnome.Shell.Extensions.GxsmWm GetGeoAction ss "*" ""
// list: '{class=...},...'

// Define the XML interface for your custom D-Bus object
const GXSMWMXML = `
<node>
  <interface name="org.gnome.Shell.Extensions.GxsmWm">
    <method name="GetGeoAction">
      <arg type="s" direction="in" name="wClass"/>
      <arg type="s" direction="in" name="wTitle"/>
      <arg type="s" direction="out" name="geom"/>
    </method>
    <method name="SetGeoAction">
      <arg type="s" direction="in" name="wClass"/>
      <arg type="s" direction="in" name="wTitle"/>
      <arg type="i" direction="in" name="x"/>
      <arg type="i" direction="in" name="y"/>
      <arg type="i" direction="in" name="width"/>
      <arg type="i" direction="in" name="height"/>
      <arg type="i" direction="in" name="op"/>
      <arg type="s" direction="out" name="res"/>
    </method>
  </interface>
</node>`;

export default class GxsmWm extends Extension {
    enable() {
        this._dbusImpl = Gio.DBusExportedObject.wrapJSObject(GXSMWMXML, this);
        this._dbusImpl.export(Gio.DBus.session, '/org/gnome/Shell/Extensions/GxsmWmExtension');
    }

    disable() {
        if (this._dbusImpl) {
            this._dbusImpl.unexport();
            this._dbusImpl = null;
        }
    }


    getWindowGeometryByClassAndTitle(targetClass, targetTitle) {
	// Get all window actors across all workspaces
	let list='';
	let windowActors = global.get_window_actors();
	
	for (let actor of windowActors) {
            let win = actor.meta_window;
            if (!win) continue;

            // Retrieve properties to inspect
            let wmClass = win.get_wm_class(); // returns e.g., "firefox" or "org.gnome.Nautilus"
            let title = win.get_title();      // returns the current visible window title string

	    if (targetClass === '*'){
		let rect = win.get_frame_rect(); 
		let winfo = `{class=${wmClass} title=${title} geometry=[${rect.x}, ${rect.y}, ${rect.width}, ${rect.height}]},`;
		list = list + winfo;
		log (winfo);
		continue;
	    }
	    
	    const regex = new RegExp(targetTitle, "g"); 
	    
            if (wmClass === targetClass && title.match(regex)) {
		// Get frame geometry matching move_resize_frame requirements
		let rect = win.get_frame_rect(); 
		return `[${rect.x}, ${rect.y}, ${rect.width}, ${rect.height}]`;
            }
	}
	
	if (targetClass === '*')
	    return list;
	else
	    return 'WNA'; // Window not found
    }

    
    moveResizeByTitleAndClass(targetClass, targetTitle, x, y, width, height, op) {
	let windowActors = global.get_window_actors();
	
	for (let actor of windowActors) {
            let win = actor.meta_window;
            if (!win) continue;
            
            // Wayland native apps use app_id for class; XWayland uses wm_class
            let wmClass = win.get_wm_class();
            let title = win.get_title();   
	    const regex = new RegExp(targetTitle, "g"); 
	    
            if (wmClass === targetClass && title.match(regex)) {

		let wactor = win.get_compositor_private();
		if (wactor) {
		    // Force an immediate un-grab before positioning
		    global.display.ungrab_keyboard();
		    
		    let signalId = actor.connect('first-frame', () => {
			win.move_resize_frame(true, x, y, width, height);
			
			// Release the handler immediately
			actor.disconnect(signalId);
		    });
		} else {
		    // Fallback if the actor isn't fully drawn yet
		    win.move_resize_frame(true, x, y, width, height);
		}

		/*

		// 1. Force clear any lingering compositor operations
		global.display.ungrab_keyboard();
		global.display.ungrab_pointer();
		
		// 2. Perform the native movement (using true for user_op)
		win.move_resize_frame(true, x, y, width, height);
		
		// 3. Re-assert strict window focus focus downward to the client app
		win.activate(global.get_current_time());
		win.focus(global.get_current_time());

		// true = move/resize frame, false = move/resize client area only
		if (op)
		    win.move_resize_frame(true, x, y, width, height);
		else
		    win.move_resize_frame(false, x, y, width, height);
		*/
		
		return 'OK';
            }
	}
	return 'WNA'; // window not available
    }
    
    // This is the function you want to execute from the shell
    GetGeoAction(wClass, wTitle) {
        //log(`GXSM-WM SHELL EXT GetGeo: ${wClass} >${wTitle}<`);
	return this.getWindowGeometryByClassAndTitle(wClass, wTitle);
    }
    
    SetGeoAction(wClass, wTitle, x, y, width, height, op) {
        //log(`GXSM-WM SHELL EXT SetGeo: ${wClass} >${wTitle}< [${x} ${y} ${width} ${height}]`);
	return this.moveResizeByTitleAndClass(wClass, wTitle, x, y, width, height, op);
    }
}
