/* Gnome gxsm - Gnome X Scanning Microscopy
 * universal STM/AFM/SARLS/SPALEED/... controlling and
 * data analysis software
 * 
 * Gxsm Plugin Name: nano_manipulator.C
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
% PlugInDocuCaption: Nano Manipulator (to be ported)
% PlugInName: nano_manipulator
% PlugInAuthor: Percy Zahl
% PlugInAuthorEmail: zahl@users.sf.net
% PlugInMenuPath: windows-sectionNano Manipulator

% PlugInDescription
This is a tool to use your tip for nano manipulating.

It will connect to the DSP/SPM and provide a realtime force feedback
(using a modern Force Feedback Joystick Control Device) while
moving/pushing things around\dots

% PlugInUsage

% OptPlugInObjects
Shows a trace and current tip position\dots

%% OptPlugInDest

%% OptPlugInConfig
%

%% OptPlugInFiles

%% OptPlugInKnownBugs

% OptPlugInNotes
This PlugIn is work in progress.

%% OptPlugInHints

% OptPlugInSubSection: Disclaimer
This tool may ruin your tip and sample if impropper used!

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



static void nano_manipulator_about( void );
static void nano_manipulator_query( void );
static void nano_manipulator_cleanup( void );

static void nano_manipulator_show_callback( GtkWidget*, void* );
static void nano_manipulator_StartScan_callback( gpointer );

GxsmPlugin nano_manipulator_pi = {
  NULL,
  NULL,
  0,
  NULL,
  "nano_manipulator",
//  "+ALL +SRanger:SPMHARD +SRangerTest:SPMHARD +Innovative_DSP:SPMHARD +Innovative_DSP:SPAHARD",
  "+Innovative_DSP:SPMHARD +Innovative_DSP:SPAHARD",
  NULL,
  "Percy Zahl",
  "windows-section",
  N_("Nano Manipulator"),
  N_("open the Nano Plotter Control"),
  "Nano Manipulator Control",
  NULL,
  NULL,
  NULL,
  nano_manipulator_query,
  nano_manipulator_about,
  NULL,
  NULL,
  nano_manipulator_cleanup
};

static const char *about_text = N_("Gxsm nano_manipulator Plugin:\n"
	);

GxsmPlugin *get_gxsm_plugin_info ( void ){ 
  nano_manipulator_pi.description = g_strdup_printf(N_("Gxsm nano_manipulator plugin %s"), VERSION);
  return &nano_manipulator_pi; 
}

class nano_manipulatorControl : public AppBase{
public:
	nano_manipulatorControl();
	virtual ~nano_manipulatorControl();
	
	void update();
	void update_pos();
	static void RunPlott(GtkWidget *widget, nano_manipulatorControl *npc);
	
	void SetNewParam(DSP_Param *dsp);
	void GoToPosition(double x, double y);

private:
	int devpcdspmon;
	Trace_Data td_current;
	UnitObj *Unity, *Volt, *Current, *Force;
	DSP_Param    move_dsp;
	DSP_Param    action_dsp;
};

nano_manipulatorControl *nano_manipulatorClass = NULL;

static void nano_manipulator_query(void)
{
  static GnomeUIInfo menuinfo[] = { 
    { GNOME_APP_UI_ITEM, 
      nano_manipulator_pi.menuentry, nano_manipulator_pi.help,
      (gpointer) nano_manipulator_show_callback, NULL,
      NULL, GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_BLANK, 
      0, GDK_CONTROL_MASK, NULL },

    GNOMEUIINFO_END
  };

  gnome_app_insert_menus
    ( GNOME_APP(nano_manipulator_pi.app->getApp()), 
      nano_manipulator_pi.menupath, menuinfo );

  // new ...
  nano_manipulatorClass = new nano_manipulatorControl;

  nano_manipulatorClass->SetResName ("Windownano_manipulatorControl", "false", xsmres.geomsave);

  nano_manipulator_pi.app->ConnectPluginToStartScanEvent
    ( nano_manipulator_StartScan_callback );

  nano_manipulator_pi.status = g_strconcat(N_("Plugin query has attached "),
				     nano_manipulator_pi.name, 
				     N_(": nano_manipulator is created."),
				     NULL);
}

static void nano_manipulator_about(void)
{
  const gchar *authors[] = { "Percy Zahl", NULL};
  gtk_show_about_dialog (NULL, "program-name",  nano_manipulator_pi.name,
				  "version", VERSION,
				    "license", GTK_LICENSE_GPL_3_0,
				    "comments", about_text,
				    "authors", authors,
				    NULL
				    ));
}

static void nano_manipulator_cleanup( void ){
  PI_DEBUG (DBG_L2, "nano_manipulator Plugin Cleanup" );
  gchar *mp = g_strconcat(nano_manipulator_pi.menupath, nano_manipulator_pi.menuentry, NULL);
  gnome_app_remove_menus (GNOME_APP( nano_manipulator_pi.app->getApp() ), mp, 1);
  g_free(mp);

  // delete ...
  if( nano_manipulatorClass )
    delete nano_manipulatorClass ;

  g_free( nano_manipulator_pi.status );
}

static void nano_manipulator_show_callback( GtkWidget* widget, void* data){
  if( nano_manipulatorClass )
    nano_manipulatorClass->show();
}

static void nano_manipulator_StartScan_callback( gpointer ){
  nano_manipulatorClass->update();
}


nano_manipulatorControl::nano_manipulatorControl ()
{
  GtkWidget *box, *hbox;
  GtkWidget *frame_param;
  GtkWidget *vbox_param, *hbox_param;
  GtkWidget *input;

  GSList *EC_list=NULL;
  GSList *EC_pos_list=NULL;
  GSList **RemoteEntryList = new GSList *;
  *RemoteEntryList = NULL;

  Gtk_EntryControl *ec;

  Unity    = new UnitObj(" "," ");
  Volt     = new UnitObj("V","V");
  Current  = new UnitObj("nA","nA");
  Force    = new UnitObj("nN","nN");

  memcpy (&move_dsp, &nano_manipulator_pi.app->xsm->data.hardpars, 
	  sizeof (DSP_Param));
  memcpy (&action_dsp, &nano_manipulator_pi.app->xsm->data.hardpars, 
	  sizeof (DSP_Param));

  XsmRescourceManager xrm("Pluginnano_manipulatorControl");
  xrm.Get("nano_manipulator.X", &td_current.x, "0.");
  xrm.Get("nano_manipulator.Y", &td_current.y, "0.");
  xrm.Get("nano_manipulator.action_dsp.ITunnelSoll", &action_dsp.ITunnelSoll, "1.");
  xrm.Get("nano_manipulator.action_dsp.UTunnel", &action_dsp.UTunnel, "3.");
  xrm.Get("nano_manipulator.action_dsp.SetPoint", &action_dsp.SetPoint, "0.");
  xrm.Get("nano_manipulator.action_dsp.usrCP", &action_dsp.usrCP, "0.05");
  xrm.Get("nano_manipulator.action_dsp.usrCI", &action_dsp.usrCI, "0.05");
  xrm.Get("nano_manipulator.action_dsp.usrCS", &action_dsp.CS, "0.0");
  xrm.Get("nano_manipulator.action_dsp.MV_stepsize", &action_dsp.MV_stepsize, "1");
  xrm.Get("nano_manipulator.action_dsp.MV_nRegel", &action_dsp.MV_nRegel, "1");
  xrm.Get("nano_manipulator.move_dsp.ITunnelSoll", &move_dsp.ITunnelSoll, "0.1");
  xrm.Get("nano_manipulator.move_dsp.UTunnel", &move_dsp.UTunnel, "0.1");
  xrm.Get("nano_manipulator.move_dsp.SetPoint", &move_dsp.SetPoint, "0.");
  xrm.Get("nano_manipulator.move_dsp.usrCP", &move_dsp.usrCP, "0.05");
  xrm.Get("nano_manipulator.move_dsp.usrCI", &move_dsp.usrCI, "0.05");
  xrm.Get("nano_manipulator.move_dsp.usrCS", &move_dsp.CS, "0.0");
  xrm.Get("nano_manipulator.move_dsp.MV_stepsize", &move_dsp.MV_stepsize, "1");
  xrm.Get("nano_manipulator.move_dsp.MV_nRegel", &move_dsp.MV_nRegel, "1");


  AppWindowInit("Nano Manipulator Control");

  box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  gtk_widget_show (box);
  gtk_box_pack_start (GTK_BOX (vbox), box, TRUE, TRUE, 0);

  hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_widget_show (hbox);
  gtk_box_pack_start (GTK_BOX (box), hbox, TRUE, TRUE, 0);

  // ========================================
#define MYGTK_INPUT(L)  mygtk_create_input(L, vbox_param, hbox_param, 50, 70);

  frame_param = gtk_frame_new (N_("Settings for movements"));
  gtk_widget_show (frame_param);
  gtk_container_add (GTK_CONTAINER (hbox), frame_param);

  vbox_param = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  gtk_widget_show (vbox_param);
  gtk_container_add (GTK_CONTAINER (frame_param), vbox_param);


  input = MYGTK_INPUT("U");
  ec = new Gtk_EntryControl (Volt, MLD_WERT_NICHT_OK, &move_dsp.UTunnel, -10., 10., "5g", input);
  EC_list = g_slist_prepend( EC_list, ec);

  input = MYGTK_INPUT("I");
  ec = new Gtk_EntryControl (Current, MLD_WERT_NICHT_OK, &move_dsp.ITunnelSoll, 0.001, 50., "5g", input);
  EC_list = g_slist_prepend( EC_list, ec);

  input = MYGTK_INPUT("SP");
  ec = new Gtk_EntryControl (Force, MLD_WERT_NICHT_OK, &move_dsp.SetPoint, -10000., 10000., "5g", input);
  EC_list = g_slist_prepend( EC_list, ec);

  input = MYGTK_INPUT("CP");
  ec = new Gtk_EntryControl (Unity, MLD_WERT_NICHT_OK, &move_dsp.usrCP, 0., 10., "5g", input);
  EC_list = g_slist_prepend( EC_list, ec);

  input = MYGTK_INPUT("CI");
  ec = new Gtk_EntryControl (Unity, MLD_WERT_NICHT_OK, &move_dsp.usrCI, 0., 10., "5g", input);
  EC_list = g_slist_prepend( EC_list, ec);

  input = MYGTK_INPUT("CS");
  ec = new Gtk_EntryControl (Unity, MLD_WERT_NICHT_OK, &move_dsp.CS, 0., 2., "5g", input);
  EC_list = g_slist_prepend( EC_list, ec);

  input = MYGTK_INPUT("Speed");
  ec = new Gtk_EntryControl (Unity, MLD_WERT_NICHT_OK, &move_dsp.MV_stepsize, 1., 100., "4.0f", input);
  EC_list = g_slist_prepend( EC_list, ec);

  input = MYGTK_INPUT("#Loops");
  ec = new Gtk_EntryControl (Unity, MLD_WERT_NICHT_OK, &move_dsp.MV_nRegel, 1., 1000., "4.0f", input);
  EC_list = g_slist_prepend( EC_list, ec);

  // ========================================
  frame_param = gtk_frame_new (N_("Settings for actions"));
  gtk_widget_show (frame_param);
  gtk_container_add (GTK_CONTAINER (hbox), frame_param);

  vbox_param = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  gtk_widget_show (vbox_param);
  gtk_container_add (GTK_CONTAINER (frame_param), vbox_param);


  input = MYGTK_INPUT("U");
  ec = new Gtk_EntryControl (Volt, MLD_WERT_NICHT_OK, &action_dsp.UTunnel, -10., 10., "5g", input);
  EC_list = g_slist_prepend( EC_list, ec);

  input = MYGTK_INPUT("I");
  ec = new Gtk_EntryControl (Current, MLD_WERT_NICHT_OK, &action_dsp.ITunnelSoll, 0.001, 50., "5g", input);
  EC_list = g_slist_prepend( EC_list, ec);

  input = MYGTK_INPUT("SP");
  ec = new Gtk_EntryControl (Force, MLD_WERT_NICHT_OK, &action_dsp.SetPoint, -10000., 10000., "5g", input);
  EC_list = g_slist_prepend( EC_list, ec);

  input = MYGTK_INPUT("CP");
  ec = new Gtk_EntryControl (Unity, MLD_WERT_NICHT_OK, &action_dsp.usrCP, 0., 10., "5g", input);
  EC_list = g_slist_prepend( EC_list, ec);

  input = MYGTK_INPUT("CI");
  ec = new Gtk_EntryControl (Unity, MLD_WERT_NICHT_OK, &action_dsp.usrCI, 0., 10., "5g", input);
  EC_list = g_slist_prepend( EC_list, ec);

  input = MYGTK_INPUT("CS");
  ec = new Gtk_EntryControl (Unity, MLD_WERT_NICHT_OK, &action_dsp.CS, 0., 2., "5g", input);
  EC_list = g_slist_prepend( EC_list, ec);

  input = MYGTK_INPUT("Speed");
  ec = new Gtk_EntryControl (Unity, MLD_WERT_NICHT_OK, &action_dsp.MV_stepsize, 1., 100., "4.0f", input);
  EC_list = g_slist_prepend( EC_list, ec);

  input = MYGTK_INPUT("#Loops");
  ec = new Gtk_EntryControl (Unity, MLD_WERT_NICHT_OK, &action_dsp.MV_nRegel, 1., 1000., "4.0f", input);
  EC_list = g_slist_prepend( EC_list, ec);

  // ========================================
  frame_param = gtk_frame_new (N_("Current Nano Manipulator Position"));
  gtk_widget_show (frame_param);
  gtk_container_add (GTK_CONTAINER (vbox), frame_param);

  vbox_param = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  gtk_widget_show (vbox_param);
  gtk_container_add (GTK_CONTAINER (frame_param), vbox_param);


// Current Position/Values
  input = mygtk_create_input("XYZ:", vbox_param, hbox_param);
  ec = new Gtk_EntryControl 
	  (nano_manipulator_pi.app->xsm->X_Unit, MLD_WERT_NICHT_OK, &td_current.x,
	   nano_manipulator_pi.app->xsm->XOffsetMin(), nano_manipulator_pi.app->xsm->XOffsetMax(), 
	   nano_manipulator_pi.app->xsm->AktUnit->prec1, input);
  EC_list = g_slist_prepend( EC_list, ec);
  EC_pos_list = g_slist_prepend( EC_pos_list, ec);

  input = mygtk_add_input(hbox_param);
  ec = new Gtk_EntryControl 
	  (nano_manipulator_pi.app->xsm->Y_Unit, MLD_WERT_NICHT_OK, &td_current.y,
	   nano_manipulator_pi.app->xsm->YOffsetMin(), nano_manipulator_pi.app->xsm->YOffsetMax(), 
	   nano_manipulator_pi.app->xsm->AktUnit->prec1, input);
  EC_list = g_slist_prepend( EC_list, ec);
  EC_pos_list = g_slist_prepend( EC_pos_list, ec);

// Limits to fix...	
  input = mygtk_add_input(hbox_param);
  ec = new Gtk_EntryControl 
	  (nano_manipulator_pi.app->xsm->Z_Unit, MLD_WERT_NICHT_OK, &td_current.z,
	   nano_manipulator_pi.app->xsm->YOffsetMin(), nano_manipulator_pi.app->xsm->YOffsetMax(), 
	   nano_manipulator_pi.app->xsm->AktUnit->prec1, input);
  EC_list = g_slist_prepend( EC_list, ec);
  EC_pos_list = g_slist_prepend( EC_pos_list, ec);

// ---
	
  input = mygtk_create_input("I,V1,V2:", vbox_param, hbox_param);
  ec = new Gtk_EntryControl 
	  (Current, MLD_WERT_NICHT_OK, &td_current.v[0],
	   -100000., 100000., 
	   "g", input);
  EC_list = g_slist_prepend( EC_list, ec);
  EC_pos_list = g_slist_prepend( EC_pos_list, ec);

//  input = mygtk_create_input("Scale XY", vbox_param, hbox_param);
//  ec = new Gtk_EntryControl (Unity, MLD_WERT_NICHT_OK, &XScale, -1e8, 1e8, "5g", input);
//  EC_list = g_slist_prepend( EC_list, ec);
//  input = mygtk_add_input(hbox_param);
//  ec = new Gtk_EntryControl (Unity, MLD_WERT_NICHT_OK, &YScale, -1e8, 1e8, "5g", input);
//  EC_list = g_slist_prepend( EC_list, ec);

// PlotFile
//  input = mygtk_create_input("PlotFile", vbox_param, hbox_param);
//  g_object_set_data( G_OBJECT (vbox), "PlotFileEntry", input);
//  g_signal_connect (G_OBJECT (input), "changed",
//			    G_CALLBACK (cbbasename),
//			    this);
// G_FREE_STRDUP(PlotFile, gtk_entry_get_text (GTK_ENTRY (g_object_get_data( G_OBJECT (as_control))),

// Run Button
  GtkWidget *button = gtk_button_new_with_label("Start Plot");
  gtk_widget_show (button);
  gtk_box_pack_start (GTK_BOX (hbox_param), button, TRUE, TRUE, 0);
//  g_object_set_data( G_OBJECT (button), "PlotFileEntry", input);
  g_signal_connect ( G_OBJECT (button), "pressed",
		       G_CALLBACK (nano_manipulatorControl::RunPlott),
		       this);

// CmdList

  // save List away...
  g_object_set_data( G_OBJECT (widget), "NANOPLOTT_EC_list", EC_list);
  g_object_set_data( G_OBJECT (widget), "NANOPLOTT_EC_POS_list", EC_pos_list);


  // connect to DSP
  devpcdspmon = open(xsmres.DSPDev, O_RDWR);
  lseek(devpcdspmon, (DSP_OSZI_CH12) << 2, SEEK_SET);

}

nano_manipulatorControl::~nano_manipulatorControl (){

	close (devpcdspmon);

	XsmRescourceManager xrm("Pluginnano_manipulatorControl");
	xrm.Put("nano_manipulator.X", td_current.x);
	xrm.Put("nano_manipulator.Y", td_current.y);
	xrm.Put("nano_manipulator.action_dsp.ITunnelSoll", action_dsp.ITunnelSoll);
	xrm.Put("nano_manipulator.action_dsp.UTunnel", action_dsp.UTunnel);
	xrm.Put("nano_manipulator.action_dsp.SetPoint", action_dsp.SetPoint);
	xrm.Put("nano_manipulator.action_dsp.usrCP", action_dsp.usrCP);
	xrm.Put("nano_manipulator.action_dsp.usrCI", action_dsp.usrCI);
	xrm.Put("nano_manipulator.action_dsp.usrCS", action_dsp.CS);
	xrm.Put("nano_manipulator.action_dsp.MV_stepsize", action_dsp.MV_stepsize);
	xrm.Put("nano_manipulator.action_dsp.MV_nRegel", action_dsp.MV_nRegel);
	xrm.Put("nano_manipulator.move_dsp.ITunnelSoll", move_dsp.ITunnelSoll);
	xrm.Put("nano_manipulator.move_dsp.Umove", move_dsp.UTunnel);
	xrm.Put("nano_manipulator.move_dsp.SetPoint", move_dsp.SetPoint);
	xrm.Put("nano_manipulator.move_dsp.usrCP", move_dsp.usrCP);
	xrm.Put("nano_manipulator.move_dsp.usrCI", move_dsp.usrCI);
	xrm.Put("nano_manipulator.move_dsp.usrCS", move_dsp.CS);
	xrm.Put("nano_manipulator.move_dsp.MV_stepsize", move_dsp.MV_stepsize);
	xrm.Put("nano_manipulator.move_dsp.MV_nRegel", move_dsp.MV_nRegel);
	
	delete Unity;
	delete Volt;
	delete Current;
	delete Force;
}

void nano_manipulatorControl::update(){
  g_slist_foreach((GSList*)g_object_get_data( G_OBJECT (widget), "NANOPLOTT_EC_list"),
		  (GFunc) App::update_ec, NULL);
}

void nano_manipulatorControl::update_pos(){
  g_slist_foreach((GSList*)g_object_get_data( G_OBJECT (widget), "NANOPLOTT_EC_POS_list"),
		  (GFunc) App::update_ec, NULL);
}

void nano_manipulatorControl::RunPlott(GtkWidget *widget, nano_manipulatorControl *npc){
/*
	npc->SetNewParam (&npc->move_dsp);
	npc->GoToPosition (x, y);

	if (nano_manipulator_pi.app->xsm->ActiveScan)
		if (nano_manipulator_pi.app->xsm->ActiveScan->view)
			nano_manipulator_pi.app->xsm->ActiveScan->view->add_object (0, (gpointer) xy);

	npc->SetNewParam (&npc->action_dsp);
	npc->GoToPosition (x, y);

	if (nano_manipulator_pi.app->xsm->ActiveScan)
		if (nano_manipulator_pi.app->xsm->ActiveScan->view)
			nano_manipulator_pi.app->xsm->ActiveScan->view->add_object (0, (gpointer) xy);

	npc->SetNewParam (&nano_manipulator_pi.app->xsm->data.hardpars);
*/

	double ch[8];
	static unsigned long dposzibuf[30];
	unsigned long *dp;
	union dpram_DataPack{ 
		unsigned long l;
		struct { signed short lo:16;
			signed short hi:16; 
		} ii;
	} datapack;
	
	read (npc->devpcdspmon, dposzibuf, sizeof(long)*10);
	
	dp = dposzibuf;
	datapack.l = *dp++;
	ch[0] = (double)datapack.ii.lo * 10./32767.;
	ch[1] = (double)datapack.ii.hi * 10./32767.;
	datapack.l = *dp++;
	ch[2] = (double)datapack.ii.lo * 10./32767.;
	ch[3] = (double)datapack.ii.hi * 10./32767.;
	datapack.l = *dp++;
	ch[4] = (double)datapack.ii.lo * 10./32767.;
	ch[5] = (double)datapack.ii.hi * 10./32767.;
	datapack.l = *dp++;
	ch[6] = (double)datapack.ii.lo * 10./32767.;
	ch[7] = (double)datapack.ii.hi * 10./32767.;

	npc->td_current.x = ch[6];
	npc->td_current.y = ch[7];
	npc->td_current.z = ch[4];
	npc->td_current.v[0] = ch[0];
	npc->td_current.v[1] = ch[1];
	npc->td_current.v[2] = ch[5];

	npc->update_pos();

}

void nano_manipulatorControl::SetNewParam(DSP_Param *dsp){
	dsp->CP = dsp->usrCP/nano_manipulator_pi.app->xsm->Inst->VZ();
	dsp->CI = dsp->usrCI/nano_manipulator_pi.app->xsm->Inst->VZ();
 	nano_manipulator_pi.app->xsm->hardware->PutParameter(dsp);
}


void nano_manipulatorControl::GoToPosition(double x, double y){
	nano_manipulator_pi.app->xsm->hardware->SetOffset(
		R2INT(nano_manipulator_pi.app->xsm->Inst->X0A2Dig(x)),
		R2INT(nano_manipulator_pi.app->xsm->Inst->Y0A2Dig(y)));
}
