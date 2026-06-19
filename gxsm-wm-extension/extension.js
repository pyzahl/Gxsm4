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
import * as Config from 'resource:///org/gnome/shell/misc/config.js';


// Get the full version string (e.g., "46.1" or "47.alpha")
const versionString = Config.PACKAGE_VERSION; 
const [GSVmajor, GSVminor] = versionString.split('.').map(s => parseInt(s, 10));
    
const GLib = imports.gi.GLib;

const sleep = (ms) => new Promise((resolve) => setTimeout(resolve, ms));

// busctl --user tree org.gnome.Shell 
// busctl --user call org.gnome.Shell /org/gnome/Shell/Extensions/GxsmWmExtension org.gnome.Shell.Extensions.GxsmWm GetGeoAction s "'Gxsm4'"
// 
// └─ /org/gnome/Shell/Extensions/GxsmWmExtension

// Set Window Geometry: SetGeoAction (wClass, title, x,y,w,h,op); use op=0
// busctl --user call org.gnome.Shell /org/gnome/Shell/Extensions/GxsmWmExtension org.gnome.Shell.Extensions.GxsmWm SetGeoAction ssiiiii -- "gxsm4" "Gxsm4" 100 100 500 600 'FALSE'

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
      <arg type="i" direction="in" name="wWorkspace"/>
      <arg type="s" direction="out" name="geom"/>
    </method>
    <method name="SetGeoAction">
      <arg type="s" direction="in" name="wClass"/>
      <arg type="s" direction="in" name="wTitle"/>
      <arg type="i" direction="in" name="wWorkspace"/>
      <arg type="i" direction="in" name="x"/>
      <arg type="i" direction="in" name="y"/>
      <arg type="i" direction="in" name="width"/>
      <arg type="i" direction="in" name="height"/>
      <arg type="b" direction="in" name="uop"/>
      <arg type="s" direction="out" name="res"/>
    </method>
    <method name="SetOnTopAction">
      <arg type="s" direction="in" name="wClass"/>
      <arg type="s" direction="in" name="wTitle"/>
      <arg type="i" direction="in" name="wWorkspace"/>
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

    // Wayland native apps use app_id for class; XWayland uses wm_class

    match_window (win, targetClass, targetTitle, targetWorkspace) {
	let wmClass   = win.get_wm_class();
        let wmTitle   = win.get_title();   
	const regex_TargetClass = new RegExp (targetClass, "g"); 
	const regex_TargetTitle = new RegExp (targetTitle, "g"); 
	if (!wmClass || !wmTitle)
	    return false;
	
        if (wmClass.match (regex_TargetClass) && wmTitle.match (regex_TargetTitle)){
	    let workspace = win.get_workspace().index();
	    let activeWorkspace = global.workspace_manager.get_active_workspace_index();
	    //  match as requested              auto match on current WS                                   match all WS
	    if (workspace == targetWorkspace || (targetWorkspace == -1 && workspace == activeWorkspace) || targetWorkspace == -2)
		return true;
	}
	return false;
    }
    
    setOnTopByTitleAndClass (targetClass, targetTitle, targetWorkspace, uop) {
	let windowActors = global.get_window_actors();
	let ret = 'WNA';
	
	for (let actor of windowActors) {
            let win = actor.meta_window;
            if (!win) continue;

	    if (this.match_window (win, targetClass, targetTitle, targetWorkspace)) {
		if (uop)
		    win.make_above();
		else
		    win.unmake_above();
		ret = 'OK';
	    }
	}
	return ret;
    }

    getWindowGeometryByClassAndTitle(targetClass, targetTitle, targetWorkspace) {
	// Get all window actors across all workspaces
	let ret  = '';
	let list = '';
	let windowActors = global.get_window_actors();
	let gotmatch = false;
	
	for (let actor of windowActors) {
            let win = actor.meta_window;
            if (!win) continue;

            // Retrieve properties to inspect
            let wmClass   = win.get_wm_class(); // returns e.g., "firefox" or "org.gnome.Nautilus"
            let title     = win.get_title();    // returns the current visible window title string

	    if (targetClass === '*'){
		let rect = win.get_frame_rect();
		let activeWorkspace = global.workspace_manager.get_active_workspace_index();
		let workspace = win.get_workspace().index();

		//  match as requested              auto match on current WS                                   match all WS
		if (workspace == targetWorkspace || (targetWorkspace == -1 && workspace == activeWorkspace) || targetWorkspace == -2){
		    let winfo = `{class=${wmClass.padEnd(20)} title=${title.padEnd(25)} geometry=[${rect.x}, ${rect.y}, ${rect.width}, ${rect.height}] on Desktop ${workspace} active: ${activeWorkspace}},`;
		    list = list + winfo;
		    log (winfo);
		}
		continue;
	    }
	    
	    if (this.match_window (win, targetClass, targetTitle, targetWorkspace)) {
		// Get frame geometry matching move_resize_frame requirements
		let rect = win.get_frame_rect(); 
		let workspace = win.get_workspace().index();
		ret = ret + `[${rect.x}, ${rect.y}, ${rect.width}, ${rect.height}] class=${wmClass}; title=${title} on ${workspace}` + '\n';
		gotmatch = true;
		//return `[${rect.x}, ${rect.y}, ${rect.width}, ${rect.height}] on ${workspace}`;
            }
	}
	
	if (targetClass === '*')
	    return list;
	else{
	    if (gotmatch)
		return ret;
	    else
		return 'WNA'; // Window not found
	}
    }

    
    moveResizeByTitleAndClass(targetClass, targetTitle, targetWorkspace, x, y, width, height, uop) {
	let windowActors = global.get_window_actors();
	
	for (let actor of windowActors) {
            let win = actor.meta_window;
            if (!win) continue;

	    if (this.match_window (win, targetClass, targetTitle, targetWorkspace)) {
		if (GSVmajor >= 50)
		    this.Gnome50Layout(win, x, y, width, height, uop);
		else 
		    this.Gnome49Layout(win, x, y, width, height, uop);
		
		return 'OK';
            }
	}
	return 'WNA'; // window not available
    }

    Gnome50Layout(window, x, y, width, height, uop) {
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


    Gnome49Layout(window, x, y, width, height, uop) {
	if (!window) return;
	window.move_frame(false, x, y);
	//win.move_resize_frame(false, x, y, width, height);
	
	sleep(200);
	window.move_resize_frame(false, x, y, width, height);
	
	if (uop){
	    window.move_resize_frame(false, x, y, width, height);
	    //log(`win.move_resize forced.`);
	    //return 'OK*';
	}
	
	GLib.timeout_add(GLib.PRIORITY_DEFAULT_IDLE, 30, () => {
	    //GLib.idle_add(GLib.PRIORITY_DEFAULT_IDLE, () => {
	    window.move_resize_frame('false', x, y, width, height);
	    //log(`win.move_resize completed OK.`);
	    return GLib.SOURCE_REMOVE; // Always return remove to prevent looping
	});
    }
    
    // This is the function you want to execute from the shell
    GetGeoAction(wClass, wTitle, wWorkspace) {
        //log(`GXSM-WM SHELL EXT GetGeo: ${wClass} >${wTitle}<`);
	return this.getWindowGeometryByClassAndTitle(wClass, wTitle, wWorkspace);
    }
    
    SetGeoAction(wClass, wTitle, wWorkspace, x, y, width, height, uop) {
        //log(`GXSM-WM SHELL EXT SetGeo: ${wClass} >${wTitle}< [${x} ${y} ${width} ${height} ${uop}]`);
	return this.moveResizeByTitleAndClass(wClass, wTitle, wWorkspace, x, y, width, height, uop);
    }

    SetOnTopAction(wClass, wTitle, wWorkspace, uop) {
	return this.setOnTopByTitleAndClass(wClass, wTitle, wWorkspace, uop);
	metaWindow.set_above(uop);
    }
}
