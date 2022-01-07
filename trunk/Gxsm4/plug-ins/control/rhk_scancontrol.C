/* Gnome gxsm - Gnome X Scanning Microscopy
 * universal STM/AFM/SARLS/SPALEED/... controlling and
 * data analysis software
 *
 * Gxsm Plugin Name: rhk_scancontrol.C
 * ========================================
 *
 * Copyright (C) 1999 The Free Software Foundation
 *
 * Authors: Percy Zahl <zahl@fkp.uni-hannover.de>
 * additional features: Farid El Gabaly <farid.elgabaly@uam.es> Juan de la Figuera <juan.delafiguera@uam.es>
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

/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 8 c-style: "K&R" -*- */

/* Please do not change the Begin/End lines of this comment section!
 * this is a LaTeX style section used for auto generation of the PlugIn Manual
 * Chapter. Add a complete PlugIn documentation inbetween the Begin/End marks!
 * All "% PlugInXXX" commentary tags are mandatory
 * All "% OptPlugInXXX" tags are optional
 * --------------------------------------------------------------------------------
% BeginPlugInDocuSection
% PlugInDocuCaption: RHK Scan Control (to be ported)
% PlugInName: rhk_scancontrol
% PlugInAuthor: Farid El Gabaly, Juan de la Figuera
% PlugInAuthorEmail: farid.elgabaly@uam.es, juan.delafiguera@uam.es
% PlugInMenuPath: windows-sectionRHK Scan Control

% PlugInDescription 
 
 
 Provides a Scan Control Window for the RHK STM
 100 electronics. It connects to the \Gxsm\ toolbar buttons
 \GxsmEmph{Scan, Movie, Stop} for quick access. The control panel
 offers in addition to \GxsmEmph{Scan, Movie, Stop} a \GxsmEmph{Pause}
 and \GxsmEmph{HS Capture} button. Use the \GxsmEmph{Pause} button to
 pause the scanning process and press it again for continuation.
 
 The RHK electronics is used through a stand-alone program (rhk\_controller)
 that has to be started before Gxsm. Use the computer and port in which that
 program is running as the Hardware/Device setting (localhost:5027 is the
 default).
 
 The \GxsmEmph{HS Capture} button starts a continous (movie) high
 speed (HS) frame capturing process, were a refresh is done after the
 whole scan data is received from the STM100 electronics. The scan
 size is limited by the available driver memory (currently 2Mb) on the
 rhk\_controller program. The \GxsmEmph{HS Capture} mode assures
 precise real time inter line timing and allows maximum frame rates
 due to minimized communication between Gxsm and the rhk\_controller
 program.

 This plugin is also the main RHK control panel. The RHK interface is slightly
 different from the rest of the SPM hardware supported by Gxsm. The
 difference is due to the fact that the scan generator is inside the
 RHK STM-100, so it is not under the control of Gxsm. The offset and
 scan size can only be read, as are the sample bias and the tunneling
 current. For now they are only updated when the "update" button is
 pressed, and before adquiring an image (the image size is also read from the
 RHK so this is a must).  An additional option is the automatic update of the
 parameters (every second or so), which can be turned on or off by a
 button.

 This RHK Gxsm PlugIn module actually provides not only the the
 scanning controls, it does the whole job of data adquisitation
 itself, and is a substitute for the standard Scan Control plugin.

 On the left side of the panel, there is a pixmap that shows the
 current scan area size and offset relative to the total range available.

% EndPlugInDocuSection
 * --------------------------------------------------------------------------------
 */

//#include <gdk>
#include <gtk/gtk.h>
#include "config.h"
#include "core-source/plugin.h"
#include "core-source/unit.h"
#include "core-source/pcs.h"
#include "core-source/xsmtypes.h"
#include "core-source/glbvars.h"
#include "core-source/action_id.h"
#include "core-source/instrument.h"
#include "include/dsp-pci32/xsm/xsmcmd.h"

typedef enum SCAN_DIR { SCAN_XY, SCAN_YX };
typedef enum SCAN_FLAG { SCAN_FLAG_READY, SCAN_FLAG_STOP,  SCAN_FLAG_PAUSE,  SCAN_FLAG_RUN };
typedef enum SCAN_DT_TYPE { SCAN_LINESCAN, SCAN_FRAMECAPTURE };

static GdkPixmap *pixmap;
static void rhk_scancontrol_StartScan_callback( gpointer );

// Plugin Prototypes - default PlugIn functions
static void rhk_scancontrol_init (void); // PlugIn init
static void rhk_scancontrol_query (void); // PlugIn "self-install"
static void rhk_scancontrol_about (void); // About
static void rhk_scancontrol_configure (void); // Configure plugIn, called via PlugIn-Configurator
static void rhk_scancontrol_cleanup (void); // called on PlugIn unload, should cleanup PlugIn rescources

// other PlugIn Functions and Callbacks (connected to Buttons, Toolbar, Menu)
static void rhk_scancontrol_show_callback (GtkWidget *w, void *data); // show ScanControl Window
static void rhk_scancontrol_start_callback (GtkWidget *w, void *data); // called on start scan
static void rhk_scancontrol_movie_callback (GtkWidget *w, void *data); // called on movie start
static void rhk_scancontrol_hscapture_callback (GtkWidget *w, void *data); // called on high speed capture start
static void rhk_scancontrol_pause_callback (GtkWidget *w, void *data); // called on pause/unpause
static void rhk_scancontrol_stop_callback (GtkWidget *w, void *data); // called on scan stop

static gint rhk_ScanControl_timed(void *dspc); // Autoupdate callback
// Fill in the GxsmPlugin Description here -- see also: Gxsm/src/plugin.h
GxsmPlugin rhk_scancontrol_pi = {
	NULL,                   // filled in and used by Gxsm, don't touch !
	NULL,                   // filled in and used by Gxsm, don't touch !
	0,                  // filled in and used by Gxsm, don't touch !
	NULL,                   // The Gxsm-App Class Ref.pointer (called "gapp" in Gxsm) is
	// filled in here by Gxsm on Plugin load,
	// just after init() is called !!!
	"rhk_ScanControl",
	"+STM +LAN_RHK:SPMHARD",
        // Description, is shown by PluginViewer (Plugin: listplugin, Tools->Plugin Details)
	"RHK scan control",
	"Farid El Gabaly, Juan de la Figuera",
	"windows-section",
	N_("RHK Scan Control"),
	N_("open RHK Scan Control Window"),
	"RHK Scan Control Window and Scan Generator PlugIn",
	NULL,          // error msg, plugin may put error status msg here later

	NULL,          // Plugin Status, managed by Gxsm, plugin may manipulate it too
	rhk_scancontrol_init,
	rhk_scancontrol_query,
	// about-function, can be "NULL"
	// can be called by "Plugin Details"
	rhk_scancontrol_about,
	// configure-function, can be "NULL"
	// can be called by "Plugin Details"
	rhk_scancontrol_configure,
	// run-function, can be "NULL", if non-Zero and no query defined,
	// it is called on menupath->"plugin"
	NULL,
	// cleanup-function, can be "NULL"
	// called if present at plugin removal
  NULL, // direct menu entry callback1 or NULL
  NULL, // direct menu entry callback2 or NULL

	rhk_scancontrol_cleanup
};

// Text used in Aboutbox, please update!!
static const char *about_text = N_("Gxsm RHK Scan Generator and Control Plugin\n\n"
                                   "This plugin manages the RHK scanning process\n"
				   "and settings readings."
                                   );

// Symbol "get_gxsm_plugin_info" is resolved by dlsym from Gxsm, used to get Plugin's info!!
// Essential Plugin Function!!
GxsmPlugin *get_gxsm_plugin_info ( void ){
	rhk_scancontrol_pi.description = g_strdup_printf(N_("Gxsm rhk_scancontrol plugin %s"), VERSION);
	return &rhk_scancontrol_pi;
}

// data passed to "idle" function call, used to refresh/draw while waiting for data
typedef struct {
	GSList *scan_list; // scans to update
	GFunc  UpdateFunc; // function to call for background updating
	gpointer data; // additional data (here: reference to the current rhk_ScanControl object)
} IdleRefreshFuncData;

// Scan Control Class based on AppBase
// -> AppBase provides a GtkWindow and some window handling basics used by Gxsm
class rhk_ScanControl : public AppBase{
public:

	rhk_ScanControl(); // create window and setup it contents, connect buttons, register cb's...
	virtual ~rhk_ScanControl(); // unregister cb's

        void updateRHK(void);
        static void ExecCmd(int cmd);
        static void ChangedNotify(GtkWidget *widget,rhk_ScanControl *dspc);
	static void ChangedAuto(GtkWidget *widget, rhk_ScanControl *dspc);




	void update(); // window update (inputs, etc. -- here currently not really necessary)

	int free_scan_lists (); // clean up all old/previous scan lists

	int initialize_scan_lists (); /* this scans the current channel list (Channelselector)
					 and sets up a list of all scans used for data storage.
				      */
	int initialize_default_pid_src (); // not jet exsistent (plans to split off initialize_scan_lists).
	int initialize_pid_src (); // not jet exsistent.
	int initialize_daq_srcs (); // not jet exsistent.

	int prepare_to_start_scan (SCAN_DT_TYPE st=SCAN_LINESCAN); /* prepare to start a scan:
								      set common scan parameters,
								      signal "scan start event" to hardware,
								      setup basic scan parameters and put to hardware,
								      initialize scan lists,
								      check for invalid parameters -- cancel in case of bad settings (return -1 if fails, 0 if OK)
								   */

	int setup_scan (int ch, const gchar *titleprefix,
			const gchar *name,
			const gchar *unit, const gchar *label,
			const gchar *vunit = NULL, const gchar *vlabel = NULL,
			const gchar *prbsrcs = NULL, int prboutp = 0);
	/* configure a scan -- gets settings from channelselector/configuration */
	void do_scanline (int init=FALSE); // execute a single scan line -- or if "line == -1" a HS 2D Area Capture/Scan
	void run_probe (int ipx, int ipy); // run a local probe
	int do_scan (); // do a full scan, line by line
	int do_hscapture (); // do a full frame capute
	void set_subscan (int ix0=0, int num=0); /* setup for partial/sub scan,
						    current line, start at x = ix0, num points = num
						 */
	void stop_scan () {
		if (scan_flag == SCAN_FLAG_RUN || scan_flag == SCAN_FLAG_PAUSE)
			scan_flag = SCAN_FLAG_STOP;
	}; // interrupt/cancel a scan in progress
	void pause_scan () {
		if (scan_flag == SCAN_FLAG_RUN)
			scan_flag = SCAN_FLAG_PAUSE;
		else
			if (scan_flag == SCAN_FLAG_PAUSE)
				scan_flag = SCAN_FLAG_RUN;
	}; // pause a scan in progress
	int scan_in_progress() {
		return scan_flag == SCAN_FLAG_RUN || scan_flag == SCAN_FLAG_PAUSE
			? TRUE : FALSE;
	}; // check if a scan is in progress
	int finish_scan (); /* finish the scan:
			       return to origin (center of first line (SPM)),
			       add some log info,
			       free scan lists
                            */

	double update_status_info (int reset=FALSE); // compute and show some scan status info
	void autosave_check (double sec, int initvalue=0); // check of autosave if requested

	int set_x_lookup_value (int i, double lv); // not jet used (future plans for remote...)
	int set_y_lookup_value (int i, double lv); // not jet used
	int set_l_lookup_value (int i, double lv); // not jet used

	// some helpers
	static void call_scan_start (Scan* sc, gpointer data){ sc->start (); };
	static void call_scan_draw_line (Scan* sc, gpointer data){
		sc->draw (((rhk_ScanControl*)data)->line2update, ((rhk_ScanControl*)data)->line2update+1);
	};
	static void call_scan_stop (Scan* sc, gpointer data){
		sc->stop (((rhk_ScanControl*)data)->scan_flag == SCAN_FLAG_STOP
			  && ((rhk_ScanControl*)data)->last_scan_dir == SCAN_XY,
			  ((rhk_ScanControl*)data)->line);
	};

	void SetScanDir (GtkWidget *w) {
		scan_dir = (SCAN_DIR) ((int) g_object_get_data ( G_OBJECT (w), "SCANDIR"));
		if (scan_dir==SCAN_XY) main_get_gapp()->xsm->hardware->ExecCmd(DSP_CMD_MOVETO_X);
		if (scan_dir==SCAN_YX) main_get_gapp()->xsm->hardware->ExecCmd(DSP_CMD_MOVETO_Y);
		PI_DEBUG(DBG_L2, "SCM=" << scan_dir); };
	void ClrScanDir (GtkWidget *w) { };

private:

        GtkWidget *drawingarea;
        /* Backing pixmap for drawing area */

        UnitObj *Unity, *Volt, *Current, *Force;
	SCAN_DATA *ScanData;
   //     DSP_Param *dsp;
	XSM_Instrument *instrument;
   //     Scan_Param *scan;
        Gtk_EntryControl *ec1;
        Gtk_EntryControl *ec2;
        Gtk_EntryControl *ec3;
        Gtk_EntryControl *ec4;
        Gtk_EntryControl *ec5;
        Gtk_EntryControl *ec6;
        Gtk_EntryControl *ec7;
        Gtk_EntryControl *ec8;
        gint	timer;


	Scan   *master_scan; // master "topo" scan -- needed as common parameter reference by probe
	Scan   *master_probescan; // master "probe" scan -- needed as common parameter reference by probe

	/* Scan and ProbeScan Lists: xp = X plus (-> dir), xm = X minus (<- dir) */
	GSList *xp_scan_list, *xp_prbscan_list;
	GSList *xm_scan_list, *xm_prbscan_list;

	/* xp/xm source mask (bit encoding of channels to aquire) */
	int    xp_srcs, xm_srcs;

	int YOriginTop; /* TRUE if the Y origin is the top (first) line (all SPM),
			   else FALSE (SPALEED uses the image center)
			*/

	int line, line2update; // current scan line and line to update in background
	int ix0off; // current X offset (in pixels) -- if in subscan mode
	SCAN_FLAG scan_flag; // scan status flag
	SCAN_DIR  scan_dir, last_scan_dir; // current and last scan direction
	gboolean  do_probe; // set if currently in probe mode
};

rhk_ScanControl *rhk_scancontrol = NULL;

// Query Function, installs Plugin's in File/Import and Export Menupaths!
// ----------------------------------------------------------------------
// Import Menupath is "File/Import/abc import"
// Export Menupath is "File/Export/abc import"
// ----------------------------------------------------------------------
// !!!! make sure the "rhk_scancontrol_cleanup()" function (see below) !!!!
// !!!! removes the correct menuentries !!!!

// Add Menu Entries:
// windows-sectionSPM Scan Control
// Action/SPM Scan Start/Pause/Stop + Toolbar

static void rhk_scancontrol_query(void)
{
	PI_DEBUG( DBG_L2, "rhk_scancontrol_query");
	static GnomeUIInfo menuinfo_windowcontrol[] = {
		{ GNOME_APP_UI_ITEM,
		  N_("RHK Scan Control"), N_("RHK Scan Control Window for advanced control"),
		  (gpointer) rhk_scancontrol_show_callback, NULL,
		  NULL, GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_OPEN,
		  0, GDK_CONTROL_MASK, NULL },
		GNOMEUIINFO_END
	};

	gnome_app_insert_menus (
                                GNOME_APP(rhk_scancontrol_pi.app->getApp()),
                                "windows-section",
                                menuinfo_windowcontrol
                                );

	if(rhk_scancontrol_pi.status) g_free(rhk_scancontrol_pi.status);
	rhk_scancontrol_pi.status = g_strconcat (
                                                 N_("Plugin query has attached "),
                                                 rhk_scancontrol_pi.name,
                                                 N_(": File IO Filters are ready to use"),
                                                 NULL);

	PI_DEBUG (DBG_L2, "rhk_scancontrol_query:new" );
	rhk_scancontrol = new rhk_ScanControl;

	PI_DEBUG (DBG_L2, "rhk_scancontrol_query:res" );
	rhk_scancontrol->SetResName ("WindowRHKScanControl", "false", xsmres.geomsave);

	PI_DEBUG (DBG_L2,"rhk_scancontrol_query:done");

}



// 5.) Start here with the plugins code, vars def., etc.... here.
// ----------------------------------------------------------------------
//


// init-Function
static void rhk_scancontrol_init(void)
{
        PI_DEBUG (DBG_L2, "rhk_scancontrol Plugin Init");
}

// about-Function
static void rhk_scancontrol_about(void)
{
        const gchar *authors[] = {rhk_scancontrol_pi.authors, NULL};
        gtk_show_about_dialog (NULL, "program-name",  rhk_scancontrol_pi.name,
                                        "version", VERSION,
                                          "license", GTK_LICENSE_GPL_3_0,
                                          "comments", about_text,
                                          "authors", authors,
                                          NULL,NULL, NULL
                                          ));
}

// configure-Function
static void rhk_scancontrol_configure(void)
{
	if(rhk_scancontrol_pi.app)
		rhk_scancontrol_pi.app->message("rhk_scancontrol Plugin Configuration");
}

// cleanup-Function, make sure the Menustrings are matching those above!!!
static void rhk_scancontrol_cleanup(void)
{
	PI_DEBUG (DBG_L2,"rhk_scancontrol Plugin Cleanup");
	gnome_app_remove_menus (GNOME_APP (rhk_scancontrol_pi.app->getApp()),
				N_("windows-sectionRHK Scan Control"), 1);
	// delete ...
	if( rhk_scancontrol )
		delete rhk_scancontrol ;

	if(rhk_scancontrol_pi.status) g_free(rhk_scancontrol_pi.status);

}

static void rhk_scancontrol_StartScan_callback( gpointer ){
        rhk_scancontrol->update();
}


static void rhk_scancontrol_show_callback( GtkWidget* widget, void* data){
	if( rhk_scancontrol )
		rhk_scancontrol->show();
}

static void cb_setscandir( GtkWidget *widget, rhk_ScanControl *scc ){
	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget)))
		scc->SetScanDir (widget);
	else
		scc->ClrScanDir (widget);
}

static gint configure_event(GtkWidget *w, GdkEventConfigure *event)
{
	if (pixmap) gdk_pixmap_unref(pixmap);
	pixmap= gdk_pixmap_new(w->window,w->allocation.width, w->allocation.height, -1);
        	gdk_draw_rectangle(pixmap, w->style->white_gc, TRUE, 0, 0,
        		w->allocation.width, w->allocation.height);
	return(TRUE);
}

static gint expose_event(GtkWidget *w, GdkEventExpose *e)
{
	gdk_draw_pixmap(w->window, w->style->fg_gc[GTK_WIDGET_STATE(w)],
		pixmap, e->area.x, e->area.y, e->area.x, e->area.y,
		e->area.width, e->area.height);
	return(FALSE);
}

static gint rhk_ScanControl_timed(void *dspc)
{
        printf("hola\n");

        ((rhk_ScanControl *)dspc)->updateRHK();
	return 1;
}

rhk_ScanControl::rhk_ScanControl ()
{
	GtkWidget *box, *hbox;
	GtkWidget *vbox_settings, *frame_settings;
	GtkWidget *frame_actions, *vbox_actions, *hbox_act1, *hbox_act2;
	GtkWidget *frame_param, *vbox_param, *hbox_param;
	GtkWidget *frame_dwg, *vbox_dwg;
	GtkWidget *vbox_extra;
	GtkWidget *button;

        GSList *EC_list=NULL;
        GSList **RemoteEntryList = new GSList *;
        *RemoteEntryList = NULL;

        GtkWidget *input1;
        GtkWidget *input2;
        GtkWidget *input3;
        GtkWidget *input4;
        GtkWidget *input5;
        GtkWidget *input6;
        GtkWidget *input7;
        GtkWidget *input8;

        GtkWidget *button2;


	master_scan      = NULL;
	master_probescan = NULL;

	xp_scan_list    = NULL;
	xp_prbscan_list = NULL;
	xm_scan_list    = NULL;
	xm_prbscan_list = NULL;
	scan_flag     = SCAN_FLAG_READY;
	scan_dir      = SCAN_XY;
	last_scan_dir = SCAN_XY;

	do_probe = FALSE;

        Unity    = new UnitObj(" "," ");
        Volt     = new UnitObj("V","V");
        Current  = new UnitObj("nA","nA");
        Force    = new UnitObj("nN","nN");

	ScanData = &rhk_scancontrol_pi.app->xsm->data;
//        dsp = &rhk_scancontrol_pi.app->xsm->data.hardpars;
        //dsp= &main_get_gapp()->xsm->data.hardpars;
//        scan = &rhk_scancontrol_pi.app->xsm->data.s;
        //scan=&main_get_gapp()->xsm->data.s;
        instrument = main_get_gapp()->xsm->Inst;

	AppWindowInit("RHK Scan Control");

	box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	gtk_widget_show (box);
	gtk_box_pack_start (GTK_BOX (vbox), box, TRUE, TRUE, 0);

	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_widget_show (hbox);
	gtk_box_pack_start (GTK_BOX (box), hbox, TRUE, TRUE, 0);

#define MYGTK_INPUT(L)  mygtk_create_input(L, vbox_param, hbox_param, 50, 70);

	frame_settings = gtk_frame_new (N_("RHK Control"));
	gtk_widget_show (frame_settings);
	gtk_container_add (GTK_CONTAINER (hbox), frame_settings);

	vbox_settings = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	gtk_widget_show (vbox_settings);
	gtk_container_add (GTK_CONTAINER (frame_settings), vbox_settings);

        // Actions frame/vbox

	frame_actions = gtk_frame_new (N_("Actions"));
	gtk_widget_show (frame_actions);
	gtk_container_add (GTK_CONTAINER (vbox_settings),frame_actions);

	vbox_actions = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	gtk_widget_show (vbox_actions);
	gtk_container_add (GTK_CONTAINER (frame_actions), vbox_actions);

        // First, buttons (hbox_act1)

	hbox_act1 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_widget_show (hbox_act1);
	gtk_box_pack_start (GTK_BOX (vbox_actions), hbox_act1, FALSE, FALSE, 0);

	button = gtk_button_new_with_label(N_("Start"));
	gtk_widget_show (button);
	gtk_box_pack_start (GTK_BOX (hbox_act1), button, TRUE, TRUE, 0);
	g_signal_connect ( G_OBJECT (button), "pressed",
			     G_CALLBACK (rhk_scancontrol_start_callback),
			     this);
	main_get_gapp()->RegisterPluginToolbarButton (G_OBJECT (button), "Toolbar_Scan_Start");

	button = gtk_button_new_with_label(N_("Movie"));
	gtk_widget_show (button);
	gtk_box_pack_start (GTK_BOX (hbox_act1), button, TRUE, TRUE, 0);
	g_signal_connect ( G_OBJECT (button), "pressed",
			     G_CALLBACK (rhk_scancontrol_movie_callback),
			     this);
	main_get_gapp()->RegisterPluginToolbarButton (G_OBJECT (button), "Toolbar_Scan_Movie");

	button = gtk_button_new_with_label(N_("HS Capture"));
	gtk_widget_show (button);
	gtk_box_pack_start (GTK_BOX (hbox_act1), button, TRUE, TRUE, 0);
	g_signal_connect ( G_OBJECT (button), "pressed",
			     G_CALLBACK (rhk_scancontrol_hscapture_callback),
			     this);
        //	main_get_gapp()->RegisterPluginToolbarButton (G_OBJECT (button), "Toolbar_Scan_HsCapture");

	button = gtk_button_new_with_label(N_("Pause"));
	gtk_widget_show (button);
	gtk_box_pack_start (GTK_BOX (hbox_act1), button, TRUE, TRUE, 0);
	g_signal_connect ( G_OBJECT (button), "pressed",
			     G_CALLBACK (rhk_scancontrol_pause_callback),
			     this);
	main_get_gapp()->RegisterPluginToolbarButton (G_OBJECT (button), "Toolbar_Scan_Pause");

	button = gtk_button_new_with_label(N_("Stop"));
	gtk_widget_show (button);
	gtk_box_pack_start (GTK_BOX (hbox_act1), button, TRUE, TRUE, 0);
	g_signal_connect ( G_OBJECT (button), "pressed",
			     G_CALLBACK (rhk_scancontrol_stop_callback),
			     this);
	main_get_gapp()->RegisterPluginToolbarButton (G_OBJECT (button), "Toolbar_Scan_Stop");


        // 	Then, radio buttons, hbox_act2

	GtkWidget *radiobutton;
	GSList    *radiogroup;

	hbox_act2 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_widget_show (hbox_act2);
	gtk_box_pack_start (GTK_BOX (vbox_actions), hbox_act2, FALSE, FALSE, 0);

	radiobutton = gtk_radio_button_new_with_label( NULL, "XY");
	gtk_box_pack_start (GTK_BOX (hbox_act2), radiobutton, TRUE, TRUE, 0);
 	gtk_widget_show (radiobutton);
 	g_object_set_data (G_OBJECT (radiobutton), "SCANDIR", (void*) SCAN_XY);
 	g_signal_connect (G_OBJECT (radiobutton), "clicked",
 			    G_CALLBACK (cb_setscandir), this);

 	radiogroup = gtk_radio_button_group (GTK_RADIO_BUTTON (radiobutton));

 	radiobutton = gtk_radio_button_new_with_label (radiogroup, "YX");
 	gtk_box_pack_start (GTK_BOX (hbox_act2), radiobutton, TRUE, TRUE, 0);
 	gtk_widget_show (radiobutton);
 	g_object_set_data (G_OBJECT (radiobutton), "SCANDIR", (void*) SCAN_YX);
 	g_signal_connect (G_OBJECT (radiobutton), "clicked",
 			    G_CALLBACK (cb_setscandir), this);


        // Parameters frame/vbox

	frame_param = gtk_frame_new (N_("Parameters"));
	gtk_widget_show (frame_param);
	gtk_container_add (GTK_CONTAINER (vbox_settings), frame_param);

	vbox_param = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	gtk_widget_show (vbox_param);
	gtk_container_add (GTK_CONTAINER (frame_param), vbox_param);

        input1 = mygtk_create_input("Xoff:", vbox_param, hbox_param);
        ec1 = new Gtk_EntryControl (Unity, MLD_WERT_NICHT_OK, &ScanData->s.x0, -40000., 40000., "6.1f", input1);
        //ec1->Set_ChangeNoticeFkt(RHKControl::ChangedNotify, this);
        EC_list = g_slist_prepend( EC_list, ec1);
        *RemoteEntryList = ec1->AddEntry2RemoteList("RHK_Xoffset", *RemoteEntryList);

        input2 = mygtk_add_input("Xscan:", hbox_param);
        ec2 = new Gtk_EntryControl (Unity, MLD_WERT_NICHT_OK, &ScanData->s.rx, -40000., 40000., "6.1f", input2);
        EC_list = g_slist_prepend( EC_list, ec2);
        *RemoteEntryList = ec2->AddEntry2RemoteList("RHK_Xscan", *RemoteEntryList);

        input3 = mygtk_create_input("Yoff:", vbox_param, hbox_param);
        ec3 = new Gtk_EntryControl (Unity, MLD_WERT_NICHT_OK, &ScanData->s.y0, -40000., 40000., "6.1f", input3);
        EC_list = g_slist_prepend( EC_list, ec3);
        *RemoteEntryList = ec3->AddEntry2RemoteList("RHK_Yoffset", *RemoteEntryList);

        input4 = mygtk_add_input("Yscan", hbox_param);
        ec4 = new Gtk_EntryControl (Unity, MLD_WERT_NICHT_OK, &ScanData->s.ry, -40000., 40000., "6.1f", input4);
        EC_list = g_slist_prepend( EC_list, ec4);
        *RemoteEntryList = ec4->AddEntry2RemoteList("RHK_Yscan", *RemoteEntryList);

        input5 = mygtk_create_input("Z range", vbox_param, hbox_param);
        ec5 = new Gtk_EntryControl (Unity, MLD_WERT_NICHT_OK, &ScanData->s.rz, -40000., 40000., "6.1f", input5);
        EC_list = g_slist_prepend( EC_list, ec5);
        *RemoteEntryList = ec5->AddEntry2RemoteList("RHK_Z_range", *RemoteEntryList);

        input6 = mygtk_add_input("nx, ny", hbox_param);
        ec6 = new Gtk_EntryControl (Unity, MLD_WERT_NICHT_OK, &ScanData->s.nx, 128, 1024, "6.0f", input6);
        EC_list = g_slist_prepend( EC_list, ec6);
        *RemoteEntryList = ec6->AddEntry2RemoteList("RHK_nx", *RemoteEntryList);

        input7 = mygtk_create_input("UBias", vbox_param, hbox_param);
        ec7 = new Gtk_EntryControl (Unity, MLD_WERT_NICHT_OK, &ScanData->hardpars.UTunnel, -40000., 40000., "6.2f", input7);
        EC_list = g_slist_prepend( EC_list, ec7);
        *RemoteEntryList = ec7->AddEntry2RemoteList("RHK_bias", *RemoteEntryList);

        input8 = mygtk_add_input("Tunnel", hbox_param);
        ec8 = new Gtk_EntryControl (Unity, MLD_WERT_NICHT_OK, &ScanData->hardpars.ITunnelSoll, -40000., 40000., "6.2f", input8);
        EC_list = g_slist_prepend( EC_list, ec8);
        *RemoteEntryList = ec8->AddEntry2RemoteList("RHK_I", *RemoteEntryList);

        //----------------------- update params buttons

        hbox_param = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
        gtk_widget_show (hbox_param);
        gtk_box_pack_start(GTK_BOX(vbox_param), hbox_param, FALSE, FALSE, 0);

        button = gtk_button_new_with_label (N_("Refresh"));
        gtk_widget_ref (button);
        // g_object_set_data_full (G_OBJECT (box), "button", button,
        //                          (GtkDestroyNotify) gtk_widget_unref);
        gtk_widget_show (button);
        gtk_box_pack_start (GTK_BOX (hbox_param), button, FALSE, FALSE, 0);
        g_signal_connect ( G_OBJECT (button), "pressed",
                             G_CALLBACK (rhk_ScanControl::ChangedNotify),
                             this);


        button2 = gtk_button_new_with_label (N_("Autoupdate!"));
        gtk_widget_ref (button2);
        //g_object_set_data_full (G_OBJECT (box), "button2", button2,
        //                        (GtkDestroyNotify) gtk_widget_unref);
        gtk_widget_show (button2);
        gtk_box_pack_start (GTK_BOX (hbox_param), button2, FALSE, FALSE, 0);
        g_signal_connect ( G_OBJECT (button2), "pressed",
                             G_CALLBACK (rhk_ScanControl::ChangedAuto),
                             this);


        // Empty box

	vbox_extra = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	gtk_widget_show (vbox_extra);
	gtk_box_pack_start(GTK_BOX(vbox_settings), vbox_extra, TRUE, TRUE, 0);

        //----------------------------TIP POSITION-------------------------------------------

        frame_dwg = gtk_frame_new ("Tip Position");
        gtk_widget_ref (frame_dwg);
        g_object_set_data_full (G_OBJECT (hbox), "frame_dwg", frame_dwg,
                                  (GtkDestroyNotify) gtk_widget_unref);
        gtk_widget_show (frame_dwg);
        gtk_container_add (GTK_CONTAINER (hbox), frame_dwg);
        vbox_dwg = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
        gtk_widget_show (vbox_dwg);
        gtk_container_add (GTK_CONTAINER (frame_dwg), vbox_dwg);

        drawingarea = gtk_drawing_area_new ();
        gtk_drawing_area_size (GTK_DRAWING_AREA(drawingarea), 300, 300);
 	gtk_widget_show (drawingarea);
 	gtk_box_pack_start (GTK_BOX (vbox_dwg), drawingarea, TRUE, TRUE, 0);


        g_signal_connect (G_OBJECT (drawingarea), "expose_event",
                            (GtkSignalFunc) expose_event, NULL);
        g_signal_connect (G_OBJECT(drawingarea),"configure_event",
                            (GtkSignalFunc) configure_event, NULL);
        g_slist_prepend(EC_list, drawingarea);

        //			 -------- END tip position END ----------

        // save List away...
	g_object_set_data( G_OBJECT (widget), "RHKCONTROL_EC_list", EC_list);

        this->updateRHK();
        timer=0;
}

rhk_ScanControl::~rhk_ScanControl (){

        //  XsmRescourceManager xrm("RHKScanControl");
        //  xrm.Put("SPMScanControl.Xorg", Xorg);

	main_get_gapp()->RegisterPluginToolbarButton (NULL, "Toolbar_Scan_Start");
 main_get_gapp()->RegisterPluginToolbarButton (NULL, "Toolbar_Scan_Movie");
	main_get_gapp()->RegisterPluginToolbarButton (NULL, "Toolbar_Scan_Pause");
	main_get_gapp()->RegisterPluginToolbarButton (NULL, "Toolbar_Scan_Stop");
	// remove assigned memory
	line = -1;
	do_scanline (TRUE);
  delete Unity;
  delete Volt;
  delete Current;
  delete Force;
  if (timer)
  	gtk_timeout_remove(timer);

}

//	main_get_gapp()->xsm->hardware->SetScanMode(MEM_ADDTO);

void rhk_ScanControl::update(){
	g_slist_foreach((GSList*)g_object_get_data( G_OBJECT (widget), "RHKCONTROL_EC_list"),
			(GFunc) App::update_ec, NULL);
}

// Menu Call Back Fkte

static void rhk_scancontrol_start_callback (GtkWidget *w, void *data){
	if (((rhk_ScanControl*)data) ->  scan_in_progress())
		return;
	time_t t; // Scan - Startzeit eintragen
	time(&t);
	G_FREE_STRDUP_PRINTF(main_get_gapp()->xsm->data.ui.dateofscan, ctime(&t));
	main_get_gapp()->spm_update_all();
	main_get_gapp()->xsm->hardware->SetScanMode();

	((rhk_ScanControl*)data) -> do_scan();
	if(main_get_gapp()->xsm->IsMode(MODE_AUTOSAVE))
		main_get_gapp()->xsm->save(AUTO_NAME_SAVE);
}

static void rhk_scancontrol_movie_callback (GtkWidget *w, void *data){
	int nostop;
	if (((rhk_ScanControl*)data) ->  scan_in_progress())
		return;
	do {
		time_t t; // Scan - Startzeit eintragen
		time(&t);
		G_FREE_STRDUP_PRINTF(main_get_gapp()->xsm->data.ui.dateofscan, ctime(&t));
		main_get_gapp()->spm_update_all();
		main_get_gapp()->xsm->hardware->SetScanMode();

		nostop = ((rhk_ScanControl*)data) -> do_scan();

		if(main_get_gapp()->xsm->IsMode(MODE_AUTOSAVE))
			main_get_gapp()->xsm->save(AUTO_NAME_SAVE);
	} while (nostop);
}



static void rhk_scancontrol_hscapture_callback (GtkWidget *w, void *data){
	int nostop;
	if (((rhk_ScanControl*)data) ->  scan_in_progress())
		return;
	do {
		time_t t; // Scan - Startzeit eintragen 
		time(&t);
		G_FREE_STRDUP_PRINTF(main_get_gapp()->xsm->data.ui.dateofscan, ctime(&t));
		main_get_gapp()->spm_update_all();
		main_get_gapp()->xsm->hardware->SetScanMode();

		nostop = ((rhk_ScanControl*)data) -> do_hscapture();

		if(main_get_gapp()->xsm->IsMode(MODE_AUTOSAVE))
			main_get_gapp()->xsm->save(AUTO_NAME_SAVE);
	} while (nostop);
}

static void rhk_scancontrol_pause_callback (GtkWidget *w, void *data){
	((rhk_ScanControl*)data) -> pause_scan();
}

static void rhk_scancontrol_stop_callback (GtkWidget *w, void *data){
	((rhk_ScanControl*)data) -> stop_scan();
}


// ==================================================
// Scan Control Routines
// ==================================================


// ========================================
// DoScan
// ========================================
//
// aktiviert automatisch alle vorherigen / bzw. einen neuen Scan Channel
// setzt Channel-Zuordnung
// SRCS MODES:
// bit 0 1 2 3:   PID (Z), ... (calculated stuff (Z=Topo))
// bit 4 5 6 7:   MUXA select PIDSRC (Force, I, dF, ..)   (mux A bei PC31, bei PCI32 alle auf A)
// bit 8,9,10,11: MUXB select Analog Value (Friction, ..) (mux B bei PC31)
// bit 12,13,14,15: AUX select C,D
#define MSK_PID(X)  (1<<((X)&3))
#define MSK_MUXA(X) (1<<(((X)&3)+4))
#define MSK_MUXB(X) (1<<(((X)&3)+8))
#define MSK_AUX(X)  (1<<(((X)&3)+12))


int rhk_ScanControl::free_scan_lists (){
	if (xp_scan_list){
		g_slist_free (xp_scan_list);
		xp_scan_list = NULL;
	}
	if (xp_prbscan_list){
		g_slist_free (xp_prbscan_list);
		xp_prbscan_list = NULL;
	}
	if (xm_scan_list){
		g_slist_free (xm_scan_list);
		xm_scan_list = NULL;
	}
	if (xm_prbscan_list){
		g_slist_free (xm_prbscan_list);
		xm_prbscan_list = NULL;
	}

	return 0;
}

// analyse channelselections and setup scanlists and scans
int rhk_ScanControl::initialize_scan_lists (){
	int i,ipid,idaq,iprb,ch,sok,checks;

	if (xp_scan_list || xm_scan_list
	    || xp_prbscan_list || xm_prbscan_list) // stop if scan lists are existing!
		return -1;

	do_probe = FALSE;
	master_scan = NULL;
	master_probescan = NULL;

	sok=FALSE; // ScanOK = no
	checks=2;    // only one second try!
	xp_srcs = xm_srcs = 0; // reset srcs, take care of PID default, to do!!!!

	// analyze channel settings and create scan lists, and setup this scans
	do {
		PI_DEBUG (DBG_L3, "rhk_SCANCONTROL::initialize_scan_lists : do");

		// Find and init Xp-Scans
		for (ipid=idaq=iprb=i=0; i < MAXSCANS; i++){
			// PID src?  Find the first/next "Topo like" scan...
			// (ipid counts until all PID type scans are checked)
			for (ch = -1; (ipid < PIDCHMAX) && ((ch=main_get_gapp()->xsm->FindChan(xsmres.pidchno[ipid++])) < 0););

			if(ch >= 0){ // got one!
				PI_DEBUG (DBG_L3, "rhk_SCANCONTROL::initialize_scan_lists : Setting up XpScan - PID src found");
				// add bit to xp_srcs mask
				xp_srcs |= MSK_PID(ipid-1);
				// setup this scan
				setup_scan (ch, "X+",
					    xsmres.pidsrc[ipid-1], 
					    xsmres.pidsrcZunit[ipid-1], 
					    xsmres.pidsrcZlabel[ipid-1]
					);
				// and add to list
				xp_scan_list = g_slist_prepend (xp_scan_list, main_get_gapp()->xsm->scan[ch]);
				// got already a master? If not used this one!
				if (!master_scan) master_scan = main_get_gapp()->xsm->scan[ch];
				// got one valid scan, so we could finish if no more... "Scan-OK"
				sok=TRUE;
			}
			else{
				// DAQ src? Find additional scans/channels to aquire...
				// (idaq counts until all checked)
				for(ch = -1; (idaq < DAQCHMAX) && ((ch=main_get_gapp()->xsm->FindChan(xsmres.daqchno[idaq++])) < 0););
				if(ch >= 0){ // got one!
					// add to xp_srcs mask... use "MUXA" (DA0..3)
					if(idaq <= 4)
						xp_srcs |= MSK_MUXA(idaq-1);
					else{
						// add to xp_srcs mask... use "MUXB" (DA4..7)
						if(idaq <= 8)
							xp_srcs |= MSK_MUXB(idaq-5);
						else
							// add to xp_srcs mask... use "AUX" (Auxillary DSP-Data)
							xp_srcs |= MSK_AUX(idaq-9);
					}
					PI_DEBUG (DBG_L3, "rhk_SCANCONTROL::initialize_scan_lists : Setting up XpScan - DAQ src found");
					// setup this scan
					setup_scan (ch, "X+",
						    xsmres.daqsrc[idaq-1],
						    xsmres.daqZunit[idaq-1], 
						    xsmres.daqZlabel[idaq-1]
						);
					// add to list...
					xp_scan_list = g_slist_prepend (xp_scan_list, main_get_gapp()->xsm->scan[ch]);
					if (!master_scan) master_scan = main_get_gapp()->xsm->scan[ch];
					sok=TRUE;
				}
				else{
					// PRB src? Similar stuff for Probe type Scans...
					for(ch = -1; (iprb < PRBMODESMAX) && ((ch=main_get_gapp()->xsm->FindChan(xsmres.prbchno[iprb++])) < 0););
					if(ch >= 0){
						PI_DEBUG (DBG_L3, "rhk_SCANCONTROL::initialize_scan_lists : Setting up XpScan - PRB src found");

						setup_scan (ch, "X+",
							    xsmres.prbid[iprb-1],
							    xsmres.prbYunit[iprb-1], 
							    xsmres.prbYlabel[iprb-1],
							    xsmres.prbXunit[iprb-1], 
							    xsmres.prbXlabel[iprb-1],
							    xsmres.prbsrcs[iprb-1], 
							    xsmres.prboutp[iprb-1]
							);
						xp_prbscan_list = g_slist_prepend (xp_prbscan_list, main_get_gapp()->xsm->scan[ch]);
						// if first probe scan found, use this as probe master
						if (!master_probescan) master_probescan = main_get_gapp()->xsm->scan[ch];
						// note that we have to run in probe mode!
						do_probe = TRUE;
						sok=TRUE;
					}
				}
			}
		}
    
		// Find and init Xm-Scans
		// similar channel analysis for all XM (<-) scans... -- no probing in this direction!
		for (ipid=idaq=i=0; i<MAXSCANS; i++){
			// new PID src?
			for(ch = -1; (ipid < PIDCHMAX) && ((ch=main_get_gapp()->xsm->FindChan(-xsmres.pidchno[ipid++])) < 0););

			if(ch >= 0){
				PI_DEBUG (DBG_L3, "rhk_SCANCONTROL::initialize_scan_lists : Setting up XmScan - PID src found");
				xm_srcs |= MSK_PID(ipid-1);

				setup_scan (ch, "X-", 
					    xsmres.pidsrc[ipid-1],
					    xsmres.pidsrcZunit[ipid-1], 
					    xsmres.pidsrcZlabel[ipid-1]
					);
				xm_scan_list = g_slist_prepend (xm_scan_list, main_get_gapp()->xsm->scan[ch]);
				if (!master_scan) master_scan = main_get_gapp()->xsm->scan[ch];
				sok=TRUE;
			}
			else{
				for(ch = -1; (idaq < DAQCHMAX) && ((ch=main_get_gapp()->xsm->FindChan(-xsmres.daqchno[idaq++])) < 0););
				if(ch >= 0){
					if(idaq <= 4)
						xm_srcs |= MSK_MUXA(idaq-1);
					else{
						if(idaq <= 8)
							xm_srcs |= MSK_MUXB(idaq-5);
						else
							xm_srcs |= MSK_AUX(idaq-9);
					}
					
					PI_DEBUG (DBG_L3, "rhk_SCANCONTROL::initialize_scan_lists : Setting up XmScan - DAQ src found");
					setup_scan (ch, "X-",
						    xsmres.daqsrc[idaq-1], 
						    xsmres.daqZunit[idaq-1],
						    xsmres.daqZlabel[idaq-1]
						);
					xm_scan_list = g_slist_prepend (xm_scan_list, main_get_gapp()->xsm->scan[ch]);
					if (!master_scan) master_scan = main_get_gapp()->xsm->scan[ch];
					sok=TRUE;
				}
				else{
					// PRB src?
					for(ch = -1; (iprb < PRBMODESMAX) && ((ch=main_get_gapp()->xsm->FindChan(xsmres.prbchno[iprb++])) < 0););
					if(ch >= 0){
						PI_DEBUG (DBG_L3, "rhk_SCANCONTROL::initialize_scan_lists : Setting up XmScan - PRB src found");
#ifdef ____NO_PRB_IN_XM_DIR____
						setup_scan (ch, "X-",
							    xsmres.prbid[iprb-1],
							    xsmres.prbYunit[iprb-1], 
							    xsmres.prbYlabel[iprb-1],
							    xsmres.prbXunit[iprb-1], 
							    xsmres.prbXlabel[iprb-1],
							    TRUE
							);
						xm_prbscan_list = g_slist_prepend (xm_prbscan_list, main_get_gapp()->xsm->scan[ch]);
						if (!master_probescan) master_probescan = main_get_gapp()->xsm->scan[ch];
						do_probe = FALSE; // not allowed jet!!!
						sok=TRUE;
#else
						PI_DEBUG (DBG_L2, "Sorry: no Prb in X- direction!!!");
#endif
					}
				}
			}
		}
		
		// Automatic mode:
		// if no Scansrc specified -- find free channel and use pid-default ("Topo")
		if(! sok){
			PI_DEBUG (DBG_L3, "rhk_SCANCONTROL::initialize_scan_lists : No Srcs specified, setting up default!");
			ch = main_get_gapp()->xsm->FindChan(ID_CH_M_ACTIVE);
			if(!(ch >= 0)){
				ch = main_get_gapp()->xsm->FindChan(ID_CH_M_MATH);
				if(!(ch >= 0))
					ch = main_get_gapp()->xsm->FindChan(ID_CH_M_OFF);
				else{
					scan_flag = SCAN_FLAG_RUN;
					XSM_SHOW_ALERT(ERR_SORRY, ERR_NOFREECHAN,"",1);
					return FALSE;
				}
			}
			main_get_gapp()->channelselector->SetMode(ch, xsmres.piddefault);
			main_get_gapp()->xsm->ChannelMode[ch] =  xsmres.piddefault;
			main_get_gapp()->xsm->ChannelScanMode[ch] = main_get_gapp()->xsm->ChannelScanMode[ch] < 0 ? -xsmres.piddefault : xsmres.piddefault;
		}
	}while (! sok && --checks);

	// if sth, went wrong... (should never happen)
	if (!checks)
		XSM_SHOW_ALERT (ERR_SORRY, ERR_NOSRCCHAN, "", 1);

	return 0;
}

int rhk_ScanControl::setup_scan (int ch, 
				 const gchar *titleprefix, 
				 const gchar *name,
				 const gchar *unit,
				 const gchar *label,
				 const gchar *vunit,
				 const gchar *vlabel,
				 const gchar *prbsrcs,
				 int prboutp
	){
	PI_DEBUG (DBG_L2, "setup_scan");
	// did this scan already exists?
	if ( ! main_get_gapp()->xsm->scan[ch]){ // make a new one ?
		main_get_gapp()->xsm->scan[ch] = main_get_gapp()->xsm->NewScan (main_get_gapp()->xsm->ChannelView[ch],
							  main_get_gapp()->xsm->data.display.ViewFlg,
							  ch, 
							  &main_get_gapp()->xsm->data);
		// Error ?
		if (!main_get_gapp()->xsm->scan[ch]){
			XSM_SHOW_ALERT (ERR_SORRY, ERR_NOMEM,"",1);
			return FALSE;
		}
	}

	// Setup correct Z unit
	UnitObj *u = main_get_gapp()->xsm->MakeUnit (unit, label);
	main_get_gapp()->xsm->scan[ch]->data.SetZUnit (u);
	delete u;

	// Probe stuff to do?
	if (prbsrcs){
		PI_DEBUG (DBG_L2, "setup_scan: analyse prbsrcs=" << prbsrcs);
		Mem2d *m=main_get_gapp()->xsm->scan[ch]->mem2d;
		m->Resize (m->GetNx (), m->GetNy (), m->GetNv (), ZD_FLOAT);
		main_get_gapp()->xsm->scan[ch]->create (TRUE, TRUE);
		main_get_gapp()->xsm->scan[ch]->data.lp.srcs  = 0;
		main_get_gapp()->xsm->scan[ch]->data.lp.nsrcs = 0;
		main_get_gapp()->xsm->scan[ch]->data.lp.outp  = prboutp;
		for (int k = 0; k < (int)strlen (prbsrcs); ++k)
			if (prbsrcs[k] == '1'){
				main_get_gapp()->xsm->scan[ch]->data.lp.srcs |= 1<<k;
				main_get_gapp()->xsm->scan[ch]->data.lp.nsrcs++;
			}
		UnitObj *u = main_get_gapp()->xsm->MakeUnit (vunit, vlabel);
		main_get_gapp()->xsm->scan[ch]->data.SetVUnit (u);
		delete u;
		PI_DEBUG (DBG_L2, "PRB: nsrcs=" << main_get_gapp()->xsm->scan[ch]->data.lp.nsrcs
		     << "  srcs=" << main_get_gapp()->xsm->scan[ch]->data.lp.srcs
		     << "  outp=" << main_get_gapp()->xsm->scan[ch]->data.lp.outp
		     << "  nvalues=" << main_get_gapp()->xsm->scan[ch]->data.lp.nvalues);
	}
	else
		main_get_gapp()->xsm->scan[ch]->create (TRUE);

	// setup dz from instrument definition
	main_get_gapp()->xsm->scan[ch]->data.s.dz = main_get_gapp()->xsm->Inst->ZResolution (unit);

	// set scan title, name, ... and draw it!
	gchar *scantitle = g_strdup_printf ("%s %s", titleprefix, name);
	main_get_gapp()->xsm->scan[ch]->data.ui.SetName (scantitle);
	main_get_gapp()->xsm->scan[ch]->data.ui.SetOriginalName ("unknown");
	main_get_gapp()->xsm->scan[ch]->data.ui.SetTitle (scantitle);
	main_get_gapp()->xsm->scan[ch]->data.ui.SetType (scantitle);
	g_free (scantitle);
	main_get_gapp()->xsm->scan[ch]->draw ();

	return 0;
}

int rhk_ScanControl::prepare_to_start_scan (SCAN_DT_TYPE st){
	// which origin mode?
	YOriginTop = IS_SPALEED_CTRL ? FALSE : TRUE;

	scan_flag = SCAN_FLAG_RUN;

	main_get_gapp()->SetStatus ("Starting Scan: Ch.Setup");
	main_get_gapp()->check_events ("Scan Start Channel Setup");
    
	// update hardware parameters
	this->updateRHK();
	main_get_gapp()->xsm->data.s.dz = main_get_gapp()->xsm->Inst->ZResolution ();
	main_get_gapp()->xsm->data.hardpars.LS_dnx = R2INT (main_get_gapp()->xsm->Inst->XA2Dig (main_get_gapp()->xsm->data.s.dx));
	main_get_gapp()->xsm->data.hardpars.LS_nx2scan = main_get_gapp()->xsm->data.s.nx;
	main_get_gapp()->xsm->data.hardpars.SPA_Length = R2INT (main_get_gapp()->xsm->Inst->XA2Dig (main_get_gapp()->xsm->data.s.rx));

	main_get_gapp()->SignalStartScanEventToPlugins ();


	// Init Scanobjects, do Channelsetup... some complex stuff...
	int result = initialize_scan_lists ();;

	if (result){
		XSM_SHOW_ALERT (ERR_SORRY, ERR_SCAN_CANCEL,
				"Channel setup failed.",
				1);
		return result;
	}

	// count channels and check if total data amount fits into hardware/transferbuffer/etc. hard limits
	int ns_xp=0;
	int ns_xm=0;
	for (int i=0; i<16; ++i){
		if (xp_srcs&(1<<i)) ++ns_xp;
		if (xm_srcs&(1<<i)) ++ns_xm;
	}

	// Check Ranges/Sizes with Hardware/DSP Memory
	switch (st){
	case SCAN_LINESCAN:
		if (main_get_gapp()->xsm->data.s.nx*ns_xp >= main_get_gapp()->xsm->hardware->GetMaxPointsPerLine ()){
			gchar *maxtxt = g_strdup_printf ("Hardware limits points per line to\n"
							 "%ld in total for all channels (->)!",
							 main_get_gapp()->xsm->hardware->GetMaxPointsPerLine());
			XSM_SHOW_ALERT (ERR_SORRY,ERR_SCAN_CANCEL, maxtxt, 1);
			g_free (maxtxt);
			return -1;
		}
		if (main_get_gapp()->xsm->data.s.nx*ns_xm >= main_get_gapp()->xsm->hardware->GetMaxPointsPerLine ()){
			gchar *maxtxt = g_strdup_printf ("Hardware limits points per line to\n"
							 "%ld in total for all channels (<-)!",
							 main_get_gapp()->xsm->hardware->GetMaxPointsPerLine());
			XSM_SHOW_ALERT (ERR_SORRY,ERR_SCAN_CANCEL, maxtxt, 1);
			g_free (maxtxt);
			return -1;
		}
		return 0;
	case SCAN_FRAMECAPTURE:
		if (main_get_gapp()->xsm->data.s.nx*main_get_gapp()->xsm->data.s.ny*ns_xp >= main_get_gapp()->xsm->hardware->GetMaxPointsPerLine ()){
			gchar *maxtxt = g_strdup_printf ("Hardware limits points per frame to\n"
							 "%ld in total for all channels (->)!",
							 main_get_gapp()->xsm->hardware->GetMaxPointsPerLine());
			XSM_SHOW_ALERT (ERR_SORRY,ERR_SCAN_CANCEL, maxtxt, 1);
			g_free (maxtxt);
			return -1;
		}
		if (main_get_gapp()->xsm->data.s.nx*main_get_gapp()->xsm->data.s.ny*ns_xm >= main_get_gapp()->xsm->hardware->GetMaxPointsPerLine ()){
			gchar *maxtxt = g_strdup_printf ("Hardware limits points per frame to\n"
							 "%ld in total for all channels (<-)!",
							 main_get_gapp()->xsm->hardware->GetMaxPointsPerLine());
			XSM_SHOW_ALERT (ERR_SORRY,ERR_SCAN_CANCEL, maxtxt, 1);
			g_free (maxtxt);
			return -1;
		}
		return 0;
	default: return -1;
	}
}

// run "background" update function(s)
void IdleRefreshFunc (gpointer data){
	g_slist_foreach ((GSList*) ((IdleRefreshFuncData*)data)->scan_list,
			 (GFunc) ((IdleRefreshFuncData*)data)->UpdateFunc,
			 ((IdleRefreshFuncData*)data)->data);
}

// execute a single scan line -- or if "line == -1" a HS 2D Area Capture/Scan
void rhk_ScanControl::do_scanline (int init){
	static Mem2d **m2d_xp=NULL;
	static Mem2d **m2d_xm=NULL;
	static IdleRefreshFuncData idf_data;

	// if first time called/first line, do some local (static vars) initializations here!
	if (init){
		int num;
		PI_DEBUG (DBG_L2, "rhk_ScanControl::do_scanline : init");
		if (m2d_xp){
			g_free (m2d_xp);
			m2d_xp = NULL;
		}
		if (m2d_xm){
			g_free (m2d_xm);
			m2d_xm = NULL;
		}
		if( line < 0)
			return;

		num = g_slist_length (xp_scan_list);
		if (num>0){
			Mem2d **m;
			m = m2d_xp = (Mem2d**) g_new (Mem2d*, num+1);
			GSList* tmp = g_slist_copy (xp_scan_list);
			GSList* rev = tmp = g_slist_reverse(tmp);
			while (tmp){
				*m++ = ((Scan*)tmp->data)->mem2d;
				tmp = g_slist_next (tmp);
			}
			*m = NULL;
			g_slist_free (rev);
		}

		num = g_slist_length (xm_scan_list);
		PI_DEBUG (DBG_L2, "rhk_ScanControl::do_scanline : init, num xm=" << num);
		if (num > 0){
			Mem2d **m;
			m = m2d_xm = (Mem2d**) g_new (Mem2d*, num+1);
			GSList* tmp = g_slist_copy (xm_scan_list);
			GSList* rev = tmp = g_slist_reverse(tmp);
			while (tmp){
				*m++ = ((Scan*)tmp->data)->mem2d;
				tmp = g_slist_next (tmp);
			}
			*m = NULL;
			g_slist_free (rev);
		}
		idf_data.scan_list  = NULL;
		idf_data.UpdateFunc = (GFunc) rhk_ScanControl::call_scan_draw_line;
		idf_data.data = this;
		PI_DEBUG (DBG_L2, "rhk_ScanControl::do_scanline : init done");

		return;
	}

	if (line == -1){ // HS Capture -- Area Scan!!!
		// do X+ Scan ?
		if (m2d_xp)
			main_get_gapp()->xsm->hardware->ScanLineM (-1,  1, xp_srcs, m2d_xp, ix0off);

		// do X- Scan ?
		if (m2d_xm)
			PI_DEBUG (DBG_L2,"Sorry -- not in HS Capture mode");

		return;
	}

	// do Scanline (XP (+))
	// and display new Data
	
	if (do_probe){ // in probe mode?
//		main_get_gapp()->xsm->hardware->SetIdleFunc (NULL, NULL);
		if ((line % master_probescan->data.lp.n_subsample) == 0){
			PI_DEBUG (DBG_L2,"RHKScanPlugin: do_scanline, STS scan " << line << " SubSmp:" << master_probescan->data.lp.n_subsample);
			// split line in segments and do each separate, then do probing, ...
			// only in forward direction now...
			for (int k = 0;
			     k < master_scan->data.s.nx && scan_flag != SCAN_FLAG_STOP;
			     k += master_probescan->data.lp.n_subsample){

				PI_DEBUG (DBG_L2,"RHKScanPlugin: do_scanline - SubSegment " << k);
				// do probe
				PI_DEBUG (DBG_L2,"RHKScanPlugin: do_scanline - SubSegment Probe here");
				main_get_gapp()->xsm->hardware->PauseScan2D ();
				run_probe (k/master_probescan->data.lp.n_subsample,
					   line/master_probescan->data.lp.n_subsample);

				// Enable sub line scan
				PI_DEBUG (DBG_L2,"RHKScanPlugin: do_scanline - SubSegment Scan...");

				if (k+master_probescan->data.lp.n_subsample >= master_scan->data.s.nx)
					set_subscan (k, master_scan->data.s.nx-k);
				else
					set_subscan (k, master_probescan->data.lp.n_subsample);
				main_get_gapp()->xsm->hardware->ResumeScan2D ();
				
				// do X+ Scan segment?
				if (m2d_xp){

					main_get_gapp()->xsm->hardware->ScanLineM(line,  1, xp_srcs, m2d_xp, k);
				}
			}

			PI_DEBUG (DBG_L2,"RHKScanPlugin: do_scanline, STS scan - update, draw");

			line2update = line/master_probescan->data.lp.n_subsample;
			idf_data.scan_list =  xp_prbscan_list;
			main_get_gapp()->xsm->hardware->SetIdleFunc (&IdleRefreshFunc, &idf_data);
			
			// Reset sub line scan
			main_get_gapp()->xsm->hardware->PauseScan2D ();
			set_subscan ();
			main_get_gapp()->xsm->hardware->ResumeScan2D ();
		}else{
			// do full inter subgrid line
			PI_DEBUG (DBG_L2, "RHKScanPlugin: do_scanline - SubScanmode, Scanning interline " << line);
			// do X+ Scan?
			if (m2d_xp){
				main_get_gapp()->xsm->hardware->ScanLineM (line,  1, xp_srcs, m2d_xp, ix0off);
				line2update = line;
				idf_data.scan_list = xp_scan_list;
				main_get_gapp()->xsm->hardware->SetIdleFunc (&IdleRefreshFunc, &idf_data);
			}
		}
	}else{ // usual scan, line by line
		// do X+ Scan?
		if (m2d_xp){
			main_get_gapp()->xsm->hardware->ScanLineM (line,  1, xp_srcs, m2d_xp, ix0off);
			// setup idle func -- executed on next wait for data!
			line2update = line;
			idf_data.scan_list = xp_scan_list;
			main_get_gapp()->xsm->hardware->SetIdleFunc (&IdleRefreshFunc, &idf_data);
		}


		// do X- Scan ?
		if (m2d_xm){
			line2update = line;
			main_get_gapp()->xsm->hardware->ScanLineM(line, -1, xm_srcs, m2d_xm, ix0off);
			idf_data.scan_list = xm_scan_list;
			main_get_gapp()->xsm->hardware->SetIdleFunc (&IdleRefreshFunc, &idf_data);
		} 
	}

	autosave_check (update_status_info ());
}

void rhk_ScanControl::run_probe (int ipx, int ipy){
	if (!xp_prbscan_list) return;

	PI_DEBUG (DBG_L2,"RHKScanPlugin: run_probe " << ipx << " " << ipy);
	// for all probe scans...
	for (GSList* tmp = xp_prbscan_list; tmp; tmp = g_slist_next (tmp)){
		Scan *ps = (Scan*)tmp->data;

		// set probe parameters
		PARAMETER_SET hardpar;
		hardpar.N   = DSP_PRBNAVE+1;
		hardpar.Cmd = DSP_CMD_PROBESCAN;
		hardpar.hp[DSP_PRBSRCS    ].value = ps->data.lp.srcs; // Codierte MUX/Srcs Configuration
		hardpar.hp[DSP_PRBOUTP    ].value = ps->data.lp.outp; // Channel to Probe
		hardpar.hp[DSP_PRBNX      ].value = ps->data.lp.nvalues;
		hardpar.hp[DSP_PRBXS      ].value = ps->data.lp.value_start; 
		hardpar.hp[DSP_PRBXE      ].value = ps->data.lp.value_end; 
		hardpar.hp[DSP_PRBACAMP   ].value = ps->data.lp.ac_amp;
		hardpar.hp[DSP_PRBACFRQ   ].value = ps->data.lp.ac_frq;
		hardpar.hp[DSP_PRBACPHASE ].value = ps->data.lp.ac_phase;
		hardpar.hp[DSP_PRBACMULT  ].value = ps->data.lp.ac_mult = 1.0; // fixed to 1 for now.
		hardpar.hp[DSP_PRBDELAY   ].value = 0.;
		hardpar.hp[DSP_PRBCIVAL   ].value = 0.;
		hardpar.hp[DSP_PRBNAVE    ].value = ps->data.lp.nAvg;
		hardpar.hp[DSP_PRBGAPADJ  ].value = ps->data.lp.gap_adj/main_get_gapp()->xsm->Inst->ZResolution(); // convert to DigUnits!!!!
		PI_DEBUG (DBG_L2,"PRB scan: Start");
		// set data and run probe...
		main_get_gapp()->xsm->hardware->SetParameter (hardpar, FALSE);
		PI_DEBUG (DBG_L2,"PRB scan: ACAmp:" << ps->data.lp.ac_amp);
		// get data
		main_get_gapp()->xsm->hardware->ReadProbeData (ps->data.lp.nsrcs, ps->data.lp.nvalues, ipx, ipy,
						    ps->mem2d, 1./ps->data.lp.ac_mult);
	}
}

void rhk_ScanControl::set_subscan (int ix0, int num){
	int n;
	ix0off = ix0;
	if(ix0 >= 0 && num > 0){ printf("Setting subscan!!!\n");
		if (master_probescan){
			main_get_gapp()->xsm->hardware->PutParameter(&master_probescan->data, 1);
			n = master_probescan->data.hardpars.LS_nx2scan = num;
			master_probescan->data.hardpars.LS_nx_pre = 0;
			master_probescan->data.hardpars.SPA_Length = R2INT(main_get_gapp()->xsm->Inst->XA2Dig(master_scan->data.s.rx*num/master_scan->data.s.nx));
			
		} else { PI_DEBUG (DBG_L2,"ERROR not probing"); return; }
	} else {
		main_get_gapp()->xsm->hardware->PutParameter (&master_scan->data, 1);
		n = master_scan->data.s.nx;
	}

	PI_DEBUG (DBG_L2, "rhk_ScanControl::set_subscan" << ix0off << ":" << n);

	// Setup Copy Mode in mem2d...
	for (GSList* tmp = xp_scan_list; tmp; tmp = g_slist_next (tmp))
		((Scan*)tmp->data) -> mem2d->data->ZPutDataSetDest (ix0off, n);

	for (GSList* tmp = xm_scan_list; tmp; tmp = g_slist_next (tmp))
		((Scan*)tmp->data) -> mem2d->data->ZPutDataSetDest (ix0off, n);
}	

int rhk_ScanControl::do_scan (){
	int yline;

	PI_DEBUG (DBG_L2, "do_scan");

	if (scan_in_progress ()){
		PI_DEBUG (DBG_L2, "do_scan scan in progress, exiting.");
		return FALSE;
	}

	if (prepare_to_start_scan ()){
		PI_DEBUG (DBG_L2, "prepare scan failed, exiting.");
		stop_scan ();
		free_scan_lists ();
		return FALSE;
	}

	PI_DEBUG (DBG_L2, "do_scan precheck done.");

	// now freeze all scanparameters
	main_get_gapp()->spm_freeze_scanparam();

	// Set Start Time, notify scans about, initialisations...
	g_slist_foreach ((GSList*) xp_scan_list,
			(GFunc) rhk_ScanControl::call_scan_start, this);
	g_slist_foreach ((GSList*) xm_scan_list,
			(GFunc) rhk_ScanControl::call_scan_start, this);

	main_get_gapp()->xsm->hardware->StartScan2D();

	line = 0;
	do_scanline (TRUE);
	update_status_info (TRUE);
	autosave_check (0., xsmres.AutosaveValue);
	set_subscan ();

	// copy muxsettings from other direction if not given
	// weird... to fix !!!
	if(!xm_srcs) xm_srcs = xp_srcs;
	if(!xp_srcs) xp_srcs = xm_srcs;
	
	PI_DEBUG (DBG_L2, "DoScan: Setup MUX");
    
	// MUX-Codierung:
#ifdef XSM_DEBUG_OPTION
	PI_DEBUG (DBG_L2,  "xp_srcs: " << xp_srcs );
	for(int i=0; i<16; i++) PI_DEBUG (DBG_L3, (int)((xp_srcs&(1<<i))?1:0));
	PI_DEBUG (DBG_L2,  "xm_srcs: " << xm_srcs );
	for(int i=0; i<16; i++) PI_DEBUG (DBG_L3, (int)((xm_srcs&(1<<i))?1:0));
#endif

	PI_DEBUG (DBG_L2, "DoScan: Start Scan now");

	{
		gchar *muxcoding = g_strdup_printf ("xp_srcs: %2X  xm_srcs: %2X", xp_srcs, xm_srcs);
		main_get_gapp()->monitorcontrol->LogEvent ("*StartScan", muxcoding);
		g_free (muxcoding);
	}

	// run scan now...
	for (line=0; (line<master_scan->data.s.ny) && scan_flag != SCAN_FLAG_STOP;){

		if (scan_flag == SCAN_FLAG_RUN){
//			PI_DEBUG (DBG_L2, "Running Line" << line);
			// execute scan line
			do_scanline ();
			++line;
		}
		main_get_gapp()->check_events_self();
	}

	// finish scan
	main_get_gapp()->xsm->hardware->EndScan2D ();
	main_get_gapp()->xsm->hardware->CallIdleFunc ();
	main_get_gapp()->xsm->hardware->SetIdleFunc (NULL, NULL);

	if( line < master_scan->data.s.ny && line&1)
		line++;

	// Set Scan End Time/trucate unfinished scans to save space

	g_slist_foreach ((GSList*) xp_scan_list,
			(GFunc) rhk_ScanControl::call_scan_stop, this);
	g_slist_foreach ((GSList*) xm_scan_list,
			(GFunc) rhk_ScanControl::call_scan_stop, this);

	return finish_scan ();
}

int rhk_ScanControl::do_hscapture (){
	if (scan_in_progress ()){
		PI_DEBUG (DBG_L2, "do_scan scan in progress, exiting.");
		return FALSE;
	}

	if (prepare_to_start_scan (SCAN_FRAMECAPTURE)){
		PI_DEBUG (DBG_L2, "prepare scan failed, exiting.");
		stop_scan ();
		free_scan_lists ();
		return FALSE;
	}

	// now freeze all scanparameters
	main_get_gapp()->spm_freeze_scanparam();

	// Set Start Time, notify scans about, initialisations...
	g_slist_foreach ((GSList*) xp_scan_list,
			(GFunc) rhk_ScanControl::call_scan_start, this);
	g_slist_foreach ((GSList*) xm_scan_list,
			(GFunc) rhk_ScanControl::call_scan_start, this);

	main_get_gapp()->xsm->hardware->StartScan2D();

	line = 0;
	do_scanline (TRUE);
	update_status_info (TRUE);

	// copy muxsettings from other direction if not given
	// weird... to fix !!!
	if(!xm_srcs) xm_srcs = xp_srcs;
	if(!xp_srcs) xp_srcs = xm_srcs;
	
	// MUX-Codierung:
	PI_DEBUG (DBG_L2,  "xp_srcs: " << xp_srcs );
	for(int i=0; i<16; i++) PI_DEBUG (DBG_L3,(int)((xp_srcs&(1<<i))?1:0));
	PI_DEBUG (DBG_L2,  "xm_srcs: " << xm_srcs );
	for(int i=0; i<16; i++) PI_DEBUG (DBG_L3, (int)((xm_srcs&(1<<i))?1:0));

	{
		gchar *muxcoding = g_strdup_printf ("xp_srcs: %2X  xm_srcs: %2X", xp_srcs, xm_srcs);
		main_get_gapp()->monitorcontrol->LogEvent ("*StartHSCapture", muxcoding);
		g_free (muxcoding);
	}

		// execute high speed capture scan (line = -1 indicates this mode)
		if (scan_flag!=SCAN_FLAG_STOP) {
		line = -1;
		do_scanline ();
		main_get_gapp()->check_events_self();
	}

	main_get_gapp()->xsm->hardware->EndScan2D ();
	line = master_scan->data.s.ny;
	g_slist_foreach ((GSList*) xp_scan_list,
			(GFunc) rhk_ScanControl::call_scan_stop, this);
	g_slist_foreach ((GSList*) xm_scan_list,
			(GFunc) rhk_ScanControl::call_scan_stop, this);

	return finish_scan ();
}

int rhk_ScanControl::finish_scan (){
	int stopped;
	PI_DEBUG (DBG_L2, "finish_scan.");

	main_get_gapp()->spm_thaw_scanparam();

	if ((stopped = scan_flag == SCAN_FLAG_STOP ? TRUE:FALSE)){
		main_get_gapp()->SetStatus("Scan interrupted");
		main_get_gapp()->monitorcontrol->LogEvent("*EndOfScan", "interrupted");
	}
	else{
		main_get_gapp()->SetStatus("Scan done, ready");
		main_get_gapp()->monitorcontrol->LogEvent("*EndOfScan", "OK");
	}

	free_scan_lists ();
	scan_flag = SCAN_FLAG_READY;


	return !stopped;
}

double rhk_ScanControl::update_status_info (int reset){
	static time_t tn, tnlog, t0;
	time_t t;

	if (reset){
		time (&t0);
		tnlog=tn=t0;
		return 0.;
	}

	time (&t);
	// update only of more than 2s old
	if ((t-tn) > 2){
		double ss,s,m,h, sse,se,me,he;
		double n_by_lines_to_do;
		gchar *tt;
		tn=t;
		ss = s = tn-t0;
		h = floor (s/3600.);
		m = floor ((s-h*3600.)/60.);
		s-= h*3600.+m*60.;

		n_by_lines_to_do = (double)master_scan->data.s.ny
			/ (double)(line+1);
		sse = se = (double)(tn-t0)*n_by_lines_to_do;
		he  = floor (se/3600.);
		me  = floor ((se-he*3600.)/60.);
		se -= he*3600.+me*60.;
		// 0123456789012345678901234567890
		// Mon Apr 17 09:10:17 2000
		if ((sse+ss) > 600){
			char *teos = ctime(&t);
			teos[24]=0;
			main_get_gapp()->SetStatus ("ScanTime",
					 tt=g_strdup_printf ("%.0f:%02.0f:%02.0f "
							     "| ETA:%.0f:%02.0f:%02.0f | %d%%\n"
							     "End of Scan: %s",
							     h,m,s, he,me,se,
							     (int)(100/n_by_lines_to_do), teos ));
		}else
			main_get_gapp()->SetStatus ("ScanTime",
					 tt=g_strdup_printf ("%.0f:%02.0f:%02.0f "
							     "| ETA:%.0f:%02.0f:%02.0f | %d%%",
							     h,m,s, he,me,se,
							     (int)(100/n_by_lines_to_do)));

		if ((t-tnlog) > 60 || (sse+ss) < 600){
			tnlog=t;
			gchar *nl;
			if ((nl=strchr (tt, '\n'))) *nl=' '; // remove \n for logfile
			gchar *mt = g_strdup_printf ("Scan Progress [%d]", line);
			main_get_gapp()->monitorcontrol->LogEvent (mt, tt);
			g_free(mt);
		}
		g_free(tt);

		t=tn+(time_t)sse;
		return ss;
	}
	return 0.;
}

void rhk_ScanControl::autosave_check (double sec, int initvalue){
	static int nextAutosaveEvent=0;

	if (initvalue > 0){
		nextAutosaveEvent = initvalue;
		return;
	}

	if ( (( !strncasecmp(xsmres.AutosaveUnit, "percent",2))
	      && ((100*line/master_scan->data.s.ny) >= nextAutosaveEvent))
	     ||
	     (( !strncasecmp(xsmres.AutosaveUnit, "lines"  ,2))
	      && (line >= nextAutosaveEvent))
	     ||
	     ((!strncasecmp(xsmres.AutosaveUnit, "seconds",2))
	      && (sec >= nextAutosaveEvent)) ){

		nextAutosaveEvent += xsmres.AutosaveValue;
		if(main_get_gapp()->xsm->IsMode(MODE_AUTOSAVE)){
			PI_DEBUG (DBG_L2,  "Autosaveevent triggered.");
			// check for overwritemode for autosave
			if(!strncasecmp(xsmres.AutosaveOverwritemode, "true",2))
				main_get_gapp()->xsm->save(AUTO_NAME_PARTIAL_SAVE, NULL, -1, TRUE);
			else
				main_get_gapp()->xsm->save(AUTO_NAME_PARTIAL_SAVE, NULL, -1, FALSE);
		}
	}

}


void rhk_ScanControl::updateRHK(void){
	  static GdkRectangle update_rect={0,0,300,300};

  gdk_draw_rectangle (pixmap,
                      widget->style->white_gc,
                      TRUE,
                      update_rect.x, update_rect.y,
                      update_rect.width, update_rect.height);
		      
// This is a nasty kludge. The problem is that PutParameter returns the readings in Dig, not in Angstroms,
// as it does not know about the calibration
// So we translate from Dig readings to Angstroms before showing the settings anywhere
  main_get_gapp()->xsm->hardware->PutParameter(ScanData, 1);
  
   update_rect.width = (gint)(150.0*ScanData->s.rx/32768.0);
  update_rect.height = (gint)(150.0*ScanData->s.ry/32768.0);
  update_rect.x = (gint)(150.0*ScanData->s.x0/32768.0+150-update_rect.width);
  update_rect.y = (gint)(150.0*ScanData->s.y0/32768.0+150-update_rect.height);
  update_rect.width*=2;
  update_rect.height*=2;

  gdk_draw_rectangle (pixmap,
                      widget->style->black_gc,
                      TRUE,
                      update_rect.x, update_rect.y,
                      update_rect.width, update_rect.height);

  update_rect.width=update_rect.height=300;
  update_rect.x=update_rect.y=0;
  gtk_widget_draw (drawingarea, &update_rect);

  ScanData->s.x0 = instrument->Dig2XA((gint)ScanData->s.x0);
  ScanData->s.rx = instrument->Dig2XA((gint)ScanData->s.rx);
  ScanData->s.y0 = instrument->Dig2YA((gint)ScanData->s.y0);
  ScanData->s.ry = instrument->Dig2YA((gint)ScanData->s.ry);
  ScanData->s.dz = ScanData->s.rz;
  ScanData->s.rz = instrument->Dig2ZA((gint)ScanData->s.rz);
  ScanData->s.dz = ScanData->s.rz/ScanData->s.dz;
  ScanData->hardpars.UTunnel = instrument->Dig2V((gint)ScanData->hardpars.UTunnel);
  //ScanData->hardpars.ITunnelSoll = instrument->Dig2nAmpere((gint)ScanData->hardpars.ITunnelSoll);
  ScanData->s.dx = ScanData->s.rx / ScanData->s.nx;
  ScanData->s.dy = ScanData->s.ry / ScanData->s.ny;
  
  main_get_gapp()->spm_update_all();
  
  ec1->Set_FromValue(ScanData->s.x0);
  ec2->Set_FromValue(ScanData->s.rx);
  ec3->Set_FromValue(ScanData->s.y0);
  ec4->Set_FromValue(ScanData->s.ry);
  ec5->Set_FromValue(ScanData->s.rz);
  ec6->Set_FromValue(ScanData->s.nx);
  ec7->Set_FromValue(ScanData->hardpars.UTunnel);
  ec8->Set_FromValue(ScanData->hardpars.ITunnelSoll);
  
 
}

void rhk_ScanControl::ChangedNotify(GtkWidget *widget, rhk_ScanControl *dspc){
dspc->updateRHK();
}

void rhk_ScanControl::ChangedAuto(GtkWidget *widget, rhk_ScanControl *dspc){
if (!dspc->timer)
	dspc->timer=gtk_timeout_add(1000, &rhk_ScanControl_timed, dspc);
else {
	gtk_timeout_remove(dspc->timer);
 	dspc->timer=0;
	}
}
