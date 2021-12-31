/* Gnome gxsm - Gnome X Scanning Microscopy
 * universal STM/AFM/SARLS/SPALEED/... controlling and
 * data analysis software
 * 
 * Gxsm Plugin Name: SpaLeedControl.C
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
% PlugInDocuCaption: SPA--LEED simulator control
% PlugInName: SpaLeedControl
% PlugInAuthor: Percy Zahl
% PlugInAuthorEmail: zahl@users.sf.net
% PlugInMenuPath: windows-sectionSPA-LEED Ctrl

% PlugInDescription
A frontend for SPA--LEED and E-gun parameter control, currently used
to configure the SPA--LEED simulator kernel module. But it has the
potential for remote controling a future SPA--LEED unit :-)

% PlugInUsage
Open \GxsmMenu{windows-sectionSPA-LEED Ctrl} and play.
\GxsmScreenShot{SPALEED-Control}{The SPA--LEED Control window.}

Not intuitive?

%% OptPlugInConfig

% OptPlugInNotes
The SPA--LEED simulator kernel module is here:\\
\GxsmFile{Gxsm/plug-ins/hard/modules/dspspaemu.o}


%% OptPlugInHints

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

#include "include/dsp-pci32/spa/spacmd.h"



static void SpaLeedControl_about( void );
static void SpaLeedControl_query( void );
static void SpaLeedControl_cleanup( void );

static void SpaLeedControl_show_callback( GtkWidget*, void* );
static void SpaLeedControl_StartScan_callback( gpointer );

GxsmPlugin SpaLeedControl_pi = {
  NULL,
  NULL,
  0,
  NULL,
  "SpaLeedControl",
  "+SPALEED",
  NULL,
  "Percy Zahl",
  "windows-section",
  N_("SPA-LEED Ctrl"),
  N_("open the SPA-LEED controlwindow"),
  "SPA-LEED control",
  NULL,
  NULL,
  NULL,
  SpaLeedControl_query,
  SpaLeedControl_about,
  NULL,
  NULL,
  SpaLeedControl_cleanup
};

static const char *about_text = N_("Gxsm SpaLeedControl Plugin:\n"
				   "This plugin runs a control window to change "
				   "the SPALEED simulator settimgs."
				   );

GxsmPlugin *get_gxsm_plugin_info ( void ){ 
  SpaLeedControl_pi.description = g_strdup_printf(N_("Gxsm SpaLeedControl plugin %s"), VERSION);
  return &SpaLeedControl_pi; 
}


class SpaLeedControl : public AppBase{
public:
  SpaLeedControl();
  virtual ~SpaLeedControl();

  void update();
  void updateSPALEED(int cmd=-1);
  static void ExecCmd(int cmd);
  static void ChangedNotify(Param_Control* pcs, gpointer data);
  static int ChangedAction(GtkWidget *widget, SpaLeedControl *spac);
  static int epitaxy_callback(GtkWidget *widget, SpaLeedControl *spac);

private:
  UnitObj *Unity, *Current, *Volt, *Percent, *Kelvin;

  gchar *ResName;
  double extractor;
  double chanhv;
  double chanrepeller;
  double cryfocus;
  double filament;
  double gunfocus;
  double gunanode;
  double smpdist;
  double smptemp;
  double growing;
};


SpaLeedControl *SpaLeedControlClass = NULL;

static void SpaLeedControl_query(void)
{
  static GnomeUIInfo menuinfo[] = { 
    { GNOME_APP_UI_ITEM, 
      SpaLeedControl_pi.menuentry, SpaLeedControl_pi.help,
      (gpointer) SpaLeedControl_show_callback, NULL,
      NULL, GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_BLANK, 
      0, GDK_CONTROL_MASK, NULL },

    GNOMEUIINFO_END
  };

  gnome_app_insert_menus
    ( GNOME_APP(SpaLeedControl_pi.app->getApp()), 
      SpaLeedControl_pi.menupath, menuinfo );

  // new ...
  SpaLeedControlClass = new SpaLeedControl;

  SpaLeedControlClass->SetResName ("WindowSpaLeedControl", "false", xsmres.geomsave);

  SpaLeedControl_pi.app->ConnectPluginToStartScanEvent
    ( SpaLeedControl_StartScan_callback );

  SpaLeedControl_pi.status = g_strconcat(N_("Plugin query has attached "),
				     SpaLeedControl_pi.name, 
				     N_(": SpaLeedControl is created."),
				     NULL);
}

static void SpaLeedControl_about(void)
{
  const gchar *authors[] = { "Percy Zahl", NULL};
  gtk_show_about_dialog (NULL, "program-name",  SpaLeedControl_pi.name,
				  "version", VERSION,
				    "license", GTK_LICENSE_GPL_3_0,
				    "comments", about_text,
				    "authors", authors,
				    NULL
				    ));
}

static void SpaLeedControl_cleanup( void ){
  PI_DEBUG (DBG_L2, "SpaLeedControl Plugin Cleanup" );
  gchar *mp = g_strconcat(SpaLeedControl_pi.menupath, SpaLeedControl_pi.menuentry, NULL);
  gnome_app_remove_menus (GNOME_APP( SpaLeedControl_pi.app->getApp() ), mp, 1);
  g_free(mp);

  // delete ...
  if( SpaLeedControlClass )
    delete SpaLeedControlClass ;
}

static void SpaLeedControl_show_callback( GtkWidget* widget, void* data){
  if( SpaLeedControlClass )
    SpaLeedControlClass->show();
}

static void SpaLeedControl_StartScan_callback( gpointer ){
  SpaLeedControlClass->update();
}

// Achtung: Remote is not released :=(
// DSP-Param sollten lokal werden...
SpaLeedControl::SpaLeedControl ()
{
  GSList *EC_list=NULL;
  //  GSList **RemoteEntryList = new GSList *;
  //  *RemoteEntryList = NULL;

  Gtk_EntryControl *ec;

  GtkWidget *box;
  GtkWidget *frame_param, *vbox_param, *hbox_param, *frontpanel;

  GtkWidget *input;

  GtkWidget *checkbutton;

  ResName = g_strdup("SpaLeedControl");
  // get default values using the resource manager
  XsmRescourceManager xrm(ResName);
  xrm.Get("spa_extractor", &extractor, "1.0");
  xrm.Get("spa_chanhv", &chanhv, "2000.0");
  xrm.Get("spa_chanrepeller", &chanrepeller, "1.0");
  xrm.Get("spa_cryfocus", &cryfocus, "0.5");
  xrm.Get("spa_filament", &filament, "2.4");
  xrm.Get("spa_gunfocus", &gunfocus, "0.5");
  xrm.Get("spa_gunanode", &gunanode, "100.0");
  xrm.Get("spa_smpdist", &smpdist, "20.0");
  xrm.Get("spa_smptemp", &smptemp, "300.0");
  xrm.Get("spa_growing", &growing, "0.0");


  Unity    = new UnitObj(" "," ");
  Current  = new UnitObj("A","A");
  Volt     = new UnitObj("V","V");
  Percent  = new LinUnit("%%","%%",1e-2);
  Kelvin   = new UnitObj("K","K");

  AppWindowInit(N_("SPA-LEED Settings"));

  box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  gtk_widget_show (box);
  gtk_box_pack_start (GTK_BOX (vbox), box, TRUE, TRUE, 0);

  // ========================================
  frame_param = gtk_frame_new (N_("Leybold SPA-LEED Control"));
  gtk_widget_show (frame_param);
  gtk_container_add (GTK_CONTAINER (box), frame_param);

  frontpanel = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  gtk_widget_show (frontpanel);
  gtk_container_add (GTK_CONTAINER (frame_param), frontpanel);

  // ========================================
  frame_param = gtk_frame_new (N_("Energy"));
  gtk_widget_show (frame_param);
  gtk_container_add (GTK_CONTAINER (frontpanel), frame_param);

  vbox_param = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  gtk_widget_show (vbox_param);
  gtk_container_add (GTK_CONTAINER (frame_param), vbox_param);

  // ------- SPA-LEED Settings

  input = mygtk_create_input("Anode", vbox_param, hbox_param);
  ec = new Gtk_EntryControl 
    (Volt, MLD_WERT_NICHT_OK, &gunanode,
     0., 200., "5.2f", input, 1., 2.);
  SetupScale(ec->GetAdjustment(), hbox_param);
  ec->Set_ChangeNoticeFkt(SpaLeedControl::ChangedNotify, this);
  EC_list = g_slist_prepend( EC_list, ec);

  input = mygtk_create_input("Filament", vbox_param, hbox_param);
  ec = new Gtk_EntryControl 
    (Current, MLD_WERT_NICHT_OK, &filament,
     0., 3., "5.2f", input, 0.01, 0.02);
  SetupScale(ec->GetAdjustment(), hbox_param);
  ec->Set_ChangeNoticeFkt(SpaLeedControl::ChangedNotify, this);
  EC_list = g_slist_prepend( EC_list, ec);

  // ========================================
  frame_param = gtk_frame_new (N_("Focus"));
  gtk_widget_show (frame_param);
  gtk_container_add (GTK_CONTAINER (frontpanel), frame_param);

  vbox_param = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  gtk_widget_show (vbox_param);
  gtk_container_add (GTK_CONTAINER (frame_param), vbox_param);

  input = mygtk_create_input("Gun Focus", vbox_param, hbox_param);
  ec = new Gtk_EntryControl 
    (Percent, MLD_WERT_NICHT_OK, &gunfocus,
     0., 1., "4.3f", input, 0.01, 0.01);
  SetupScale(ec->GetAdjustment(), hbox_param);
  ec->Set_ChangeNoticeFkt(SpaLeedControl::ChangedNotify, this);
  EC_list = g_slist_prepend( EC_list, ec);

  input = mygtk_create_input("Cryst. Focus", vbox_param, hbox_param);
  ec = new Gtk_EntryControl 
    (Percent, MLD_WERT_NICHT_OK, &cryfocus,
     0., 1., "4.3f", input, 0.005, 0.005);
  SetupScale(ec->GetAdjustment(), hbox_param);
  ec->Set_ChangeNoticeFkt(SpaLeedControl::ChangedNotify, this);
  EC_list = g_slist_prepend( EC_list, ec);

  // ========================================
  frame_param = gtk_frame_new (N_("Channeltron"));
  gtk_widget_show (frame_param);
  gtk_container_add (GTK_CONTAINER (frontpanel), frame_param);

  vbox_param = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  gtk_widget_show (vbox_param);
  ec->Set_ChangeNoticeFkt(SpaLeedControl::ChangedNotify, this);
  gtk_container_add (GTK_CONTAINER (frame_param), vbox_param);

  input = mygtk_create_input("Repeller", vbox_param, hbox_param);
  ec = new Gtk_EntryControl 
    (Volt, MLD_WERT_NICHT_OK, &chanrepeller,
     0., 10., "5.2f", input, 0.01, 0.01);
  SetupScale(ec->GetAdjustment(), hbox_param);
  ec->Set_ChangeNoticeFkt(SpaLeedControl::ChangedNotify, this);
  EC_list = g_slist_prepend( EC_list, ec);

  input = mygtk_create_input("HV", vbox_param, hbox_param);
  ec = new Gtk_EntryControl 
    (Volt, MLD_WERT_NICHT_OK, &chanhv,
     0., 6e3, "5.0f", input, 10., 20.);
  SetupScale(ec->GetAdjustment(), hbox_param);
  ec->Set_ChangeNoticeFkt(SpaLeedControl::ChangedNotify, this);
  EC_list = g_slist_prepend( EC_list, ec);



  // ========================================
  frame_param = gtk_frame_new (N_("Experiment"));
  gtk_widget_show (frame_param);
  gtk_container_add (GTK_CONTAINER (frontpanel), frame_param);

  vbox_param = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  gtk_widget_show (vbox_param);
  gtk_container_add (GTK_CONTAINER (frame_param), vbox_param);

  input = mygtk_create_input("Temp.", vbox_param, hbox_param);
  ec = new Gtk_EntryControl 
    (Kelvin, MLD_WERT_NICHT_OK, &smptemp,
     0., 3000., "6.1f", input, 10., 10.);
  SetupScale(ec->GetAdjustment(), hbox_param);
  ec->Set_ChangeNoticeFkt(SpaLeedControl::ChangedNotify, this);
  EC_list = g_slist_prepend( EC_list, ec);

  input = mygtk_create_input("Growing", vbox_param, hbox_param);
  ec = new Gtk_EntryControl 
    (Unity, MLD_WERT_NICHT_OK, &growing,
     0., 10., "6.1f", input, 1., 0.1);
  SetupScale(ec->GetAdjustment(), hbox_param);
  ec->Set_ChangeNoticeFkt(SpaLeedControl::ChangedNotify, this);
  EC_list = g_slist_prepend( EC_list, ec);

  checkbutton = gtk_check_button_new_with_label(N_("Epitaxy"));
  gtk_widget_set_size_request (checkbutton, 100, -1);
  gtk_box_pack_start (GTK_BOX (hbox_param), checkbutton, TRUE, TRUE, 0);
  gtk_widget_show (checkbutton);
  g_signal_connect (G_OBJECT (checkbutton), "clicked",
		      G_CALLBACK (SpaLeedControl::epitaxy_callback), this);

  // save List away...
  g_object_set_data( G_OBJECT (widget), "SPALEED_EC_list", EC_list);
  update();
}

SpaLeedControl::~SpaLeedControl (){
  // save the actual values in resources
  XsmRescourceManager xrm(ResName);
  xrm.Put("spa_extractor", extractor);
  xrm.Put("spa_chanhv", chanhv);
  xrm.Put("spa_chanrepeller", chanrepeller);
  xrm.Put("spa_cryfocus", cryfocus);
  xrm.Put("spa_filament", filament);
  xrm.Put("spa_gunfocus", gunfocus);
  xrm.Put("spa_gunanode", gunanode);
  xrm.Put("spa_smpdist", smpdist);
  xrm.Put("spa_smptemp", smptemp);
  xrm.Put("spa_growing", growing);
  g_free(ResName);
  ResName=NULL;

  delete Unity;
  delete Current;
  delete Volt;
  delete Percent;
  delete Kelvin;
}

void SpaLeedControl::update(){
  g_slist_foreach((GSList*)g_object_get_data( G_OBJECT (widget), "SPALEED_EC_list"),
		  (GFunc) App::update_ec, NULL);
}

void SpaLeedControl::updateSPALEED(int cmd){
  PARAMETER_SET hardpar;

  hardpar.N   = DSP_SPACTRL_GROWING+1;
  hardpar.Cmd = DSP_CMD_SPACTRL_SET;
  
  hardpar.hp[DSP_SPACTRL_EXTRACTOR   ].value = extractor;
  hardpar.hp[DSP_SPACTRL_CHANHV      ].value = chanhv;
  hardpar.hp[DSP_SPACTRL_CHANREPELLER ].value = chanrepeller;
  hardpar.hp[DSP_SPACTRL_CRYFOCUS    ].value = cryfocus;
  hardpar.hp[DSP_SPACTRL_FILAMENT    ].value = filament;
  hardpar.hp[DSP_SPACTRL_GUNFOCUS    ].value = gunfocus;
  hardpar.hp[DSP_SPACTRL_GUNANODE    ].value = gunanode;
  hardpar.hp[DSP_SPACTRL_SMPDIST     ].value = smpdist;
  hardpar.hp[DSP_SPACTRL_SMPTEMP     ].value = smptemp;
  hardpar.hp[DSP_SPACTRL_GROWING     ].value = growing;
  SpaLeedControl_pi.app->xsm->hardware->SetParameter(hardpar);
}

int SpaLeedControl::ChangedAction(GtkWidget *widget, SpaLeedControl *spac){
  spac->updateSPALEED();
  return 0;
}

void SpaLeedControl::ChangedNotify(Param_Control* pcs, gpointer spac){
  ((SpaLeedControl*)spac)->updateSPALEED();
}

void SpaLeedControl::ExecCmd(int cmd){
  SpaLeedControl_pi.app->xsm->hardware->ExecCmd(cmd);
}

int SpaLeedControl::epitaxy_callback( GtkWidget *widget, SpaLeedControl *spac){
  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget)))
    spac->growing = 1.;
  else
    spac->growing = 0.;
  return 0;
}
