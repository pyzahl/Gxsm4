/* Gnome gxsm - Gnome X Scanning Microscopy
 * universal STM/AFM/SARLS/SPALEED/... controlling and
 * data analysis software
 * 
 * Gxsm Plugin Name: CCDControl.C
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
% PlugInDocuCaption: CCD exposure control
% PlugInName: CCDControl
% PlugInAuthor: Percy Zahl
% PlugInAuthorEmail: zahl@users.sf.net
% PlugInMenuPath: windows-sectionCCD Control

% PlugInDescription
This is a control dialog for my (PZ) selfbuild Astro CCD camera, using
a TC211, a control unit described in CT 19xx on a parallel port of the
PC. It allows to set the exposure time and monitor control\dots

\GxsmScreenShot{CCDControl}{The CCD Control window.}

%% PlugInUsage
%Write how to use it.

%% OptPlugInConfig
%describe the configuration options of your plug in here!

% OptPlugInRefs
CT 19xx

%% OptPlugInKnownBugs
%Are there known bugs? List! How to work around if not fixed?

%% OptPlugInNotes
%If you have any additional notes

% EndPlugInDocuSection
 * -------------------------------------------------------------------------------- 
 */


#include <gtk/gtk.h>
#include "config.h"
#include "plugin.h"

#include "unit.h"
#include "pcs.h"
#include "glbvars.h"
#include "plug-ins/hard/modules/ccd.h"

#include "include/dsp-pci32/xsm/xsmcmd.h"



static void CCDControl_about( void );
static void CCDControl_query( void );
static void CCDControl_cleanup( void );

static void CCDControl_show_callback( GtkWidget*, void* );
static void CCDControl_StartScan_callback( gpointer );

GxsmPlugin CCDControl_pi = {
	NULL,
	NULL,
	0,
	NULL,
	"CCDControl",
	"+CCD",
	NULL,
	"Percy Zahl",
	"windows-section",
	N_("CCD Control"),
	N_("open the CCD exposure controlwindow"),
	"CCD exposure control",
	NULL,
	NULL,
	NULL,
	CCDControl_query,
	CCDControl_about,
	NULL,
	NULL,
	CCDControl_cleanup
};

static const char *about_text = N_("Gxsm CCDControl Plugin:\n"
				   "This plugin runs a control window to set "
				   "CCD exposure time and monitoring control mode."
	);

GxsmPlugin *get_gxsm_plugin_info ( void ){ 
	CCDControl_pi.description = g_strdup_printf(N_("Gxsm CCDControl plugin %s"), VERSION);
	return &CCDControl_pi; 
}


class CCDControl : public AppBase{
public:
	CCDControl();
	virtual ~CCDControl();

	void update();
	void updateCCD(int cmd=-1);
	static void ExecCmd(int cmd);
	static void ChangedNotify(Param_Control* pcs, gpointer data);
	static int ChangedAction(GtkWidget *widget, CCDControl *dspc);
	static int monitor_callback(GtkWidget *widget, CCDControl *dspc);

	/*
	  static int autoexp_callback(GtkWidget *widget, CCDControl *dspc);

	  static gint PFtmoutfkt(DSPProbeControl *pc);

	  void addPFtmout(){
	  if(!IdPFtmout)
	  IdPFtmout = gtk_timeout_add(PFtmoutms, PFtmoutfkt, this);
	  };
	  void rmPFtmout(){
	  if(IdPFtmout){
	  gtk_timeout_remove(IdPFtmout);
	  IdPFtmout=0;
	  }
	  };
	  gint IdPFtmout;

	*/

private:
	UnitObj *Unity, *Time;
	double CCD_exptime;
};


CCDControl *CCDControlClass = NULL;

static void CCDControl_query(void)
{
	static GnomeUIInfo menuinfo[] = { 
		{ GNOME_APP_UI_ITEM, 
		  CCDControl_pi.menuentry, CCDControl_pi.help,
		  (gpointer) CCDControl_show_callback, NULL,
		  NULL, GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_BLANK, 
		  0, GDK_CONTROL_MASK, NULL },

		GNOMEUIINFO_END
	};

	gnome_app_insert_menus
		( GNOME_APP(CCDControl_pi.app->getApp()), 
		  CCDControl_pi.menupath, menuinfo );

	// new ...
	CCDControlClass = new CCDControl;

	CCDControlClass->SetResName ("WindowCCDControl", "false", xsmres.geomsave);

	CCDControl_pi.app->ConnectPluginToStartScanEvent
		( CCDControl_StartScan_callback );

	CCDControl_pi.status = g_strconcat(N_("Plugin query has attached "),
					   CCDControl_pi.name, 
					   N_(": CCDControl is created."),
					   NULL);
}

static void CCDControl_about(void)
{
	const gchar *authors[] = { "Percy Zahl", NULL};
	gtk_show_about_dialog (NULL, "program-name",  CCDControl_pi.name,
					"version", VERSION,
					  "license", GTK_LICENSE_GPL_3_0,
					  "comments", about_text,
					  "authors", authors,
					  NULL
				));
}

static void CCDControl_cleanup( void ){
	PI_DEBUG (DBG_L2, "CCDControl Plugin Cleanup" );
	gchar *mp = g_strconcat(CCDControl_pi.menupath, CCDControl_pi.menuentry, NULL);
	gnome_app_remove_menus (GNOME_APP( CCDControl_pi.app->getApp() ), mp, 1);
	g_free(mp);

	// delete ...
	if( CCDControlClass )
		delete CCDControlClass ;
}

static void CCDControl_show_callback( GtkWidget* widget, void* data){
	if( CCDControlClass )
		CCDControlClass->show();
}

static void CCDControl_StartScan_callback( gpointer ){
	CCDControlClass->update();
}

// Achtung: Remote is not released :=(
// DSP-Param sollten lokal werden...
CCDControl::CCDControl ()
{
	GSList *EC_list=NULL;
	//  GSList **RemoteEntryList = new GSList *;
	//  *RemoteEntryList = NULL;

	Gtk_EntryControl *ec;

	GtkWidget *box;
	GtkWidget *frame_param, *vbox_param, *hbox_param;

	GtkWidget *input;

	GtkWidget *label;
	GtkWidget *checkbutton;

	Unity    = new UnitObj(" "," ");
	Time     = new UnitObj("ms","ms");

	AppWindowInit(N_("CCD Control"));

	box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	gtk_widget_show (box);
	gtk_box_pack_start (GTK_BOX (vbox), box, TRUE, TRUE, 0);

	// ========================================
	frame_param = gtk_frame_new (N_("CCD-Camera Settings"));
	gtk_widget_show (frame_param);
	gtk_container_add (GTK_CONTAINER (box), frame_param);

	vbox_param = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	gtk_widget_show (vbox_param);
	gtk_container_add (GTK_CONTAINER (frame_param), vbox_param);

	// ------- CCD Settings

	hbox_param = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_widget_show (hbox_param);
	gtk_container_add (GTK_CONTAINER (vbox_param), hbox_param);
	label = gtk_label_new (N_("CCD"));
	gtk_widget_set_size_request (label, 100, -1);
	gtk_widget_show (label);
	gtk_box_pack_start (GTK_BOX (hbox_param), label, FALSE, TRUE, 0);

	checkbutton = gtk_check_button_new_with_label(N_("Monitor"));
	gtk_widget_set_size_request (checkbutton, 100, -1);
	gtk_box_pack_start (GTK_BOX (hbox_param), checkbutton, TRUE, TRUE, 0);
	gtk_widget_show (checkbutton);
	g_signal_connect (G_OBJECT (checkbutton), "clicked",
			    G_CALLBACK (CCDControl::monitor_callback), this);


	input = mygtk_create_input("Exposure", vbox_param, hbox_param);
	ec = new Gtk_EntryControl (Time, MLD_WERT_NICHT_OK, &CCD_exptime, 40., 99999., ".0f", input);
	ec->Set_ChangeNoticeFkt(CCDControl::ChangedNotify, this);
	EC_list = g_slist_prepend( EC_list, ec);
	//  *RemoteEntryList = ec->AddEntry2RemoteList("CCD_Exposure", *RemoteEntryList);

	// save List away...
	g_object_set_data( G_OBJECT (widget), "CCD_EC_list", EC_list);
}

CCDControl::~CCDControl (){
	delete Unity;
	delete Time;
}

void CCDControl::update(){
	g_slist_foreach((GSList*)g_object_get_data( G_OBJECT (widget), "CCD_EC_list"),
			(GFunc) App::update_ec, NULL);
}

void CCDControl::updateCCD(int cmd){
	PI_DEBUG (DBG_L2, "Hallo CCD ! cmd=" << cmd );
// **=	CCDControl_pi.app->xsm->hardware->PutParameter(dsp);
	if(cmd > 0)
		ExecCmd(cmd);
}

int CCDControl::ChangedAction(GtkWidget *widget, CCDControl *dspc){
	dspc->updateCCD();
	return 0;
}

void CCDControl::ChangedNotify(Param_Control* pcs, gpointer dspc){
	//  gchar *us=pcs->Get_UsrString();
	//  PI_DEBUG (DBG_L2, "CCD: " << us );
	//  g_free(us);
	((CCDControl*)dspc)->updateCCD();
}

void CCDControl::ExecCmd(int cmd){
	// Do it now here.
}

int CCDControl::monitor_callback( GtkWidget *widget, CCDControl *dspc){
	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget)))
		dspc->updateCCD(CCD_CMD_MONITORENABLE);
	return 0;
}
