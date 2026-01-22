/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 8 c-style: "K&R" -*- */

/* Gnome gxsm - Gnome X Scanning Microscopy
 * universal STM/AFM/SARLS/SPALEED/... controlling and
 * data analysis software
 * 
 * Gxsm Plugin Name: ProbeIndicator.C
 * ========================================
 * 
 * Copyright (C) 2018 The Free Software Foundation
 *
 * Authors: Percy Zahl <zahl@users.sf.net>
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
% PlugInDocuCaption: Probe Indicator

% PlugInName: ProbeIndicator
% PlugInAuthor: Percy Zahl
% PlugInAuthorEmail: zahl@users.sf.net
% PlugInMenuPath: Tools/Probe Indicator
% PlugInDescription
The Probe Indicator (Note: head up display (HUD) as future option was designed to be overlayed on any scans canvas) 
is intended to give a real time feedback and provide dedicated monitoring of the tip/probe conditions. 
Visually indicating probe postion (Z), current and frequency.
It also provides a contineous rolling graphical view of Z and current plus a (work in progress) spectral analysis.
Plus a recording button allows to gapless record two channels (Current, Z,...) as selected for the Recorder Signals1 and 2 inputs.
You will have to use currently the pyton spm control app/Oszi if you like to change the default channels used here.

\begin{figure}[hbt]
\center { \fighalf{ProbeIndicator}}
\caption{The ProbeIndicator window explained.}
\label{fig:ProbeIndicator}
\end{figure}


% PlugInUsage
Although this is a plugin it is opened automatically upon startup of GXSM automatically, you will need to open first time via Main menu Window/Probe-Hud.

Please have a look for a demo:

\GxsmVideoURL{https://youtu.be/eB1FO76M7gI}

\GxsmVideoURL{https://youtu.be/vTrldyKxrZ4}


\GxsmNote{not included: a signal selector for Signal 1 and 2 (recorder). 
Use the spm control python app, oscilloscope and setup teh channels there: 
Select "MIX IN 0" for CH1 and "Z Servo Neg" for CH2.}

What the recorder does excatly:

It records 2x 4096 data points (scope channel) at full BW, they are plotted in a 4096 to 128 decimated graph.

And it records in a constant gap less continuous stream a 1:256 decimated data stream for both signals.

Checking the [o] record button this data is (for ever, until you
delete it) appended into two files located in the working directory
(where you start gxsm4). It writes away plain integer numbers and
occasional (every 60s) a line starting with a double comment \#\# nnnn
absolute system time stamp in us. And for every read block a index \# $n \dots m$ range.
That may be used to precisely align both data sets in two
individual files. Sample rate is exactly 150kHz/256.

Using the "Magnifying Button" (Zoom) you can choose to plot at FULL BW the first 128 points.


The Signal Button turns the scope on.

The Info Button toggles some text info overlay.

The [x] Button (left) selects FULL BW FFT display.


Scope scaling is fully automatic...

FFT scale in double log dB(Freq).... but needs still some optimizations. FFT is run either on decimated 1:256 or FULL BW data.

Also it shows a symbolic tip... indication the actual Z-position, up it tip "up".


And a logarithmic current scale bar polar graph and indicators on the outside. Bottom is Zero, 1st tic is 10p, then 100p, ... left / right = pos/neg

The markers and short polar indicator at the position indicated the current absolute min/max (low pass filtered) for the current as read by the scope!



% OptPlugInNotes
MK3 only.

% OptPlugInHints


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

#include "ProbeIndicator.h"

#define UTF8_ANGSTROEM "\303\205"
#define UTF8_DEGREE "\302\260"

// Plugin Prototypes
static void ProbeIndicator_init (void);
static void ProbeIndicator_query (void);
static void ProbeIndicator_about (void);
static void ProbeIndicator_configure (void);
static void ProbeIndicator_cleanup (void);

//extern DSPPACControl *DSPPACClass;


ProbeIndicator *HUD_Window = NULL;
gboolean refresh_function(GtkWidget *w, GdkEvent *event, void *data);

gboolean ProbeIndicator_valid = FALSE;


static void ProbeIndicator_show_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data);

      // Fill in the GxsmPlugin Description here
GxsmPlugin ProbeIndicator_pi = {
	  NULL,                   // filled in and used by Gxsm, don't touch !
	  NULL,                   // filled in and used by Gxsm, don't touch !
	  0,                      // filled in and used by Gxsm, don't touch !
	  NULL,                   // The Gxsm-App Class Ref.pointer (called "gapp" in Gxsm) is
	  // filled in here by Gxsm on Plugin load,
	  // just after init() is called !!!
	  // ----------------------------------------------------------------------
	  // Plugins Name, CodeStly is like: Name-M1S[ND]|M2S-BG|F1D|F2D|ST|TR|Misc
	  (char *)"Probe Indicator",
	  NULL,
	  // Description, is shown by PluginViewer (Plugin: listplugin, Tools->Plugin Details)
	  (char *)"Scan Area visualization and life SPM parameter readings",
	  // Author(s)
	  (char *) "Percy Zahl",
	  // Menupath to position where it is appended to
	  (char *)"windows-section",
	  // Menuentry
	  N_("Probe Indicator"),
	  // help text shown on menu
	  N_("Probe Status visualization."),
	  // more info...
	  (char *)"See Manual.",
	  NULL,          // error msg, plugin may put error status msg here later
	  NULL,          // Plugin Status, managed by Gxsm, plugin may manipulate it too
	  // init-function pointer, can be "NULL",
	  // called if present at plugin load
	  ProbeIndicator_init,
	  // query-function pointer, can be "NULL",
	  // called if present after plugin init to let plugin manage it install itself
	  ProbeIndicator_query, // query should be "NULL" for Gxsm-Math-Plugin !!!
	  // about-function, can be "NULL"
	  // can be called by "Plugin Details"
	  ProbeIndicator_about,
	  // configure-function, can be "NULL"
	  // can be called by "Plugin Details"
	  ProbeIndicator_configure,
	  // run-function, can be "NULL", if non-Zero and no query defined,
	  // it is called on menupath->"plugin"
	  NULL, // run should be "NULL" for Gxsm-Math-Plugin !!!
	  // cleanup-function, can be "NULL"
	  // called if present at plugin removal
          ProbeIndicator_show_callback, // direct menu entry callback1 or NULL
          NULL, // direct menu entry callback2 or NULL

	  ProbeIndicator_cleanup
};

//
//Text used in the About Box
//
static const char *about_text = N_("HUD style probe status indicator");
                                   
	
//
// Symbol "get_gxsm_plugin_info" is resolved by dlsym from Gxsm, used to get Plugin's info!! 
// Essential Plugin Function!!
//
GxsmPlugin *get_gxsm_plugin_info ( void ){ 
	ProbeIndicator_pi.description = g_strdup_printf(N_("Gxsm HUD probe indicator window %s"), VERSION);
	return &ProbeIndicator_pi; 
}

//
// init-Function
//

static void ProbeIndicator_show_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data){
        PI_DEBUG (DBG_L2, "ProbeIndicator_show_callback" );
	if( HUD_Window ){
                HUD_Window->show();
		HUD_Window->start();
	}
}

static void ProbeIndicator_refresh_callback (gpointer data){
	if (HUD_Window)
		HUD_Window->refresh ();
}

static void ProbeIndicator_init(void)
{
	PI_DEBUG(DBG_L2, "ProbeIndicator Plugin Init" );
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

static void ProbeIndicator_query(void)
{
        PI_DEBUG (DBG_L2, "ProbeIndicator_query" );

	if(ProbeIndicator_pi.status) g_free(ProbeIndicator_pi.status); 
	ProbeIndicator_pi.status = g_strconcat (
		N_("Plugin query has attached, status:"),
		(xsmres.HardwareType[0] == 'n' && xsmres.HardwareType[1] == 'o')?"Offline (no hardware)":"Online",
		ProbeIndicator_pi.name, 
		NULL);

#if 1 // disable auto terminate with no real hardware
	if (xsmres.HardwareType[0] == 'n' && xsmres.HardwareType[1] == 'o'){
                HUD_Window = NULL;
		return;
        }
#endif
        
	HUD_Window = new ProbeIndicator (main_get_gapp() -> get_app ()); // ProbeIndicator(ProbeIndicator_pi.app->getApp());
	ProbeIndicator_pi.app->ConnectPluginToSPMRangeEvent (ProbeIndicator_refresh_callback);
	HUD_Window->start ();

// not yet needed
//	ProbeIndicator_pi.app->ConnectPluginToCDFSaveEvent (ProbeIndicator_SaveValues_callback);

	PI_DEBUG (DBG_L2, "ProbeIndicator_query:done" );
}

//
// about-Function
//
static void ProbeIndicator_about(void)
{
	const gchar *authors[] = { ProbeIndicator_pi.authors, NULL};
	gtk_show_about_dialog (NULL, 
			       "program-name",  ProbeIndicator_pi.name,
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
static void ProbeIndicator_configure(void)
{
        PI_DEBUG (DBG_L2, "ProbeIndicator_configure" );
	if(ProbeIndicator_pi.app)
		ProbeIndicator_pi.app->message("ProbeIndicator Plugin Configuration");

	if (HUD_Window){
                HUD_Window->start ();
	}
}

//
// cleanup-Function, make sure the Menustrings are matching those above!!!
//
static void ProbeIndicator_cleanup(void)
{
	ProbeIndicator_valid = FALSE;

	PI_DEBUG(DBG_L2, "ProbeIndicator_cleanup(void). Entering.\n");
	if (HUD_Window) {
		delete HUD_Window;
	}
	PI_DEBUG(DBG_L2, "ProbeIndicator_cleanup(void). Exiting.\n");
}

static gint ProbeIndicator_tip_refresh_callback (ProbeIndicator *pv){
	if (!main_get_gapp()->xsm->hardware)
		return TRUE;

	if (main_get_gapp()->xsm->hardware->IsSuspendWatches ())
		return TRUE;

	if (pv){
                return pv->refresh ();
	}
	return FALSE;
}


void ProbeIndicator::close_callback (GtkWidget *widget, gpointer user_data) {
        ProbeIndicator *pv = (ProbeIndicator *) user_data; 
        g_print ("ProbeIndicator::close_callback TB: %d\n", gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget)));

        if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget)))
                pv->modes = (pv->modes & ~SCOPE_DBG) | SCOPE_DBG;
        else
                pv->modes &= ~SCOPE_DBG;
}

void ProbeIndicator::shutdown_callback (GtkWidget *widget, gpointer user_data) {
        //ProbeIndicator *pv = (ProbeIndicator *) user_data; 
        g_print ("ProbeIndicator::shutdown_callback TB: %d\n", gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget)));
}

void ProbeIndicator::run_scope_callback (GtkWidget *widget, gpointer user_data) {
        ProbeIndicator *pv = (ProbeIndicator *) user_data; 
        //g_print ("ProbeIndicator::run_scope_callback TB: %d\n", gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget)));
        if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget)))
                pv->modes = (pv->modes & ~SCOPE_ON) | SCOPE_ON;
        else
                pv->modes &= ~SCOPE_ON;
}

void ProbeIndicator::zoom_scope_callback (GtkWidget *widget, gpointer user_data) {
        ProbeIndicator *pv = (ProbeIndicator *) user_data; 
        //g_print ("ProbeIndicator::zoom_scope_callback TB: %d\n", gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget)));
        if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget)))
                pv->modes = (pv->modes & ~SCOPE_ZOOM) | SCOPE_ZOOM;
        else
                pv->modes &= ~SCOPE_ZOOM;
}

void ProbeIndicator::scope_ftfast_callback (GtkWidget *widget, gpointer user_data) {
        ProbeIndicator *pv = (ProbeIndicator *) user_data; 
        //g_print ("ProbeIndicator::zoom_scope_callback TB: %d\n", gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget)));
        if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget)))
                pv->modes = (pv->modes & ~SCOPE_FTFAST) | SCOPE_FTFAST;
        else
                pv->modes &= ~SCOPE_FTFAST;
}

void ProbeIndicator::record_callback (GtkWidget *widget, gpointer user_data) {
        ProbeIndicator *pv = (ProbeIndicator *) user_data; 
        //g_print ("ProbeIndicator:record_:scope_callback TB: %d\n", gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget)));
        if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget)))
                pv->modes = (pv->modes & ~SCOPE_RECORD) | SCOPE_RECORD;
        else
                pv->modes &= ~SCOPE_RECORD;
}

void ProbeIndicator::pause_callback (GtkWidget *widget, gpointer user_data) {
        ProbeIndicator *pv = (ProbeIndicator *) user_data; 
        //g_print ("ProbeIndicator::pause_callback TB: %d\n", gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget)));
        if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget))){
                pv->modes = (pv->modes & ~SCOPE_PAUSE) | SCOPE_PAUSE;
                pv->stop ();
        } else {
                pv->modes &= ~SCOPE_PAUSE;
                pv->start ();
        }
}

void ProbeIndicator::info_callback (GtkWidget *widget, gpointer user_data) {
        ProbeIndicator *pv = (ProbeIndicator *) user_data; 
        //g_print ("ProbeIndicator::info_callback TB: %d\n", gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget)));
        if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget)))
                pv->modes = (pv->modes & ~SCOPE_INFO) | SCOPE_INFO;
        else
                pv->modes &= ~SCOPE_INFO;
}

void ProbeIndicator::more_info_callback (GtkWidget *widget, gpointer user_data) {
        ProbeIndicator *pv = (ProbeIndicator *) user_data; 
        //g_print ("ProbeIndicator::more_info_callback TB: %d\n", gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget)));
        if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget)))
                pv->modes = (pv->modes & ~SCOPE_INFOPLUS) | SCOPE_INFOPLUS;
        else
                pv->modes &= ~SCOPE_INFOPLUS;
}

void ProbeIndicator::less_info_callback (GtkWidget *widget, gpointer user_data) {
        ProbeIndicator *pv = (ProbeIndicator *) user_data; 
        //g_print ("ProbeIndicator::less_info_callback TB: %d\n", gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget)));
        if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget)))
                pv->modes = (pv->modes & ~SCOPE_INFOMINUS) | SCOPE_INFOMINUS;
        else
                pv->modes &= ~SCOPE_INFOMINUS;
}

#if 0
GtkWidget *ProbeIndicator::signal_input_signal_options (gint channel, gint preset, gpointer ref){
        GtkWidget *cbtxt = gtk_combo_box_text_new (); 
        g_object_set_data(G_OBJECT (cbtxt), "signal_channel", GINT_TO_POINTER (channel)); 
#if 0
        if ( !strcmp (sranger_common_hwi->lookup_dsp_signal_managed (jj)->module, "Analog_IN") 
             || !strcmp (sranger_common_hwi->lookup_dsp_signal_managed (jj)->module, "Control") 
             || !strcmp (sranger_common_hwi->lookup_dsp_signal_managed (jj)->module, "Counter") 
             || !strcmp (sranger_common_hwi->lookup_dsp_signal_managed (jj)->module, "Mixer") 
             || !strcmp (sranger_common_hwi->lookup_dsp_signal_managed (jj)->module, "LockIn") 
             || !strcmp (sranger_common_hwi->lookup_dsp_signal_managed (jj)->module, "PAC")){
        }
#endif
        for (int jj=0;  jj<NUM_SIGNALS_UNIVERSAL && sranger_common_hwi->lookup_dsp_signal_managed (jj)->p; ++jj){ 
                gchar *id = g_strdup_printf ("%d", jj);
                const gchar *label = sranger_common_hwi->lookup_dsp_signal_managed (jj)->label;
                gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (cbtxt), id, label?label:"???");
                g_free (id);
        } 
        gtk_combo_box_set_active (GTK_COMBO_BOX (cbtxt), preset); 
        g_signal_connect (G_OBJECT (cbtxt), "changed",	
                          G_CALLBACK (DSPControl::choice_lcksource_callback), 
                          ref);				
        grid_add_widget (cbtxt);
        return cbtxt;
};
#endif


ProbeIndicator::ProbeIndicator (Gxsm4app *app):AppBase(app){ 
        hud_size = 150;
	timer_id = 0;
	probe = NULL;
        modes = SCOPE_NONE;

	AppWindowInit (N_("HUD Probe Indicator"));

	canvas = gtk_drawing_area_new(); // make a drawing area

        gtk_drawing_area_set_content_width  (GTK_DRAWING_AREA (canvas), 2*hud_size);
        gtk_drawing_area_set_content_height (GTK_DRAWING_AREA (canvas), 2*hud_size);

        gtk_widget_set_hexpand(GTK_WIDGET (canvas), true);
        gtk_widget_set_vexpand(GTK_WIDGET (canvas), true);

        gtk_widget_set_halign(v_grid, GTK_ALIGN_FILL); // Make grid fill horizontally
        gtk_widget_set_valign(v_grid, GTK_ALIGN_FILL); // Make grid fill vertically
        
        gtk_drawing_area_set_draw_func (GTK_DRAWING_AREA (canvas),
                                        G_CALLBACK (ProbeIndicator::canvas_draw_function),
                                        this, NULL);
		
	gtk_widget_show (canvas);
	gtk_grid_attach (GTK_GRID (v_grid), canvas, 1,1, 40,40);

        GtkWidget *tb = gtk_toggle_button_new ();
        gtk_button_set_icon_name (GTK_BUTTON (tb), "utilities-system-monitor-symbolic");
        gtk_widget_show (tb);
        gtk_widget_set_name (tb, "probe-indicator-button"); // name used by CSS to apply custom color scheme
	gtk_widget_set_tooltip_text (tb, N_("Enable Scope"));
        g_signal_connect (G_OBJECT (tb), "toggled",
                          G_CALLBACK (ProbeIndicator::run_scope_callback), this);
	gtk_grid_attach (GTK_GRID (v_grid), tb, 1,1, 1,1);


        tb = gtk_toggle_button_new ();
        gtk_button_set_icon_name (GTK_BUTTON (tb), "system-search-symbolic");
        gtk_widget_show (tb);
        gtk_grid_attach (GTK_GRID (v_grid), tb, 40,40, 1,1);

        

        tb = gtk_toggle_button_new ();
        gtk_button_set_icon_name (GTK_BUTTON (tb), "system-search-symbolic");
        gtk_widget_show (tb);
        gtk_widget_set_name (tb, "probe-indicator-button"); // name used by CSS to apply custom color scheme
	gtk_widget_set_tooltip_text (tb, N_("Zoom Scope"));
        g_signal_connect (G_OBJECT (tb), "toggled",
                          G_CALLBACK (ProbeIndicator::zoom_scope_callback), this);
	gtk_grid_attach (GTK_GRID (v_grid), tb, 1,2, 1,1);

        tb = gtk_toggle_button_new ();
        gtk_button_set_icon_name (GTK_BUTTON (tb), "edit-delete-symbolic");
        gtk_widget_show (tb);
        gtk_widget_set_name (tb, "probe-indicator-button"); // name used by CSS to apply custom color scheme
	gtk_widget_set_tooltip_text (tb, N_("Full BW FFT/decimated 256 FFT"));
        g_signal_connect (G_OBJECT (tb), "toggled",
                          G_CALLBACK (ProbeIndicator::scope_ftfast_callback), this);
	gtk_grid_attach (GTK_GRID (v_grid), tb, 1,3, 1,1);
        
        tb = gtk_toggle_button_new ();
        gtk_button_set_icon_name (GTK_BUTTON (tb), "media-record-symbolic");
        gtk_widget_show (tb);
        gtk_widget_set_name (tb, "probe-indicator-button"); // name used by CSS to apply custom color scheme
	gtk_widget_set_tooltip_text (tb, N_("Enable Record Signal 1 and 2 Streams, decimated 256, realt time"));
        g_signal_connect (G_OBJECT (tb), "toggled",
                          G_CALLBACK (ProbeIndicator::record_callback), this);
	gtk_grid_attach (GTK_GRID (v_grid), tb, 1,4, 1,1);

        tb = gtk_toggle_button_new ();
        gtk_button_set_icon_name (GTK_BUTTON (tb), "preferences-system-details-symbolic");
        gtk_widget_show (tb);
        gtk_widget_set_name (tb, "probe-indicator-button"); // name used by CSS to apply custom color scheme
	gtk_widget_set_tooltip_text (tb, N_("Enable Basic Info"));
        g_signal_connect (G_OBJECT (tb), "toggled",
                          G_CALLBACK (ProbeIndicator::info_callback), this);
	gtk_grid_attach (GTK_GRID (v_grid), tb, 2,1, 1,1);

        tb = gtk_toggle_button_new ();
        gtk_button_set_icon_name (GTK_BUTTON (tb), "list-add-symbolic");
        gtk_widget_show (tb);
        gtk_widget_set_name (tb, "probe-indicator-button"); // name used by CSS to apply custom color scheme
	gtk_widget_set_tooltip_text (tb, N_("More details for Info"));
        g_signal_connect (G_OBJECT (tb), "toggled",
                          G_CALLBACK (ProbeIndicator::more_info_callback), this);
	gtk_grid_attach (GTK_GRID (v_grid), tb, 2,2, 1,1);

        tb = gtk_toggle_button_new ();
        gtk_button_set_icon_name (GTK_BUTTON (tb), "media-playback-pause-symbolic");
        gtk_widget_show (tb);
        gtk_widget_set_name (tb, "probe-indicator-button"); // name used by CSS to apply custom color scheme
	gtk_widget_set_tooltip_text (tb, N_("Pause all udpates, scope, recordings"));
        g_signal_connect (G_OBJECT (tb), "toggled",
                          G_CALLBACK (ProbeIndicator::pause_callback), this);
	gtk_grid_attach (GTK_GRID (v_grid), tb, 3,1, 1,1);

        tb = gtk_toggle_button_new ();
        gtk_button_set_icon_name (GTK_BUTTON (tb), "system-shutdown-symbolic");
        gtk_widget_show (tb);
        gtk_widget_set_name (tb, "probe-indicator-button"); // name used by CSS to apply custom color scheme
	gtk_widget_set_tooltip_text (tb, N_("N/A"));
        g_signal_connect (G_OBJECT (tb), "toggled",
                          G_CALLBACK (ProbeIndicator::shutdown_callback), this);
	gtk_grid_attach (GTK_GRID (v_grid), tb, 40,2, 1,1);

        tb = gtk_toggle_button_new ();
        gtk_button_set_icon_name (GTK_BUTTON (tb), "window-close-symbolic");
        gtk_widget_show (tb);
        gtk_widget_set_name (tb, "probe-indicator-button"); // name used by CSS to apply custom color scheme
	gtk_widget_set_tooltip_text (tb, N_("N/A"));
        g_signal_connect (G_OBJECT (tb), "toggled",
                          G_CALLBACK (ProbeIndicator::close_callback), this);
	gtk_grid_attach (GTK_GRID (v_grid), tb, 40,1, 1,1);
        
        probe = new hud_object();
        probe->add_tics ("T1", 0, 1., 9, 25.);
        probe->add_tics ("T1", 240, 1., 13, 10.);
        
        tip=probe->add_mark ("MT", 200, 0.);
        //probe->add_mark ("MB", 0, 0.7);

        m1=probe->add_mark ("M1", 0, 0., 1, -1, 0.75);
        m2=probe->add_mark ("M2", 0, 0., 1, -1, 0.75);
        probe->set_mark_color (m1, CAIRO_COLOR_RED_ID);
        probe->set_mark_color (m2, CAIRO_COLOR_RED_ID);
        
        ipos=probe->add_indicator ("IPos", 100.0, 50., 0, 2);
        ipos2=probe->add_indicator ("IPos", 100.0, 50., 1, 2);
        ineg=probe->add_indicator ("INeg", 100.0, -50., 0, 2);
        //ineg2=probe->add_indicator ("INeg", 100.0, -50., 1, 2);

        fpos=probe->add_indicator ("IPos", 300.0, 25., 0, 2);
        fpos2=probe->add_indicator ("IPos", 300.0, 20., 1, 2);
        fneg=probe->add_indicator ("INeg", 300.0, -12., 0, 2);
        fneg2=probe->add_indicator ("INeg", 300.0, -5., 1, 2);


        horizon[0]=probe->add_horizon ("Ifast", 0.0, 0.0, 128);
        probe->set_horizon_color(horizon[0], CAIRO_COLOR_RED_ID);

        horizon[1]=probe->add_horizon ("Zfast", 0.0, 0.0, 128);
        probe->set_horizon_color(horizon[1], CAIRO_COLOR_WHITE_ID);

        horizon[2]=probe->add_horizon ("IPSD", 0.0, 0.0, 128);
        probe->set_horizon_color(horizon[2], CAIRO_COLOR_GREEN_ID);

        horizon[3]=probe->add_horizon ("IdecHi", 0.0, 0.0, 128);
        probe->set_horizon_color(horizon[3], CAIRO_COLOR_ORANGE_ID);

        horizon[4]=probe->add_horizon ("IdecLo", 0.0, 0.0, 128);
        probe->set_horizon_color(horizon[4], CAIRO_COLOR_ORANGE_ID);

        horizon[5]=probe->add_horizon ("Zdec", 0.0, 0.0, 128);
        probe->set_horizon_color(horizon[5], CAIRO_COLOR_WHITE_ID);

        
	//probe->queue_update (canvas);
        background = new cairo_item_circle (0.,0., 100.);
	background->set_line_width (2.);
	background->set_stroke_rgba (CAIRO_COLOR_GRAY5_ID);
	background->set_fill_rgba (CAIRO_COLOR_BLACK_ID);
        
	info = new cairo_item_text (60.0, 35.0, "Probe HUD");
        //info->set_text ("Probe HUD")
	info->set_stroke_rgba (CAIRO_COLOR_WHITE);
	info->set_font_face_size ("Ununtu", 10.);
	info->set_spacing (-.1);
	info->set_anchor (CAIRO_ANCHOR_E);
	//info->queue_update (canvas);

	refresh ();
}

ProbeIndicator::~ProbeIndicator (){
	ProbeIndicator_valid = FALSE;

        PI_DEBUG (DBG_L4, "ProbeIndicator::~ProbeIndicator -- stop_tip_monitor RTQuery");

	stop ();
        run_fft (0, NULL, NULL, 0.,0.);

        UNREF_DELETE_CAIRO_ITEM (probe, canvas);
        UNREF_DELETE_CAIRO_ITEM (info, canvas);

	PI_DEBUG (DBG_L4, "ProbeIndicator::~ProbeIndicator () -- done.");
}

void ProbeIndicator::AppWindowInit(const gchar *title, const gchar *subtitle){
        PI_DEBUG (DBG_L2, "ProbeIndicator::AppWindowInit -- header bar");

        app_window = gxsm4_app_window_new (GXSM4_APP (main_get_gapp()->get_application ()));
        window = GTK_WINDOW (app_window);

	gtk_window_set_default_size (GTK_WINDOW(window), 2*hud_size, 2*hud_size);
	gtk_window_set_title (GTK_WINDOW(window), title);
	gtk_widget_set_opacity (GTK_WIDGET(window), 1.0);
	gtk_window_set_resizable (GTK_WINDOW(window), TRUE);
	gtk_window_set_decorated (GTK_WINDOW(window), FALSE);
        
	v_grid = gtk_grid_new ();
        gtk_window_set_child (GTK_WINDOW (window), v_grid);
	g_object_set_data (G_OBJECT (window), "v_grid", v_grid);
	gtk_widget_show (GTK_WIDGET (v_grid));
	gtk_widget_show (GTK_WIDGET (window));

	set_window_geometry ("probe-hud");
}

#if 0
gint ProbeIndicator::canvas_event_cb(GtkWidget *canvas, GdkEvent *event, ProbeIndicator *pv){
	//static int dragging=FALSE;
	//static GtkWidget *coordpopup=NULL;
	//static GtkWidget *coordlab=NULL;
        //static int pi=-1;
        //static int pj=-1;
        double mouse_pix_xy[2];
        //static double preset[2];
        
        //---------------------------------------------------------
	// cairo_translate (cr, 12.+pv->WXS/2., 12.+pv->WYS/2.);
        // scale to volate range
	// cairo_scale (cr, 0.5*pv->WXS/pv->max_x, -0.5*pv->WYS/pv->max_y);

        // undo cairo image translation/scale:
        mouse_pix_xy[0] = event->button.x-pv->hud_size; //(event->button.x - (double)(12+pv->WXS/2.))/( 0.5*pv->WXS/pv->max_x);
        mouse_pix_xy[1] = event->button.y-pv->hud_size; //(event->button.y - (double)(12+pv->WYS/2.))/( 0.5*pv->WYS/pv->max_y);


        switch (event->type) {
	case GDK_BUTTON_PRESS:
		switch(event->button.button) {
		case 1:
                        //g_object_set_data (G_OBJECT(canvas), "preset_xy", preset);
                        //main_get_gapp()->offset_to_preset_callback (canvas, gapp);
                        g_message ("ProbeIndicator Button1 Pressed at XY=%g, %g",  mouse_pix_xy[0], mouse_pix_xy[1]);
                }
                break;
		
	case GDK_MOTION_NOTIFY:
		break;
		
	case GDK_ENTER_NOTIFY:
                //pv->show_preset_grid = true;
		break;
	case GDK_LEAVE_NOTIFY:
                //pv->show_preset_grid = false;
                break;
		
	default: break;
	}
	return FALSE;
}
#endif

gboolean  ProbeIndicator::canvas_draw_function (GtkDrawingArea *area,
                                                cairo_t        *cr,
                                                int             width,
                                                int             height,
                                                ProbeIndicator *pv){
        
        double actual_size = (width < height ? width : height)/2;
        double scale = actual_size / pv->hud_size;
        
        // scale and translate origin to window center
	cairo_scale (cr, scale, scale);
	cairo_translate (cr, pv->hud_size, pv->hud_size);
        cairo_save (cr);

        // scale to volate range
	//cairo_scale (cr, 0.5*pv->WXS/pv->max_x, -0.5*pv->WYS/pv->max_y);

	pv->background->draw (cr); // fill/clear background
                                                 
	pv->probe->draw (cr);         // probe indicator
	pv->info->draw (cr);          // info text

        cairo_restore (cr);
        return TRUE;
}
 

void ProbeIndicator::start (){
	if (!timer_id){
		PI_DEBUG (DBG_L1, "ProbeIndicator::start_tip_monitor \n");
		timer_id = g_timeout_add (150, (GSourceFunc) ProbeIndicator_tip_refresh_callback, this);
	}
}

void ProbeIndicator::stop (){
	if (timer_id){
		PI_DEBUG (DBG_L1, "ProbeIndicator::stop_tip_monitor \n");
		timer_id = 0;
	}
	PI_DEBUG (DBG_L1, "ProbeIndicator::stop_tip_monitor OK.\n");
}


gint ProbeIndicator::refresh(){
        #define SCOPE_N 4096
        #define SCOPE_VIEW_DEC 4096/128
        static gfloat scope[4][SCOPE_N+1];
        static gfloat scopedec[2][SCOPE_N+1];
        static gfloat scope_min[4] = {0,0,0,0};
        static gfloat scope_max[4] = {0,0,0,0};
        static double xrms = 0.;
        static double xavg = 0.;
        static gint busy=FALSE;
        static gint infoflag=1;
        static gint scopeflag=1;
        gint dec=SCOPE_N/128;
        
        if (busy) return FALSE; // skip this one, busy

	double x,y,z,q,Ilg, Ilgmp, Ilgmi;
        double max_z = xsmres.AnalogVMaxOut*main_get_gapp()->xsm->Inst->VZ();

        busy = TRUE;
	x=y=z=q=0.;
        
        if (main_get_gapp()->xsm->hardware)
                main_get_gapp()->xsm->hardware->RTQuery ("zxy", z, x, y); // get tip position in volts
        else {
                busy = FALSE;
                return FALSE;
        }
        
        probe->set_mark_len (tip, z/max_z);
        if (fabs(z/max_z) < 0.8)
                probe->set_mark_color (tip, -1);
        else
                probe->set_mark_color (tip, CAIRO_COLOR_RED_ID);

	if (main_get_gapp()->xsm->hardware){
		//double x0,y0,z0;
#if 0
		if (main_get_gapp()->xsm->hardware->RTQuery ("O", z0, x0, y0)){ // get HR Offset
			gchar *tmp = NULL;
			tmp = g_strdup_printf ("Offset Z0: %7.3f " UTF8_ANGSTROEM
                                               "\nXY0: %7.3f " UTF8_ANGSTROEM
                                               ", %7.3f " UTF8_ANGSTROEM
					       "\nXYs: %7.3f " UTF8_ANGSTROEM
                                               ", %7.3f " UTF8_ANGSTROEM,
					       main_get_gapp()->xsm->Inst->V2ZAng(z0),
					       main_get_gapp()->xsm->Inst->V2XAng(x0),
					       main_get_gapp()->xsm->Inst->V2YAng(y0),
					       main_get_gapp()->xsm->Inst->V2XAng(x),
					       main_get_gapp()->xsm->Inst->V2YAng(y));
                        infoXY0->set_text (tmp);
                        //infoXY0->queue_update (canvas);
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
                        //tip_marker_z0->queue_update (canvas); // schedule update
		}
#endif

                // Life Paramater Info
		main_get_gapp()->xsm->hardware->RTQuery ("f0I", x, y, q); // get f0, I -- val1,2,3=[fo,Iav,Irms]

                // Freq
                if (fabs(x) < 200.){
                        probe->set_indicator_val (fpos2, 300.0, x > 0. ? x:0.);
                        probe->set_indicator_val (fneg2, 300.0, x < 0. ? x:0.);
                } else {
                        probe->set_indicator_val (fpos2, 300.0, 0.);
                        probe->set_indicator_val (fneg2, 300.0, 0.);
                }
                if (fabs(x) > 10.)
                        x *= 0.1;
                if (fabs(x) > 10.)
                        x = 10.*x/fabs(x);
                probe->set_indicator_val (fpos,  300.0, x > 0.? 10.*x : 0.);
                probe->set_indicator_val (fneg,  300.0, x < 0.? 10.*x : 0.);
              
                // Current
                Ilg = log10 (fabs(1000.*y) + 1.0);
                probe->set_indicator_val (ipos,  100.0, y > 0.? 25.*Ilg : 0.);
                probe->set_indicator_val (ineg,  100.0, y < 0.? -25.*Ilg : 0.);

                if (modes & SCOPE_ON){
                        main_get_gapp()->xsm->hardware->RTQuery ("S1", SCOPE_N+1, &scope[0][0]); // RT Query S1
                        main_get_gapp()->xsm->hardware->RTQuery ("S2", SCOPE_N+1, &scope[1][0]); // RT Query S2
                        if (modes & SCOPE_RECORD){
                                main_get_gapp()->xsm->hardware->RTQuery ("R3", SCOPE_N+1, &scopedec[0][0]); // RT Query S1 dec, record
                                main_get_gapp()->xsm->hardware->RTQuery ("R4", SCOPE_N+1, &scopedec[1][0]); // RT Query S2 dec, record
                        } else {
                                main_get_gapp()->xsm->hardware->RTQuery ("D3", SCOPE_N+1, &scopedec[0][0]); // RT Query S1 dec
                                main_get_gapp()->xsm->hardware->RTQuery ("D4", SCOPE_N+1, &scopedec[1][0]); // RT Query S2 dec
                        }
                        main_get_gapp()->xsm->hardware->RTQuery ("T",  SCOPE_N+1, NULL); // RT Query, Trigger next
                
                        gfloat xmax, xmin;
                        dec=SCOPE_N/128;
                        if (modes & SCOPE_ZOOM)
                                dec=1;


                        // Plot Signal1 (designed for Current (or may use dFreq) or MIX-IN-0, full BW 4096->128
                        xmax=xmin=scope[0][0]*dec;
                        xavg = 0.;
                        xrms = 0.;
                        gfloat xr,xc;
                        xc = 0.5*(scope_max[0]+scope_min[0]);
                        xr = scope_max[0]-scope_min[0];
                        int i,k;
                        for(i=k=0; i<128; ++i){
                                gfloat x=0.;
                                for (int j=0; j<dec; ++j, ++k){
                                        x += scope[0][k];
                                        xrms += scope[0][k]*scope[0][k];
                                        xavg += scope[0][k];
                                }
                                // x /= dec; // no need as auto scaled regardless
                                if (x>xmax)
                                        xmax = x;
                                if (x<xmin)
                                        xmin = x;
                      
                                horizon[0]->set_xy (i, i-64., -32.*(x-xc)/xr);
                        }
                        xrms /= SCOPE_N;
                        xavg /= SCOPE_N;
                        xrms = sqrt(xrms);
                        scope_max[0] = 0.9*scope_max[0] + 0.1*xmax;
                        scope_min[0] = 0.9*scope_min[0] + 0.1*xmin;
                        Ilgmp = log10 (fabs(1000.*scope_max[0]/dec / main_get_gapp()->xsm->Inst->nAmpere2V(1.)) + 1.0);
                        Ilgmi = log10 (fabs(1000.*scope_min[0]/dec / main_get_gapp()->xsm->Inst->nAmpere2V(1.)) + 1.0);
                        double upper=25.*(scope_max[0] > 0.? Ilgmp : -Ilgmp);
                        double lower=25.*(scope_min[0] > 0.? Ilgmi : -Ilgmi);
                        probe->set_indicator_val (ipos2, 100.+upper, lower-upper);
                        probe->set_mark_pos (m1,  upper);
                        probe->set_mark_pos (m2,  lower);

                        // Plot Signal1 (designed for Current (or may use dFreq) or MIX-IN-0, decimated DSP reat time scrolled view FULL-BW:256 @ 4096->128
                        // slow dec signal
                        xmax=xmin=scopedec[0][0];
                        xc = 0.5*(scope_max[2]+scope_min[2]);
                        xr = scope_max[2]-scope_min[2];
                        k = SCOPE_N-dec*128;
                        for(i=0; i<128; ++i){
                                gfloat x=scopedec[0][k];
                                gfloat x2=scopedec[0][k];
                                for (int j=0; j<dec; ++j, ++k){
                                        if (x <= scopedec[0][k])
                                                x = scopedec[0][k];
                                        if (x2 >= scopedec[0][k])
                                                x2 = scopedec[0][k];
                                        if (scopedec[0][k] > xmax)
                                                xmax = scopedec[0][k];
                                        if (scopedec[0][k] < xmin)
                                                xmin = scopedec[0][k];
                                }
                                horizon[3]->set_xy (i, i-64., -32.*(x-xc)/xr);
                                horizon[4]->set_xy (i, i-64., -32.*(x2-xc)/xr);
                        }
                        scope_max[2] = 0.7*scope_max[2] + 0.3*xmax;
                        scope_min[2] = 0.7*scope_min[2] + 0.3*xmin;

                        // decimated fs: 150000 Hz / 256 = 585.9375 Hz

                        // Calculate FFT
                        if (modes & SCOPE_FTFAST)
                                run_fft (SCOPE_N, &scope[0][0], &scope[2][0], 1e-7, 10.,0.1); // on full BW signal1
                        else
                                run_fft (SCOPE_N, &scopedec[0][0], &scope[2][0], 1e-7, 10.,0.1); // on decimated signal1

                        gint right=SCOPE_N/2;
                        if (modes & SCOPE_ZOOM)
                                right = SCOPE_N/8;

                        // Plot FFT
                        for(k=i=0; i<128; ++i){
                                gfloat x=100.;
                                gint next=(int)(log(i+1.0)/log(128.0)*right);
                                for (; k<=next && k<right; ++k){
                                        if (modes & SCOPE_DBG){
                                                x = scope[2][k];
                                                continue;
                                        }
                                        //g_print ("%d %g\n",k,scope[2][k]);
                                        if (x > scope[2][k]) // peak decimated 0 ... -NN db
                                                x = scope[2][k];
                                }
                                horizon[2]->set_xy (i, i-64., -32.*x/96.); // x: 0..-96db
                        }

                
                        // Plot Signal2 (designed for Current (or may use dFreq) or MIX-IN-0, decimated DSP reat time scrolled view FULL-BW and :256 @ 4096->128
                        xmax=xmin = main_get_gapp()->xsm->Inst->V2ZAng (scope[1][0]); // in Ang
                        xc = 0.5*(scope_max[1]+scope_min[1]); // center
                        xr = scope_max[1]-scope_min[1]; // range
                        for(i=k=0; i<128; ++i){
                                gfloat x=0.;
                                gfloat xdec=0.;
                                for (int j=0; j<dec; ++j, ++k)
                                        x += scope[1][k];
                                
                                x /= dec;
                                x = main_get_gapp()->xsm->Inst->V2ZAng(x); // Z in Ang

                                if (i <= 64) // shift so center is current (where the tip is), lhs = now to -T1/2, rhs: older (-T1/2..-T)
                                        xdec = scopedec[1][(i+64)*SCOPE_VIEW_DEC-1]; // simple decimated data stream, in V
                                else
                                        xdec = scopedec[1][(i-64)*SCOPE_VIEW_DEC-1]; // simple decimated data stream, in V
                        
                                if (x>xmax)
                                        xmax = x;
                                if (x<xmin)
                                        xmin = x;

                                //horizon[1]->set_xy (i, i-64., 32.*(x-xc)/xr.); // autorange
                                horizon[1]->set_xy (i, i-64., 64.*(x-xc)/16.); // xr is too jumpy ... fixed 2A/div (+/-16A on grid)
                                horizon[5]->set_xy (i, i-64, 100.*xdec/max_z); // decimated rolling signal, full scale (max z range matching tip marker)
                                //g_print ("decZ %d %g  fBW: %g   xc: %g xdec: %g V, max_z: %g V\n",i,
                                //         main_get_gapp()->xsm->Inst->V2ZAng(scopedec[1][k]), main_get_gapp()->xsm->Inst->V2ZAng(scope[1][k]), xc, xdec, max_z);

                        }
                        // update smoothly
                        scope_max[1] = 0.9*scope_max[1] + 0.1*xmax;
                        scope_min[1] = 0.9*scope_min[1] + 0.1*xmin;
                        scopeflag=1;
                } else {
                        if (scopeflag){
                                for(int i=0; i<128; ++i)
                                        for(int k=0; k<=5; ++k)
                                                horizon[k]->set_xy (i, i-64., 0.);
                                scopeflag=0;
                        }
                }

                if (modes & SCOPE_INFO){
                        gchar *tmp = NULL;
                        double yy = 0.;
                        if (modes & SCOPE_ON){
                                y = xavg/main_get_gapp()->xsm->Inst->nAmpere2V(1.);
                                yy = xrms/main_get_gapp()->xsm->Inst->nAmpere2V(1.);
                        }
                        if (modes & SCOPE_INFOPLUS){
                                if (fabs(y) < 0.00025) // I display in atto amp
                                        tmp = g_strdup_printf ("I@%s: %6.3f [%.2f] aA\ndF: %8.2f Hz\nZ: %8.4f " UTF8_ANGSTROEM" \nI: %.2f : %.2f pA\nZ: %.2f : %.2f " UTF8_ANGSTROEM,
                                                               main_get_gapp()->xsm->Inst->IVC_Ampere2Volt_Setting(),
                                                               y*1e6, yy*1e6, x, main_get_gapp()->xsm->Inst->V2ZAng(z),
                                                               scope_min[0]/dec/main_get_gapp()->xsm->Inst->nAmpere2V(1.)*1000., scope_max[0]/dec/main_get_gapp()->xsm->Inst->nAmpere2V(1.)*1000.,
                                                               scope_min[1], scope_max[1]
                                                               );
                                else if (fabs(y) < 0.25)
                                        tmp = g_strdup_printf ("I@%s: %8.1f [%.2f] pA\ndF: %8.2f Hz\nZ: %8.4f" UTF8_ANGSTROEM "\nI: %.2f : %.2f pA\nZ: %.2f : %.2f " UTF8_ANGSTROEM,
                                                               main_get_gapp()->xsm->Inst->IVC_Ampere2Volt_Setting(),
                                                               y*1e3, yy*1e3, x, main_get_gapp()->xsm->Inst->V2ZAng(z),
                                                               scope_min[0]/dec/main_get_gapp()->xsm->Inst->nAmpere2V(1.)*1000., scope_max[0]/dec/main_get_gapp()->xsm->Inst->nAmpere2V(1.)*1000.,
                                                               scope_min[1], scope_max[1]
                                                               );
                                else
                                        tmp = g_strdup_printf ("I@%s: %8.1f [%.2f] nA\ndF: %8.2f Hz\nZ: %8.4f" UTF8_ANGSTROEM "\nI: %.2f : %.2f nA\nZ: %.2f : %.2f " UTF8_ANGSTROEM,
                                                               main_get_gapp()->xsm->Inst->IVC_Ampere2Volt_Setting(),
                                                               y, yy, x, main_get_gapp()->xsm->Inst->V2ZAng(z),
                                                               scope_min[0]/dec/main_get_gapp()->xsm->Inst->nAmpere2V(1.), scope_max[0]/dec/main_get_gapp()->xsm->Inst->nAmpere2V(1.),
                                                               scope_min[1], scope_max[1]
                                                               );
                                //tmp = g_strdup_printf ("I: %8.4f nA\ndF: %8.1f Hz\nZ: %8.4f" UTF8_ANGSTROEM,
                                //                               y, x); //  "\n%g:%g", main_get_gapp()->xsm->Inst->V2ZAng(z), scope_min[0]/dec,scope_max[0]/dec);

                                // query and print to terminal DSP task list
                                double a,b,c;
                                main_get_gapp()->xsm->hardware->RTQuery ("S", a, b, c); // get DST RT statemachine status info, with termial process list dump option on

                        } else {
                                if (fabs(y) < 0.00025)
                                        tmp = g_strdup_printf ("I@%s: %8.1f aA\ndF: %8.2f Hz\nZ: %8.4f" UTF8_ANGSTROEM,
                                                               main_get_gapp()->xsm->Inst->IVC_Ampere2Volt_Setting(),
                                                               y*1e6, x, main_get_gapp()->xsm->Inst->V2ZAng(z));
                                else if (fabs(y) < 0.25)
                                        tmp = g_strdup_printf ("I@%s: %8.1f pA\ndF: %8.2f Hz\nZ: %8.4f" UTF8_ANGSTROEM,
                                                               main_get_gapp()->xsm->Inst->IVC_Ampere2Volt_Setting(),
                                                               y*1e3, x, main_get_gapp()->xsm->Inst->V2ZAng(z));
                                else
                                        tmp = g_strdup_printf ("I@%s: %8.4f nA\ndF: %8.2f Hz\nZ: %8.4f" UTF8_ANGSTROEM,
                                                               main_get_gapp()->xsm->Inst->IVC_Ampere2Volt_Setting(),
                                                               y, x, main_get_gapp()->xsm->Inst->V2ZAng(z));
                        }
                        info->set_text (tmp);
                        g_free (tmp);
                        info->queue_update (canvas);
                        infoflag=1;
                } else {
                        if (infoflag){
                                info->set_text ("");
                                info->queue_update (canvas);
                                infoflag=0;
                        }
                }
	}

        probe->queue_update (canvas);
        
        busy = FALSE;

	if (timer_id)
                return TRUE; // keep repeating
        else
                return FALSE; // cancel g_timeout calls
}

////////////////////////////////////////
// ENDE PROBE INDCATOR   ///////////////
////////////////////////////////////////
