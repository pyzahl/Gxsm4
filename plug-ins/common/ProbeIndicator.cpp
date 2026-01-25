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

// set KAO zoom
void ProbeIndicator::KAO_zoom_callback (GtkWidget *widget, gpointer user_data) {
        ProbeIndicator *pv = (ProbeIndicator *) user_data; 
        gchar *tmp = g_strdup_printf ("zoom %s %d",
                                      gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(widget)),
                                      gtk_combo_box_get_active(GTK_COMBO_BOX(widget))
                                      );
        double KAO_zf[] = { 1, 1.27, 1.5 };
        if (pv->probe)
                pv->probe->set_kao_zoom (KAO_zf[gtk_combo_box_get_active(GTK_COMBO_BOX(widget))]);
        g_message ("ProbeIndicator::KAO_zoom_callback: %s", tmp);
        g_free (tmp);
}

// set #CH
void ProbeIndicator::KAO_kaoch_callback (GtkWidget *widget, gpointer user_data) {
        ProbeIndicator *pv = (ProbeIndicator *) user_data; 
        gchar *tmp = g_strdup_printf ("kaoch %s %d",
                                      gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(widget)),
                                      gtk_combo_box_get_active(GTK_COMBO_BOX(widget))
                                      );
        
        if (gtk_combo_box_get_active(GTK_COMBO_BOX(widget)) == 0)
                g_slist_foreach (pv->kao_ch34_widget_list, (GFunc) App::hide_w, NULL), pv->kao_num_ch = 2;
        else
                g_slist_foreach (pv->kao_ch34_widget_list, (GFunc) App::show_w, NULL), pv->kao_num_ch = 4;

        g_message ("ProbeIndicator::KAO_zoom_callback: %s", tmp);
        g_free (tmp);
}

// set TDIV i.e. # samples from history, power of two
void ProbeIndicator::KAO_tdiv_callback (GtkWidget *widget, gpointer user_data) {
        ProbeIndicator *pv = (ProbeIndicator *) user_data; 
        gchar *tmp = g_strdup_printf ("tdiv %s %d",
                                      gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(widget)),
                                      gtk_combo_box_get_active(GTK_COMBO_BOX(widget))
                                      );
        //int KAO_hlen[] = { 128, 256, 512, 1024, 2048, 4096, 8192, 16384 };
        int KAO_hlen[] = { 256, 512, 1024, 2048, 4096, 8192, 16384 };
        int hlen=0;
        if (main_get_gapp()->xsm->hardware){
                hlen = main_get_gapp()->xsm->hardware->RTQuery ("L", 0, NULL); // query actual historty len available
                if (hlen >= KAO_hlen[gtk_combo_box_get_active(GTK_COMBO_BOX(widget))])
                        pv->kao_samples = KAO_hlen[gtk_combo_box_get_active(GTK_COMBO_BOX(widget))];
                else
                        g_warning ("ProbeIndicator::KAO_tdiv_callback current history len too short.");
        }
        g_message ("ProbeIndicator::KAO_tdiv_callback: %s => HLN is %d, selected %d", tmp, hlen, pv->kao_samples);
        g_free (tmp);
}

// set CH mode
void ProbeIndicator::KAO_mode_callback (GtkWidget *widget, gpointer user_data) {
        ProbeIndicator *pv = (ProbeIndicator *) user_data; 
        int ch= GPOINTER_TO_INT (g_object_get_data(G_OBJECT (widget), "SN"));
        gchar *tmp = g_strdup_printf ("mode%d %s %d", ch+1,
                                      gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(widget)),
                                      gtk_combo_box_get_active(GTK_COMBO_BOX(widget))
                                      );
        pv->kao_mode[ch] = gtk_combo_box_get_active(GTK_COMBO_BOX(widget));
        g_message ("ProbeIndicator::KAO_mode_callback: %s", tmp);
        g_free (tmp);
}

// set scale
void ProbeIndicator::KAO_skl_callback (GtkWidget *widget, gpointer user_data) {
        ProbeIndicator *pv = (ProbeIndicator *) user_data; 
        int ch= GPOINTER_TO_INT (g_object_get_data(G_OBJECT (widget), "SN"));
        int s = gtk_combo_box_get_active(GTK_COMBO_BOX(widget));
        if (s==0)
                pv->kao_scale[ch] = -1; // Auto
        else if (ch < 4){
                pv->kao_scale_M[ch] = 1./(1e-6*pow (10., (double)s-1));
                pv->kao_scale[ch] = pv->kao_scale_M[ch] * pv->kao_scale_m[ch];
        } else {
                ch -= 10;
                if (ch < 4 && ch >= 0){
                        pv->kao_scale_m[ch] = s == 0? 1. : s == 1? 2. : 5.;
                        pv->kao_scale[ch] = pv->kao_scale_M[ch] / pv->kao_scale_m[ch];
                }
        }
        gchar *tmp = g_strdup_printf ("skl%d %s %d x (%g x %g) = %g", ch+1, 
                                      gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(widget)),
                                      s,
                                      pv->kao_scale_M[ch], pv->kao_scale_m[ch], pv->kao_scale[ch]
                                      );
        g_message ("ProbeIndicator::KAO_skl_callback: %s", tmp);
        g_free (tmp);
}

// Request signal
void ProbeIndicator::KAO_signal_callback (GtkWidget *widget, gpointer user_data) {
        ProbeIndicator *pv = (ProbeIndicator *) user_data; 
        if (main_get_gapp()->xsm->hardware){
                int ch= GPOINTER_TO_INT (g_object_get_data(G_OBJECT (widget), "SN"));
                pv->kao_ch_unit[ch][0] = '*';
                pv->kao_ch_unit[ch][1] = '\0';
                gchar *tmp = g_strdup_printf ("C%d%s", ch+1,
                                              gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(widget))
                                              );
                int sn = main_get_gapp()->xsm->hardware->RTQuery (tmp, 0, (float*)pv->kao_ch_unit[ch]); // Request Signal, Copy unit sym into unit
                g_message ("ProbeIndicator::KAO_signal_callback: %s  in %d", tmp, pv->kao_ch_unit[ch]);
                g_free (tmp);
        }
        g_message ("ProbeIndicator::KAO_signal_callback OK");
}

void ProbeIndicator::KAO_dc_set_callback (GtkWidget *widget, gpointer user_data) {
        ProbeIndicator *pv = (ProbeIndicator *) user_data; 
        int ch = GPOINTER_TO_INT (g_object_get_data(G_OBJECT (widget), "SN"));
        pv->kao_dc_set[ch]=1e99;
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

void ProbeIndicator::scope_fft_callback (GtkWidget *widget, gpointer user_data) {
        ProbeIndicator *pv = (ProbeIndicator *) user_data; 
        //g_print ("ProbeIndicator::zoom_scope_callback TB: %d\n", gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget)));
        if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget)))
                pv->modes = (pv->modes & ~SCOPE_FFT) | SCOPE_FFT;
        else
                pv->modes &= ~SCOPE_FFT;
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


ProbeIndicator::ProbeIndicator (Gxsm4app *app):AppBase(app){ 
        hud_size = 160;
	timer_id = 0;
	probe = NULL;
        modes = SCOPE_NONE;
        
	AppWindowInit (N_("HUD Probe Indicator"));

	canvas = gtk_drawing_area_new(); // make a drawing area

        gtk_drawing_area_set_content_width  (GTK_DRAWING_AREA (canvas), 2*hud_size);
        gtk_drawing_area_set_content_height (GTK_DRAWING_AREA (canvas), 2.4*hud_size);

        gtk_widget_set_hexpand(GTK_WIDGET (canvas), true);
        gtk_widget_set_vexpand(GTK_WIDGET (canvas), true);

        gtk_widget_set_halign(v_grid, GTK_ALIGN_FILL); // Make grid fill horizontally
        gtk_widget_set_valign(v_grid, GTK_ALIGN_FILL); // Make grid fill vertically
        
        gtk_drawing_area_set_draw_func (GTK_DRAWING_AREA (canvas),
                                        G_CALLBACK (ProbeIndicator::canvas_draw_function),
                                        this, NULL);
		
	gtk_widget_show (canvas);
	gtk_grid_attach (GTK_GRID (v_grid), canvas, 1,1, 20,20);

        GtkWidget *tb = gtk_toggle_button_new ();
        gtk_button_set_icon_name (GTK_BUTTON (tb), "utilities-system-monitor-symbolic");
        gtk_widget_show (tb);
        gtk_widget_set_name (tb, "probe-indicator-button"); // name used by CSS to apply custom color scheme
	gtk_widget_set_tooltip_text (tb, N_("Enable Scope"));
        g_signal_connect (G_OBJECT (tb), "toggled",
                          G_CALLBACK (ProbeIndicator::run_scope_callback), this);
	gtk_grid_attach (GTK_GRID (v_grid), tb, 1,1, 1,1);

        tb = gtk_toggle_button_new ();
        gtk_button_set_icon_name (GTK_BUTTON (tb), "emblem-music-symbolic");
        gtk_widget_show (tb);
        gtk_widget_set_name (tb, "probe-indicator-button"); // name used by CSS to apply custom color scheme
	gtk_widget_set_tooltip_text (tb, N_("Calculate FFT. log (dB) vs. log (freq) plot."));
        g_signal_connect (G_OBJECT (tb), "toggled",
                          G_CALLBACK (ProbeIndicator::scope_fft_callback), this);
	gtk_grid_attach (GTK_GRID (v_grid), tb, 1,2, 1,1);

        tb = gtk_toggle_button_new ();
        gtk_button_set_icon_name (GTK_BUTTON (tb), "system-search-symbolic");
        gtk_widget_show (tb);
        gtk_widget_set_name (tb, "probe-indicator-button"); // name used by CSS to apply custom color scheme
	gtk_widget_set_tooltip_text (tb, N_("Zoom into FFT low freq"));
        g_signal_connect (G_OBJECT (tb), "toggled",
                          G_CALLBACK (ProbeIndicator::zoom_scope_callback), this);
	gtk_grid_attach (GTK_GRID (v_grid), tb, 1,3, 1,1);

        tb = gtk_toggle_button_new ();
        gtk_button_set_icon_name (GTK_BUTTON (tb), "starred-symbolic");
        gtk_widget_show (tb);
        gtk_widget_set_name (tb, "probe-indicator-button"); // name used by CSS to apply custom color scheme
	gtk_widget_set_tooltip_text (tb, N_("FFT peak detect"));
        g_signal_connect (G_OBJECT (tb), "toggled",
                          G_CALLBACK (ProbeIndicator::scope_ftfast_callback), this);
	gtk_grid_attach (GTK_GRID (v_grid), tb, 1,4, 1,1);

        tb = gtk_toggle_button_new ();
        gtk_button_set_icon_name (GTK_BUTTON (tb), "media-record-symbolic");
        gtk_widget_show (tb);
        gtk_widget_set_name (tb, "probe-indicator-button"); // name used by CSS to apply custom color scheme
	gtk_widget_set_tooltip_text (tb, N_("TEST MODE *** Enable Recording"));
        g_signal_connect (G_OBJECT (tb), "toggled",
                          G_CALLBACK (ProbeIndicator::record_callback), this);
	gtk_grid_attach (GTK_GRID (v_grid), tb, 1,5, 1,1);

        tb = gtk_toggle_button_new ();
        gtk_button_set_icon_name (GTK_BUTTON (tb), "media-playback-pause-symbolic");
        gtk_widget_show (tb);
        gtk_widget_set_name (tb, "probe-indicator-button"); // name used by CSS to apply custom color scheme
	gtk_widget_set_tooltip_text (tb, N_("Pause all udpates, scope, recordings"));
        g_signal_connect (G_OBJECT (tb), "toggled",
                          G_CALLBACK (ProbeIndicator::pause_callback), this);
	gtk_grid_attach (GTK_GRID (v_grid), tb, 1,6, 1,1);

        // ==== 2nd
        tb = gtk_toggle_button_new ();
        gtk_button_set_icon_name (GTK_BUTTON (tb), "preferences-system-details-symbolic");
        gtk_widget_show (tb);
        gtk_widget_set_name (tb, "probe-indicator-button"); // name used by CSS to apply custom color scheme
	gtk_widget_set_tooltip_text (tb, N_("Enable SPM Info"));
        g_signal_connect (G_OBJECT (tb), "toggled",
                          G_CALLBACK (ProbeIndicator::info_callback), this);
	gtk_grid_attach (GTK_GRID (v_grid), tb, 2,1, 2,1);

        tb = gtk_toggle_button_new ();
        gtk_button_set_icon_name (GTK_BUTTON (tb), "list-add-symbolic");
        gtk_widget_show (tb);
        gtk_widget_set_name (tb, "probe-indicator-button"); // name used by CSS to apply custom color scheme
	gtk_widget_set_tooltip_text (tb, N_("Channel scale/DIV info"));
        g_signal_connect (G_OBJECT (tb), "toggled",
                          G_CALLBACK (ProbeIndicator::more_info_callback), this);
	gtk_grid_attach (GTK_GRID (v_grid), tb, 2,2, 2,1);

        
#if 0
        tb = gtk_toggle_button_new ();
        gtk_button_set_icon_name (GTK_BUTTON (tb), "window-close-symbolic");
        gtk_widget_show (tb);
        gtk_widget_set_name (tb, "probe-indicator-button"); // name used by CSS to apply custom color scheme
	gtk_widget_set_tooltip_text (tb, N_("N/A"));
        g_signal_connect (G_OBJECT (tb), "toggled",
                          G_CALLBACK (ProbeIndicator::close_callback), this);
	gtk_grid_attach (GTK_GRID (v_grid), tb, 20,1, 1,1);

        tb = gtk_toggle_button_new ();
        gtk_button_set_icon_name (GTK_BUTTON (tb), "system-shutdown-symbolic");
        gtk_widget_show (tb);
        gtk_widget_set_name (tb, "probe-indicator-button"); // name used by CSS to apply custom color scheme
	gtk_widget_set_tooltip_text (tb, N_("N/A"));
        g_signal_connect (G_OBJECT (tb), "toggled",
                          G_CALLBACK (ProbeIndicator::shutdown_callback), this);
	gtk_grid_attach (GTK_GRID (v_grid), tb, 20,2, 1,1);
#endif   

        // Mini KAO Controls
        // ********************************************************************************

        // KAO time/div -- approx, exact time/DIV is measured and displayed
        //static gchar *KAO_TDIV[] = { "~10ms/DIV [128]", "~25ms/DIV [256]", "~50ms/DIV [512]", "~100ms/DIV [1024]", "~200ms/DIV [2048]", "~0.5s/DIV [4096]", "~1s/DIV [8192]", "~2s/DIV [16384]", NULL };
        static gchar *KAO_TDIV[] = { "~25ms/DIV [256]", "~50ms/DIV [512]", "~100ms/DIV [1024]", "~200ms/DIV [2048]", "~0.5s/DIV [4096]", "~1s/DIV [8192]", "~2s/DIV [16384]", NULL };
        GtkWidget *kao_tdivcb = gtk_combo_box_text_new (); 
        for (int j=0; KAO_TDIV[j]; ++j){ gchar *id = g_strdup_printf ("%d", j);  gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (kao_tdivcb), id, KAO_TDIV[j]); g_free (id); }
        gtk_widget_show (kao_tdivcb);
        g_signal_connect (G_OBJECT (kao_tdivcb), "changed",
                          G_CALLBACK (ProbeIndicator::KAO_tdiv_callback), this);
        gtk_combo_box_set_active (GTK_COMBO_BOX (kao_tdivcb), 5); 
	gtk_grid_attach (GTK_GRID (v_grid), kao_tdivcb, 16,1, 5,1);

        kao_ch34_widget_list = NULL;
        
        // CH Controls ****************************************
        for (int ch=0; ch<4; ++ch){
                int kao_row=18; // start CH controls at this grid row
                static int col[] = { 1,17,5,13 };

                // CH Label
                GtkWidget *l;
                static gchar *CH[] = { "CH1", "CH2", "CH3", "CH4", NULL };
                static gchar *CHname[] = { "kao-ch1-label", "kao-ch2-label", "kao-ch3-label", "kao-ch4-label", NULL };
                gtk_grid_attach (GTK_GRID (v_grid), l=gtk_label_new (CH[ch]), col[ch],kao_row++, 4,1);
                gtk_widget_set_name (l, CHname[ch]); // name used by CSS to apply custom color scheme
                gtk_label_set_width_chars (GTK_LABEL(l), 12);
                if (ch >=2) kao_ch34_widget_list = g_slist_prepend( kao_ch34_widget_list, l);

                // CH Mode
                static gchar *CHmod[] = { "AC", "DC", "GND", "OFF", NULL };
                GtkWidget *CHmodcb = gtk_combo_box_text_new (); 
                g_object_set_data(G_OBJECT (CHmodcb), "SN", GINT_TO_POINTER (ch)); 
                for (int j=0; CHmod[j]; ++j){ gchar *id = g_strdup_printf ("%d", j);  gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (CHmodcb), id, CHmod[j]); g_free (id); }
                g_signal_connect (G_OBJECT (CHmodcb), "changed",
                                  G_CALLBACK (ProbeIndicator::KAO_mode_callback), this);
                gtk_combo_box_set_active (GTK_COMBO_BOX (CHmodcb), 0); 
                gtk_widget_show (CHmodcb);
                gtk_grid_attach (GTK_GRID (v_grid), CHmodcb, col[ch],kao_row++, 4,1);
                if (ch >= 2) kao_ch34_widget_list = g_slist_prepend( kao_ch34_widget_list, CHmodcb);
       
                // CH Scale
                static gchar *CMskl[] = { "Auto", "1M", "100k", "10k", "1k", "100", "10", "1", "100m", "10m",  "1m", "100μ", "10μ", "1μ", NULL };
                GtkWidget *CMsklcb = gtk_combo_box_text_new (); 
                g_object_set_data(G_OBJECT (CMsklcb), "SN", GINT_TO_POINTER (ch)); 
                for (int j=0; CMskl[j]; ++j){ gchar *id = g_strdup_printf ("%d", j);  gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (CMsklcb), id, CMskl[j]); g_free (id); }
                gtk_widget_show (CMsklcb);
                g_signal_connect (G_OBJECT (CMsklcb), "changed",
                                  G_CALLBACK (ProbeIndicator::KAO_skl_callback), this);
                gtk_combo_box_set_active (GTK_COMBO_BOX (CMsklcb), 0); 
                gtk_grid_attach (GTK_GRID (v_grid), CMsklcb, col[ch],kao_row, 2,1);
                if (ch >= 2) kao_ch34_widget_list = g_slist_prepend( kao_ch34_widget_list, CMsklcb);

                static gchar *CMsklm[] = { "x1", "x2", "x5", NULL };
                CMsklcb = gtk_combo_box_text_new (); 
                g_object_set_data(G_OBJECT (CMsklcb), "SN", GINT_TO_POINTER (10+ch)); 
                for (int j=0; CMsklm[j]; ++j){ gchar *id = g_strdup_printf ("%d", j);  gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (CMsklcb), id, CMsklm[j]); g_free (id); }
                gtk_widget_show (CMsklcb);
                g_signal_connect (G_OBJECT (CMsklcb), "changed",
                                  G_CALLBACK (ProbeIndicator::KAO_skl_callback), this);
                gtk_combo_box_set_active (GTK_COMBO_BOX (CMsklcb), 0);
                gtk_grid_attach (GTK_GRID (v_grid), CMsklcb, col[ch]+2,kao_row++, 2,1);
                if (ch >= 2) kao_ch34_widget_list = g_slist_prepend( kao_ch34_widget_list, CMsklcb);
        
                // Signal Selectors *** RPSPMC ***
                static gchar *VCmap[] = { "X", "Y", "Z", "BIAS", "CURR", "IN1", "IN2", "IN3", "IN4", "AMP", "EXEC", "DFREQ", "PHASE", NULL };
                static int Sdefault[] = { 4,2,9,11 };
                GtkWidget *CHScb = gtk_combo_box_text_new (); 
                g_object_set_data(G_OBJECT (CHScb), "SN", GINT_TO_POINTER (ch)); 
                for (int j=0; VCmap[j]; ++j){ gchar *id = g_strdup_printf ("%d", j);  gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (CHScb), id, VCmap[j]); g_free (id); }
                gtk_widget_show (CHScb);
                g_signal_connect (G_OBJECT (CHScb), "changed",
                                  G_CALLBACK (ProbeIndicator::KAO_signal_callback), this);
                gtk_combo_box_set_active (GTK_COMBO_BOX (CHScb),  Sdefault[ch]);  // default: Current (ZSSIG)
                gtk_grid_attach (GTK_GRID (v_grid), CHScb, col[ch],kao_row++, 4,1);
                if (ch >= 2) kao_ch34_widget_list = g_slist_prepend( kao_ch34_widget_list, CHScb);
        
                // Auto AC pos or fixed DC reading and DC set to auto center on click
                kao_dc[ch] = gtk_button_new_with_label ("AC");
                g_object_set_data(G_OBJECT (kao_dc[ch]), "SN", GINT_TO_POINTER (ch)); 
                gtk_widget_show (kao_dc[ch]);
                g_signal_connect (G_OBJECT (kao_dc[ch]), "clicked",
                                  G_CALLBACK (ProbeIndicator::KAO_dc_set_callback), this);
                gtk_grid_attach (GTK_GRID (v_grid), kao_dc[ch], col[ch],kao_row++, 4,1);
                if (ch >= 2) kao_ch34_widget_list = g_slist_prepend( kao_ch34_widget_list, kao_dc[ch]);
        }

        
        g_message ("Probe Indicator HUD Object Init");
        
        probe = new hud_object();

        probe->set_kao_zoom(1.27);
        
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

        CAIRO_BASIC_COLOR_IDS chc[KAO_CHANNEL_NUMBER] = { CAIRO_COLOR_RED_ID, CAIRO_COLOR_YELLOW_ID, CAIRO_COLOR_GREEN_ID, CAIRO_COLOR_CYAN_ID };
        //CAIRO_BASIC_COLOR_IDS chc[KAO_CHANNEL_NUMBER] = { CAIRO_COLOR_CSS_LRED_ID, CAIRO_COLOR_CSS_DYELLOW_ID, CAIRO_COLOR_CSS_LGREEN_ID, CAIRO_COLOR_CSS_CYANBLUE_ID };
        //const gchar* chc[KAO_CHANNEL_NUMBER] = { "#e81b1b", "#e8e01b", "#69e81b", "#1be5e8" };
        const gchar *chl[KAO_CHANNEL_NUMBER]  = { "CH1", "CH2", "CH3", "CH4" }; 
        const gchar *chpl[KAO_CHANNEL_NUMBER] = { "CH1PSD", "CH2PSD", "CH3PSD", "CH4PSD" }; 

        kao_num_ch = 2;
        kao_trace_len = 256;
        kao_samples = 4096;

	kao_tinfo = new cairo_item_text (62.0, -62.0, "--s/DIV");
	kao_tinfo->set_stroke_rgba (CAIRO_COLOR_WHITE);
	kao_tinfo->set_font_face_size ("Ununtu", 8.);
	kao_tinfo->set_spacing (-.1);
	kao_tinfo->set_anchor (CAIRO_ANCHOR_E);
        
        for (int ch=0; ch<KAO_CHANNEL_NUMBER; ch++){
                trace[ch]=probe->add_horizon (chl[ch], 0.0, 0.0, kao_trace_len);
                probe->set_horizon_color(trace[ch], chc[ch]);
                trace_psd[ch]=probe->add_horizon (chpl[ch], 0.0, 0.0, kao_trace_len);
                probe->set_horizon_color(trace_psd[ch], chc[ch], 1.2);

                ch_info[ch]= new cairo_item_text (-62.0, -62.0+10*ch, "");
                ch_info[ch]->set_stroke_rgba (chc[ch]);
                ch_info[ch]->set_font_face_size ("Ununtu", 8.);
                ch_info[ch]->set_spacing (-.1);
                ch_info[ch]->set_anchor (CAIRO_ANCHOR_W);
        }
        
        background = new cairo_item_circle (0.,0., 100.);
	background->set_line_width (2.);
	background->set_stroke_rgba (CAIRO_COLOR_GRAY5_ID);
	background->set_fill_rgba (CAIRO_COLOR_BLACK_ID);
        
	info = new cairo_item_text (60.0, 35.0, "Probe HUD");
	info->set_stroke_rgba (CAIRO_COLOR_WHITE);
	info->set_font_face_size ("Ununtu", 10.);
	info->set_spacing (-.1);
	info->set_anchor (CAIRO_ANCHOR_E);

        g_message ("Probe Indicator GUI build complete.");


        // KAO Zoom -- init requires probe object (above) been build!
        static gchar *CMzoom[] = { "NORMAL", "OVER", "MAX", NULL };
        GtkWidget *CMzoomcb = gtk_combo_box_text_new (); 
        for (int j=0; CMzoom[j]; ++j){ gchar *id = g_strdup_printf ("%d", j);  gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (CMzoomcb), id, CMzoom[j]); g_free (id); }
        gtk_widget_show (CMzoomcb);
        g_signal_connect (G_OBJECT (CMzoomcb), "changed",
                          G_CALLBACK (ProbeIndicator::KAO_zoom_callback), this);
        gtk_combo_box_set_active (GTK_COMBO_BOX (CMzoomcb), 1); 
	gtk_grid_attach (GTK_GRID (v_grid), CMzoomcb, 18,2, 3,1);
        
        static gchar *CMkaoch[] = { "2CH", "4CH", NULL };
        GtkWidget *CMkaochcb = gtk_combo_box_text_new (); 
        for (int j=0; CMkaoch[j]; ++j){ gchar *id = g_strdup_printf ("%d", j);  gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (CMkaochcb), id, CMkaoch[j]); g_free (id); }
        gtk_widget_show (CMkaochcb);
        g_signal_connect (G_OBJECT (CMkaochcb), "changed",
                          G_CALLBACK (ProbeIndicator::KAO_kaoch_callback), this);
        gtk_combo_box_set_active (GTK_COMBO_BOX (CMkaochcb), 0); 
	gtk_grid_attach (GTK_GRID (v_grid), CMkaochcb, 18,3, 3,1);
                
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
	//gtk_window_set_resizable (GTK_WINDOW(window), TRUE);
	//gtk_window_set_decorated (GTK_WINDOW(window), FALSE);
        
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


        cairo_save (cr);
        cairo_scale (cr, pv->probe->get_kao_zoom(), pv->probe->get_kao_zoom());
        pv->kao_tinfo->draw (cr);     // T/DIV info text
        pv->ch_info[0]->draw (cr);    // CH info text
        pv->ch_info[1]->draw (cr);    // CH info text
        pv->ch_info[2]->draw (cr);    // CH info text
        pv->ch_info[3]->draw (cr);    // CH info text
        cairo_restore (cr);

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
        #define SCOPE_N_MAX 16384
        int SCOPE_N=kao_samples;
        static gfloat scope_t[SCOPE_N_MAX+1];
        static gfloat scope[KAO_CHANNEL_NUMBER][SCOPE_N_MAX+1];
        static gfloat scope_psd[KAO_CHANNEL_NUMBER][SCOPE_N_MAX+1];
        static gfloat scope_min[KAO_CHANNEL_NUMBER] = {0,0,0,0};
        static gfloat scope_max[KAO_CHANNEL_NUMBER] = {0,0,0,0};
        static double xrms[KAO_CHANNEL_NUMBER];
        static double xavg[KAO_CHANNEL_NUMBER] = { 0., 0., 0., 0. };
        static gint busy=FALSE;
        static gint ch34flag=1;
        static gint infoflag=1;
        static gint chinfoflag=1;
        static gint scopeflag=1;

        if (busy) return FALSE; // skip this one, busy
        if (!probe) return FALSE;
        
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
                        main_get_gapp()->xsm->hardware->RTQuery ("ST", SCOPE_N_MAX+1, &scope_t[0]);  // RT Query STime
                        main_get_gapp()->xsm->hardware->RTQuery ("S1", SCOPE_N_MAX+1, &scope[0][0]); // RT Query S1
                        main_get_gapp()->xsm->hardware->RTQuery ("S2", SCOPE_N_MAX+1, &scope[1][0]); // RT Query S2
                        if (kao_num_ch==4){
                                main_get_gapp()->xsm->hardware->RTQuery ("S3", SCOPE_N_MAX+1, &scope[2][0]); // RT Query S3
                                main_get_gapp()->xsm->hardware->RTQuery ("S4", SCOPE_N_MAX+1, &scope[3][0]); // RT Query S4
                        }
                        //main_get_gapp()->xsm->hardware->RTQuery ("S4", SCOPE_N_MAX+1, &scope[3][0]); // RT Query S4
                        //main_get_gapp()->xsm->hardware->RTQuery ("T",  SCOPE_N+1, NULL); // RT Query, Trigger next
                        
                        // TEST / DEMO MODE
                        if(modes & SCOPE_RECORD){
                                double w = 4./SCOPE_N;
                                static double phi=0;
                                for (int i=0; i<SCOPE_N+1; ++i){
                                        scope_t[SCOPE_N-i] = 0.33*w*i*2*M_PI+phi;
                                        scope[0][i] = sin (w*i*2*M_PI+phi);
                                        scope[1][i] = cos (w*i*2*M_PI+phi) > 0. ? 1:-1;
                                }
                                phi += 2*M_PI/360.;
                        }
                        
                        gfloat xmax, xmin;
                        gint dec=SCOPE_N/kao_trace_len;
                        double td = (scope_t[0] - scope_t[SCOPE_N-1])/16.;
                        double dxdt = 128./(scope_t[0] - scope_t[SCOPE_N-1]); // fixed canvas x width is 128.0
                        
                        if (td < 0.6) {
                                gchar *tmp=g_strdup_printf ("%5.1f ms/DIV", 1e3*td); kao_tinfo->set_text (tmp); g_free (tmp); kao_tinfo->queue_update (canvas);
                        } else {
                                gchar *tmp=g_strdup_printf ("%5.3f s/DIV", td); kao_tinfo->set_text (tmp); g_free (tmp); kao_tinfo->queue_update (canvas);
                        }

                        // Signal1 (designed for Current (or may use dFreq) or MIX-IN-0, full BW 4096->128
                        
                        // process KOA channels and prepare traces (traces)
                        //for (int ch=0; ch<KAO_CHANNEL_NUMBER; ++ch){

                        if (kao_num_ch < 4 && ch34flag){
                                ch34flag=0;
                                for(int i=0; i<kao_trace_len; ++i){
                                        trace[2]->set_xy (i, 0., 0.);
                                        trace[3]->set_xy (i, 0., 0.);
                                        trace_psd[2]->set_xy (i, 0., 0.);
                                        trace_psd[3]->set_xy (i, 0., 0.);
                                }
                                ch_info[2]->set_text (""); ch_info[2]->queue_update (canvas);
                                ch_info[3]->set_text (""); ch_info[3]->queue_update (canvas);
                        } else ch34flag=1;
                        
                        for (int ch=0; ch<kao_num_ch; ++ch){

                                // process auto scaleing and manual overrides
                                xmax=xmin=scope[ch][0];
                                xrms[ch] = 0.;
                                gfloat xr,xc;

                                // kao_scale[]: -1: -> Auto
                                if (kao_scale[ch] > 0.)
                                        xr = kao_scale[ch]*10.; // +/-8 DIV Full Scale  10px grid spacing
                                else
                                        xr = (scope_max[ch]-scope_min[ch])*5.*0.5; // Auto Scale to fit in +/-2 DIV

                                //if (xr < 1e-9) xr=1e-9;
                                
                                //kao_mode[]: AC DC GND
                                switch (kao_mode[ch]){
                                case 0: // AC Auto
                                        kao_dc_set[ch] = xavg[ch];
                                        break;
                                case 1: // DC
                                        if (kao_dc_set[ch] > 1e98) // auto set to avg on button
                                                kao_dc_set[ch] = xavg[ch];
                                        break; // DC
                                case 2: kao_dc_set[ch] = 0.0; // GND = center
                                        break;
                                default: kao_dc_set[ch] = 0.0; // OFF
                                        ch_info[ch]->set_text (""); ch_info[ch]->queue_update (canvas);
                                        gtk_button_set_label (GTK_BUTTON(kao_dc[ch]), "--");
                                        for(int i=0; i<kao_trace_len; ++i){
                                                trace[ch]->set_xy (i, 0., 0.);
                                                trace_psd[ch]->set_xy (i, 0., 0.);
                                        }
                                        continue;
                                        break;
                                }

                                // Signal Level DC "@=0"
                                { gchar *tmp=g_strdup_printf ("%.4g %s", kao_dc_set[ch], kao_ch_unit[ch]); gtk_button_set_label (GTK_BUTTON(kao_dc[ch]), tmp); g_free (tmp); }

                                // CH Scale Info
                                if (modes & SCOPE_INFOPLUS){
                                        static double lasts=0.;
                                        chinfoflag=1;
                                        double s=xr/10.0; // scale / DIV
                                        if (lasts != s){
                                                lasts=s;
                                                const gchar *AA = "Å";
                                                const gchar *nA = "nA";
                                                const gchar *mV = "mV";
                                                //if (!strcmp(*kao_ch_unit[ch],nA) || !strcmp(*kao_ch_unit[ch],mV) || !strcmp(*kao_ch_unit[ch],AA)){ // do not do auto prefixing
                                                if (kao_ch_unit[ch][0] == nA[0] || kao_ch_unit[ch][0] == mV[0] || kao_ch_unit[ch][0] == AA[0]){ // faster trick does. do not do auto prefixing
                                                        gchar *tmp=g_strdup_printf ("CH%d %5.3f %s/DIV", ch+1, s, kao_ch_unit[ch]); ch_info[ch]->set_text (tmp); g_free (tmp); ch_info[ch]->queue_update (canvas);
                                                } else {
                                                        int M=6;
                                                        //                                                            6                  10
                                                        static gchar *prefix[]    = { "a",  "f", "p", "n", "μ", "m", " ", "k", "M", "G", "T", NULL };
                                                        while (s > 999. && M < 10) { s*=1e-3; M++; }
                                                        while (s < 0.999 && M > 0) { s*=1e3; M--; }
                                                        gchar *tmp=g_strdup_printf ("CH%d %5.1f %s%s/DIV", ch+1, s, prefix[M], kao_ch_unit[ch]); ch_info[ch]->set_text (tmp); g_free (tmp); ch_info[ch]->queue_update (canvas);
                                                }
                                        }
                                } else {
                                        if (chinfoflag){
                                                chinfoflag=0;
                                                ch_info[0]->set_text (""); ch_info[0]->queue_update (canvas);
                                                ch_info[1]->set_text (""); ch_info[1]->queue_update (canvas);
                                                ch_info[2]->set_text (""); ch_info[2]->queue_update (canvas);
                                                ch_info[3]->set_text (""); ch_info[3]->queue_update (canvas);
                                        }
                                }
                                
                                xc = kao_dc_set[ch]; // Signal Center Value
                                
                                int i,k;
                                double xav=0.;

                                // process, scale, prepare trace horizon object(s)
                                double tcenter = scope_t[SCOPE_N>>1];
                                for(i=k=0; i<kao_trace_len; ++i){
                                        gfloat x=0.;
                                        for (int j=0; j<dec; ++j, ++k){
                                                x += scope[ch][k];
                                                xrms[ch] += scope[ch][k]*scope[ch][k];
                                        }
                                        xav += x;
                                        x /= dec; // native Units
                                        if (x>xmax)
                                                xmax = x;
                                        if (x<xmin)
                                                xmin = x;
                      
                                        trace[ch]->set_xy (i, dxdt*(scope_t[k]-tcenter), -80*(x-xc)/xr); // X=i-64
                                }
                                
                                // signal filters for smooth auto scaling
                                xrms[ch] /= SCOPE_N;
                                xav /= SCOPE_N;
                                xavg[ch] = xavg[ch]*0.9 + xav*0.1;
                                xrms[ch] = sqrt(xrms[ch]);
                                
                                // process IIR LP for ranges/limts for smooth visials
                                scope_max[ch] = 0.9*scope_max[ch] + 0.1*xmax;
                                scope_min[ch] = 0.9*scope_min[ch] + 0.1*xmin;
                                
                                if (kao_ch_unit[ch][0]=='n' && kao_ch_unit[ch][1]=='A'){ // test for CURR signal as only one in 'nA' 
                                        Ilgmp = log10 (fabs(1000.*scope_max[ch] / main_get_gapp()->xsm->Inst->nAmpere2V(1.)) + 1.0);
                                        Ilgmi = log10 (fabs(1000.*scope_min[ch] / main_get_gapp()->xsm->Inst->nAmpere2V(1.)) + 1.0);
                                        double upper=25.*(scope_max[ch] > 0.? Ilgmp : -Ilgmp);
                                        double lower=25.*(scope_min[ch] > 0.? Ilgmi : -Ilgmi);
                                        probe->set_indicator_val (ipos2, 100.+upper, lower-upper);
                                        probe->set_mark_pos (m1,  upper);
                                        probe->set_mark_pos (m2,  lower);
                                }

                                // compute FFTs
                                if (modes & SCOPE_FFT){
                                        run_fft (SCOPE_N_MAX, &scope[ch][0], &scope_psd[ch][0], 1e-7, 10.,0.1); // on full BW signal1

                                        gint C=SCOPE_N_MAX>>1;
                                        gint right=C;
                                        if (modes & SCOPE_ZOOM)
                                                right = C>>2;

                                        // Plot FFT (PSD) on dB
                                        double xs = 128./kao_trace_len;
                                        int i0 = kao_trace_len>>1;
                                        for(k=i=0; i<kao_trace_len; ++i){
                                                gfloat x=0.;
                                                gint next=(int)(log(i+1.0)/log((double)kao_trace_len)*right);
                                                int cnt;
                                                for (cnt=0; k<=next && k<right; ++k,++cnt){
                                                        if (modes & SCOPE_DBG){
                                                                x = scope_psd[ch][k];
                                                                continue;
                                                        }
                                                        if (modes & SCOPE_FTFAST){
                                                                x += scope_psd[ch][C-k]; // avg in window
                                                        } else {
                                                                if (cnt==0) x = scope_psd[ch][C-k];
                                                                else if (x > scope_psd[ch][C-k]) // peak decimated 0 ... -NN db
                                                                        x = scope_psd[ch][C-k];
                                                        }
                                                        //g_print ("%d %g\n",k,scope_psd[ch][k]);
                                                }
                                                if (modes & SCOPE_FTFAST)
                                                        x /= cnt;
                                                trace_psd[ch]->set_xy (i, -xs*(i-i0), 64-32.*x/96.); // x: 0..-96db
                                        }
                                } else {
                                        for(i=0; i<kao_trace_len; ++i)
                                                trace_psd[ch]->set_xy (i, 0., 0.);
                                }
                        }
                        scopeflag=1;
                } else {
                        if (scopeflag){
                                for(int i=0; i<kao_trace_len; ++i)
                                        for(int ch=0; ch<KAO_CHANNEL_NUMBER; ++ch){
                                                trace[ch]->set_xy (i, 0., 0.);
                                                trace_psd[ch]->set_xy (i, 0., 0.);
                                        }
                                scopeflag=0;
                        }
                }
                if (modes & SCOPE_INFO){
                        gchar *tmp = NULL;
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
