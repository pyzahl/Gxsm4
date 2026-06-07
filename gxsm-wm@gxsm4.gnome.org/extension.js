import Gio from 'gi://Gio';
import Meta from 'gi://Meta';
import { Extension } from 'resource:///org/gnome/shell/extensions/extension.js';

// busctl --user tree org.gnome.Shell 
// busctl --user call org.gnome.Shell /org/gnome/Shell/Extensions/GxsmWmExtension org.gnome.Shell.Extensions.GxsmWm GetGeoAction s "'Gxsm4'"
// 
// └─ /org/gnome/Shell/Extensions/GxsmWmExtension

// busctl --user call org.gnome.Shell /org/gnome/Shell/Extensions/GxsmWmExtension org.gnome.Shell.Extensions.GxsmWm SetGeoAction ssiiii "gxsm4" "Gxsm4" 100 100 500 600
// busctl --user call org.gnome.Shell /org/gnome/Shell/Extensions/GxsmWmExtension org.gnome.Shell.Extensions.GxsmWm GetGeoAction ss "gxsm4" "Gxsm4"                
// => s "[100, 100, 500, 600]"


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
	let windowActors = global.get_window_actors();
	
	for (let actor of windowActors) {
            let win = actor.meta_window;
            if (!win) continue;

            // Retrieve properties to inspect
            let wmClass = win.get_wm_class(); // returns e.g., "firefox" or "org.gnome.Nautilus"
            let title = win.get_title();      // returns the current visible window title string

	    const regex = new RegExp(targetTitle, "g"); 
	    
            if (wmClass === targetClass && title.match(regex)) {
		// Get frame geometry matching move_resize_frame requirements
		let rect = win.get_frame_rect(); 
		return rect;
            }
	}
	return null; // Window not found
    }

    
    moveResizeByTitleAndClass(targetClass, targetTitle, x, y, width, height) {
	let windowActors = global.get_window_actors();
	
	for (let actor of windowActors) {
            let win = actor.meta_window;
            if (!win) continue;
            
            // Wayland native apps use app_id for class; XWayland uses wm_class
            let wmClass = win.get_wm_class();
            let title = win.get_title();
	    
	    const regex = new RegExp(targetTitle, "g"); 
	    
            if (wmClass === targetClass && title.match(regex)) {
		// true = move/resize frame, false = move/resize client area only
		win.move_resize_frame(true, x, y, width, height);
		return 'OK';
            }
	}
	return 'WNA'; // window not available
    }
    
    // This is the function you want to execute from the shell
    GetGeoAction(wClass, wTitle) {
        //log(`GXSM-WM SHELL EXT GetGeo: ${wClass} >${wTitle}<`);
	let geo = this.getWindowGeometryByClassAndTitle(wClass, wTitle);
	if (geo === null){
	    return 'WNA'; // window not available
	}
        //log(`GEO: [${geo.x}, ${geo.y}, ${geo.width}, ${geo.height}]`);
	return `[${geo.x}, ${geo.y}, ${geo.width}, ${geo.height}]`;

    }
    
    SetGeoAction(wClass, wTitle, x, y, width, height) {
        //log(`GXSM-WM SHELL EXT SetGeo: ${wClass} >${wTitle}< [${x} ${y} ${width} ${height}]`);
	return this.moveResizeByTitleAndClass(wClass, wTitle, x, y, width, height);
    }
}
