/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 8 c-style: "K&R" -*- */

/* Gxsm - Gnome X Scanning Microscopy
 * universal STM/AFM/SARLS/SPALEED/... controlling and
 * data analysis software
 * 
 * Copyright (C) 1999,2000,2001,2002,2003 Percy Zahl
 *
 * Authors: Percy Zahl <zahl@users.sf.net>
 * additional features: Andreas Klust <klust@users.sf.net>
 * WWW Home: http://gxsm.sf.net
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

/* ignore this module for docuscan
% PlugInModuleIgnore
*/

//#define PACSCOPE_GTKMM_EXPERIMENTAL
#ifdef PACSCOPE_GTKMM_EXPERIMENTAL

#include <plotmm/plot.h>
#include <plotmm/scalediv.h>
#include <plotmm/curve.h>
#include <plotmm/errorcurve.h>
#include <plotmm/symbol.h>
#include <plotmm/paint.h>
#include <plotmm/rectangle.h>

#include <gtkmm/checkbutton.h>
#include <gtkmm/button.h>
#include <gtkmm/box.h>
#include <gtkmm/statusbar.h>
#include <gtkmm/label.h>
#include <gtkmm/adjustment.h>
#include <gtkmm/scale.h>
#include <gtkmm/menuitem.h>
#include <gtkmm/menu.h>
#include <gtkmm/comboboxtext.h>
#include <gtkmm/liststore.h>
#include <glibmm/ustring.h>


#include <gtkmm/window.h>
#include <gtkmm/main.h>
#endif


#include <iostream>


#include <locale.h>
#include <libintl.h>

#include <time.h>

#include "glbvars.h"
#include "modules/dsp.h"
#include <fcntl.h>
#include <sys/ioctl.h>

#include "sranger_mk2_hwi_control.h"
#include "sranger_mk23common_hwi.h"
#include "modules/sranger_mk2_ioctl.h"
#include "../common/pyremote.h"

#define UTF8_DEGREE    "\302\260"
#define UTF8_MU        "\302\265"
#define UTF8_ANGSTROEM "\303\205"

#define MD_PLL 0x1000 // match MK3 FB_spm_dataexchange.h

extern GxsmPlugin sranger_mk2_hwi_pi;
extern DSPControl *DSPControlClass;
extern sranger_common_hwi_dev *sranger_common_hwi; // instance of the HwI derived XSM_Hardware class
extern sranger_common_hwi_dev *sranger_common_hwi;

class PAC_GUI_Builder : public BuildParam{
public:
        PAC_GUI_Builder (GtkWidget *build_grid=NULL, GSList *ec_list_start=NULL, GSList *ec_remote_list=NULL) :
                BuildParam (build_grid, ec_list_start, ec_remote_list) {
                wid = NULL;
                config_checkbutton_list = NULL;
        };

        void start_notebook_tab (GtkWidget *notebook, const gchar *name, const gchar *settings_name,
                                 GSettings *settings) {
                new_grid (); tg=grid;
                grid_add_check_button("Configuration: Enable This");
                g_object_set_data (G_OBJECT (button), "TabGrid", grid);
                config_checkbutton_list = g_slist_append (config_checkbutton_list, button);
                configure_list = g_slist_prepend (configure_list, button);

                new_line ();
                
		MoverCtrl = gtk_label_new (name);
		gtk_widget_show (MoverCtrl);
		gtk_notebook_append_page (GTK_NOTEBOOK (notebook), grid, MoverCtrl);

                g_settings_bind (settings, settings_name,
                                 G_OBJECT (button), "active",
                                 G_SETTINGS_BIND_DEFAULT);
        };

        void notebook_tab_show_all () {
                gtk_widget_show (tg);
        };

        GSList *get_config_checkbutton_list_head () { return config_checkbutton_list; };
        
        GtkWidget *wid;
        GtkWidget *tg;
        GtkWidget *MoverCtrl;
        GSList *config_checkbutton_list;
};

DSPPACControl::DSPPACControl ()
{
	gint coldstart = FALSE;
	XsmRescourceManager xrm("sranger_mk2_hwi_control");
	PI_DEBUG_GP (DBG_L3, "DSPPACControl::DSPPACControl -- constructor\n");
	refresh_timer_id = 0;
	pll.Tau_PAC = 0.;

	pll.Signal1   = g_new (double, PAC_BLCKLEN); 
	pll.Signal1x  = g_new (double, PAC_BLCKLEN); 
	pll.Signal2   = g_new (double, PAC_BLCKLEN); 
	pll.Signal2x  = g_new (double, PAC_BLCKLEN); 
	pll.tmp_array = g_new (gint32, 2*PAC_BLCKLEN); 
	pll.x_array   = g_new (double, PAC_BLCKLEN); 
	pll.x_array_x  = g_new (double, PAC_BLCKLEN); 

// update from DSP
	PI_DEBUG_GP (DBG_L3, "DSPPACControl::DSPPACControl -- read PAC status\n");
	sranger_common_hwi->read_pll (pll, PLL_TAUPAC);
	sranger_common_hwi->read_pll (pll, PLL_CONTROLLER_PHASE);
	sranger_common_hwi->read_pll (pll, PLL_CONTROLLER_AMPLITUDE);
	sranger_common_hwi->read_pll (pll, PLL_OPERATION);

// from apps/gxsm4 GCONF
	PI_DEBUG_GP (DBG_L3, "DSPPACControl::DSPPACControl -- read selected from GCONF\n");
	if (pll.Tau_PAC == 0.){
		PI_DEBUG_GP (DBG_L3, "DSPPACControl::DSPPACControl -- PAC cold start detected or PAC-Tau just set zero.\n");
		coldstart = TRUE; // assume coldstart of PAC lib if Tau from DSP == 0
		xrm.Get("PAC_Tau_PAC", &pll.Tau_PAC, "15e-6");
		xrm.Get("PAC_FSineHz", &pll.FSineHz, "32766.0");
	}
	if (pll.Tau_PAC < 0. || pll.Tau_PAC > 63.)
		pll.Tau_PAC = 15e-6;
	if (pll.FSineHz < 0. || pll.FSineHz > 75000.)
		pll.FSineHz = 32766.0;
	
	PI_DEBUG_GP (DBG_L3, "DSPPACControl::DSPPACControl -- Sine\n");
	pll.deltaReT = 0.;
	pll.deltaImT = 0.;

	pll.PI_Phase_Out = 0.;
	xrm.Get("PAC_volumeSine", &pll.volumeSine, "0.");

	PI_DEBUG_GP (DBG_L3, "DSPPACControl::DSPPACControl -- Controller-Phase\n");
	if (coldstart) xrm.Get("PAC_ctrlmode_Phase", &pll.ctrlmode_Phase, "0.");
	xrm.Get("PAC_setpoint_Phase", &pll.setpoint_Phase, "0.");
	pll.setpoint_Phase = 0.; // force =zero all times.
	pll.corrmax_Phase = 0.;
	pll.corrmin_Phase = 0.;
	xrm.Get("PAC_signum_cp_Phase", &pll.signum_cp_Phase, "-1.");
	xrm.Get("PAC_cp_gain_Phase", &pll.cp_gain_Phase, "-61.6");
	xrm.Get("PAC_signum_ci_Phase", &pll.signum_ci_Phase, "-1.");
	xrm.Get("PAC_ci_gain_Phase", &pll.ci_gain_Phase, "-127.2");
	xrm.Get("PAC_auto_set_BW_Phase", &pll.auto_set_BW_Phase, "8000.");
	pll.memI_Phase = pll.FSineHz;
	pll.icoef_Phase =  0.;
	pll.pcoef_Phase = 0.;
	pll.errorPI_Phase = 0.;

	PI_DEBUG_GP (DBG_L3, "DSPPACControl::DSPPACControl -- Controller-Amplitude\n");
	if (coldstart) xrm.Get("PAC_ctrlmode_Amp", &pll.ctrlmode_Amp, "0.");
	xrm.Get("PAC_setpoint_Amp", &pll.setpoint_Amp, "0.");
        pll.ctrlmode_Amp_adaptive = 0;
        pll.ctrlmode_Amp_adaptive_delta = 0.15;
        pll.ctrlmode_Amp_adaptive_ratio = 50.;
        pll.ctrlmode_Amp_adaptive_max = 20.;

	pll.corrmax_Amp = 0.;
	pll.corrmin_Amp = 0.;
	xrm.Get("PAC_signum_cp_Amp", &pll.signum_cp_Amp, "1.");
	xrm.Get("PAC_cp_gain_Amp", &pll.cp_gain_Amp, "8.1");
	xrm.Get("PAC_signum_ci_Amp", &pll.signum_ci_Amp, "1.");
	xrm.Get("PAC_ci_gain_Amp", &pll.ci_gain_Amp, "-77.5");
	xrm.Get("PAC_auto_set_BW_Amp", &pll.auto_set_BW_Amp, "8.");
	pll.memI_Amp = pll.volumeSine;
	pll.icoef_Amp = 0.;
	pll.pcoef_Amp = 0.;
	pll.errorPI_Amp = 0.;

	pll.Qfactor = 30000.;
	pll.GainRes = 1.;
	pll.res_gain_computed = 1.;

	PI_DEBUG_GP (DBG_L3, "DSPPACControl::DSPPACControl -- LP settings\n");
	if (coldstart) xrm.Get("PAC_LPSelect_Phase", &pll.LPSelect_Phase, "23");
	if (coldstart) xrm.Get("PAC_LPSelect_Amp", &pll.LPSelect_Amp, "47");

	PI_DEBUG_GP (DBG_L3, "DSPPACControl::DSPPACControl -- References and Clipping\n");
	xrm.Get("PAC_Reference0", &pll.Reference[0], "29800.0");
	xrm.Get("PAC_Reference1", &pll.Reference[1], "0.");
	xrm.Get("PAC_Reference2", &pll.Reference[2], "0.");
	xrm.Get("PAC_Reference3", &pll.Reference[3], "0.");
	xrm.Get("PAC_Range0", &pll.Range[0], "187.");
	xrm.Get("PAC_Range1", &pll.Range[1], "7.16.");
	xrm.Get("PAC_Range2", &pll.Range[2], "0.625");
	xrm.Get("PAC_Range3", &pll.Range[3], "1.25");
	xrm.Get("PAC_ClipMin0", &pll.ClipMin[0], "29000.");
	xrm.Get("PAC_ClipMin1", &pll.ClipMin[1], "-180.");
	xrm.Get("PAC_ClipMin2", &pll.ClipMin[2], "-0.05");
	xrm.Get("PAC_ClipMin3", &pll.ClipMin[3], "0.");
	xrm.Get("PAC_ClipMax0", &pll.ClipMax[0], "31000.");
	xrm.Get("PAC_ClipMax1", &pll.ClipMax[1], "180.");
	xrm.Get("PAC_ClipMax2", &pll.ClipMax[2], "1.0");
	xrm.Get("PAC_ClipMax3", &pll.ClipMax[3], "5.0");

	PI_DEBUG_GP (DBG_L3, "DSPPACControl::DSPPACControl -- init remaining module variables\n");
// life watches
	pll.InputFiltered = 0.;
	pll.SineOut0 = 0.;
	pll.phase = 0.;
	pll.amp_estimation = 0.;
	pll.Filter64Out[0] = 0.;
	pll.Filter64Out[1] = 0.;
	pll.Filter64Out[2] = 0.;
	pll.Filter64Out[3] = 0.;

// other
	pll.tune_freq[0] = 31000.;
	pll.tune_freq[1] = 33000.;
	pll.tune_freq[2] = 1.;
	pll.tune_integration_time = 0.1;


	Unity    = new UnitObj(" "," ");
	Hz       = new UnitObj("Hz","Hz");
	Deg      = new UnitObj(UTF8_DEGREE,"deg");
	VoltDeg  = new UnitObj("V/" UTF8_DEGREE, "V/deg");
	Volt     = new UnitObj("V","V");
	VoltHz   = new UnitObj("mV/Hz","mV/Hz");
	dB       = new UnitObj("dB","dB");
	mVolt    = new UnitObj("mV","mV");
	Time     = new UnitObj("s","s");
	uTime    = new LinUnit(UTF8_MU "s", "us", "Time", 1e-6);

	PI_DEBUG_GP (DBG_L3, "DSPPACControl::DSPPACControl -- Create GUI\n");
	create_folder ();
}

DSPPACControl::~DSPPACControl (){
	XsmRescourceManager xrm("sranger_mk2_hwi_control");

	xrm.Put("PAC_Tau_PAC", pll.Tau_PAC);
	xrm.Put("PAC_FSineHz", pll.FSineHz);
	xrm.Put("PAC_volumeSine", pll.volumeSine);

	xrm.Put("PAC_setpoint_Phase", pll.setpoint_Phase);
	xrm.Put("PAC_signum_cp_Phase", pll.signum_cp_Phase);
	xrm.Put("PAC_cp_gain_Phase", pll.cp_gain_Phase);
	xrm.Put("PAC_signum_ci_Phase", pll.signum_ci_Phase);
	xrm.Put("PAC_ci_gain_Phase", pll.ci_gain_Phase);
	xrm.Put("PAC_auto_set_BW_Phase", pll.auto_set_BW_Phase);

	xrm.Put("PAC_setpoint_Amp", pll.setpoint_Amp);
	xrm.Put("PAC_signum_cp_Amp", pll.signum_cp_Amp);
	xrm.Put("PAC_cp_gain_Amp", pll.cp_gain_Amp);
	xrm.Put("PAC_signum_ci_Amp", pll.signum_ci_Amp);
	xrm.Put("PAC_ci_gain_Amp", pll.ci_gain_Amp);
	xrm.Put("PAC_auto_set_BW_Amp", pll.auto_set_BW_Amp);

	xrm.Put("PAC_LPSelect_Phase", pll.LPSelect_Phase);
	xrm.Put("PAC_LPSelect_Amp", pll.LPSelect_Amp);

	xrm.Put("PAC_Reference0", pll.Reference[0]);
	xrm.Put("PAC_Reference1", pll.Reference[1]);
	xrm.Put("PAC_Reference2", pll.Reference[2]);
	xrm.Put("PAC_Reference3", pll.Reference[3]);
	xrm.Put("PAC_Range0", pll.Range[0]);
	xrm.Put("PAC_Range1", pll.Range[1]);
	xrm.Put("PAC_Range2", pll.Range[2]);
	xrm.Put("PAC_Range3", pll.Range[3]);
	xrm.Put("PAC_ClipMin0", pll.ClipMin[0]);
	xrm.Put("PAC_ClipMin1", pll.ClipMin[1]);
	xrm.Put("PAC_ClipMin2", pll.ClipMin[2]);
	xrm.Put("PAC_ClipMin3", pll.ClipMin[3]);
	xrm.Put("PAC_ClipMax0", pll.ClipMax[0]);
	xrm.Put("PAC_ClipMax1", pll.ClipMax[1]);
	xrm.Put("PAC_ClipMax2", pll.ClipMax[2]);
	xrm.Put("PAC_ClipMax3", pll.ClipMax[3]);

	delete uTime;
	delete Time;
	delete mVolt;
	delete dB;
	delete VoltHz;
	delete Volt;
	delete VoltDeg;
	delete Deg;
	delete Hz;
	delete Unity;

	g_free (pll.Signal1);
	g_free (pll.Signal1x);
	g_free (pll.Signal2);
	g_free (pll.Signal2x);
	g_free (pll.tmp_array);
	g_free (pll.x_array);
	g_free (pll.x_array_x);

}

#if 0
static gboolean create_window_key_press_event_lcb(GtkWidget *widget, GdkEventKey *event,GtkWidget *win)
{
	if (event->keyval == GDK_KEY_Escape) {
		PI_DEBUG (DBG_L2, "Got escape\n" );
		return TRUE;
	}
	return FALSE;
}
#endif			

void DSPPACControl::create_folder (){
	GSList *EC_FRQ_list=NULL;
	GSList *EC_R_list=NULL;
	GSList *EC_CLIP_list=NULL;
	GSList *EC_CIP_amp_list=NULL;
	GSList *EC_CIP_phase_list=NULL;

	GSList *EC_amp_freeze_list=NULL;
	GSList *EC_phase_freeze_list=NULL;

	GtkWidget *frame_param, *wid, *menu, *menuitem;
        gint    clip_index;

	AppWindowInit("Phase/Amplitude Convergent Detector Control");

	// ========================================
	GtkWidget *notebook = gtk_notebook_new ();
	gtk_grid_attach (GTK_GRID (v_grid), notebook, 1,1, 1,1);

        // **************************************************
        // ========== Operation Folder
	GtkWidget *PACCrtl = gtk_label_new ("Operation");
        pac_bp = new PAC_GUI_Builder ();
        pac_bp->set_error_text ("Invalid Value.");

        pac_bp->set_no_spin ();
	gtk_widget_show (PACCrtl);
	gtk_notebook_append_page (GTK_NOTEBOOK (notebook), pac_bp->grid, PACCrtl);

        // **************************************************
        //----- EXCITATION: [0]
        clip_index=0;
        
	pac_bp->new_grid_with_frame ("Excitation Freq.");

        pac_bp->set_pcs_remote_prefix ("dsp-pac-exci-freq-");
        
	pac_bp->grid_add_ec ("Ref.", Hz, &pll.Reference[clip_index], 0., 75000., "11.5f", 0.1, 100., "ref");
        pac_bp->set_ec_change_notice_fkt (DSPPACControl::Changed_Operation, this);
	gtk_widget_set_tooltip_text (pac_bp->input, "Reference Freq.");
	EC_FRQ_list = g_slist_prepend( EC_FRQ_list, pac_bp->ec);
        pac_bp->new_line();
        
	pac_bp->grid_add_button ("Set Ranges", NULL, 2);
	g_object_set_data(G_OBJECT (pac_bp->button), "CLIP_index", GINT_TO_POINTER (clip_index));
	g_signal_connect (G_OBJECT (pac_bp->button), "clicked", G_CALLBACK (SetClipping_callback), this);
        pac_bp->new_line();

	pac_bp->grid_add_ec ("Range", Hz, &pll.Range[clip_index], 0., 75000., ".3f", 1., 100., "range");
	pac_bp->set_ec_change_notice_fkt (DSPPACControl::Changed_Operation, this);
	gtk_widget_set_tooltip_text (pac_bp->input, "Freq. Range");
        pac_bp->new_line();

	pac_bp->grid_add_ec ("Min.", Hz, &pll.ClipMin[clip_index], 0., 75000., "g", 1., 10., "min");
	pac_bp->set_ec_change_notice_fkt (DSPPACControl::Changed_Operation, this);
	EC_CLIP_list = g_slist_prepend( EC_CLIP_list, pac_bp->ec);
	gtk_widget_set_tooltip_text (pac_bp->input, "Min. Freq. (abs)");
        pac_bp->new_line();

	pac_bp->grid_add_ec ("Max.", Hz, &pll.ClipMax[clip_index], 0., 75000., "g", 1., 10., "max");
	pac_bp->set_ec_change_notice_fkt (DSPPACControl::Changed_Operation, this);
	EC_CLIP_list = g_slist_prepend( EC_CLIP_list, pac_bp->ec);
	gtk_widget_set_tooltip_text (pac_bp->input, "Max. Freq. (abs)");

#if 0
// --- Mux to Output -- N/A w GXSM-PAC
	pac_bp->grid_add_ec ("Sens.", VoltHz, &pll.xx, 0., 75000., "g", 1., 1., "sens");
	pac_bp->set_ec_change_notice_fkt (DSPPACControl::Changed_Operation, this);
	gtk_widget_set_tooltip_text (pac_bp->input, "Sensitivity (Freq.)");

	pac_bp->grid_add_ec ("Mux. Freq", Out, &pll.xx, 0., 75000., "g", 1., 1., "muxf");
	pac_bp->set_ec_change_notice_fkt (DSPPACControl::Changed_Operation, this);
	gtk_widget_set_tooltip_text (pac_bp->input, "Mux Freq.");
#endif

        pac_bp->pop_grid ();
        
        // **************************************************
        //----- PHASE: [1]
        clip_index=1;
	pac_bp->new_grid_with_frame ("Resonator Phase");

        pac_bp->set_pcs_remote_prefix ("dsp-pac-res-phase-");

	pac_bp->grid_add_ec ("Ref.", Deg, &pll.Reference[clip_index], -180., 180., ".3f", 0.1, 5., "ref");
	pac_bp->set_ec_change_notice_fkt (DSPPACControl::Changed_Operation, this);
	gtk_widget_set_tooltip_text (pac_bp->input, "Reference Phase. (setpoint Phase)");
        pac_bp->new_line();
        
	pac_bp->grid_add_button ("Set Ranges", NULL, 2);
	g_object_set_data(G_OBJECT (pac_bp->button), "CLIP_index", GINT_TO_POINTER (clip_index));
	g_signal_connect (G_OBJECT (pac_bp->button), "clicked", G_CALLBACK (SetClipping_callback), this);
        pac_bp->new_line();

	pac_bp->grid_add_ec ("Range", Deg, &pll.Range[clip_index], 0., 360., "g", 1., 30., "range");
	pac_bp->set_ec_change_notice_fkt(DSPPACControl::Changed_Operation, this);
	gtk_widget_set_tooltip_text (pac_bp->input, "Phase. Range");
        pac_bp->new_line();

	pac_bp->grid_add_ec ("Min.", Deg, &pll.ClipMin[clip_index], -180., 180., "g", 1., 30., "min");
	pac_bp->set_ec_change_notice_fkt(DSPPACControl::Changed_Operation, this);
	EC_CLIP_list = g_slist_prepend( EC_CLIP_list, pac_bp->ec);
	gtk_widget_set_tooltip_text (pac_bp->input, "Min. Phase. (rel)");
        pac_bp->new_line();

	pac_bp->grid_add_ec ("Max.", Deg, &pll.ClipMax[clip_index], -180., 180., "g", 1., 30., "max");
	pac_bp->set_ec_change_notice_fkt(DSPPACControl::Changed_Operation, this);
	EC_CLIP_list = g_slist_prepend( EC_CLIP_list, pac_bp->ec);
	gtk_widget_set_tooltip_text (pac_bp->input, "Max. Phase. (rel)");
        pac_bp->new_line();

#if 0
// --- Mux to Output -- N/A w GXSM-PAC
	pac_bp->grid_add_ec ("Sens.", VoltDeg, &pll.xx, 0., 75000., "g");
	pac_bp->set_ec_change_notice_fkt(DSPPACControl::Changed_Operation, this);
	gtk_widget_set_tooltip_text (pac_bp->input, "Sensitivity (Phase)");
        pac_bp->new_line();

	pac_bp->grid_add_ec ("Mux. Phase", Out, &pll.xx, 0., 75000., "g");
	pac_bp->set_ec_change_notice_fkt(DSPPACControl::Changed_Operation, this);
	gtk_widget_set_tooltip_text (pac_bp->input, "Mux Phase");
#endif

        pac_bp->pop_grid ();

        // **************************************************
        //----- EXCI-AMP: [3] doc is wrong, it is NOT 3!
        clip_index=3; // see above note
        pac_bp->new_grid_with_frame ("Excitation Amp.");

        pac_bp->set_pcs_remote_prefix ("dsp-pac-exci-amp-");

	pac_bp->grid_add_ec ("Ref.", Volt, &pll.Reference[clip_index], 0., 10., ".4f", 0.1, 1.0, "ref");
	pac_bp->set_ec_change_notice_fkt(DSPPACControl::Changed_Operation, this);
	gtk_widget_set_tooltip_text (pac_bp->input, "Exitation Ref (Volume Sine)");
        pac_bp->new_line();
 
	pac_bp->grid_add_button ("Set Ranges", NULL, 2);
	g_object_set_data(G_OBJECT (pac_bp->button), "CLIP_index", GINT_TO_POINTER (clip_index));
	g_signal_connect (G_OBJECT (pac_bp->button), "clicked", G_CALLBACK (SetClipping_callback), this);
        pac_bp->new_line();

	pac_bp->grid_add_ec ("Range", Volt, &pll.Range[clip_index], -10., 10., "g", 0.1, 1., "range");
	pac_bp->set_ec_change_notice_fkt(DSPPACControl::Changed_Operation, this);
	gtk_widget_set_tooltip_text (pac_bp->input, "Amp. Range");
        pac_bp->new_line();

	pac_bp->grid_add_ec ("Min.", Volt, &pll.ClipMin[clip_index], -10., 10., "g", 0.1, 1., "min");
	pac_bp->set_ec_change_notice_fkt(DSPPACControl::Changed_Operation, this);
	EC_CLIP_list = g_slist_prepend( EC_CLIP_list, pac_bp->ec);
	gtk_widget_set_tooltip_text (pac_bp->input, "Min. Amp. (abs)");
        pac_bp->new_line();

	pac_bp->grid_add_ec ("Max.", Volt, &pll.ClipMax[clip_index], -10., 10., "g", 0.1, 1., "max");
	pac_bp->set_ec_change_notice_fkt(DSPPACControl::Changed_Operation, this);
	EC_CLIP_list = g_slist_prepend( EC_CLIP_list, pac_bp->ec);
	gtk_widget_set_tooltip_text (pac_bp->input, "Max. Amp. (abs)");
        pac_bp->new_line();

#if 0
// --- Mux to Output -- N/A w GXSM-PAC
	pac_bp->grid_add_ec ("Sens.", Volt, &pll.xx, 0., 75000., "g", 0.1, 1., "sens");
	pac_bp->set_ec_change_notice_fkt(DSPPACControl::Changed_Operation, this);
	gtk_widget_set_tooltip_text (pac_bp->input, "Sensitivity (Amp.)");
        pac_bp->new_line();

	pac_bp->grid_add_ec ("Mux. Amp", Out, &pll.xx, 0., 75000., "g", 0.1, 1., "muxa");
	pac_bp->set_ec_change_notice_fkt(DSPPACControl::Changed_Operation, this);
	EC_list = g_slist_prepend( EC_list, ec);
	gtk_widget_set_tooltip_text (pac_bp->input, "Mux Amp.");
#endif

        pac_bp->pop_grid ();

        // **************************************************
        //----- RES-AMP: [2] // doc is wrong, it is NOT 3!
        clip_index=2;
	pac_bp->new_grid_with_frame ("Resonator Amp.");
        pac_bp->set_pcs_remote_prefix ("dsp-pac-res-amp-");

	pac_bp->grid_add_ec ("Ref.", Volt, &pll.Reference[clip_index], 0., 10., "g", 1e-3, 0.1, "ref");
	pac_bp->set_ec_change_notice_fkt(DSPPACControl::Changed_Operation, this);
	gtk_widget_set_tooltip_text (pac_bp->input, "Reference Amp. (setpoint Amp)");
        pac_bp->new_line ();
        
	pac_bp->grid_add_button ("Set Ranges", NULL, 2);
	g_object_set_data(G_OBJECT (pac_bp->button), "CLIP_index", GINT_TO_POINTER (clip_index));
	g_signal_connect (G_OBJECT (pac_bp->button), "clicked", G_CALLBACK (SetClipping_callback), this);
        pac_bp->new_line ();

	pac_bp->grid_add_ec ("Range", Volt, &pll.Range[clip_index], 0., 10., "g", 0.1, 1., "range");
	pac_bp->set_ec_change_notice_fkt(DSPPACControl::Changed_Operation, this);
	gtk_widget_set_tooltip_text (pac_bp->input, "Freq. Range");
        pac_bp->new_line ();

	pac_bp->grid_add_ec ("Min.", Volt, &pll.ClipMin[clip_index], 0., 10., "g", 0.1, 1., "min");
	pac_bp->set_ec_change_notice_fkt(DSPPACControl::Changed_Operation, this);
	EC_CLIP_list = g_slist_prepend( EC_CLIP_list, pac_bp->ec);
	gtk_widget_set_tooltip_text (pac_bp->input, "Min. Freq. (abs)");
        pac_bp->new_line ();

	pac_bp->grid_add_ec ("Max.", Volt, &pll.ClipMax[clip_index], 0., 10., "g", 0.1, 1., "max");
	pac_bp->set_ec_change_notice_fkt(DSPPACControl::Changed_Operation, this);
	EC_CLIP_list = g_slist_prepend( EC_CLIP_list, pac_bp->ec);
	gtk_widget_set_tooltip_text (pac_bp->input, "Max. Freq. (abs)");
        pac_bp->new_line ();

#if 0
// --- Mux to Output -- N/A w GXSM-PAC
	pac_bp->grid_add_ec ("Sens.", Volt, &pll.xx, 0., 75000., "g", 0.1, 1., "sens");
	pac_bp->set_ec_change_notice_fkt(DSPPACControl::Changed_Operation, this);
	gtk_widget_set_tooltip_text (pac_bp->input, "Sensitivity (Exci Amp.)");
        pac_bp->new_line ();

	pac_bp->grid_add_ec ("Mux. Exci Amp", Out, &pll.xx, 0., 75000., "g", 0.1, 1., "muxea");
	pac_bp->set_ec_change_notice_fkt(DSPPACControl::Changed_Operation, this);
	gtk_widget_set_tooltip_text (pac_bp->input, "Mux Exci Amp.");
#endif

        pac_bp->pop_grid ();

        pac_bp->new_line ();
        
        pac_bp->set_pcs_remote_prefix ("dsp-pac-");

        // **************************************************
        // ------- Low Pass Settings  for   Freq&Phase
	pac_bp->grid_add_label ("Low Pass Filter Freq.&Phase");
	wid = gtk_combo_box_text_new ();
        g_object_set_data(G_OBJECT (wid), "LP_var", &pll.LPSelect_Phase);
        g_signal_connect (G_OBJECT (wid), "changed",
                          G_CALLBACK (DSPPACControl::choice_PAC_LP_callback),
                          this);
        pac_bp->grid_add_widget (wid);
        
	//                       0      1           2          3          4          5         6         7         8        9        10       11       12        13        14
	const gchar *LP_option[] = { "OFF", "16548 Hz", "6868 Hz", "3188 Hz", "1541 Hz", "757 Hz", "376 Hz", "187 Hz", "93 Hz", "47 Hz", "23 Hz", "12 Hz", "5.8 Hz", "2.9 Hz", "1.5 Hz", NULL };

	// Init LP-choicelist
	for(int i=0; LP_option[i]; i++)
                gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (wid), LP_option[i], LP_option[i]);

	gtk_combo_box_set_active (GTK_COMBO_BOX (wid), pll.LPSelect_Phase);


	// Freq: +/- 187 kHz ... 23.9 kHz ... 187 Hz** ... 2.85 mHz  in x2
	// Deg:  +/- 57, 28.6, ... 7.16** ... 14 m ... 6.83u         in x2
	// Volt: +/- 10, 5, 2.5, 1.25V ** ... 4.88 mV, ... 1.19 uV   in x2



        // **************************************************
        // ------- Low Pass Settings  for   Amplitude
	pac_bp->grid_add_label ("Low Pass Filter Amp.");
	wid = gtk_combo_box_text_new ();
        g_object_set_data(G_OBJECT (wid), "LP_var", &pll.LPSelect_Amp);
        g_signal_connect (G_OBJECT (wid), "changed",
                          G_CALLBACK (DSPPACControl::choice_PAC_LP_callback),
                          this);
        pac_bp->grid_add_widget (wid);

	// Init LP-choicelist
	for(int i=0; LP_option[i]; i++)
                gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (wid), LP_option[i], LP_option[i]);

	gtk_combo_box_set_active (GTK_COMBO_BOX (wid), pll.LPSelect_Amp);

        pac_bp->new_line ();

        // **************************************************
        //----- TAU PAC
	pac_bp->new_grid_with_frame ("Phase Detector Time Constant", 4);

        //	pac_bp->grid_add_ec ("Tau PAC", uTime, &pll.Tau_PAC, 0., 63., "11.2f", 10., 100., "tau");
	pac_bp->grid_add_ec ("Tau PAC", Time, &pll.Tau_PAC, 0., 63., "g", 10., 100., "tau");
	pac_bp->set_ec_change_notice_fkt (DSPPACControl::Changed_TauPAC, this);
	gtk_widget_set_tooltip_text (pac_bp->input, "Tau PAC");

	pac_bp->grid_add_check_button ("PAC Processing");
	g_object_set_data(G_OBJECT (pac_bp->button), "CONTROLLER_ID", GINT_TO_POINTER (101)); // PAC Processing
	gint s=0; sranger_common_hwi->read_dsp_state(s);
	gtk_check_button_set_active (GTK_CHECK_BUTTON (pac_bp->button), (s & MD_PLL) ? 1:0);
	g_signal_connect (G_OBJECT (pac_bp->button), "toggled", G_CALLBACK (DSPPACControl::controller_callback), this);

        pac_bp->pop_grid ();
        

        gtk_widget_show (pac_bp->grid);

        // **************************************************
        // ========== PAC-Feedback Folder
	PACCrtl = gtk_label_new ("PAC-Feedback");
	gtk_widget_show (PACCrtl);

        pac_bp->new_grid ();
	gtk_notebook_append_page (GTK_NOTEBOOK (notebook), pac_bp->grid, PACCrtl);

        // **************************************************
        // ========== Excitation Sine/Monitor
	pac_bp->new_grid_with_frame ("Excitation Sine");

        pac_bp->set_pcs_remote_prefix ("dsp-pac-excitation-sine-");
        pac_bp->set_no_spin (false);
	pac_bp->grid_add_ec ("Sine-Amp", Volt, &pll.volumeSine, 0., 10., ".3f", 0.01, 0.1,"amp");
	pac_bp->set_ec_change_notice_fkt(DSPPACControl::Changed_Sine, this);
	EC_amp_freeze_list = g_slist_prepend (EC_amp_freeze_list, pac_bp->ec);
	gtk_widget_set_tooltip_text (pac_bp->input, "Excitation Sine Wave Amplitude");
        pac_bp->new_line ();

	pac_bp->grid_add_ec ("Sine-Freq", Hz, &pll.FSineHz, 0., 75000., ".4f", 0.1, 100.,"freq");
	pac_bp->set_ec_change_notice_fkt(DSPPACControl::Changed_Sine, this);
	EC_phase_freeze_list = g_slist_prepend( EC_phase_freeze_list, pac_bp->ec);
	gtk_widget_set_tooltip_text (pac_bp->input, "Excitation Frequency");
        pac_bp->new_line ();
        pac_bp->set_no_spin ();
	EC_FRQ_list = g_slist_prepend( EC_FRQ_list, pac_bp->ec);

        pac_bp->set_pcs_remote_prefix ("dsp-pac-");

//----- Monitoring
	pac_bp->grid_add_ec ("Res-Freq", Hz, &pll.Filter64Out[F64_Excitation], 0., 75000., ".4f");
	//	pac_bp->set_ec_change_notice_fkt(DSPPACControl::Changed_Sine, this);
	EC_R_list = g_slist_prepend( EC_R_list, pac_bp->ec);
	pac_bp->ec->Freeze ();
        pac_bp->new_line ();

	pac_bp->grid_add_ec ("Res-Phase", Deg, &pll.Filter64Out[F64_ResPhaseLP], -180., 180., "g");
	//	pac_bp->set_ec_change_notice_fkt(DSPPACControl::Changed_Sine, this);
	EC_R_list = g_slist_prepend( EC_R_list, pac_bp->ec);
	pac_bp->ec->Freeze ();
        pac_bp->new_line ();

	pac_bp->grid_add_ec ("Res-Amp", Volt, &pll.Filter64Out[F64_ResAmpLP], 0., 10., "g");
	//	pac_bp->set_ec_change_notice_fkt(DSPPACControl::Changed_Sine, this);
	EC_R_list = g_slist_prepend( EC_R_list, pac_bp->ec);
	pac_bp->ec->Freeze ();
        pac_bp->new_line ();

	pac_bp->grid_add_ec ("Exec-Amp", Volt, &pll.Filter64Out[F64_ExecAmpLP], 0., 10., "g");
	//	pac_bp->set_ec_change_notice_fkt(DSPPACControl::Changed_Sine, this);
	EC_R_list = g_slist_prepend( EC_R_list, pac_bp->ec);
	pac_bp->ec->Freeze ();
        pac_bp->new_line ();

	pac_bp->grid_add_ec ("Phase-Mon", Deg, &pll.phase, -180., 180., "g");
	//	pac_bp->set_ec_change_notice_fkt(DSPPACControl::Changed_Sine, this);
	EC_R_list = g_slist_prepend( EC_R_list, pac_bp->ec);
	pac_bp->ec->Freeze ();
        pac_bp->new_line ();

	pac_bp->grid_add_ec ("AmpEst-Mon", Volt, &pll.amp_estimation, 0., 10., "g");
	//	pac_bp->set_ec_change_notice_fkt(DSPPACControl::Changed_Sine, this);
	EC_R_list = g_slist_prepend( EC_R_list, pac_bp->ec);
	pac_bp->ec->Freeze ();
        pac_bp->new_line ();

	pac_bp->grid_add_check_button ("Monitor", NULL, 1);
	g_object_set_data (G_OBJECT (pac_bp->button), "CONTROLLER_ID", GINT_TO_POINTER (100)); // 100 is MONITOR SWITCH
	gtk_check_button_set_active (GTK_CHECK_BUTTON (pac_bp->button), refresh_timer_id ? 1:0);
	g_signal_connect (G_OBJECT (pac_bp->button), "toggled",
			    G_CALLBACK (DSPPACControl::controller_callback), this);

	pac_bp->grid_add_button ("Copy Res-Freq to Ref.", NULL, 1);
	g_signal_connect (G_OBJECT (pac_bp->button), "clicked",
                          G_CALLBACK (DSPPACControl::copy_ref_freq_to_ref_callback), this);

        pac_bp->pop_grid ();
        
        // **************************************************
        // ----- Amplitude FB:
	pac_bp->new_grid_with_frame ("Amplitude FB Controller");

        pac_bp->set_no_spin (false);
	pac_bp->grid_add_ec ("Setpoint-Amp", Volt, &pll.setpoint_Amp, 0., 10., ".3f", 1e-3, 0.01, "am-set");
	pac_bp->set_ec_change_notice_fkt(DSPPACControl::Changed_ControllerAmp, this);
	gtk_widget_set_tooltip_text (pac_bp->input, "Setpoint Amp.");
        pac_bp->new_line ();

	pac_bp->grid_add_ec ("CP-Amp gain", dB, &pll.cp_gain_Amp, -140., 100., "g", 0.1, 5.0, "am-cp-gain");
	pac_bp->set_ec_change_notice_fkt(DSPPACControl::Changed_ControllerAmp, this);
	EC_CIP_amp_list = g_slist_prepend (EC_CIP_amp_list, pac_bp->ec);
	gtk_widget_set_tooltip_text (pac_bp->input, "Coef. Proportional Amplitude Gain");

	pac_bp->grid_add_check_button ("Inv");
	g_object_set_data(G_OBJECT (pac_bp->button), "CONTROLLER_ID", GINT_TO_POINTER (1)); // 1 is AMPLITUDE SWITCH CP INV (negatibe signum)
	gtk_check_button_set_active (GTK_CHECK_BUTTON (pac_bp->button), pll.signum_cp_Amp<0. ? 1:0);
	g_signal_connect (G_OBJECT (pac_bp->button), "toggled", G_CALLBACK (DSPPACControl::controller_callback), this);
        pac_bp->new_line ();

	pac_bp->grid_add_ec ("CI-Amp gain", dB, &pll.ci_gain_Amp, -140., 100., "g", 0.1, 5.0, "am-ci-gain");
	pac_bp->set_ec_change_notice_fkt (DSPPACControl::Changed_ControllerAmp, this);
	EC_CIP_amp_list = g_slist_prepend (EC_CIP_amp_list, pac_bp-> ec);
	gtk_widget_set_tooltip_text (pac_bp->input, "Coef. Integral Amplitide gain");
        pac_bp->set_no_spin (true);

	pac_bp->grid_add_check_button ("Inv");
	g_object_set_data(G_OBJECT (pac_bp->button), "CONTROLLER_ID", GINT_TO_POINTER (2)); // 2 is AMPLITUDE SWITCH CI INV (negatibe signum)
	gtk_check_button_set_active (GTK_CHECK_BUTTON (pac_bp->button), pll.signum_ci_Amp<0. ? 1:0);
	g_signal_connect (G_OBJECT (pac_bp->button), "toggled", G_CALLBACK (DSPPACControl::controller_callback), this);

        pac_bp->new_line ();


	pac_bp->grid_add_ec ("BW-Amp", Hz, &pll.auto_set_BW_Amp, 0.01, 100., "g", 1., 1., "am-bw-set");
	pac_bp->set_ec_change_notice_fkt(DSPPACControl::Changed_ControllerAmpAutoSet, this);
	gtk_widget_set_tooltip_text (pac_bp->input, "Auto set CP,CI for Amplitude controller from bandwidth");
        pac_bp->new_line ();

	pac_bp->grid_add_ec ("Q", Unity, &pll.Qfactor, 0.1, 1e8, "g", 10., 1000., "res-q-factor");
	pac_bp->set_ec_change_notice_fkt(DSPPACControl::Changed_ControllerAmpAutoSet, this);
	gtk_widget_set_tooltip_text (pac_bp->input, "Q factor needed for auto set CP,CI calculation");
        pac_bp->new_line ();

	pac_bp->grid_add_ec ("Res. Gain", Unity, &pll.GainRes, 1e-6, 1e10, "g", 1., 1., "res-gain");
	pac_bp->set_ec_change_notice_fkt(DSPPACControl::Changed_ControllerAmpAutoSet, this);
	gtk_widget_set_tooltip_text (pac_bp->input, "Resonator gain needed for auto set CP,CI calculation");
        pac_bp->new_line ();

	pac_bp->grid_add_ec ("Res. Gain now", Unity, &pll.res_gain_computed, -1e10, 1e10, "g");
	EC_R_list = g_slist_prepend( EC_R_list, pac_bp->ec);
	pac_bp->ec->Freeze ();
        pac_bp->new_line ();

	pac_bp->grid_add_check_button ("Amplitude Feedback", NULL, 2);
	g_object_set_data(G_OBJECT (pac_bp->button), "CONTROLLER_ID", GINT_TO_POINTER (0)); // 0 is AMPLITUDE SWITCH
	gtk_check_button_set_active (GTK_CHECK_BUTTON (pac_bp->button), pll.ctrlmode_Amp ? 1:0);
	g_signal_connect (G_OBJECT (pac_bp->button), "toggled",
                          G_CALLBACK (DSPPACControl::controller_callback), this);
        pac_bp->new_line ();

	pac_bp->grid_add_check_button ("Adaptive Mode (experimental)", NULL, 2);
	g_object_set_data(G_OBJECT (pac_bp->button), "CONTROLLER_ID", GINT_TO_POINTER (200)); // 0 is AMPLITUDE ADAPTIVE SWITCH
	gtk_check_button_set_active (GTK_CHECK_BUTTON (pac_bp->button), pll.ctrlmode_Amp_adaptive ? 1:0);
        // _delta,ratio,max
	g_signal_connect (G_OBJECT (pac_bp->button), "toggled",
                          G_CALLBACK (DSPPACControl::controller_callback), this);
        pac_bp->new_line ();
        
        pac_bp->set_configure_hide_list_b_mode_on ();
	pac_bp->grid_add_ec ("Ada-delta", Unity, &pll.ctrlmode_Amp_adaptive_delta, 0., 1., ".5f", 0.01, 0.01);
	gtk_widget_set_tooltip_text (pac_bp->input, "Adaptive threashold, relative to setpoint");
        pac_bp->new_line ();

	pac_bp->grid_add_ec ("Ada-ratio", Unity, &pll.ctrlmode_Amp_adaptive_ratio, -1., 1000., ".1f", 1., 1.);
	gtk_widget_set_tooltip_text (pac_bp->input, "Adaptive threashold, relative to setpoint");
        pac_bp->new_line ();

	pac_bp->grid_add_ec ("Ada-max", dB, &pll.ctrlmode_Amp_adaptive_max, 0., 30., ".1f", 1., 1.);
	gtk_widget_set_tooltip_text (pac_bp->input, "Adaptive threashold, relative to setpoint");

        pac_bp->set_configure_hide_list_b_mode_off ();
	g_object_set_data(G_OBJECT (pac_bp->button), "ADA-LIST", pac_bp->get_configure_hide_list_b_head ()); // 0 is AMPLITUDE ADAPTIVE SWITCH

        pac_bp->pop_grid ();
        
        // **************************************************
        // ----- Phase FB:
	pac_bp->new_grid_with_frame ("Phase FB Controller");

        pac_bp->set_no_spin (false);
	pac_bp->grid_add_ec ("Setpoint-Phase", Deg, &pll.setpoint_Phase, -180., 180., "g", 0.1, 5., "ph-set");
	pac_bp->set_ec_change_notice_fkt(DSPPACControl::Changed_ControllerPhase, this);
	EC_CIP_phase_list = g_slist_prepend( EC_CIP_phase_list, pac_bp->ec);
	gtk_widget_set_tooltip_text (pac_bp->input, "Setpoint Phase");
//	pac_bp->ec->Freeze ();
        pac_bp->new_line ();

	pac_bp->grid_add_ec ("CP-Phase gain", dB, &pll.cp_gain_Phase, -140., 100., "g", 0.1, 5.0, "ph-cp-gain");
	pac_bp->set_ec_change_notice_fkt(DSPPACControl::Changed_ControllerPhase, this);
	EC_CIP_phase_list = g_slist_prepend( EC_CIP_phase_list, pac_bp->ec);
	gtk_widget_set_tooltip_text (pac_bp->input, "Coef. Proportional Phase gain");

	pac_bp->grid_add_check_button ("Inv");
	g_object_set_data(G_OBJECT (pac_bp->button), "CONTROLLER_ID", GINT_TO_POINTER (11)); // 11 is Phase SWITCH CP INV (negatibe signum)
	gtk_check_button_set_active (GTK_CHECK_BUTTON (pac_bp->button), pll.signum_cp_Phase<0. ? 1:0);
	g_signal_connect (G_OBJECT (pac_bp->button), "toggled", G_CALLBACK (DSPPACControl::controller_callback), this);
        pac_bp->new_line ();

	pac_bp->grid_add_ec ("CI-Phase gain", dB, &pll.ci_gain_Phase, -140., 100., "g", 0.1, 5.0, "ph-ci-gain");
	pac_bp->set_ec_change_notice_fkt(DSPPACControl::Changed_ControllerPhase, this);
	EC_CIP_phase_list = g_slist_prepend( EC_CIP_phase_list, pac_bp->ec);
	gtk_widget_set_tooltip_text (pac_bp->input, "Coef. Integral Phase gain");
        pac_bp->set_no_spin (true);

	pac_bp->grid_add_check_button ("Inv");
	g_object_set_data(G_OBJECT (pac_bp->button), "CONTROLLER_ID", GINT_TO_POINTER (12)); // 12 is Phase SWITCH CI INV (negatibe signum)
	gtk_check_button_set_active (GTK_CHECK_BUTTON (pac_bp->button), pll.signum_ci_Phase<0. ? 1:0);
	g_signal_connect (G_OBJECT (pac_bp->button), "toggled", G_CALLBACK (DSPPACControl::controller_callback), this);
        pac_bp->new_line ();

	pac_bp->grid_add_ec ("BW-Phase", Hz, &pll.auto_set_BW_Phase, 0.1, 10000., "g", 1., 1., "ph-bw-set");
	pac_bp->set_ec_change_notice_fkt(DSPPACControl::Changed_ControllerPhaseAutoSet, this);
	gtk_widget_set_tooltip_text (pac_bp->input, "Auto set CP,CI for Phase controller from bandwidth");
        pac_bp->new_line ();

	pac_bp->grid_add_check_button ("Phase Feedback", NULL, 2);
	// read currect FB (MD_PID) state
	g_object_set_data(G_OBJECT (pac_bp->button), "CONTROLLER_ID", GINT_TO_POINTER (10)); // 10 is PHASE CONTROLLER SWITCH
	gtk_check_button_set_active (GTK_CHECK_BUTTON (pac_bp->button), pll.ctrlmode_Phase ? 1:0);
	g_signal_connect (G_OBJECT (pac_bp->button), "toggled",
                          G_CALLBACK (DSPPACControl::controller_callback), this);


        pac_bp->pop_grid ();

        gtk_widget_show (v_grid);

	// ============================================================
	// save List away...
	g_object_set_data( G_OBJECT (window), "PAC_EC_list", pac_bp->ec_list);
	g_object_set_data( G_OBJECT (window), "PAC_EC_READINGS_list", EC_R_list);
	g_object_set_data( G_OBJECT (window), "PAC_EC_FRQ_list", EC_FRQ_list);
	g_object_set_data( G_OBJECT (window), "PAC_EC_AMP_FREEZE_list", EC_amp_freeze_list);
	g_object_set_data( G_OBJECT (window), "PAC_EC_PHASE_FREEZE_list", EC_phase_freeze_list);
	g_object_set_data( G_OBJECT (window), "PAC_EC_CLIP_list", EC_CLIP_list);
	g_object_set_data( G_OBJECT (window), "PAC_EC_CIP_amp_list", EC_CIP_amp_list);
	g_object_set_data( G_OBJECT (window), "PAC_EC_CIP_phase_list", EC_CIP_phase_list);

	sranger_mk2_hwi_pi.app->RemoteEntryList = g_slist_concat (sranger_mk2_hwi_pi.app->RemoteEntryList, pac_bp->remote_list_ec);

        set_window_geometry ("dsp-pac-control");

}

guint DSPPACControl::refresh_readings(DSPPACControl *dspc){ 
	if (main_get_gapp()->xsm->hardware->IsSuspendWatches ())
		return TRUE;

	dspc->update_readings ();
	return TRUE;
}

void DSPPACControl::update_readings(){
        static double ref=-1.;
        static gint is_fast = -1;
        double tmp[2];
	sranger_common_hwi->read_pll( pll, PLL_READINGS);
	g_slist_foreach
		( (GSList*) g_object_get_data( G_OBJECT (window), "PAC_EC_READINGS_list"),
		  (GFunc) App::update_ec, NULL
			);
        if (pll.ctrlmode_Amp_adaptive){
                if (pll.ctrlmode_Amp_adaptive_ratio < 0.){
                        for (int i=0; i<5; ++i){
                                if (is_fast)
                                        pll.setpoint_Amp = ref*(1.+fabs(pll.ctrlmode_Amp_adaptive_ratio));
                                else
                                        pll.setpoint_Amp = ref*(1.-fabs(pll.ctrlmode_Amp_adaptive_ratio));
                                is_fast = is_fast ? 0:-1;
                                sranger_common_hwi->write_pll (pll, PLL_CONTROLLER_AMPLITUDE);
                        }
                } else {
                        double delta = fabs (pll.Filter64Out[F64_ResAmpLP] - pll.setpoint_Amp) / pll.setpoint_Amp;
                        if (is_fast<1 && delta > 1.25*pll.ctrlmode_Amp_adaptive_delta){ // make fast
                                tmp[0] = pll.cp_gain_Amp;
                                tmp[1] = pll.ci_gain_Amp;
                                double adjust = fabs(delta) *1.2*pll.ctrlmode_Amp_adaptive_ratio;
                                if (adjust > pll.ctrlmode_Amp_adaptive_max)
                                        adjust = pll.ctrlmode_Amp_adaptive_max;
                                pll.cp_gain_Amp += adjust;
                                pll.ci_gain_Amp += adjust;
                                g_message ("AM-ADA-FFF: %g %g d=%g (%g)",  pll.cp_gain_Amp, pll.ci_gain_Amp, delta, pll.Filter64Out[F64_ResAmpLP]);
                                sranger_common_hwi->write_pll (pll, PLL_CONTROLLER_AMPLITUDE);
                                pll.cp_gain_Amp = tmp[0];
                                pll.ci_gain_Amp = tmp[1];
                                is_fast = 1;
                        } else if (is_fast > 0 && delta < 0.75*pll.ctrlmode_Amp_adaptive_ratio){ // make slow
                                sranger_common_hwi->write_pll (pll, PLL_CONTROLLER_AMPLITUDE);
                                g_message ("AM-ADA-REG: %g %g d=%g (%g)",  pll.cp_gain_Amp, pll.ci_gain_Amp, delta, pll.Filter64Out[F64_ResAmpLP]);
                                is_fast = 0;
                        }
                }
        } else
                ref = pll.setpoint_Amp;
}

void DSPPACControl::update(){
	g_slist_foreach
		( (GSList*) g_object_get_data( G_OBJECT (window), "PAC_EC_list"),
		  (GFunc) App::update_ec, NULL
			);
}

void DSPPACControl::ChangedNotify(Param_Control* pcs, gpointer dspc){
}

void DSPPACControl::Changed_Sine(Param_Control* pcs, gpointer dspc){
	sranger_common_hwi->write_pll (((DSPPACControl*)dspc)->pll, PLL_SINE);
	((DSPPACControl*)dspc)->update ();
}

void DSPPACControl::Changed_Operation(Param_Control* pcs, gpointer dspc){
	sranger_common_hwi->write_pll (((DSPPACControl*)dspc)->pll, PLL_OPERATION);
}

void DSPPACControl::Changed_ControllerAmp(Param_Control* pcs, gpointer dspc){
	sranger_common_hwi->write_pll (((DSPPACControl*)dspc)->pll, PLL_CONTROLLER_AMPLITUDE);
}

void DSPPACControl::Changed_ControllerPhase(Param_Control* pcs, gpointer dspc){
	sranger_common_hwi->write_pll (((DSPPACControl*)dspc)->pll, PLL_CONTROLLER_PHASE);
}

void DSPPACControl::Changed_ControllerAmpAutoSet(Param_Control* pcs, gpointer dspc){
	sranger_common_hwi->write_pll (((DSPPACControl*)dspc)->pll, PLL_CONTROLLER_AMPLITUDE_AUTO_SET);
	g_slist_foreach
		( (GSList*) g_object_get_data( G_OBJECT (((DSPPACControl*)dspc)->window), "PAC_EC_CIP_amp_list"),
		  (GFunc) App::update_ec, NULL
			);
}

void DSPPACControl::Changed_ControllerPhaseAutoSet(Param_Control* pcs, gpointer dspc){
	sranger_common_hwi->write_pll (((DSPPACControl*)dspc)->pll, PLL_CONTROLLER_PHASE_AUTO_SET);
	g_slist_foreach
		( (GSList*) g_object_get_data( G_OBJECT (((DSPPACControl*)dspc)->window), "PAC_EC_CIP_phase_list"),
		  (GFunc) App::update_ec, NULL
			);
}

void DSPPACControl::Changed_TauPAC(Param_Control* pcs, gpointer dspc){
	sranger_common_hwi->write_pll (((DSPPACControl*)dspc)->pll, PLL_TAUPAC);
}


void DSPPACControl::SetClipping_callback(GtkWidget* widget, DSPPACControl *dspc){
	int i = GPOINTER_TO_INT (g_object_get_data( G_OBJECT (widget), "CLIP_index"));
	if (i >= 0 && i < 4){
		dspc->pll.ClipMin[i] = dspc->pll.Reference[i] - dspc->pll.Range[i]/2.;
		dspc->pll.ClipMax[i] = dspc->pll.Reference[i] + dspc->pll.Range[i]/2.;
		sranger_common_hwi->write_pll (((DSPPACControl*)dspc)->pll, PLL_OPERATION);
		g_slist_foreach
			( (GSList*) g_object_get_data( G_OBJECT (((DSPPACControl*)dspc)->window), "PAC_EC_CLIP_list"),
			  (GFunc) App::update_ec, NULL
				);
	}
}

int DSPPACControl::choice_PAC_LP_callback (GtkWidget *widget, DSPPACControl *dspc){
	gint32 *lpi = (gint32*)(g_object_get_data( G_OBJECT (widget), "LP_var"));
	*lpi = gtk_combo_box_get_active (GTK_COMBO_BOX (widget));
        // GPOINTER_TO_INT (g_object_get_data( G_OBJECT (widget), "LP_index"));
	sranger_common_hwi->write_pll( dspc->pll, PLL_OPERATION);
        return 0;
}

int DSPPACControl::controller_callback (GtkWidget *widget, DSPPACControl *dspc){
	int controller_id = GPOINTER_TO_INT (g_object_get_data( G_OBJECT (widget), "CONTROLLER_ID"));
	switch (controller_id){
	case 0: // Amp Controller SW
		dspc->pll.ctrlmode_Amp = (gtk_check_button_get_active (GTK_CHECK_BUTTON (widget))) ? 1:0;
		sranger_common_hwi->write_pll( dspc->pll, PLL_CONTROLLER_AMPLITUDE);
		if (dspc->pll.ctrlmode_Amp)
			g_slist_foreach
				( (GSList*) g_object_get_data( G_OBJECT (((DSPPACControl*)dspc)->window), "PAC_EC_AMP_FREEZE_list"),
				  (GFunc) App::freeze_ec, NULL
					);
		else
			g_slist_foreach
				( (GSList*) g_object_get_data( G_OBJECT (((DSPPACControl*)dspc)->window), "PAC_EC_AMP_FREEZE_list"),
				  (GFunc) App::thaw_ec, NULL
					);
		break;
	case 1: // Amp Controller CP INV
		dspc->pll.signum_cp_Amp = (gtk_check_button_get_active (GTK_CHECK_BUTTON (widget))) ? -1.:1.;
		sranger_common_hwi->write_pll( dspc->pll, PLL_CONTROLLER_AMPLITUDE);
		break;
	case 2: // Amp Controller CI INV
		dspc->pll.signum_ci_Amp = (gtk_check_button_get_active (GTK_CHECK_BUTTON (widget))) ? -1.:1.;
		sranger_common_hwi->write_pll( dspc->pll, PLL_CONTROLLER_AMPLITUDE);
		break;
	case 10: // Phase Controller
		dspc->pll.ctrlmode_Phase = (gtk_check_button_get_active (GTK_CHECK_BUTTON (widget))) ? 1:0;
		sranger_common_hwi->write_pll( dspc->pll, PLL_CONTROLLER_PHASE);
		if (dspc->pll.ctrlmode_Phase)
			g_slist_foreach
				( (GSList*) g_object_get_data( G_OBJECT (((DSPPACControl*)dspc)->window), "PAC_EC_PHASE_FREEZE_list"),
				  (GFunc) App::freeze_ec, NULL
					);
		else
			g_slist_foreach
				( (GSList*) g_object_get_data( G_OBJECT (((DSPPACControl*)dspc)->window), "PAC_EC_PHASE_FREEZE_list"),
				  (GFunc) App::thaw_ec, NULL
					);
		break;
	case 11: // Phase Controller CP INV
		dspc->pll.signum_cp_Phase = (gtk_check_button_get_active (GTK_CHECK_BUTTON (widget))) ? -1.:1.;
		sranger_common_hwi->write_pll( dspc->pll, PLL_CONTROLLER_PHASE);
		break;
	case 12: // Phase Controller CI INV
		dspc->pll.signum_ci_Phase = (gtk_check_button_get_active (GTK_CHECK_BUTTON (widget))) ? -1.:1.;
		sranger_common_hwi->write_pll( dspc->pll, PLL_CONTROLLER_PHASE);
		break;
	case 100: 
		if (gtk_check_button_get_active (GTK_CHECK_BUTTON (widget)) && !dspc->refresh_timer_id){
			dspc->refresh_timer_id = g_timeout_add (200, (GSourceFunc)DSPPACControl::refresh_readings, dspc);
			PI_DEBUG_GP (DBG_L3, "controller_callback -- Monitoring ON\n");
		} else
			if (dspc->refresh_timer_id){
				g_source_remove (dspc->refresh_timer_id);
				dspc->refresh_timer_id = 0;
				PI_DEBUG_GP (DBG_L3, "controller_callback -- Monitoring OFF\n");
			}
		break;
	case 101: // PLL Processing Mode
	        if (gtk_check_button_get_active (GTK_CHECK_BUTTON (widget)))
			// update Tau, then turn on processing
			sranger_common_hwi->write_pll (((DSPPACControl*)dspc)->pll, PLL_TAUPAC);

	        sranger_common_hwi->write_dsp_state((gtk_check_button_get_active (GTK_CHECK_BUTTON (widget))) ? MD_PLL:-MD_PLL);
		break;
        case 200: // Ampl Ctrl Adaptive
	        if (((DSPPACControl*)dspc)->pll.ctrlmode_Amp_adaptive = gtk_check_button_get_active (GTK_CHECK_BUTTON (widget)))
                        g_slist_foreach
				( (GSList*) g_object_get_data( G_OBJECT (widget), "ADA-LIST"),
				  (GFunc) gtk_widget_show, NULL
                                  );
                else
                        g_slist_foreach
				( (GSList*) g_object_get_data( G_OBJECT (widget), "ADA-LIST"),
				  (GFunc) gtk_widget_hide, NULL
                                  );
                break;
	}
        return 0;
}

void DSPPACControl::copy_ref_freq_to_ref_callback (Param_Control* pcs, DSPPACControl *dspc){
        gchar *msg = g_strdup_printf ("Copy res. freq. %g", dspc->pll.Filter64Out[F64_Excitation]);
        main_get_gapp()->monitorcontrol->LogEvent ("DSPPACControl", msg);
        dspc->pll.FSineHz      = dspc->pll.Filter64Out[F64_Excitation];
        dspc->pll.Reference[0] = dspc->pll.Filter64Out[F64_Excitation];
	g_slist_foreach
		( (GSList*) g_object_get_data( G_OBJECT (dspc->window), "PAC_EC_FRQ_list"),
		  (GFunc) App::update_ec, NULL
			);
}


//========

#ifdef PACSCOPE_GTKMM_EXPERIMENTAL
// NEEDS plotmm, and newer gtk libs, etc...

// ======================= SCOPE ==============================

#define BLOCK_LENGTH    10000
const gchar *signal_name[] = { 
    "Filter64Out[0]: ResFreq [Hz]",
    "Filter64Out[1] LP: ResPhase [deg]",
    "Filter64Out[2] LP: ResAmp [mV]",
    "Filter64Out[3] LP: ExecAmp [mV]",
    "Amp. Estimation [mV]",
    "Phase [deg]",
    "SineOut0 [mV]",
    "SignalIn Filt. [mV]",
    "cb_Ic",
    "Zneg",
    "Iiir",
    "Iavg",
    "Irms",
    "delta",
    "Xs",
    "Ys",
    "Zo",
    "Xsr",
    "Ysr",
    "IN0",
    "OUT0",
    "USER-ADDRESS",
    NULL
};
#define USER_ADDRESS 21

const gint64 signal_address[] = {
    0,0,0,0, 0,0,0,0, // use PAC lib sym lookup
    -1
};

const gchar *signal_unit[] = { 
    "Hz",
    "deg",
    "mV",
    "mV",
    "mV",
    "deg",
    "mV",
    "mV",
    "cb_Ic",
    "Zneg V",
    "Iiir V",
    "Iavg V",
    "Irms V",
    "delta V",
    "Xs V",
    "Ys V",
    "Zo V",
    "Xsr V",
    "Ysr V",
    "N/A",
    NULL
};

double f0func(double qf15){
    if (qf15 > 0)
	return (-log (qf15 / 32767.) / (2.*M_PI/75000.)); // f0 in Hz
    else
	return 75000.; // FBW
}

#define CPN(N) ((double)(1LL<<(N))-1.)

double signal_scale_factor[] = { 
    150000./(CPN(29)*2.*M_PI), // Excit. Freq
    180./(CPN(29)*M_PI), // ResPhase
    10e3/CPN(29), // ResAmp LP
    10e3/CPN(29), // ExecAmp LP
    10e3/CPN(29), // amp_est
    180./(CPN(29)*M_PI), // phase
    10e3/CPN(29), // SineOut0
    10e3/CPN(29), // InputFiltered
    (CPN(15))/(2.*M_PI/75000.), // -- cb_Ic --> to get Hz must use f0func
    10./CPN(31), // -- Zneg
    10./CPN(31), // -- Iiir
    10./CPN(31), // -- Iavg
    10./CPN(31), // -- Irms
    10./CPN(31), // -- delta
    10./CPN(31), // -- Xs
    10./CPN(31), // -- Ys
    10./CPN(31), // -- Zo
    10./CPN(31), // -- Xsr
    10./CPN(31), // -- Ysr
    1., // --
    1. // N/A
};


#define CPN(N) ((double)(1LL<<(N))-1.)

static int      timer_intervall = 100;

static gboolean tune         = FALSE;
static gint     tune_fc      = 32766;
static gint     tune_span    = 40;
static gint     tune_res_mhz = 50;
static gint     tune_int_ms  = 150;

static gboolean scope       = FALSE;
static gint     scope_fps   = 20;
static gint     scope_s1    = 1;
static gint     scope_s2    = 2;
static gint     scope_pA1   = 0;
static gint     scope_pA2   = 0;
static gint     scope_ymin1 = -999999;
static gint     scope_ymax1 = -999999;
static gint     scope_ymin2 = -999999;
static gint     scope_ymax2 = -999999;
static gint     scope_len   = 5000;
static gint     scope_inc   = 1;
static gint     scope_level = 0;
static gboolean scope_trigger = FALSE;

static gboolean step       = FALSE;
static gint     step_amp   = 0; // uV
static gint     step_phase = 0; // mDeg

static gint     PACscopeVersion = 0;

class PlotSRSignal  : public AppBase    // : public Gtk::Window
{
public:
    PlotSRSignal();
    virtual ~PlotSRSignal();

    void setup_scope();
    void setup_tune();
    void setup_stepresponse();
    void setup_mode(){
//	if (scope)
//	    setup_scope ();
//	else if (step)  
//	    setup_stepresponse ();
//	else if (tune)
	    setup_tune ();
//	else {
//	    PI_DEBUG_GP (DBG_L3, "PACscope Version " VERSION "\n");
//	    PI_DEBUG_GP (DBG_L3, "Error: please seletced operation mode, one of scope, tune or stepresponse. [s%d,t%d,p%d]\nByby.\n", scope, tune, step);
//	    exit (0);
//	}
    };

    bool timer_callback_scope(){
	static int ton=0;
	static int s1_last=-1;
	static int s2_last=-1;
	static int count=-20;
	double x0= 0.;
	double ex= 100.;
	double y0= 0.;
	double yt= 100.;
	double yb= -100.;

	int num=0;
	
	if (!m_chk_run.get_active()) return 1;
	++loop;
	
	scope_len     = (int)m_HScale_N.get_adjustment()->get_value ();
	double trigger_level = (int)m_HScale_Lv.get_adjustment()->get_value ();
	
	num = scope_len; 
	if (num >= PAC_BLCKLEN) num = PAC_BLCKLEN;
	if (num < 0) num = 1;
	
	if (m_chk_trigger.get_active ())
	    scope_trigger = TRUE;
	else
	    scope_trigger = FALSE;
	
	
	while (sr->read_pll_signal1 (pll, num, signal_scale_factor[scope_s1]));
	while (sr->read_pll_signal2 (pll, num, signal_scale_factor[scope_s2]));

	sr->set_scope (scope_pA1, scope_pA2);
	sr->set_blcklen (scope_len-1);

	// some base line math
	double S1_mean = 0.;
	double S1_rms = 0.;
	double S2_mean = 0.;
	double S2_rms = 0.;
	for (int i=0; i<num; ++i){
	    S1_mean += pll.Signal1[i];
	    S2_mean += pll.Signal2[i];
	}
	S1_mean /= num;
	S2_mean /= num;
	if (m_chk_S1_auto.get_active())
	     S1_scale[0] =  S1_scale[1] = pll.Signal1[0];
	if (m_chk_S2_auto.get_active())
	     S2_scale[0] =  S2_scale[1] = pll.Signal2[0];
	    
	for (int i=0; i<num; ++i){
	    double p = pll.Signal1[i] - S1_mean;
	    S1_rms += p*p;
	    p = pll.Signal2[i] - S2_mean;
	    S2_rms += p*p;
	    if (m_chk_S1_auto.get_active()){
		p = pll.Signal1[i];
		if (p > S1_scale[0])
		    S1_scale[0] = p;
		if (p < S1_scale[1])
		    S1_scale[1] = p;
	    }
	    if (m_chk_S2_auto.get_active()){
		p = pll.Signal2[i];
		if (p > S2_scale[0])
		    S2_scale[0] = p;
		if (p < S2_scale[1])
		    S2_scale[1] = p;
	    }
	}
	S1_rms /= num;
	S2_rms /= num;

	int td = 0;
	double tlv = trigger_level > -10. ? trigger_level : S1_mean;
	if (scope_trigger){
	    int it=num/4;
	    for (; it<3*num/4; ++it)
		if (pll.Signal1[it-1] < tlv && pll.Signal1[it] >= tlv)
		    break;
	    td = it-num/2;
	    for (int i=num/4+td-1; i<=3*num/4+td+1; ++i){
		pll.x_array[num-i-1]=(i-it)/150.; // absolut X
//		pll.x_array_x[num-i-1]=(i-it)/150.;
	    }
	    for (int j=0; j<num/2; ++j)
		pll.Signal1x[j] = 0.95*pll.Signal1x[j] + 0.05*pll.Signal1[num/4+td+j];
	    myCurve1->set_data(&pll.x_array[num/4-td], &pll.Signal1[num/4+td], num/2);
	    myCurve2->set_data(&pll.x_array[num/4-td], &pll.Signal2[num/4+td], num/2);
	    myCurve1x->set_data(&pll.x_array[num/4-td], &pll.Signal1x[0], num/2);
	    myCurve1x->set_enabled(TRUE);
	    if (!ton){
		m_plot.scale(PlotMM::AXIS_BOTTOM)->set_autoscale(false);
		m_plot.scale(PlotMM::AXIS_BOTTOM)->set_range((-num/4.)/150., (num/4.)/150.);
		ton=true;
	    }
	} else {
	    ton = 0;
	    memset (pll.Signal1x, 0, sizeof (pll.Signal1x));
	    for (int i=0; i<num; ++i){
		pll.x_array[num-i-1]=i/150.;
		pll.x_array_x[num-i-1]=i/150.;
	    }
	    myCurve1->set_data(pll.x_array, &pll.Signal1[0], num);
	    myCurve2->set_data(pll.x_array, &pll.Signal2[0], num);
	    myCurve1x->set_enabled(FALSE);
	}
	myCurve1->set_baseline(y0);

	if (scope_s1 != s1_last)
	    m_plot.scale(PlotMM::AXIS_LEFT)->set_enabled(false);
	if (scope_s2 != s2_last)
	    m_plot.scale(PlotMM::AXIS_RIGHT)->set_enabled(false);
	m_plot.scale(PlotMM::AXIS_BOTTOM)->set_enabled(false);

	char tmp[1000];
	sprintf(tmp,"Time [ms] (# %d, step: %d, %d fps, %s [%fms] %f)",
		scope_len, scope_inc, scope_fps, scope_trigger? "Trigger@":"Trigger=Off ", (double)td/150., tlv);
	m_plot.label(PlotMM::AXIS_BOTTOM)->set_text(tmp);
	//	    (0..3: F64[0..3], 4: Amp_est, 5: Phase, 6: SineOut0, 7: In Filt
	sprintf(tmp,"blue: Signal1 [%d: %s]",scope_s1, signal_name[scope_s1>=0 && scope_s1 < USER_ADDRESS? scope_s1:USER_ADDRESS]);
	scope_s1 = scope_s1>=0 && scope_s1 < USER_ADDRESS? scope_s1 : USER_ADDRESS;
	m_plot.label(PlotMM::AXIS_LEFT)->set_text(tmp);
	sprintf(tmp,"red: Signal2 [%d: %s]",scope_s2, signal_name[scope_s2>=0 && scope_s2 < USER_ADDRESS? scope_s2 : USER_ADDRESS]);
	scope_s2 = scope_s2>=0 && scope_s2 < USER_ADDRESS? scope_s2 : USER_ADDRESS;
	m_plot.label(PlotMM::AXIS_RIGHT)->set_text(tmp);


	if (scope_s1 != s1_last){
	    s1_last = scope_s1;
	    m_plot.scale(PlotMM::AXIS_LEFT)->set_enabled(true);
	}
	if (scope_s2 != s2_last){
	    s2_last = scope_s2;
	    m_plot.scale(PlotMM::AXIS_RIGHT)->set_enabled(true);
	}
	m_plot.scale(PlotMM::AXIS_BOTTOM)->set_enabled(true);

	Glib::Timer timer;
	timer.start();

	m_plot.replot();
	timer.stop();
   
	fps_.stop();
	double time= fps_.elapsed();
	fps_.start();
	sprintf(tmp,
		"%.1f fps\n"
		"S1 %12.5f %s mean\n"	
		"S1 %12.5f %s rms\n"
		"S2 %12.5f %s mean\n"
		"S2 %12.5f %s rms\n"
		,1/time,
		S1_mean, signal_unit[scope_s1], S1_rms, signal_unit[scope_s1], 
		S2_mean, signal_unit[scope_s2], S2_rms, signal_unit[scope_s2]);
	l_fps.set_text(tmp);
	print_coords(mx_,my_);
	return 1;
    }

    bool timer_callback_tune(){
	static int count=-20;
	double x0= 0.;
	double ex= 100.;
	double y0= 0.;
	double yt= 100.;
	double yb= -100.;
	double uw = 0.;

	int num=0;

	if (!m_chk_run.get_active()) return 1;
	++loop;    

	int NNN = (int)(tune_span*1000./tune_res_mhz);
	if (NNN > PAC_BLCKLEN) NNN =PAC_BLCKLEN;
	double fspan = tune_span;
	double fi = tune_fc-fspan/2.;
	x0=fi;
	yt=0.;
	y0=0;
	yb=0.;
	ex=fi+fspan;
	if (count >= 0){
	    num = count++;

	    sr->read_pll (pll, PLL_READINGS);
	      
	    pll.x_array[num] = fi + (double)num*fspan/(double)NNN;
	    pll.Signal1[num] = pll.amp_estimation * 1000.;
	    if (num == 0)
		uw = 0.;
	    pll.Signal2[num] = pll.phase + uw;
	    if (num > 1)
		if (fabs (pll.Signal2[num-1] - pll.Signal2[num]) > 180.){
		    pll.Signal2[num] -= uw;
		    if (pll.Signal2[num] > 0)
			uw += -360.;
		    else
			uw += 360.;
		    pll.Signal2[num] += uw;
		}
//	    pll.Signal2[num] = pll.phase < 0. ? pll.phase+360. : pll.phase;
//		while (pll.Signal2[num] > 180.) pll.Signal2[num] -= 360.. 
//		pll.Signal2[num] = pll.phase < 0. ? pll.phase-(180.*floor(pll.phase/-180.)) 
//		    : pll.phase-(180.*floor(pll.phase/180.));
		
	    num++;
	    pll.FSineHz = fi + (double)num*fspan/(double)NNN;
	    sr->write_pll (pll, PLL_SINE);

	    if (count >= NNN)
		count = -20;
	} else {
	    ++count;
	    pll.FSineHz = fi;
	    sr->write_pll (pll, PLL_SINE);
	    char tmp[100];
	    sprintf(tmp,"Starting...\n %d\n %10.3f Hz", count, pll.FSineHz);
	    l_fps.set_text(tmp);
	    return 1;
	}
	myCurve1->set_data(pll.x_array, pll.Signal1, num);
	myCurve2->set_data(pll.x_array, pll.Signal2, num);
	myCurve1->set_baseline(y0);

	Glib::Timer timer;
	timer.start();

	m_plot.replot();
	timer.stop();
   
	fps_.stop();
	double time= fps_.elapsed();
	fps_.start();
	char tmp[100];
	sprintf(tmp,"%.1f fps\n%10.3f Hz\n%.4f mV\n%.2f deg",1/time, pll.FSineHz, pll.Signal1[num-1], pll.Signal2[num-1]);
	l_fps.set_text(tmp);
	print_coords(mx_,my_);
	return 1;
    }
    
    bool timer_callback_step(){
	static int count=-20;
	double x0= 0.;
	double ex= 100.;
	double y0= 0.;
	double yt= 100.;
	double yb= -100.;

	int num=0;

	if (!m_chk_run.get_active()) return 1;
	++loop;

	scope_len = (int)m_HScale_N.get_adjustment()->get_value ();

	num = scope_len; 
	if (num >= PAC_BLCKLEN) num = PAC_BLCKLEN;
	if (num < 0) num = 1;

	while (sr->read_pll_signal1 (pll, num, signal_scale_factor[scope_s1]));
	while (sr->read_pll_signal2 (pll, num, signal_scale_factor[scope_s2]));

	for (int i=0; i<num; ++i){
	    pll.x_array[num-i-1]=i/150.;
	}
	myCurve1->set_data(pll.x_array, pll.Signal1, num);
	myCurve2->set_data(pll.x_array, pll.Signal2, num);
	myCurve1->set_baseline(y0);

	m_plot.scale(PlotMM::AXIS_BOTTOM)->set_enabled(false);
	gchar tmp[1000];
	sprintf(tmp,"Time [ms] (# %d (%g ms), step amp: %d uV, phase: %d mDeg)",
		scope_len, scope_len/150., step_amp, step_phase);
	m_plot.label(PlotMM::AXIS_BOTTOM)->set_text(tmp); 
	m_plot.scale(PlotMM::AXIS_BOTTOM)->set_enabled(true);

	// start next test run

	step_amp   = (int)m_HScale_s_amp.get_adjustment()->get_value ();
	step_phase = (int)m_HScale_s_phase.get_adjustment()->get_value ();

	if (step_phase > 0)
	    sr->setup_step_response ((double)step_phase * 1e-3, 0.);
	else
	    sr->setup_step_response (0., (double)step_amp * 1e-6);

	sr->set_scope (scope_pA1, scope_pA2);
	sr->set_blcklen (scope_len-1);

	Glib::Timer timer;
	timer.start();

	m_plot.replot();
	timer.stop();
   
	fps_.stop();
	double time= fps_.elapsed();
	fps_.start();
	sprintf(tmp,"%.1f fps",1/time);
	l_fps.set_text(tmp);
	print_coords(mx_,my_);
	return 1;
    }

    void setup_signal_chooser (int si, const gchar *list[], Gtk::ComboBoxText *m_ComboBoxText, int pos)
	{
	    for (int i=0; list[i]; ++i)
		m_ComboBoxText->append_text (list[i]);

	    switch (si){
	    case 1: m_ComboBoxText->signal_changed().connect(sigc::mem_fun(*this, &PlotSRSignal::on_combo_position_S1)); break;
	    case 2: m_ComboBoxText->signal_changed().connect(sigc::mem_fun(*this, &PlotSRSignal::on_combo_position_S2)); break;
	    }
	    m_ComboBoxText->set_active_text (list[pos]);
	}

    void on_combo_position_S1 ()
	{
	    Glib::ustring text = m_ComboBoxText_S1.get_active_text();
	    if(!(text.empty())){
		for (int i=0; signal_name[i]; ++i)
		    if (strcmp (signal_name[i], text.c_str()) == 0) scope_s1 = scope_pA1 = i, PI_DEBUG_GP (DBG_L3, "S1[%d]: %s\n", i, text.c_str() );
	    }
	} 

    void on_combo_position_S2 ()
	{
	    Glib::ustring text = m_ComboBoxText_S2.get_active_text();
	    if(!(text.empty())){
		for (int i=0; signal_name[i]; ++i)
		    if (strcmp (signal_name[i], text.c_str()) == 0) scope_s2 = scope_pA2 = i, PI_DEBUG_GP (DBG_L3, "S2[%d]: %s\n", i, text.c_str() );
	    }
	} 

    void on_scale_toggled(PlotMM::PlotAxisID id,const Gtk::CheckButton *chk) 
	{
	    m_plot.scale(id)->set_enabled(chk->get_active());
	    m_plot.label(id)->set_enabled(chk->get_active());
	}

    void on_auto_toggled(PlotMM::PlotAxisID id,const Gtk::CheckButton *chk) 
	{
	    m_plot.scale(id)->set_autoscale(chk->get_active());
	}

    void on_plot_mouse_press(int x,int y, GdkEventButton *ev)
	{
	    if (ev->button>0 && ev->button<4) button_[ev->button-1]= true;
	    print_coords(mx_=x,my_=y);

    
	    if ((ev->button==1)) {  // zoom
		if (ev->state == 0x11){ // Shift -- S1
		    PI_DEBUG_GP (DBG_L3, "pB1 pre S1 Shift zoom: %g, %g, %g, %g\n", S1_scale[0],  S1_scale[1],  S1_scale[2],  S1_scale[3]);
		    S1_scale[2] = (S1_scale[0] + S1_scale[1])/2.;
		    S1_scale[3] = S1_scale[0] - S1_scale[1];
		    S1_scale[0] = S1_scale[2] + S1_scale[3]/4.;
		    S1_scale[1] = S1_scale[2] - S1_scale[3]/4.;
		    PI_DEBUG_GP (DBG_L3, "pB1 post S1 Shift zoom: %g, %g, %g, %g\n", S1_scale[0],  S1_scale[1],  S1_scale[2],  S1_scale[3]);
		    m_plot.scale(PlotMM::AXIS_LEFT)->set_autoscale(false);
		    m_plot.scale(PlotMM::AXIS_LEFT)->set_range(S1_scale[1], S1_scale[0]);
		} else if (ev->state == 0x14){ // Ctrl -- S2
		    S2_scale[2] = (S2_scale[0] + S2_scale[1])/2.;
		    S2_scale[3] = S2_scale[0] - S2_scale[1];
		    S2_scale[0] = S2_scale[2] + S2_scale[3]/4.;
		    S2_scale[1] = S2_scale[2] - S2_scale[3]/4.;
		    m_plot.scale(PlotMM::AXIS_RIGHT)->set_autoscale(false);
		    m_plot.scale(PlotMM::AXIS_RIGHT)->set_range(S2_scale[1], S2_scale[0]);
		} else {
		    m_plot.scale(PlotMM::AXIS_BOTTOM)->set_autoscale(false);
		    m_plot.scale(PlotMM::AXIS_LEFT)->set_autoscale(false);
		    zoomRect_.set_rect(x,y,0,0);
		    m_plot.set_selection(zoomRect_);
		    m_plot.enable_selection();
		}
		PI_DEBUG_GP (DBG_L3, "pB1: %x\n", ev->state);
	    } else if ((ev->button==3)) {  // unzoom
		if (ev->state == 0x11){ // Shift -- S1
		    PI_DEBUG_GP (DBG_L3, "pB1 pre S1 Shift unzoom: %g, %g, %g, %g\n", S1_scale[0],  S1_scale[1],  S1_scale[2],  S1_scale[3]);
		    S1_scale[2] = (S1_scale[0] + S1_scale[1])/2.;
		    S1_scale[3] = S1_scale[0] - S1_scale[1];
		    S1_scale[0] = S1_scale[2] + S1_scale[3];
		    S1_scale[1] = S1_scale[2] - S1_scale[3];
		    PI_DEBUG_GP (DBG_L3, "pB1 post S1 Shift unzoom: %g, %g, %g, %g\n", S1_scale[0],  S1_scale[1],  S1_scale[2],  S1_scale[3]);
		    m_plot.scale(PlotMM::AXIS_LEFT)->set_autoscale(false);
		    m_plot.scale(PlotMM::AXIS_LEFT)->set_range(S1_scale[1], S1_scale[0]);
		} else if (ev->state == 0x14){ // Ctrl -- S2
		    S2_scale[2] = (S2_scale[0] + S2_scale[1])/2.;
		    S2_scale[3] = S2_scale[0] - S2_scale[1];
		    S2_scale[0] = S2_scale[2] + S2_scale[3];
		    S2_scale[1] = S2_scale[2] - S2_scale[3];
		    m_plot.scale(PlotMM::AXIS_RIGHT)->set_autoscale(false);
		    m_plot.scale(PlotMM::AXIS_RIGHT)->set_range(S2_scale[1], S2_scale[0]);
		} else {
		    m_plot.scale(PlotMM::AXIS_BOTTOM)->set_autoscale(true);
		    m_plot.scale(PlotMM::AXIS_LEFT)->set_autoscale(true);
		}
		PI_DEBUG_GP (DBG_L3, "pB3: %x\n", ev->state);
	    } else if ((ev->button==4)) {  // manual Yscale Y-S1
		PI_DEBUG_GP (DBG_L3, "pB4: %x\n", ev->state);
	    } else if ((ev->button==2)) {  // manual Yscale Y-S1
		PI_DEBUG_GP (DBG_L3, "pB2: %x\n", ev->state);
	    } else if ((ev->button==5)) {  // manual Yscale Y-S1
		PI_DEBUG_GP (DBG_L3, "pB5: %x\n", ev->state);
	    }
	    m_plot.replot();
	}

    void on_plot_mouse_release(int x,int y, GdkEventButton *ev)
	{
	    if ((ev->button==4)) {  // manual Yscale Y-S1
		PI_DEBUG_GP (DBG_L3, "rB4: %x\n", ev->state);
	    } else if ((ev->button==2)) {  // manual Yscale Y-S1
		PI_DEBUG_GP (DBG_L3, "rB2: %x\n", ev->state);
	    } else if ((ev->button==5)) {  // manual Yscale Y-S1
		PI_DEBUG_GP (DBG_L3, "rB5: %x\n", ev->state);
	    }

	    if (ev->button>0 && ev->button<4) button_[ev->button-1]= false;
	    print_coords(mx_=x,my_=y);

	    if ((ev->button==1)) {
		double x0,y0,x1,y1;
		int ix0,iy0,ix1,iy1;
		zoomRect_= m_plot.get_selection();
		ix0= zoomRect_.get_x_min(); 
		ix1= zoomRect_.get_x_max();
		iy0= zoomRect_.get_y_min(); 
		iy1= zoomRect_.get_y_max();
		x0= m_plot.scale(PlotMM::AXIS_BOTTOM)->scale_map().inv_transform(ix0);
		x1= m_plot.scale(PlotMM::AXIS_BOTTOM)->scale_map().inv_transform(ix1);
		y0= m_plot.scale(PlotMM::AXIS_LEFT)->scale_map().inv_transform(iy1);
		y1= m_plot.scale(PlotMM::AXIS_LEFT)->scale_map().inv_transform(iy0);
		m_plot.disable_selection();
		if (zoomRect_.get_width()==0 && zoomRect_.get_height()==0)
		    return;
		m_plot.scale(PlotMM::AXIS_BOTTOM)->set_range(x0,x1);
		m_plot.scale(PlotMM::AXIS_LEFT)->set_range(y0,y1);
		m_plot.replot();
	    }
	}

    void on_plot_mouse_move(int x,int y, GdkEventMotion *ev)
	{
	    print_coords(mx_=x,my_=y);
	    zoomRect_.set_destination(x,y);
	    m_plot.set_selection(zoomRect_);
	}

    void print_coords(int x, int y)
	{
	    char tmp[1000];
	    sprintf(tmp,"(%d,%d): b:%12.5f; t:%12.5f; l:%12.5f; r:%12.5f; %c%c%c",x,y,
		    m_plot.scale(PlotMM::AXIS_BOTTOM)->scale_map().inv_transform(x),
		    m_plot.scale(PlotMM::AXIS_TOP)->scale_map().inv_transform(x),
		    m_plot.scale(PlotMM::AXIS_LEFT)->scale_map().inv_transform(y),
		    m_plot.scale(PlotMM::AXIS_RIGHT)->scale_map().inv_transform(y),
		    button_[0]?'*':'O',button_[1]?'*':'O',button_[2]?'*':'O');
	    m_sb.pop();
	    if (x>-1000&&y>-1000)
		m_sb.push(tmp);
	}
 
protected:
    bool button_[3];
    int mx_,my_;
    PlotMM::Rectangle zoomRect_;
    // Child widgets
    Gtk::Label l_fps;
    Gtk::CheckButton m_chk_l;
    Gtk::CheckButton m_chk_r;
    Gtk::CheckButton m_chk_t;
    Gtk::CheckButton m_chk_b;
    Gtk::CheckButton m_chk_run;
    Gtk::CheckButton m_chk_trigger;
    Gtk::CheckButton m_chk_S1_auto;
    Gtk::CheckButton m_chk_S2_auto;
    Gtk::HScale m_HScale_N;
    Gtk::HScale m_HScale_Lv;
    Gtk::ComboBoxText m_ComboBoxText_S1;
    Gtk::ComboBoxText m_ComboBoxText_S2;
    Gtk::HScale m_HScale_s_amp;
    Gtk::HScale m_HScale_s_phase;
    Gtk::VBox m_box0;
    Gtk::HBox m_box1;
    Gtk::VBox m_box2;
    Gtk::VBox m_box3; //empty box

    Gtk::Button m_button1, m_button2;
    Gtk::Statusbar m_sb;

//    Glib::RefPtr<PlotMM::ErrorCurve> myCurve0;
    Glib::RefPtr<PlotMM::Curve> myCurve1;
    Glib::RefPtr<PlotMM::Curve> myCurve2;
    Glib::RefPtr<PlotMM::Curve> myCurve1x;
    Glib::RefPtr<PlotMM::Curve> myCurve2x;

    PlotMM::Plot m_plot;
    Glib::Timer fps_;
    int loop;
    int cs;
    double S1_scale[4];
    double S2_scale[4];

    sranger_common_hwi_dev* sr;
    PAC_control pll;
};

void PlotSRSignal::setup_scope(){
	gchar tmp[1000];

	timer_intervall = 1000/scope_fps;

	m_plot.title()->set_text("PAC/PLL/Gxsm Scope");
	m_plot.title()->set_enabled(true);
	m_plot.label(PlotMM::AXIS_TOP)->set_text("top axis");

	sr->set_scope (scope_pA1, scope_pA2);
	sr->set_blcklen (scope_len-1);
 
	m_chk_trigger.set_active(scope_trigger);

	sprintf(tmp,"Time [ms] (# %d, step: %d, %d fps, %s %d)",
		scope_len, scope_inc, scope_fps, scope_trigger? "Trigger@":"Trigger=Off ", scope_level);
	m_plot.label(PlotMM::AXIS_BOTTOM)->set_text(tmp);
//	    (0..3: F64[0..3], 4: Amp_est, 5: Phase, 6: SineOut0, 7: In Filt
	sprintf(tmp,"blue: Signal1 [%d: %s]",scope_s1, signal_name[scope_s1>=0 && scope_s1 < USER_ADDRESS? scope_s1 : USER_ADDRESS]);
	scope_s1 = scope_s1>=0 && scope_s1 < USER_ADDRESS? scope_s1 : USER_ADDRESS;
	m_plot.label(PlotMM::AXIS_LEFT)->set_text(tmp);
	sprintf(tmp,"red: Signal2 [%d: %s]",scope_s2, signal_name[scope_s2>=0 && scope_s2 < USER_ADDRESS? scope_s2 : USER_ADDRESS]);
	scope_s2 = scope_s2>=0 && scope_s2 < USER_ADDRESS? scope_s2 : USER_ADDRESS;
	m_plot.label(PlotMM::AXIS_RIGHT)->set_text(tmp);

	/* create some named curves with different colors and symbols */
	myCurve1= Glib::RefPtr<PlotMM::Curve>(new PlotMM::Curve("Signal 1"));
	myCurve2= Glib::RefPtr<PlotMM::Curve>(new PlotMM::Curve("Signal 2"));
	myCurve1x= Glib::RefPtr<PlotMM::Curve>(new PlotMM::Curve("Signal 1x"));
	myCurve2x= Glib::RefPtr<PlotMM::Curve>(new PlotMM::Curve("Signal 2x"));
	myCurve1->paint()->set_pen_color(Gdk::Color("blue"));
	myCurve1x->paint()->set_pen_color(Gdk::Color("purple"));
	myCurve2->paint()->set_pen_color(Gdk::Color("red"));
	myCurve2x->paint()->set_pen_color(Gdk::Color("yellow"));
	m_plot.add_curve(myCurve1,PlotMM::AXIS_BOTTOM,PlotMM::AXIS_LEFT);
	m_plot.add_curve(myCurve1x,PlotMM::AXIS_BOTTOM,PlotMM::AXIS_LEFT);
	m_plot.add_curve(myCurve2,PlotMM::AXIS_BOTTOM,PlotMM::AXIS_RIGHT);
	m_plot.add_curve(myCurve2x,PlotMM::AXIS_BOTTOM,PlotMM::AXIS_RIGHT);
    
	m_chk_run.set_active(true);

	PI_DEBUG_GP (DBG_L3, "Timeout setup.\n");

#if (GTKMM_MAJOR_VERSION == 2 && GTKMM_MINOR_VERSION >= 4)
	Glib::signal_timeout().connect(sigc::mem_fun(*this, &PlotSRSignal::timer_callback_scope),
				       timer_intervall,Glib::PRIORITY_DEFAULT_IDLE);
#else
	Glib::signal_timeout().connect(SigC::slot(*this, &PlotSRSignal::timer_callback_scope),
				       timer_intervall,Glib::PRIORITY_DEFAULT_IDLE);
#endif
}

void PlotSRSignal::setup_tune(){
	gchar tmp[1000];

	timer_intervall = tune_int_ms;

	m_plot.title()->set_text("PAC/PLL Tuning Scope");
	m_plot.title()->set_enabled(true);
	m_plot.label(PlotMM::AXIS_TOP)->set_text("top axis");

	sprintf(tmp,"Frequency [Hz] (Tune @Fc= %d Hz, span= %d Hz, step= %d mHz, int= %d ms)",
		tune_fc, tune_span, tune_res_mhz, tune_int_ms);
	m_plot.label(PlotMM::AXIS_BOTTOM)->set_text(tmp); 
	m_plot.label(PlotMM::AXIS_LEFT)->set_text("blue: Amplitude [mV]");
	m_plot.label(PlotMM::AXIS_RIGHT)->set_text("red: Phase [deg]");
	
	/* create some named curves with different colors and symbols */
	myCurve1= Glib::RefPtr<PlotMM::Curve>(new PlotMM::Curve("Signal 1"));
	myCurve2= Glib::RefPtr<PlotMM::Curve>(new PlotMM::Curve("Signal 2"));
	myCurve1->paint()->set_pen_color(Gdk::Color("blue"));
	m_plot.add_curve(myCurve1,PlotMM::AXIS_BOTTOM,PlotMM::AXIS_LEFT);
	m_plot.add_curve(myCurve2,PlotMM::AXIS_BOTTOM,PlotMM::AXIS_RIGHT);
	myCurve2->paint()->set_pen_color(Gdk::Color("red"));
    
	m_chk_run.set_active(true);
#if (GTKMM_MAJOR_VERSION == 2 && GTKMM_MINOR_VERSION >= 4)
	Glib::signal_timeout().connect(sigc::mem_fun(*this, &PlotSRSignal::timer_callback_tune),
				       timer_intervall,Glib::PRIORITY_DEFAULT_IDLE);
#else
	Glib::signal_timeout().connect(SigC::slot(*this, &PlotSRSignal::timer_callback_tune),
				       timer_intervall,Glib::PRIORITY_DEFAULT_IDLE);
#endif
}

void PlotSRSignal::setup_stepresponse(){
	gchar tmp[1000];

	timer_intervall = 2*scope_len/75;

	m_plot.title()->set_text("PAC/PLL Controller Closed-Loop Step Response Measurement");
	m_plot.title()->set_enabled(true);
	m_plot.label(PlotMM::AXIS_TOP)->set_text("top axis");

	sprintf(tmp,"Time [ms] (# %d (%g ms), step amp: %d uV, phase: %d mDeg)",
		scope_len, scope_len/150., step_amp, step_phase);
	m_plot.label(PlotMM::AXIS_BOTTOM)->set_text(tmp); 

	if (step_phase > 0){
	    m_plot.title()->set_text("PAC/PLL Controller Closed-Loop Step Response Measurement - Phase");
	    sr->setup_step_response ((double)step_phase * 1e-3, 0.);
	    scope_s1 = 1;
	    scope_s2 = 0;
	} else {
	    m_plot.title()->set_text("PAC/PLL Controller Closed-Loop Step Response Measurement - Amplitude");
	    sr->setup_step_response (0., (double)step_amp * 1e-6);
	    scope_s1 = 2;
	    scope_s2 = 3;
	}
	m_plot.title()->set_enabled(true);
	m_plot.label(PlotMM::AXIS_TOP)->set_text("top axis");

	sr->set_scope (scope_pA1, scope_pA2);
	sr->set_blcklen (scope_len-1);

	sprintf(tmp,"blue: Signal1 [%d: %s]",scope_s1, signal_name[scope_s1>=0 && scope_s1<8? scope_s1:8]);
	scope_s1 = scope_s1>=0 && scope_s1<8? scope_s1:8;
	m_plot.label(PlotMM::AXIS_LEFT)->set_text(tmp);
	sprintf(tmp,"red: Signal2 [%d: %s]",scope_s2, signal_name[scope_s2>=0 && scope_s2<8? scope_s2:8]);
	scope_s2 = scope_s2>=0 && scope_s2<8? scope_s2:8;
	m_plot.label(PlotMM::AXIS_RIGHT)->set_text(tmp);

	/* create some named curves with different colors and symbols */
	myCurve1= Glib::RefPtr<PlotMM::Curve>(new PlotMM::Curve("Signal 1"));
	myCurve2= Glib::RefPtr<PlotMM::Curve>(new PlotMM::Curve("Signal 2"));
	myCurve1->paint()->set_pen_color(Gdk::Color("blue"));
	m_plot.add_curve(myCurve1,PlotMM::AXIS_BOTTOM,PlotMM::AXIS_LEFT);
	m_plot.add_curve(myCurve2,PlotMM::AXIS_BOTTOM,PlotMM::AXIS_RIGHT);
	myCurve2->paint()->set_pen_color(Gdk::Color("red"));
    
	m_chk_run.set_active(true);
#if (GTKMM_MAJOR_VERSION == 2 && GTKMM_MINOR_VERSION >= 4)
	Glib::signal_timeout().connect(sigc::mem_fun(*this, &PlotSRSignal::timer_callback_step),
				       timer_intervall,Glib::PRIORITY_DEFAULT_IDLE);
#else
	Glib::signal_timeout().connect(SigC::slot(*this, &PlotSRSignal::timer_callback_step),
				       timer_intervall,Glib::PRIORITY_DEFAULT_IDLE);
#endif

}


PlotSRSignal::PlotSRSignal() :
    mx_(-1000),my_(-1000),
    l_fps("",Gtk::ALIGN_RIGHT),
    m_chk_l("Left"),
    m_chk_r("Right"),
    m_chk_t("Top"),
    m_chk_b("Bottom"),
    m_chk_run("Run/Stop"),
    m_chk_trigger("Trigger On/Off"),
    m_chk_S1_auto("S1 auto"),
    m_chk_S2_auto("S2 auto"),
    m_HScale_N(100., 999999., 100.),
    m_HScale_Lv(-100., 100., 0.01),
    m_HScale_s_amp(0., 1e6, 100.),
    m_HScale_s_phase(0., 15000., 100.),
    m_ComboBoxText_S1(),
    m_ComboBoxText_S2(),
    m_box0(/*homogeneous*/false, /*spacing*/5), 
    m_box1(false, 5), m_box2(false, 5), m_box3(false, 5),
    m_button1("Dialog"), m_button2("Quit"), 
    m_sb(), 
    m_plot(),
    loop(0)
{
//    sr->magic_data.version;
//    sr->magic_data.year;
//    sr->magic_data.mmdd;

	PI_DEBUG_GP (DBG_L3, "  PlotSRSignal::PlotSRSignal  1\n");

	AppWindowInit("PACscope/Tune");

	Glib::RefPtr<Gtk::Application> mmapp =
		Gtk::Application::create(argc, argv,
					 "org.gtkmm.examples.base");


//    set_title("PACscope " VERSION);

    pll.Signal1   = g_new (double, PAC_BLCKLEN); 
    pll.Signal1x  = g_new (double, PAC_BLCKLEN); 
    pll.Signal2   = g_new (double, PAC_BLCKLEN); 
    pll.Signal2x  = g_new (double, PAC_BLCKLEN); 
    pll.tmp_array = g_new (gint32, 2*PAC_BLCKLEN); 
    pll.x_array   = g_new (double, PAC_BLCKLEN); 
    pll.x_array_x  = g_new (double, PAC_BLCKLEN); 

//    sr->read_pll_symbols ();
//    sr->read_pll (pll, PLL_ALL);
//    sr->read_pll (pll, PLL_READINGS);


	PI_DEBUG_GP (DBG_L3, "  PlotSRSignal::PlotSRSignal  3\n");

    
    m_HScale_N.get_adjustment()->set_value ((double)scope_len);
    m_HScale_N.set_digits(0);
    m_HScale_N.set_value_pos(Gtk::POS_TOP);
    m_HScale_N.set_draw_value();
    m_HScale_N.set_size_request(200, 30);

    m_HScale_Lv.get_adjustment()->set_value ((double)0.);
    m_HScale_Lv.set_digits(3);
    m_HScale_Lv.set_value_pos(Gtk::POS_TOP);
    m_HScale_Lv.set_draw_value();
    m_HScale_Lv.set_size_request(200, 30);

    m_HScale_s_amp.get_adjustment()->set_value ((double)step_amp);
    m_HScale_s_amp.set_digits(0);
    m_HScale_s_amp.set_value_pos(Gtk::POS_TOP);
    m_HScale_s_amp.set_draw_value();
    m_HScale_s_amp.set_size_request(200, 30);

    m_HScale_s_phase.get_adjustment()->set_value ((double)step_phase);
    m_HScale_s_phase.set_digits(0);
    m_HScale_s_phase.set_value_pos(Gtk::POS_TOP);
    m_HScale_s_phase.set_draw_value();
    m_HScale_s_phase.set_size_request(200, 30);

    fps_.start();
    button_[0]= button_[1]= button_[2]= false;
    // box2
#if (GTKMM_MAJOR_VERSION == 2 && GTKMM_MINOR_VERSION >= 4)
//    m_button2.signal_clicked().connect(sigc::mem_fun(*this, &Gtk::Widget::hide));
#else
//    m_button2.signal_clicked().connect(SigC::slot(*this, &Gtk::Widget::hide));
#endif
    m_box3.pack_start(m_chk_t, Gtk::PACK_SHRINK,5);
    m_box3.pack_start(m_chk_l, Gtk::PACK_SHRINK,5);
    m_box3.pack_start(m_chk_r, Gtk::PACK_SHRINK,5);
    m_box3.pack_start(m_chk_b, Gtk::PACK_SHRINK,5);
    m_box3.pack_start(l_fps, Gtk::PACK_SHRINK,5);
    m_box2.pack_start(m_box3, Gtk::PACK_EXPAND_WIDGET, /*padding*/5);
    //m_box2.pack_start(m_button1, Gtk::PACK_SHRINK, 5);
    m_box2.pack_start(m_chk_run, Gtk::PACK_SHRINK, 5);
    if (scope){
	m_box2.pack_start(m_chk_trigger, Gtk::PACK_SHRINK, 5);
	m_box2.pack_start(m_HScale_Lv, Gtk::PACK_SHRINK, 5);
    }
    m_box2.pack_start(m_chk_S1_auto, Gtk::PACK_SHRINK, 5);
    m_box2.pack_start(m_chk_S2_auto, Gtk::PACK_SHRINK, 5);

    m_box2.pack_start(m_HScale_N, Gtk::PACK_SHRINK, 5);
    if (step & step_amp > 0)
	m_box2.pack_start(m_HScale_s_amp, Gtk::PACK_SHRINK, 5);
    if (step & step_phase > 0)
	m_box2.pack_start(m_HScale_s_phase, Gtk::PACK_SHRINK, 5);

    if (scope){
	setup_signal_chooser (1, signal_name, &m_ComboBoxText_S1, scope_s1);
	setup_signal_chooser (2, signal_name, &m_ComboBoxText_S2, scope_s2);
	m_box2.pack_start(m_ComboBoxText_S1, Gtk::PACK_SHRINK, 5);
	m_box2.pack_start(m_ComboBoxText_S2, Gtk::PACK_SHRINK, 5);
    }


    // quit
    m_box2.pack_start(m_button2, Gtk::PACK_SHRINK, 5);

    // box1
    m_plot.set_size_request(500, 300);
    m_box1.pack_start(m_plot, Gtk::PACK_EXPAND_WIDGET, 5);
    m_box1.pack_start(m_box2, Gtk::PACK_SHRINK, 5);
    
    // box0
    m_box0.pack_start(m_box1, Gtk::PACK_EXPAND_WIDGET, 5);
    m_box0.pack_start(m_sb, Gtk::PACK_SHRINK, 5);
  
//    set_border_width(0);
//    add(m_box0);

    S1_scale[0] = 1.;
    S1_scale[1] = -1.;
    S1_scale[2] = 0.;
    S2_scale[0] = 1.;
    S2_scale[1] = -1.;
    S2_scale[2] = 0.;

    sr = sranger_common_hwi;

	PI_DEBUG_GP (DBG_L3, "  PlotSRSignal::PlotSRSignal  2\n");

#if 0
    sr = new sranger_mk3_hwi_dev();
    if (sr->get_mark_id() != 3){
	delete sr;
	sr = new sranger_mk2_hwi_dev();
	if (sr->get_mark_id() != 2){
	    PI_DEBUG_GP (DBG_L3, "Con not identify SignalRanger MK3 or MK2 with suitable DSP code running.\nMake sure it's connected and the proper DSP code is flashed and running.\nByBy\n");
	    exit (0);
	}
	PI_DEBUG_GP (DBG_L3, "DSP MK2 detected -- only recorder/scope mode, forcing.\n");
	scope = TRUE;
	step  = tune = FALSE; 
    }
#endif
    tune = TRUE;
    step  = scope = FALSE; 

    scope_pA1   = scope_s1;
    scope_pA2   = scope_s2;
    if (scope_s1 < 0) scope_s1 = USER_ADDRESS;
    if (scope_s2 < 0) scope_s2 = USER_ADDRESS;

    pll.Signal1   = g_new (double, PAC_BLCKLEN); 
    pll.Signal1x  = g_new (double, PAC_BLCKLEN); 
    pll.Signal2   = g_new (double, PAC_BLCKLEN); 
    pll.Signal2x  = g_new (double, PAC_BLCKLEN); 
    pll.tmp_array = g_new (gint32, 2*PAC_BLCKLEN); 
    pll.x_array   = g_new (double, PAC_BLCKLEN); 
    pll.x_array_x  = g_new (double, PAC_BLCKLEN); 

//    sr->read_pll_symbols ();
//    sr->read_pll (pll, PLL_ALL);
//    sr->read_pll (pll, PLL_READINGS);


	PI_DEBUG_GP (DBG_L3, "  PlotSRSignal::PlotSRSignal  3\n");

    
    m_HScale_N.get_adjustment()->set_value ((double)scope_len);
    m_HScale_N.set_digits(0);
    m_HScale_N.set_value_pos(Gtk::POS_TOP);
    m_HScale_N.set_draw_value();
    m_HScale_N.set_size_request(200, 30);

    m_HScale_Lv.get_adjustment()->set_value ((double)0.);
    m_HScale_Lv.set_digits(3);
    m_HScale_Lv.set_value_pos(Gtk::POS_TOP);
    m_HScale_Lv.set_draw_value();
    m_HScale_Lv.set_size_request(200, 30);

    m_HScale_s_amp.get_adjustment()->set_value ((double)step_amp);
    m_HScale_s_amp.set_digits(0);
    m_HScale_s_amp.set_value_pos(Gtk::POS_TOP);
    m_HScale_s_amp.set_draw_value();
    m_HScale_s_amp.set_size_request(200, 30);

    m_HScale_s_phase.get_adjustment()->set_value ((double)step_phase);
    m_HScale_s_phase.set_digits(0);
    m_HScale_s_phase.set_value_pos(Gtk::POS_TOP);
    m_HScale_s_phase.set_draw_value();
    m_HScale_s_phase.set_size_request(200, 30);

    fps_.start();
    button_[0]= button_[1]= button_[2]= false;
    // box2
#if (GTKMM_MAJOR_VERSION == 2 && GTKMM_MINOR_VERSION >= 4)
//    m_button2.signal_clicked().connect(sigc::mem_fun(*this, &Gtk::Widget::hide));
#else
//    m_button2.signal_clicked().connect(SigC::slot(*this, &Gtk::Widget::hide));
#endif
    m_box3.pack_start(m_chk_t, Gtk::PACK_SHRINK,5);
    m_box3.pack_start(m_chk_l, Gtk::PACK_SHRINK,5);
    m_box3.pack_start(m_chk_r, Gtk::PACK_SHRINK,5);
    m_box3.pack_start(m_chk_b, Gtk::PACK_SHRINK,5);
    m_box3.pack_start(l_fps, Gtk::PACK_SHRINK,5);
    m_box2.pack_start(m_box3, Gtk::PACK_EXPAND_WIDGET, /*padding*/5);
    //m_box2.pack_start(m_button1, Gtk::PACK_SHRINK, 5);
    m_box2.pack_start(m_chk_run, Gtk::PACK_SHRINK, 5);
    if (scope){
	m_box2.pack_start(m_chk_trigger, Gtk::PACK_SHRINK, 5);
	m_box2.pack_start(m_HScale_Lv, Gtk::PACK_SHRINK, 5);
    }
    m_box2.pack_start(m_chk_S1_auto, Gtk::PACK_SHRINK, 5);
    m_box2.pack_start(m_chk_S2_auto, Gtk::PACK_SHRINK, 5);

    m_box2.pack_start(m_HScale_N, Gtk::PACK_SHRINK, 5);
    if (step & step_amp > 0)
	m_box2.pack_start(m_HScale_s_amp, Gtk::PACK_SHRINK, 5);
    if (step & step_phase > 0)
	m_box2.pack_start(m_HScale_s_phase, Gtk::PACK_SHRINK, 5);

    if (scope){
	setup_signal_chooser (1, signal_name, &m_ComboBoxText_S1, scope_s1);
	setup_signal_chooser (2, signal_name, &m_ComboBoxText_S2, scope_s2);
	m_box2.pack_start(m_ComboBoxText_S1, Gtk::PACK_SHRINK, 5);
	m_box2.pack_start(m_ComboBoxText_S2, Gtk::PACK_SHRINK, 5);
    }


    // quit
    m_box2.pack_start(m_button2, Gtk::PACK_SHRINK, 5);

    // box1
    m_plot.set_size_request(500, 300);
    m_box1.pack_start(m_plot, Gtk::PACK_EXPAND_WIDGET, 5);
    m_box1.pack_start(m_box2, Gtk::PACK_SHRINK, 5);
    
    // box0
    m_box0.pack_start(m_box1, Gtk::PACK_EXPAND_WIDGET, 5);
    m_box0.pack_start(m_sb, Gtk::PACK_SHRINK, 5);
  
//    set_border_width(0);
//    add(m_box0);

    gtk_box_pack_start (GTK_BOX (vbox), GTK_WIDGET( m_box0.gobj() ), TRUE, TRUE, 0);
    m_box0.show_all();

#if 0
#if (GTKMM_MAJOR_VERSION == 2 && GTKMM_MINOR_VERSION >= 4)
    m_plot.signal_plot_mouse_press().
	connect(sigc::mem_fun(*this,&PlotSRSignal::on_plot_mouse_press));
    m_plot.signal_plot_mouse_release().
	connect(sigc::mem_fun(*this,&PlotSRSignal::on_plot_mouse_release));
    m_plot.signal_plot_mouse_move().
	connect(sigc::mem_fun(*this,&PlotSRSignal::on_plot_mouse_move));
    m_button2.signal_clicked().connect(sigc::mem_fun(*this, &Gtk::Widget::hide));
#else
    m_plot.signal_plot_mouse_press().
	connect(SigC::slot(*this,&PlotSRSignal::on_plot_mouse_press));
    m_plot.signal_plot_mouse_release().
	connect(SigC::slot(*this,&PlotSRSignal::on_plot_mouse_release));
    m_plot.signal_plot_mouse_move().
	connect(SigC::slot(*this,&PlotSRSignal::on_plot_mouse_move));
    m_button2.signal_clicked().connect(SigC::slot(*this, &Gtk::Widget::hide));
#endif
#endif

#if (GTKMM_MAJOR_VERSION == 2 && GTKMM_MINOR_VERSION >= 4)
#define CHECKBOX_SCALE_HELPER(AXIS,CHECKBOX)				\
    m_plot.scale(AXIS)->signal_enabled.connect(sigc::mem_fun(CHECKBOX,&Gtk::CheckButton::set_active)); \
    CHECKBOX.signal_toggled().connect(sigc::bind(sigc::mem_fun(*this,&PlotSRSignal::on_scale_toggled),AXIS,&CHECKBOX)); \
    m_plot.scale(AXIS)->set_enabled(true)
#else
#define CHECKBOX_SCALE_HELPER(AXIS,CHECKBOX)				\
    m_plot.scale(AXIS)->signal_enabled.connect(SigC::slot(CHECKBOX,&Gtk::CheckButton::set_active)); \
    CHECKBOX.signal_toggled().connect(SigC::bind(SigC::slot(*this,&PlotSRSignal::on_scale_toggled),AXIS,&CHECKBOX)); \
    m_plot.scale(AXIS)->set_enabled(true)
#endif

    CHECKBOX_SCALE_HELPER(PlotMM::AXIS_TOP,m_chk_t);
    CHECKBOX_SCALE_HELPER(PlotMM::AXIS_BOTTOM,m_chk_b);
    CHECKBOX_SCALE_HELPER(PlotMM::AXIS_LEFT,m_chk_l);
    CHECKBOX_SCALE_HELPER(PlotMM::AXIS_RIGHT,m_chk_r);
#undef CHECKBOX_SCALE_HELPER

#if (GTKMM_MAJOR_VERSION == 2 && GTKMM_MINOR_VERSION >= 4)
#define CHECKBOX_SCALE_HELPER(AXIS,CHECKBOX)				\
    m_plot.scale(AXIS)->signal_enabled.connect(sigc::mem_fun(CHECKBOX,&Gtk::CheckButton::set_active)); \
    CHECKBOX.signal_toggled().connect(sigc::bind(sigc::mem_fun(*this,&PlotSRSignal::on_auto_toggled),AXIS,&CHECKBOX));
#else
#define CHECKBOX_SCALE_HELPER(AXIS,CHECKBOX)				\
    m_plot.scale(AXIS)->signal_enabled.connect(SigC::slot(CHECKBOX,&Gtk::CheckButton::set_active)); \
    CHECKBOX.signal_toggled().connect(SigC::bind(SigC::slot(*this,&PlotSRSignal::on_auto_toggled),AXIS,&CHECKBOX));
#endif

    m_chk_S1_auto.set_active (TRUE);
    m_chk_S2_auto.set_active (TRUE);
    CHECKBOX_SCALE_HELPER(PlotMM::AXIS_LEFT,m_chk_S1_auto);
    CHECKBOX_SCALE_HELPER(PlotMM::AXIS_RIGHT,m_chk_S2_auto);
#undef CHECKBOX_SCALE_HELPER

    /* set some axes to linear and others to logarithmic scale */
    m_plot.scale(PlotMM::AXIS_BOTTOM)->set_range(-10,1000,false);
//  m_plot.scale(PlotMM::AXIS_TOP)->set_range(10,1000,true);
    if (scope_ymin1 == -999999 && scope_ymax1 == -999999){
	m_plot.scale(PlotMM::AXIS_LEFT)->set_range(-100.,100.,false);
	m_plot.scale(PlotMM::AXIS_LEFT)->set_autoscale(true);
    } else {
	m_plot.scale(PlotMM::AXIS_LEFT)->set_range(scope_ymin1,scope_ymax1, false);
    }
    if (scope_ymin2 == -999999 && scope_ymax2 == -999999){
	m_plot.scale(PlotMM::AXIS_RIGHT)->set_range(-180., 180.,false);
	m_plot.scale(PlotMM::AXIS_RIGHT)->set_autoscale(true);
    } else {
	m_plot.scale(PlotMM::AXIS_RIGHT)->set_range(scope_ymin2, scope_ymax2, false);
    }
    /* allow for autoscaling on all axes */
//  m_plot.scale(PlotMM::AXIS_TOP)->set_autoscale(true);
    m_plot.scale(PlotMM::AXIS_BOTTOM)->set_autoscale(true);


    PI_DEBUG_GP (DBG_L3, "GUI build, setting mode.\n");

    /* set a plot title and some axis labels */
    setup_mode ();

    PI_DEBUG_GP (DBG_L3, "running.\n");
}


PlotSRSignal::~PlotSRSignal(){}

#if 0
//----------------------------------------------------
int scalediv(int argc, char** argv)
{
    int i,k;
    PlotMM::ScaleDiv sd;

    sd.rebuild(0.0064, 1012, 10, 10, true, 0.0);

    k=0;
    for (i=0;i<sd.maj_count();i++) {
	while(k < sd.min_count()) {
	    if (sd.min_mark(k) < sd.maj_mark(i)) {
		std::cout << " - " << sd.min_mark(k) << "\n";
		k++;
	    } else break;
	}
	std::cout << "-- " << sd.maj_mark(i) << "\n";
    }
    while(k < sd.min_count()) {
	std::cout << " - " << sd.min_mark(k) << "\n";
	k++;
    }
    return 0;
}

int plot(int argc, char** argv) 
{
    Gtk::Main main_instance (argc,argv);
    PlotSRSignal plot;
    Gtk::Main::run(plot);
    return 0;
}
#endif



int DSPPACControl::tune_callback (GtkWidget *widget, DSPPACControl *dspc){
	static PlotSRSignal *tune = NULL;

	int m = GPOINTER_TO_INT (g_object_get_data( G_OBJECT (widget), "TUNE_START"));
	PI_DEBUG_GP (DBG_L3, " DSPPACControl::tune_callback  1\n");

//	Gtk::Main main_instance ();
//	PI_DEBUG_GP (DBG_L3, " DSPPACControl::tune_callback  2\n");
//	PlotSRSignal plot;
//	PI_DEBUG_GP (DBG_L3, " DSPPACControl::tune_callback  3\n");
//	Gtk::Main::run(plot);
//	PI_DEBUG_GP (DBG_L3, " DSPPACControl::tune_callback  4\n");

	if (!tune){
		tune = new PlotSRSignal;
	}
}

#endif
