/* Gnome gxsm - Gnome X Scanning Microscopy
 * universal STM/AFM/SARLS/SPALEED/... controlling and
 * data analysis software
 * 
 * Gxsm Plugin Name: NanoPlott.C
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
% BeginPlugInDocuSection
% PlugInDocuCaption: Nano HPLG plotter
% PlugInName: NanoPlott
% PlugInAuthor: Percy Zahl
% PlugInAuthorEmail: zahl@users.sf.net
% PlugInMenuPath: windows-sectionNano Plotter

% PlugInDescription
 This is a tool to use your tip for writing or manipulating by moving
 the tip (i.e. the scan-piezo) along an arbitrary programmable
 path. There are two DSP-parameter sets for bias, feedback and scan
 settings which can be used for to assign two ''writing'' modes called
 PU (pen up) and PD (pen down). The path is read from a plotter-file in
 a very simple HPGL-plotter language style. This file can be
 created by hand (editor) or from line or polyline object(s) drawn in a
 scan.

 \GxsmScreenShot{NanoPlott}{The Nano-Plott window.}

% PlugInUsage
 Set desired parameters for Pen Up and Pen Down, put in the path/file
 to your plottfile (.plt) and press start\dots You can translate and
 scale you plot data using the parameters Offset and Scale. Make sure
 the plot file name is correct in can be found from current working
 directory or use an absolute path.

% OptPlugInObjects
 Poly line objects are used for display only -- not for input of path!

% OptPlugInDest
 The plotted path is shown on the active scan using poly line objects.

%% OptPlugInConfig
%

% OptPlugInFiles
 Create a simple HPGL plott file for input. You can do this with Gxsm
 itself: place a bunch of PolyLine objects and use \GxsmPopup{Grey
 2D}{File/Save Objects} to create a HPGL file, enter therefore a file
 with a \filename{.plt} extension! Use this as input for the
 NanoPlotter.

% OptPlugInKnownBugs
 Not 100\% tested, beta state.

% OptPlugInNotes
 The PCI32-DSP vector scan generator is only able to move in horizontal,
 vertical or diagonal direction. This limitation dose not apply to the SRanger DSP code.

% OptPlugInHints
 Use \GxsmEmph{hp2xx} for preview of your HPGL files, available for debian!

 Example file to draw a line from 0$\:$\AA, 0$\:$\AA to 0$\:$\AA, 5000$\:$\AA:\\
\ \\
PU 0.0,0.0;\\
PD 0.0,5000.0;\\
                                                                              


% OptPlugInSubSection: Disclaimer
 This tool may ruin your tip and sample, so take care!

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


static void NanoPlott_about( void );
static void NanoPlott_query( void );
static void NanoPlott_cleanup( void );

static void NanoPlott_show_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data);
static void NanoPlott_StartScan_callback( gpointer );

GxsmPlugin NanoPlott_pi = {
	NULL,
	NULL,
	0,
	NULL,
	"NanoPlott",
	"+ALL +SRanger:SPMHARD +SRangerTest:SPMHARD +Innovative_DSP:SPMHARD +Innovative_DSP:SPAHARD",
	//  "-LAN_RHK:SPMHARD", //"+spmHARD +STM +AFM",
	NULL,
	"Percy Zahl",
	"windows-section",
	N_("Nano Plotter"),
	N_("open the Nano Plotter Control"),
	"Nano Plotter Control",
	NULL,
	NULL,
	NULL,
	NanoPlott_query,
	NanoPlott_about,
	NULL, // config
	NULL, // run
	NanoPlott_show_callback, // MCB1
	NULL, // MCB2
	NanoPlott_cleanup
};

static const char *about_text = N_("Gxsm NanoPlotter Plugin:\n"
				   "This plugin runs a control window to "
				   "do simple HPGL 'Nano Plotting'.\n\n"
				   "Supported Commands are PU (PenUp) and PD (PenDown)\n"
				   "to given coordinates X,Y.\n"
				   "Syntax PenUp movement:   'PU X,Z;'\n"
				   "Syntax PenDown movement: 'PD X,Z;'\n\n"
				   "Example Plot File of a 'square plott':\n"
				   "PU 0,0;\n"
				   "PD 100,0;\n"
				   "PD 100,100;\n"
				   "PD 0,100;\n"
				   "PD 0,0;\n"
				   );


typedef struct{
	double bias;
	double current;
	double setpoint;
	double cp, ci;
	double speed;
} DSP_Remote;



GxsmPlugin *get_gxsm_plugin_info ( void ){ 
	NanoPlott_pi.description = g_strdup_printf(N_("Gxsm NanoPlott plugin %s"), VERSION);
	return &NanoPlott_pi; 
}

class NanoPlottControl : public AppBase{
public:
	NanoPlottControl();
	virtual ~NanoPlottControl();
	
	void update();
	void savestate();
	static void ExecCmd(int cmd);
	static void RunPlott(GtkWidget *widget, NanoPlottControl *npc);
	static void StopPlott(GtkWidget *widget, NanoPlottControl *npc);
	
	void SetNewParam(DSP_Remote *dsp);
	void Transform(double &x, double &y);
	void GoToPosition(double x, double y);

private:
	gchar *PlotFile;
	double Xorg, Yorg, XScale, YScale;
	int    repeat, count;
	UnitObj *Unity, *Volt, *Current, *Force, *Speed;
	DSP_Remote dsp_move;
	DSP_Remote dsp_plot;
};

NanoPlottControl *NanoPlottClass = NULL;

static void NanoPlott_query(void)
{
	// new ...
	NanoPlottClass = new NanoPlottControl;

	NanoPlottClass->set_window_geometry ("nano-plott-control");

	NanoPlott_pi.app->ConnectPluginToStartScanEvent
		( NanoPlott_StartScan_callback );

	NanoPlott_pi.status = g_strconcat(N_("Plugin query has attached "),
					  NanoPlott_pi.name, 
					  N_(": NanoPlott is created."),
					  NULL);
}

static void NanoPlott_about(void)
{
	const gchar *authors[] = { "Percy Zahl", NULL};
	gtk_show_about_dialog (NULL,
			       "program-name",  NanoPlott_pi.name,
			       "version", VERSION,
			       "license", GTK_LICENSE_GPL_3_0,
			       "comments", about_text,
			       "authors", authors,
			       NULL
			       );
}

static void NanoPlott_cleanup( void ){
	PI_DEBUG (DBG_L2, "NanoPlott Plugin Cleanup" );

	// delete ...
	if( NanoPlottClass )
		delete NanoPlottClass ;

	g_free( NanoPlott_pi.status );
}

static void NanoPlott_show_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data){
	if( NanoPlottClass )
		NanoPlottClass->show();
}

static void NanoPlott_StartScan_callback( gpointer ){
	NanoPlottClass->update();
}


NanoPlottControl::NanoPlottControl ()
{
	GtkWidget *box, *hbox;
	GtkWidget *frame_param;
	GtkWidget *vbox_param, *hbox_param;
	GtkWidget *input, *rep_input;

	GSList *EC_list=NULL;
	GSList **RemoteEntryList = new GSList *;
	*RemoteEntryList = NULL;

	Gtk_EntryControl *ec;

#define UTF8_DEGREE    "\302\260"
#define UTF8_MU        "\302\265"
#define UTF8_ANGSTROEM "\303\205"

	Unity    = new UnitObj(" "," ");
	Volt     = new UnitObj("V","V");
	Current  = new UnitObj("nA","nA");
	Force    = new UnitObj("nN","nN");
	Speed    = new UnitObj(UTF8_ANGSTROEM"/s","A/s");


	XsmRescourceManager xrm("PluginNanoPlottControl");
	xrm.Get("NanoPlott.Xorg", &Xorg, "0.");
	xrm.Get("NanoPlott.Yorg", &Yorg, "0.");
	xrm.Get("NanoPlott.XScale", &XScale, "1.");
	xrm.Get("NanoPlott.YScale", &YScale, "1.");
	xrm.Get("NanoPlott.Repeat", &repeat, "1");

	// ** User Params
	xrm.Get("NanoPlott.move_bias", &dsp_move.bias, "2.");
	xrm.Get("NanoPlott.move_current", &dsp_move.current, "1.");
	xrm.Get("NanoPlott.move_setpoint", &dsp_move.setpoint, "1.");
	xrm.Get("NanoPlott.move_cp", &dsp_move.cp, "0.1");
	xrm.Get("NanoPlott.move_ci", &dsp_move.ci, "0.1");
	xrm.Get("NanoPlott.move_speed", &dsp_move.speed, "100.");

	xrm.Get("NanoPlott.plot_bias", &dsp_plot.bias, "2.");
	xrm.Get("NanoPlott.plot_current", &dsp_plot.current, "1.");
	xrm.Get("NanoPlott.plot_setpoint", &dsp_plot.setpoint, "1.");
	xrm.Get("NanoPlott.plot_cp", &dsp_plot.cp, "0.1");
	xrm.Get("NanoPlott.plot_ci", &dsp_plot.ci, "0.1");
	xrm.Get("NanoPlott.plot_speed", &dsp_plot.speed, "100.");

	xrm.Get("NanoPlott.PlotFile", &PlotFile, "myplottfile");

	count = -1;

	//	AppWindowInit("Nano Plott");

	//	GtkDialogFlags flags =  (GtkDialogFlags) (GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT);
	GtkDialogFlags flags =  (GtkDialogFlags) (GTK_DIALOG_DESTROY_WITH_PARENT);
	GtkWidget *dialog = gtk_dialog_new_with_buttons (N_("Nano Plott"),
							 GTK_WINDOW (main_get_gapp()->get_app_window ()),
							 flags,
							 _("_OK"),
							 GTK_RESPONSE_ACCEPT,
							 _("_Cancel"),
							 GTK_RESPONSE_REJECT,
							 NULL);
	BuildParam bp;
	gtk_container_add (GTK_CONTAINER (gtk_dialog_get_content_area (GTK_DIALOG (dialog))), bp.grid);
  
	frame_param = gtk_frame_new (N_("Settings for Move: PU X,Y;"));

	bp.grid_add_widget (frame_param);
	bp.new_line ();

	BuildParam bpf;

	gtk_container_add (GTK_CONTAINER (frame_param), bpf.grid);
	bpf.grid_add_ec ("U", Volt, &dsp_move.bias, -10., 10., "5g"); bpf.new_line ();

	if (IS_AFM_CTRL){ // AFM (linear)
		bpf.grid_add_ec ("SP", Force, &dsp_move.setpoint, -10000., 10000., "5g"); bpf.new_line ();
	} else {
		bpf.grid_add_ec ("I", Current, &dsp_move.current, 0.001, 50., "5g"); bpf.new_line ();
	}

	bpf.grid_add_ec ("CP", Unity, &dsp_move.cp, -10., 10., "5g"); bpf.new_line ();
	bpf.grid_add_ec ("CI", Unity, &dsp_move.ci, -10., 10., "5g"); bpf.new_line ();

	bpf.grid_add_ec ("Speed", Speed, &dsp_move.speed, 1., 100000., "4.0f"); bpf.new_line ();

	// ========================================

	frame_param = gtk_frame_new (N_("Settings for Plot: PD X,Y;"));
	bp.grid_add_widget (frame_param);
	bp.new_line ();

	bpf.new_grid ();

	gtk_container_add (GTK_CONTAINER (frame_param), bpf.grid);

	bpf.grid_add_ec ("U", Volt, &dsp_plot.bias, -10., 10., "5g"); bpf.new_line ();

	if (IS_AFM_CTRL){ // AFM (linear)
		bpf.grid_add_ec ("SP", Force,  &dsp_plot.setpoint, -10000., 10000., "5g"); bpf.new_line ();
	} else {
		bpf.grid_add_ec ("I", Current, &dsp_plot.current, 0.001, 50., "5g"); bpf.new_line ();
	}

	bpf.grid_add_ec ("CP", Unity, &dsp_plot.cp, -10., 10., "5g"); bpf.new_line ();
	bpf.grid_add_ec ("CI", Unity, &dsp_plot.ci, -10., 10., "5g"); bpf.new_line ();
	bpf.grid_add_ec ("Speed", Speed, &dsp_plot.speed, 1., 100000., "4.0f"); bpf.new_line ();

	// ========================================
	frame_param = gtk_frame_new (N_("Nano Plot Settings"));
	bp.grid_add_widget (frame_param);
	bp.new_line ();

	bpf.new_grid ();

	gtk_container_add (GTK_CONTAINER (frame_param), bpf.grid);

	// Plott Offset, Scale
	bpf.grid_add_ec ("Offset XY",
			 NanoPlott_pi.app->xsm->X_Unit, &Xorg,
			 NanoPlott_pi.app->xsm->XOffsetMin(), NanoPlott_pi.app->xsm->XOffsetMax(), 
			 NanoPlott_pi.app->xsm->AktUnit->prec1);
	
	bpf.grid_add_ec (NULL,
			 NanoPlott_pi.app->xsm->Y_Unit, &Yorg,
			 NanoPlott_pi.app->xsm->YOffsetMin(), NanoPlott_pi.app->xsm->YOffsetMax(), 
			 NanoPlott_pi.app->xsm->AktUnit->prec1);
	bpf.new_line ();
	
	bpf.grid_add_ec ("Scale XY", Unity, &XScale, -1e8, 1e8, "5g");
	bpf.grid_add_ec (NULL, Unity, &YScale, -1e8, 1e8, "5g");
	bpf.new_line ();
	
	bpf.grid_add_ec ("Repeat #", Unity, &repeat, 1, 99999, "5.0f");
	bpf.new_line ();

	rep_input = input;

	// PlotFile
#if 1
	input = bpf.grid_add_input ("PlotFile");
	bpf.new_line ();
	g_object_set_data( G_OBJECT (v_grid), "PlotFileEntry", input);
	gtk_entry_set_text ( GTK_ENTRY (input), PlotFile);
#else
	input = gtk_file_chooser_button_new (_("Select a plot file (.plt)"), GTK_FILE_CHOOSER_ACTION_OPEN);
	bpf.grid_add_widget (input);
	bpf.new_line ();
	//	gnome_file_entry_set_filename (GNOME_FILE_ENTRY (input), PlotFile);
#endif

	gtk_widget_show_all (dialog);

	gint r=gtk_dialog_run(GTK_DIALOG(dialog));
	//	gtk_widget_destroy (dialog);

	/*
		// Run Button
	GtkWidget *button = gtk_button_new_with_label("Start Plot");
	gtk_widget_show (button);
	gtk_box_pack_start (GTK_BOX (hbox_param), button, TRUE, TRUE, 0);
	g_object_set_data( G_OBJECT (button), "PlotFileEntry", input);
	g_object_set_data( G_OBJECT (button), "RepeatEntry", rep_input);
	g_signal_connect ( G_OBJECT (button), "pressed",
			   G_CALLBACK (NanoPlottControl::RunPlott),
			   this);

	button = gtk_button_new_with_label("Stop");
	gtk_widget_show (button);
	gtk_box_pack_start (GTK_BOX (hbox_param), button, TRUE, TRUE, 0);
	g_object_set_data( G_OBJECT (button), "PlotFileEntry", input);
	g_signal_connect ( G_OBJECT (button), "pressed",
			   G_CALLBACK (NanoPlottControl::StopPlott),
			   this);
	// CmdList

	// save List away...
	g_object_set_data( G_OBJECT (window), "NANOPLOTT_EC_list", EC_list);
	*/
}

NanoPlottControl::~NanoPlottControl (){

	savestate ();

	delete Unity;
	delete Volt;
	delete Current;
	delete Force;
	delete Speed;
}


void NanoPlottControl::savestate (){
	XsmRescourceManager xrm("PluginNanoPlottControl");
	xrm.Put("NanoPlott.Xorg", Xorg);
	xrm.Put("NanoPlott.Yorg", Yorg);
	xrm.Put("NanoPlott.XScale", XScale);
	xrm.Put("NanoPlott.YScale", YScale);
	xrm.Put("NanoPlott.Repeat", repeat);

	// ** User Params
	xrm.Put("NanoPlott.move_bias", dsp_move.bias);
	xrm.Put("NanoPlott.move_current", dsp_move.current);
	xrm.Put("NanoPlott.move_setpoint", dsp_move.setpoint);
	xrm.Put("NanoPlott.move_cp", dsp_move.cp);
	xrm.Put("NanoPlott.move_ci", dsp_move.ci);
	xrm.Put("NanoPlott.move_speed", dsp_move.speed);
	
	xrm.Put("NanoPlott.plot_bias", dsp_plot.bias);
	xrm.Put("NanoPlott.plot_current", dsp_plot.current);
	xrm.Put("NanoPlott.plot_setpoint", dsp_plot.setpoint);
	xrm.Put("NanoPlott.plot_cp", dsp_plot.cp);
	xrm.Put("NanoPlott.plot_ci", dsp_plot.ci);
	xrm.Put("NanoPlott.plot_speed", dsp_plot.speed);

	xrm.Put("NanoPlott.PlotFile", PlotFile);
}


void NanoPlottControl::update(){
	g_slist_foreach((GSList*)g_object_get_data( G_OBJECT (window), "NANOPLOTT_EC_list"),
			(GFunc) App::update_ec, NULL);
}

void NanoPlottControl::StopPlott(GtkWidget *widget, NanoPlottControl *npc){
	npc->savestate ();
	npc->count = npc->repeat;

	// 	gtk_entry_set_text (
	// 		GTK_ENTRY (g_object_get_data ( 
	// 				   G_OBJECT (widget),
	// 				   "PlotFileEntry"
	// 				   )),
	// 		npc->PlotFile
	// 		);

	npc->update ();
}

void NanoPlottControl::RunPlott(GtkWidget *widget, NanoPlottControl *npc){
	if (npc->count != -1) return;
	gtk_widget_set_sensitive (widget, FALSE);

#define MAXPOLYNODES 8
	// 	G_FREE_STRDUP (
	// 		npc->PlotFile, 
	// 		gtk_entry_get_text (
	// 			GTK_ENTRY (g_object_get_data ( 
	// 					   G_OBJECT (widget),
	// 					   "PlotFileEntry"
	// 					   ))
	// 			)
	// 		);
	if (npc->PlotFile)
		g_free (npc->PlotFile);
	// GTK3QQQ FIX!!!
	npc->PlotFile = g_strdup("text.plt");
	//	npc->PlotFile = g_strdup( gnome_file_entry_get_full_path (GNOME_FILE_ENTRY (g_object_get_data (G_OBJECT (widget), "PlotFileEntry")), TRUE));
	PI_DEBUG (DBG_L2, npc->PlotFile );

	npc->count = 0;
	do {
		PI_DEBUG (DBG_L2, "----------- Start Plotting ----------" );
		std::ifstream hp;
		hp.open (npc->PlotFile, std::ios::in);
		int k=0;
		double xy[1+2*MAXPOLYNODES];
		double x0,y0;
		x0=0.; y0=0.;
		
		gchar *mld = g_strdup_printf ("Running: %d/%d: %5.1f%% done", npc->count, npc->repeat, 100.*npc->count/npc->repeat);
		gtk_entry_set_text (GTK_ENTRY (g_object_get_data (G_OBJECT (widget), "RepeatEntry")), mld);
		g_free (mld);
		while(gtk_events_pending()) gtk_main_iteration();

		while (hp.good()){
			gchar cmd[64];
			hp.getline (cmd, 64);
			PI_DEBUG (DBG_L2, "$>" << cmd );
			if (!strncmp(cmd, "PU", 2)){
				double x,y;
				strtok(cmd, " ,;");
				x = atof (strtok(NULL, " ,;"));
				y = atof (strtok(NULL, " ,;"));
				npc->SetNewParam (&npc->dsp_move);
				npc->Transform (x, y);
				npc->GoToPosition (x, y);

				while(gtk_events_pending()) gtk_main_iteration();

				PI_DEBUG (DBG_L2, "Pen Up: (" << x << "," << y << ")" );
				x0=x; y0=y;
				if(k){ 
					if (NanoPlott_pi.app->xsm->ActiveScan)
						if (NanoPlott_pi.app->xsm->ActiveScan->view && npc->count == 0)
							NanoPlott_pi.app->xsm->ActiveScan->view->add_object (0, (gpointer) xy);
					k=0;
				}
			}
			if (!strncmp(cmd, "PD", 2)){
				double x,y;
				if(!k) { 
					xy[0] = (double)(k+1);
					xy[1 + 2*k    ] = x0;
					xy[1 + 2*k + 1] = y0;
					++k;
				}
				strtok(cmd, " ,;");
				x = atof (strtok(NULL, " ,;"));
				y = atof (strtok(NULL, " ,;"));
				npc->SetNewParam (&npc->dsp_plot);
				npc->Transform (x, y);
				npc->GoToPosition (x, y);

				while(gtk_events_pending()) gtk_main_iteration();

				PI_DEBUG (DBG_L2, "Pen Down: (" << x << "," << y << ")" );
				// x,y in Pixel via nx,ny,user(rx,ry)
				if(k<MAXPOLYNODES){
					x0=x; y0=y;
					xy[0] = (double)(k+1);
					xy[1 + 2*k    ] = x;
					xy[1 + 2*k + 1] = y;
					++k;
				}else{
					// add polyobj. and start new one 
					// "appending" at last pos.
					if (NanoPlott_pi.app->xsm->ActiveScan)
						if (NanoPlott_pi.app->xsm->ActiveScan->view && npc->count == 0)
							NanoPlott_pi.app->xsm->ActiveScan->view->add_object (0, (gpointer) xy);
					k=0;
					xy[0] = (double)(k+1);
					xy[1 + 2*k    ] = x0;
					xy[1 + 2*k + 1] = y0;
					++k;
					xy[0] = (double)(k+1);
					xy[1 + 2*k    ] = x;
					xy[1 + 2*k + 1] = y;
					++k;
				}
			}
		}
		if(k){ 
			if (NanoPlott_pi.app->xsm->ActiveScan)
				if (NanoPlott_pi.app->xsm->ActiveScan->view && npc->count == 0)
					NanoPlott_pi.app->xsm->ActiveScan->view->add_object (0, (gpointer) xy);
		}
		hp.close();
		PI_DEBUG (DBG_L2, "-----------  End Plotting  ----------" );

		++npc->count;
	} while (npc->count < npc->repeat);

	PI_DEBUG (DBG_L2, "Restoring FB Parameters" );

	// **=== restore missing ===
	// **===	npc->SetNewParam (&NanoPlott_pi.app->xsm->data.hardpars);

	gtk_widget_set_sensitive (widget, TRUE);
	npc->count = -1;

	npc->update ();
}


/* stolen from app_remote.C & pyremote.C */
static void check_remote_ec(Gtk_EntryControl* ec, remote_args* ra){
	ec->CheckRemoteCmd(ra);
}

static void set_via_remote (const gchar *arg, double value){
	gchar *args[] = { g_strdup("set"), g_strdup (arg), g_strdup_printf ("%g", value) };
	g_slist_foreach (main_get_gapp()->RemoteEntryList, (GFunc) check_remote_ec, (gpointer)args);
	g_free (args[0]);
	g_free (args[1]);
	g_free (args[2]);
}

void NanoPlottControl::SetNewParam(DSP_Remote *dsp){
	set_via_remote ("DSP_Bias", dsp->bias);
	set_via_remote ("DSP_Current", dsp->current);
	set_via_remote ("DSP_SetPoint", dsp->setpoint);
	set_via_remote ("DSP_CP", dsp->cp);
	set_via_remote ("DSP_CI", dsp->ci);
	set_via_remote ("DSP_MoveSpd", dsp->speed);
}

void NanoPlottControl::Transform(double &x, double &y){
	x *= XScale; x += Xorg;
	y *= YScale; y += Yorg;
}

void NanoPlottControl::GoToPosition(double x, double y){
	NanoPlott_pi.app->xsm->hardware->SetOffset(
						   R2INT(NanoPlott_pi.app->xsm->Inst->X0A2Dig(x)),
						   R2INT(NanoPlott_pi.app->xsm->Inst->Y0A2Dig(y)));
}
