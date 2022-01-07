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

#include "app_peakfind.h"

#define DSP_CMD_PRB_RUNPF  1515


DSPPeakFindControl::DSPPeakFindControl (XSM_Hardware *Hard, GSList **RemoteEntryList, int InWindow)
{
		GtkWidget *box;

		PI_DEBUG (DBG_L2, "DSPPeakFindControl::DSPPeakFindControl");

		hard=Hard;
		itab=0;
		IdPFtmout=0;
		PFtmoutms=2000;

		PfpList = NULL;

#define VOLT_MAX 10.
#define VOLT_MIN -10.

		Unity       = new UnitObj(" "," ");
		Volt        = new UnitObj("V","V", "g", "Volt");
		TimeUnitms  = new LinUnit("ms","ms", "Time", 1e-3);
		TimeUnit    = new UnitObj("s","s", "g", "Time");
		CPSUnit     = new CPSCNTUnit("Cps","Cps","Intensity");
		EnergyUnit  = new UnitObj("eV","eV","Energy");

		pfscan = new PeakFindScan;

		AppWindowInit("DSP Peakfind/Focus/Oszi Control");

		PI_DEBUG (DBG_L2, "DSPPeakFindControl::DSPPeakFindControl widget setup ");

		box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
		gtk_widget_show (box);
		gtk_box_pack_start (GTK_BOX (vbox), box, TRUE, TRUE, 0);

		notebook = gtk_notebook_new ();
		gtk_widget_show (notebook);
		gtk_box_pack_start(GTK_BOX(box), notebook, TRUE, TRUE, GXSM_WIDGET_PAD);

		addPFfolder();
		addPFcontrol(box);
}

DSPPeakFindControl::~DSPPeakFindControl (){
		PI_DEBUG (DBG_L2, "DSPPeakFindControl::~DSPPeakFindControl" );

		rmPFtmout();

		//  nodestroy=TRUE;
		//  while(gtk_events_pending()) gtk_main_iteration();
		//  destroy();
		//  while(gtk_events_pending()) gtk_main_iteration();

		g_slist_foreach(PfpList, (GFunc) DSPPeakFindControl::delete_pfp_cb, this);
		g_slist_free(PfpList);
  
		delete pfscan;
  
		delete Unity;
		delete Volt;
		delete TimeUnitms;
		delete TimeUnit;
		delete CPSUnit;
		delete EnergyUnit;
  
		PI_DEBUG (DBG_L2, "done." );
}


void DSPPeakFindControl::addPFcontrol(GtkWidget *box){
		GtkWidget *frame_param, *vbox_param, *hbox_param;

		GtkWidget *input;
		GtkWidget *button;

		Gtk_EntryControl *ec;

		PI_DEBUG (DBG_L2, "Adding PF Control Widget" );

		GSList *EC_list=(GSList*)g_object_get_data( G_OBJECT (widget), "Probe_EC_list"); // get list

		frame_param = gtk_frame_new ("Global Peak Finder Control");
		gtk_widget_show (frame_param);
		gtk_container_add (GTK_CONTAINER (box), frame_param);

		vbox_param = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
		gtk_widget_show (vbox_param);
		gtk_container_add (GTK_CONTAINER (frame_param), vbox_param);

		// PF parameters
		input = mygtk_create_input("Timeout [ms]:", vbox_param, hbox_param);
		ec = new Gtk_EntryControl(Unity, MLD_WERT_NICHT_OK, &PFtmoutms, 100., 100000., "6.0f", input);
  
		//  input = mygtk_add_input("Y0", hbox_param);
		//  ec = new Gtk_EntryControl(Volt, MLD_WERT_NICHT_OK, &pfp->y0, -200., 200., "5.3f", input);


		hbox_param = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
		gtk_widget_show (hbox_param);
		gtk_container_add (GTK_CONTAINER (vbox_param), hbox_param);

#define DSP_CMD_PRB_RUNPFI0 1516
#define DSP_CMD_PRB_DELPFI0 1517
#define DSP_CMD_PRB_PFreset 1518
#define DSP_CMD_PRB_PFsaveNc 1519

		button = gtk_button_new_with_label("Run");
		gtk_widget_show (button);
		gtk_container_add (GTK_CONTAINER (hbox_param), button);
		g_object_set_data( G_OBJECT (button), "DSP_cmd", (gpointer)DSP_CMD_PRB_RUNPFI0);
		g_signal_connect ( G_OBJECT (button), "pressed",
							 G_CALLBACK (DSPPeakFindControl::CmdAction),
							 this);

		button = gtk_button_new_with_label("Stop");
		gtk_widget_show (button);
		gtk_container_add (GTK_CONTAINER (hbox_param), button);
		g_object_set_data( G_OBJECT (button), "DSP_cmd", (gpointer)DSP_CMD_PRB_DELPFI0);
		g_signal_connect ( G_OBJECT (button), "pressed",
							 G_CALLBACK (DSPPeakFindControl::CmdAction),
							 this);

		button = gtk_button_new_with_label("Reset");
		gtk_widget_show (button);
		gtk_container_add (GTK_CONTAINER (hbox_param), button);
		g_object_set_data( G_OBJECT (button), "DSP_cmd", (gpointer)DSP_CMD_PRB_PFreset);
		g_signal_connect ( G_OBJECT (button), "pressed",
							 G_CALLBACK (DSPPeakFindControl::CmdAction),
							 this);

		button = gtk_button_new_with_label("SaveNc");
		gtk_widget_show (button);
		gtk_container_add (GTK_CONTAINER (hbox_param), button);
		g_object_set_data( G_OBJECT (button), "DSP_cmd", (gpointer)DSP_CMD_PRB_PFsaveNc);
		g_signal_connect ( G_OBJECT (button), "pressed",
							 G_CALLBACK (DSPPeakFindControl::CmdAction),
							 this);

  
		// save list away...
		g_object_set_data( G_OBJECT (widget), "Probe_EC_list", EC_list);
}

void DSPPeakFindControl::addPFfolder(){

#define LABENTRYSIZES 70,100

		static int bf_num=0;
		GtkWidget *frame_param, *vbox_nb, *vbox_param, *hbox_param, *hbox, *vbox_r, *hbox_r, *hpaned;
		GtkWidget *PeakFindCtrl;

		GtkWidget *input;
		GtkWidget *button;

		Gtk_EntryControl *ec;
		remote_action_cb *ra;

		GSList *EC_list=(GSList*)g_object_get_data( G_OBJECT (widget), "Probe_EC_list"); // get list

		gchar *tmp;

		SPA_PeakFind_p *pfp;

		PI_DEBUG (DBG_L2, "---- Adding PF Folder ----" );

		pfp = new SPA_PeakFind_p(bf_num);
		PfpList = g_slist_prepend( PfpList, pfp);

		// add to notebook folder...
		hpaned = gtk_hpaned_new ();
		gtk_widget_show (hpaned);
		//  gtk_container_add (GTK_CONTAINER (notebook), hpaned);
		//  gtk_paned_set_position (GTK_PANED (hpaned), -1);

		vbox_nb = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
		gtk_widget_show (vbox_nb);
		gtk_paned_pack1 (GTK_PANED (hpaned), vbox_nb, FALSE, TRUE);
  
		// set label
		tmp = g_strdup_printf("PF-%d",++bf_num);
		PeakFindCtrl = gtk_label_new (tmp);
		g_free(tmp);
		gtk_widget_show (PeakFindCtrl);
		gtk_notebook_append_page (GTK_NOTEBOOK (notebook), hpaned, PeakFindCtrl);
		itab++;
// set_notebook_tab (notebook, itab++, PeakFindCtrl);
  
		// make frame, attach to folder
		frame_param = gtk_frame_new ("Peak Finder / Monitor");
		gtk_widget_show (frame_param);
		gtk_container_add (GTK_CONTAINER (vbox_nb), frame_param);
  
		// make hbox, left visible, righr on demand !
		hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
		gtk_widget_show (hbox);
		gtk_container_add (GTK_CONTAINER (frame_param), hbox);

		vbox_param = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
		gtk_widget_show (vbox_param);
		gtk_container_add (GTK_CONTAINER (hbox), vbox_param);

		// make invisible vbox right --------------------
		hbox_r = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
		vbox_r = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
		gtk_widget_set_size_request(hbox_r, -1, -1);
		gtk_widget_set_size_request(vbox_r, 200, -1);

		gtk_box_pack_start (GTK_BOX (hbox_r), vbox_r, TRUE, TRUE, 0);
		gtk_widget_show (vbox_r);

		gtk_paned_pack2 (GTK_PANED (hpaned), hbox_r, FALSE, TRUE);

		// pfp->focview = new Focus<long> (this, pfp, vbox_r, hbox_r);
		pfp->focview = new Focus (this, pfp, vbox_r, hbox_r);

		// -----------------------------------------------

		// Special PF Action Toolbar
		GtkWidget *toolbar = gtk_toolbar_new(); // GTK_ORIENTATION_HORIZONTAL,
//					GTK_TOOLBAR_TEXT ); // ?????****** fixme
		gtk_container_set_border_width ( GTK_CONTAINER ( toolbar ) , 5 );
//  gtk_toolbar_set_space_size ( GTK_TOOLBAR ( toolbar ), 5 ); // ???***** fixme
		gtk_container_add ( GTK_CONTAINER ( vbox_param ) , toolbar );
		gtk_widget_show (toolbar);

		//       gtk_toolbar_append_space ( GTK_TOOLBAR ( toolbar ) );

		button = gtk_toolbar_prepend_item
				(  GTK_TOOLBAR ( toolbar ),
				   "XY0",
				   "copy found XY0 to Xorg",
				   "tooltip_private_text",
				   NULL, //   GtkWidget     *icon,
				   G_CALLBACK (DSPPeakFindControl::PFcpyorg),
				   this );
		gtk_widget_show (button);
		g_object_set_data( G_OBJECT (button), "PF_folder_pfp",  (gpointer)pfp);
		ra = g_new( remote_action_cb, 1);
		ra -> cmd = g_strdup_printf("%s_XY0_%d", DSPPeakFind_pi.name, pfp->index+1);
		ra -> RemoteCb = &DSPPeakFindControl::PFcpyorg;
		ra -> widget = button;
		ra -> data = this;
		main_get_gapp()->RemoteActionList = g_slist_prepend
				( main_get_gapp()->RemoteActionList, ra );
		PI_DEBUG (DBG_L2, "Adding new Remote Cmd: " << ra->cmd );

		// ----------

		button = gtk_toolbar_prepend_item
				(  GTK_TOOLBAR ( toolbar ),
				   "Offset from Main",
				   "copy XY Offset from main to XYorg",
				   "tooltip_private_text",
				   NULL, //   GtkWidget     *icon,
				   G_CALLBACK (DSPPeakFindControl::PFcpyFromM),
				   this );
		gtk_widget_show (button);
		g_object_set_data( G_OBJECT (button), "PF_folder_pfp",  (gpointer)pfp);
		ra = g_new( remote_action_cb, 1);
		ra -> cmd = g_strdup_printf("%s_OffsetFromMain_%d", DSPPeakFind_pi.name, pfp->index+1);
		ra -> RemoteCb = & DSPPeakFindControl::PFcpyFromM;
		ra -> widget = button;
		ra -> data = this;
		main_get_gapp()->RemoteActionList = g_slist_prepend
				( main_get_gapp()->RemoteActionList, ra );
		PI_DEBUG (DBG_L2, "Adding new Remote Cmd: " << ra->cmd );

		// ----------

		button = gtk_toolbar_prepend_item
				(  GTK_TOOLBAR ( toolbar ),
				   "Offset to Main",
				   "copy XY0 Offset to main",
				   "tooltip_private_text",
				   NULL, //   GtkWidget     *icon,
				   G_CALLBACK (DSPPeakFindControl::PFcpyToM),
				   this );
		gtk_widget_show (button);
		g_object_set_data( G_OBJECT (button), "PF_folder_pfp",  (gpointer)pfp);
		ra = g_new( remote_action_cb, 1);
		ra -> cmd = g_strdup_printf("%s_OffsetToMain_%d", DSPPeakFind_pi.name, pfp->index+1);
		ra -> RemoteCb = &DSPPeakFindControl::PFcpyToM;
		ra -> widget = button;
		ra -> data = this;
		main_get_gapp()->RemoteActionList = g_slist_prepend
				( main_get_gapp()->RemoteActionList, ra );
		PI_DEBUG (DBG_L2, "Adding new Remote Cmd: " << ra->cmd );

		button = gtk_toolbar_prepend_item
				(  GTK_TOOLBAR ( toolbar ),
				   "E from Main",
				   "copy Energy from main",
				   "tooltip_private_text",
				   NULL, //   GtkWidget     *icon,
				   G_CALLBACK (DSPPeakFindControl::PFcpyEfromM),
				   this );
		gtk_widget_show (button);
		g_object_set_data( G_OBJECT (button), "PF_folder_pfp",  (gpointer)pfp);
		ra = g_new( remote_action_cb, 1);
		ra -> cmd = g_strdup_printf("%s_EfromMain_%d", DSPPeakFind_pi.name, pfp->index+1);
		ra -> RemoteCb = &DSPPeakFindControl::PFcpyEfromM;
		ra -> widget = button;
		ra -> data = this;
		main_get_gapp()->RemoteActionList = g_slist_prepend
				( main_get_gapp()->RemoteActionList, ra );
		PI_DEBUG (DBG_L2, "Adding new Remote Cmd: " << ra->cmd );


		button = gtk_toolbar_prepend_item
				(  GTK_TOOLBAR ( toolbar ),
				   "Run PF",
				   "run PF once",
				   "tooltip_private_text",
				   NULL, //   GtkWidget     *icon,
				   G_CALLBACK (DSPPeakFindControl::CmdAction),
				   this );
		gtk_widget_show (button);
		g_object_set_data( G_OBJECT (button), "DSP_cmd", (gpointer)DSP_CMD_PRB_RUNPF);
		g_object_set_data( G_OBJECT (button), "DSP_PFP", (gpointer)pfp );
		ra = g_new( remote_action_cb, 1);
		ra -> cmd = g_strdup_printf("%s_RunPF_%d", DSPPeakFind_pi.name, pfp->index+1);
		ra -> RemoteCb = &DSPPeakFindControl::CmdAction;
		ra -> widget = button;
		ra -> data = this;
		main_get_gapp()->RemoteActionList = g_slist_prepend
				( main_get_gapp()->RemoteActionList, ra );
		PI_DEBUG (DBG_L2, "Adding new Remote Cmd: " << ra->cmd );

		button = gtk_check_button_new_with_label ("Follow");
		gtk_widget_show (button);
		gtk_container_add (GTK_CONTAINER ( toolbar ), button);
		g_object_set_data( G_OBJECT (button), "PF_folder_pfp",  (gpointer)pfp);
		g_signal_connect (G_OBJECT (button), "clicked",
							G_CALLBACK (DSPPeakFindControl::PFfollow),
							this);

		// PF parameters
		input = mygtk_create_input("Xorg:", vbox_param, hbox_param, LABENTRYSIZES);
		ec = new Gtk_EntryControl(Volt, MLD_WERT_NICHT_OK, &pfp->xorg, -200., 200., "5.3f", input);
		EC_list = g_slist_prepend( EC_list, ec);
  
		input = mygtk_add_input("Yorg", hbox_param, LABENTRYSIZES);
		ec = new Gtk_EntryControl(Volt, MLD_WERT_NICHT_OK, &pfp->yorg, -200., 200., "5.3f", input);
		EC_list = g_slist_prepend( EC_list, ec);
  

		input = mygtk_add_input("rel.Lim.", hbox_param, LABENTRYSIZES);
		ec = new Gtk_EntryControl(Volt, MLD_WERT_NICHT_OK, &pfp->rellim, -200., 200., "5.3f", input);
		//  EC_list = g_slist_prepend( EC_list, ec);
  
		// ----------------------------------------
  
		input = mygtk_create_input("X0:", vbox_param, hbox_param, LABENTRYSIZES);
		ec = new Gtk_EntryControl(Volt, MLD_WERT_NICHT_OK, &pfp->x0, -200., 200., "5.3f", input);
		EC_list = g_slist_prepend( EC_list, ec);
  
		input = mygtk_add_input("Y0", hbox_param, LABENTRYSIZES);
		ec = new Gtk_EntryControl(Volt, MLD_WERT_NICHT_OK, &pfp->y0, -200., 200., "5.3f", input);
		EC_list = g_slist_prepend( EC_list, ec);
  
		input = mygtk_add_input("abs.Lim.", hbox_param, LABENTRYSIZES);
		ec = new Gtk_EntryControl(Volt, MLD_WERT_NICHT_OK, &pfp->abslim, -200., 200., "5.3f", input);
		//  EC_list = g_slist_prepend( EC_list, ec);
  

		// ----------------------------------------

		input = mygtk_create_input("Points:", vbox_param, hbox_param, LABENTRYSIZES);
		ec = new Gtk_EntryControl(Unity, MLD_WERT_NICHT_OK, &pfp->npkte, 3., 500., "3.0f", input);
		//  EC_list = g_slist_prepend( EC_list, ec);
		//  ec->Set_ChangeNoticeFkt(DSPPeakFindControl::ChangedNotify, this);
		//  EC_list = g_slist_prepend( EC_list, ec);
		//  *RemoteEntryList = PrbLower->AddEntry2RemoteList("SPA_PF_POINTS", *RemoteEntryList);
  
		input = mygtk_add_input("#P2d", hbox_param, LABENTRYSIZES);
		ec = new Gtk_EntryControl(Unity, MLD_WERT_NICHT_OK, &pfp->npkte2d, 3, 41., "5.3f", input);
		//  EC_list = g_slist_prepend( EC_list, ec);

		input = mygtk_add_input("Radius", hbox_param, LABENTRYSIZES);
		ec = new Gtk_EntryControl(Volt, MLD_WERT_NICHT_OK, &pfp->radius, 0.01, 100., "5.3f", input);
		//  EC_list = g_slist_prepend( EC_list, ec);

		// ----------------------------------------
  
		input = mygtk_create_input("Gatetime:", vbox_param, hbox_param, LABENTRYSIZES);
		ec = new Gtk_EntryControl(TimeUnitms, MLD_WERT_NICHT_OK, &pfp->gate, 1e-3, 100., "3.0f", input);
		//  EC_list = g_slist_prepend( EC_list, ec);
  
		input = mygtk_add_input("Gate2d:", hbox_param, LABENTRYSIZES);
		ec = new Gtk_EntryControl(TimeUnitms, MLD_WERT_NICHT_OK, &pfp->gate2d, 0.1e-3, 60., "4.1f", input);
		//  EC_list = g_slist_prepend( EC_list, ec);
  
		input = mygtk_add_input("Energy", hbox_param, LABENTRYSIZES);
		ec = new Gtk_EntryControl(EnergyUnit, MLD_WERT_NICHT_OK, &pfp->energy, 0., 400., "5.3f", input);
		EC_list = g_slist_prepend( EC_list, ec);

		// ----------------------------------------
  
		input = mygtk_create_input("Loops:", vbox_param, hbox_param, LABENTRYSIZES);
		ec = new Gtk_EntryControl(Unity, MLD_WERT_NICHT_OK, &pfp->maxloops, 0., 100., "3.0f", input);
		//  EC_list = g_slist_prepend( EC_list, ec);

		input = mygtk_add_input("Delta", hbox_param, LABENTRYSIZES);
		ec = new Gtk_EntryControl(Volt, MLD_WERT_NICHT_OK, &pfp->konvfac, 0., 100., "5.3f", input);
		//  EC_list = g_slist_prepend( EC_list, ec);
  
		input = mygtk_add_input("OffsetCorr.", hbox_param, LABENTRYSIZES);
		ec = new Gtk_EntryControl(Unity, MLD_WERT_NICHT_OK, &pfp->offsetcorrection, 0., 1000., "5.1f", input);
		//  EC_list = g_slist_prepend( EC_list, ec);
  
		// ----------------------------------------

		hbox_param = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
		gtk_widget_show (hbox_param);
		gtk_container_add (GTK_CONTAINER (vbox_param), hbox_param);

		GtkWidget *radiobutton;
		GSList    *radiogroup;
		radiobutton = gtk_radio_button_new_with_label (NULL, "Fit Poly 2nd");
		gtk_box_pack_start (GTK_BOX (hbox_param), radiobutton, TRUE, TRUE, 0);
		gtk_widget_show (radiobutton);
		g_object_set_data( G_OBJECT (radiobutton), "PF_MODE", (gpointer)PF_FITMODE_FIT2ND);
		g_signal_connect (G_OBJECT (radiobutton), "clicked",
							G_CALLBACK (PFsetmode), pfp);

		radiogroup = gtk_radio_button_group (GTK_RADIO_BUTTON (radiobutton));

		radiobutton = gtk_radio_button_new_with_label( radiogroup, "Gaus");
		gtk_box_pack_start (GTK_BOX (hbox_param), radiobutton, TRUE, TRUE, 0);
		gtk_widget_show (radiobutton);
		g_object_set_data( G_OBJECT (radiobutton), "PF_MODE", (gpointer)PF_FITMODE_FITGAUS);
		g_signal_connect (G_OBJECT (radiobutton), "clicked",
							G_CALLBACK (PFsetmode), pfp);

		radiogroup = gtk_radio_button_group (GTK_RADIO_BUTTON (radiobutton));

		radiobutton = gtk_radio_button_new_with_label( radiogroup, "Lorenz");
		gtk_box_pack_start (GTK_BOX (hbox_param), radiobutton, TRUE, TRUE, 0);
		gtk_widget_show (radiobutton);
		g_object_set_data( G_OBJECT (radiobutton), "PF_MODE", (gpointer)PF_FITMODE_FITLORENZ);
		g_signal_connect (G_OBJECT (radiobutton), "clicked",
							G_CALLBACK (PFsetmode), pfp);

		radiogroup = gtk_radio_button_group (GTK_RADIO_BUTTON (radiobutton));

		radiobutton = gtk_radio_button_new_with_label( radiogroup, "Disable");
		gtk_box_pack_start (GTK_BOX (hbox_param), radiobutton, TRUE, TRUE, 0);
		gtk_widget_show (radiobutton);
		g_object_set_data( G_OBJECT (radiobutton), "PF_MODE", NULL);
		g_signal_connect (G_OBJECT (radiobutton), "clicked",
							G_CALLBACK (PFsetmode), pfp);


		GtkWidget *Set = gtk_check_button_new_with_label ("-X");
		gtk_widget_show (Set);
		gtk_box_pack_start (GTK_BOX (hbox_param), Set, FALSE, FALSE, 0);
		g_signal_connect (G_OBJECT (Set), "clicked",
							G_CALLBACK (on_mX_clicked),
							pfp);
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (Set), pfp->xsig < 0.);

		Set = gtk_check_button_new_with_label ("-Y");
		gtk_widget_show (Set);
		gtk_box_pack_start (GTK_BOX (hbox_param), Set, FALSE, FALSE, 0);
		g_signal_connect (G_OBJECT (Set), "clicked",
							G_CALLBACK (on_mY_clicked),
							pfp);
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (Set), pfp->ysig < 0.);

		input = mygtk_create_input("X- fwhm:", vbox_param, hbox_param,LABENTRYSIZES);
		ec = new Gtk_EntryControl(Volt, MLD_WERT_NICHT_OK, &pfp->xfwhm, 0., 100., ".3f", input);
		EC_list = g_slist_prepend( EC_list, ec);
		input = mygtk_add_input("I0", hbox_param,LABENTRYSIZES);
		ec = new Gtk_EntryControl(CPSUnit, MLD_WERT_NICHT_OK, &pfp->xI0, 0., 100., ".0f", input);
		EC_list = g_slist_prepend( EC_list, ec);
		input = mygtk_add_input("x0", hbox_param,LABENTRYSIZES);
		ec = new Gtk_EntryControl(Volt, MLD_WERT_NICHT_OK, &pfp->xxf0, 0., 100., ".3f", input);
		EC_list = g_slist_prepend( EC_list, ec);
  
		input = mygtk_create_input("Y- fwhm:", vbox_param, hbox_param,LABENTRYSIZES);
		ec = new Gtk_EntryControl(Volt, MLD_WERT_NICHT_OK, &pfp->yfwhm, 0., 100., ".3f", input);
		EC_list = g_slist_prepend( EC_list, ec);
		input = mygtk_add_input("I0", hbox_param,LABENTRYSIZES);
		ec = new Gtk_EntryControl(CPSUnit, MLD_WERT_NICHT_OK, &pfp->yI0, 0., 100., ".0f", input);
		EC_list = g_slist_prepend( EC_list, ec);
		input = mygtk_add_input("x0", hbox_param,LABENTRYSIZES);
		ec = new Gtk_EntryControl(Volt, MLD_WERT_NICHT_OK, &pfp->yxf0, 0., 100., ".3f", input);
		EC_list = g_slist_prepend( EC_list, ec);
  
  
		// graph color picker
  
		// Checkbutton: PF on
  
		// Checkbutton: to enable
  
		// Buttons: Start/Stop
		// ----------
		hbox_param = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
		gtk_widget_show (hbox_param);
		gtk_container_add (GTK_CONTAINER (vbox_param), hbox_param);

		button = gtk_button_new_with_label("Run Peak Finder");
		gtk_widget_show (button);
		gtk_container_add (GTK_CONTAINER (hbox_param), button);
		g_object_set_data( G_OBJECT (button), "DSP_cmd", (gpointer)DSP_CMD_PRB_RUNPF);
		g_object_set_data( G_OBJECT (button), "DSP_PFP", (gpointer)pfp );
		g_signal_connect ( G_OBJECT (button), "pressed",
							 G_CALLBACK (DSPPeakFindControl::CmdAction),
							 this);

		button = gtk_button_new_with_label("add PF");
		gtk_widget_show (button);
		gtk_container_add (GTK_CONTAINER (hbox_param), button);
		g_signal_connect ( G_OBJECT (button), "pressed",
							 G_CALLBACK (DSPPeakFindControl::PFadd),
							 this);
  

		if(bf_num > 1){ // do not allow to kill the first one...
				button = gtk_button_new_with_label("del PF");
				gtk_widget_show (button);
				gtk_container_add (GTK_CONTAINER (hbox_param), button);
				g_object_set_data( G_OBJECT (button), "PF_folder_pfp",  (gpointer)pfp);
				g_object_set_data( G_OBJECT (button), "PF_folder_itab", (gpointer)(itab-1));
				g_signal_connect ( G_OBJECT (button), "pressed",
									 G_CALLBACK (DSPPeakFindControl::PFremove),
									 this);
		}
  
		button = gtk_button_new_with_label("Peak View >>");
		gtk_widget_show (button);
		gtk_container_add (GTK_CONTAINER (hbox_param), button);
		g_object_set_data( G_OBJECT (button), "PF_BOX", (gpointer)hbox_r);
		g_object_set_data( G_OBJECT (button), "PF_BOX_FLAG", NULL);
		g_signal_connect ( G_OBJECT (button), "pressed",
							 G_CALLBACK (DSPPeakFindControl::PFExpandView),
							 this);
  
		// save list away...
		g_object_set_data( G_OBJECT (widget), "Probe_EC_list", EC_list);
}

void DSPPeakFindControl::update(){
		g_slist_foreach (
				(GSList*) g_object_get_data (
						G_OBJECT (widget), "Probe_EC_list"),
				(GFunc) App::update_ec, NULL);
}

void DSPPeakFindControl::CmdAction(GtkWidget *widget, void *vpc){
		DSPPeakFindControl *pc = (DSPPeakFindControl*)vpc;
		switch((int)g_object_get_data( G_OBJECT (widget), "DSP_cmd")){
		case DSP_CMD_PRB_RUNPF:
				PI_DEBUG (DBG_L2, "PF-Test" ); 
				((DSPPeakFindControl*)pc)->pfscan->PFrun
						(pc->hard, 
						 (SPA_PeakFind_p *)
						 (g_object_get_data( G_OBJECT (widget), "DSP_PFP")) 
								);
				break;
		case DSP_CMD_PRB_RUNPFI0:
				PI_DEBUG (DBG_L2, "add PFtmout");
				pc->addPFtmout();
				break;
		case DSP_CMD_PRB_DELPFI0:
				PI_DEBUG (DBG_L2, "rm PFtmout");
				pc->rmPFtmout();
				break;
		case DSP_CMD_PRB_PFreset:
				g_slist_foreach(pc->PfpList, (GFunc) DSPPeakFindControl::PFreset, pc);
				break;
		case DSP_CMD_PRB_PFsaveNc:
				gchar *fname = main_get_gapp()->file_dialog("Save Probe Scan", NULL, NULL, "Probescan.nc", "probesave");
				pc->pfscan->Save(fname);
				break;
		}
		pc->update();
}

void DSPPeakFindControl::on_mX_clicked(GtkWidget *button, SPA_PeakFind_p *pfp){
		pfp->xsig = GTK_TOGGLE_BUTTON (button)->active ? -1.:1.;
}
void DSPPeakFindControl::on_mY_clicked(GtkWidget *button, SPA_PeakFind_p *pfp){
		pfp->ysig = GTK_TOGGLE_BUTTON (button)->active ? -1.:1.;
}

void DSPPeakFindControl::PFsetmode(GtkWidget *widget, SPA_PeakFind_p *pfp){
		pfp->SetFitMode((int)g_object_get_data( G_OBJECT (widget), "PF_MODE"));
}

void DSPPeakFindControl::PFadd(GtkWidget *widget, gpointer pc){
		((DSPPeakFindControl*)pc)->addPFfolder();
}

void DSPPeakFindControl::PFExpandView(GtkWidget *button, gpointer pc){
		if(g_object_get_data( G_OBJECT (button), "PF_BOX_FLAG")){
				gtk_widget_hide( (GtkWidget*) g_object_get_data( G_OBJECT (button), "PF_BOX") );
				g_object_set_data( G_OBJECT (button), "PF_BOX_FLAG", NULL);
				//	gtk_label_set_text( GTK_LABEL ( GTK_BUTTON (button)), "View >>" );
				//	PI_DEBUG (DBG_L2, ">>>>" );
		}else{
				gtk_widget_show( (GtkWidget*) g_object_get_data( G_OBJECT (button), "PF_BOX") );
				g_object_set_data( G_OBJECT (button), "PF_BOX_FLAG", (gpointer)1);
				//	gtk_label_set_text( GTK_LABEL ( GTK_BUTTON (button)), "View <<" );
				//	PI_DEBUG (DBG_L2, "<<<<<" );
		}
}

void DSPPeakFindControl::PFremove(GtkWidget *widget, gpointer pc){
		return; // disable remove 
#if 0
		// should remove EC_list entrys too... ???!!!
		SPA_PeakFind_p *pfp = (SPA_PeakFind_p *) g_object_get_data( G_OBJECT (widget), "PF_folder_pfp");
		gtk_notebook_remove_page (GTK_NOTEBOOK (((DSPPeakFindControl*)pc)->notebook), (int)g_object_get_data( G_OBJECT (widget), "PF_folder_itab") );
		delete pfp;
		((DSPPeakFindControl*)pc)->PfpList = g_slist_remove( pc->PfpList, pfp );
#endif
}

void DSPPeakFindControl::PFcpyorg(GtkWidget *widget, gpointer pc){
		SPA_PeakFind_p *pfp =  (SPA_PeakFind_p*)g_object_get_data( G_OBJECT (widget), "PF_folder_pfp");
		pfp->xorg=pfp->x0;
		pfp->yorg=pfp->y0;
		PI_DEBUG (DBG_L2, "----------- PFcpyorg !!! -------------" );
		((DSPPeakFindControl*)pc)->update();
}

void DSPPeakFindControl::PFcpyFromM(GtkWidget *widget, gpointer pc){
		SPA_PeakFind_p *pfp =  (SPA_PeakFind_p*)g_object_get_data( G_OBJECT (widget), "PF_folder_pfp");
		pfp->xorg=pfp->x0 = main_get_gapp()->xsm->data.s.x0;
		pfp->yorg=pfp->y0 = main_get_gapp()->xsm->data.s.y0;
		((DSPPeakFindControl*)pc)->update();
}

void DSPPeakFindControl::PFcpyEfromM(GtkWidget *widget, gpointer pc){
		SPA_PeakFind_p *pfp =  (SPA_PeakFind_p*)g_object_get_data( G_OBJECT (widget), "PF_folder_pfp");
		pfp->energy = main_get_gapp()->xsm->data.hardpars.SPA_Energy;
		((DSPPeakFindControl*)pc)->update();
}

void DSPPeakFindControl::PFcpyToM(GtkWidget *widget, gpointer pc){
		SPA_PeakFind_p *pfp =  (SPA_PeakFind_p*)g_object_get_data( G_OBJECT (widget), "PF_folder_pfp");
		main_get_gapp()->xsm->data.s.x0 = pfp->xorg;
		main_get_gapp()->xsm->data.s.y0 = pfp->yorg;
		main_get_gapp()->spm_update_all();
}

void DSPPeakFindControl::PFfollow(GtkButton *button, gpointer pc){
		SPA_PeakFind_p *pfp =  (SPA_PeakFind_p*)g_object_get_data( G_OBJECT (button), "PF_folder_pfp");
		if (GTK_TOGGLE_BUTTON (button)->active)
				pfp->follow=TRUE;
		else
				pfp->follow=FALSE;
}

void DSPPeakFindControl::PFenable(GtkWidget *widget, gpointer pc){
}

void DSPPeakFindControl::PFreset(SPA_PeakFind_p *pfp, gpointer pc){
		pfp->count=0;
}

void DSPPeakFindControl::PFrunI0pfp(SPA_PeakFind_p *pfp, gpointer pc){
		PI_DEBUG (DBG_L2, "PFrunI0 from list:" << pfp->index);
		PI_DEBUG (DBG_L2, "PFrunI0 from list:" << pfp->index );
		((DSPPeakFindControl*)pc)->pfscan->PFrunI0(((DSPPeakFindControl*)pc)->hard, pfp);
		((DSPPeakFindControl*)pc)->update();
}

gint DSPPeakFindControl::PFtmoutfkt(DSPPeakFindControl *pc){
		static int active=FALSE;
		PI_DEBUG (DBG_L2, "DSPPeakFindControl::PFtmoutfkt");
		PI_DEBUG (DBG_L2, "DSPPeakFindControl::PFtmoutfkt ----------------" );
		if(active){
				PI_DEBUG (DBG_L2, "PFtmout called too fast, slowing down...");
				pc->PFtmoutms+=200;
				pc->update();
				pc->rmPFtmout();
				pc->addPFtmout();
				return FALSE;
		}
		active=TRUE;
		PI_DEBUG (DBG_L2, "PF: running all BF...");
		g_slist_foreach(pc->PfpList, (GFunc) DSPPeakFindControl::PFrunI0pfp, pc);
		PI_DEBUG (DBG_L2, "PF: run done.");
		active=FALSE;
		return TRUE;
}

void DSPPeakFindControl::ChangedNotify(Param_Control* pcs, gpointer pc){
		//  gchar *us = pcs->Get_UsrString();
		//  PI_DEBUG (DBG_L2, "PRBC: " << us );
		//  g_free(us);

		//  ((DSPPeakFindControl*)pc)->updateDSP();
}

void DSPPeakFindControl::ExecCmd(int cmd){
		main_get_gapp()->xsm->hardware->ExecCmd(cmd);
}

