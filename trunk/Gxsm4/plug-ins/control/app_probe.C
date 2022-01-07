/* Gnome gxsm - Gnome X Scanning Microscopy
 * universal STM/AFM/SARLS/SPALEED/... controlling and
 * data analysis software
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

/* ignore tag evaluated by for docuscangxsmplugins.pl -- this is no main plugin file
% PlugInModuleIgnore
*/

#include "app_probe.h"

// Remote Disabled... PZ
DSPProbeControl::DSPProbeControl (XSM_Hardware *Hard, GSList **RemoteEntryListXX, int InWindow)
{
  int i;
  //  GSList **RemoteEntryList = new GSList *;
  GSList *EC_list=NULL;

  GtkWidget *box;
  GtkWidget *frame_param, *vbox_nb, *vbox_param, *hbox_param;

  GtkWidget *input;
  GtkWidget *button;

  Gtk_EntryControl *ec;

  PI_DEBUG (DBG_L2, "DSPProbeControl::DSPProbeControl");

  PrbList = NULL;
  prbscan = new ProbeScan;

  hard=Hard;
  itab=0;

#define VOLT_MAX 10.
#define VOLT_MIN -10.

  Unity       = new UnitObj(" "," ");
  Deg         = new UnitObj("°","°");

  UGapAdj     = new UnitObj("A/V","A/V");
  Current     = new UnitObj("nA","nA");
  Force       = new UnitObj("nN","nN");
  TimeUnitms  = new LinUnit("ms","ms", "Time", 1e-3);
  TimeUnit    = new UnitObj("s","s", "g", "Time");
  FrqUnit     = new UnitObj("Hz","Hz", "g", "Frq.");

  AppWindowInit(PROBE_CONTROL_TITLE);

  PI_DEBUG (DBG_L2, "DSPProbeControl::DSPProbeControl widget setup ");

  box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  gtk_widget_show (box);
  gtk_box_pack_start (GTK_BOX (vbox), box, TRUE, TRUE, 0);

  GtkWidget *ProbeCtrl;

  notebook = gtk_notebook_new ();
  gtk_widget_show (notebook);
  gtk_box_pack_start(GTK_BOX(box), notebook, TRUE, TRUE, GXSM_WIDGET_PAD);

  for(itab=i=0; i < PRBMODESMAX && xsmres.prbname[i][0] != '-'; ++i){

    SPM_Probe_p *prbp;
    gchar *title = g_strconcat (xsmres.prbname[i], " [", xsmres.prbparam[i], "]", NULL);
    prbp = new SPM_Probe_p(xsmres.prbid[i], title);
    g_free (title);
    PrbList = g_slist_prepend( PrbList, prbp);
    UnitObj *ux, *uy;
    
    if (strcmp (xsmres.prbXunit[i], "1") == 0)
	    ux = new LinUnit(" "," ", xsmres.prbXlabel[i], (double)xsmres.prbXfactor[i], (double)xsmres.prbXoffset[i]);
    else
	    ux = new LinUnit(xsmres.prbXunit[i], xsmres.prbXunit[i], xsmres.prbXlabel[i], (double)xsmres.prbXfactor[i], (double)xsmres.prbXoffset[i]);
    ux->SetAlias (xsmres.prbXunit[i]);

// Y:	1e-2/1e10*(10^(\$2/3.33))
//	1e-2/1e10*(pow(10., (double)linebuffer[k]/3.33));


    if (strcmp (xsmres.prbYunit[i], "1") == 0)
	    uy = new LinUnit(" ", " ", xsmres.prbYlabel[i], (double)xsmres.prbYfactor[i], (double)xsmres.prbYoffset[i]);
    else
	    if (strncmp (xsmres.prbYunit[i], "log", 3) == 0)
		    uy = new LogUnit(" ", " ", xsmres.prbYlabel[i], (double)xsmres.prbYfactor[i], (double)xsmres.prbYoffset[i]);
	    else
		    uy = new LinUnit(xsmres.prbYunit[i], xsmres.prbYunit[i], xsmres.prbYlabel[i], (double)xsmres.prbYfactor[i], (double)xsmres.prbYoffset[i]);
    uy->SetAlias (xsmres.prbYunit[i]);

    prbp->SetUnits (ux, uy);
    
    prbp->srcs  = 0;
    prbp->nsrcs = 0;
    for(int k=4; k<(int)strlen(xsmres.prbsrcs[i]); ++k)
      if(xsmres.prbsrcs[i][k]=='1'){
	prbp->srcs |= 1<<k;
	prbp->nsrcs++;
      }	
    
    prbp->outp = xsmres.prboutp[i];
    
    // add folder
    vbox_nb = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_show (vbox_nb);
//    gtk_container_add (GTK_CONTAINER (notebook), vbox_nb);
    
    // set label
    ProbeCtrl = gtk_label_new (xsmres.prbname[i]);
    gtk_widget_show (ProbeCtrl);
    gtk_notebook_append_page (GTK_NOTEBOOK (notebook), vbox_nb, ProbeCtrl);
    itab++;
// set_notebook_tab (notebook, itab++, ProbeCtrl);
    
    // make frame, attach to folder
    frame_param = gtk_frame_new (xsmres.prbparam[i]);
    gtk_widget_show (frame_param);
    gtk_container_add (GTK_CONTAINER (vbox_nb), frame_param);
    
    vbox_param = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_show (vbox_param);
    gtk_container_add (GTK_CONTAINER (frame_param), vbox_param);
    
    // make input fields
    
    // ----------
    
    input = mygtk_create_input("Range: Start", vbox_param, hbox_param);
    ec = new Gtk_EntryControl(prbp->XUnit, MLD_WERT_NICHT_OK, &prbp->xS, VOLT_MIN, VOLT_MAX, "5.3f", input);
    
    input = mygtk_add_input("End", hbox_param);
    ec = new Gtk_EntryControl(prbp->XUnit, MLD_WERT_NICHT_OK, &prbp->xE, VOLT_MIN, VOLT_MAX, "5.3f", input);
    
    input = mygtk_add_input("GapAdj", hbox_param);
    ec = new Gtk_EntryControl(UGapAdj, MLD_WERT_NICHT_OK, &prbp->GapAdj, 0., 10., "5.3f", input);
	EC_list = g_slist_prepend( EC_list, ec);
    
    // ----------
    input = mygtk_create_input("AC: Amp", vbox_param, hbox_param);
    ec = new Gtk_EntryControl(prbp->XUnit, MLD_WERT_NICHT_OK, &prbp->ACAmp, VOLT_MIN, VOLT_MAX, "5.3f", input);
	EC_list = g_slist_prepend( EC_list, ec);
    
    input = mygtk_add_input("Frq", hbox_param);
    ec = new Gtk_EntryControl(FrqUnit, MLD_WERT_NICHT_OK, &prbp->ACFrq, 0., 1100., "4.0f", input);
	EC_list = g_slist_prepend( EC_list, ec);

    input = mygtk_add_input("Phase", hbox_param);
    ec = new Gtk_EntryControl(Deg, MLD_WERT_NICHT_OK, &prbp->ACPhase, 0., 360., "3.1f", input);
	EC_list = g_slist_prepend( EC_list, ec);
    // ----------
    
    input = mygtk_create_input("Points", vbox_param, hbox_param);
    ec = new Gtk_EntryControl(Unity, MLD_WERT_NICHT_OK, &prbp->nx, 2., 3400., "4.0f", input);
	EC_list = g_slist_prepend( EC_list, ec);
    
    input = mygtk_add_input("Delay", hbox_param);
    ec = new Gtk_EntryControl(Unity, MLD_WERT_NICHT_OK, &prbp->delay, 0., 1000., "5.3f", input);
	EC_list = g_slist_prepend( EC_list, ec);
    
    input = mygtk_add_input("#Ch", hbox_param);
    ec = new Gtk_EntryControl(Unity, MLD_WERT_NICHT_OK, &prbp->channels, 0., 3., "1.0f", input);
	EC_list = g_slist_prepend( EC_list, ec);
    
    // ----------
    
    input = mygtk_create_input("#Ave", vbox_param, hbox_param);
    ec = new Gtk_EntryControl(Unity, MLD_WERT_NICHT_OK, &prbp->nAve, 0., 1000., "4.0f", input);
	EC_list = g_slist_prepend( EC_list, ec);
    
    input = mygtk_add_input("ACMult", hbox_param);
    ec = new Gtk_EntryControl(Unity, MLD_WERT_NICHT_OK, &prbp->ACMultiplier, 1., 100., "1.0f", input);
	EC_list = g_slist_prepend( EC_list, ec);
    
    input = mygtk_add_input("CI", hbox_param);
    ec = new Gtk_EntryControl(Unity, MLD_WERT_NICHT_OK, &prbp->CIval, -1., 1., ".5f", input);
	EC_list = g_slist_prepend( EC_list, ec);
    
    // Buttons: Start/Stop
    // ----------
    hbox_param = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_show (hbox_param);
    gtk_container_add (GTK_CONTAINER (vbox_param), hbox_param);
    
    button = gtk_button_new_with_label("Start");
    gtk_widget_show (button);
    gtk_box_pack_start (GTK_BOX (hbox_param), button, TRUE, TRUE, 0);
    
    g_object_set_data( G_OBJECT (button), "DSP_cmd", (gpointer)"StartProbe");
    g_object_set_data( G_OBJECT (button), "DSP_PROBE", (gpointer)prbp );
    g_signal_connect ( G_OBJECT (button), "pressed",
			 G_CALLBACK (DSPProbeControl::CmdStartAction),
			 this);
    
    button = gtk_button_new_with_label("Stop");
    gtk_widget_show (button);
    gtk_container_add (GTK_CONTAINER (hbox_param), button);
    g_object_set_data( G_OBJECT (button), "DSP_cmd", (gpointer)"StopProbe");
    g_signal_connect (G_OBJECT (button), "pressed",
			G_CALLBACK (DSPProbeControl::CmdStopAction),
			this);
  }

  // save list away...
  g_object_set_data( G_OBJECT (widget), "Probe_EC_list", EC_list);
}

DSPProbeControl::~DSPProbeControl (){
  PI_DEBUG (DBG_L2, "DSPProbeControl::~DSPProbeControl" );
  g_slist_foreach(PrbList, (GFunc) DSPProbeControl::delete_prb_cb, this);
  g_slist_free(PrbList);

  delete Unity;
  delete Deg;
  delete Current;
  delete Force;
  delete TimeUnitms;
  delete TimeUnit;
  delete FrqUnit;

  delete prbscan;
  PI_DEBUG (DBG_L2, "DSPProbeControl::~DSPProbeControl: done." );
}

void DSPProbeControl::delete_prb_cb(SPM_Probe_p *prb, DSPProbeControl *pc) { delete prb; }

void DSPProbeControl::update(){
  g_slist_foreach
    ((GSList*)g_object_get_data( G_OBJECT (widget), "Probe_EC_list"),
     (GFunc) App::update_ec, NULL);
}

void DSPProbeControl::CmdStartAction(GtkWidget *widget, DSPProbeControl *pc){
  PI_DEBUG (DBG_L2, (char*) g_object_get_data( G_OBJECT (widget), "DSP_cmd") );
  pc->prbscan->Probe
    (pc->hard, 
     (SPM_Probe_p *)(g_object_get_data
		     ( G_OBJECT (widget), "DSP_PROBE"))
     );
  pc->update();
}

void DSPProbeControl::CmdStopAction(GtkWidget *widget, DSPProbeControl *pc){
  PI_DEBUG (DBG_L2, (char*) g_object_get_data( G_OBJECT (widget), "DSP_cmd") );
  pc->prbscan->Stop();
  pc->update();
}

void DSPProbeControl::ChangedNotify(Param_Control* pcs, gpointer pc){
  PI_DEBUG (DBG_L2, "PRBC: " << pcs->Get_UsrString() );
  //  ((DSPProbeControl*)pc)->updateDSP();
}

void DSPProbeControl::ExecCmd(int cmd){
  main_get_gapp()->xsm->hardware->ExecCmd(cmd);
}


