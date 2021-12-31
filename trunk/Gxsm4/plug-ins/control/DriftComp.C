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
% PlugInDocuCaption: Drift Compensation (OBSOLETE)
% PlugInName: DriftComp
% PlugInAuthor: Percy Zahl
% PlugInAuthorEmail: zahl@users.sf.net
% PlugInMenuPath: windows-sectionDrift Comp.

% PlugInDescription
This PlugIn will attempt to compensate image drifting inbetween
scanlines and images.

%% PlugInUsage
%Write how to use it.

%% OptPlugInSources
%The active channel is used as data source.

%% OptPlugInObjects
%A optional rectangle is used for data extraction...

%% OptPlugInConfig
%describe the configuration options of your plug in here!

% OptPlugInNotes
This PlugIn is in a early design phase. And did not work at all.

%% OptPlugInHints
%Any tips and tricks?

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



static void DriftComp_about( void );
static void DriftComp_query( void );
static void DriftComp_cleanup( void );

static void DriftComp_show_callback( GtkWidget*, void* );
static void DriftComp_StartScan_callback( gpointer );

GxsmPlugin DriftComp_pi = {
	NULL,
	NULL,
	0,
	NULL,
	"DriftComp",
	"+spmHARD +STM +AFM",
	NULL,
	"Percy Zahl",
	"windows-section",
	N_("Drift Comp."),
	N_("open the drift compensation controlwindow"),
	"drift compensation control",
	NULL,
	NULL,
	NULL,
	DriftComp_query,
	DriftComp_about,
	NULL,
	NULL,
	DriftComp_cleanup
};

static const char *about_text = N_("Gxsm DriftComp Plugin:\n"
				   "This plugin runs a control window to enable "
				   "drift-compensation between scans (movie). "
				   "Therfore a drift speed and direction can be specified.\n"
				   "May be extended to inner scan corrections... PZ"
	);

GxsmPlugin *get_gxsm_plugin_info ( void ){ 
	DriftComp_pi.description = g_strdup_printf(N_("Gxsm DriftComp plugin %s"), VERSION);
	return &DriftComp_pi; 
}

/*
 * Anti Drift
 */

typedef struct AntiDrift_Param{
	double vx, vy, tx, ty;
	double px, py, tcx, tcy;
	int mode;
};

typedef union AmpIndex {
	struct { unsigned char ch, x, y, z; } s;
	unsigned long   l;
};

class DriftControl : public AppBase{
public:
	DriftControl();
	virtual ~DriftControl();

	static void ChangedNotify(Param_Control* pcs, gpointer data);
	void update();

private:
	UnitObj *Unity, *Volt, *Time, *Speed, *Creep;
	AntiDrift_Param adp;
};

DriftControl *DriftCompClass = NULL;

static void DriftComp_query(void)
{
	static GnomeUIInfo menuinfo[] = { 
		{ GNOME_APP_UI_ITEM, 
		  DriftComp_pi.menuentry, DriftComp_pi.help,
		  (gpointer) DriftComp_show_callback, NULL,
		  NULL, GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_BLANK, 
		  0, GDK_CONTROL_MASK, NULL },

		GNOMEUIINFO_END
	};

	gnome_app_insert_menus
		( GNOME_APP(DriftComp_pi.app->getApp()), 
		  DriftComp_pi.menupath, menuinfo );

	// new ...
	DriftCompClass = new DriftControl;

	DriftCompClass->SetResName ("WindowDriftControl", "false", xsmres.geomsave);

	DriftComp_pi.app->ConnectPluginToStartScanEvent
		( DriftComp_StartScan_callback );

	DriftComp_pi.status = g_strconcat(N_("Plugin query has attached "),
					  DriftComp_pi.name, 
					  N_(": DriftComp is created."),
					  NULL);
}

static void DriftComp_about(void)
{
	const gchar *authors[] = { "Percy Zahl", NULL};
	gtk_show_about_dialog (NULL, "program-name",  DriftComp_pi.name,
					"version", VERSION,
					  "license", GTK_LICENSE_GPL_3_0,
					  "comments", about_text,
					  "authors", authors,
					  NULL
				));
}

static void DriftComp_cleanup( void ){
	PI_DEBUG (DBG_L2, "DriftComp Plugin Cleanup" );
	gchar *mp = g_strconcat(DriftComp_pi.menupath, DriftComp_pi.menuentry, NULL);
	gnome_app_remove_menus (GNOME_APP( DriftComp_pi.app->getApp() ), mp, 1);
	g_free(mp);

	// delete ...
	if( DriftCompClass )
		delete DriftCompClass ;

	g_free( DriftComp_pi.status );
}

static void DriftComp_show_callback( GtkWidget* widget, void* data){
	if( DriftCompClass )
		DriftCompClass->show();
}

static void DriftComp_StartScan_callback( gpointer ){
	DriftCompClass->update();
}


DriftControl::DriftControl ()
{
	Gtk_EntryControl *ec;

	Unity    = new UnitObj(" "," ");
	Volt     = new UnitObj("V","V");
	Speed    = new UnitObj("A/min","A/min");
	Creep    = new UnitObj("A/Vs","A/Vs");
	Time     = new UnitObj("s","s");

	GSList *EC_list=NULL;

	GtkWidget *box;
	GtkWidget *frame_param;
	GtkWidget *vbox_param, *hbox_param;
	GtkWidget *input;

	XsmRescourceManager xrm("PluginDriftControl");
	xrm.Get("adp.vx", &adp.vx, "0");
	xrm.Get("adp.vy", &adp.vy, "0");
	xrm.Get("adp.tx", &adp.tx, "0");
	xrm.Get("adp.ty", &adp.ty, "0");
	xrm.Get("adp.px", &adp.px, "0");
	xrm.Get("adp.py", &adp.py, "0");
	xrm.Get("adp.tcx", &adp.tcx, "0");
	xrm.Get("adp.tcy", &adp.tcy, "0");
	xrm.Get("adp.mode", &adp.mode, "0");

	AppWindowInit(ANTIDRIFT_TITLE);

	box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	gtk_widget_show (box);
	gtk_box_pack_start (GTK_BOX (vbox), box, TRUE, TRUE, 0);

	// ========================================
	frame_param = gtk_frame_new (N_("thermic drift"));
	gtk_widget_show (frame_param);
	gtk_container_add (GTK_CONTAINER (box), frame_param);

	vbox_param = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	gtk_widget_show (vbox_param);
	gtk_container_add (GTK_CONTAINER (frame_param), vbox_param);

	input = mygtk_create_input("Vx", vbox_param, hbox_param);
	ec = new Gtk_EntryControl (Speed, MLD_WERT_NICHT_OK, &adp.vx, -1000., 1000., "5.1f", input);
	ec->Set_ChangeNoticeFkt(DriftControl::ChangedNotify, this);
	EC_list = g_slist_prepend( EC_list, ec);

	input = mygtk_create_input("Vy", vbox_param, hbox_param);
	ec = new Gtk_EntryControl (Speed, MLD_WERT_NICHT_OK, &adp.vy, -1000., 1000., "5.1f", input);
	ec->Set_ChangeNoticeFkt(DriftControl::ChangedNotify, this);
	EC_list = g_slist_prepend( EC_list, ec);

	input = mygtk_create_input("tx", vbox_param, hbox_param);
	ec = new Gtk_EntryControl (Time, MLD_WERT_NICHT_OK, &adp.tx, -1000., 1000., "5.1f", input);
	ec->Set_ChangeNoticeFkt(DriftControl::ChangedNotify, this);
	EC_list = g_slist_prepend( EC_list, ec);

	input = mygtk_create_input("ty", vbox_param, hbox_param);
	ec = new Gtk_EntryControl (Time, MLD_WERT_NICHT_OK, &adp.ty, -1000., 1000., "5.1f", input);
	ec->Set_ChangeNoticeFkt(DriftControl::ChangedNotify, this);
	EC_list = g_slist_prepend( EC_list, ec);


	// save List away...
	g_object_set_data( G_OBJECT (widget), "DRIFT_EC_list", EC_list);

}

DriftControl::~DriftControl (){

	XsmRescourceManager xrm("PluginDriftControl");
	xrm.Put("adp.vx", adp.vx);
	xrm.Put("adp.vy", adp.vy);
	xrm.Put("adp.tx", adp.tx);
	xrm.Put("adp.ty", adp.ty);
	xrm.Put("adp.px", adp.px);
	xrm.Put("adp.py", adp.py);
	xrm.Put("adp.tcx", adp.tcx);
	xrm.Put("adp.tcy", adp.tcy);
	xrm.Put("adp.mode", adp.mode);

	delete Unity;
	delete Volt;
	delete Time;
	delete Speed;
	delete Creep;
}

void DriftControl::update(){
	g_slist_foreach((GSList*)g_object_get_data( G_OBJECT (widget), "DRIFT_EC_list"),
			(GFunc) App::update_ec, NULL);
}

void DriftControl::ChangedNotify(Param_Control* pcs, gpointer data){
	PI_DEBUG (DBG_L2, "Drift Param. Changed!" );
}
