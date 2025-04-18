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


#ifndef GXSM_APP_H
#define GXSM_APP_H

#include <config.h>

#include <netcdf.hh>
//#include <netcdf>
//using namespace netCDF;

#include "gxsm_menu-extension.h"
#include "gapp_service.h"
#include "pcs.h"
#include "monitor.h"
#include "app_profile.h"
#include "app_monitorcontrol.h"
#include "app_channelselector.h"
#include "lineprofile.h"

#include "xsmmath.h"
#include "xsm_limits.h"

#include "app_vobj.h"
#include "plugin_ctrl.h"

#include "gxsm_app.h"

class Surface;

/*
 * G_APPLICATION CODE FOR GXSM4 GOBJECT
 */

#define GXSM4_APP_TYPE (gxsm4_app_get_type ())
#define GXSM4_APP(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GXSM4_APP_TYPE, Gxsm4app))

typedef struct _Gxsm4appWindow         Gxsm4appWindow;

typedef struct _Gxsm4app       Gxsm4app;
typedef struct _Gxsm4appClass  Gxsm4appClass;


GType         gxsm4_app_get_type    (void);
Gxsm4app     *gxsm4_app_new         (void);

/* ***************** */


/* Client Windows... */
class RemoteControl;
class gxsm_plugins;
class AppBase;

/* App. Main Window */

class App : public GnomeAppService, AppBase {
public:
        App (Gxsm4app *g_app);
        virtual ~App ();

        void MAINAppWindowInit(Gxsm4appWindow* win, const gchar *title=NULL, const gchar *sub_title=NULL);
        void build_gxsm (Gxsm4appWindow *win);

        static gboolean finish_system_startup_idle_callback (App *self) { return self-> finish_system_startup (); }; // ret=FALSE => finished
        gboolean finish_system_startup ();
        
        /* Menu - callbacks */
        static void file_open_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data);
        static void file_open_in_new_window_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data);
        static void file_open_mode_callback (GSimpleAction *action, GVariant *parameter, gpointer user_data);
        static void file_browse_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data);
        static void file_set_datapath_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data);
        static void file_set_probepath_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data);
        static void file_save_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data);
        static void file_update_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data);
        static void file_save_as_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data);
        static void file_import_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data);
        static void file_export_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data);
        static void file_print_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data);
        static void file_close_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data);
        static void file_quit_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data);

        static void action_toolbar_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data);

        static void view_autozoom_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data);
        static void view_tolerant_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data);
        static void view_zoom_in_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data);
        static void view_zoom_out_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data);

        //        static void math_onearg_nodest_callback (GtkWidget *widget, gboolean (*MOp)(MATHOPPARAMSNODEST));
        //        static void math_onearg_callback (GtkWidget *widget, gboolean (*MOp)(MATHOPPARAMS));
        //        static void math_onearg_for_all_vt_callback (GtkWidget *widget, gboolean (*MOp)(MATHOPPARAMS));
        //        static void math_twoarg_callback (GtkWidget *widget, gboolean (*MOp)(MATH2OPPARAMS));
        //        static void math_twoarg_no_same_size_check_callback (GtkWidget *widget, gboolean (*MOp)(MATH2OPPARAMS));
  
        static void math_onearg_nodest_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data);
        static void math_onearg_dest_only_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data);
        static void math_onearg_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data);
        static void math_onearg_for_all_vt_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data);
        static void math_twoarg_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data);
        static void math_twoarg_no_same_size_check_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data);


        static void tools_monitor_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data);
        static void tools_chanselwin_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data);
        static void tools_plugin_reload_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data);
        static void tools_plugin_info_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data);

        static void options_preferences_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data);
        static void save_geometry_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data);
        static void load_geometry_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data);
        static void auto_scanview_geometry_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data);
        static void help_about_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data);

        /* Window Delete Events */ 
        static gint close_scan_event_cb (GtkWidget *window);
 
        /* Main - Window - Callbacks */
        static void browse_callback(gchar *selection, App* ap);

        void auto_save_scans (); // auto savescan(s) in progress or completed.
        void auto_update_scans (); // auto update scan(s) in progress or completed.
        void set_toolbar_autosave_button (gboolean update_mode=false);
        
        
        /* Emit Toolbar Actions */
        int signal_emit_toolbar_action (const gchar *action, GSimpleAction *simple=NULL);

        void set_dsp_scan_in_progress (gboolean flg);
        
        /* Status Handling */

        void SetStatus(const gchar *mld, const gchar *val=NULL){
                gtk_statusbar_remove_all (GTK_STATUSBAR (appbar), appbar_ctx_id);
                if(val){
                        gchar *longmld = g_strdup_printf("%s: %s",mld, val);
                        gtk_statusbar_push(GTK_STATUSBAR (appbar), appbar_ctx_id, longmld);
                        g_free(longmld);
                }
                else
                        gtk_statusbar_push(GTK_STATUSBAR (appbar), appbar_ctx_id, mld);
                check_events_self();
        };
        void ClearStatus(){
                gtk_statusbar_remove_all (GTK_STATUSBAR (appbar), appbar_ctx_id);
        };

        void SetProgress(gfloat p){
                if(p>=0. && p<=1.)
                        ; //GTK3QQQ: gnome_appbar_set_progress_percentage( GNOME_APPBAR (appbar), p);
                // there is no tool bar progress bar any more
        };

        void add_appwindow_to_list (AppBase *w) {
                gxsm_app_windows_list = g_slist_prepend (gxsm_app_windows_list, w);
        };

        void remove_appwindow_from_list (AppBase *w) {
                gxsm_app_windows_list = g_slist_remove (gxsm_app_windows_list, w);
        };

        static void call_save_geometry (AppBase* a, gpointer data){ a->SaveGeometry (); };
        static void call_load_geometry (AppBase* a, gpointer data){ a->LoadGeometry (); };
        static void call_auto_scanview_geometry (AppBase* a, gpointer data){
                a->LoadGeometryWRefAutoPlace ("view-scan2d");
                a->LoadGeometryWRefAutoPlace ("view-profile-redline","view-scan2d-1");
        };
        void save_app_geometry () { g_slist_foreach (gxsm_app_windows_list, (GFunc) App::call_save_geometry, NULL); };
        void load_app_geometry () { g_slist_foreach (gxsm_app_windows_list, (GFunc) App::call_load_geometry, NULL); };
        void auto_scanview_geometry () {
                save_app_geometry ();
                g_slist_foreach (gxsm_app_windows_list, (GFunc) App::call_auto_scanview_geometry, NULL);
        };

        /* Init Stuff */
        void gxsm_new_user_config (RES_ENTRY *res_def);
        static gint GxsmSplashRemove (gpointer data);
        static void splash_draw_function (GtkWidget *area, cairo_t *cr, int width, int height, gpointer data);
        void GxsmSplash(gdouble progress=0., const gchar *text=NULL, const gchar *info=NULL);

	GtkWidget *splash_progress_bar;
        GtkWidget *splash;
        GtkWidget *splash_darea;
        GdkPixbuf *splash_img;

        
        /* Application Windows: Creation Funktion and Widget-Pointer */
        GtkWidget* create_spm_control (); // SPM Control Elements, added to main_control
        GtkWidget* create_spa_control ();  // Auto Save Control Elements, added to main_control
        GtkWidget* create_ui_control ();  // User Info Control Elements, added to main_control
        GtkWidget* create_as_control ();  // Auto Save Control Elements, added to main_control
        GtkWidget* spm_control;
        GtkWidget* spa_control;
        GtkWidget* ui_control;
        GtkWidget* as_control;

        ProfileControl* new_profile_control(const gchar *filename, const gchar *id){
                return new ProfileControl (gxsm4app, filename, id);
        };
        
        void spa_update();
        void spa_setdata();

        void ui_update();
        void ui_setcomment();

        void as_update();
        void as_setdata();

        void spm_update_all(int Vflg=0);
        void spm_freeze_scanparam(int subset=0);
        static void freeze_ec(Gtk_EntryControl* ec, gpointer data){ ec->Freeze(); };

        void spm_thaw_scanparam(int subset=0);
        static void thaw_ec(Gtk_EntryControl* ec, gpointer data){ ec->Thaw(); };

        // generic GFunc compat util
        static void show_w (GtkWidget* w, gpointer data){ gtk_widget_show (w); };
        static void hide_w (GtkWidget* w, gpointer data){ gtk_widget_hide (w); };

        void spa_show_scan_time();

        static void spm_range_check(Param_Control* pcs, gpointer data);
        static void spm_offset_check(Param_Control* pcs, gpointer data);
        static void spm_scanpos_check(Param_Control* pcs, gpointer data);
        static void offset_to_preset_callback(GtkWidget* w, gpointer app);

        static void spm_nlayer_update(Param_Control* pcs, gpointer data);
        static void spm_select_layer(Param_Control* pcs, gpointer data);
        static void spm_select_time(Param_Control* pcs, gpointer data);
        static void spa_energy_check(Param_Control* pcs, gpointer data);
        static void spa_gate_check(Param_Control* pcs, gpointer data);
        void spa_mode_switch_check();
        void spa_SeV_unit_switch_check();
        static void spa_switch_unit(Param_Control* pcs, gpointer data);
        static void recalc_volt_from_new_Energy(double* x, double *Eneu);

        static void update_ec(Gtk_EntryControl* ec, gpointer data){ ec->Put_Value(); };
        static void update_parameter(Gtk_EntryControl* ec, gpointer data){ ec->Set_Parameter (); }; // force update parameter from entry text

        ChannelSelector *channelselector;
        //  ProbeControl *probecontrol;
        //  GrOffsetControl *groffsetcontrol;
        MonitorControl *monitorcontrol;
        //  DriftControl *driftcontrol;

        //  OptiControl *opticontrol;
        //  OsziControl *oszicontrol;

        /* Main Menu Shell Referenz */
        GtkWidget *gxsmmenu;

        /* Main XSM Objekt */
        Surface *xsm;

        /* List of remotely acessable entrys */
        GSList *RemoteEntryList;
        GSList *RemoteActionList;
        GSList *RemoteConfigureList;

        GSList *gxsm_app_windows_list;
        
        /* Remote */
        RemoteControl *remotecontrol;

        void add_configure_object_to_remote_list (GnomeResPreferences *grp){
                RemoteConfigureList = g_slist_prepend (RemoteConfigureList, grp);
        };
        void remove_configure_object_from_remote_list (GnomeResPreferences *grp){
                RemoteConfigureList = g_slist_remove (RemoteConfigureList, grp);
        };
        
        /* Gxsm PlugIns management and app menu support */  
	GxsmMenuExtension *gxsm_app_extend_menu (const gchar *extension_point, const gchar *menu_entry_text, const gchar *action);

        static gint Gxsm_Plugin_Check (const gchar *category);
        void reload_gxsm_plugins( gint killflag=FALSE );
        gxsm_plugins *GxsmPlugins;

        void ConnectPluginToSPMRangeEvent( void (*cbfkt)(gpointer) );
        void ConnectPluginToStartScanEvent( void (*cbfkt)(gpointer) );
        void ConnectPluginToStopScanEvent( void (*cbfkt)(gpointer) );
        void ConnectPluginToCDFLoadEvent( void (*cbfkt)(gpointer) ); // => NcFile *ncf
        void ConnectPluginToCDFSaveEvent( void (*cbfkt)(gpointer) ); // => NcFile *ncf
        void ConnectPluginToLoadFileEvent( void (*cbfkt)(gpointer) ); // => gchar **filename
        void ConnectPluginToSaveFileEvent( void (*cbfkt)(gpointer) ); // => gchar **filename
        void ConnectPluginToRemoteAction( void (*cbfkt)(gpointer) );

        void RegisterPluginToolbarButton (GObject *b, const gchar* ToolbarButtonName){ 
                g_object_set_data (G_OBJECT (app_window), ToolbarButtonName, b);
        };

        void ConnectPluginToGetNCInfoEvent (void (*cbfkt)(gchar *filename ) );
        
        void PutPluginData (gchar *data){
                if (DataStack)
                        g_free (DataStack);
                DataStack = g_strdup (data);
        }
        gchar* GetPluginData (){
                gchar *tmp = NULL;
                if (DataStack){
                        tmp = g_strdup (DataStack);
                        g_free (DataStack);
                        DataStack = NULL;
                }
                return tmp;
        }

        void CallGetNCInfoPlugin (gchar *filename) {
                if (PluginCallGetNCInfo){
                        PluginCallGetNCInfo (filename);
                }
        };

        static void CallPlugin( void (*cbfkt)( gpointer ), gpointer data);

        void SignalEventToPlugins( GList *PluginNotifyList, gpointer data );

        void SignalSPMRangeEventToPlugins(void){ 
                SignalEventToPlugins( PluginNotifyOnSPMRange, NULL ); 
        };
        void SignalStartScanEventToPlugins(void){ 
                SignalEventToPlugins( PluginNotifyOnStartScan, NULL ); 
        };
        void SignalStopScanEventToPlugins(void){ 
                SignalEventToPlugins( PluginNotifyOnStopScan, NULL ); 
        };
        void SignalCDFLoadEventToPlugins(NcFile *ncf){ 
                SignalEventToPlugins( PluginNotifyOnCDFLoad, (gpointer)ncf ); 
        };
        void SignalCDFSaveEventToPlugins(NcFile *ncf){ 
                SignalEventToPlugins( PluginNotifyOnCDFSave, (gpointer)ncf ); 
        };
        void SignalLoadFileEventToPlugins(gchar **filename){ 
                SignalEventToPlugins( PluginNotifyOnLoadFile, filename ); 
        };
        void SignalSaveFileEventToPlugins(gchar **filename){ 
                SignalEventToPlugins( PluginNotifyOnSaveFile, filename ); 
        };
        void SignalRemoteActionToPlugins(gchar **actionid){ 
                SignalEventToPlugins( PluginRemoteAction, actionid ); 
        };

        Gxsm4app *get_application () { return gxsm4app; };
        Gxsm4appWindow *get_app_window  () { return app_window; };
        gboolean gxsm_app_window_present () { return app_window ? true : false; };

        GObject *get_gxsm_main_menu () { return gxsm_main_menu; };
        GObject *get_gxsm_app_menu () { return gxsm_app_menu; };
        GObject *get_monitor_menu () { return monitor_menu; };
        GObject *get_view2d_menu () { return view2d_menu; };
        GObject *get_vobj_ctx_menu_1p () { return vobj_ctx_menu_1p; };
        GObject *get_vobj_ctx_menu_2p () { return vobj_ctx_menu_2p; };
        GObject *get_vobj_ctx_menu_np () { return vobj_ctx_menu_np; };
        GObject *get_vobj_ctx_menu_event () { return vobj_ctx_menu_event; };
        GObject *get_view3d_menu () { return view3d_menu; };
        GObject *get_profile_popup_menu () { return profile_popup_menu; };

        //GObject *get_hwi_gvpmover_popup_menu () { return hwi_gvpmover_popup_menu; };
        GObject *get_hwi_mover_popup_menu () { return hwi_mover_popup_menu; };
        GObject *get_hwi_control_popup_menu () { return hwi_control_popup_menu; };

        GObject *get_plugin_pyremote_file_menu () { return plugin_pyremote_file_menu; };
        
        
        GObject *set_gxsm_main_menu (GObject *o=NULL) { if (o) { gxsm_main_menu=o; g_object_ref (o); } return gxsm_main_menu; };
        GObject *set_gxsm_app_menu (GObject *o=NULL) { if (o) { gxsm_app_menu=o; g_object_ref (o); }  return gxsm_app_menu; };
        GObject *set_monitor_menu (GObject *o=NULL) { if (o) { monitor_menu=o; g_object_ref (o); }  return monitor_menu; };
        GObject *set_view2d_menu (GObject *o=NULL) { if (o) { view2d_menu=o; g_object_ref (o); }  return view2d_menu; };
        GObject *set_vobj_ctx_menu_1p (GObject *o=NULL) { if (o) { vobj_ctx_menu_1p=o; g_object_ref (o); } return vobj_ctx_menu_1p; };
        GObject *set_vobj_ctx_menu_2p (GObject *o=NULL) {  if (o) { vobj_ctx_menu_2p=o; g_object_ref (o); }  return vobj_ctx_menu_2p; };
        GObject *set_vobj_ctx_menu_np (GObject *o=NULL) {  if (o) { vobj_ctx_menu_np=o; g_object_ref (o); }  return vobj_ctx_menu_np; };
        GObject *set_vobj_ctx_menu_event (GObject *o=NULL) {  if (o) { vobj_ctx_menu_event=o; g_object_ref (o); }  return vobj_ctx_menu_event; };
        GObject *set_view3d_menu (GObject *o=NULL) { if (o) { view3d_menu=o; g_object_ref (o); }  return view3d_menu; };
        GObject *set_profile_popup_menu (GObject *o=NULL) { if (o) { profile_popup_menu=o; g_object_ref (o); } return profile_popup_menu; };

        GObject *set_hwi_mover_popup_menu (GObject *o=NULL) { if (o) { hwi_mover_popup_menu=o; g_object_ref (o); } return hwi_mover_popup_menu; };
        //GObject *set_hwi_gvpmover_popup_menu (GObject *o=NULL) { if (o) { hwi_gvpmover_popup_menu=o; g_object_ref (o); } return hwi_gvpmover_popup_menu; };
        GObject *set_hwi_control_popup_menu (GObject *o=NULL) { if (o) { hwi_control_popup_menu=o; g_object_ref (o); } return hwi_control_popup_menu; };

        GObject *set_plugin_pyremote_file_menu (GObject *o=NULL) { if (o) { plugin_pyremote_file_menu=o; g_object_ref (o); } return plugin_pyremote_file_menu; };

        GSettings *get_as_settings () { return as_settings; };
        GSettings *get_app_settings () { return gxsm_app_settings; };

        Gxsm4app* get_app() { return gxsm4app; };

protected:
        GList *PluginNotifyOnSPMRange;
        GList *PluginNotifyOnStartScan;
        GList *PluginNotifyOnStopScan;
        GList *PluginNotifyOnCDFLoad;
        GList *PluginNotifyOnCDFSave;
        GList *PluginNotifyOnLoadFile;
        GList *PluginNotifyOnSaveFile;

        GList *PluginRemoteAction;

        void (*PluginCallGetNCInfo)(gchar*);

private:
        /* Gxsm Application */
        GtkWidget* appbar;
        guint      appbar_ctx_id;
        GtkWidget *gxsm_menu;
        GtkWidget *grid;
        GtkWidget *scan_button; //  GtkToolItem
        GtkWidget *tool_button_save_all; // GtkToolButton
        gboolean auto_update_all;
        
        GSettings *gxsm_app_settings;
        GSettings *as_settings;
        
        GObject*   gxsm_main_menu;
        GObject*   gxsm_app_menu;
        GObject*   monitor_menu;
        GObject*   view2d_menu;
        GObject*   vobj_ctx_menu_1p;
        GObject*   vobj_ctx_menu_2p;
        GObject*   vobj_ctx_menu_np;
        GObject*   vobj_ctx_menu_event;
        GObject*   view3d_menu;
        GObject*   profile_popup_menu;

        GObject*   hwi_mover_popup_menu;
        //GObject*   hwi_gvpmover_popup_menu;
        GObject*   hwi_control_popup_menu;

        GObject*   plugin_pyremote_file_menu;

        gchar *DataStack;

public:
        Gxsm4appWindow *app_window;
        int glb_ref_point_xylt_index[4];
        double glb_ref_point_xylt_world[4];
};

#endif
