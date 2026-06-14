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
import Clutter from 'gi://Clutter'
import GObject from 'gi://GObject'
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
      <arg type="b" direction="in" name="uop"/>
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

    
    moveResizeByTitleAndClass(targetClass, targetTitle, x, y, width, height, uop) {
	let windowActors = global.get_window_actors();
	
	for (let actor of windowActors) {
            let win = actor.meta_window;
            if (!win) continue;
            
            // Wayland native apps use app_id for class; XWayland uses wm_class
            let wmClass = win.get_wm_class();
            let title = win.get_title();   
	    const regex = new RegExp(targetTitle, "g"); 
	    
            if (wmClass === targetClass && title.match(regex)) {

		this.Gnome50Layout(win, x, y, width, height);

		/*
		win.move_resize_frame(false, x, y, width, height);

		if (uop){
		    win.move_resize_frame(false, x, y, width, height);
		    //log(`win.move_resize forced.`);
		    //return 'OK*';
		}

		GLib.timeout_add(GLib.PRIORITY_DEFAULT_IDLE, 30, () => {
		//GLib.idle_add(GLib.PRIORITY_DEFAULT_IDLE, () => {
		    win.move_resize_frame('false', x, y, width, height);
		    //log(`win.move_resize completed OK.`);
		    return GLib.SOURCE_REMOVE; // Always return remove to prevent looping
		    });
		*/
		return 'OK';

            }
	}
	return 'WNA'; // window not available
    }

    Gnome50Layout(window, x, y, width, height) {
	if (!window) return;

	window.move_resize_frame(false, x, y, width, height);

	GLib.idle_add(GLib.PRIORITY_DEFAULT_IDLE, () => {
            // Fetch the window actor layer safely
            const actor = window.get_compositor_private();
            
            // THE FIX: If actor is gone or explicitly destroyed, drop out safely
            if (!actor || actor.is_destroyed?.()) {
		return GLib.SOURCE_REMOVE; 
            }

            // Safe to execute structural commands now
            window.move_frame(false, x, y);

            if (global.stage?.update_pointer_focus) {
		global.stage.update_pointer_focus();
            }

            return GLib.SOURCE_REMOVE;
	});
    }

    
    // This is the function you want to execute from the shell
    GetGeoAction(wClass, wTitle) {
        //log(`GXSM-WM SHELL EXT GetGeo: ${wClass} >${wTitle}<`);
	return this.getWindowGeometryByClassAndTitle(wClass, wTitle);
    }
    
    SetGeoAction(wClass, wTitle, x, y, width, height, uop) {
        //log(`GXSM-WM SHELL EXT SetGeo: ${wClass} >${wTitle}< [${x} ${y} ${width} ${height} ${uop}]`);
	return this.moveResizeByTitleAndClass(wClass, wTitle, x, y, width, height, uop);
    }
}

/*
import GLib from 'gi://GLib';

let mutterLock = false;
let queuedLayout = null;

function forceMutterCompliance(window, x, y, w, h) {
    if (!window || window.is_destroyed()) return;

    // 1. If Mutter is already busy processing a layout change, just cache the newest target
    if (mutterLock) {
        queuedLayout = { x, y, w, h };
        return;
    }

    mutterLock = true;

    // 2. Push the first pass layout to Mutter's event queue
    window.move_resize_frame(false, x, y, w, h);

    // 3. Give Mutter a generous 16ms (1 full frame tick at 60Hz) to process the round-trip
    GLib.timeout_add(GLib.PRIORITY_DEFAULT_IDLE, 16, () => {
        if (!window || window.is_destroyed()) {
            mutterLock = false;
            return GLib.SOURCE_REMOVE;
        }

        // Force absolute placement alignment 
        window.move_frame(false, x, y);
        
        // Lift the lock
        mutterLock = false;

        // 4. If a new request came in while Mutter was processing, run it now
        if (queuedLayout) {
            let next = queuedLayout;
            queuedLayout = null;
            forceMutterCompliance(window, next.x, next.y, next.w, next.h);
        }

        return GLib.SOURCE_REMOVE;
    });
    }



import GLib from 'gi://GLib';

function bulletproofWaylandInputMove(window, x, y, width, height) {
    if (!window || window.is_destroyed()) return;

    // 1. Core Layout Change
    window.move_resize_frame(false, x, y, width, height);

    // 2. Yield to let the app draw, then manually force Mutter's seat to re-evaluate input focus
    GLib.timeout_add(GLib.PRIORITY_DEFAULT_IDLE, 25, () => {
        if (!window || window.is_destroyed()) return GLib.SOURCE_REMOVE;

        // Force secondary positioning alignment
        window.move_frame(false, x, y);

        // 3. THE FIX: Force Clutter / Mutter to re-calculate the pointer focus area.
        // This instantly snaps the invisible input mask to the current mouse coordinates.
        if (global.stage && typeof global.stage.update_pointer_focus === 'function') {
            global.stage.update_pointer_focus();
        }

        // 4. Force a surface commit notice back to the window actor
        let actor = window.get_compositor_private();
        if (actor && typeof actor.queue_redraw === 'function') {
            actor.queue_redraw();
        }

        return GLib.SOURCE_REMOVE;
    });
}



import GLib from 'gi://GLib';

function gnome50WaylandLayout(window, targetX, targetY, targetW, targetH) {
    if (!window || window.is_destroyed()) return;

    // 1. In GNOME 50, clearing states is lighter but must still happen first
    if (window.get_maximized()) {
        window.unmaximize(3); // Clear both axes
    }

    // 2. Dispatch layout request to the Wayland/XWayland surface
    window.move_resize_frame(false, targetX, targetY, targetW, targetH);

    // 3. Native Wayland tracking: wait for the frame buffer to commit 
    // to prevent the input-mask desync (dead buttons) on high refresh rate displays.
    GLib.idle_add(GLib.PRIORITY_DEFAULT_IDLE, () => {
        if (!window || window.is_destroyed()) return GLib.SOURCE_REMOVE;

        // Force pointer focus areas to snap to the newly rendered textures
        if (global.stage && typeof global.stage.update_pointer_focus === 'function') {
            global.stage.update_pointer_focus();
        }
        
        return GLib.SOURCE_REMOVE;
    });
}


    
*/
