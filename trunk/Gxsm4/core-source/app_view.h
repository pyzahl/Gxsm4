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


#ifndef APP_VIEW_H
#define APP_VIEW_H

#include "xshmimg.h"
#include "gapp_service.h"
#include "view.h"

#define OB_MARKER_SCALE (xsmres.HandleSize/100./vc->vinfo->GetZfac())

class Mem2d;
class ViewControl;
class app_vpdata_view;

class ViewControl : public AppBase{
public:
        ViewControl(Gxsm4app *app, char *title, int nx, int ny, int ChNo, Scan *scan=NULL, int ZoomFac=1, int QuenchFac=1);
        virtual ~ViewControl();

        void Resize(char *title, int nx, int ny, int ChNo, Scan *scan=NULL, int ZoomFac=1, int QuenchFac=1);

        virtual void AppWindowInit(const gchar *title=NULL, const gchar *sub_title=NULL);

        static void view_file_openhere_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data);
        static void view_file_save_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data);
        static void view_file_save_as_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data);
        static void view_file_update_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data);
        static void view_file_save_drawing_exec  (GtkDialog *dialog,  int response, gpointer user_data);
        static void view_file_saveimage_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data);
        static void view_file_saveobjects_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data);
        static void view_file_loadobjects_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data);
        static void view_file_getinfo_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data);
        static void view_file_print_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data);
        static void view_file_kill_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data);
        static void view_window_close_callback (GtkWidget *widget, ViewControl *vc);
        static void view_edit_copy_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data);
        static void view_edit_crop_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data);
        static void view_edit_zoomin_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data);
        static void view_edit_zoomout_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data);
        static void view_tool_remove_all_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data);
        static void view_tool_remove_active_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data);
        static void view_tool_mvprop_radius_radio_callback (GSimpleAction *action, GVariant *parameter, gpointer user_data);
        static void view_tool_all2locmax_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data);
        static void view_object_mode_radio_callback (GSimpleAction *action, GVariant *parameter, gpointer user_data);
        static void view_tool_addpoint (GtkWidget *widget, ViewControl *vc);
        static void view_tool_addmarker (GtkWidget *widget, ViewControl *vc);
        static void view_tool_addshowpoint (GtkWidget *widget, ViewControl *vc);
        static void view_tool_addline (GtkWidget *widget, ViewControl *vc);
        static void view_tool_addpolyline (GtkWidget *widget, ViewControl *vc);
        static void view_tool_addksys (GtkWidget *widget, ViewControl *vc);
        static void view_tool_addparabel (GtkWidget *widget, ViewControl *vc);
        static void view_tool_addrectangle (GtkWidget *widget, ViewControl *vc);
        static void view_tool_addcircle (GtkWidget *widget, ViewControl *vc);
        static void view_tool_addshowcircle (GtkWidget *widget, ViewControl *vc);
        static void view_tool_addshowline (GtkWidget *widget, ViewControl *vc);
        static void view_tool_labels_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data);
        static void view_tool_legend_radio_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data);
        static void view_tool_legend_position_radio_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data);
        static void view_tool_marker_group_radio_callback (GSimpleAction *action, GVariant *parameter, gpointer user_data);
        //
        static void view_view_set_view_mode_radio_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data);
        static void view_view_x_linearize_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data);
        static void view_view_attach_redline_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data);
        static void view_view_redline_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data);
        static void view_view_blueline_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data);
        static void view_view_world_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data);
        static void view_view_color_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data);
        static void view_view_color_rgb_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data);
        static void view_view_tolerant_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data);
        //
        static void view_view_coordinate_mode_radio_callback (GSimpleAction *action, GVariant *parameter, gpointer user_data);
        static void view_view_coord_time_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data);
        static void view_view_toggleoffset_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data);
        static void view_view_zoom_in_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data){
                ViewControl *vc = (ViewControl *) user_data; if(vc->ZoomQFkt) (*vc->ZoomQFkt)(1,0,vc->ZQFktData); };
        static void view_view_zoom_out_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data){
                ViewControl *vc = (ViewControl *) user_data; if(vc->ZoomQFkt) (*vc->ZoomQFkt)(0,1,vc->ZQFktData); };
        static void view_view_zoom_fix_radio_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data);

        static void events_probe_callback (GtkWidget *widget, gpointer user_data);
        static void events_user_callback (GtkWidget *widget, gpointer user_data);

        static void events_labels_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data);
        static void events_verbose_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data);
        static void events_remove_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data);
        static void indicators_remove_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data);
        void events_update ();

        static void sort_time_elements_callback (GSimpleAction *action, GVariant *parameter, gpointer user_data);

        static void action_callback (GSimpleAction *action, GVariant *parameter, gpointer user_data);

        static void Activate_window_callback (GtkWindow *window, gpointer user_data);
        static void Activate_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data);
        static void SetOff_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data);
        static void SetOn_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data);
        static void SetMath_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data);
        static void SetX_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data);
        static void AutoDisp_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data);
        static void MapToWorld_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data);
        static void ClrWorld_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data);

        static void obj_remove_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data);
        static void obj_properties_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data);
        static void obj_dump_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data);
        static void obj_reset_counter_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data);
        static void obj_show_counter_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data);

        static void obj_event_add_ref_probe_callback (GtkWidget *widget, gpointer user_data);
        static void obj_event_clear_ref_probe_callback (GtkWidget *widget, gpointer user_data);
        static void obj_circle_get_center_coords_callback (GtkWidget *widget, gpointer user_data);
        static void obj_event_plot_callback (GtkWidget *widget, gpointer user_data);
        static void obj_event_dump_callback (GtkWidget *widget, gpointer user_data);

        static void side_pane_action_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data); // F9 accel key
        static void side_pane_callback (GtkWidget *widget, gpointer user_data);
        static void tip_follow_callback (GtkWidget *widget, gpointer user_data);
        static void scan_start_stop_callback (GtkWidget *widget, gpointer user_data);
        static void close_side_pane_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data);

        static void obj_setoffset_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data);
        static void obj_global_ref_point_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data);
        static void obj_getcoords_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data);
        static void obj_follow_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data);
        static void obj_go_locmax_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data);
        static void obj_addnode_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data);
        static void obj_delnode_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data);
        static void obj_event_use_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data);
        static void obj_event_save_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data);
        static void obj_event_save_callback_exec (GtkDialog *dialog,  int response, gpointer user_data);
        static void obj_event_remove_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data);
        static void check_obj_event(VObject *vo, ViewControl *vc){ 
                if  (vc->tmp_effected == 0){
                        vc->tmp_object_op = NULL;
                        if (vo->check_event (vc->tmp_event, vc->tmp_xy)){
                                vc->tmp_effected++; 
                                vc->tmp_object_op = vo;
                        }
                }
        };
        static void move2locmax_obj(VObject *vo, ViewControl *vc){ vo->GoLocMax(vc->local_radius); };
        static void obj_update(VObject *vo, ViewControl *vc){ 
                int spt[2];
                spt[0] = vc->scan->data.display.vlayer;
                spt[1] = vc->scan->data.display.vframe;
                vo -> set_spacetime (spt);
                vo -> Update();
        };
        static void obj_label_on(VObject *vo, ViewControl *vc){ vo->show_label(true); };
        static void obj_label_off(VObject *vo, ViewControl *vc){ vo->show_label(false); };
        static void remove_obj(VObject *vo, ViewControl *vc){ 
                if (vc->tmp_object_op == vo){
                        main_get_gapp ()->message ("Ilya don't do that!");
                        vc->tmp_object_op = NULL;
                }
                vc->scan->del_object (vo);
        };
        static void unflag_scan_event_and_remove_obj(VObject *vo, ViewControl *vc){ 
                vo->get_scan_event () -> flag = FALSE;
                vc->scan->del_object (vo);
        };
        static void draw_obj(VObject *vo, cairo_t *cr){ vo->draw (cr);  };
        static void activate_obj(VObject *vo, ViewControl *vc){ vo->set_color_to_active();  };
        static void deactivate_obj(VObject *vo, ViewControl *vc){ vo->set_color_to_inactive(); };
        static void save_obj(VObject *vo, ViewControl *vc){ vo->save(vc->objsavestream); };
        static void save_obj_HPGL(VObject *vo, ViewControl *vc){ vo->saveHPGL(vc->objsavestream); };

        //static gint canvas_event_cb(GtkWidget *canvas, GdkEvent *event, ViewControl *vo);

        // Objects Handling
        gboolean check_on_object(VObjectEvent* event);
        static void drag_motion (GtkEventControllerMotion *motion, gdouble x, gdouble y, ViewControl *vc);
        static void pressed_cb (GtkGesture *gesture, int n_press, double x, double y, ViewControl *vc);
        static void released_cb (GtkGesture *gesture, int n_press, double x, double y, ViewControl *vc);

#if 0   //TEST STUFF
        static GdkContentProvider* drag_prepare (GtkDropTarget *source, double x, double y, ViewControl *vc);
        static void drag_begin (GtkDragSource *source, GdkDrag *drag, ViewControl *vc);
        static void drag_end (GtkDragSource *source, GdkDrag *drag, gboolean delete_data, ViewControl *vc);
        static void drag_cancel (GtkDragSource *source, GdkDrag *drag, GdkDragCancelReason  reason, ViewControl *vc);
        static gboolean drag_drop (GtkDropTarget *target, const GValue  *value, double x, double y, ViewControl *vc);
#endif
        
        static void osd_check_callback (GtkWidget *widget, ViewControl *vc);
        static void osd_on_toggle_callback (GtkWidget *widget, ViewControl *vc);
        static void osd_off_toggle_callback (GtkWidget *widget, ViewControl *vc);

        static void canvas_draw_function (GtkDrawingArea *area,
                                          cairo_t        *cr,
                                          int             width,
                                          int             height,
                                          ViewControl *vc);

        static void display_changed_hl_callback (Param_Control *pc, gpointer vc);
        static void display_changed_vr_callback (Param_Control *pc, gpointer vc);
        static void display_changed_sh_callback (Param_Control *pc, gpointer vc);

        static void update_ec(Gtk_EntryControl* ec, UnitObj *u){
                ec->Put_Value();
                if (u){
                        if (strcmp (u->Symbol (), ec->get_unit_symbol())){
                                XSM_DEBUG(DBG_L2, "uvp-uec-unit change: " << u->Symbol());
                                ec->changeUnit(u);
                        }
                }
        };

        void view_file_save_drawing (const gchar* imgname); // save current view as png, pdf, svg auto detected by file name extension
        
        void update_view_panel (){
                g_slist_foreach (view_bp->get_ec_list_head (), (GFunc) ViewControl::update_ec, scan->data.Zunit);
        };
        
        void setup_side_pane (gboolean show);
        void tip_follow_control (gboolean follow);
        gboolean tip_follow_mode () { return tip_follow_flag; };
        
        void SetActive(int flg);
        void CheckRedLine();
        GtkWidget *GetCanvas(){ return canvas; };
        ShmImage2D *GetXImg(){ return ximg; };

        void DrawObjects(cairo_t *cr);
        void UpdateObjects();

        void AddObject(VObject *vo);
        void MoveAllObjects2LocMax();
        void CheckAllObjectsLabels (int flg=-1);
        void RemoveObjects();
        void SaveObjects();
        void SaveObjectsHPGL();

        void AddIndicator(VObject *vo);
        void RemoveIndicators();

        void SetEventFilter(const gchar *filter, gint pos);
        void RescanEventObjects();
        void RemoveEventObjects(VObject *vo=NULL);
        static void add_event_obj(ScanEvent *se, ViewControl *vc);

        void SetEventLabels(int mode);
        static void set_event_label(ScanEvent *se, ViewControl *vc);

        void SetZoomQFkt(int (*ZQFkt)(int, int, Grey2D*), Grey2D *data){ ZoomQFkt = ZQFkt; ZQFktData=data; };

        void PaintAllRegionsInactive ();
        void PaintAllRegionsActive ();

        void update_trace (double *xy, int len);
        void remove_trace ();
        void update_event_panel (ScanEvent *se);

        ScanEvent* GetActiveScanEvent () { return active_event; };

        void set_osd (gchar *osd_text, int pos);

        GtkWidget* grab_canvas(){ return canvas; }; // only for OSD grabbing purpose!

        void SetMarkerGroup (const gchar *mgcolor=NULL);
        gchar *MakeMarkerLabel () { return g_strdup_printf ("%d:%d", marker_group, ++marker_counter [marker_group]); };

        GtkWidget *canvas;
        GtkWidget *side_pane_tab_objects;

        inline int get_npx () { return npx; }
        inline int get_npy () { return npy; }
        
private:
        GSettings *view_settings;
        gboolean destruction_in_progress;
        gboolean tip_follow_flag;
        gboolean attach_redline_flag;
        
        void (*AddObjFkt)(GtkWidget*, ViewControl*);
        int (*ZoomQFkt)(int, int, Grey2D*);
        Grey2D *ZQFktData;

        int rulewidth;
        int border;
        int npx,npy;
        int chno;
        int usx,usy;

        const gchar *legend_items_code;
        int legend_position_code;
        int legend_size_code;
        int legend_color_code;
        
        int local_radius;
        Scan *scan;
        ProfileControl *RedLine;
        ProfileControl *EventPlot[4];
        app_vpdata_view *EventGraphView;

        ViewInfo *vinfo;
        BuildParam *view_bp;
        BuildParam *pe_bp;
        BuildParam *ue_bp;
        GSList  *gobjlist;
        GSList  *geventlist;
        GSList  *gindicatorlist;
        GSList  *greference_eventlist;
        GtkWidget *v_popup_menu;
        GtkWidget *v_popup_menu_cv;
        GActionGroup *view_action_group;
	GtkWidget *coordpopup;
	GtkWidget *coordlab;

        VObject *current_vobject;

        GtkWidget *side_pane_control;
        GtkWidget *gridnet;
        GtkWidget *hpaned;
        GtkWidget *sidepane;
        GtkWidget *tab_info;
        GtkWidget *tab_ncraw;

        double ActiveFrameWidth; // 8 at zoomfac 1, 0 for inactive.
        double CursorXYVt[4];
        double CursorRadius;
        double ReferenceWeight;
        gint   MaxNumberEvents;
        double ArrowSize;
        Gtk_EntryControl* ec_zoom;
        Gtk_EntryControl* ec_radius;
        Gtk_EntryControl* ec_number;
        Gtk_EntryControl* ec_arrowsize;
        GtkWidget* tog_plot;
        GtkWidget* tog_average;
        GtkWidget* tog_probe_events;
        GtkWidget* tog_user_events;
        GtkWidget* control_box;

        GtkWidget* pe_info[5];
        GtkWidget* ue_info[10];

#define OSD_MAX 20
        GtkWidget* osd_entry[OSD_MAX];
        VObPoint* osd_item[OSD_MAX];
        int osd_item_enable[OSD_MAX];
        int osd_item_active_count;
        
        guint active_event_num_channels;
        guint active_event_num_events;
        GtkWidget* active_event_xchan;
        GtkWidget* active_event_ychan[4];
        GtkWidget* active_event_ynchan;
        GtkWidget* active_event_list;
        GtkWidget* select_events_by;
        GtkWidget* plot_formula;
        GtkWidget* control_box_event_xysel;

        ScanEvent* active_event;


        ShmImage2D *ximg;

        VObTrace *v_trace;

        gchar *event_filter;

        std::ofstream objsavestream;
        std::ifstream objloadstream;

        gint marker_group;
        gint marker_counter[6];

        //----- tmp data
        VObjectEvent *tmp_event;
        double   *tmp_xy;
        int     tmp_effected;
        VObject *tmp_object_op;
        gboolean obj_drag_start;
        gboolean pointer_coord_display;
};

#endif
