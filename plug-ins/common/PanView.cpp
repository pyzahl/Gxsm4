/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 8 c-style: "K&R" -*- */

/* Gnome gxsm - Gnome X Scanning Microscopy
 * universal STM/AFM/SARLS/SPALEED/... controlling and
 * data analysis software
 * 
 * Gxsm Plugin Name: PanView.C
 * ========================================
 * 
 * Copyright (C) 2003 The Free Software Foundation
 *
 * Authors: Kristan Temme <etptt@users.sf.net>
 *          Percy Zahl <zahl@users.sf.net>
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

/* Please do not change the Begin/End lines of this comment section!
 * this is a LaTeX style section used for auto generation of the PlugIn Manual 
 * Chapter. Add a complete PlugIn documentation inbetween the Begin/End marks!
 * --------------------------------------------------------------------------------
% BeginPlugInDocuSection
% PlugInDocuCaption: Pan View

% PlugInName: PanView
% PlugInAuthor: Kristan Temme, Thorsten Wagner, Percy Zahl
% PlugInAuthorEmail: stm@users.sf.net
% PlugInMenuPath: Tools/Pan View 
% PlugInDescription
 This is a handy tool which shows you, where your current scan is in the
 range of the maximum scan. Especially it gives you an error message if you
 leave the scan area. It also shows the position of rotated scans.

 In addition the current realtime XY position of the tip plus it
 indicates the Z-position of the tip is visualized by the marker on
 the right edge of the window.

\GxsmScreenShot{PanView}{The PanView window.}


% PlugInUsage
 Although this is a plugin it is opened automatically upon startup of GXSM.
 There is not interaction with the user.

\GxsmScreenShot{PanView_indicators}{The indicators of the PanView window.}

 1.) Indicator of the state machine on the DSP. In general the colors 
 indicate green=ON/in progress, red=OFF/inactive. From left to right
 the boxed indicate the status of the feedback, scan in progress, 
 vector proce in progress, mover in progress (coarse approach)

 2.) Indicators of the 8 GPIO channels. They can be read on/off 
 (red/black) or write on/off (green/white) giving you four possible 
 states.

 3.) Indicator of the Z position (Z-offset/z-scan)

% OptPlugInNotes
 The tip-position is close to realtime, but it is refreshed only
 several times per second and only if GXSM is idle. Thus the
 display/update may get stuck at times GXSM is very busy. Never the
 less the tip position is read back from the DSP and thus indicates
 the true position, regardless what in happening to GXSM of the DSP --
 so if anything goes wrong, you will see it here!

% OptPlugInHints
 For now the plug-in assumes a scan which is centered in the middle of the
 scan not in the middle of the topline (as default for the pci32).

% OptPlugInKnownBugs
 None

% EndPlugInDocuSection
 * -------------------------------------------------------------------------------- 
 */
#include "config.h"
#include "plugin.h"
#include "glbvars.h"
#include "xsmtypes.h"

#include "gxsm_app.h"
#include "gxsm_window.h"
#include "surface.h"

#include "PanView.h"

#define UTF8_ANGSTROEM "\303\205"
#define UTF8_DEGREE "\302\260"

// Plugin Prototypes
static void PanView_init (void);
static void PanView_query (void);
static void PanView_about (void);
static void PanView_configure (void);
static void PanView_cleanup (void);

PanView *Pan_Window = NULL;

gboolean PanView_valid = FALSE;

#define PRE_AREA 0

// #define PI_DEBUG(NN, TXT) std::cout << TXT << std::endl

// XBM bitmap
#define warn_width 20
#define warn_height 20
const guchar warn_bits[] = {
   0xff, 0x01, 0x08, 0xff, 0x00, 0x0c, 0x7f, 0x00, 0x0e, 0x3f, 0x00, 0x0f,
   0x1f, 0x80, 0x0f, 0x0f, 0xc0, 0x0f, 0x07, 0xe0, 0x0f, 0x03, 0xf0, 0x0f,
   0x01, 0xf8, 0x0f, 0x00, 0xfc, 0x0f, 0x00, 0xfe, 0x07, 0x00, 0xff, 0x03,
   0x80, 0xff, 0x01, 0xc0, 0xff, 0x00, 0xe0, 0x7f, 0x00, 0xf0, 0x3f, 0x00,
   0xf8, 0x1f, 0x00, 0xfc, 0x0f, 0x00, 0xfe, 0x07, 0x00, 0xff, 0x03, 0x00 };

static void PanView_show_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data);

      // Fill in the GxsmPlugin Description here
GxsmPlugin PanView_pi = {
	  NULL,                   // filled in and used by Gxsm, don't touch !
	  NULL,                   // filled in and used by Gxsm, don't touch !
	  0,                      // filled in and used by Gxsm, don't touch !
	  NULL,                   // The Gxsm-App Class Ref.pointer (called "gapp" in Gxsm) is
	  // filled in here by Gxsm on Plugin load,
	  // just after init() is called !!!
	  // ----------------------------------------------------------------------
	  // Plugins Name, CodeStly is like: Name-M1S[ND]|M2S-BG|F1D|F2D|ST|TR|Misc
	  (char *)"Pan View",
	  NULL,
	  // Description, is shown by PluginViewer (Plugin: listplugin, Tools->Plugin Details)
	  (char *)"Scan Area visualization and life SPM parameter readings",
	  // Author(s)
	  (char *) "Kristan Temme, Thorsten Wagner, Percy Zahl",
	  // Menupath to position where it is appended to
	  (char *)"windows-section",
	  // Menuentry
	  N_("Pan View"),
	  // help text shown on menu
	  N_("Scan Area visualization and SPM data view."),
	  // more info...
	  (char *)"See Manual.",
	  NULL,          // error msg, plugin may put error status msg here later
	  NULL,          // Plugin Status, managed by Gxsm, plugin may manipulate it too
	  // init-function pointer, can be "NULL",
	  // called if present at plugin load
	  PanView_init,
	  // query-function pointer, can be "NULL",
	  // called if present after plugin init to let plugin manage it install itself
	  PanView_query, // query should be "NULL" for Gxsm-Math-Plugin !!!
	  // about-function, can be "NULL"
	  // can be called by "Plugin Details"
	  PanView_about,
	  // configure-function, can be "NULL"
	  // can be called by "Plugin Details"
	  PanView_configure,
	  // run-function, can be "NULL", if non-Zero and no query defined,
	  // it is called on menupath->"plugin"
	  NULL, // run should be "NULL" for Gxsm-Math-Plugin !!!
	  // cleanup-function, can be "NULL"
	  // called if present at plugin removal
          PanView_show_callback, // direct menu entry callback1 or NULL
          NULL, // direct menu entry callback2 or NULL

	  PanView_cleanup
};

//
//Text used in the About Box
//
static const char *about_text = N_("A little scanning reference");
                                   
	
//
// Symbol "get_gxsm_plugin_info" is resolved by dlsym from Gxsm, used to get Plugin's info!! 
// Essential Plugin Function!!
//
GxsmPlugin *get_gxsm_plugin_info ( void ){ 
	PanView_pi.description = g_strdup_printf(N_("Gxsm pan view window %s"), VERSION);
	return &PanView_pi; 
}

//
// init-Function
//

static void PanView_show_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data){
        PI_DEBUG (DBG_L2, "PanView_show_callback" );
	if( Pan_Window ){
                Pan_Window->show();
		Pan_Window->start_tip_monitor ();
	}
}

static void PanView_refresh_callback (gpointer data){
	if (Pan_Window)
		Pan_Window -> refresh ();
}

static void PanView_init(void)
{
	PI_DEBUG(DBG_L2, "PanView Plugin Init" );
}

// Query Function, installs Plugin's in File/Import and Export Menupaths!
// ----------------------------------------------------------------------
// Import Menupath is "File/Import/abc import"
// Export Menupath is "File/Export/abc import"
// ----------------------------------------------------------------------
// !!!! make sure the "spm_scancontrol_cleanup()" function (see below) !!!!
// !!!! removes the correct menuentries !!!!

// Add Menu Entries:
// windows-sectionSPM Scan Control
// Action/SPM Scan Start/Pause/Stop + Toolbar

static void PanView_query(void)
{
        PI_DEBUG (DBG_L2, "PanView_query enter" );

	if(PanView_pi.status) g_free(PanView_pi.status); 
	PanView_pi.status = g_strconcat (
		N_("Plugin query has attached, status:"),
		(xsmres.HardwareType[0] == 'n' && xsmres.HardwareType[1] == 'o')?"Offline (no hardware)":"Online",
		PanView_pi.name, 
		NULL);

#if 1 // disable auto terminate with no real hardware
	if (xsmres.HardwareType[0] == 'n' && xsmres.HardwareType[1] == 'o'){
                Pan_Window = NULL;
		return;
        }
#endif
   
	Pan_Window = new PanView (main_get_gapp() -> get_app ()); // PanView(PanView_pi.app->getApp());
	PanView_pi.app->ConnectPluginToSPMRangeEvent (PanView_refresh_callback);
	Pan_Window->start_tip_monitor ();

// not yet needed
//	PanView_pi.app->ConnectPluginToCDFSaveEvent (PanView_SaveValues_callback);

	PI_DEBUG (DBG_L2, "PanView_query:done" );
}

//
// about-Function
//
static void PanView_about(void)
{
	const gchar *authors[] = { PanView_pi.authors, NULL};
	gtk_show_about_dialog (NULL, 
			       "program-name",  PanView_pi.name,
			       "version", VERSION,
			       "license", GTK_LICENSE_GPL_3_0,
			       "comments", about_text,
			       "authors", authors,
			       NULL
			       );
}
//
// configure-Function
//
static void PanView_configure(void)
{
        PI_DEBUG (DBG_L2, "PanView_configure" );
	if(PanView_pi.app)
		PanView_pi.app->message("PanView Plugin Configuration");

	if (Pan_Window){
		if (Pan_Window->finish ()){
			PanView_pi.app->message("PanView: Starting tip monitor");
			Pan_Window->start_tip_monitor ();
		} else {
			PanView_pi.app->message("PanView: Stopping tip monitor");
			Pan_Window->stop_tip_monitor ();
		}
	}
}

//
// cleanup-Function, make sure the Menustrings are matching those above!!!
//
static void PanView_cleanup(void)
{
	PanView_valid = FALSE;

	PI_DEBUG(DBG_L2, "PanView_cleanup(void). Entering.\n");
	if (Pan_Window) {
		delete Pan_Window;
	}
	PI_DEBUG(DBG_L2, "PanView_cleanup(void). Exiting.\n");
}

static gint PanView_tip_refresh_callback (PanView *pv){
	if (!PanView_valid) return FALSE;

	if (!main_get_gapp()->xsm->hardware)
		return TRUE;

	if (main_get_gapp()->xsm->hardware->IsSuspendWatches ())
		return TRUE;

	if (pv){
		if (! pv -> finish ()){
			pv -> tip_refresh ();
			return TRUE;
		}
		pv -> finish (-999);
	}
	return FALSE;
}


PanView::PanView (Gxsm4app *app):AppBase(app){
 	int i;

	pan_area = NULL;
	pan_area_extends = NULL;
    	current_view = NULL;
	current_view_zoom = NULL;
	pre_current_view = NULL;
        tip_marker = NULL;
	tip_marker_zoom = NULL;
        tip_marker_z = NULL;
        tip_marker_z0 = NULL;
        info = NULL;
        infoXY0 = NULL;
        show_preset_grid = false;

	timer_id = 0;

	for(i=0; i<16; ++i){
		DSP_status_indicator[i] = NULL;
                DSP_status_indicator_ID[i] = NULL;
        }
        
	for(i=0; i<16; ++i)
		DSP_gpio_indicator[i] = NULL;

        for(i=0; i<N_PRESETS; ++i)
                for(int j=0; j<N_PRESETS; ++j)
                        pos_preset_box[i][j] = NULL;

	finish (FALSE);

	AppWindowInit (N_("Pan View and OSD"));

	// ========================================
	/* set up the system relevant Parameters */
	update_expanded_scan_limits ();

	// Cairo: gtk drawing area
	canvas = gtk_drawing_area_new(); // gtk3 cairo drawing-area -> "canvas"

        gtk_drawing_area_set_content_width  (GTK_DRAWING_AREA (canvas), WXS+24);
        gtk_drawing_area_set_content_height (GTK_DRAWING_AREA (canvas), WXS+24);

        gtk_drawing_area_set_draw_func (GTK_DRAWING_AREA (canvas),
                                        G_CALLBACK (PanView::canvas_draw_function),
                                        this, NULL);


        // mouse gestures/clicks
        GtkGesture *gesture = gtk_gesture_click_new ();
        gtk_gesture_single_set_button (GTK_GESTURE_SINGLE (gesture), 0);
        g_signal_connect (gesture, "released", G_CALLBACK (released_cb), this);
        gtk_widget_add_controller (canvas, GTK_EVENT_CONTROLLER (gesture));

        GtkEventController* motion = gtk_event_controller_motion_new ();
        g_signal_connect (motion, "enter", G_CALLBACK (motion_enter_cb), this);
        g_signal_connect (motion, "motion", G_CALLBACK (motion_cb), this);
        g_signal_connect (motion, "leave", G_CALLBACK (motion_leave_cb), this);
        gtk_widget_add_controller (canvas, GTK_EVENT_CONTROLLER (motion));
        
	gtk_widget_show (canvas);
	gtk_grid_attach (GTK_GRID (v_grid), canvas, 1,1, 10,10);

	pan_area_extends = new cairo_item_rectangle (min_x, min_y, max_x, max_y);
	pan_area_extends->set_stroke_rgba (CAIRO_COLOR_RED);
	pan_area_extends->set_fill_rgba (CAIRO_COLOR_YELLOW);
	pan_area_extends->set_line_width (get_lw (1.0));
	pan_area_extends->queue_update (canvas);

        double pxw=(2*x0r)/N_PRESETS;
        double pyw=(2*y0r)/N_PRESETS;
        for(i=0; i<N_PRESETS; ++i)
                for(int j=0; j<N_PRESETS; ++j){
                        pos_preset_box[i][j] = new cairo_item_rectangle(0,0, pxw,pyw);
                        pos_preset_box[i][j]->set_position (-x0r+pxw*i, -y0r+pyw*j);
                        pos_preset_box[i][j]->set_stroke_rgba (CAIRO_COLOR_GREY1);
                        pos_preset_box[i][j]->set_fill_rgba (0,0,0,0.1);
                        pos_preset_box[i][j]->set_line_width (get_lw (1.0));
                        pos_preset_box[i][j]->queue_update (canvas);
                }
        
  	/*create a rectangle*/
	pan_area = new cairo_item_rectangle (x0r, y0r, -x0r, -y0r);
	pan_area->set_stroke_rgba (CAIRO_COLOR_RED);
	pan_area->set_fill_rgba (CAIRO_COLOR_WHITE);
	pan_area->set_line_width (get_lw (1.0)); // fix me
	pan_area->queue_update (canvas);

        // TEXTs are drawn in fixed pixel coordinate system
	info = new cairo_item_text (WXS/2.-10, -WYS/2.+5., "I: --- nA");
	//	info->set_text ("updated text")
	info->set_stroke_rgba (CAIRO_COLOR_BLUE);
	info->set_font_face_size ("Ununtu", 12.);
	info->set_spacing (-.1);
	info->set_anchor (CAIRO_ANCHOR_E);
	info->queue_update (canvas);

	infoXY0 = new cairo_item_text (WXS/2.-10, +WYS/2.-8., "XY0: --,--");
	infoXY0->set_stroke_rgba (CAIRO_COLOR_BLUE);
	infoXY0->set_font_face_size ("Ununtu", 12.);
	infoXY0->set_spacing (.1);
	infoXY0->set_anchor (CAIRO_ANCHOR_E);
	infoXY0->queue_update (canvas);
 	

	PanView_valid = TRUE;

	refresh ();
	tip_refresh ();

}

PanView::~PanView (){
	PanView_valid = FALSE;

        PI_DEBUG (DBG_L4, "PanView::~PanView -- stop_tip_monitor RTQuery");

	stop_tip_monitor ();

        UNREF_DELETE_CAIRO_ITEM (pan_area_extends, canvas);
        UNREF_DELETE_CAIRO_ITEM (pan_area, canvas);
        UNREF_DELETE_CAIRO_ITEM (info, canvas);
        UNREF_DELETE_CAIRO_ITEM (current_view, canvas);
        UNREF_DELETE_CAIRO_ITEM (pre_current_view, canvas);
        UNREF_DELETE_CAIRO_ITEM (infoXY0, canvas);
        UNREF_DELETE_CAIRO_ITEM (tip_marker, canvas);
        UNREF_DELETE_CAIRO_ITEM (tip_marker_zoom, canvas);
        UNREF_DELETE_CAIRO_ITEM (tip_marker_z, canvas);
        UNREF_DELETE_CAIRO_ITEM (tip_marker_z0, canvas);

	for (int i=0; i<16; ++i){
                UNREF_DELETE_CAIRO_ITEM (DSP_status_indicator[i], canvas);
                UNREF_DELETE_CAIRO_ITEM (DSP_status_indicator_ID[i], canvas);
        }
	for (int i=0; i<16; ++i){
                UNREF_DELETE_CAIRO_ITEM (DSP_gpio_indicator[i], canvas);
        }
        for(int i=0; i<N_PRESETS; ++i)
                for(int j=0; j<N_PRESETS; ++j){
                        UNREF_DELETE_CAIRO_ITEM (pos_preset_box[i][j], canvas);
                }
        
	PI_DEBUG (DBG_L4, "PanView::~PanView () -- done.");
}

void PanView::AppWindowInit(const gchar *title, const gchar *sub_title){
        PI_DEBUG (DBG_L2, "PanView::AppWindowInit -- header bar");

        app_window = gxsm4_app_window_new (GXSM4_APP (main_get_gapp()->get_application ()));
        window = GTK_WINDOW (app_window);

	gtk_window_set_default_size (GTK_WINDOW(window), WXS+12, WYS+2);
	gtk_window_set_title (GTK_WINDOW(window), title);
	gtk_widget_set_opacity (GTK_WIDGET(window), 0.75);
	gtk_window_set_resizable (GTK_WINDOW(window), FALSE);
	gtk_window_set_decorated (GTK_WINDOW(window), FALSE);
        
	v_grid = gtk_grid_new ();
        gtk_window_set_child (GTK_WINDOW (window), v_grid);
	gtk_widget_show (v_grid);
	g_object_set_data (G_OBJECT (window), "v_grid", v_grid);
	gtk_widget_show (GTK_WIDGET (window));

	set_window_geometry ("pan-view");
}

void PanView::determine_ij_patch (gdouble x, gdouble y, int &i, int &j){
        double mouse_pix_xy[2];
        //---------------------------------------------------------
	// cairo_translate (cr, 12.+pv->WXS/2., 12.+pv->WYS/2.);
        // scale to volate range
	// cairo_scale (cr, 0.5*pv->WXS/pv->max_x, -0.5*pv->WYS/pv->max_y);

        // undo cairo image translation/scale:
        mouse_pix_xy[0] = (x - (double)(12+WXS/2.))/( 0.5*WXS/max_x);
        mouse_pix_xy[1] = (y - (double)(12+WYS/2.))/( 0.5*WYS/max_y);
   
        double pxw=(2*x0r)/N_PRESETS;
        double pyw=(2*y0r)/N_PRESETS;

        i = (int)((mouse_pix_xy[0]+x0r)/pxw);
        j = (N_PRESETS-1)-(int)((mouse_pix_xy[1]+y0r)/pyw);
        if (i < 0 || i > (N_PRESETS-1))
                i=j=-1;
        if (j < 0 || j > (N_PRESETS-1))
                i=j=-1;
}

void PanView::motion_enter_cb (GtkEventControllerMotion *motion, gdouble x, gdouble y, PanView *pv){
        pv->show_preset_grid = true;
}

void PanView::motion_cb (GtkEventControllerMotion *motion, gdouble x, gdouble y, PanView *pv){
        static int pi=-1;
        static int pj=-1;
        int i,j;
        pv->determine_ij_patch (x,y, i,j);
        if (pi != i || pj != j){
                if (pi>=0 && pj>=0){
                        pv->pos_preset_box[pi][pj]->set_fill_rgba (0,0,0,0.1);
                        pi=pj=-1;
                }
                        
                if (i>=0 && j>=0){
                        pv->pos_preset_box[i][j]->set_fill_rgba (1,0,0,0.3);
                        pi=i; pj=j;
                }
                // g_message ("PanView XY=%g, %g => %d, %d",  mouse_pix_xy[0], mouse_pix_xy[1], i,j);
        }
}

void PanView::motion_leave_cb (GtkEventControllerMotion *motion, gdouble x, gdouble y, PanView *pv){
        pv->show_preset_grid = false;
}

void PanView::released_cb (GtkGesture *gesture, int n_press, double x, double y, PanView *pv){
        static double preset[3];
        int i,j;
        pv->determine_ij_patch (x,y, i,j);
        preset[0] = i-(N_PRESETS-1)/2;
        preset[1] = j-(N_PRESETS-1)/2;
        preset[2] = N_PRESETS/2.0;
        g_object_set_data (G_OBJECT(pv->canvas), "preset_xy", preset);
        main_get_gapp()->offset_to_preset_callback (pv->canvas, gapp);
        // g_message ("PanView Button1 Pressed at XY=%g, %g => %d, %d",  mouse_pix_xy[0], mouse_pix_xy[1], i,j );
}

gboolean PanView::canvas_draw_function (GtkDrawingArea *area,
                                        cairo_t        *cr,
                                        int             width,
                                        int             height,
                                        PanView *pv){
        // translate origin to window center
	cairo_translate (cr, 12.+pv->WXS/2., 12.+pv->WYS/2.);
        cairo_save (cr);

        // scale to volate range
	cairo_scale (cr, 0.5*pv->WXS/pv->max_x, -0.5*pv->WYS/pv->max_y);

	// draw pan elements, tip, ...
        pv->pan_area_extends->draw (cr); // full area of reach with offsets
	pv->pan_area->draw (cr);         // pan area

        if (pv->show_preset_grid)
                for(int i=0; i<N_PRESETS; ++i)
                        for(int j=0; j<N_PRESETS; ++j)
                                if (pv->pos_preset_box[i][j])
                                        pv->pos_preset_box[i][j]->draw (cr);

        if (pv->current_view)
                pv->current_view->draw (cr);     // current set scan area

        if (pv->Zratio > 2.){
                cairo_save (cr);
                cairo_scale (cr, 0.7*pv->Zratio, 0.7*pv->Zratio);
                if (pv->current_view)
                        pv->current_view->draw (cr, 0.3, false);     // current set scan area zoomed
                cairo_restore (cr);
                if (pv->tip_marker_zoom)
                        pv->tip_marker_zoom->draw (cr); // tip on zoomed view scan area
        }
        if (pv->tip_marker)
                pv->tip_marker->draw (cr); // absolute position

        cairo_restore (cr);

        // text in PIXEL coordinates (+/-WXS/2, +/-WYS/2), 0,0 is center.

	for (int i=0; i<16; ++i){
                if (pv->DSP_status_indicator[i])
                        pv->DSP_status_indicator[i]->draw (cr);
                if (pv->DSP_status_indicator_ID[i])
                        pv->DSP_status_indicator_ID[i]->draw (cr);
        }
        
	for (int i=0; i<16; ++i)
                if (pv->DSP_gpio_indicator[i])
                        pv->DSP_gpio_indicator[i]->draw (cr);
                else
                        break;

	if (pv->info){
                pv->info->draw (cr);
                pv->infoXY0->draw (cr);
        }
        
        cairo_scale (cr, 1., -1);
        if (pv->tip_marker_z0)
                pv->tip_marker_z0->draw (cr);
        if (pv->tip_marker_z)
                pv->tip_marker_z->draw (cr);

        return TRUE;
}
 

void PanView::start_tip_monitor (){
	if (!timer_id){
		PI_DEBUG (DBG_L1, "PanView::start_tip_monitor \n");
		finish (FALSE);
		timer_id = g_timeout_add (150, (GSourceFunc) PanView_tip_refresh_callback, this);
	}
}

void PanView::stop_tip_monitor (){
	if (timer_id){
		PI_DEBUG (DBG_L1, "PanView::stop_tip_monitor \n");
		finish (TRUE);
		timer_id = 0;
	}
	PI_DEBUG (DBG_L1, "PanView::stop_tip_monitor OK.\n");
}

void PanView::update_expanded_scan_limits (){
	// This should give the maximum voltage after the HV-Amp

	max_x =    xsmres.AnalogVMaxOut*main_get_gapp()->xsm->Inst->VX();
	min_x =   -xsmres.AnalogVMaxOut*main_get_gapp()->xsm->Inst->VX();
	max_y =	   xsmres.AnalogVMaxOut*main_get_gapp()->xsm->Inst->VY();
	min_y =   -xsmres.AnalogVMaxOut*main_get_gapp()->xsm->Inst->VY();
	max_z =	   xsmres.AnalogVMaxOut*main_get_gapp()->xsm->Inst->VZ();
	min_z =   -xsmres.AnalogVMaxOut*main_get_gapp()->xsm->Inst->VZ();

	xsr = xsmres.AnalogVMaxOut*main_get_gapp()->xsm->Inst->VX();
	x0r = xsmres.AnalogVMaxOut*main_get_gapp()->xsm->Inst->VX();
	ysr = xsmres.AnalogVMaxOut*main_get_gapp()->xsm->Inst->VY();
	y0r = xsmres.AnalogVMaxOut*main_get_gapp()->xsm->Inst->VY();
	zsr = xsmres.AnalogVMaxOut*main_get_gapp()->xsm->Inst->VZ();
	z0r = xsmres.AnalogVMaxOut*main_get_gapp()->xsm->Inst->VZ();

	// expand if analog offset adding is used 
	// --> but that's not all, more to do:
	// ** check scan range if rotated
	if (main_get_gapp()->xsm->Inst->OffsetMode() == OFM_ANALOG_OFFSET_ADDING){
		max_x +=    xsmres.AnalogVMaxOut*main_get_gapp()->xsm->Inst->VX0();
		min_x -=    xsmres.AnalogVMaxOut*main_get_gapp()->xsm->Inst->VX0();
		max_y +=    xsmres.AnalogVMaxOut*main_get_gapp()->xsm->Inst->VY0();
		min_y -=    xsmres.AnalogVMaxOut*main_get_gapp()->xsm->Inst->VY0();

		x0r = xsmres.AnalogVMaxOut*main_get_gapp()->xsm->Inst->VX0();
		y0r = xsmres.AnalogVMaxOut*main_get_gapp()->xsm->Inst->VY0();
		z0r = xsmres.AnalogVMaxOut*main_get_gapp()->xsm->Inst->VZ0();

		if (pan_area){
			pan_area->set_xy (0,  x0r,  y0r);
			pan_area->set_xy (1, -x0r, -y0r);
                        pan_area->set_line_width (get_lw (1.0));
                        pan_area->queue_update (canvas);
                }
        }
        if (pos_preset_box[0][0]){
                double pxw=(2*x0r)/N_PRESETS;
                double pyw=(2*y0r)/N_PRESETS;
                for(int i=0; i<N_PRESETS; ++i)
                        for(int j=0; j<N_PRESETS; ++j){
                                pos_preset_box[i][j]->set_xy (1, pxw,pyw);
                                pos_preset_box[i][j]->set_position (-x0r+pxw*i, -y0r+pyw*j);
                                pos_preset_box[i][j]->set_line_width (get_lw (1.0));
                                pos_preset_box[i][j]->queue_update (canvas);
                        }
        }
}

void PanView :: tip_refresh()
{
	// tip marker item
	//	GnomeCanvasPoints* tipmark_points = gnome_canvas_points_new (4);
	//	GnomeCanvasPoints* zoffset_points = gnome_canvas_points_new (2);
	double rsz = max_x/15.;
	double asp = 1.67;
	double x,y,z,q;
        double Ilg=0.;
	x=y=z=q=0.;

	if (!PanView_valid) return;

        // ** I'll fix this sub-routine to return the correct x/y/z volated including added offsets if used!
        if (main_get_gapp()->xsm->hardware)
                main_get_gapp()->xsm->hardware->RTQuery ("zxy", z, x, y); // get tip position in volts
        else 
                return;

        //g_print ("PanView zxy: %g %g %g\n", z,x,y);

	// example for new cairo_item code to replace the code below:
	if (tip_marker == NULL){
		tip_marker = new cairo_item_path_closed (4);
                tip_marker->set_stroke_rgba (CAIRO_COLOR_MAGENTA); // 1., finish () ? 0.:1., 0., 1.);
                tip_marker->set_fill_rgba (1., finish () ? 0.:1., 0., 1.);
	}
	// XY Scan&Offset position marker
	tip_marker->set_position (x, y);
	tip_marker->set_xy (0, -rsz/asp, +rsz);
	tip_marker->set_xy (1, +rsz/asp, -rsz);
	tip_marker->set_xy (2, -rsz/asp, -rsz);
	tip_marker->set_xy (3, +rsz/asp, +rsz);
        tip_marker->set_stroke_rgba (CAIRO_COLOR_MAGENTA); // 1., finish () ? 0.:1., 0., 1.);
        tip_marker->set_fill_rgba (1., finish () ? 0.:1., 0., 1.);
        tip_marker->set_line_width (get_lw (1.0));
	tip_marker->queue_update (canvas); // schedule update


        // Tip marker on zoomed scan area view w/o offset
	if (Zratio > 2.){
		rsz *= 1.7;

                // example for new cairo_item code to replace the code below:
                if (tip_marker_zoom == NULL){
                        tip_marker_zoom = new cairo_item_path_closed (4);
                        tip_marker_zoom->set_stroke_rgba (0.3, 0.3, 0.3, 0.5);
                        tip_marker_zoom->set_fill_rgba (finish ()? 0:3);
                }
                // XY Scan&Offset position marker  -- removed offset.
                tip_marker_zoom->set_position ((x-x_offset)*0.7*Zratio, (y-y_offset)*0.7*Zratio);
                tip_marker_zoom->set_xy (0, -rsz/asp, +rsz);
                tip_marker_zoom->set_xy (1, +rsz/asp, -rsz);
                tip_marker_zoom->set_xy (2, -rsz/asp, -rsz);
                tip_marker_zoom->set_xy (3, +rsz/asp, +rsz);
                tip_marker_zoom->set_fill_rgba (1., finish () ? 0.:1., 1., 1.);
                tip_marker_zoom->set_line_width (get_lw (1.0));
                tip_marker_zoom->show ();
        } else {
                if (tip_marker_zoom)
                        tip_marker_zoom->hide ();
        }
        if (tip_marker_zoom)
                tip_marker_zoom->queue_update (canvas); // schedule update


	// Z tip position marker
	rsz = WXS/2./15.;
	asp = 1.67;

	if (tip_marker_z == NULL){
		tip_marker_z = new cairo_item_path_closed (4);
                tip_marker_z->set_stroke_rgba (CAIRO_COLOR_YELLOW);
                tip_marker_z->set_fill_rgba (CAIRO_COLOR_YELLOW);
                tip_marker_z->set_line_width (get_lw (1.0));
	}
	// XY Scan&Offset position marker
	tip_marker_z->set_position (WXS/2., WYS/2.*z/max_z);
	tip_marker_z->set_xy (0, +rsz, -rsz/asp);
	tip_marker_z->set_xy (1, -rsz, +rsz/asp);
	tip_marker_z->set_xy (2, -rsz, -rsz/asp);
	tip_marker_z->set_xy (3, +rsz, +rsz/asp);
        if (fabs(z/max_z) < 0.8)
                tip_marker_z->set_stroke_rgba (CAIRO_COLOR_FORESTGREEN_ID, 0.8);
        else
                tip_marker_z->set_stroke_rgba (CAIRO_COLOR_RED);
        //tip_marker_z->set_fill_rgba (1., finish () ? 0.:1., 0., 1.);
	tip_marker_z->queue_update (canvas); // schedule update
	
	if (main_get_gapp()->xsm->hardware){
		double x0,y0,z0;
                x0=y0=z0=0.0;
		if (main_get_gapp()->xsm->hardware->RTQuery ("O", z0, x0, y0)){ // get HR Offset
                        //g_print ("PanView O: %g %g %g\n", z0,x0,y0);
			gchar *tmp = NULL;
			tmp = g_strdup_printf (//"Offset Z0: %7.3f " UTF8_ANGSTROEM // not used by RPSPMC
                                               "\nXY0: %7.3f " UTF8_ANGSTROEM
                                               ", %7.3f " UTF8_ANGSTROEM
					       "\nXYs: %7.3f " UTF8_ANGSTROEM
                                               ", %7.3f " UTF8_ANGSTROEM,
					       //main_get_gapp()->xsm->Inst->V2ZAng(z0),
					       main_get_gapp()->xsm->Inst->V2XAng(x0),
					       main_get_gapp()->xsm->Inst->V2YAng(y0),
					       main_get_gapp()->xsm->Inst->V2XAng(x),
					       main_get_gapp()->xsm->Inst->V2YAng(y));
                        infoXY0->set_text (tmp);
                        infoXY0->queue_update (canvas);
			g_free (tmp);

			// Z0 position marker
			rsz = WXS/2./10.;
                        if (tip_marker_z0 == NULL){
                                tip_marker_z0 = new cairo_item_path (2);
                                tip_marker_z0->set_xy (0, +rsz, 0.);
                                tip_marker_z0->set_xy (1, -rsz, 0.);
                                tip_marker_z0->set_fill_rgba (CAIRO_COLOR_BLUE_ID, 0.8);
                                tip_marker_z0->set_stroke_rgba (CAIRO_COLOR_BLUE_ID, 0.8);
                                tip_marker_z0->set_line_width (get_lw (3.0));
                        }
                        // XY Scan&Offset position marker
                        tip_marker_z0->set_position (WXS/2., WYS/2.*z0/z0r);
                        tip_marker_z0->set_line_width ( WYS/2.*0.03);
                        if (fabs(z/z0r) < 0.75)
                                tip_marker_z0->set_stroke_rgba (CAIRO_COLOR_BLUE_ID, 0.8);
                        else
                                tip_marker_z0->set_stroke_rgba (CAIRO_COLOR_RED);
                        tip_marker_z0->queue_update (canvas); // schedule update
		}

                // Life Paramater Info
                x=y=q=0.0;
		main_get_gapp()->xsm->hardware->RTQuery ("f0I", x, y, q); // get f0, I -- val1,2,3=[fo,Iav,Irms]
                //g_print ("PanView f0I: %g %g %g\n", x,y,q);
                Ilg = log10 (fabs(y) + 1.0);
		
		gchar *tmp = NULL;

                double u,v,w;
		main_get_gapp()->xsm->hardware->RTQuery ("B", u, v, w); // Bias Monitor, Bias Reg, Bias Set
                double v1,v2,vdum;
		main_get_gapp()->xsm->hardware->RTQuery ("V", v1, v2, vdum); // Volatges IN3, IN4
                double gu,ga,gb;
		main_get_gapp()->xsm->hardware->RTQuery ("G", gu, ga, gb); // GVP
                double gamc,gfmc,gdum;
		main_get_gapp()->xsm->hardware->RTQuery ("F", gamc, gfmc, gdum); // GVP-AMC, FMC
                double s1,s2,s3;
                main_get_gapp()->xsm->hardware->RTQuery ("S", s1, s2, s3); // Status
                if (fabs(y) < 0.25)
                        tmp = g_strdup_printf ("I: %8.1f pA\ndF: %8.1f Hz\nZ: %8.4f " UTF8_ANGSTROEM
                                               "\nBias: %8.4f V"// %8.4f V %8.4f V"
                                               //"\n UMon: %g USet: %g"
                                               //"\nGVPU: %6.4f A: %6.4f B: %6.4f V"
                                               //"\nGVPAM: %6.4f FM: %6.4f Veq\n"
                                               "\nIN3: %8.4f IN4: %8.4f",
                                               //"\nZSM: h%04x S%02x G%02x",
                                               y*1000., x, main_get_gapp()->xsm->Inst->V2ZAng(z),
                                               u,//v,w,
                                               v1,v2//,
                                               //gu, ga, gb,
                                               //gamc, gfmc,
                                               //q, (int)s2, (int)s3
                                               );
                else
                        tmp = g_strdup_printf ("I: %8.4f nA\ndF: %8.1f Hz\nZ: %8.4f " UTF8_ANGSTROEM
                                               "\nBias: %8.4f V"// %8.4f V %8.4f V"
                                               //"\n %g %g"
                                               //"\nGVPU: %6.4f A: %6.4f B: %6.4f V"
                                               //"\nGVPAM: %6.4f FM: %6.4f Veq\n"
                                               "\nIN3: %8.4f IN4: %8.4f",
                                               //"\nZSM: h%04x %02x",
                                               y,       x, main_get_gapp()->xsm->Inst->V2ZAng(z),
                                               u,//v,w,
                                               v1,v2//,
                                               //gu, ga, gb,
                                               //gamc, gfmc,
                                               //q, (int)s2, (int)s3
                                               );

                info->set_text (tmp);
                info->queue_update (canvas);

		g_free (tmp);

		// DSP RT Status update
                x=y=q=0.0;
		main_get_gapp()->xsm->hardware->RTQuery ("status", x, y, q); // status code in x
                //g_print ("PanView status: %g %g %g\n", x,y,q);
		int status = (int)(x);

                static int status_id[]      = { DSP_STATUSIND_FB, DSP_STATUSIND_SCAN, DSP_STATUSIND_VP, DSP_STATUSIND_MOV,
                                                DSP_STATUSIND_PLL, DSP_STATUSIND_ZPADJ, DSP_STATUSIND_AAP,
                                                -1 };
                static int status_bm_a[]     = { 1, 2, 8, 16,  32, 64, 128, -1 };
                static int status_on_color[] = { CAIRO_COLOR_GREEN_ID, CAIRO_COLOR_GREEN_ID, CAIRO_COLOR_GREEN_ID, CAIRO_COLOR_GREEN_ID,
                                                 CAIRO_COLOR_YELLOW_ID, CAIRO_COLOR_GREEN_ID, CAIRO_COLOR_ORANGE_ID, CAIRO_COLOR_GREEN_ID,
                                                 0
                };
                static int status_bm_b[]     = { 0, 4, 0x1000,  0x1000,
                                                 0x1000, 0x1000, 0x1000,
                                                 -1 };

                main_get_gapp()->set_dsp_scan_in_progress (status & 6 ? true : false);

                // Servo (FB), GVP, Hold (Pause), Move
                const gchar *indicator_id[16] = { "Z","G","P","R",
                                                  "F","H","h","",
                                                  "","","","",
                                                  "","","","" };
                for (int i=0; status_id[i]>=0; ++i){
                        const double w=WXS/2./12.;
                    	if (DSP_status_indicator[status_id[i]] == NULL){
                                DSP_status_indicator[status_id[i]] = new cairo_item_rectangle (0.,0.,w, 2.*w);
                                DSP_status_indicator[status_id[i]]->set_position (-WXS/2+i*w*1.05, -WYS/2.);
                                DSP_status_indicator[status_id[i]]->set_stroke_rgba (CAIRO_COLOR_WHITE);
                                DSP_status_indicator[status_id[i]]->set_line_width (get_lw (0.5));
                                DSP_status_indicator_ID[i] = new cairo_item_text (-WXS/2+i*w*1.05+w/2, -WYS/2.+w, indicator_id[i]);
                                DSP_status_indicator_ID[i]->set_font_face_size ("Unutu", 7);
                                DSP_status_indicator_ID[i]->set_angle (90);
                                DSP_status_indicator_ID[i]->set_stroke_rgba (CAIRO_COLOR_WHITE);
                        }
                        if (status &  status_bm_a[i])
                                DSP_status_indicator[status_id[i]]->set_fill_rgba (CAIRO_BASIC_COLOR (status_on_color[i]));
                        else
                                if (status_bm_b[i])
                                        if (status &  status_bm_b[i])
                                                DSP_status_indicator[status_id[i]]->set_fill_rgba (CAIRO_COLOR_ORANGE);
                                        else
                                                DSP_status_indicator[status_id[i]]->set_fill_rgba (CAIRO_COLOR_BLACK);
                                else
                                        DSP_status_indicator[status_id[i]]->set_fill_rgba (CAIRO_COLOR_RED);
                }

                // Act Load and peak:
		if (DSP_status_indicator[DSP_STATUSIND_LOAD] == NULL){
                        const double w=WXS/2./20.;
                        DSP_status_indicator[DSP_STATUSIND_LOAD] = new cairo_item_rectangle (0.,0.,w, w/2);
                        DSP_status_indicator[DSP_STATUSIND_LOAD]->set_line_width (get_lw (1.0));
                }
                DSP_status_indicator[DSP_STATUSIND_LOAD]->set_position (-WXS/2, WYS/2.-WYS*q);
                DSP_status_indicator[DSP_STATUSIND_LOAD]->set_stroke_rgba ((q>0.85) ? CAIRO_COLOR_RED : CAIRO_COLOR_MAGENTA);
                DSP_status_indicator[DSP_STATUSIND_LOAD]->set_fill_rgba ((q>0.85) ? CAIRO_COLOR_RED : CAIRO_COLOR_MAGENTA);

		if (DSP_status_indicator[DSP_STATUSIND_LOADP] == NULL){
                        const double w=WXS/2./24.;
                        DSP_status_indicator[DSP_STATUSIND_LOADP] = new cairo_item_rectangle (0.,0.,w, w/2);
                        DSP_status_indicator[DSP_STATUSIND_LOADP]->set_line_width (get_lw (1.0));
                }
                DSP_status_indicator[DSP_STATUSIND_LOADP]->set_position (-WXS/2, WYS/2.-WYS*y);
                DSP_status_indicator[DSP_STATUSIND_LOADP]->set_stroke_rgba ((y>0.85) ? CAIRO_COLOR_RED : CAIRO_COLOR_MAGENTA);
                DSP_status_indicator[DSP_STATUSIND_LOADP]->set_fill_rgba ((y>0.85) ? CAIRO_COLOR_RED : CAIRO_COLOR_MAGENTA);

                // Pseudo Current Log Scale Bar: 3 decade by color green, blue red
                y=Ilg;
                if (Ilg < 0.01)
                        y *= 100.;
                else if (Ilg < 0.1)
                        y *= 10.;
                
		if (DSP_status_indicator[DSP_STATUSIND_CURRENT] == NULL){
                        const double w=WXS/2./24.;
                        DSP_status_indicator[DSP_STATUSIND_CURRENT] = new cairo_item_rectangle (0.,0.,w, -y*WYS);
                        DSP_status_indicator[DSP_STATUSIND_CURRENT]->set_position (-WXS/2, WYS/2.);
                        DSP_status_indicator[DSP_STATUSIND_CURRENT]->set_line_width (get_lw (1.0));
		}
                DSP_status_indicator[DSP_STATUSIND_CURRENT]->set_xy (1, WXS/2./24., -y*WYS);
                DSP_status_indicator[DSP_STATUSIND_CURRENT]->set_stroke_rgba ((Ilg<0.01) ?
                                                                              CAIRO_COLOR_BLUE : (Ilg < 0.1) ?
                                                                              CAIRO_COLOR_GREEN : CAIRO_COLOR_RED);
                DSP_status_indicator[DSP_STATUSIND_CURRENT]->set_fill_rgba ((Ilg<0.01) ?
                                                                            CAIRO_COLOR_BLUE : (Ilg < 0.1) ?
                                                                            CAIRO_COLOR_GREEN : CAIRO_COLOR_RED);
		// DSP RT GPIO update
                x=y=q=0.0;
		main_get_gapp()->xsm->hardware->RTQuery ("iomirror", x, y, q); // status code in x
                //g_print ("PanView iomonitor: %g %g %g\n", x,y,q);
		int gpio_out = (int)(x);
		int gpio_in  = (int)(y);
		int gpio_dir = (int)(q);

		for (int i=0; i<16; ++i){
                        const double w=WXS/2./12.;
			if (DSP_gpio_indicator[i] == NULL){
                                DSP_gpio_indicator[i] = new cairo_item_rectangle (0.,0.,w, 0.7*w);
                                DSP_gpio_indicator[i]->set_position (WXS/2-(i+1)*w*1.05, WYS/2.+1.);
                                DSP_gpio_indicator[i]->set_stroke_rgba (1.,1.,1.,1.);
                                DSP_gpio_indicator[i]->set_line_width (get_lw (0.5));
                        }
                        DSP_gpio_indicator[i]->set_fill_rgba ( (gpio_dir & (1<<i)) ? 
                                                               ( (gpio_out & (1<<i)) ? CAIRO_COLOR_RED : CAIRO_COLOR_BLACK)
                                                               :
                                                               ( (gpio_in  & (1<<i)) ? CAIRO_COLOR_GREEN : CAIRO_COLOR_WHITE));
                }
	}
	else 
		return;

	return;
}

void PanView :: refresh()
{
	static gboolean prev_error = false;
	gboolean error = false;
	double alpha;
	int 	i; 

	if (!PanView_valid) return;

	/* update system parameters */
	update_expanded_scan_limits ();

	PI_DEBUG (DBG_L2, min_x<<"<X<"<<max_x );
	PI_DEBUG (DBG_L2, min_y<<"<Y<"<<max_y );

	/*fill in the corners of the scanning area*/
	if (IS_SPALEED_CTRL||xsmres.ScanOrgCenter){ // Origin is middle of scan
		point[0][0] = -0.5*main_get_gapp()->xsm->data.s.nx*main_get_gapp()->xsm->data.s.dx/xsmres.XPiezoAV;	
		point[0][1] = +0.5*main_get_gapp()->xsm->data.s.ny*main_get_gapp()->xsm->data.s.dy/xsmres.YPiezoAV;
		 
		point[1][0] = -1*point[0][0];        point[1][1] = +1*point[0][1];
		point[2][0] = -1*point[0][0];        point[2][1] = -1*point[0][1];
		point[3][0] = +1*point[0][0];        point[3][1] = -1*point[0][1];
	}
	else{                                       // Origin is center of top line
		point[0][0] =-0.5*main_get_gapp()->xsm->data.s.nx*main_get_gapp()->xsm->data.s.dx/xsmres.XPiezoAV;	point[0][1] = 0;
		point[1][0] = -1*point[0][0];        point[1][1] = 0;
		point[2][0] = -1*point[0][0];        point[2][1] = -main_get_gapp()->xsm->data.s.ny*main_get_gapp()->xsm->data.s.dy/xsmres.YPiezoAV;
		point[3][0] = +1*point[0][0];        point[3][1] = +1*point[2][1];
	}
	PI_DEBUG (DBG_L2, "Original coordinates: \t"<< 
		  point[0][0]<<","<<point[0][1]<<"\t"<<
		  point[1][0]<<","<<point[1][1]<<"\t"<<
		  point[2][0]<<","<<point[2][1]<<"\t"<<
		  point[3][0]<<","<<point[3][1] );

	/*read the parameters from the control*/
	y_offset = main_get_gapp()->xsm->data.s.y0/xsmres.YPiezoAV;
	x_offset = main_get_gapp()->xsm->data.s.x0/xsmres.XPiezoAV;
	alpha	= main_get_gapp()->xsm->data.s.alpha;	

#if PRE_AREA
	/* this are the edges of the scan area plus the prescan */
	double w = 2*main_get_gapp()->xsm->data.s.dx*main_get_gapp()->xsm->data.hardpars.LS_nx_pre/xsmres.XPiezoAV;
	pre_point[0][0] = point[0][0]-w; pre_point[0][1] = point [0][1];
	pre_point[1][0] = point[1][0]+w; pre_point[1][1] = point [1][1];
	pre_point[2][0] = point[2][0]+w; pre_point[2][1] = point [2][1];
	pre_point[3][0] = point[3][0]-w; pre_point[3][1] = point [3][1];
#endif

	/*Apply the current offset and rotation to prescan area*/
	for(i=0;i<4;i++){
		transform(max_corn[i], point[i], -alpha, y_offset,x_offset);
                //		transform(max_corn[i], point[i], -alpha, y_offset, x_offset);
#if PRE_AREA
		transform(max_corn[i],pre_point[i],-alpha,y_offset,x_offset);
#endif
	}
	
	/*check whether our scanning area (extended by the prescan) is within the limits*/	
        for (i=0; i < 4; i++){
		if (max_corn[i][0] >= max_x || max_corn[i][0] <= min_x){
                        error = true;
                        break;
                }
		
		if (max_corn[i][1] >= max_y || max_corn[i][1] <= min_y){
                        error = true;
                        break;
                }
	}
	
#if 0 // ?????
	if(max_corn[0][0] == corn_oo && max_corn[3][1] == corn_ii)
		error = false;
	
	corn_oo = max_corn[0][0];
	corn_ii = max_corn[3][1];
#endif

#if PRE_AREA

	/*redraw the PanView prescan area */
        pre_scanarea_points = gnome_canvas_points_new (4);
	{
                int j;
                for(i=j=0;i < 4;i++){
                        pre_scanarea_points->coords[j]   = TO_CANVAS_X(max_corn[i][0]);
                        pre_scanarea_points->coords[j+1] = TO_CANVAS_Y(max_corn[i][1]);
                        j+=2;
                }
  	}
	if (pre_current_view)
		gnome_canvas_item_set (pre_current_view, 
				       "points", pre_scanarea_points, 
				       "outline_color", "black", 
				       "fill_color","white",
				       NULL );
	else
		pre_current_view = gnome_canvas_item_new( gnome_canvas_root (GNOME_CANVAS (canvas)), 
							  gnome_canvas_polygon_get_type(),
							  "points", pre_scanarea_points,
							  "fill_color", "white",
							  "outline_color", "black",
							  "width_units", 1.5,
							  NULL );
	
	gnome_canvas_points_free (pre_scanarea_points);
#endif

	/*Apply the current offset and rotation to scan area -- offset: y_offset,x_offset is set to object now*/
	for(i=0;i<4;i++)
		transform(max_corn[i],point[i],-alpha);

	PI_DEBUG (DBG_L2, "Transformed coordinates: \t"<<
		  max_corn[0][0]<<","<<max_corn[0][1]<<"\t"<<
		  max_corn[1][0]<<","<<max_corn[1][1]<<"\t"<<
		  max_corn[2][0]<<","<<max_corn[2][1]<<"\t"<<
		  max_corn[3][0]<<","<<max_corn[3][1] );

	Zratio = fabs (max_x - min_x) / (fabs (max_corn[0][0]-max_corn[1][0]) + fabs (max_corn[1][0]-max_corn[2][0]));

        // update scan area
        if (!current_view)
                current_view = new cairo_item_path_closed (4);
        //current_view->set_stroke_rgba (1.,0.,0.,1.);
        //current_view->set_fill_rgba   (1.,0.,0., 0.8);
        if (error)
                current_view->set_stroke_rgba   (1.,0.,0., 0.8);
        else
                current_view->set_stroke_rgba   (.2,.2,.2, 0.8);
        current_view->set_line_width (get_lw (1.0));
        current_view->set_position (x_offset, y_offset);
	for(i=0; i < 4; ++i)
                current_view->set_xy (i, max_corn[i][0], max_corn[i][1]);

        current_view->queue_update (canvas);
                
	if(error && !prev_error){
		prev_error = error;
//*		warning ("Scanning area is out of limits!");
		PI_DEBUG(DBG_L2, "Error: Scanning area is out of limits!" );
	}
	prev_error = error;
}

void PanView :: transform(double *dest, double *src, double rot, double y_off, double x_off)
{
	double D[2][2];
	int i,j;	
	
	D[0][0] =   cos(TO_RAD(rot));
	D[1][1] =   cos(TO_RAD(rot));
	D[0][1] =  -sin(TO_RAD(rot));
	D[1][0] =   sin(TO_RAD(rot));

	dest[0] = 0;
	dest[1] = 0;
	
	for(i=0; i<2 ; i++)			
		for(j=0;j<2;j++)
			dest[i] += D[i][j]*src[j];

	dest[0] += x_off;
	dest[1] += y_off;
	return;
}

////////////////////////////////////////
// ENDE PAN VIEW CONTROL ///////////////
////////////////////////////////////////
