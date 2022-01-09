/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 8 c-style: "K&R" -*- */

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

// NOTE: pzahl@phenom:/usr/share/help/C/gnome-devel-demos/samples

#include <locale.h>
#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>

#include "gxsm_app.h"
#include "gxsm_window.h"

#include "glbvars.h"

#include "unit.h"
#include "pcs.h"

#include "xsmtypes.h"
#include "surface.h"
#include "plugin_ctrl.h"
#include "gxsm_resoucetable.h"

extern gboolean generate_preferences_gschema;
extern gboolean generate_gl_preferences_gschema;
void surf3d_write_schema (); // in vsurf3d.C


typedef struct { 
        const gchar *label;
        const gchar *tooltip;
        const gchar *toolbar_action;
        const gchar *icon_name;
        GActionEntry gaction;
} GXSM_ACTION_INFO;

#define GXSM_TOOLBAR_ACTION_ENTRY(L, ACTION, T, CB, TA, ICON) { N_(L), N_(T), TA, ICON, { ACTION, CB, NULL, NULL, NULL} }
#define GXSM_TOOLBAR_END { NULL, NULL, NULL, NULL, { NULL, NULL, NULL, NULL } }

static GXSM_ACTION_INFO main_actions[] = {
	GXSM_TOOLBAR_ACTION_ENTRY ("Start Scan", "scan-start", "Start scanning the surface",
                                   App::action_toolbar_callback, "Toolbar_Scan_Start",
                                   "system-run-symbolic"),
	GXSM_TOOLBAR_ACTION_ENTRY ("Start Movie", "scan-movie", "Repeat scanning the surface and store in time dimension",
                                   App::action_toolbar_callback, "Toolbar_Scan_Movie",
                                   "camera-video-symbolic"),
#if 0
	GXSM_TOOLBAR_ACTION_ENTRY ("Pause Scan", "scan-pause", "Pause a running scan",
                                   App::action_toolbar_callback, "Toolbar_Scan_Pause",
                                   "media-playback-pause-symbolic"),
#endif
	GXSM_TOOLBAR_ACTION_ENTRY ("Stop Scan", "scan-stop", "Stop a running scan",
                                   App::action_toolbar_callback, "Toolbar_Scan_Stop",
                                   "process-stop-symbolic"),
	GXSM_TOOLBAR_ACTION_ENTRY ("Open File", "file-open", "Open an existing scan", 
                                    App::file_open_callback, NULL,
                                   "document-open-symbolic"),
	GXSM_TOOLBAR_ACTION_ENTRY ("Save All", "file-save", "Save scans with automatically generated names",
                                   App::file_save_callback, "Toolbar_Scan_Save_All_Update",
                                   "document-save-symbolic"),
	GXSM_TOOLBAR_ACTION_ENTRY ("Exit Gxsm", "quit", "Exit the GXSM",
                                   App::file_quit_callback, NULL,
                                   "system-shutdown-symbolic"),
        GXSM_TOOLBAR_END
};
        
static GActionEntry app_gxsm_action_entries[] = {
        // { "rgb-mode", ViewControl::view_view_color_rgb_callback, NULL, "false", NULL },
        // { "set-marker-group", ViewControl::view_tool_marker_group_radio_callback, "s", "'red'", NULL },
        { "file-update", App::file_update_callback, NULL, NULL, NULL },
        { "file-open", App::file_open_callback, NULL, NULL, NULL },
        { "file-open-to-free-channel", App::file_open_in_new_window_callback, NULL, NULL, NULL },
        { "file-close", App::file_close_callback, NULL, NULL, NULL },
        { "file-open-mode", App::file_open_mode_callback, "s", "'append-time'", NULL },
        { "plugins-reload", App::tools_plugin_reload_callback, NULL, NULL, NULL },
        { "plugins-info", App::tools_plugin_info_callback, NULL, NULL, NULL },
        { "preferences", App::options_preferences_callback, NULL, NULL, NULL },
        { "save-geometry", App::save_geometry_callback, NULL, NULL, NULL },
        { "load-geometry", App::load_geometry_callback, NULL, NULL, NULL },
        { "about", App::help_about_callback, NULL, NULL, NULL },
        { "quit", App::file_quit_callback, NULL, NULL, NULL }
};





static void
add_accelerator (GApplication    *app,
                 const gchar *action_name_and_target,
                 const gchar *accel)
{
	const gchar *vaccels[] = {
		accel,
		NULL
	};
        //        PI_DEBUG_GP (DBG_L1, "add_accel for %s = %s\n", action_name_and_target, accel );
	gtk_application_set_accels_for_action (GTK_APPLICATION (app), action_name_and_target, vaccels);
}


// ========================================
// gxsm4_app_class  G-OBJECT / GTK CORE
// ========================================

/*
 * G_APPLICATION CODE FOR GXSM4 GOBJECT
 */

struct _Gxsm4app
{
  GtkApplication parent;
};

struct _Gxsm4appClass
{
  GtkApplicationClass parent_class;
};

G_DEFINE_TYPE(Gxsm4app, gxsm4_app, GTK_TYPE_APPLICATION);

static void
gxsm4_app_init (Gxsm4app *app)
{
        XSM_DEBUG(DBG_L2, "gxsm4_app_init =======================================================" );
}



static void
gxsm4_app_startup (GApplication *app)
{
        GtkBuilder *builder;
        XSM_DEBUG(DBG_L2, "gxsm4_app_startup ====================================================" );
        

        G_APPLICATION_CLASS (gxsm4_app_parent_class)->startup (app);

        XSM_DEBUG(DBG_L2, "gxsm4_app ** adding css styles" );

        /* add additional stylings */
        GtkCssProvider* provider = gtk_css_provider_new();
        gtk_css_provider_load_from_resource (GTK_CSS_PROVIDER(provider), "/" GXSM_RES_BASE_PATH "/gxsm4-styles.css");

        // https://developer.gnome.org/gtk3/stable/chap-css-overview.html
        /*
        gtk_css_provider_load_from_data(GTK_CSS_PROVIDER(provider),
                                        "#view-headerbar-tip-follow:checked{\n"
                                        "    color: #dd0000;\n"
                                        "}\n"
                                        "\n",
                                        -1, NULL);
        */
        g_object_unref(provider);
        // GXSM core application -- NOT the G APPLCIATION/WINDOW MANAGEMENT
        XSM_DEBUG(DBG_L2, "gxsm4 core startup" );
       
        XSM_DEBUG(DBG_L1, "START ** Setup dynamic preferences");
        gxsm_init_dynamic_res ();
        XSM_DEBUG(DBG_L1, "DONE ** Setup dynamic preferences");

        XSM_DEBUG(DBG_L1, "GXSM / PlugIn debug / logging level is: " << main_get_debug_level() << " / " << main_get_pi_debug_level() << " / " << logging_level);

        XSM_DEBUG(DBG_L2, "gapplication startup -- app menu installations" );
  
        //#define USE_COMPILED_IN_RESOURCES
#define USE_COMPILED_IN_RESOURCES
#ifdef USE_COMPILED_IN_RESOURCES
        PI_DEBUG_GP (DBG_L1, "USING COMPILED IN RESCOURCES -- gxsm4-menu.ui.\n");
        builder = gtk_builder_new_from_resource ("/" GXSM_RES_BASE_PATH "/gxsm4-menu.ui");
#else  // testing/development
        GError *error=NULL;
        PI_DEBUG_GP (DBG_L1, "USING BUILDER RESCOURCES from file -- gxsm4-menu.ui.\n");
        builder = gtk_builder_new ();
        gtk_builder_add_from_file (builder, "gxsm4-menu.ui", &error);
        if (error != NULL){
                // Report error to user, and free error
                PI_DEBUG_GP (DBG_L1, "gxsm4-menu.ui file not found: (%s) fallback to build in resource.\n", error->message);
                g_error_free (error);
                g_object_unref (builder);
                builder = gtk_builder_new_from_resource ("/" GXSM_RES_BASE_PATH "/gxsm4-menu.ui");
        }
#endif
        GObject *app_menu = gtk_builder_get_object (builder, "appmenu");
        if (!app_menu)
                PI_DEBUG_GP_ERROR (DBG_L1, "id appmenu can not be found in resource.\n");
        else
                ;//gtk_application_set_app_menu (GTK_APPLICATION (app), G_MENU_MODEL (app_menu)); // FIX-ME-GTK4

        GObject *gxsm_menubar = gtk_builder_get_object (builder, "gxsm-menubar");
        if (!gxsm_menubar)
                PI_DEBUG_GP_ERROR (DBG_L1, "id gxsm_menubar can not be found in resource.\n");

#define ENABLE_GXSM_MENUBAR
#ifdef ENABLE_GXSM_MENUBAR

        else
                gtk_application_set_menubar (GTK_APPLICATION (app), G_MENU_MODEL (gxsm_menubar));
#endif
        
        // g_object_unref (builder); // tryed to add a  g_object_ref (...menu) -- dose not help, keep.


        
     	//      add_accelerator (app, "app.new-window", "<Primary>N");
        //	add_accelerator (app, "app.quit", "<Primary>Q");
	add_accelerator (app, "win.view-activate", "F1");
	add_accelerator (app, "win.view-autodisp", "F2");
	add_accelerator (app, "win.view-maptoworld", "F4");
	add_accelerator (app, "win.view-clr-world", "<Ctrl>F4");
	add_accelerator (app, "win.object-mode::rectangle", "F5");
	add_accelerator (app, "win.object-mode::point", "F6");
	add_accelerator (app, "win.object-mode::line-show", "F7");
	add_accelerator (app, "win.remove-all-objects", "F12");
	add_accelerator (app, "win.event-remove", "F11"); // delete key is a problem to use

	add_accelerator (app, "win.side-pane", "F9");

	add_accelerator (app, "win.view-mode::direct", "<Ctrl>D");
	add_accelerator (app, "win.view-mode::quick", "<Ctrl>Q");
	add_accelerator (app, "win.fix-zoom::zoomfactor-1x", "Q");
	add_accelerator (app, "win.fix-zoom::zoomfactor-2x", "<Shift>W");
	add_accelerator (app, "win.fix-zoom::zoomfactor-1by2", "W");
	add_accelerator (app, "win.fix-zoom::zoomfactor-auto", "<Ctrl>A");

	add_accelerator (app, "win.gear-menu", "F10");

	add_accelerator (app, "win.action::sf1", "<Shift>F1");
	add_accelerator (app, "win.action::sf2", "<Shift>F2");
	add_accelerator (app, "win.action::sf3", "<Shift>F3");
	add_accelerator (app, "win.action::sf4", "<Shift>F4");
	add_accelerator (app, "win.action::sf5", "<Shift>F5");
	add_accelerator (app, "win.action::sf6", "<Shift>F6");
	add_accelerator (app, "win.action::sf7", "<Shift>F7");
	add_accelerator (app, "win.action::sf8", "<Shift>F8");
	add_accelerator (app, "win.action::sf9", "<Shift>F9");
	add_accelerator (app, "win.action::sf10", "<Shift>F10");
	add_accelerator (app, "win.action::sf11", "<Shift>F11");
	add_accelerator (app, "win.action::sf12", "<Shift>F12");
 
        XSM_DEBUG(DBG_L1, "GXSM: create core gxsm application ************************************ gapp = new App ()");

  	// Now create GxsmApplication (gapp) class:
	// this starts at App::App() and fires up the application's initialisiation

        gapp = new App (GXSM4_APP (app));
        g_object_set_data (G_OBJECT (app), "APP-MAIN", gapp);
        gapp -> set_gxsm_main_menu (gxsm_menubar);
        gapp -> set_gxsm_app_menu (app_menu);
        
        // create all later used POPUP menus and keep GObjects for later activations

        XSM_DEBUG(DBG_L1, "************ GXSM: loading menu rescourses ***************");
        
#define USE_COMPILED_IN_RESOURCES
#ifdef USE_COMPILED_IN_RESOURCES
        builder = gtk_builder_new_from_resource ("/" GXSM_RES_BASE_PATH "/gxsm4-popupmenus.ui");
#else  // testing/development
        error=NULL;
        builder = gtk_builder_new ();
        gtk_builder_add_from_file (builder, "gxsm4_popupmenus.ui", &error);
        if (error != NULL){
                // Report error to user, and free error
                PI_DEBUG_GP (DBG_L1, "gxsm4_appmenu.ui file not found: (%s) fallback to build in resource.\n", error->message);
                g_error_free (error);
                g_object_unref (builder);
                builder = gtk_builder_new_from_resource ("/" GXSM_RES_BASE_PATH "/gxsm4_popupmenus.ui");
        }
#endif

        XSM_DEBUG(DBG_L1, "************ GXSM: loading popup menus from rescources ***************");
   
        GObject *monitor_menu = gtk_builder_get_object (builder, "monitor-menu");
        if (!monitor_menu)
                PI_DEBUG_GP_ERROR (DBG_L1, "id view2d-menu not found in resource.\n");
        else
                gapp -> set_monitor_menu (monitor_menu);

        GObject *view2d_menu = gtk_builder_get_object (builder, "view2d-menu");
        if (!view2d_menu)
                PI_DEBUG_GP_ERROR (DBG_L1, "id view2d-menu not found in resource.\n");
        else
                gapp -> set_view2d_menu (view2d_menu);

        GObject *vobj_ctx_menu_1p = gtk_builder_get_object (builder, "vobj-ctx-menu-1p");
        if (!vobj_ctx_menu_1p)
                PI_DEBUG_GP_ERROR (DBG_L1, "id vobj-ctx-menu-1p  not found in resource.\n");
        else
                gapp -> set_vobj_ctx_menu_1p (vobj_ctx_menu_1p);

        GObject *vobj_ctx_menu_2p = gtk_builder_get_object (builder, "vobj-ctx-menu-2p");
        if (!vobj_ctx_menu_2p)
                PI_DEBUG_GP_ERROR (DBG_L1, "id vobj-ctx-menu-2p not found in resource.\n");
        else 
                gapp -> set_vobj_ctx_menu_2p (vobj_ctx_menu_2p);

        GObject *vobj_ctx_menu_np = gtk_builder_get_object (builder, "vobj-ctx-menu-np");
        if (!vobj_ctx_menu_np)
                PI_DEBUG_GP_ERROR (DBG_L1, "id vobj-ctx-menu-np not found in resource.\n");
        else 
                gapp -> set_vobj_ctx_menu_np (vobj_ctx_menu_np);

        GObject *vobj_ctx_menu_event = gtk_builder_get_object (builder, "vobj-ctx-menu-event");
        if (!vobj_ctx_menu_event)
                PI_DEBUG_GP_ERROR (DBG_L1, "id vobj-ctx-menu-event not found in resource.\n");
        else
                gapp -> set_vobj_ctx_menu_event (vobj_ctx_menu_event);

        GObject *view3d_menu = gtk_builder_get_object (builder, "view3d-menu");
        if (!view3d_menu)
                PI_DEBUG_GP_ERROR (DBG_L1, "id view3d-menu not found in resource.\n");
        else
                gapp -> set_view3d_menu (view3d_menu);

        GObject *profile_popup_menu = gtk_builder_get_object (builder, "profile-popup-menu");
        if (!profile_popup_menu)
                PI_DEBUG_GP_ERROR (DBG_L1, "id profile-popup-menu not found in resource.\n");
        else
                gapp -> set_profile_popup_menu (profile_popup_menu);

        XSM_DEBUG(DBG_L1, "************ GXSM: loading popup menus for hwi from rescources ***************");

        builder = gtk_builder_new_from_resource ("/" GXSM_RES_BASE_PATH "/gxsm4-hwi-menus.ui");

        GObject *hwi_mover_popup_menu = gtk_builder_get_object (builder, "hwi-mover-popup-menu");
        if (!hwi_mover_popup_menu)
                PI_DEBUG_GP_ERROR (DBG_L1, "id hwi-mover-popup-menu not found in resource.\n");
        else
                gapp -> set_hwi_mover_popup_menu (hwi_mover_popup_menu);

        GObject *hwi_control_popup_menu = gtk_builder_get_object (builder, "hwi-control-popup-menu");
        if (!hwi_control_popup_menu)
                PI_DEBUG_GP_ERROR (DBG_L1, "id hwi-control-popup_menu not found in resource.\n");
        else
                gapp -> set_hwi_control_popup_menu (hwi_control_popup_menu);

        XSM_DEBUG(DBG_L1, "************ GXSM: loading popup menus for plugins from rescources ***************");

        builder = gtk_builder_new_from_resource ("/" GXSM_RES_BASE_PATH "/gxsm4-plugin-pyremote-menu.ui");

        GObject *plugin_pyremote_file_menu = gtk_builder_get_object (builder, "plugin-pyremote-file-menu");
        if (!plugin_pyremote_file_menu)
                PI_DEBUG_GP_ERROR (DBG_L1, "id plugin-pyremote-file-menu not found in resource.\n");
        else
                main_get_gapp ()->set_plugin_pyremote_file_menu (plugin_pyremote_file_menu);

        XSM_DEBUG(DBG_L1, "************ GXSM: startup core completed ***************");
}

static void
gxsm4_app_activate (GApplication *app)
{
        Gxsm4appWindow *win;
        XSM_DEBUG(DBG_L2, "gxsm4_app_activate ==================================================" );

        if (gapp->gxsm_app_window_present ()){
                XSM_DEBUG(DBG_L2, "gxsm4_app_activate -- presenting existing Gxsm Main Window ===================" );
                win = gapp->get_app_window ();
        } else {
                win = gxsm4_app_window_new (GXSM4_APP (app));
        } 
        gtk_window_present (GTK_WINDOW (win));
}

static void
gxsm4_app_open (GApplication  *app,
                  GFile        **files,
                  gint           n_files,
                  const gchar   *hint)
{
  GList *windows;
  Gxsm4appWindow *win;

        XSM_DEBUG(DBG_L2, "gxsm4_app_open =======================================================" );

        windows = gtk_application_get_windows (GTK_APPLICATION (app));
        if (windows)
                win = GXSM4_APP_WINDOW (windows->data);
        else
                win = gxsm4_app_window_new (GXSM4_APP (app));

        if (load_files_as_movie)
                main_get_gapp ()->xsm->ActivateFreeChannel();

        for (gint i=0; i < n_files; ++i)
                if (gxsm4_app_window_open (win, files[i], load_files_as_movie) == false)
                        break;

        gtk_window_present (GTK_WINDOW (win));
}

static void
gxsm4_app_class_init (Gxsm4appClass *klass)
{
        XSM_DEBUG(DBG_L2, "gxsm4_app_class_init =================================================" );

        G_APPLICATION_CLASS (klass)->startup = gxsm4_app_startup;
        G_APPLICATION_CLASS (klass)->activate = gxsm4_app_activate;
        G_APPLICATION_CLASS (klass)->open = gxsm4_app_open;
}

Gxsm4app *
gxsm4_app_new (void)
{
        XSM_DEBUG(DBG_L2, "gxsm4_app_new ========================================================" );

        if (gxsm_new_instance) // allow new instance, disable checks
                return (Gxsm4app*) g_object_new (GXSM4_APP_TYPE,
                                                 "flags", G_APPLICATION_HANDLES_OPEN,
                                                 NULL);
        else
                return (Gxsm4app*) g_object_new (GXSM4_APP_TYPE,
                                                 "application-id", GXSM_RES_BASE_PATH_DOT,
                                                 "flags", G_APPLICATION_HANDLES_OPEN,
                                                 NULL);
}


// ========================================
// gxsm4_app_class  G-OBJECT / GTK CORE END
// ========================================





        
// ========================================
// Gxsm MAIN Application Class Startup Point
// ========================================

App::App(Gxsm4app *gxsm4_app):AppBase(gxsm4_app)
{
        XSM_DEBUG(DBG_L2, "App::App" );

        // gxsm base settings object
        gxsm_app_settings = g_settings_new (GXSM_RES_BASE_PATH_DOT);
        as_settings       = g_settings_new (GXSM_RES_BASE_PATH_DOT".gui.as");
        gxsm_app_windows_list  = NULL; // holds a list of GXSM windows classes

        app_window = NULL;
	appbar = NULL;
	appbar_ctx_id = 0;
        tool_button_save_all = NULL;
        auto_update_all = false;

	// still not created
        channelselector = NULL;
        monitorcontrol = NULL;
        spm_control = NULL;
        xsm = NULL;

	// Remote Control Lists
        RemoteEntryList  = NULL;  
        RemoteActionList = NULL;  
        RemoteConfigureList = NULL;
	remotecontrol    = NULL;

	// Plungin Interface, Plugin Events
	GxsmPlugins = NULL;
	PluginNotifyOnSPMRange = NULL;
	PluginNotifyOnStartScan = NULL;
	PluginNotifyOnStopScan = NULL;
	PluginNotifyOnCDFLoad = NULL;
	PluginNotifyOnCDFSave = NULL;
	PluginNotifyOnLoadFile = NULL;
	PluginNotifyOnSaveFile = NULL;
	PluginRemoteAction = NULL;

	PluginCallGetNCInfo = NULL;
  
	// Application Gxsm
        nodestroy = TRUE;

        // Splash widgets
	splash_progress_bar = NULL;
        splash = NULL;
        splash_darea = NULL;
        splash_img = NULL;

	for(int i=0; i<4; ++i){
		glb_ref_point_xylt_index[i]=0;
		glb_ref_point_xylt_world[i]=0.;
	}
}

App::~App(){
	XSM_DEBUG (DBG_L1,  "App::~App ** cleaning up, unloading plugins **" );

	// remove plugins: killflag = TRUE
        monitorcontrol->LogEvent ("GXSM shudown", "unloading plugins.", 3);
	reload_gxsm_plugins( TRUE );

	XSM_DEBUG (DBG_L1,  "App::~App ** unloading plugins done. **" );
	XSM_DEBUG (DBG_L1,  "App::~App ** Deleting Channelselector **" );

        monitorcontrol->LogEvent ("GXSM shudown", "deleting channelselector.", 3);
        delete channelselector;

	XSM_DEBUG (DBG_L1,  "App::~App ** Deleting xsm **" );
        monitorcontrol->LogEvent ("GXSM shudown", "deleting xsm object.", 3);

        ClearStatus();
        
        delete xsm;

        // ------ not good, don't,  automatic
        //        g_clear_object (&gxsm_app_settings);
        //        g_clear_object (&as_settings);

	XSM_DEBUG (DBG_L1,  "App::~App ** Deleting Monitor **" );

        monitorcontrol->LogEvent ("GXSM shudown", "closing logging monitor, last log of session.");
        delete monitorcontrol;

        XSM_DEBUG(DBG_L1, "App::~App: done." );
}


void App::MAINAppWindowInit(Gxsm4appWindow* win, const gchar *title, const gchar *sub_title){
	XSM_DEBUG (DBG_L2,  "App::MAINWindowInit" );

        g_message ("App::MAINAppWindowInit** <%s : %s> **", title, sub_title);
        
        if (win){
                g_message ("App::MAINAppWindowInit... GOT WINDOW");
                app_window = win;;
        } else {
                g_message ("App::MAINAppWindowInit... NEW WINDOW");
                app_window = gxsm4_app_window_new (GXSM4_APP (get_app ()));
        }

        
        //app_window = gxsm4_app_window_new ( get_app ());
        window = GTK_WINDOW (app_window);

        GtkIconTheme *icon_theme;
        icon_theme = gtk_icon_theme_get_for_display (gtk_widget_get_display (GTK_WIDGET (window)));
        gtk_icon_theme_add_resource_path (icon_theme, "/org/gnome/gxsm4/resources/icons");
        
        header_bar = gtk_header_bar_new ();
        gtk_widget_show (header_bar);
        gtk_header_bar_set_decoration_layout (GTK_HEADER_BAR (header_bar), "icon"); //  "icon,menu:close");
                                              
        g_action_map_add_action_entries (G_ACTION_MAP (gxsm4app),
                                         app_gxsm_action_entries, G_N_ELEMENTS (app_gxsm_action_entries),
                                         this);

        // create window PopUp menu  ---------------------------------------------------------------------
        XSM_DEBUG (DBG_L2,  "App::AppWindowInit main menu" );

        gxsm_menu = gtk_popover_menu_new_from_model (G_MENU_MODEL (get_gxsm_main_menu ()));

        XSM_DEBUG (DBG_L2,  "App GXSM main popup Header Buttons setup. " );

        // attach full view popup menu to tool button ----------------------------------------------------
        GtkWidget *header_menu_button = gtk_menu_button_new ();
        //        gtk_button_set_image (GTK_BUTTON (header_menu_button), gtk_image_new_from_icon_name ("emblem-system-symbolic", tmp_toolbar_icon_size));
        gtk_menu_button_set_popover (GTK_MENU_BUTTON (header_menu_button), gxsm_menu);
        gtk_header_bar_pack_end (GTK_HEADER_BAR (header_bar), header_menu_button);

        // attach display mode section from popup menu to tool button --------------------------------
        header_menu_button = gtk_menu_button_new ();
        gtk_menu_button_set_icon_name (GTK_MENU_BUTTON (header_menu_button), "emblem-system-symbolic");
        GMenuModel *section = find_extension_point_section (G_MENU_MODEL (get_gxsm_main_menu ()), "math-section");
        if (section) {
                gtk_menu_button_set_popover (GTK_MENU_BUTTON (header_menu_button), gtk_popover_menu_new_from_model (G_MENU_MODEL (section)));
                gtk_header_bar_pack_end (GTK_HEADER_BAR (header_bar), header_menu_button);
        }

        XSM_DEBUG (DBG_L2,  "App GXSM main popup Header Buttons setup. C TOOLBAR POPULATE " );

        // Create GXSM main action bar buttons and configure special toolbar_callback type actions
        // load, populate toolbar and setup all application actions

        for (GXSM_ACTION_INFO *gai=&main_actions[0]; gai->gaction.name; ++gai){
                XSM_DEBUG (DBG_L2, "GXSM MAIN TOOLBAR SETUP -- adding action = " << gai->gaction.name << " ** ceck:" << g_action_name_is_valid (gai->gaction.name));

                GSimpleAction *ti_action = g_simple_action_new (gai->gaction.name, NULL);
                // GSimpleAction *ti_action = g_simple_action_new_stateful (gai->gaction.name, NULL, g_variant_new_boolean (FALSE));
                // g_simple_action_set_enabled (ti_action, TRUE);
                g_signal_connect (ti_action, "activate", G_CALLBACK (gai->gaction.activate), gxsm4app);
                if (gai->toolbar_action){
                        g_object_set_data (G_OBJECT (ti_action), "toolbar_action", (gpointer) gai->toolbar_action);
                        XSM_DEBUG(DBG_L2, "........ adding toolbar_action = " << gai->toolbar_action);
                }
                g_action_map_add_action (G_ACTION_MAP (gxsm4app), G_ACTION (ti_action));

                if (gai->label){
                        XSM_DEBUG(DBG_L2, "........ adding toolbutton = " << gai->label);

                        /* B) Create a button for the action, with a stock image */

                        GtkWidget *ti_button = gtk_button_new ();

                        if (gai == &main_actions[0]) scan_button = ti_button; // Scan Button ref

                        gtk_button_set_icon_name (GTK_BUTTON (ti_button), gai->icon_name);
                        gtk_widget_set_tooltip_text (GTK_WIDGET (ti_button), gai->label);

                        if (strcmp (gai->label, "Save All") == 0)
                                tool_button_save_all = ti_button;

                        gchar *app_action = g_strconcat ("app.", gai->gaction.name, NULL); 
                        gtk_actionable_set_action_name (GTK_ACTIONABLE (ti_button), app_action);
                        g_object_set_data (G_OBJECT (ti_action), "toolbar_button", (gpointer)ti_button);
                        g_free (app_action);

                        gtk_header_bar_pack_start (GTK_HEADER_BAR (header_bar), GTK_WIDGET (ti_button));

                        //if (gai->toolbar_action)
                        //        g_object_set_data (G_OBJECT (app_window), gai->toolbar_action, ti_button);

                }
        }

        // Title bar
        SetTitle (title, sub_title);
        gtk_window_set_titlebar (GTK_WINDOW (window), header_bar);

        /* main window content is a grid to place main control elements */
        grid = gtk_grid_new ();
        gtk_window_set_child (GTK_WINDOW (window), grid);
        gtk_widget_show (grid); // FIX-ME GTK4 SHOWALL

        /* Make Statusbar and attach to grid at bottom now */
	appbar = gtk_statusbar_new ();
	appbar_ctx_id = gtk_statusbar_get_context_id (GTK_STATUSBAR (appbar), "GXSM MAIN STATUS");
        gtk_grid_attach (GTK_GRID (grid), appbar, 0, 10, 1, 1);
        gtk_widget_show (appbar); // FIX-ME GTK4 SHOWALL

       	gtk_widget_show (GTK_WIDGET (window)); // FIX-ME GTK4 SHOWALL
}

void App::build_gxsm (Gxsm4appWindow *win){
        //        const gchar *quit_accels[2] = { "<Ctrl>Q", NULL };

	xsmres.gxsm4_ready = false;
        
        XSM_DEBUG(DBG_L2, "App::build_gxsm_1" );

        pcs_set_current_gschema_group ("core");

	// manage main window geometry
        app_window = win;
        // setup core functionality header bar, grid with status bar

        g_message ("App::build_gxsm calling MAINAppWindowInit(%s, %s)", "*Gxsm4*", "*" GXSM_VERSION_NAME "*");
        MAINAppWindowInit (win, "Gxsm4", GXSM_VERSION_NAME);
        g_message ("App::build_gxsm BUILDING GXSM....");
        
        XSM_DEBUG(DBG_L2, "App::build_gxsm - starting up" );
        SetStatus(N_("Starting GXSM..."));
        
        /* Now read User Confiugartion */
        XSM_DEBUG(DBG_L2, "App::build_gxsm - reading config" );
        SetStatus(N_("Reading User Config"));

        XSM_DEBUG(DBG_L2, "App::build_gxsm - gnome_res_preferences_new (xsm_res_def)" );
	GnomeResPreferences *pref = gnome_res_preferences_new (xsm_res_def, GXSM_RES_PREFERENCES_PATH);
        XSM_DEBUG(DBG_L2, "App::build_gxsm - read_user_config" );

        if (generate_preferences_gschema) {
                gchar *gschema = gnome_res_write_gschema (pref);
                
                std::ofstream f;
                f.open (GXSM_RES_BASE_PATH_DOT ".preferences.gschema.xml", std::ios::out);
                f << gschema
                  << std::endl;
                f.close ();

                g_free (gschema);
                gnome_res_destroy (pref);

                exit (0);
        }
        if (generate_gl_preferences_gschema) {
                surf3d_write_schema ();
                exit (0);
        }
                
        gnome_res_read_user_config (pref);
        XSM_DEBUG(DBG_L2, "App::build_gxsm - destroy preferences object" );
	gnome_res_destroy (pref);

        set_window_geometry    ("main");
        
        XSM_DEBUG(DBG_L2, "App::build_gxsm_10" );
	
	if(xsmres.HardwareTypeCmd)
		strcpy(xsmres.HardwareType, xsmres.HardwareTypeCmd);
	if(xsmres.DSPDevCmd)
		strcpy(xsmres.DSPDev, xsmres.DSPDevCmd);

	XSM_DEBUG (DBG_L1,  "DSPDev :" << xsmres.DSPDev );
	XSM_DEBUG (DBG_L1,  "DSPType:" << xsmres.HardwareType );

        ClearStatus();

        XSM_DEBUG(DBG_L2, "App::build_gxsm - Monitor");
        pcs_set_current_gschema_group ("monitorwindow");

        monitorcontrol  = new MonitorControl ( get_app (), logging_level);
        
        /* Erzeuge und Initialise Xsm system */

        pcs_set_current_gschema_group ("corehwi");

        XSM_DEBUG(DBG_L2, "App::build_gxsm - xsm = new Surface" );
        SetStatus(N_("Generating Surface Object..."));
        xsm = new Surface (this); // This is the master backend, channel control
        g_object_set_data (G_OBJECT (gxsm4app), "Surface", xsm);
        xsm -> set_app (gxsm4app);
        
        XSM_DEBUG(DBG_L2, "App::build_gxsm_12" );


	/* Enable DND to accept file Drops */
        XSM_DEBUG(DBG_L2, "App::build_gxsm - DnD setup" );
        GType types[1] = { G_TYPE_FILE };
        GtkDropTarget *target = gtk_drop_target_new (G_TYPE_INVALID, GDK_ACTION_COPY);
        gtk_drop_target_set_gtypes (target, types, G_N_ELEMENTS (types));
        g_signal_connect (target, "drop", G_CALLBACK (AppBase::gapp_load_on_drop_files), xsm);
        gtk_widget_add_controller (GTK_WIDGET (app_window), GTK_EVENT_CONTROLLER (target));


        /* Attach more Elements... to main window */

        XSM_DEBUG(DBG_L2, "App::build_gxsm_15" );

	/* fill in Gxsm main control elements SPM + AS + UI */
	xsm->SetModeFlg (MODE_SETSTEPS);

        XSM_DEBUG(DBG_L2, "App::build_gxsm - attach control elements SPM+AS+UI" );

        pcs_set_current_gschema_group ("mainwindow");

	if(IS_SPALEED_CTRL)
		// SPALEED Controls
		spm_control  = create_spa_control ();
	else
		// SPM Controls
		spm_control  = create_spm_control ();

        gtk_grid_attach (GTK_GRID (grid), spm_control, 0, 1, 1, 1);
        gtk_widget_show (spm_control);

        XSM_DEBUG(DBG_L2, "App::build_gxsm_16" );

	// Auto Save Control
        XSM_DEBUG(DBG_L2, "App::build_gxsm - AS control" );
        as_control   = create_as_control ();
        gtk_grid_attach (GTK_GRID (grid), as_control, 0, 2, 1, 1);
        gtk_widget_show (as_control); // FIX-ME GTK4 SHOWALL

	// User Info Control
        XSM_DEBUG(DBG_L2, "App::build_gxsm - UI control" );
        ui_control   = create_ui_control ();
        gtk_grid_attach (GTK_GRID (grid), ui_control, 0, 3, 1, 1);
        gtk_widget_show (ui_control); // FIX-ME GTK4 SHOWALL

	gtk_widget_show (GTK_WIDGET (window)); // FIX-ME GTK4 SHOWALL

        /* create default control windows */
        
        XSM_DEBUG(DBG_L2, "App::build_gxsm - Channelselector");
        pcs_set_current_gschema_group ("channelselectorwindow");

        channelselector = new ChannelSelector (get_app ());

        g_idle_add (App::finish_system_startup_idle_callback, this);
}

void App::finish_system_startup (){
        
  	/* Load Gxsm Plugins: additional control windows, math/filters/etc. */
        XSM_DEBUG(DBG_L2, "App::build_gxsm - Scanning for PlugIns...");
        pcs_set_current_gschema_group ("plugins");
        // SetStatus(N_("Scanning for GXSM Plugins..."));
	if( !xsmres.disableplugins )
		reload_gxsm_plugins();

	/* call hook to update gxsm setting from hardware as desired */
        XSM_DEBUG(DBG_L2, "App::build_gxsm - call xsmhard->update_gxsm_configurations");

        if (xsm->hardware)
                xsm->hardware->update_gxsm_configurations ();
        else{
                XSM_DEBUG(DBG_L2, "App::build_gxsm - no call: xsm->hardware invalid.");
        }

        pcs_set_current_gschema_group ("post-build-error-path");


        // monitorcontrol->LogEvent ("Hardware Information", xsm->hardware->get_info ());

	xsmres.gxsm4_ready = true;
        SetStatus(N_("Ready."));

        
        /* Update all fields */
        XSM_DEBUG (DBG_L2, "App::build_gxsm - update all entries");
        spm_update_all (-xsm->data.display.ViewFlg);
        monitorcontrol->LogEvent ("GXSM", "startup");

        // update all window geometry again
        load_app_geometry ();
        
        XSM_DEBUG(DBG_L2, "App::build_gxsm - done.");
}

// Plugin handling
// ========================================

gint App::Gxsm_Plugin_Check (const gchar *category){
	if( category ){
		// HARD_ELEM = noHARD, spmHARD, spaHARD, ccdHARD, [bbHARD, dspHARD]
		// (+HARD) = (+HARD +HARD_ELEM)
		// (-HARD) = (-HARD -HARD_ELEM)
		// INST_ELEM = STM, AFM, SARLS, SPALEED, ELSLEED, CCD, ALL
		// (+INST) = (+INST +INST_ELEM)
		// (-INST) = (-INST -INST_ELEM)
		// Decision is:
		// LoadPlugin if ( (+HARD && -HARD) && (+INST && -INST) )
		// (+HARD) = (+HARD || (+HARD_ELEM is HardwareType))
		// (-HARD) = (-HARD && (-HARD_ELEM is not HardwareType))
		// (+INST) = (+INST || (+INST_ELEM is InstrumentType))
		// (-INST) = (-INST && (-INST_ELEM is not InstrumentType))
		
		// z.B. load if
		// category = "+noHARD -SPALEED" :  no Hardware and not SPALEED
		// category = "+SPALEED"  :  only SPALEED
		// category = "+spm -AFM" :  spm Hardware and not AFM
		
		gint  phard, mhard, pinst, minst;
		gchar *pht=g_strconcat("+", xsmres.HardwareType, "HARD", NULL);
		gchar *mht=g_strconcat("-", xsmres.HardwareType, "HARD", NULL);
		gchar *pit=g_strconcat("+", xsmres.InstrumentType, NULL);
		gchar *mit=g_strconcat("-", xsmres.InstrumentType, NULL);
		
		if(  strstr( category, pht ) || !strstr( category, "HARD") )
			phard = TRUE; else phard = FALSE;
		if( !strstr( category, mht ) || !strstr( category, "HARD") )
			mhard = TRUE; else mhard = FALSE;
		
		if(  strstr( category, pit ) || strstr( category, "+ALL") )
			pinst = TRUE; else pinst = FALSE;
		if( !strstr( category, mit ) || strstr( category, "+ALL") )
			minst = TRUE; else minst = FALSE;
		
		g_free(pit);
		g_free(mit);
		g_free(pht);
		g_free(mht);
		
		if((phard && mhard) && (pinst && minst))
			return TRUE;
		return FALSE;
	}
	return TRUE;
}


GxsmMenuExtension *App::gxsm_app_extend_menu (const gchar *extension_point, const gchar *menu_entry_text, const gchar *action)
{
	GMenuModel *model;
	GMenuModel *section;

        //g_message ("GxsmMenuExtension *App::gxsm_app_extend_menu");
        if (!action || !extension_point || !menu_entry_text ){
                //g_message ("GxsmMenuExtension *App::gxsm_app_extend_menu *** REQUEST ERROR: BAD STRING");
                XSM_DEBUG_GW (DBG_L2, "valid action: App::gxsm_app_extend_menu  *** ERORR: BAD REQUEST");
                return NULL;
        }
        //	g_return_val_if_fail (extension_point != NULL, NULL);

        XSM_DEBUG_GM (DBG_L2, "App::gxsm_app_extend_menu at >%s< Entry: >%s< Action => >%s<", extension_point, menu_entry_text, action);
        //g_message ("GxsmMenuExtension *App::gxsm_app_extend_menu ***  %s : %s -> %s", extension_point, menu_entry_text, action);
        
	/* First look in the window menu */
	section = find_extension_point_section (G_MENU_MODEL (get_gxsm_main_menu ()), extension_point);

        //        g_print ("App::gxsm_app_extend_menu - section\n");

        
	/* otherwise look in the app menu */
	if (section == NULL) {
                XSM_DEBUG_GM (DBG_L2, "App::gxsm_app_extend_menu SECTION=NULL...");
                //g_message ("App::gxsm_app_extend_menu - section search in app_menu DISABLED GTK4 PORT PENDING");
                // FIX-ME-GTK4 what do we really need here ???
                /*
                GMenu*
                        gtk_application_get_menu_by_id (
                                                        GtkApplication* application,
                                                        const char* id
                                                        )

                
                //                g_print ("App::gxsm_app_extend_menu - looking for app menu...\n");
                model = gtk_application_get_app_menu (GTK_APPLICATION (gxsm4app));
                
                if (model != NULL)
                        section = find_extension_point_section (model, extension_point);
                */
        }

	if (section == NULL){ // try last resort fallback to fixed plugin-section 
                XSM_DEBUG_GW (DBG_EVER, "App::gxsm_app_extend_menu - failed to locate extension point '%s', fallback to PlugIn menu...", extension_point);
                section = find_extension_point_section (G_MENU_MODEL (get_gxsm_main_menu ()), "plugins-section");
        }
        
	if (section != NULL){
                //                priv->menu_ext = gedit_app_activatable_extend_menu (gxsm4app, "tools-section");
                //                item = g_menu_item_new (_("S_ort..."), "win.sort");
                GxsmMenuExtension *menu_ext = gxsm_menu_extension_new (G_MENU (section));
                GMenuItem *item = g_menu_item_new (menu_entry_text, action);

                gxsm_menu_extension_append_menu_item (menu_ext, item);

                // gtk_widget_set_tooltip_text (GTK_WIDGET (item), action); // -- not supported ???
                // Print action string info at startup
#if 0
                gchar *info = g_strdup_printf ("GXSM-Menu-Action for %s/%s => %s", extension_point, menu_entry_text, action);
                g_message (info);
                g_free (info);
#endif
                g_object_unref (item);

                XSM_DEBUG_GM (DBG_L2, "App::gxsm_app_extend_menu extend completed for >%s<.", menu_entry_text);
                return menu_ext;
        } else {
                XSM_DEBUG_GM (DBG_L2, "App::gxsm_app_extend_menu extend failed for >%s<.", menu_entry_text);
                return NULL;
        }
}

int App::signal_emit_toolbar_action (const gchar *action, GSimpleAction *simple){
        GtkWidget *w = (GtkWidget*) g_object_get_data (G_OBJECT (app_window), action);
        if (w){
                if (simple)
                        g_object_set_data (G_OBJECT (w), "simple-action", (gpointer)simple);
                g_signal_emit_by_name (G_OBJECT (w), "clicked" );
                XSM_DEBUG(DBG_L2, "Toolbar Plugin for action \"" << action << "\" action requested by program." );
                return 0;
        }
        XSM_DEBUG(DBG_L2, "no Toolbar Plugin \"" << action << "\" Registerd!" );
        return -1;
}

void App::set_dsp_scan_in_progress (gboolean flg){
        static gboolean last = false;

        if (flg != last){ // be smart
#if 0 // GTKQQQ                
                GdkRGBA red = { 1.,0.,0., 0.5 };
                GdkRGBA *color = flg ? &red : NULL;
                gtk_widget_override_background_color (GTK_WIDGET (scan_button),
                                                      GTK_STATE_FLAG_NORMAL,
                                                      color);
                gtk_widget_override_background_color (GTK_WIDGET (scan_button),
                                                      GTK_STATE_FLAG_INSENSITIVE,
                                                      color);
                gtk_widget_override_background_color (GTK_WIDGET (scan_button),
                                                      GTK_STATE_FLAG_ACTIVE,
                                                      color);
#endif
                last = flg;
        }
}

void App::ConnectPluginToSPMRangeEvent( void (*cbfkt)(gpointer) ){
	PluginNotifyOnSPMRange = g_list_prepend(PluginNotifyOnSPMRange, (gpointer) cbfkt);
}
void App::ConnectPluginToStartScanEvent( void (*cbfkt)(gpointer) ){
	PluginNotifyOnStartScan = g_list_prepend(PluginNotifyOnStartScan, (gpointer) cbfkt);
}
void App::ConnectPluginToStopScanEvent( void (*cbfkt)(gpointer) ){
	PluginNotifyOnStopScan = g_list_prepend(PluginNotifyOnStopScan, (gpointer) cbfkt);
}
void App::ConnectPluginToCDFLoadEvent( void (*cbfkt)(gpointer) ){
	PluginNotifyOnCDFLoad = g_list_prepend(PluginNotifyOnCDFLoad, (gpointer) cbfkt);
}
void App::ConnectPluginToCDFSaveEvent( void (*cbfkt)(gpointer) ){
	PluginNotifyOnCDFSave = g_list_prepend(PluginNotifyOnCDFSave, (gpointer) cbfkt);
}
void App::ConnectPluginToLoadFileEvent( void (*cbfkt)(gpointer) ){
	PluginNotifyOnLoadFile = g_list_prepend(PluginNotifyOnLoadFile, (gpointer) cbfkt);
}
void App::ConnectPluginToSaveFileEvent( void (*cbfkt)(gpointer) ){
	PluginNotifyOnSaveFile = g_list_prepend(PluginNotifyOnSaveFile, (gpointer) cbfkt);
}

void App::ConnectPluginToGetNCInfoEvent( void (*cbfkt)(gchar *filename ) ){
	PluginCallGetNCInfo = cbfkt;
}

void App::ConnectPluginToRemoteAction( void (*cbfkt)(gpointer) ){
	PluginRemoteAction = g_list_prepend(PluginRemoteAction, (gpointer) cbfkt);
}

void App::reload_gxsm_plugins( gint killflag ){
	gint (*gxsm_plugin_check)(const gchar *) = Gxsm_Plugin_Check;
	GList *PluginDirs = NULL;
	XSM_DEBUG(DBG_L2, "Remove/Reload Gxsm plugins" );

	
	if( PluginNotifyOnSPMRange ){
		g_list_free( PluginNotifyOnSPMRange);
		PluginNotifyOnSPMRange = NULL;
	}
	if( PluginNotifyOnStartScan ){
		g_list_free( PluginNotifyOnStartScan );
		PluginNotifyOnStartScan = NULL;
	}
	if( PluginNotifyOnStopScan ){
		g_list_free( PluginNotifyOnStopScan );
		PluginNotifyOnStopScan = NULL;
	}
	if( PluginNotifyOnCDFLoad ){
		g_list_free( PluginNotifyOnCDFLoad );
		PluginNotifyOnCDFLoad = NULL;
	}
	if( PluginNotifyOnCDFSave ){
		g_list_free( PluginNotifyOnCDFSave );
		PluginNotifyOnCDFSave = NULL;
	}
	if( PluginNotifyOnLoadFile ){
		g_list_free( PluginNotifyOnLoadFile );
		PluginNotifyOnLoadFile = NULL;
	}
	if( PluginNotifyOnSaveFile ){
		g_list_free( PluginNotifyOnSaveFile );
		PluginNotifyOnSaveFile = NULL;
	}

	PluginCallGetNCInfo = NULL;
        
	if( !GxsmPlugins && killflag ){
		XSM_DEBUG(DBG_L2, "no GXSM plugins found, done." );
                main_get_gapp ()->GxsmSplash (2.0, "no GXSM PlugIns found.", "Finishing."); // this will auto remove splash after timout!
		return;
	}
	
	if( GxsmPlugins ){
		SetStatus(N_("Removing Plugins..."));
		delete GxsmPlugins;
		GxsmPlugins = NULL;
		if( killflag ){ // cleanup only ?
			// (re)load HwI
			xsm->reload_hardware_interface (killflag ? NULL:this);
			return;
		}
		
		XSM_DEBUG(DBG_L2, "Plugins removed" );
		SetStatus(N_("Plugins removed"));

	}

	// (re)load HwI
	xsm->reload_hardware_interface (killflag ? NULL:this);

        GxsmSplash(0.01, "Looking for GXSM PlugIns", " ... ");
	
	// Make plugin search dir list
	PluginDirs = g_list_prepend
		(PluginDirs, g_strconcat(PACKAGE_PLUGIN_DIR, "/common", NULL));
	PluginDirs = g_list_prepend
		(PluginDirs, g_strconcat(PACKAGE_PLUGIN_DIR, "/math", NULL));
	PluginDirs = g_list_prepend
		(PluginDirs, g_strconcat(PACKAGE_PLUGIN_DIR, "/probe", NULL));
	PluginDirs = g_list_prepend
		(PluginDirs, g_strconcat(PACKAGE_PLUGIN_DIR, "/control", NULL));
	PluginDirs = g_list_prepend
		(PluginDirs, g_strconcat(PACKAGE_PLUGIN_DIR, "/scan", NULL));

#ifdef GXSM_ENABLE_SCAN_USER_PLUGIN_PATH
	PluginDirs = g_list_prepend
		(PluginDirs, g_strconcat(g_get_home_dir(), "/.gxsm/plugins", NULL));
	PluginDirs = g_list_prepend
		(PluginDirs, g_strdup(xsmres.UserPluginPath));
#endif
	

	GxsmPlugins = new gxsm_plugins(this, PluginDirs, gxsm_plugin_check);
	
	// and remove list
	GList *node = PluginDirs;
	while(node){
		g_free(node->data);
		node = node->next;
	}
	g_list_free(PluginDirs);
	
	gchar *txt = g_strdup_printf
		(N_("%d Plugins loaded!"), GxsmPlugins->how_many());

        main_get_gapp ()->GxsmSplash (2.0, txt, "Init and hookup completed."); // this will auto remove splash after timout!
        
        SetStatus(txt);
	//  message((const char*)txt);
	g_free(txt);
}

void App::CallPlugin( void (*cbfkt)( gpointer ), gpointer data){
	(*cbfkt) ( data ); 
}

void App::SignalEventToPlugins( GList *PluginNotifyList, gpointer data ){
	g_list_foreach( PluginNotifyList, (GFunc) CallPlugin, data);    
}


// Gxsm Splash
// ========================================

gint App::RemoveGxsmSplash(GtkWidget *widget, gpointer data){
        // App *a = (App *)data;

        XSM_DEBUG_GP (DBG_L1, "App::RemoveGxsmSplash (hiding it)\n");

        //        gtk_widget_destroy (widget); // splash window
        //gtk_widget_hide (widget); // splash window

        //      a->splash = NULL;
        //	a->splash_progress_bar = NULL;
        //      a->splash_darea = NULL;

        splash_draw_function (NULL, NULL, 0,0, NULL); // clean up

        gtk_popover_popdown (GTK_POPOVER (widget));
        
	return FALSE;
}

void App::splash_draw_function (GtkWidget *area, cairo_t *cr,
                                int width, int height,
                                gpointer data){
        // App *a = (App *)data;

        static cairo_item_text *text1 = NULL;
        static cairo_item_text *text2 = NULL;
        static cairo_item_text *text3 = NULL;
        static cairo_item_text *text4 = NULL;


        if (area == NULL){ // cleanup self
                if (text1) { delete text1; text1=NULL; }
                if (text2) { delete text2; text2=NULL; }
                if (text3) { delete text3; text3=NULL; }
                if (text4) { delete text4; text4=NULL; }
                return true;
        }
        
        XSM_DEBUG_GP (DBG_L1, "App::splash_draw_callback: %s, %s \n",
                 (const gchar*) g_object_get_data( G_OBJECT (area), "splash_progress_text"),
                 (const gchar*) g_object_get_data( G_OBJECT (area), "splash_info_text"));
        
        gint w = GPOINTER_TO_INT (g_object_get_data( G_OBJECT (area), "splash_w"));
        gint h = GPOINTER_TO_INT (g_object_get_data( G_OBJECT (area), "splash_h"));

        if (text1 == NULL){
                text1 = new  cairo_item_text (w/2., 30., "GXSM-" VERSION " 4 GTK4");
                text1->set_font_face_size ("Ununtu", 24);
                text1->set_stroke_rgba (0.9, 0.8, 0.8, 1.);

                text2 = new  cairo_item_text (w/2., 30.+18.+5., "\"" GXSM_VERSION_NAME "\"");
                text2->set_font_face_size ("Ununtu", 18.);
                text2->set_stroke_rgba (1.0, 0.8, 0.8, 1.);

                text3 = new  cairo_item_text (w/2., h-20., (const gchar*) g_object_get_data( G_OBJECT (area), "splash_message"));
                text3->set_stroke_rgba (0.6, 1.0, 0.1, 1.);
                text3->set_pango_font ((const gchar*) g_object_get_data( G_OBJECT (area), "splash_message_font"));

                if (g_object_get_data( G_OBJECT (area), "splash_info_text")){
                        text4 = new  cairo_item_text (w/2., h+10., (const gchar*) g_object_get_data( G_OBJECT (area), "splash_info_text"));
                        text4->set_font_face_size ("Ununtu", 12.);
                        text4->set_stroke_rgba (0.6, 1.0, 1.0, 1.);
                }
        }
        
        if (text4){
                if (g_object_get_data( G_OBJECT (area), "splash_info_text") == NULL)
                        text4->hide ();
                else{
                        text4->set_text ((const gchar*) g_object_get_data( G_OBJECT (area), "splash_info_text"));
                        text4->show ();
                }
        }

        /* Set color for background */
        cairo_set_source_rgba(cr, 0., 0., 0., 1.);
        /* fill in the background color*/
        cairo_paint(cr);

        /* draw logo pixbuf */
        gdk_cairo_set_source_pixbuf (cr, GDK_PIXBUF (g_object_get_data( G_OBJECT (area), "splash_gdk_pixbuf")), 0, 0);
        cairo_paint (cr);

        /* set color for rectangle */
        cairo_set_source_rgb (cr, 0.8, 0.0, 0.0);
        /* set the line width */
        cairo_set_line_width (cr, 2);
        /* draw the rectangle's path beginning at 3,3 */
        cairo_rectangle (cr, 1, 1, w-2, h+20-2);
        /* stroke the rectangle's path with the chosen color so it's actually visible */
        cairo_stroke(cr);

        /* Draw Text Lines */

        text1->draw (cr);
        text2->draw (cr);
        text3->draw (cr);
        if (text4) text4->draw (cr);
}

void App::GxsmSplash(gdouble progress, const gchar *info, const gchar* text){
        GVariant *splash_timeout = g_settings_get_value (gxsm_app_settings, "splash-timeout");

	if (splash_progress_bar && splash && progress > -0.2){
                XSM_DEBUG_GM (DBG_L1, "App::GxsmSplash Update: %g, %s, %s", progress, info, text);
		if (progress >= 0. && progress <= 1.0)
			gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (splash_progress_bar), progress);
		else
			gtk_progress_bar_pulse (GTK_PROGRESS_BAR (splash_progress_bar));
		if (text){
                        g_object_set_data( G_OBJECT (splash_darea), "splash_progress_text", (gpointer) text);
			gtk_progress_bar_set_text (GTK_PROGRESS_BAR (splash_progress_bar), text);
		}
		if (info && splash_darea){
                        g_object_set_data( G_OBJECT (splash_darea), "splash_info_text", (gpointer) info);
                }
                gtk_widget_queue_draw (splash_darea);

                if (progress > 1.1){
                        XSM_DEBUG_GM (DBG_L1, "App::GxsmSplash Update [%g] -- schedule remove in %d ms", progress, (guint)g_variant_get_double (splash_timeout));
                        g_timeout_add ((guint)g_variant_get_double (splash_timeout), 
                                       (GSourceFunc) App::RemoveGxsmSplash, 
                                       splash);
                }
                return;
	}
        
        XSM_DEBUG_GP (DBG_L1, "App::GxsmSplash Build: %s, %s \n", info, text);

        // const gint ImgW = atoi(xxsmlogo_xpm[0]);
        // const gint ImgH = atoi(xxsmlogo_xpm[0]+4);
        const gint ImgW = 320;
        const gint ImgH = 320;

        GVariant *splash_display = g_settings_get_value (gxsm_app_settings, "splash");
        GVariant *splash_message = g_settings_get_value (gxsm_app_settings, "splash-message");
        GVariant *splash_message_font = g_settings_get_value (gxsm_app_settings, "splash-message-font");

        if (g_variant_get_boolean (splash_display)){
                splash = gtk_popover_new ();
                GtkWidget *vb = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
		gtk_widget_show (vb);
                gtk_popover_set_child (GTK_POPOVER (splash), vb);

                splash_darea = gtk_drawing_area_new ();
                gtk_drawing_area_set_content_width  (GTK_DRAWING_AREA (splash_darea), ImgW);
                gtk_drawing_area_set_content_height (GTK_DRAWING_AREA (splash_darea), ImgH);
                
                if (!splash_img){ // keep for ever!
                        splash_img = gdk_pixbuf_new_from_file (PACKAGE_ICON_DIR "/gxsm4-icon.svg", NULL);
                }
                
                g_object_set_data( G_OBJECT (splash_darea), "splash_w", GINT_TO_POINTER (ImgW));
                g_object_set_data( G_OBJECT (splash_darea), "splash_h", GINT_TO_POINTER (ImgH));
                g_object_set_data( G_OBJECT (splash_darea), "splash_gdk_pixbuf", (gpointer) splash_img);
                g_object_set_data( G_OBJECT (splash_darea), "splash_info_text", (gpointer) info);

                g_object_set_data( G_OBJECT (splash_darea), "splash_message", (gpointer) g_variant_get_string (splash_message, NULL));
                g_object_set_data( G_OBJECT (splash_darea), "splash_message_font", (gpointer) g_variant_get_string (splash_message_font, NULL));

                gtk_drawing_area_set_draw_func (GTK_DRAWING_AREA (splash_darea),
                                                G_CALLBACK (App::splash_draw_function),
                                                this, NULL);
                
                gtk_box_append (GTK_BOX (vb), splash_darea);
                gtk_widget_show (splash_darea);

		splash_progress_bar = gtk_progress_bar_new ();
                gtk_progress_bar_set_show_text (GTK_PROGRESS_BAR (splash_progress_bar), true);
		gtk_progress_bar_set_ellipsize (GTK_PROGRESS_BAR (splash_progress_bar), PANGO_ELLIPSIZE_START);
		gtk_widget_show (splash_progress_bar);
		gtk_box_append (GTK_BOX (vb), splash_progress_bar);

                gtk_widget_set_parent (splash, GTK_WIDGET(app_window));
                gtk_popover_set_has_arrow (GTK_POPOVER (splash), FALSE);
                // FIX-ME GTK4 
                gtk_popover_set_pointing_to (GTK_POPOVER (splash), &(GdkRectangle){ 1000-ImgW/2, 500-ImgH/2, 1, 1});

                gtk_popover_popup (GTK_POPOVER (splash));
        }
}

