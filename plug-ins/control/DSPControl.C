/* Gnome gxsm - Gnome X Scanning Microscopy
 * universal STM/AFM/SARLS/SPALEED/... controlling and
 * data analysis software
 * 
 * Gxsm Plugin Name: DSPControl.C
 * ========================================
 * 
 * Copyright (C) 1999 The Free Software Foundation
 *
 * Authors: Percy Zahl <zahl@fkp.uni-hannover.de>
 * additional features: Andreas Klust <klust@fkp.uni-hannover.de>
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
 * All "% PlugInXXX" commentary tags are mandatory
 * All "% OptPlugInXXX" tags are optional
 * --------------------------------------------------------------------------------
% PlugInModuleIgnore

% BeginPlugInDocuSection
% PlugInDocuCaption: DSP Feedback and Scan Control (OBSOLETE)
% PlugInName: DSPControl
% PlugInAuthor: Percy Zahl
% PlugInAuthorEmail: zahl@users.sf.net
% PlugInMenuPath: windows-sectionDSP Control

% PlugInDescription
This the main DSP feedback control panel. Here the feedback parameters
and tunneling/force settings are adjusted. The feedback loop can also
be disabled/enabled here. The second purpose of this control panel is
the adjustment of all scan parameters of the digital vector scan
generator like speed and subsampling.

\GxsmScreenShot{DSPControl}{The DSPControl window.}

%% PlugInUsage

% OptPlugInSubSection: Proportional Integral Feedback Loop Control

\begin{figure}[hbt]
\includegraphics[width=10cm]{feedbk}
\caption{Control circuit - scheme (STM)}
\label{feedbk}
\end{figure}

All control parameters of the digital vector scan generator are set in
this dialog. In STM-mode $U$ is the tunnel voltage and $I$ the tunneling
current setpoint. In AFM-mode the setpoint -- that is the voltage or
corresponding force to which the z-force-signal from the PSD should
be adjusted to -- is used for the feedback, $I$ and $U$ are without
function in this case.

The control behaviour -- \menuentry{FeedBack - Characteristics} -- is set
through \textbf{CP} (proportional part) and \textbf{CI} (integral part).

Recipe if no sensible values are known:
\begin{enumerate}
\item Set CP to zero, CI to 0.0005. If you now change the z-offset,
  the control should follow slowly -- typical stable values are around 0.001.
\item Increase CI slowly until the control behaviour begins to become
  unstable. When you abruptly change z-offset the control oscillates.
\item Now decrease CI again until all oscillations are gone.  
\item Set CP to 100\% to 200\% of CI to stabilize the control.
\end{enumerate}
You may increase CI and CP experimentally for quicker response.

\textbf{CS} is a slope correction in the case, that your sample is
tilted towards the intended optimal focus plane.  CS=1 results in the
addition of the mean slope to lighten the load off the
z-signal-correction.  CS=0 prevents this slope correction, CS=0.5 adds
only the half of the extrapolated height, CS$>$1 adds accordingly more
than the average slope would suggest. Usage of the slope parameter CS
is dependent on the DSP software version.

% OptPlugInSubSection: Scan Characteristics

\begin{figure}[hbt]
\includegraphics[width=14cm]{scanandsample}
\caption{Linescan execution}
\label{linescan}
\end{figure}

The dialog allows the user to set the following scan properties:

A 2D-Scan is defined by a parameter set: Size (range X, Y), and a
number of points in X and Y (points x, y). With this information one
can calculate the sample dot distance (dx, dy). This dx and dy have to
be a integer multiple of the smallest possible stepwidth
(\filename{DA-unit}) and, as a consequence, the total range has to be
a multiple of the small stepwidth (dx, resp. dy).
\GxsmNote{In principal this is not necessary, because the integer data 
acquisition position could be rounded up, but it is not really a disadvantage:
It assures a very continous scanning with constant and precise step width.}

If the stepwidth is large, it leads to a smoother movement not
to jump from sample point to sample point, but to define some intermediate
steps (Inter-steps).

Inter-steps contains the stepwidth of intermediate steps in multiples of
DA-units. An entry of 1 means, that the range between two sample points
is covered via steps of the length 1xDA-unit. A smaller value is not
possible.

During a scan for position changes the \GxsmEmph{Line} entry is used, otherwise
the \GxsmEmph{Move} entry.

Additionaly it is possible to stay for some time at any sample point.
The textfield \GxsmEmph{FB-Loops} contains the number of feedback-loops which
halts the scanhead for a limited time. The loop runs with 50 kHz. 

The parameter \GxsmEmph{N} sets the how often at any position a value is sampled.
After these values are measured. The result is their average.

\GxsmEmph{Pre} defined the number of steps, which are scanned before any
data acquisition takes place.

\GxsmEmph{DPL} (dots per line) and \GxsmEmph{dos distance} (in DA-units) cannot be set.
They are FYI only and set at scan-start

%% OptPlugInSection: replace this by the section caption

%% OptPlugInSubSection: replace this line by the subsection caption

% OptPlugInConfig

% OptPlugInKnownBugs
Dual Mode is still beta! Be careful and prepared something worse could
happen\dots! But testing reports are welcome!
\GxsmScreenShot{DSPControl-Dual}{This is how the DSPControl window
looks like in dual mode.}

% OptPlugInNotes
CS usage depends on DSP program version running.

% OptPlugInHints
I have a lot\dots

% EndPlugInDocuSection
 * -------------------------------------------------------------------------------- 
 */

#include <gtk/gtk.h>
#include "config.h"
#include "core-source/plugin.h"

#include "core-source/unit.h"
#include "core-source/pcs.h"
#include "core-source/xsmtypes.h"
#include "core-source/glbvars.h"

#include "include/dsp-pci32/xsm/xsmcmd.h"



					static void DSPControl_about( void );
static void DSPControl_query( void );
static void DSPControl_cleanup( void );

static void DSPControl_show_callback( GtkWidget*, void* );
static void DSPControl_StartScan_callback( gpointer );

GxsmPlugin DSPControl_pi = {
        NULL,
        NULL,
        0,
        NULL,
        "DSPControl",
        "+spmHARD +Innovative_DSP:SPMHARD +STM +AFM +SARLS",
//	"+ALL",
        NULL,
        "Percy Zahl",
        "windows-section",
        N_("DSP Control"),
        N_("open the DSP feedback/scan controlwindow"),
        "DSP feedback control",
        NULL,
        NULL,
        NULL,
        DSPControl_query,
        DSPControl_about,
        NULL,
        NULL,
        DSPControl_cleanup
};

static const char *about_text = N_(
        "Gxsm DSPControl Plugin:\n"
        "This plugin runs a control window to set "
        "the DSP feedback and scan characteristics.\n"
        "The digital feedback can be turned on and off, "
        "All feedback constants (CP, CI, CS (slope) are "
        "controlled here.\n"
        "Via scan characteristics the digital DSP vector scan "
        "generator is programmed."
        );

GxsmPlugin *get_gxsm_plugin_info ( void ){ 
        DSPControl_pi.description = g_strdup_printf(N_("Gxsm DSPControl plugin %s"), VERSION);
        return &DSPControl_pi; 
}


typedef union AmpIndex {
        struct { unsigned char ch, x, y, z; } s;
        unsigned long   l;
};

typedef struct DSP_Param{
	/* Feedback Control */
	double        UTunnel;                /* Tunnelvoltage */
	double        ITunnelSoll;            /* wanted tunnel current */
	double        SetPoint;               /* AFM FB Setpoint */
	double        LogOffset;              /* Offset for Logcontrol */
	double        LogSkl;                 /* Logscale */
	double        usrCP, usrCI, usrCD;    /* control constant User */
	double        CP, CI, CS, tau;        /* control constant DSP (scaled) */
	double        Rotation;               /* rotation angle */
	double        fb_frq;                 /* Timer frequency of control interrupt [Hz] */
	double        fir_fg;                 /* FIR limit frq. / LowPass */
	int           LinLog;                 /* Lin / Log Flag */

	/* Scan Control */
	int   LS_nx2scan;             /* Scanline length */
	int   LS_nx_pre;              /* Scan prerun length */
	int   LS_dnx;                 /* point distance */
	int   LS_stepsize;            /* max. step width */
	int   LS_nRegel;              /* count control runs per step */
	int   LS_nAve;                /* count number for averages */
	int   LS_IntAve;              /* =0: kein IntAve, =1: Aufsummieren von Punkte zu Punkt */

	int   MV_stepsize;            /* max. Schrittweite f<FC>r MoveTo */
	int   MV_nRegel;              /* Anzahl Reglerdurchl<E4>ufe je Schritt */
};

DSP_Param data_hardpars;
DSP_Param data_hardpars_dual;


typedef struct DSP_Param_Mover{
	/* Mover Control */
	double MOV_Ampl, MOV_Speed, MOV_Steps;

	/* Auto Approch */
	int    TIP_nSteps;            /* Anzahl Z Steps (grob) */
	double TIP_Delay;             /* Anzahl Wartezyclen zwischen Z-Steps */
	int    TIP_DUz;               /* Schrittweite f<FC>r Z-Piezo-Spannung "ran" */
	int    TIP_DUzRev;            /* Schrittweite f<FC>r Z-Piezo-Spannung "zur<FC>ck" */

	/* AFM special */
#define DSP_AFMMOV_MODES 4
	double  AFM_Amp, AFM_Speed, AFM_Steps; /* AFM Besocke Kontrolle */
	double  AFM_usrAmp[DSP_AFMMOV_MODES];
	double  AFM_usrSpeed[DSP_AFMMOV_MODES], AFM_usrSteps[DSP_AFMMOV_MODES]; /* Parameter Memory */

	/* SPA-LEED special */
	double SPA_Energy, SPA_EnergyVolt;
	double SPA_Gatetime;
	double SPA_Length;
};
 
class DSPControl : public AppBase{
#define DSP_FB_ON  1
#define DSP_FB_OFF 0
public:
        DSPControl();
        virtual ~DSPControl();
        
        void update();
        void updateDSP(int FbFlg=-1);
        static void ExecCmd(int cmd);
        static void ChangedNotify(Param_Control* pcs, gpointer data);
        static int ChangedAction(GtkWidget *widget, DSPControl *dspc);
        static int feedback_callback(GtkWidget *widget, DSPControl *dspc);
        static int dualmode_callback(GtkWidget *widget, DSPControl *dspc);
        static int choice_Ampl_callback(GtkWidget *widget, DSPControl *dspc);
private:
	GSList *RemoteEntryList;
        UnitObj *Unity, *Volt, *Current, *SetPtUnit;
        DSP_Param *dsp, *dsp_dual;
        GtkWidget *DualSettingsFrame;
};


DSPControl *DSPControlClass = NULL;

static void DSPControl_query(void)
{
	static GnomeUIInfo menuinfo[] = { 
		{ GNOME_APP_UI_ITEM, 
		  DSPControl_pi.menuentry, DSPControl_pi.help,
		  (gpointer) DSPControl_show_callback, NULL,
		  NULL, GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_BLANK, 
		  0, GDK_CONTROL_MASK, NULL },

		GNOMEUIINFO_END
	};

	gnome_app_insert_menus
		( GNOME_APP(DSPControl_pi.app->getApp()), 
		  DSPControl_pi.menupath, menuinfo );

	// new ...
	DSPControlClass = new DSPControl;

	DSPControlClass->SetResName ("WindowDSPControl", "false", xsmres.geomsave);

	DSPControl_pi.app->ConnectPluginToStartScanEvent
		( DSPControl_StartScan_callback );

	DSPControl_pi.status = g_strconcat(N_("Plugin query has attached "),
					   DSPControl_pi.name, 
					   N_(": DSPControl is created."),
					   NULL);
}

static void DSPControl_about(void)
{
	const gchar *authors[] = { "Percy Zahl", NULL};
	gtk_show_about_dialog (NULL, "program-name",  DSPControl_pi.name,
					"version", VERSION,
					  "license", GTK_LICENSE_GPL_3_0,
					  "comments", about_text,
					  "authors", authors,
					  NULL
				));
}

static void DSPControl_cleanup( void ){
	PI_DEBUG (DBG_L2, "DSPControl Plugin Cleanup" );
	gchar *mp = g_strconcat(DSPControl_pi.menupath, DSPControl_pi.menuentry, NULL);
	gnome_app_remove_menus (GNOME_APP( DSPControl_pi.app->getApp() ), mp, 1);
	g_free(mp);

	// delete ...
	if( DSPControlClass )
		delete DSPControlClass ;
}

static void DSPControl_show_callback( GtkWidget* widget, void* data){
	if( DSPControlClass )
		DSPControlClass->show();
}

static void DSPControl_StartScan_callback( gpointer ){
	DSPControlClass->update();
}

// Achtung: Remote is not released :=(
// DSP-Param sollten lokal werden...
DSPControl::DSPControl ()
{
	XsmRescourceManager xrm("innovative_dsp_hwi_control");

	int i,j;
	AmpIndex  AmpI;
	GSList *EC_list=NULL;

	RemoteEntryList = NULL;

	Gtk_EntryControl *ec;

	GtkWidget *box;
	GtkWidget *frame_param, *vbox_param, *hbox_param;

	GtkWidget *input;

	GtkWidget* wid, *label;
	GtkWidget* menu;
	GtkWidget* menuitem;
	GtkWidget *checkbutton;

	Unity    = new UnitObj(" "," ");
	Volt     = new UnitObj("V","V");
	Current  = new UnitObj("nA","nA");
	SetPtUnit = gapp->xsm->MakeUnit (xsmres.daqZunit[0], xsmres.daqZlabel[0]);
	if (strncmp (xsmres.daqZunit[0], "nN", 2) == 0) SetPtUnit->setval ("f", xsmres.nNewton2Volt);
	if (strncmp (xsmres.daqZunit[0], "Hz", 2) == 0) SetPtUnit->setval ("f", xsmres.dHertz2Volt);

	// shorthand
	dsp = &data_hardpars;
	dsp_dual = &data_hardpars_dual;

	// get values from resources
        xrm.Get("data_hardpars.UTunnel", &data_hardpars.UTunnel, "2.0");
        xrm.Get("data_hardpars_dual.UTunnel", &data_hardpars_dual.UTunnel, "2.0");
        xrm.Get("data_hardpars.ITunnelSoll", &data_hardpars.ITunnelSoll, "0.8");
        xrm.Get("data_hardpars_dual.ITunnelSoll", &data_hardpars_dual.ITunnelSoll, "0.8");
        xrm.Get("data_hardpars.SetPoint", &data_hardpars.SetPoint, "-1.0");
        xrm.Get("data_hardpars_dual.SetPoint", &data_hardpars_dual.SetPoint, "-1.0");
        xrm.Get("data_hardpars.LogOffset", &data_hardpars.LogOffset, "100.0");
        xrm.Get("data_hardpars_dual.LogOffset", &data_hardpars_dual.LogOffset, "100.0");
        xrm.Get("data_hardpars.LogSkl", &data_hardpars.LogSkl, "5.0");
        xrm.Get("data_hardpars_dual.LogSkl", &data_hardpars_dual.LogSkl, "5.0");
        xrm.Get("data_hardpars.usrCP", &data_hardpars.usrCP, "0.01");
        xrm.Get("data_hardpars_dual.usrCP", &data_hardpars_dual.usrCP, "0.01");
        xrm.Get("data_hardpars.usrCI", &data_hardpars.usrCI, "0.01");
        xrm.Get("data_hardpars_dual.usrCI", &data_hardpars_dual.usrCI, "0.01");
        xrm.Get("data_hardpars.CS", &data_hardpars.CS, "1.0");
        xrm.Get("data_hardpars_dual.CS", &data_hardpars_dual.CS, "1.0");
        xrm.Get("data_hardpars.Rotation", &data_hardpars.Rotation, "0.0");
        xrm.Get("data_hardpars_dual.Rotation", &data_hardpars_dual.Rotation, "0.0");
        xrm.Get("data_hardpars.fb_frq", &data_hardpars.fb_frq, "50000");
        xrm.Get("data_hardpars_dual.fb_frq", &data_hardpars_dual.fb_frq, "50000");
        xrm.Get("data_hardpars.fir_fg", &data_hardpars.fir_fg, "3000");
        xrm.Get("data_hardpars_dual.fir_fg", &data_hardpars_dual.fir_fg, "3000");
        xrm.Get("data_hardpars.LinLog", &data_hardpars.LinLog, "0");
        xrm.Get("data_hardpars_dual.LinLog", &data_hardpars_dual.LinLog, "0");
        xrm.Get("data_hardpars.LS_nx2scan", &data_hardpars.LS_nx2scan, "1");
        xrm.Get("data_hardpars_dual.LS_nx2scan", &data_hardpars_dual.LS_nx2scan, "1");
        xrm.Get("data_hardpars.LS_nx_pre", &data_hardpars.LS_nx_pre, "10");
        xrm.Get("data_hardpars_dual.LS_nx_pre", &data_hardpars_dual.LS_nx_pre, "10");
        xrm.Get("data_hardpars.LS_dnx", &data_hardpars.LS_dnx, "1");
        xrm.Get("data_hardpars_dual.LS_dnx", &data_hardpars_dual.LS_dnx, "1");
        xrm.Get("data_hardpars.LS_stepsize", &data_hardpars.LS_stepsize, "1");
        xrm.Get("data_hardpars_dual.LS_stepsize", &data_hardpars_dual.LS_stepsize, "1");
        xrm.Get("data_hardpars.LS_nRegel", &data_hardpars.LS_nRegel, "5");
        xrm.Get("data_hardpars_dual.LS_nRegel", &data_hardpars_dual.LS_nRegel, "1");
        xrm.Get("data_hardpars.LS_nAve", &data_hardpars.LS_nAve, "1");
        xrm.Get("data_hardpars_dual.LS_nAve", &data_hardpars_dual.LS_nAve, "1");
        xrm.Get("data_hardpars.LS_IntAve", &data_hardpars.LS_IntAve, "1");
        xrm.Get("data_hardpars_dual.LS_IntAve", &data_hardpars_dual.LS_IntAve, "1");
        xrm.Get("data_hardpars.MV_stepsize", &data_hardpars.MV_stepsize, "1");
        xrm.Get("data_hardpars_dual.MV_stepsize", &data_hardpars_dual.MV_stepsize, "1");
        xrm.Get("data_hardpars.MV_nRegel", &data_hardpars.MV_nRegel, "5");
        xrm.Get("data_hardpars_dual.MV_nRegel", &data_hardpars_dual.MV_nRegel, "1");



	AppWindowInit(N_("DSP Control"));

	box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	gtk_widget_show (box);
	gtk_box_pack_start (GTK_BOX (vbox), box, TRUE, TRUE, 0);

	// ========================================
	frame_param = gtk_frame_new (N_("FB Characteristics for <-> or -> Scandir in Dual Mode"));
	gtk_widget_show (frame_param);
	gtk_container_add (GTK_CONTAINER (box), frame_param);

	vbox_param = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	gtk_widget_show (vbox_param);
	gtk_container_add (GTK_CONTAINER (frame_param), vbox_param);

	// ------- FB Characteristics

	hbox_param = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_widget_show (hbox_param);
	gtk_container_add (GTK_CONTAINER (vbox_param), hbox_param);
	label = gtk_label_new (N_("FB Switch"));
	gtk_widget_set_size_request (label, 100, -1);
	gtk_widget_show (label);
	gtk_box_pack_start (GTK_BOX (hbox_param), label, FALSE, TRUE, 0);

	checkbutton = gtk_check_button_new_with_label(N_("Feed Back"));
	gtk_widget_set_size_request (checkbutton, 100, -1);
	gtk_box_pack_start (GTK_BOX (hbox_param), checkbutton, TRUE, TRUE, 0);
	gtk_widget_show (checkbutton);
	g_signal_connect (G_OBJECT (checkbutton), "clicked",
			    G_CALLBACK (DSPControl::feedback_callback), this);

	checkbutton = gtk_check_button_new_with_label(N_("Dual DSP Settings"));
	gtk_widget_set_size_request (checkbutton, 100, -1);
	gtk_box_pack_start (GTK_BOX (hbox_param), checkbutton, TRUE, TRUE, 0);
	gtk_widget_show (checkbutton);
	g_signal_connect (G_OBJECT (checkbutton), "clicked",
			    G_CALLBACK (DSPControl::dualmode_callback), this);


	input = mygtk_create_input("U", vbox_param, hbox_param);
	ec = new Gtk_EntryControl (Volt, MLD_WERT_NICHT_OK, &dsp->UTunnel, -10., 10., "5g", input, 0.0001);
	ec->Set_ChangeNoticeFkt(DSPControl::ChangedNotify, this);
	EC_list = g_slist_prepend( EC_list, ec);
	RemoteEntryList = ec->AddEntry2RemoteList("DSP_U", RemoteEntryList);

	input = mygtk_add_input("CP", hbox_param);
	ec = new Gtk_EntryControl (Unity, MLD_WERT_NICHT_OK, &dsp->usrCP, -10., 10., "5g", input);
	ec->Set_ChangeNoticeFkt(DSPControl::ChangedNotify, this);
	EC_list = g_slist_prepend( EC_list, ec);
	RemoteEntryList = ec->AddEntry2RemoteList("DSP_CP", RemoteEntryList);

	input = mygtk_create_input("I", vbox_param, hbox_param);
	ec = new Gtk_EntryControl (Current, MLD_WERT_NICHT_OK, &dsp->ITunnelSoll, 0.0001, 50., "5g", input);
	ec->Set_ChangeNoticeFkt(DSPControl::ChangedNotify, this);
	EC_list = g_slist_prepend( EC_list, ec);
	RemoteEntryList = ec->AddEntry2RemoteList("DSP_I", RemoteEntryList);

	input = mygtk_add_input("CI", hbox_param);
	ec = new Gtk_EntryControl (Unity, MLD_WERT_NICHT_OK, &dsp->usrCI, -10., 10., "5g", input);
	ec->Set_ChangeNoticeFkt(DSPControl::ChangedNotify, this);
	EC_list = g_slist_prepend( EC_list, ec);
	RemoteEntryList = ec->AddEntry2RemoteList("DSP_CI", RemoteEntryList);

	input = mygtk_create_input("SetPoint", vbox_param, hbox_param);
	ec = new Gtk_EntryControl (SetPtUnit, MLD_WERT_NICHT_OK, &dsp->SetPoint, -500., 500., "5g", input);
	ec->Set_ChangeNoticeFkt(DSPControl::ChangedNotify, this);
	EC_list = g_slist_prepend( EC_list, ec);
	RemoteEntryList = ec->AddEntry2RemoteList("DSP_SetPoint", RemoteEntryList);

	input = mygtk_add_input("CS", hbox_param);
	ec = new Gtk_EntryControl (Unity, MLD_WERT_NICHT_OK, &dsp->CS, 0., 4., "5g", input);
	ec->Set_ChangeNoticeFkt(DSPControl::ChangedNotify, this);
	EC_list = g_slist_prepend( EC_list, ec);
	RemoteEntryList = ec->AddEntry2RemoteList("DSP_CS", RemoteEntryList);



	// ========================================
	frame_param = gtk_frame_new (N_("Scan Characteristics"));
	gtk_widget_show (frame_param);
	gtk_container_add (GTK_CONTAINER (box), frame_param);

	vbox_param = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	gtk_widget_show (vbox_param);
	gtk_container_add (GTK_CONTAINER (frame_param), vbox_param);

	input = mygtk_create_input("Move Spd:", vbox_param, hbox_param);
	ec = new Gtk_EntryControl (Unity, MLD_WERT_NICHT_OK, &dsp->MV_stepsize, 1., 100., "4.0f", input);
	ec->Set_ChangeNoticeFkt(DSPControl::ChangedNotify, this);
	EC_list = g_slist_prepend( EC_list, ec);
	RemoteEntryList = ec->AddEntry2RemoteList("DSP_MoveSpd", RemoteEntryList);

	input = mygtk_add_input("#Loops", hbox_param);
	ec = new Gtk_EntryControl (Unity, MLD_WERT_NICHT_OK, &dsp->MV_nRegel, 1., 1000., "4.0f", input);
	ec->Set_ChangeNoticeFkt(DSPControl::ChangedNotify, this);
	EC_list = g_slist_prepend( EC_list, ec);
	RemoteEntryList = ec->AddEntry2RemoteList("DSP_MoveLoops", RemoteEntryList);

	input = mygtk_create_input("Scan Spd:", vbox_param, hbox_param);
	ec = new Gtk_EntryControl (Unity, MLD_WERT_NICHT_OK, &dsp->LS_stepsize, 1., 1000., "4.0f", input);
	ec->Set_ChangeNoticeFkt(DSPControl::ChangedNotify, this);
	EC_list = g_slist_prepend( EC_list, ec);
	RemoteEntryList = ec->AddEntry2RemoteList("DSP_ScanSpd", RemoteEntryList);

	input = mygtk_add_input("#Loops", hbox_param);
	ec = new Gtk_EntryControl (Unity, MLD_WERT_NICHT_OK, &dsp->LS_nRegel, 1., 1000., "4.0f", input);
	ec->Set_ChangeNoticeFkt(DSPControl::ChangedNotify, this);
	EC_list = g_slist_prepend( EC_list, ec);
	RemoteEntryList = ec->AddEntry2RemoteList("DSP_ScanLoops", RemoteEntryList);

	input = mygtk_create_input("#Pre:", vbox_param, hbox_param);
	ec = new Gtk_EntryControl (Unity, MLD_WERT_NICHT_OK, &dsp->LS_nx_pre, 0., 1000., "4.0f", input);
	ec->Set_ChangeNoticeFkt(DSPControl::ChangedNotify, this);
	EC_list = g_slist_prepend( EC_list, ec);
	RemoteEntryList = ec->AddEntry2RemoteList("DSP_Pre", RemoteEntryList);

	input = mygtk_add_input("#N Avg", hbox_param);
	ec = new Gtk_EntryControl (Unity, MLD_WERT_NICHT_OK, &dsp->LS_nAve, 1., 1000., "4.0f", input);
	ec->Set_ChangeNoticeFkt(DSPControl::ChangedNotify, this);
	EC_list = g_slist_prepend( EC_list, ec);
	RemoteEntryList = ec->AddEntry2RemoteList("DSP_NAvg", RemoteEntryList);


	input = mygtk_create_input("DPL:", vbox_param, hbox_param);
	gtk_entry_set_editable(GTK_ENTRY(input), FALSE);
	ec = new Gtk_EntryControl (Unity, MLD_WERT_NICHT_OK, &dsp->LS_nx2scan, 0., 9999., "4.0f", input);
	EC_list = g_slist_prepend( EC_list, ec);

	input = mygtk_add_input("DDist", hbox_param);
	gtk_entry_set_editable(GTK_ENTRY(input), FALSE);
	ec = new Gtk_EntryControl (Unity, MLD_WERT_NICHT_OK, &dsp->LS_dnx, 1., 9999., "4.0f", input);
	EC_list = g_slist_prepend( EC_list, ec);



	// ======================================== 2nd DSP Set in Dual Mode
	frame_param = gtk_frame_new (N_("FB and Scan Characteristics for <- Scan Dir in Dual Mode"));
	DualSettingsFrame = frame_param;
	//  gtk_widget_show (frame_param);
	gtk_container_add (GTK_CONTAINER (box), frame_param);

	vbox_param = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	gtk_widget_show (vbox_param);
	gtk_container_add (GTK_CONTAINER (frame_param), vbox_param);

	// ------- FB Characteristics

	input = mygtk_create_input("U", vbox_param, hbox_param);
	ec = new Gtk_EntryControl (Volt, MLD_WERT_NICHT_OK, &dsp_dual->UTunnel, -10., 10., "5g", input);
	ec->Set_ChangeNoticeFkt(DSPControl::ChangedNotify, this);
	EC_list = g_slist_prepend( EC_list, ec);
	RemoteEntryList = ec->AddEntry2RemoteList("DSP2_U", RemoteEntryList);

	input = mygtk_add_input("CP", hbox_param);
	ec = new Gtk_EntryControl (Unity, MLD_WERT_NICHT_OK, &dsp_dual->usrCP, -10., 10., "5g", input);
	ec->Set_ChangeNoticeFkt(DSPControl::ChangedNotify, this);
	EC_list = g_slist_prepend( EC_list, ec);
	RemoteEntryList = ec->AddEntry2RemoteList("DSP2_CP", RemoteEntryList);

	input = mygtk_create_input("I", vbox_param, hbox_param);
	ec = new Gtk_EntryControl (Current, MLD_WERT_NICHT_OK, &dsp_dual->ITunnelSoll, 0.0001, 50., "5g", input);
	ec->Set_ChangeNoticeFkt(DSPControl::ChangedNotify, this);
	EC_list = g_slist_prepend( EC_list, ec);
	RemoteEntryList = ec->AddEntry2RemoteList("DSP2_I", RemoteEntryList);

	input = mygtk_add_input("CI", hbox_param);
	ec = new Gtk_EntryControl (Unity, MLD_WERT_NICHT_OK, &dsp_dual->usrCI, -10., 10., "5g", input);
	ec->Set_ChangeNoticeFkt(DSPControl::ChangedNotify, this);
	EC_list = g_slist_prepend( EC_list, ec);
	RemoteEntryList = ec->AddEntry2RemoteList("DSP2_CI", RemoteEntryList);

	input = mygtk_create_input("SetPoint", vbox_param, hbox_param);
	ec = new Gtk_EntryControl (SetPtUnit, MLD_WERT_NICHT_OK, &dsp_dual->SetPoint, -500., 500., "5g", input);
	ec->Set_ChangeNoticeFkt(DSPControl::ChangedNotify, this);
	EC_list = g_slist_prepend( EC_list, ec);
	RemoteEntryList = ec->AddEntry2RemoteList("DSP2_SetPoint", RemoteEntryList);

	input = mygtk_add_input("CS", hbox_param);
	ec = new Gtk_EntryControl (Unity, MLD_WERT_NICHT_OK, &dsp_dual->CS, 0., 2., "5g", input);
	ec->Set_ChangeNoticeFkt(DSPControl::ChangedNotify, this);
	EC_list = g_slist_prepend( EC_list, ec);
	RemoteEntryList = ec->AddEntry2RemoteList("DSP2_CS", RemoteEntryList);

	input = mygtk_create_input("Move Spd:", vbox_param, hbox_param);
	ec = new Gtk_EntryControl (Unity, MLD_WERT_NICHT_OK, &dsp_dual->MV_stepsize, 1., 100., "4.0f", input);
	ec->Set_ChangeNoticeFkt(DSPControl::ChangedNotify, this);
	EC_list = g_slist_prepend( EC_list, ec);
	RemoteEntryList = ec->AddEntry2RemoteList("DSP2_MoveSpd", RemoteEntryList);

	input = mygtk_add_input("#Loops", hbox_param);
	ec = new Gtk_EntryControl (Unity, MLD_WERT_NICHT_OK, &dsp_dual->MV_nRegel, 1., 1000., "4.0f", input);
	ec->Set_ChangeNoticeFkt(DSPControl::ChangedNotify, this);
	EC_list = g_slist_prepend( EC_list, ec);
	RemoteEntryList = ec->AddEntry2RemoteList("DSP2_MoveLoops", RemoteEntryList);

	input = mygtk_create_input("Scan Spd:", vbox_param, hbox_param);
	ec = new Gtk_EntryControl (Unity, MLD_WERT_NICHT_OK, &dsp_dual->LS_stepsize, 1., 1000., "4.0f", input);
	ec->Set_ChangeNoticeFkt(DSPControl::ChangedNotify, this);
	EC_list = g_slist_prepend( EC_list, ec);
	RemoteEntryList = ec->AddEntry2RemoteList("DSP2_ScanSpd", RemoteEntryList);

	input = mygtk_add_input("#Loops", hbox_param);
	ec = new Gtk_EntryControl (Unity, MLD_WERT_NICHT_OK, &dsp_dual->LS_nRegel, 1., 1000., "4.0f", input);
	ec->Set_ChangeNoticeFkt(DSPControl::ChangedNotify, this);
	EC_list = g_slist_prepend( EC_list, ec);
	RemoteEntryList = ec->AddEntry2RemoteList("DSP2_ScanLoops", RemoteEntryList);

	input = mygtk_create_input("#Pre:", vbox_param, hbox_param);
	ec = new Gtk_EntryControl (Unity, MLD_WERT_NICHT_OK, &dsp_dual->LS_nx_pre, 0., 1000., "4.0f", input);
	ec->Set_ChangeNoticeFkt(DSPControl::ChangedNotify, this);
	EC_list = g_slist_prepend( EC_list, ec);
	RemoteEntryList = ec->AddEntry2RemoteList("DSP2_Pre", RemoteEntryList);

	input = mygtk_add_input("#N Avg", hbox_param);
	ec = new Gtk_EntryControl (Unity, MLD_WERT_NICHT_OK, &dsp_dual->LS_nAve, 1., 1000., "4.0f", input);
	ec->Set_ChangeNoticeFkt(DSPControl::ChangedNotify, this);
	EC_list = g_slist_prepend( EC_list, ec);
	RemoteEntryList = ec->AddEntry2RemoteList("DSP2_NAvg", RemoteEntryList);




	// ======================================== Piezo Drive / Amplifier Settings
	frame_param = gtk_frame_new (N_("Piezo Drive Settings"));
	gtk_widget_show (frame_param);
	gtk_container_add (GTK_CONTAINER (box), frame_param);

	vbox_param = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	gtk_widget_show (vbox_param);
	gtk_container_add (GTK_CONTAINER (frame_param), vbox_param);

	/* Amplifier Settings */
	hbox_param = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_widget_show (hbox_param);
	gtk_container_add (GTK_CONTAINER (vbox_param), hbox_param);
  
	for(j=0; j<3; j++){
		switch(j){
		case 0: label = gtk_label_new ("VX"); break;
		case 1: label = gtk_label_new ("VY"); break;
		case 2: label = gtk_label_new ("VZ"); break;
		}
		gtk_widget_show (label);
		gtk_container_add (GTK_CONTAINER (hbox_param), label);
		//    gtk_box_pack_start (GTK_BOX (hbox_param), label, FALSE, TRUE, 0);
		gtk_widget_set_size_request (label
        xrm.Put("data_hardpars.LS_IntAve", data_hardpars.LS_IntAve);
        xrm.Put("data_hardpars_dual.LS_IntAve", data_hardpars_dual.LS_IntAve);
        xrm.Put("data_hardpars.MV_stepsize", data_hardpars.MV_stepsize);
        xrm.Put("data_hardpars_dual.MV_stepsize", data_hardpars_dual.MV_stepsize);
        xrm.Put("data_hardpars.MV_nRegel", data_hardpars.MV_nRegel);
        xrm.Put("data_hardpars_dual.MV_nRegel", data_hardpars_dual.MV_nRegel);


	g_slist_foreach(RemoteEntryList, (GFunc) remove, gapp->RemoteEntryList);
	g_slist_free (RemoteEntryList);
	RemoteEntryList = NULL;
	delete Unity;
	delete Volt;
	delete Current;
	delete SetPtUnit;
}

void DSPControl::update(){
	g_slist_foreach((GSList*)g_object_get_data( G_OBJECT (widget), "DSP_EC_list"),
			(GFunc) App::update_ec, NULL);
}

void DSPControl::updateDSP(int FbFlg){
	PI_DEBUG (DBG_L2, "Hallo DSP ! FB=" << FbFlg );
	switch(FbFlg){
	case DSP_FB_ON: ExecCmd(DSP_CMD_START); break;
	case DSP_FB_OFF: ExecCmd(DSP_CMD_HALT); break;
	}
	// Scale Regler Consts. with 1/VZ
	dsp->CP = dsp->usrCP/DSPControl_pi.app->xsm->Inst->VZ();
	dsp->CI = dsp->usrCI/DSPControl_pi.app->xsm->Inst->VZ();
	DSPControl_pi.app->xsm->hardware->PutParameter(dsp);
}

int DSPControl::ChangedAction(GtkWidget *widget, DSPControl *dspc){
	dspc->updateDSP();
	return 0;
}

void DSPControl::ChangedNotify(Param_Control* pcs, gpointer dspc){
	//  gchar *us=pcs->Get_UsrString();
	//  PI_DEBUG (DBG_L2, "DSPC: " << us );
	//  g_free(us);
	((DSPControl*)dspc)->updateDSP();
}

void DSPControl::ExecCmd(int cmd){
	DSPControl_pi.app->xsm->hardware->ExecCmd(cmd);
}

int DSPControl::feedback_callback( GtkWidget *widget, DSPControl *dspc){
	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget)))
		dspc->updateDSP(DSP_FB_ON);
	else
		dspc->updateDSP(DSP_FB_OFF);
	return 0;
}

int DSPControl::dualmode_callback( GtkWidget *widget, DSPControl *dspc){
        if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget))){
                DSPControl_pi.app->xsm->data.scan_mode = SCAN_MODE_DUAL_DSPSET;
                gtk_widget_show (dspc->DualSettingsFrame);
        }
        else{
                DSPControl_pi.app->xsm->data.scan_mode = SCAN_MODE_SINGLE_DSPSET;
                gtk_widget_hide (dspc->DualSettingsFrame);
        }
        return 0;
}

int DSPControl::choice_Ampl_callback (GtkWidget *widget, DSPControl *dspc){
	AmpIndex i;
	i.l=(long)g_object_get_data( G_OBJECT (widget), "chindex");
	switch(i.s.ch){
	case 0: DSPControl_pi.app->xsm->Inst->VX((int)i.s.x); break;
	case 1: DSPControl_pi.app->xsm->Inst->VY((int)i.s.x); break;
	case 2: DSPControl_pi.app->xsm->Inst->VZ((int)i.s.x); break;
	}
	PI_DEBUG (DBG_L2, "Ampl: " << i.l << " " << (int)i.s.ch << " " << (int)i.s.x );
	DSPControl_pi.app->spm_range_check(NULL, DSPControl_pi.app);
	dspc->updateDSP();
	return 0;
}


