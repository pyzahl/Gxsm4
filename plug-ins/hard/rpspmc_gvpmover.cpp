/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 8 c-style: "K&R" -*- */

/* Gnome gxsm - Gnome X Scanning Microscopy
 * universal STM/AFM/SARLS/SPALEED/... controlling and
 * data analysis software
 *
 * Gxsm Plugin Name: rpspmc_gvpmover.cpp
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
% PlugInDocuCaption: RPSPMC PACPLL GVP Mover
% PlugInName: rpspmc_pacpll
% PlugInAuthor: Percy Zahl
% PlugInAuthorEmail: zahl@users.sf.net
% PlugInMenuPath: windows-section Mover
Mover/Slider analog signal/wave generation via GVP.

% PlugInDescription

% PlugInUsage

% OptPlugInRefs

% OptPlugInNotes

% OptPlugInHints

% EndPlugInDocuSection
 * -------------------------------------------------------------------------------- 
 */


#include <gtk/gtk.h>
#include <gio/gio.h>
#include <libsoup/soup.h>
#include <zlib.h>

#include "config.h"
#include "plugin.h"

#include "unit.h"
#include "pcs.h"
#include "xsmtypes.h"
#include "glbvars.h"
#include "action_id.h"

#include "gxsm_app.h"
#include "gxsm_window.h"
#include "surface.h"

#include "../common/pyremote.h"

#include "rpspmc_hwi_structs.h"
#include "rpspmc_pacpll.h"
#include "rpspmc_gvpmover.h"


extern int debug_level;
extern int force_gxsm_defaults;


extern "C++" {
        extern RPSPMC_Control *RPSPMC_ControlClass;
        extern RPspmc_pacpll *rpspmc_pacpll;
        extern GxsmPlugin rpspmc_pacpll_hwi_pi;
	extern GVPMoverControl *rpspmc_gvpmover;
        extern rpspmc_hwi_dev *rpspmc_hwi;
}

GVPMoverControl *this_mover_control=NULL;	 

typedef struct {
        int curve_id;
        const char* wave_form_label;
        const char* tool_tip_info;
        int num_waves;
} WAVE_FORM_CONFIG_OPTIONS;

static WAVE_FORM_CONFIG_OPTIONS wave_form_options[] = {
        { MOV_WAVE_SAWTOOTH, "Sawtooth", "Support for stick-slip motion based on a sawtooth signal.", 1 },
        { MOV_WAVE_SINE, "Sine", "Generation of generic sinudal waveforms, which exhibit a phase shift. For demonstration.", 3 },
        { MOV_WAVE_CYCLO, "Cycloide", "Support for stick-slip motion based on a cycloidic signal - full range", 1 },
        { MOV_WAVE_CYCLO_PL, "Cycloide+", "Support for stick-slip motion based on a positively accelarating signal and finally jump to zero.", 1 },
        { MOV_WAVE_CYCLO_MI, "Cycloide-", "Support for stick-slip motion based on a negatively accelarating signal and finally jump to zero.", 1 },
        { MOV_WAVE_KOALA, "Koala **", "Support for Koala type STM heads. INITIAL PORT FROM GXSM3 to 4 and GVP. Needs opt of jumps/Vector dt's", 2 },
        { MOV_WAVE_BESOCKE, "Besocke **", "Support for bettle type STM heads with 3fold segmented piezos for the coarse approach without inner z-electrode. INITIAL PORT FROM GXSM3 to 4 and GVP. NEEDS some work, please see code notes.", 3 },
        { MOV_WAVE_PULSE, "Pulse",	 "Support for arbitrary rectangular pulses.", 1 },

        { MOV_WAVE_STEP, "Stepfunc", "Support for step function.", 1 },
        // Pulses are generated as arbitrary wave forms (stm, 08.10.2010)
        // positive pluse: base line is zero, pulse as height given by amplitude
        // ratio between on/off: 1:1
        // total time = ton + toff
        { MOV_WAVE_DELTA, "Delta Function", "Support for delta function series.", 6 },

        { MOV_WAVE_STEPPERMOTOR, "Stepper Motor", "Support for stepper motor (2 channel).", 2 },
        { -1, NULL, NULL, 0 },
};


class GVP_MOV_GUI_Builder : public BuildParam{
public:
        GVP_MOV_GUI_Builder (GtkWidget *build_grid=NULL, GSList *ec_list_start=NULL, GSList *ec_remote_list=NULL) :
                BuildParam (build_grid, ec_list_start, ec_remote_list) {
                wid = NULL;
                config_checkbutton_list = NULL;
        };

        void start_notebook_tab (GtkWidget *notebook, const gchar *name, const gchar *settings_name,
                                 GSettings *settings) {
                new_grid (); tg=grid;
                gtk_widget_set_hexpand (grid, TRUE);
                gtk_widget_set_vexpand (grid, TRUE);
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




static void GVPMoverControl_Freeze_callback (gpointer x){	 
	if (this_mover_control)	 
		this_mover_control->freeze ();	 
}	 
static void GVPMoverControl_Thaw_callback (gpointer x){	 
	if (this_mover_control)	 
		this_mover_control->thaw ();	 
}

GVPMoverControl::GVPMoverControl (Gxsm4app *app):AppBase(app)
{
        hwi_settings = g_settings_new (GXSM_RES_BASE_PATH_DOT".hwi.rpspmc-gvpmover");
        mover_param.AFM_GPIO_setting = g_settings_get_int (hwi_settings, "mover-gpio-last");
       
	mover_param.AFM_Amp=1;
	mover_param.AFM_Speed=3;
	mover_param.AFM_Steps=10;

        for (int i=0; i< GVP_AFMMOV_MODES; ++i){
                mover_param.AFM_usrAmp[i]=1;
		mover_param.AFM_usrSpeed[i]=3;
                mover_param.AFM_usrSteps[i]=10;
                mover_param.AFM_GPIO_usr_setting[i]=0;

                mover_param.axis_counts[i][0] = 0;
                mover_param.axis_counts[i][1] = 0;
                mover_param.axis_counts[i][2] = 0;
        }

        mover_param.wave_out_channels_used=6;
	for (int k=0; k<mover_param.wave_out_channels_used; ++k)
                mover_param.wave_out_channel_xyz[k][0]=mover_param.wave_out_channel_xyz[k][1]=mover_param.wave_out_channel_xyz[k][2]=0;

        mover_param.wave_out_channel_xyz[0][0]=3, mover_param.wave_out_channel_xyz[0][1]=4, mover_param.wave_out_channel_xyz[0][2]=3;

	mover_param.MOV_output = 0;
	mover_param.MOV_waveform_id = 0;

        mover_param.MOV_waveform_id = g_settings_get_int (hwi_settings, "mover-waveform-id");

	mover_param.final_delay = 50;
	mover_param.max_settling_time = 1000;
	mover_param.inch_worm_phase = 0;
        
	mover_param.time_delay_2 = 0.09;
	mover_param.time_delay_1 = 0.09;
	mover_param.z_Rate = 0.1;
	mover_param.Wave_space = 0.0;
	mover_param.Wave_offset = 0.0;

	mover_param.GPIO_on = 0;
	mover_param.GPIO_off = 0;
	mover_param.GPIO_reset = 0;
	mover_param.GPIO_scan = 0;
	mover_param.GPIO_tmp1 = 0;
	mover_param.GPIO_tmp2 = 0;
	mover_param.GPIO_direction = 0;
	mover_param.GPIO_delay = 250.0;

	Z0_speed = 500.;
	Z0_adjust = 500.;
	Z0_goto = 0.;

	mover_param.MOV_wave_len=1024;
	for (int i=0; i < MOV_MAXWAVELEN; ++i)
		mover_param.MOV_waveform[i] = (short)0;

	Unity    = new UnitObj(" "," ");
	Phase    = new UnitObj(UTF8_DEGREE, "deg");
	Hex      = new UnitObj("h","h");
	Volt     = new UnitObj("V","V");
	Time     = new UnitObj("ms","ms");
	Length   = new UnitObj("nm","nm");
	Ang      = new UnitObj(UTF8_ANGSTROEM,"A");
	Speed    = new UnitObj(UTF8_ANGSTROEM"/s","A/s");

	PI_DEBUG (DBG_L2, "... OK");
	PI_DEBUG (DBG_L2, "GVPMoverControl::GVPMoverControl create folder");
	create_folder ();
	PI_DEBUG (DBG_L2, "... OK");
	this_mover_control=this;	 
	rpspmc_pacpll_hwi_pi.app->ConnectPluginToStartScanEvent (GVPMoverControl_Freeze_callback);	 
	rpspmc_pacpll_hwi_pi.app->ConnectPluginToStopScanEvent (GVPMoverControl_Thaw_callback);
}

short koala_func (double amplitude, double phi){
        // 1/6 ... 1/2 ... 1
        if (phi > 3.*M_PI)
                phi -= 3.*M_PI;
        if (phi < 0.)
                phi += 3.*M_PI;
        if (phi < 3.*M_PI/6.)
                return (short)round (amplitude * sin (phi));
        else if (phi < 3.*M_PI/2.)
                return (short)round (amplitude);
        else
                return (short)round (amplitude * sin (phi-M_PI));
}

short steppermotor_func (double amplitude, double phi){
        while (phi > 2.*M_PI)
                phi -= 2.*M_PI;
        while (phi < 0.)
                phi += 2.*M_PI;
        if (phi < M_PI/2.0)
                return (short)(round(amplitude));
        else if (phi < M_PI*0.75)
                return (short)(round(0.0));
        else if (phi < M_PI*1.5)
                return (short)(round(-amplitude));
        else if (phi < M_PI*1.75)
                return (short)(round (0.0));
        else return (short)(round(amplitude));
}

// port of Besocke Function -- needs some work to optimize vector distributions (dT's) vs fixed grid, address preview/angle ?!?
// phase: 0..1 normalized time/phase
// may inv ampl
double besocke_func (double amp, double pduration, double phase, double tt1, double tt2, double z_Rate, int channel, double angle){
        //double pduration;          // positive duration of the waveform
        //double tt1 = mover_param.time_delay_1;
        //double tt2 = mover_param.time_delay_2;        
        static double tramp ;         // duration of ramps before and after the jump
        static int init=1;
        static double besocke_step_a; // until end of first ramp                
        static double besocke_step_b; // wait before z-jump
        static double besocke_step_c; // wait after z-jump, before xy-motion
        static double besocke_step_d; // wait after xy-motion, before final ramp
        static double R;              // ratio between z-jump and xy-motion
                        
        if (channel < 0 || init) { // init only
                // calculate time in ms for ramps                 
                if (pduration > 2*fabs(tt1)+fabs(tt2)) {
                        PI_DEBUG_GP (DBG_L2,"duration is longer than expected delay times");
                        tramp=(pduration - 2*fabs(tt1) - fabs(tt2))/2;
                } else {
                        // do not allow for negative times!
                        PI_DEBUG_GP (DBG_L2,"duration is too short");
                        tramp=pduration/2;
                        tt1 = 0;
                        tt2 = 0;                
                }

                // define individual time steps within the signal
                besocke_step_a = tramp / pduration;
                besocke_step_b = besocke_step_a + tt1 / pduration;
                besocke_step_c = besocke_step_b + tt2 / pduration;
                besocke_step_d = besocke_step_c + tt1 / pduration;

                R=z_Rate;           //ratio between z-jump and xy-motion

                PI_DEBUG_GP (DBG_L2, "case BESOCKE t0=%f (ramp time) t1=%f (settling time) t2=%f (period of fall) pduration=%f  amp=%f  z-Rate=%f\n", 
                             tramp,                                
                             tt1, 
                             tt2, 
                             pduration, 
                             amp,
                             z_Rate);
                PI_DEBUG_GP (DBG_L2,"step_a %d step_b %d step_c %d step_d %d\n", besocke_step_a, besocke_step_b, besocke_step_c, besocke_step_d); 
                init = 0;
                return 0.;
        } 
        //if (pointing < 0)
        //     amp = -amp; // invert

        double y[3] = {0.,0.,0.};
        
        // generate fundamental waveforms xy and z
        if (phase <= besocke_step_a){ // first ramp
                switch (channel){
                case 0: y[0] = 0.5*amp*phase/besocke_step_a; // xy signal stored in channel #1                                        
                case 2: y[2] = -R*fabs (amp)*phase/besocke_step_a; // z signal stored in channel #3                
                case 1: y[1] = amp; // channel #2 only set for debugging
                }
        } else if (besocke_step_a < phase && phase <= besocke_step_b) { // first delay, set constant values
                switch (channel){
                case 0: y[0] = amp/2.;
                case 2: y[2] = R*fabs(amp);
                case 1: y[1] = amp;
                }
        } else if (besocke_step_b < phase && phase <= besocke_step_c) { // second delay, z-jump, constant values
                switch (channel){
                case 0: y[0] = amp/2.;
                case 2: y[2] = 0.0;
                case 1: y[1] = amp;
                }
        } else if (besocke_step_c < phase && phase <= besocke_step_d) { // third delay, xy-jump, constant values
                switch (channel){
                case 0: y[0] = -amp/2.;
                case 2: y[2] = 0.0;
                case 1: y[1] = amp;
                }
        } else if (besocke_step_d < phase) { // last ramp
                switch (channel){
                case 0: y[0] = -0.5*amp*(1.-phase)/besocke_step_a; //mover_param.MOV_waveform[i] = -mover_param.MOV_waveform[mover_param.MOV_wave_len-i];
                case 2: y[2] = 0.0;
                case 1: y[1] = -amp;
                }
        }
                
        if (angle < -180.) angle=-180.;                 //cases for Z-direction
        else if (angle > 180.) angle=0.;

        double c=cos (angle*M_PI/180);
        double s=sin (angle*M_PI/180);
        
        double mov = y[0];
        double jump = y[2];
        double temp;
        switch (channel){
        case 0: return c*mov - 0.5*s*mov + jump;   // Ch0 ... X+
        case 1: return s*mov + jump;               // Ch1 ... Y+
        case 2: return -c*mov - 0.5*s*mov + jump;  // Ch2 --- X-
        }

        return 0.;
}   



// Parametric Generation of GVP for Wave Forms
// duration in ms
int GVPMoverControl::create_waveform (double amp, double duration, int limit_cycles, int pointing, int axis){
        // check and find wave valid wave form id "k"
        int k=0;
        for (k=0; wave_form_options[k].wave_form_label; ++k)
                if (mover_param.MOV_waveform_id == wave_form_options[k].curve_id)
                        break;
        int num_waves = wave_form_options[k].num_waves;
        
        PI_DEBUG_GP(DBG_L2, "GVPMoverControl::create_waveform ID=%d  %s #Waves=%d\n", mover_param.MOV_waveform_id,  wave_form_options[k].wave_form_label, wave_form_options[k].num_waves);
        g_message ("GVPMoverControl::create_waveform ID=%d  %s #Waves=%d", mover_param.MOV_waveform_id,  wave_form_options[k].wave_form_label, wave_form_options[k].num_waves);

        // wave_out_channel_xyz[nth-wave 0..5=GVP CH][axis: XYZ 0..2];
 
        double phase = mover_param.inch_worm_phase/360.;

        double wo = mover_param.Wave_offset;
        double t_space = mover_param.Wave_space*1e-3; // sec
        double t_jump  = 0.1e-6; // sec
        double t_wave  = fabs(duration)*1e-3 - t_jump; // sec
        int    n_reps  = limit_cycles;
        double t_delta2 = 0.5*t_space+t_jump;
        
        int vector_index=0;
        int gvp_options=0;
        int SRCS=0x0000c03f;
        //rpspmc_hwi->resetVPCconfirmed ();

        // loookups
        #define MAX_CH 6
        double pa[MAX_CH]  = { 0.,0.,0.,0., 0.,0.}; // pointing ampl. vector [x, dy, dz, du, da, db]
        double p0[MAX_CH]  = { 0.,0.,0.,0., 0.,0.}; // offset vector [x, dy, dz, du, da, db]
        int chi[MAX_CH];
        for (int j=0; j < num_waves && j < MAX_CH; ++j){ // up to 6 Waves the same time possible
                int ch = mover_param.wave_out_channel_xyz[j][axis]-1;
                if (ch >= 0 && ch < 6) {
                        pa[ch] = pointing * amp;
                        p0[ch] = wo;
                        chi[j] = ch;
                } else { chi[j]=0; }
        }
        
        double vp_duration=0.;
	switch (mover_param.MOV_waveform_id){
	case MOV_WAVE_SAWTOOTH:
                PI_DEBUG_GP (DBG_L2, " ** SAWTOOTH WAVEFORM CALC\n");
                {
                        // Init
                        vp_duration += RPSPMC_ControlClass->make_dUZXYAB_vector_all_volts (vector_index++,
                                                                                 p0[3], p0[2], p0[0], p0[1], p0[4], p0[5], //  GVP_du[k], GVP_dz[k], GVP_dx[k], GVP_dy[k], GVP_da[k], GVP_db[k],
                                                                                 10, 0, 0, t_wave/20, // GVP_points[k], GVP_vnrep[k], GVP_vpcjr[k], GVP_ts[k],
                                                                                 SRCS, VP_INITIAL_SET_VEC | gvp_options);
                        // Ramp
                        vp_duration += RPSPMC_ControlClass->make_dUZXYAB_vector_all_volts (vector_index++,
                                                                                 pa[3], pa[2], pa[0], pa[1], pa[4], pa[5], // GVP_du[k], GVP_dz[k], GVP_dx[k], GVP_dy[k], GVP_da[k], GVP_db[k],
                                                                                 100, 0, 0, 0.5*t_wave, // GVP_points[k], GVP_vnrepa[k], GVP_vpcjr[k], GVP_ts[k],
                                                                                 SRCS, gvp_options);
                        // Jump
                        vp_duration += RPSPMC_ControlClass->make_dUZXYAB_vector_all_volts (vector_index++,
                                                                                 -2*pa[3], -2*pa[2], -2*pa[0], -2*pa[1], -2*pa[4], -2*pa[5], // GVP_du[k], GVP_dz[k], GVP_dx[k], GVP_dy[k], GVP_da[k], GVP_db[k],
                                                                                 2, 0, 0, t_jump, // GVP_points[k], GVP_vnrep[k], GVP_vpcjr[k], GVP_ts[k],
                                                                                 SRCS, gvp_options);
                        // Ramp
                        int nr=0;
                        int jmp=0;
                        if (t_space <= t_jump){ nr = n_reps; jmp = -2; } // repeat from here
                        vp_duration += RPSPMC_ControlClass->make_dUZXYAB_vector_all_volts (vector_index++,
                                                                                 pa[3], pa[2], pa[0], pa[1], pa[4], pa[5], // GVP_du[k], GVP_dz[k], GVP_dx[k], GVP_dy[k], GVP_da[k], GVP_db[k],
                                                                                 100, nr, jmp, 0.5*t_wave, // GVP_points[k], GVP_vnrep[k], GVP_vpcjr[k], GVP_ts[k],
                                                                                 SRCS, gvp_options);
                        // add space, repeat
                        if (nr == 0)
                                vp_duration += RPSPMC_ControlClass->make_dUZXYAB_vector_all_volts (vector_index++,
                                                                                         0., 0., 0., 0., 0., 0., //  GVP_du[k], GVP_dz[k], GVP_dx[k], GVP_dy[k], GVP_da[k], GVP_db[k],
                                                                                         10, n_reps, -3, t_space, // GVP_points[k], GVP_vnrep[k], GVP_vpcjr[k], GVP_ts[k],
                                                                                         SRCS, gvp_options);
                        // total GVP points: 10 + Reps*(100 + 2 + 100 + 10) => 222 at end of 1st rep
                }
                RPSPMC_ControlClass->append_null_vector (vector_index, gvp_options);
		break;
	case MOV_WAVE_SINE:
                PI_DEBUG_GP (DBG_L2, " ** SINE WAVEFORM CALC\n");
                {
                        double p[MAX_CH]  = { 0.,0.,0.,0.,0.,0.}; // ch/pointing vector [x, dy, dz, du, da, db]
                        
                        // Init
                        double phi[MAX_CH], yp[MAX_CH], y[MAX_CH];
                        for (int j=0; j<num_waves && j < MAX_CH; ++j){
                                phi[j]    = j*phase*2.0*M_PI;
                                p[chi[j]] = yp[j] = wo + pointing*amp*sin (phi[j]);
                        }
                        vp_duration += RPSPMC_ControlClass->make_dUZXYAB_vector_all_volts (vector_index++,
                                                                                 p[3], p[2], p[0], p[1], p[4], p[5],
                                                                                 10, 0, 0, t_wave/20,
                                                                                 SRCS, VP_INITIAL_SET_VEC | gvp_options);
                        int NumVecs = 28;
                        int nr=0;
                        for (int t=0; t <= NumVecs; ++t){
                                for (int j=0; j<num_waves && j < MAX_CH; ++j){
                                        y[j] = wo + pointing*amp*sin (phi[j] + (double)t*2.*M_PI/NumVecs);
                                        p[chi[j]] = y[j] - yp[j];
                                        yp[j] = y[j];
                                }
                                // Ramp to next point
                                nr=0;
                                int jmp=0;
                                if (t == NumVecs && t_space <= t_jump){ nr = n_reps; jmp = -(NumVecs-1); }
                                vp_duration += RPSPMC_ControlClass->make_dUZXYAB_vector_all_volts (vector_index++,
                                                                                         p[3], p[2], p[0], p[1], p[4], p[5],
                                                                                         10, nr, jmp, t_wave/NumVecs,
                                                                                         SRCS, gvp_options);
                        }
                        // Space, Repeat
                        if (nr == 0)
                                vp_duration += RPSPMC_ControlClass->make_dUZXYAB_vector_all_volts (vector_index++,
                                                                                         0., 0., 0., 0., 0., 0., //  GVP_du[k], GVP_dz[k], GVP_dx[k], GVP_dy[k], GVP_da[k], GVP_db[k],
                                                                                         10, n_reps, -NumVecs, t_space, // GVP_points[k], GVP_vnrep[k], GVP_vpcjr[k], GVP_ts[k],
                                                                                         SRCS, gvp_options);
                        RPSPMC_ControlClass->append_null_vector (vector_index, gvp_options);
                }
		break;
	default: // othere func like:
                PI_DEBUG_GP (DBG_L2, " ** CYCLO* CALC\n");
                {
                        double p[MAX_CH]  = { 0.,0.,0.,0.,0.,0.}; // ch/pointing vector [x, dy, dz, du, da, db]
                        
                        int NumVecs  = 28;
                        int NumVecs2 = 14;
                        int nr=0;
                        double yp[MAX_CH], y[MAX_CH], yi[MAX_CH];
                        double tp;
                        double t=0.0;
                        double it=0.;
                        double dt = 1./NumVecs;
                        int jump = 0;
                        double t4 = 0.;
                        double c=0.;
                        double mi=1;

                        if (mover_param.MOV_waveform_id == MOV_WAVE_BESOCKE)
                                besocke_func (amp, duration, t, mover_param.time_delay_1, mover_param.time_delay_2, mover_param.z_Rate, -1, mover_param.MOV_angle); // init

                        for (int tv=0; tv <= NumVecs; ++tv){
                                // template to construct any funcion with potential steps
                                // calculate function y[j](t) = ...
                                switch (mover_param.MOV_waveform_id){
                                case MOV_WAVE_CYCLO_MI: mi = -1.;
                                case MOV_WAVE_CYCLO_PL:
                                        t = sqrt(sqrt(it)); // calculate inverse to have equidist moves in y vs. x per vector! Cycloid like t^4 +/-
                                        t4 = it; //t*t*t*t;
                                        for (int j=0; j<num_waves && j < MAX_CH; ++j){
                                                if (pointing > 0)
                                                        y[j] = -mi*amp*t4;
                                                else
                                                        y[j] = -mi*amp*(1.-t4);
                                        }
                                        //g_print ("CyclPM %d it %g t %g t4 %g y %g\n", tv, it, t, t4, y[0]);
                                        it += dt;
                                        if (tv == NumVecs) jump = 2;
                                        else jump = 0;
                                        break;
                                
                                case MOV_WAVE_CYCLO:
                                        //double dt = M_PI/(NumVecs - 1);
                                        t = acos(1.-it)/M_PI; // calculate inverse to have equidist moves in y vs. x per vector! Cycloid/Rollcurve symmetric
                                        c = 1.-it; //cos (t);
                                        if (tv == NumVecs2) jump = 1; // jump
                                        else jump = 0;
                                        for (int j=0; j<num_waves && j < MAX_CH; ++j){
                                                if (c > 0.)
                                                        y[j] = pointing*amp*(1.-c);
                                                else
                                                        y[j] = -pointing*amp*(1.+c);
                                        }
                                        //g_print ("CyclS %d it %g t %g c %g y %g\n", tv, it, t, c, y[0]);
                                        it += 2*dt;
                                        break;
                                
                                case MOV_WAVE_PULSE: pointing = 1.; // no pointing for pulse
                                case MOV_WAVE_STEP:
                                        for (int j=0; j<num_waves && j < MAX_CH; ++j){
                                                switch (tv){
                                                case 0: t=0.;      y[j]=0; break;
                                                case 1: t=t_jump+phase*j*t_wave; y[j]=0.; break;
                                                case 2: t+=t_jump; y[j]=pointing*amp; break;
                                                case 3: t+=0.5;    y[j]=pointing*amp; break;
                                                case 4: t+t_jump;  y[j]=0.; break;
                                                case 5: t=1.0;     y[j]=0.0; tv=NumVecs; break;
                                                default: break;
                                                }
                                        }
#if 0 // FUNCTIONAL, RAMP JUMP RATE LIMITED BY dT GRID                                                
                                        for (int j=0; j<num_waves && j < MAX_CH; ++j){
                                                t = ((double)tv/NumVecs)+j*phase*t_wave;
                                                y[j] = pointing*amp*(round(t - floor(t)));
                                        }
#endif
                                        break;

                                case MOV_WAVE_DELTA:
                                        for (int j=0; j<num_waves && j < MAX_CH; ++j){
                                                switch (tv){
                                                case 0: t=0.;        y[j]=0; break;
                                                case 1: t=t_jump+0.2*j*t_wave; y[j]=0.; break;
                                                case 2: t+=t_jump;   y[j]=pointing*amp; break;
                                                case 3: t+=t_delta2; y[j]=pointing*amp; break;
                                                case 4: t+t_jump;    y[j]=0.; break;
                                                case 5: t=1.0;       y[j]=0.0; tv=NumVecs; break;
                                                default: break;
                                                }
                                        }
                                        break;

                                case MOV_WAVE_STEPPERMOTOR:
                                        for (int j=0; j<num_waves && j < MAX_CH; ++j){
                                                t = (double)tv/NumVecs;
                                                y[j] = steppermotor_func (amp, 2*M_PI*t+j*pointing*M_PI/4.0);
                                        }
                                        break;

                                case MOV_WAVE_KOALA:
                                        for (int j=0; j<num_waves && j < MAX_CH; ++j){
                                                t = (double)tv/NumVecs;
                                                if (pointing > 0)
                                                        y[j] = koala_func (amp, (double)t*3.*M_PI + 2*j*M_PI);
                                                else
                                                        y[j] = koala_func (amp, (double)(1.0-t)*3.*M_PI + 2*j*M_PI);
                                        }
                                        break;
                                        
                                case MOV_WAVE_BESOCKE:
                                        for (int j=0; j<num_waves && j < MAX_CH; ++j){
                                                t = (double)tv/NumVecs;
                                                y[j] = besocke_func (pointing*amp, duration, t, mover_param.time_delay_1, mover_param.time_delay_2, mover_param.z_Rate, j, mover_param.MOV_angle);
                                        }
                                        break;

                                default: break;
                                }

                                if (tv == 0){ // Init
                                        for (int j=0; j<num_waves && j < MAX_CH; ++j)
                                                p[chi[j]] = yi[j] = yp[j] = y[j];
                                        tp = t;
                                        vp_duration += RPSPMC_ControlClass->make_dUZXYAB_vector_all_volts (vector_index++,
                                                                                                 p[3], p[2], p[0], p[1], p[4], p[5],
                                                                                                 10, 0, 0, t_jump,
                                                                                                 SRCS, VP_INITIAL_SET_VEC | gvp_options);
                                } else {
                                        for (int j=0; j<num_waves && j < MAX_CH; ++j){
                                                p[chi[j]] = y[j] - yp[j]; // differentials
                                                yp[j] = y[j];
                                        }
                                        // Ramp to next point
                                        vp_duration += RPSPMC_ControlClass->make_dUZXYAB_vector_all_volts (vector_index++,
                                                                                                 p[3], p[2], p[0], p[1], p[4], p[5],
                                                                                                 10, 0, 0, fabs(t-tp)*t_wave+t_jump,
                                                                                                 SRCS, gvp_options);
                                        if (jump==1){ // add jump vector to inverse
                                                for (int j=0; j<num_waves && j < MAX_CH; ++j){
                                                        p[chi[j]] = -y[j] - yp[j]; // differentials
                                                        yp[j] = -y[j];
                                                }
                                                vp_duration += RPSPMC_ControlClass->make_dUZXYAB_vector_all_volts (vector_index++,
                                                                                                         p[3], p[2], p[0], p[1], p[4], p[5],
                                                                                                         2, 0, 0, t_jump,
                                                                                                         SRCS, gvp_options);
                                        }

                                        if (jump==2){ // jump to initial
                                                for (int j=0; j<num_waves && j < MAX_CH; ++j){
                                                        p[chi[j]] = yi[j] - yp[j]; // differentials
                                                        yp[j] = yi[j];
                                                }
                                                vp_duration += RPSPMC_ControlClass->make_dUZXYAB_vector_all_volts (vector_index++,
                                                                                                         p[3], p[2], p[0], p[1], p[4], p[5],
                                                                                                         2, 0, 0, t_jump,
                                                                                                         SRCS, gvp_options);
                                        }
                                        tp = t;
                                }
                        }
                        // Space, Repeat
                        if (t_space < t_jump)
                                t_space = t_jump;
                        int jmp=1-vector_index; // -NumVecs-1
                        vp_duration += RPSPMC_ControlClass->make_dUZXYAB_vector_all_volts (vector_index++,
                                                                                 0., 0., 0., 0., 0., 0., //  GVP_du[k], GVP_dz[k], GVP_dx[k], GVP_dy[k], GVP_da[k], GVP_db[k],
                                                                                 10, n_reps, jmp, t_space, // GVP_points[k], GVP_vnrep[k], GVP_vpcjr[k], GVP_ts[k],
                                                                                 SRCS, gvp_options);
                        RPSPMC_ControlClass->append_null_vector (vector_index, gvp_options);
                }
		break;
#if 0 // OLD DSP Wave Version
	case MOV_WAVE_STEP:
                PI_DEBUG_GP (DBG_L2, " ** STEP CALC\n");
                for (int i=0; i < mover_param.MOV_wave_len; i += channels){
                        double x = (double)i/mover_param.MOV_wave_len;
                        for (int k=0; k<channels; ++k){
                                double xx = x+k*phase;
                                if (i==0 || i ==  mover_param.MOV_wave_len-1)
                                        mover_param.MOV_waveform[i+k] = 0;
                                else
                                        mover_param.MOV_waveform[i+k] = (short)round(SR_VFAC*pointing*amp*(round(xx - floor(xx))));
                        }
                }
		break;
	case MOV_WAVE_STEPPERMOTOR:
               PI_DEBUG_GP (DBG_L2, " ** STEPPER MOTOR CALC\n");
               PI_DEBUG_GP (DBG_L2, "case StepperMotor channel=%d amp=%f pointing=%f MOV_wave_len=%d \n",
                                channels,
                                amp,
                                pointing,
                                mover_param.MOV_wave_len);
                for (int i=0; i < mover_param.MOV_wave_len; i += channels){
                        double x = (double)2.*M_PI*i/mover_param.MOV_wave_len;
                        for (int k=0; k<channels; ++k){                        
                                if (i ==  mover_param.MOV_wave_len-1){
                                // just make sure that there is not voltage at the end of the pulses
                                        mover_param.MOV_waveform[i+k] = (short) (0.0);
                                } else {
                                        mover_param.MOV_waveform[i+k] = steppermotor_func(SR_VFAC*amp, x+k*pointing*M_PI/4.0);
                                        PI_DEBUG_GP (DBG_L2, "case i=%d, x=%f, wave=%d \n",
                                i+k,x,mover_param.MOV_waveform[i+k]);
                                }
                        }
                }
		break;

	case MOV_WAVE_PULSE:
                PI_DEBUG_GP (DBG_L2, " ** PULSE CALC\n");
                for (int i=0; i < mover_param.MOV_wave_len; i += channels){
                        double x = (double)i/mover_param.MOV_wave_len;
                        for (int k=0; k<channels; ++k){
                                double xx = x+k*phase;
                                if (i==0 || i ==  mover_param.MOV_wave_len-1)
                                        mover_param.MOV_waveform[i+k] = 0;
                                else
                                        mover_param.MOV_waveform[i+k] = (short)round(SR_VFAC*amp*(round(xx - floor(xx))));
                        }
                }
		break;

	case MOV_WAVE_KOALA:
                PI_DEBUG_GP (DBG_L2, " ** KOALA CALC\n");
                {             
                int i=0;
                for (; i <= mover_param.MOV_wave_len; i += channels)
                        for (int k=0; k<channels; ++k)
                                if (pointing > 0) // wave for forward direction 
                                        mover_param.MOV_waveform[i+k]      = koala_func (SR_VFAC*amp, (double)i*3.*M_PI/kn + 2*k*M_PI);
                                else
                                        mover_param.MOV_waveform[imax-i+k] = koala_func (SR_VFAC*amp, (double)i*3.*M_PI/kn + 2*k*M_PI);
                }
		break;
        
	case MOV_WAVE_BESOCKE:
                PI_DEBUG_GP (DBG_L2, " ** BESOCKE CALC\n");
                {
                double pduration;               // positive duration of the waveform
                double tramp ;                  // duration of ramps before and after the jump
                double tt1 = mover_param.time_delay_1;
                double tt2 = mover_param.time_delay_2;        
                        
                // calculate time in ms for ramps                 
                pduration = abs(duration);
                if (pduration > 2*abs(tt1)+abs(tt2)) {
                        PI_DEBUG_GP (DBG_L2,"duration is longer than expected delay times");
                        tramp=(pduration - 2*abs(tt1) - abs(tt2))/2;
                } else {
                        // do not allow for negative times!
                        PI_DEBUG_GP (DBG_L2,"duration is too short");
                        tramp=pduration/2;
                        tt1 = 0;
                        tt2 = 0;                
                }

                // define individual time steps within the signal
                int besocke_step_a; // steps until end of first ramp                
                besocke_step_a = (int) round((double) tramp / pduration * mover_param.MOV_wave_len);
                int besocke_step_b; // (total number of) steps to wait before z-jump
                besocke_step_b = besocke_step_a + (int) round((double) tt1 / pduration * mover_param.MOV_wave_len);
                int besocke_step_c; // (total number of) steps to wait after z-jump, before xy-motion
                besocke_step_c = besocke_step_b + (int) round((double) tt2 / pduration * mover_param.MOV_wave_len);
                int besocke_step_d; // (total number of) steps to wait after xy-motion, before final ramp
                besocke_step_d = besocke_step_c + (int) round((double) tt1 / pduration * mover_param.MOV_wave_len);

                double R=mover_param.z_Rate;           //ratio between z-jump and xy-motion
                        

                PI_DEBUG_GP (DBG_L2, "case BESOCKE t0=%f (ramp time) t1=%f (settling time) t2=%f (period of fall) pduration=%f  amp=%f  z-Rate=%f  MOV_wave_len=%d \n", 
                                tramp,                                
                                tt1, 
                                tt2, 
                                pduration, 
                                amp,
                                mover_param.z_Rate,
                                mover_param.MOV_wave_len);
                PI_DEBUG_GP (DBG_L2,"step_a %d step_b %d step_c %d step_d %d\n", besocke_step_a, besocke_step_b, besocke_step_c, besocke_step_d); 
 
                //if (pointing < 0)
                //     amp = -amp; // invert

                // generate fundamental waveforms xy and z
                for (int i = 0; i <= mover_param.MOV_wave_len; i += channels){
                        if (i <= besocke_step_a){ // first ramp
                                // xy signal stored in channel #1                                        
                                mover_param.MOV_waveform[i] = (short)round(SR_VFAC*amp/2.0*((double)i/(double)besocke_step_a));
                                // z signal stored in channel #3                
                                mover_param.MOV_waveform[i+2] = (short)round((-1.0)*SR_VFAC*abs(amp)*R*((double)i/(double)besocke_step_a));  
                                // channel #2 only set for debuggin
                                //mover_param.MOV_waveform[i+1] = (short)round(SR_VFAC*(1.0)*amp);              
                        }
                        if (besocke_step_a < i && i <= besocke_step_b) { // first delay, set constant values
                                mover_param.MOV_waveform[i] = (short)round(SR_VFAC*amp/2.0);
                                mover_param.MOV_waveform[i+2] = (short)round((-1.0)*SR_VFAC*abs(amp)*R);
                                //mover_param.MOV_waveform[i+1] = (short)round(SR_VFAC*(1.0)*amp);
                        }
                        if (besocke_step_b < i && i <= besocke_step_c) { // second delay, z-jump, constant values
                                mover_param.MOV_waveform[i] = (short)round(SR_VFAC*amp/2.0);
                                mover_param.MOV_waveform[i+2] = (short) 0.0;
                                //mover_param.MOV_waveform[i+1] = (short)round(SR_VFAC*(1.0)*amp);
                        }
                        if (besocke_step_c < i && i <= besocke_step_d) { // third delay, xy-jump, constant values
                                mover_param.MOV_waveform[i] = (short)round((-1.0)*SR_VFAC*amp/2.0);
                                mover_param.MOV_waveform[i+2] = (short) 0.0;
                                //mover_param.MOV_waveform[i+1] = (short)round(SR_VFAC*(-1.0)*amp);
                        }
                        if (besocke_step_d < i) { // last ramp
                                mover_param.MOV_waveform[i] = (-1.0)*mover_param.MOV_waveform[mover_param.MOV_wave_len-i];
                                mover_param.MOV_waveform[i+2] = (short) 0.0;
                                //mover_param.MOV_waveform[i+1] = (short)round(SR_VFAC*(-1.0)*amp);
                        }
                }

                
                if (mover_param.MOV_angle < (-180.)) mover_param.MOV_angle=(-180.);                 //cases for Z-direction
                else if (mover_param.MOV_angle > (180.)) mover_param.MOV_angle=(0.);

                double cosinus=cos(mover_param.MOV_angle*M_PI/180);
                double sinus=sin(mover_param.MOV_angle*M_PI/180);
                
                for (int i=0; i < mover_param.MOV_wave_len; i += channels)
                {
                        short mov = mover_param.MOV_waveform[i];
                        short jump = mover_param.MOV_waveform[i+2];
                        // make sure that values stay within limits of short integer                        
                        long temp;
                        temp = (long) round (cosinus*mov + sinus*(-1)*mov/2 + jump);
                        temp = temp > SHRT_MAX ? SHRT_MAX : temp;
                        temp = temp < SHRT_MIN ? SHRT_MIN : temp;
                        mover_param.MOV_waveform[i] = (short) temp;
                        temp = (long) round (sinus*mov + jump);
                        temp = temp > SHRT_MAX ? SHRT_MAX : temp;
                        temp = temp < SHRT_MIN ? SHRT_MIN : temp;
                        mover_param.MOV_waveform[i+1] = (short) temp;
                        temp = (long) round (cosinus*(-1)*mov + sinus*(-1)*mov/2 + jump);
                        temp = temp > SHRT_MAX ? SHRT_MAX : temp;
                        temp = temp < SHRT_MIN ? SHRT_MIN : temp;
                        mover_param.MOV_waveform[i+2] = (short) temp;
                }
                                
                /*Now Ch0 ... X+
                      Ch1 ... Y+
                      Ch2 --- X-*/ 
                }   
		break;
#endif
        }
	
        return 0;
}


GVPMoverControl::~GVPMoverControl (){        
	this_mover_control=NULL;

	delete Length;
	delete Phase;
	delete Time;
	delete Volt;
	delete Unity;
	delete Hex;
	delete Ang;
	delete Speed;
}



#define ARROW_SIZE 40

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

// ============================================================
// Popup Menu and Object Action Map
// ============================================================

static GActionEntry win_GVPMover_popup_entries[] = {
        { "gvp-mover-configure", GVPMoverControl::configure_callback, NULL, "true", NULL },
};

void GVPMoverControl::configure_callback (GSimpleAction *action, GVariant *parameter, 
                                          gpointer user_data){
        GVPMoverControl *mc = (GVPMoverControl *) user_data;
        GVariant *old_state, *new_state;

        if (action){
                old_state = g_action_get_state (G_ACTION (action));
                new_state = g_variant_new_boolean (!g_variant_get_boolean (old_state));
                
                g_simple_action_set_state (action, new_state);

                PI_DEBUG_GP (DBG_L4, "Toggle action %s activated, state changes from %d to %d\n",
                             g_action_get_name (G_ACTION (action)),
                             g_variant_get_boolean (old_state),
                             g_variant_get_boolean (new_state));
                g_simple_action_set_state (action, new_state);
                g_variant_unref (old_state);

                g_settings_set_boolean (mc->hwi_settings, "configure-mode", g_variant_get_boolean (new_state));
        } else {
                // new_state = g_variant_new_boolean (true);
                new_state = g_variant_new_boolean (g_settings_get_boolean (mc->hwi_settings, "configure-mode"));
        }

	if (g_variant_get_boolean (new_state)){
                g_slist_foreach
                        ( mc->mov_bp->get_configure_list_head (),
                          (GFunc) App::show_w, NULL
                          );
                g_slist_foreach
                        ( mc->mov_bp->get_config_checkbutton_list_head (),
                          (GFunc) GVPMoverControl::show_tab_to_configure, NULL
                          );
        } else  {
                g_slist_foreach
                        ( mc->mov_bp->get_configure_list_head (),
                          (GFunc) App::hide_w, NULL
                          );
                g_slist_foreach
                        ( mc->mov_bp->get_config_checkbutton_list_head (),
                          (GFunc) GVPMoverControl::show_tab_as_configured, NULL
                          );
        }
        if (!action){
                g_variant_unref (new_state);
        }
}

void GVPMoverControl::AppWindowInit(const gchar *title, const gchar *sub_title){
        if (title) { // stage 1
                PI_DEBUG (DBG_L2, "GVPMoverControl::AppWindowInit -- header bar");

                app_window = gxsm4_app_window_new (GXSM4_APP (main_get_gapp()->get_application ()));
                window = GTK_WINDOW (app_window);

                header_bar = gtk_header_bar_new ();
                gtk_widget_show (header_bar);

                XSM_DEBUG (DBG_L2,  "VC::VC setup titlbar" );

                gtk_window_set_title (GTK_WINDOW (window), title);
                gtk_window_set_titlebar (GTK_WINDOW (window), header_bar);
        
                v_grid = gtk_grid_new ();

                gtk_window_set_child (GTK_WINDOW (window), v_grid);
                g_object_set_data (G_OBJECT (window), "v_grid", v_grid); // was "vbox"

                gtk_widget_show (GTK_WIDGET (window));

        } else {
                PI_DEBUG (DBG_L2, "GVPMoverControl::AppWindowInit -- header bar -- stage two, hook configure menu");

                win_GVPMover_popup_entries[0].state = g_settings_get_boolean (hwi_settings, "configure-mode") ? "true":"false"; // get last state
                g_action_map_add_action_entries (G_ACTION_MAP (app_window),
                                                 win_GVPMover_popup_entries, G_N_ELEMENTS (win_GVPMover_popup_entries),
                                                 this);

                // create window PopUp menu  ---------------------------------------------------------------------

		GtkBuilder *builder = gtk_builder_new_from_resource ("/" GXSM_RES_BASE_PATH "/gxsm4-hwi-menus.ui");

		GObject *hwi_gvpmover_popup_menu = gtk_builder_get_object (builder, "hwi-gvpmover-popup-menu");
		if (!hwi_gvpmover_popup_menu)
			PI_DEBUG_GP_ERROR (DBG_L1, "id hwi-gvpmover-popup-menu not found in resource.\n");
		else
			mc_popup_menu = gtk_popover_menu_new_from_model (G_MENU_MODEL (hwi_gvpmover_popup_menu));

		//mc_popup_menu = gtk_popover_menu_new_from_model (G_MENU_MODEL (main_get_gapp()->get_hwi_mover_popup_menu ()));

                // attach popup menu configuration to tool button --------------------------------
                GtkWidget *header_menu_button = gtk_menu_button_new ();
                gtk_menu_button_set_icon_name (GTK_MENU_BUTTON (header_menu_button), "applications-utilities-symbolic");
                gtk_menu_button_set_popover (GTK_MENU_BUTTON (header_menu_button), mc_popup_menu);
                gtk_header_bar_pack_end (GTK_HEADER_BAR (header_bar), header_menu_button);
                gtk_widget_show (header_menu_button);
        }
}

#if 0 // TEST CALLBACKS FOR BUTTON BEHAVIOR TEST BELOW
static void
wid_wu_test (GtkWidget *widget,
             gpointer   user_data)
{
  g_message ("BUTTON WIDGET GESTURE TEST: %s", (const gchar*)user_data);
}

static void
wid_gnxyu_test (GtkGesture *gesture, int n_press, double x, double y, gpointer user_data)
{
  g_message ("GNXYU BUTTON WIDGET GESTURE TEST: %s", (const gchar*)user_data);
}

static void
wid_gu_test (GtkGesture *gesture, gpointer user_data)
{
  g_message ("GU*** BUTTON WIDGET GESTURE TEST: %s", (const gchar*)user_data);
}
#endif

void GVPMoverControl::create_folder (){
	GtkWidget *notebook;
	GtkWidget *button;
        // FIX-ME GTK4
	//GtkAccelGroup *accel_group=NULL;

        //Gtk_EntryControl *ec_phase;
 
        PI_DEBUG (DBG_L2, "GVPMoverControl::create_folder");

	if( IS_MOVER_CTRL ){
		AppWindowInit (MOV_MOVER_TITLE); // stage one
	}
	else {
		AppWindowInit (MOV_SLIDER_TITLE); // stage one
        }
        
	// ========================================
	notebook = gtk_notebook_new ();
        gtk_notebook_set_scrollable (GTK_NOTEBOOK (notebook), TRUE);
        gtk_notebook_popup_enable (GTK_NOTEBOOK (notebook));

	gtk_widget_show (notebook);
	gtk_grid_attach (GTK_GRID (v_grid), notebook, 1,1, 1,1);

	const char *MoverNames[] = { "XY", "Rot", "PSD", "Lens", "Auto", "SM", "Z0", "Config", NULL};
	const char *mover_tab_key[] = { "mover-tab-xy", "mover-tab-rot", "mover-tab-psd", "mover-tab-lens", "mover-tab-auto", "mover-tab-sm", "mover-tab-z0", "mover-tab-config", NULL};
	const char *pcs_tab_remote_key_prefix[] = { "gvpmover-xy-", "gvpmover-rot-", "gvpmover-psd-", "gvpmover-lens-", "gvpmover-auto-", "gvpmover-sm-", "gvpmover-z0-", "gvpmover-config-", NULL};

	int i,itab;

        mov_bp = new GVP_MOV_GUI_Builder (v_grid);
        mov_bp->set_error_text ("Invalid Value.");
        mov_bp->set_input_width_chars (10);
        mov_bp->set_no_spin ();

        for(itab=i=0; MoverNames[i]; ++i){                
                Gtk_EntryControl *ec_axis[3];

                if (main_get_gapp()->xsm->Inst->OffsetMode() != OFM_ANALOG_OFFSET_ADDING && i == 5) continue;
		if (IS_SLIDER_CTRL && i < 4 ) continue;
		PI_DEBUG (DBG_L2, "GVPMoverControl::Mover:" << MoverNames[i]);

                mov_bp->start_notebook_tab (notebook, MoverNames[i], mover_tab_key[i], hwi_settings);
                itab++;
                        
                mov_bp->set_pcs_remote_prefix (pcs_tab_remote_key_prefix[i]);

                if (i==6){ // Z0 Tab
                        mov_bp->new_grid_with_frame ("Z-Offset Control");
			mov_bp->set_default_ec_change_notice_fkt (GVPMoverControl::ChangedNotify, this);

  			mov_bp->grid_add_ec ("Adjust Range", Ang, &Z0_adjust, 1., 50000., "4.0f", 10., 100., "range");
                        mov_bp->new_line ();

                        mov_bp->set_configure_list_mode_on ();
  			mov_bp->grid_add_ec ("Adjust Speed", Speed, &Z0_speed, 0.1, 5000., "4.1f", 10., 100., "speed");
                        mov_bp->new_line ();
  			mov_bp->grid_add_ec ("Adjust Goto", Ang, &Z0_goto, -1e6, 1e6, "8.1f", 10., 100., "goto");
                        mov_bp->set_configure_list_mode_off ();

                        mov_bp->pop_grid ();
                        mov_bp->new_line ();
                        mov_bp->new_grid_with_frame ("Direction & Action Control");

                        
			// ========================================
			// Direction Buttons
			mov_bp->set_xy (1,11); mov_bp->grid_add_label (" Z+ ");
			mov_bp->set_xy (1,13); mov_bp->grid_add_label (" Z- ");
			mov_bp->set_xy (5,11); mov_bp->grid_add_label (" Auto Adj. ");
			mov_bp->set_xy (5,12); mov_bp->grid_add_label (" Center ");
			mov_bp->set_xy (5,13); mov_bp->grid_add_label (" Goto ");

			// STOP
                        mov_bp->set_xy (3,12);
                        mov_bp->grid_add_widget (button=gtk_button_new_from_icon_name ("process-stopall-symbolic"));
			g_object_set_data( G_OBJECT (button), "GVP_cmd", GINT_TO_POINTER (GVP_CMD_Z0_STOP));
			g_object_set_data( G_OBJECT (button), "MoverNo", GINT_TO_POINTER (99));
			g_signal_connect ( G_OBJECT (button), "clicked",
                                           G_CALLBACK (GVPMoverControl::CmdAction),
                                           this);
                        
			{ // remote hook
				remote_action_cb *ra = g_new( remote_action_cb, 1);
				ra -> cmd = g_strdup_printf("GVP_CMD_STOP_Z0");
				ra -> RemoteCb = (void (*)(GtkWidget*, void*))GVPMoverControl::CmdAction;
				ra -> widget = button;
				ra -> data = this;
				main_get_gapp()->RemoteActionList = g_slist_prepend ( main_get_gapp()->RemoteActionList, ra );
				gchar *help = g_strconcat ("Remote example: action (\"", ra->cmd, "\"", NULL);
				gtk_widget_set_tooltip_text (button, help);
			}

                        // UP arrow (back)
			mov_bp->set_xy (3,11);
                        mov_bp->grid_add_fire_icon_button (
                                                           "arrow-up-symbolic",
                                                           GCallback (GVPMoverControl::CmdAction), this,
                                                           GCallback (GVPMoverControl::StopAction), this);
                        button = mov_bp->icon;
                        //mov_bp->grid_add_widget (button = gtk_button_new_from_icon_name ("seek-backward-symbolic"));
			g_object_set_data( G_OBJECT (button), "GVP_cmd", GINT_TO_POINTER (GVP_CMD_Z0_P));
			g_object_set_data( G_OBJECT (button), "MoverNo", GINT_TO_POINTER (99));

#if 0
			#define signal_name_button_pressed "pressed"
			#define signal_name_button_released "released"
                        mov_bp->grid_add_widget (button = gtk_button_new_from_icon_name ("seek-backward-symbolic"));
			g_object_set_data( G_OBJECT (button), "GVP_cmd", GINT_TO_POINTER (GVP_CMD_Z0_M));
			g_object_set_data( G_OBJECT (button), "MoverNo", GINT_TO_POINTER (99));
			g_signal_connect ( G_OBJECT (button), signal_name_button_pressed,
                                           G_CALLBACK (GVPMoverControl::CmdAction),
                                           this);
			g_signal_connect ( G_OBJECT (button), signal_name_button_released,
                                           G_CALLBACK (GVPMoverControl::StopAction),
                                           this);
#endif
			{ // remote hook -- may be trouble?? TDB
				remote_action_cb *ra = g_new( remote_action_cb, 1);
				ra -> cmd = g_strdup_printf("GVP_CMD_UP_Z0");
				ra -> RemoteCb = (void (*)(GtkWidget*, void*))GVPMoverControl::CmdAction;
				ra -> widget = button;
				ra -> data = this;
				main_get_gapp()->RemoteActionList = g_slist_prepend ( main_get_gapp()->RemoteActionList, ra );
				gchar *help = g_strconcat ("Remote example: action (\"", ra->cmd, "\"", NULL);
				gtk_widget_set_tooltip_text (button, help);
			}


			// DOWN arrow (forward)
			mov_bp->set_xy (3,13);
                        mov_bp->grid_add_fire_icon_button (
                                                           "arrow-down-symbolic",
                                                           GCallback (GVPMoverControl::CmdAction), this,
                                                           GCallback (GVPMoverControl::StopAction), this);
                        button = mov_bp->icon;
			g_object_set_data( G_OBJECT (button), "GVP_cmd", GINT_TO_POINTER (GVP_CMD_Z0_M));
			g_object_set_data( G_OBJECT (button), "MoverNo", GINT_TO_POINTER (99));
                        
#if 0
                        mov_bp->grid_add_widget (button = gtk_button_new_from_icon_name ("seek-forward-symbolic"));
			g_object_set_data( G_OBJECT (button), "GVP_cmd", GINT_TO_POINTER (GVP_CMD_Z0_M));
			g_object_set_data( G_OBJECT (button), "MoverNo", GINT_TO_POINTER (99));
			g_signal_connect ( G_OBJECT (button), signal_name_button_pressed,
                                           G_CALLBACK (GVPMoverControl::CmdAction),
                                           this);
			g_signal_connect ( G_OBJECT (button), signal_name_button_released,
                                           G_CALLBACK (GVPMoverControl::StopAction),
                                           this);
#endif
                        
			{ // remote hook
				remote_action_cb *ra = g_new( remote_action_cb, 1);
				ra -> cmd = g_strdup_printf("GVP_CMD_DOWN_Z0");
				ra -> RemoteCb = (void (*)(GtkWidget*, void*))GVPMoverControl::CmdAction;
				ra -> widget = button;
				ra -> data = this;
				main_get_gapp()->RemoteActionList = g_slist_prepend ( main_get_gapp()->RemoteActionList, ra );
				gchar *help = g_strconcat ("Remote example: action (\"", ra->cmd, "\"", NULL);
				gtk_widget_set_tooltip_text (button, help);
			}

                        // approach (connect, disconnect)
			mov_bp->set_xy (6,11);
                        mov_bp->grid_add_widget (button = gtk_button_new_from_icon_name ("goto-center-symbolic"));
			g_object_set_data( G_OBJECT (button), "GVP_cmd", GINT_TO_POINTER (GVP_CMD_Z0_AUTO));
			g_object_set_data( G_OBJECT (button), "MoverNo", GINT_TO_POINTER (99));
			g_signal_connect ( G_OBJECT (button), "clicked",
                                           G_CALLBACK (GVPMoverControl::CmdAction),
                                           this);
			{ // remote hook
				remote_action_cb *ra = g_new( remote_action_cb, 1);
				ra -> cmd = g_strdup_printf("GVP_CMD_AUTOCENTER_Z0");
				ra -> RemoteCb = (void (*)(GtkWidget*, void*))GVPMoverControl::CmdAction;
				ra -> widget = button;
				ra -> data = this;
				main_get_gapp()->RemoteActionList = g_slist_prepend ( main_get_gapp()->RemoteActionList, ra );
				gchar *help = g_strconcat ("Remote example: action (\"", ra->cmd, "\"", NULL);
				gtk_widget_set_tooltip_text (button, help);
			}


			mov_bp->set_xy (6,12);
                        mov_bp->grid_add_widget (button = gtk_button_new_from_icon_name ("goto-home-symbolic"));
			g_object_set_data( G_OBJECT (button), "GVP_cmd", GINT_TO_POINTER (GVP_CMD_Z0_CENTER));
			g_object_set_data( G_OBJECT (button), "MoverNo", GINT_TO_POINTER (99));
			g_signal_connect ( G_OBJECT (button), "clicked",
                                           G_CALLBACK (GVPMoverControl::CmdAction),
                                           this);
			{ // remote hook
				remote_action_cb *ra = g_new( remote_action_cb, 1);
				ra -> cmd = g_strdup_printf("GVP_CMD_HOME_Z0");
				ra -> RemoteCb = (void (*)(GtkWidget*, void*))GVPMoverControl::CmdAction;
				ra -> widget = button;
				ra -> data = this;
				main_get_gapp()->RemoteActionList = g_slist_prepend ( main_get_gapp()->RemoteActionList, ra );
				gchar *help = g_strconcat ("Remote example: action (\"", ra->cmd, "\"", NULL);
				gtk_widget_set_tooltip_text (button, help);
			}


                        mov_bp->set_xy (6,13);
                        mov_bp->grid_add_widget (button = gtk_button_new_from_icon_name ("goto-position-symbolic"));
			g_object_set_data( G_OBJECT (button), "GVP_cmd", GINT_TO_POINTER (GVP_CMD_Z0_GOTO));
			g_object_set_data( G_OBJECT (button), "MoverNo", GINT_TO_POINTER (99));
			g_signal_connect ( G_OBJECT (button), "clicked",
                                           G_CALLBACK (GVPMoverControl::CmdAction),
                                           this);
			{ // remote hook
				remote_action_cb *ra = g_new( remote_action_cb, 1);
				ra -> cmd = g_strdup_printf("GVP_CMD_GOTO_Z0");
				ra -> RemoteCb = (void (*)(GtkWidget*, void*))GVPMoverControl::CmdAction;
				ra -> widget = button;
				ra -> data = this;
				main_get_gapp()->RemoteActionList = g_slist_prepend ( main_get_gapp()->RemoteActionList, ra );
				gchar *help = g_strconcat ("Remote example: action (\"", ra->cmd, "\"", NULL);
				gtk_widget_set_tooltip_text (button, help);
			}


                        
			mov_bp->notebook_tab_show_all ();
			continue;
		}

                // ================================================== CONFIG TAB
		if (i==7){ // config tab
                        mov_bp->new_grid_with_frame ("Output Configuration");
			mov_bp->set_default_ec_change_notice_fkt (GVPMoverControl::ChangedNotify, this);

                        mov_bp->new_grid_with_frame ("Waveform selection");

                        GtkWidget *cbx;
                        mov_bp->grid_add_widget (cbx = gtk_combo_box_text_new (),2);
                        // fill with options
                        for(int j=0; wave_form_options[j].wave_form_label; j++){
                                 gchar *id = g_strdup_printf ("%d",  wave_form_options[j].curve_id);
                                 gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (cbx), id, wave_form_options[j].wave_form_label);
                                 g_free (id);
                        }
                        /* connect with signal-handler if selected */
                        g_signal_connect (G_OBJECT (cbx), "changed", G_CALLBACK (GVPMoverControl::config_waveform), this);

                        {
                                gchar *id = g_strdup_printf ("%d", mover_param.MOV_waveform_id);
                                gtk_combo_box_set_active_id (GTK_COMBO_BOX (cbx), id);
                                g_free (id);
                        }
                        mov_bp->new_line ();

                        mov_bp->grid_add_ec ("Space", Time, &mover_param.Wave_space, 0., 1000., ".3f", 1., 10., "Wave-Space");
			g_object_set_data( G_OBJECT (mov_bp->input), "Wavespace ", GINT_TO_POINTER (i));
			gtk_widget_set_tooltip_text (mov_bp->input, "Wave form spacing ");
                        g_signal_connect (G_OBJECT (mov_bp->input), "changed", G_CALLBACK (GVPMoverControl::config_waveform), this);
                        mov_bp->new_line ();

                        mov_bp->grid_add_ec ("Offset", Unity, &mover_param.Wave_offset, -10., 10., ".2f", 1., 10., "Wave-Offset");
			g_object_set_data( G_OBJECT (mov_bp->input), "Waveoffset ", GINT_TO_POINTER (i));
			gtk_widget_set_tooltip_text (mov_bp->input, "Wave form relative DC Offset (in Amplitude Volts) ");
                        g_signal_connect (G_OBJECT (mov_bp->input), "changed", G_CALLBACK (GVPMoverControl::config_waveform), this);
                        mov_bp->new_line ();

                        // config options, managed -- moved here
	                mov_bp->set_configure_hide_list_b_mode_on ();
                        mov_bp->grid_add_ec ("Phase", Phase, &mover_param.inch_worm_phase, 0., 360., "3.0f", 1., 60., "IW-Phase");
	                gtk_widget_set_tooltip_text (mov_bp->input,
                                                     "Generic Phase value may be used by custom wave form generation.\n"
                                                     "Used by Sine and Pulse\n");
                        g_signal_connect (G_OBJECT (mov_bp->input), "changed", G_CALLBACK (GVPMoverControl::config_waveform), this);
                        mov_bp->new_line ();
                        mov_bp->set_configure_hide_list_b_mode_off ();;

                        mov_bp->set_configure_hide_list_b_mode_on ();
                        mov_bp->grid_add_ec ("z-jump/xy-amplitude", Unity, &mover_param.z_Rate, 0., 1., ".2f", 0.01, 0.1, "besocke-z-jump-ratio");
                        gtk_widget_set_tooltip_text (mov_bp->input, "Relative amplitude of the z-jump in respect to the amplitude of the xy-slide");
                        g_signal_connect (G_OBJECT (mov_bp->input), "changed", G_CALLBACK (GVPMoverControl::config_waveform), this);
                        mov_bp->new_line ();

                        mov_bp->grid_add_ec ("settling time t1", Time, &mover_param.time_delay_1, 0., 1., ".3f", 0.001, 0.01, "besocke-t1");
                        gtk_widget_set_tooltip_text (mov_bp->input, "Time delay of the signals to settle the tip before and after the z-jump");
                        g_signal_connect (G_OBJECT (mov_bp->input), "changed", G_CALLBACK (GVPMoverControl::config_waveform), this);
                        mov_bp->new_line ();

                        mov_bp->grid_add_ec ("period of fall t2", Time, &mover_param.time_delay_2, 0., 1., ".3f", 0.001, 0.01, "besocke-t2");
                        gtk_widget_set_tooltip_text (mov_bp->input, "Time between the z-jump and the xy-slide.");
                        g_signal_connect (G_OBJECT (mov_bp->input), "changed", G_CALLBACK (GVPMoverControl::config_waveform), this);
                        mov_bp->new_line ();
                        mov_bp->set_configure_hide_list_b_mode_off ();

                        mov_bp->pop_grid ();
                        
                        // ===
                        mov_bp->new_line ();
                        // ======================================== Wave Output Setup ========================================
			mov_bp->new_grid_with_frame ("#wave/direction(xyz)->DAC(0-6) * Preview for a 5ms Duration Wave, 2 cycles");
                        gtk_widget_set_tooltip_text (mov_bp->frame,"rows: waveform, columns: direction, cells: output channel; channel 7 does not work as it is overwritten by another signal."); 

                        mov_bp->new_line ();

                        mov_bp->set_configure_hide_list_b_mode_on ();
                        mov_bp->set_input_width_chars (3);

                        for (int k=0; k<6; ++k){
                                gchar *wchlab= g_strdup_printf("Wave %d: X", k);
                                gchar *wchid = g_strdup_printf("wave-out%d-ch-x", k);
                                mov_bp->grid_add_ec (wchlab, Unity, &mover_param.wave_out_channel_xyz[k][0], 0, 17, ".0f", wchid);
                                gtk_widget_set_tooltip_text (mov_bp->input, "map wave N onto DAC channel (GVP component XYZUAB=1-6) for X direction move action.");
                                g_free (wchid);
                                wchid = g_strdup_printf("wave-out%d-ch-y", k);
                                mov_bp->grid_add_ec ("Y", Unity, &mover_param.wave_out_channel_xyz[k][1], 0, 17, ".0f", wchid);
                                gtk_widget_set_tooltip_text (mov_bp->input, "map wave N onto DAC channel (GVP component XYZUAB=1-6) for Y direction move action.");
                                g_free (wchid);
                                wchid = g_strdup_printf("wave-out%d-ch-z", k);
                                mov_bp->grid_add_ec ("Z", Unity, &mover_param.wave_out_channel_xyz[k][2], 0, 17, ".0f", wchid);
                                gtk_widget_set_tooltip_text (mov_bp->input, "map wave N onto DAC channel (GVP component XYZUAB=1-6) for Z direction move action.");
                                g_free (wchid);
                                g_free (wchlab);

                                GtkWidget *wave_preview_area = gtk_drawing_area_new ();
				gtk_widget_set_size_request (wave_preview_area, 128, 34); // ?!?!?
                                gtk_drawing_area_set_content_width (GTK_DRAWING_AREA (wave_preview_area), 128);
                                gtk_drawing_area_set_content_height (GTK_DRAWING_AREA (wave_preview_area), 34);
                                gtk_drawing_area_set_draw_func (GTK_DRAWING_AREA (wave_preview_area),
                                                                GtkDrawingAreaDrawFunc (wave_preview_draw_function),
                                                                this, NULL);
                                g_object_set_data  (G_OBJECT (wave_preview_area), "wave_ch", GINT_TO_POINTER (k));
                                mov_bp->grid_add_widget (wave_preview_area);

                                mov_bp->new_line ();
                        }
                        mov_bp->set_configure_hide_list_b_mode_off ();
                        // g_object_set_data(G_OBJECT (mov_bp->button), "WAVE-CHXY-LIST", mov_bp->get_configure_hide_list_b_head ());
                                
                        mov_bp->set_default_ec_change_notice_fkt (GVPMoverControl::ChangedNotify, this);

#define ENABLE_GPIO_RPSPMC_CONTROLS // no GPIO on RPSPMC
#ifdef ENABLE_GPIO_RPSPMC_CONTROLS // no GPIO on RPSPMC
                        mov_bp->set_input_width_chars (8);
                        mov_bp->set_configure_hide_list_b_mode_on ();
			mov_bp->grid_add_ec ("GPIO Pon", Hex, &mover_param.GPIO_on, 0x0000, 0xffff, "04X", "GPIO-on");
			g_object_set_data( G_OBJECT (mov_bp->input), "GPIO_on", GINT_TO_POINTER (i));
			gtk_widget_set_tooltip_text (mov_bp->input, "GPIO setting for Pulses >High<.\nNote: Added bits to GPIO setting for each tab.");
                        mov_bp->new_line ();
                        
			mov_bp->grid_add_ec ("GPIO Poff", Hex, &mover_param.GPIO_off, 0x0000, 0xffff, "04X", "GPIO-off"); 
			g_object_set_data( G_OBJECT (mov_bp->input), "GPIO_off", GINT_TO_POINTER (i));
			gtk_widget_set_tooltip_text (mov_bp->input, "GPIO setting for Pulses >Low<.\nNote: Added bits to GPIO setting for each tab.");
                        mov_bp->new_line ();
                        
			mov_bp->grid_add_ec ("GPIO Preset", Hex, &mover_param.GPIO_reset, 0x0000, 0xffff, "04X", "GPIO-reset");
			g_object_set_data( G_OBJECT (mov_bp->input), "GPIO_reset", GINT_TO_POINTER (i));
			gtk_widget_set_tooltip_text (mov_bp->input, "GPIO setting after Pulses are done.\nNote: Added bits to GPIO setting for each tab.");
                        mov_bp->new_line ();
                        mov_bp->set_configure_hide_list_b_mode_off ();
#endif


                        
                        mov_bp->pop_grid ();
                        mov_bp->set_input_width_chars (10);
                        
#if 0 // moved to Auto-App-Tab where it's applying only!
                        // ======================================== Auto Approach Timings ========================================
                        mov_bp->new_line ();

                        mov_bp->new_grid_with_frame ("Auto Approach Timings");

			mov_bp->grid_add_ec("Delay", Time, &mover_param.final_delay, 0., 430., "4.1f", 1., 10., "Auto-App-Delay");
			gtk_widget_set_tooltip_text (mov_bp->input, "Auto Approach Delay: IVC recovery delay after step series\nfired for auto approach.");

                        mov_bp->new_line ();
			mov_bp->grid_add_ec("Max Settling Time", Time, &mover_param.max_settling_time, 0., 10000., "6.1f", 1., 10., "Auto-App-Max-Settling-Time");
			gtk_widget_set_tooltip_text (mov_bp->input, "Auto Approach Max Settlings Time: max allowed time to settle for feedback\nbefore declaring it to be finished.\n(max allowed time/cycle for FB-delta>0)");

                        mov_bp->new_line ();
			mov_bp->grid_add_ec("Retract CI", Unity, &mover_param.retract_ci, 0., 500., "5.1f", 1., 10., "Auto-App-Retract-CI");
			gtk_widget_set_tooltip_text (mov_bp->input, "Auto Approach Retract CI: Feeback CI used in inverse mode for retract tip, start with regular CI");

                        mov_bp->pop_grid ();
#endif

                        // =================
#ifdef ENABLE_GPIO_RPSPMC_CONTROLS // no GPIO on RPSPMC
                        mov_bp->new_line ();
                        // ======================================== GPIO ========================================
                        //mov_bp->new_line ();
			mov_bp->new_grid_with_frame ("GPIO Configuration");

			mov_bp->grid_add_ec ("IO-dir", Hex, &mover_param.GPIO_direction, 0x0000, 0xffff, "04X", "GPIO-direction");
			g_object_set_data( G_OBJECT (mov_bp->input), "GPIO_direction", GINT_TO_POINTER (i));
			gtk_widget_set_tooltip_text (mov_bp->input, "GPIO direction (in/out) configuration\nWARNING: DO NOT TOUCH IF UNSURE.\nPut 0 for all inputs (safe), but no action (will also disable use of GPIO).");

			mov_bp->grid_add_ec ("XY-scan", Hex, &mover_param.GPIO_scan, 0x0000, 0xffff, "04X", "GPIO-scan"); 
                        g_object_set_data( G_OBJECT (mov_bp->input), "GPIO_scan", GINT_TO_POINTER (i));
			gtk_widget_set_tooltip_text (mov_bp->input, "GPIO setting for regular XY-scan mode [XY, Main].");
                        mov_bp->new_line ();

			mov_bp->grid_add_ec ("tmp1", Hex, &mover_param.GPIO_tmp1, 0x0000, 0xffff, "04X", "GPIO-tmp1");
			g_object_set_data( G_OBJECT (mov_bp->input), "GPIO_tmp1", GINT_TO_POINTER (i));
			gtk_widget_set_tooltip_text (mov_bp->input, "GPIO intermediate setting 1 for switching to XY-scan mode [XY, Main].");

			mov_bp->grid_add_ec ("tmp2", Hex, &mover_param.GPIO_tmp2, 0x0000, 0xffff, "04X", "GPIO-tmp2");
			g_object_set_data( G_OBJECT (mov_bp->input), "GPIO_tmp2", GINT_TO_POINTER (i));
			gtk_widget_set_tooltip_text (mov_bp->input, "GPIO intermediate setting 2 for switching to XY-scan mode [XY, Main].");
                        mov_bp->new_line ();

			mov_bp->grid_add_ec ("SW delay", Time, &mover_param.GPIO_delay, 0.,1000., ".1f", 1., 10., "GPIO-delay"); 
			g_object_set_data( G_OBJECT (mov_bp->input), "GPIO_delay", GINT_TO_POINTER (i));
			gtk_widget_set_tooltip_text (mov_bp->input, "GPIO delay: delay in milliseconds after switching bits.");

                        mov_bp->pop_grid ();
#endif
                        
			mov_bp->notebook_tab_show_all ();

                        configure_waveform (cbx); // update now

			continue;
		}

		if (i<GVP_AFMMOV_MODES){ // modes stores 0..5
                        mov_bp->new_grid_with_frame ("Mover Timing");
                        mov_bp->set_default_ec_change_notice_fkt (GVPMoverControl::ChangedNotify, this);

                        mov_bp->grid_add_ec ("Max. Steps", Unity, &mover_param.AFM_usrSteps[i], 1., 5000., "4.0f", 1., 10., "max-steps"); //mov_bp->new_line ();
                        g_object_set_data( G_OBJECT (mov_bp->input), "MoverNo", GINT_TO_POINTER (i));
                        
                        mov_bp->set_configure_list_mode_on ();
                        mov_bp->grid_add_ec ("Amplitude", Volt, &mover_param.AFM_usrAmp[i], -20., 20., "5.2f", 0.1, 1., "amplitude"); mov_bp->new_line ();
                        g_object_set_data( G_OBJECT (mov_bp->input), "MoverNo", GINT_TO_POINTER (i));

                        mov_bp->grid_add_ec ("Duration", Time, &mover_param.AFM_usrSpeed[i], 0.001, 1000., "4.3f", 0.1, 1., "duration"); //mov_bp->new_line ();
                        g_object_set_data( G_OBJECT (mov_bp->input), "MoverNo", GINT_TO_POINTER (i));

                        mov_bp->grid_add_ec ("GPIO", Hex, &mover_param.AFM_GPIO_usr_setting[i], 0, 0xffff, "04X", "gpio"); mov_bp->new_line ();
                        g_object_set_data( G_OBJECT (mov_bp->input), "MoverNo", GINT_TO_POINTER (i));
                        gtk_widget_set_tooltip_text (mov_bp->input, "GPIO setting used to engage this tab mode.");

                        // Auto Tab Extra Options
                        if (i ==4){
                                mov_bp->set_pcs_remote_prefix (pcs_tab_remote_key_prefix[7]); // override for those to keep it in config schema

                                mov_bp->grid_add_ec("Delay", Time, &mover_param.final_delay, 0., 430., "4.1f", 1., 10., "Auto-App-Delay");
                                gtk_widget_set_tooltip_text (mov_bp->input, "Auto Approach Delay: IVC recovery delay after step series\nfired for auto approach.");
                                //mov_bp->new_line ();

                                mov_bp->grid_add_ec("Settle Time", Time, &mover_param.max_settling_time, 0., 10000., "6.1f", 1., 10., "Auto-App-Max-Settling-Time");
                                gtk_widget_set_tooltip_text (mov_bp->input, "Auto Approach Max Settlings Time: max allowed time to settle for feedback\nbefore declaring it to be finished.\n(max allowed time/cycle for FB-delta>0)");
                                mov_bp->new_line ();

                                mov_bp->grid_add_ec("Retract CI", Unity, &mover_param.retract_ci, 0., 500., "5.1f", 1., 10., "Auto-App-Retract-CI");
                                gtk_widget_set_tooltip_text (mov_bp->input, "Auto Approach Retract CI: Feeback CI used in inverse mode for retract tip, start with regular CI");
                                //mov_bp->new_line ();

                                mov_bp->set_pcs_remote_prefix (pcs_tab_remote_key_prefix[i]); // restore

                                mov_bp->grid_add_label ("---- V/ms");
                                mc_rampspeed_label = GTK_WIDGET(mov_bp->label);
                                gtk_widget_set_tooltip_text (mov_bp->label, "Besocke Ramp Speed -- press update button to recalculate.");
        
                                mov_bp->grid_add_widget (mov_bp->button = gtk_button_new_from_icon_name ("view-refresh-symbolic"));
                                g_object_set_data( G_OBJECT (mov_bp->button), "MoverNo", GINT_TO_POINTER (i));  
                                gtk_widget_set_tooltip_text (mov_bp->button, "Besocke Ramp Speed -- press update button to recalculate.");
                                g_signal_connect (G_OBJECT (mov_bp->button), "clicked",
	        				    G_CALLBACK (GVPMoverControl::RampspeedUpdate),
	        				    this);
                        }
                        mov_bp->set_configure_list_mode_off ();
                }

                mov_bp->pop_grid ();
                mov_bp->new_line ();
                mov_bp->new_grid_with_frame ("Direction & Action Control");
                mov_bp->set_xy (10,1); mov_bp->grid_add_ec ("  X=", Unity, &mover_param.axis_counts[i][0], -9999999, 9999999, ".0f", "axis-X"); ec_axis[0] = mov_bp->ec;
                mov_bp->set_xy (10,2); mov_bp->grid_add_ec ("  Y=", Unity, &mover_param.axis_counts[i][1], -9999999, 9999999, ".0f", "axis-Y"); ec_axis[1] = mov_bp->ec;
                mov_bp->set_xy (10,3); mov_bp->grid_add_ec ("  Z=", Unity, &mover_param.axis_counts[i][2], -9999999, 9999999, ".0f", "axis-Z"); ec_axis[2] = mov_bp->ec;

		// ========================================
		// Direction Buttons

		if( IS_MOVER_CTRL ){
                        if (i!=4){
                                mov_bp->set_xy (1,1);
                                mov_bp->push_grid ();
                                mov_bp->new_grid_with_frame (NULL, 5,3);
                                gtk_grid_set_row_homogeneous (GTK_GRID (mov_bp->grid), true);
                                gtk_grid_set_column_homogeneous (GTK_GRID (mov_bp->grid), true);
                        }
                        
			// STOP
			mov_bp->set_xy (3,2);
                        mov_bp->grid_add_widget (button = gtk_button_new_from_icon_name ("process-stopall-symbolic"));
                        g_object_set_data( G_OBJECT (button), "AXIS-X", ec_axis[0]);
                        g_object_set_data( G_OBJECT (button), "AXIS-Y", ec_axis[1]);
                        g_object_set_data( G_OBJECT (button), "AXIS-Z", ec_axis[2]);
			g_signal_connect (G_OBJECT (button), "clicked",
					    G_CALLBACK (GVPMoverControl::StopAction),
					    this);

                        if (i!=4)
                        {
                                // UP
	        		mov_bp->set_xy (3,1);
                                mov_bp->grid_add_fire_icon_button (
                                                                   "arrow-up-symbolic",
                                                                   GCallback (GVPMoverControl::CmdAction), this,
                                                                   GCallback (GVPMoverControl::StopAction), this);
                                button = mov_bp->icon;
                                //mov_bp->grid_add_widget (button = gtk_button_new_from_icon_name ("seek-backward-symbolic"));
	        		g_object_set_data( G_OBJECT (button), "GVP_cmd", GINT_TO_POINTER (GVP_CMD_AFM_MOV_YP));
	        		g_object_set_data( G_OBJECT (button), "MoverNo", GINT_TO_POINTER (i));
                                g_object_set_data( G_OBJECT (button), "AXIS-X", ec_axis[0]);
                                g_object_set_data( G_OBJECT (button), "AXIS-Y", ec_axis[1]);
                                g_object_set_data( G_OBJECT (button), "AXIS-Z", ec_axis[2]);
#if 0
	        		g_signal_connect (G_OBJECT (button), signal_name_button_pressed,
	        				    G_CALLBACK (GVPMoverControl::CmdAction),
	        				    this);
	        		g_signal_connect (G_OBJECT (button), signal_name_button_released,
	        				    G_CALLBACK (GVPMoverControl::StopAction),
	        				    this);
#endif

                                /*
	        		g_signal_connect(G_OBJECT(v_grid *** box), "key_press_event", 
	        				   G_CALLBACK(create_window_key_press_event_lcb), this);
	        		*/
	        		{ // pyremote hook
	        			remote_action_cb *ra = g_new( remote_action_cb, 1);
	        			ra -> cmd = g_strdup_printf("GVP_CMD_MOV-YP_%s",MoverNames[i]);
	        			ra -> RemoteCb = (void (*)(GtkWidget*, void*))GVPMoverControl::CmdAction;
	        			ra -> widget = button;
	        			ra -> data = this;
	        			main_get_gapp()->RemoteActionList = g_slist_prepend ( main_get_gapp()->RemoteActionList, ra );
	        			gchar *help = g_strconcat ("Remote example: action (\"", ra->cmd, "\"", NULL);
	        			gtk_widget_set_tooltip_text (button, help);
	        		}
                        }
//      gtk_widget_add_accelerator (button, "pressed", accel_group,
//                                  GDK_F3+4*i, (GdkModifierType)0,
//                                  GTK_ACCEL_VISIBLE);
                        else
                        {
                                // UP
	        		mov_bp->set_xy (3,1);
                                mov_bp->grid_add_fire_icon_button (
                                                                   "arrow-up-symbolic",
                                                                   GCallback (GVPMoverControl::CmdAction), this,
                                                                   GCallback (GVPMoverControl::StopAction), this);
                                button = mov_bp->icon;
                                //mov_bp->grid_add_widget (button = gtk_button_new_from_icon_name ("seek-backward-symbolic"));
	        		g_object_set_data( G_OBJECT (button), "GVP_cmd", GINT_TO_POINTER (GVP_CMD_AFM_MOV_ZP));
	        		g_object_set_data( G_OBJECT (button), "MoverNo", GINT_TO_POINTER (i));
                                g_object_set_data( G_OBJECT (button), "AXIS-X", ec_axis[0]);
                                g_object_set_data( G_OBJECT (button), "AXIS-Y", ec_axis[1]);
                                g_object_set_data( G_OBJECT (button), "AXIS-Z", ec_axis[2]);
#if 0
	        		g_signal_connect (G_OBJECT (button), signal_name_button_pressed,
	        				    G_CALLBACK (GVPMoverControl::CmdAction),
	        				    this);
	        		g_signal_connect (G_OBJECT (button), signal_name_button_released,
	        				    G_CALLBACK (GVPMoverControl::StopAction),
	        				    this);
#endif

	        		/*
	        		g_signal_connect(G_OBJECT(v_grid *** box), "key_press_event", 
	        				   G_CALLBACK(create_window_key_press_event_lcb), this);
	        		*/
	        		{ // pyremote hook
	        			remote_action_cb *ra = g_new( remote_action_cb, 1);
	        			ra -> cmd = g_strdup_printf("GVP_CMD_MOV-ZP_%s",MoverNames[i]);
	        			ra -> RemoteCb = (void (*)(GtkWidget*, void*))GVPMoverControl::CmdAction;
	        			ra -> widget = button;
	        			ra -> data = this;
	        			main_get_gapp()->RemoteActionList = g_slist_prepend ( main_get_gapp()->RemoteActionList, ra );
	        			gchar *help = g_strconcat ("Remote example: action (\"", ra->cmd, "\"", NULL);
	        			gtk_widget_set_tooltip_text (button, help);
	        		}
                        }

//      gtk_widget_add_accelerator (button, "pressed", accel_group,
//                                  GDK_F3+4*i, (GdkModifierType)0,
//                                  GTK_ACCEL_VISIBLE);
      
			if (i == 6 || i == 4){
                                mov_bp->set_xy (1,1); mov_bp->grid_add_label ("Z+");
                                mov_bp->set_xy (1,3); mov_bp->grid_add_label ("Z-");
			} else {
                                mov_bp->set_xy (1,2); mov_bp->grid_add_label ("X-");
                                mov_bp->set_xy (5,2); mov_bp->grid_add_label ("X+");
                                mov_bp->set_xy (1,1); mov_bp->grid_add_label ("Y+");
                                mov_bp->set_xy (1,3); mov_bp->grid_add_label ("Y-");
			}

			if (i!=6 && i!=4) {
                                //mov_bp->set_xy (2,1);
                                //mov_bp->grid_add_icon ("process-stopall-symbolic");
                                //mov_bp->set_xy (4,3);
                                //mov_bp->grid_add_icon ("process-stopall-symbolic");

                                // LEFT
                                mov_bp->set_xy (2,2);
                                mov_bp->grid_add_fire_icon_button (
                                                                   "arrow-left-symbolic",
                                                                   GCallback (GVPMoverControl::CmdAction), this,
                                                                   GCallback (GVPMoverControl::StopAction), this);
                                button = mov_bp->icon;
                                
                                //mov_bp->grid_add_widget (button = gtk_button_new_from_icon_name ("seek-left-symbolic"));
				g_object_set_data( G_OBJECT (button), "GVP_cmd", GINT_TO_POINTER (GVP_CMD_AFM_MOV_XM));
				g_object_set_data( G_OBJECT (button), "MoverNo", GINT_TO_POINTER (i));
                                g_object_set_data( G_OBJECT (button), "AXIS-X", ec_axis[0]);
                                g_object_set_data( G_OBJECT (button), "AXIS-Y", ec_axis[1]);
                                g_object_set_data( G_OBJECT (button), "AXIS-Z", ec_axis[2]);
#if 0
				g_signal_connect (G_OBJECT (button), signal_name_button_pressed,
						    G_CALLBACK (GVPMoverControl::CmdAction),
						    this);
				g_signal_connect (G_OBJECT (button), signal_name_button_released,
						    G_CALLBACK (GVPMoverControl::StopAction),
						    this);
#endif

				{ // pyremote hook
					remote_action_cb *ra = g_new( remote_action_cb, 1);
					ra -> cmd = g_strdup_printf("GVP_CMD_MOV-XM_%s",MoverNames[i]);
					ra -> RemoteCb = (void (*)(GtkWidget*, void*))GVPMoverControl::CmdAction;
					ra -> widget = button;
					ra -> data = this;
					main_get_gapp()->RemoteActionList = g_slist_prepend ( main_get_gapp()->RemoteActionList, ra );
					gchar *help = g_strconcat ("Remote example: action (\"", ra->cmd, "\"", NULL);
					gtk_widget_set_tooltip_text (button, help);
				}
	//      gtk_widget_add_accelerator (button, "pressed", accel_group,
	//                                  GDK_F1+4*i, (GdkModifierType)0,
	//                                  GTK_ACCEL_VISIBLE);

				// RIGHT
                                mov_bp->set_xy (4,2);
                                mov_bp->grid_add_fire_icon_button (
                                                                   "arrow-right-symbolic",
                                                                   GCallback (GVPMoverControl::CmdAction), this,
                                                                   GCallback (GVPMoverControl::StopAction), this);
                                // button = gtk_button_new_from_icon_name ("seek-right-symbolic");
                                button = mov_bp->icon;
                                //mov_bp->grid_add_widget (button = gtk_button_new_from_icon_name ("media-seek-right-symbolic"));
				g_object_set_data( G_OBJECT (button), "GVP_cmd", GINT_TO_POINTER (GVP_CMD_AFM_MOV_XP));
				g_object_set_data( G_OBJECT (button), "MoverNo", GINT_TO_POINTER (i));
                                g_object_set_data( G_OBJECT (button), "AXIS-X", ec_axis[0]);
                                g_object_set_data( G_OBJECT (button), "AXIS-Y", ec_axis[1]);
                                g_object_set_data( G_OBJECT (button), "AXIS-Z", ec_axis[2]);
#if 0
				g_signal_connect (G_OBJECT (button), signal_name_button_pressed,
						    G_CALLBACK (GVPMoverControl::CmdAction),
						    this);
				g_signal_connect (G_OBJECT (button), signal_name_button_released,
						    G_CALLBACK (GVPMoverControl::StopAction),
						    this);
#endif

				{ // pyremote hook
					remote_action_cb *ra = g_new( remote_action_cb, 1);
					ra -> cmd = g_strdup_printf("GVP_CMD_MOV-XP_%s",MoverNames[i]);
					ra -> RemoteCb = (void (*)(GtkWidget*, void*))GVPMoverControl::CmdAction;
					ra -> widget = button;
					ra -> data = this;
					main_get_gapp()->RemoteActionList = g_slist_prepend ( main_get_gapp()->RemoteActionList, ra );
					gchar *help = g_strconcat ("Remote example: action (\"", ra->cmd, "\"", NULL);
					gtk_widget_set_tooltip_text (button, help);
				}
			}
                        
// gtk_widget_get_toplevel()
//      gtk_widget_add_accelerator (button, "pressed", accel_group,
//                                  GDK_F1+4*i, (GdkModifierType)0,
//                                  GTK_ACCEL_VISIBLE);

                        if (i!=4)
                        {
        			// DOWN
                                mov_bp->set_xy (3,3);
                                mov_bp->grid_add_fire_icon_button (
                                                                   "arrow-down-symbolic",
                                                                   GCallback (GVPMoverControl::CmdAction), this,
                                                                   GCallback (GVPMoverControl::StopAction), this);
                                button = mov_bp->icon;
                                //mov_bp->grid_add_widget (button = gtk_button_new_from_icon_name ("seek-forward-symbolic"));
	        		g_object_set_data( G_OBJECT (button), "GVP_cmd", GINT_TO_POINTER (GVP_CMD_AFM_MOV_YM));
	        		g_object_set_data( G_OBJECT (button), "MoverNo", GINT_TO_POINTER (i));
                                g_object_set_data( G_OBJECT (button), "AXIS-X", ec_axis[0]);
                                g_object_set_data( G_OBJECT (button), "AXIS-Y", ec_axis[1]);
                                g_object_set_data( G_OBJECT (button), "AXIS-Z", ec_axis[2]);
#if 0
	        		g_signal_connect (G_OBJECT (button), signal_name_button_pressed,
	        				    G_CALLBACK (GVPMoverControl::CmdAction),
	        				    this);
	        		g_signal_connect (G_OBJECT (button), signal_name_button_released,
	        				    G_CALLBACK (GVPMoverControl::StopAction),
	        				    this);
#endif
                                
	        		{ // pyremote hook
	        			remote_action_cb *ra = g_new( remote_action_cb, 1);
                                        ra -> cmd = g_strdup_printf("GVP_CMD_MOV-YM_%s",MoverNames[i]);
	        			ra -> RemoteCb = (void (*)(GtkWidget*, void*))GVPMoverControl::CmdAction;
	        			ra -> widget = button;
	        			ra -> data = this;
	        			main_get_gapp()->RemoteActionList = g_slist_prepend ( main_get_gapp()->RemoteActionList, ra );
	        			gchar *help = g_strconcat ("Remote example: action (\"", ra->cmd, "\"", NULL);
	        			gtk_widget_set_tooltip_text (button, help);
	        		}
                        }
//      gtk_widget_add_accelerator (button, "pressed", accel_group,
//                                  GDK_F2+4*i, (GdkModifierType)0,
//                                  GTK_ACCEL_VISIBLE);
                        else
                        {
        			// DOWN
                                mov_bp->set_xy (3,3);
                                mov_bp->grid_add_fire_icon_button (
                                                                   "arrow-down-symbolic",
                                                                   GCallback (GVPMoverControl::CmdAction), this,
                                                                   GCallback (GVPMoverControl::StopAction), this);
                                button = mov_bp->icon;
                                //mov_bp->grid_add_widget (button = gtk_button_new_from_icon_name ("seek-forward-symbolic"));
	        		g_object_set_data( G_OBJECT (button), "GVP_cmd", GINT_TO_POINTER (GVP_CMD_AFM_MOV_ZM));
	        		g_object_set_data( G_OBJECT (button), "MoverNo", GINT_TO_POINTER (i));
                                g_object_set_data( G_OBJECT (button), "AXIS-X", ec_axis[0]);
                                g_object_set_data( G_OBJECT (button), "AXIS-Y", ec_axis[1]);
                                g_object_set_data( G_OBJECT (button), "AXIS-Z", ec_axis[2]);
#if 0
	        		g_signal_connect (G_OBJECT (button), "pressed",
	        				    G_CALLBACK (GVPMoverControl::CmdAction),
	        				    this);
	        		g_signal_connect (G_OBJECT (button), "released",
	        				    G_CALLBACK (GVPMoverControl::StopAction),
	        				    this);
#endif

	        		{ // pyremote hook
	        			remote_action_cb *ra = g_new( remote_action_cb, 1);
                                        ra -> cmd = g_strdup_printf("GVP_CMD_MOV-ZM_%s", MoverNames[i]);
	        			ra -> RemoteCb = (void (*)(GtkWidget*, void*))GVPMoverControl::CmdAction;
	        			ra -> widget = button;
	        			ra -> data = this;
	        			main_get_gapp()->RemoteActionList = g_slist_prepend ( main_get_gapp()->RemoteActionList, ra );
	        			gchar *help = g_strconcat ("Remote example: action (\"", ra->cmd, "\"", NULL);
	        			gtk_widget_set_tooltip_text (button, help);
	        		}
                        }
//      gtk_widget_add_accelerator (button, "pressed", accel_group,
//                                  GDK_F2+4*i, (GdkModifierType)0,
//                                  GTK_ACCEL_VISIBLE);

    
                        if (i!=4){
                                mov_bp->pop_grid ();
                        }
		}
    
		if(i==4){
			// ========================================
			// Auto App. Control Buttons  GTK_STOCK_GOTO_TOP  GTK_STOCK_GOTO_BOTTOM GTK_STOCK_MEDIA_STOP
			mov_bp->set_xy (6,1); mov_bp->grid_add_label ("Auto Control");

                        mov_bp->set_xy (6,2);
                        mov_bp->grid_add_widget (button = gtk_button_new_from_icon_name ("approach-symbolic"));
			gtk_widget_set_tooltip_text (button, "Start Auto Approaching. GPIO [Rot, Coarse Mode]");
			if(IS_MOVER_CTRL)
				g_object_set_data( G_OBJECT (button), "GVP_cmd", GINT_TO_POINTER (GVP_CMD_APPROCH_MOV_XP));
			else
				g_object_set_data( G_OBJECT (button), "GVP_cmd", GINT_TO_POINTER (GVP_CMD_APPROCH));

			g_object_set_data( G_OBJECT (button), "MoverNo", GINT_TO_POINTER (i));
                        g_object_set_data( G_OBJECT (button), "AXIS-X", ec_axis[0]);
                        g_object_set_data( G_OBJECT (button), "AXIS-Y", ec_axis[1]);
                        g_object_set_data( G_OBJECT (button), "AXIS-Z", ec_axis[2]);
			g_signal_connect (G_OBJECT (button), "clicked",
					    G_CALLBACK (GVPMoverControl::CmdAction),
					    this);
      
			{ // remote hook
				remote_action_cb *ra = g_new( remote_action_cb, 1);
				ra -> cmd = g_strdup_printf("GVP_CMD_AUTOAPP");
				ra -> RemoteCb = (void (*)(GtkWidget*, void*))GVPMoverControl::CmdAction;
				ra -> widget = button;
				ra -> data = this;
				main_get_gapp()->RemoteActionList = g_slist_prepend ( main_get_gapp()->RemoteActionList, ra );
				gchar *help = g_strconcat ("Remote example: action (\"", ra->cmd, "\"", NULL);
				gtk_widget_set_tooltip_text (button, help);
			}

                        mov_bp->set_xy (6,3);
                        mov_bp->grid_add_widget (button = gtk_button_new_from_icon_name ("system-off-symbolic"));
			gtk_widget_set_tooltip_text (button, "Cancel Auto Approach.");

			g_object_set_data( G_OBJECT (button), "GVP_cmd", GINT_TO_POINTER (GVP_CMD_CLR_PA));
			g_object_set_data( G_OBJECT (button), "MoverNo", GINT_TO_POINTER (i));
                        g_object_set_data( G_OBJECT (button), "AXIS-X", ec_axis[0]);
                        g_object_set_data( G_OBJECT (button), "AXIS-Y", ec_axis[1]);
                        g_object_set_data( G_OBJECT (button), "AXIS-Z", ec_axis[2]);
			g_signal_connect (G_OBJECT (button), "clicked",
					    G_CALLBACK (GVPMoverControl::CmdAction),
					    this);
      
			{ // remote hook
				remote_action_cb *ra = g_new( remote_action_cb, 1);
				ra -> cmd = g_strdup_printf("GVP_CMD_STOPALL");
				ra -> RemoteCb = (void (*)(GtkWidget*, void*))GVPMoverControl::CmdAction;
				ra -> widget = button;
				ra -> data = this;
				main_get_gapp()->RemoteActionList = g_slist_prepend ( main_get_gapp()->RemoteActionList, ra );
				gchar *help = g_strconcat ("Remote example: action (\"", ra->cmd, "\"", NULL);
				gtk_widget_set_tooltip_text (button, help);
			}

                        mov_bp->set_xy (5,3);
                        mov_bp->grid_add_widget (button = gtk_button_new_from_icon_name ("xymode-symbolic"));
			gtk_widget_set_tooltip_text (button, "Switch GPIO to XY Scan (Main) Mode.");

			g_object_set_data( G_OBJECT (button), "GVP_cmd", GINT_TO_POINTER (GVP_CMD_CLR_PA));
			g_object_set_data( G_OBJECT (button), "MoverNo", GINT_TO_POINTER (100));
			g_signal_connect (G_OBJECT (button), "clicked",
					    G_CALLBACK (GVPMoverControl::CmdAction),
					    this);
		}
		
		// STEPPER MOTOR controls (29/1/2013 dalibor.sulc@gmail.com) 		
		if(i==6){
			// ========================================
                        mov_bp->set_xy (6,1); mov_bp->grid_add_label ("Stepper Motor");
                        mov_bp->set_xy (6,2);
                        mov_bp->grid_add_widget (button = gtk_button_new_from_icon_name ("approach-symbolic"));
			gtk_widget_set_tooltip_text (button, "Start Auto Approaching. GPIO [Rot, Coarse Mode]");
			g_object_set_data( G_OBJECT (button), "MoverNo", GINT_TO_POINTER (i));
                        g_object_set_data( G_OBJECT (button), "AXIS-X", ec_axis[0]);
                        g_object_set_data( G_OBJECT (button), "AXIS-Y", ec_axis[1]);
                        g_object_set_data( G_OBJECT (button), "AXIS-Z", ec_axis[2]);
			g_signal_connect ( G_OBJECT (button), "clicked",
                                           G_CALLBACK (GVPMoverControl::CmdAction),
                                           this);

			if(IS_MOVER_CTRL)
				g_object_set_data( G_OBJECT (button), "GVP_cmd", GINT_TO_POINTER (GVP_CMD_APPROCH_MOV_XP));
			else
				g_object_set_data( G_OBJECT (button), "GVP_cmd", GINT_TO_POINTER (GVP_CMD_APPROCH));

      
			{ // remote hook
				remote_action_cb *ra = g_new( remote_action_cb, 1);
				ra -> cmd = g_strdup_printf("GVP_CMD_AUTOAPP_STEPPER");
				ra -> RemoteCb = (void (*)(GtkWidget*, void*))GVPMoverControl::CmdAction;
				ra -> widget = button;
				ra -> data = this;
				main_get_gapp()->RemoteActionList = g_slist_prepend ( main_get_gapp()->RemoteActionList, ra );
				gchar *help = g_strconcat ("Remote example: action (\"", ra->cmd, "\"", NULL);
				gtk_widget_set_tooltip_text (button, help);
			}

			mov_bp->set_xy (6,3);
                        mov_bp->grid_add_widget (button = gtk_button_new_from_icon_name ("system-off-symbolic"));
			gtk_widget_set_tooltip_text (button, "Stop Auto Approaching.");
			g_object_set_data( G_OBJECT (button), "GVP_cmd", GINT_TO_POINTER (GVP_CMD_CLR_PA));
			g_object_set_data( G_OBJECT (button), "MoverNo", GINT_TO_POINTER (i));
                        g_object_set_data( G_OBJECT (button), "AXIS-X", ec_axis[0]);
                        g_object_set_data( G_OBJECT (button), "AXIS-Y", ec_axis[1]);
                        g_object_set_data( G_OBJECT (button), "AXIS-Z", ec_axis[2]);
			g_signal_connect ( G_OBJECT (button), "clicked",
                                           G_CALLBACK (GVPMoverControl::CmdAction),
                                           this);
      
			{ // remote hook
				remote_action_cb *ra = g_new( remote_action_cb, 1);
				ra -> cmd = g_strdup_printf("GVP_CMD_STOPALL_STEPPER");
				ra -> RemoteCb = (void (*)(GtkWidget*, void*))GVPMoverControl::CmdAction;
				ra -> widget = button;
				ra -> data = this;
				main_get_gapp()->RemoteActionList = g_slist_prepend ( main_get_gapp()->RemoteActionList, ra );
				gchar *help = g_strconcat ("Remote example: action (\"", ra->cmd, "\"", NULL);
				gtk_widget_set_tooltip_text (button, help);
			}
		}

                mov_bp->notebook_tab_show_all ();
	}
  
	// ============================================================
	// save List away...
        g_object_set_data( G_OBJECT (window), "MOVER_EC_list", mov_bp->get_ec_list_head ());
        rpspmc_pacpll_hwi_pi.app->RemoteEntryList = g_slist_concat (rpspmc_pacpll_hwi_pi.app->RemoteEntryList, mov_bp->get_remote_list_head ());
        configure_callback (NULL, NULL, this); // configure "false"
        
        AppWindowInit (NULL); // stage two
        set_window_geometry ("dsp-mover-control");
}

void GVPMoverControl::update(){
	g_slist_foreach
		( (GSList*) g_object_get_data( G_OBJECT (window), "MOVER_EC_list"),
		  (GFunc) App::update_ec, NULL
			);
}

void GVPMoverControl::updateDSP(int sliderno){
	PI_DEBUG (DBG_L2, "update: Mover No:" << sliderno );

	if (sliderno == 100){
	        // auto offset ZERO, GPIO... N/A
		return;
	}

	if (mover_param.AFM_GPIO_setting == mover_param.GPIO_scan 
	    && ((sliderno >= 0 && sliderno < GVP_AFMMOV_MODES)? mover_param.AFM_GPIO_usr_setting[sliderno] : mover_param.MOV_GPIO_setting) != mover_param.GPIO_scan){
	        // auto offset ZERO
		//sranger_common_hwi->MovetoXY (0,0); // set
		//sranger_common_hwi->SetOffset (0,0); // wait for finish
		// GPIO... N/A
	}
	if (sliderno >= 0 && sliderno < GVP_AFMMOV_MODES){
	        // auto scan XY and offset to ZERO
                //sranger_common_hwi->MovetoXY (0,0);
		//sranger_common_hwi->SetOffset (0,0); // wait for finish
		mover_param.AFM_Amp   = mover_param.AFM_usrAmp  [sliderno];
		mover_param.AFM_Speed = mover_param.AFM_usrSpeed[sliderno];
		mover_param.AFM_Steps = mover_param.AFM_usrSteps[sliderno];
                // GPIO... N/A
	}
        g_settings_set_int (hwi_settings, "mover-gpio-last", mover_param.AFM_GPIO_setting);
}


int GVPMoverControl::config_waveform(GtkWidget *widget, GVPMoverControl *self){
        return self->configure_waveform (widget);
}

int GVPMoverControl::configure_waveform(GtkWidget *widget){
        if (gtk_combo_box_get_active_id (GTK_COMBO_BOX (widget)))
                mover_param.MOV_waveform_id = atoi (gtk_combo_box_get_active_id (GTK_COMBO_BOX (widget)));
        else
                PI_DEBUG_GP(DBG_L1, "ERROR: INVALID WAVE FORM SELECTED -- last [%d]\n", mover_param.MOV_waveform_id );
                
        int k=0;
        for (k=0; wave_form_options[k].wave_form_label; ++k)
                if (mover_param.MOV_waveform_id == wave_form_options[k].curve_id)
                        break;
        
        gtk_widget_set_tooltip_text (widget, wave_form_options[k].tool_tip_info);

        g_settings_set_int (hwi_settings, "mover-waveform-id", mover_param.MOV_waveform_id);

        PI_DEBUG_GP(DBG_L2, "Config Mover Wave Form: ID=%d  %s -- stored to settings.\n", mover_param.MOV_waveform_id,  wave_form_options[k].wave_form_label);
        
        gint nw = wave_form_options[k].num_waves;
	GSList *wc = mov_bp->get_configure_hide_list_b_head ();

        if (!wc) return 0;
        int i=7*6; // 7 widgets x 6 wave channels max
        if (!g_slist_nth_data (wc, i-1)) return 0;

        // 6 lines for waves. 7 widgets: Wave N: X, ###, Y, ###, Z, ###, wave-icon
        for (int k=0; k<nw; ++k){
                gtk_widget_queue_draw ((GtkWidget*) g_slist_nth_data (wc, i-1)); // update wave icon
                for (int q=0; q<7; ++q)
                        gtk_widget_show ((GtkWidget*) g_slist_nth_data (wc, --i+6));
        }
        for (int k=nw; k<6; ++k)
                for (int q=0; q<7; ++q)
                        gtk_widget_hide ((GtkWidget*) g_slist_nth_data (wc, --i+6));


        int q = 2*3 + 7*6; // 3 GPIO + 6x6 WXYZ
        if (mover_param.MOV_waveform_id == MOV_WAVE_SINE) {
                gtk_widget_hide ((GtkWidget*) g_slist_nth_data (wc, 0)); //GPIO Preset
                gtk_widget_hide ((GtkWidget*) g_slist_nth_data (wc, 1)); //GPIO L..
                gtk_widget_hide ((GtkWidget*) g_slist_nth_data (wc, 2)); //GPIO Poff
                gtk_widget_hide ((GtkWidget*) g_slist_nth_data (wc, 3)); //GPIO L..
                gtk_widget_hide ((GtkWidget*) g_slist_nth_data (wc, 4)); //GPIO Pon
                gtk_widget_hide ((GtkWidget*) g_slist_nth_data (wc, 5)); //GPIO L..
                gtk_widget_hide ((GtkWidget*) g_slist_nth_data (wc,   q)); //Besocke pt2
                gtk_widget_hide ((GtkWidget*) g_slist_nth_data (wc, 1+q)); //L
                gtk_widget_hide ((GtkWidget*) g_slist_nth_data (wc, 2+q)); //Besocke pt1
                gtk_widget_hide ((GtkWidget*) g_slist_nth_data (wc, 3+q)); //L
                gtk_widget_hide ((GtkWidget*) g_slist_nth_data (wc, 4+q)); //Besocke zj-xza
                gtk_widget_hide ((GtkWidget*) g_slist_nth_data (wc, 5+q)); //L
                gtk_widget_show ((GtkWidget*) g_slist_nth_data (wc, 6+q)); //Phase
                gtk_widget_show ((GtkWidget*) g_slist_nth_data (wc, 7+q)); //Phase L
        } else if (mover_param.MOV_waveform_id == MOV_WAVE_BESOCKE) {
                gtk_widget_hide ((GtkWidget*) g_slist_nth_data (wc, 0));
                gtk_widget_hide ((GtkWidget*) g_slist_nth_data (wc, 1));
                gtk_widget_hide ((GtkWidget*) g_slist_nth_data (wc, 2));
                gtk_widget_hide ((GtkWidget*) g_slist_nth_data (wc, 3));
                gtk_widget_hide ((GtkWidget*) g_slist_nth_data (wc, 4));
                gtk_widget_hide ((GtkWidget*) g_slist_nth_data (wc, 5));
                gtk_widget_show ((GtkWidget*) g_slist_nth_data (wc,   q));
                gtk_widget_show ((GtkWidget*) g_slist_nth_data (wc, 1+q));
                gtk_widget_show ((GtkWidget*) g_slist_nth_data (wc, 2+q));
                gtk_widget_show ((GtkWidget*) g_slist_nth_data (wc, 3+q));
                gtk_widget_show ((GtkWidget*) g_slist_nth_data (wc, 4+q));
                gtk_widget_show ((GtkWidget*) g_slist_nth_data (wc, 5+q));
                gtk_widget_hide ((GtkWidget*) g_slist_nth_data (wc, 6+q));
                gtk_widget_hide ((GtkWidget*) g_slist_nth_data (wc, 7+q));
        } else if (mover_param.MOV_waveform_id == MOV_WAVE_GPIO) {
                gtk_widget_show ((GtkWidget*) g_slist_nth_data (wc, 0));
                gtk_widget_show ((GtkWidget*) g_slist_nth_data (wc, 1));
                gtk_widget_show ((GtkWidget*) g_slist_nth_data (wc, 2));
                gtk_widget_show ((GtkWidget*) g_slist_nth_data (wc, 3));
                gtk_widget_show ((GtkWidget*) g_slist_nth_data (wc, 4));
                gtk_widget_show ((GtkWidget*) g_slist_nth_data (wc, 5));
                gtk_widget_hide ((GtkWidget*) g_slist_nth_data (wc,   q));
                gtk_widget_hide ((GtkWidget*) g_slist_nth_data (wc, 1+q));
                gtk_widget_hide ((GtkWidget*) g_slist_nth_data (wc, 2+q));
                gtk_widget_hide ((GtkWidget*) g_slist_nth_data (wc, 3+q));
                gtk_widget_hide ((GtkWidget*) g_slist_nth_data (wc, 4+q));
                gtk_widget_hide ((GtkWidget*) g_slist_nth_data (wc, 5+q));
                gtk_widget_hide ((GtkWidget*) g_slist_nth_data (wc, 6+q));
                gtk_widget_hide ((GtkWidget*) g_slist_nth_data (wc, 7+q));
        } else {
                gtk_widget_hide ((GtkWidget*) g_slist_nth_data (wc, 0));
                gtk_widget_hide ((GtkWidget*) g_slist_nth_data (wc, 1));
                gtk_widget_hide ((GtkWidget*) g_slist_nth_data (wc, 2));
                gtk_widget_hide ((GtkWidget*) g_slist_nth_data (wc, 3));
                gtk_widget_hide ((GtkWidget*) g_slist_nth_data (wc, 4));
                gtk_widget_hide ((GtkWidget*) g_slist_nth_data (wc, 5));
                gtk_widget_hide ((GtkWidget*) g_slist_nth_data (wc,   q));
                gtk_widget_hide ((GtkWidget*) g_slist_nth_data (wc, 1+q));
                gtk_widget_hide ((GtkWidget*) g_slist_nth_data (wc, 2+q));
                gtk_widget_hide ((GtkWidget*) g_slist_nth_data (wc, 3+q));
                gtk_widget_hide ((GtkWidget*) g_slist_nth_data (wc, 4+q));
                gtk_widget_hide ((GtkWidget*) g_slist_nth_data (wc, 5+q));
                gtk_widget_show ((GtkWidget*) g_slist_nth_data (wc, 6+q));
                gtk_widget_show ((GtkWidget*) g_slist_nth_data (wc, 7+q));
        }        

        return 1;
}


void GVPMoverControl::wave_preview_draw_function (GtkDrawingArea *area, cairo_t *cr,
                                                  int             width,
                                                  int             height,
                                                  GVPMoverControl *self){

        // prepare job lookups
        PROBE_VECTOR_GENERIC v = { 0,0.,0,0, 0,0,0,  0.0, 0.0, 0.0, 0.0, 0.0, 0.0 };
        double *gvp_y[6] = { NULL, NULL, NULL, NULL, NULL, NULL };
        int k=0;
        // find wave form index by id
        for (k=0; wave_form_options[k].wave_form_label; ++k)
                if (self->mover_param.MOV_waveform_id == wave_form_options[k].curve_id)
                        break;
        // get num waves
        int ns=wave_form_options[k].num_waves; // num selected channels
        if (ns < 1) return;

        // wave form to preview
        int wn = GPOINTER_TO_INT (g_object_get_data  (G_OBJECT (area), "wave_ch"));

        // setup lookups
        for (int i=1; i<=6; ++i){
                switch (i){
                case 1: gvp_y[0] = &v.f_dx; break;
                case 2: gvp_y[1] = &v.f_dy; break;
                case 3: gvp_y[2] = &v.f_dz; break;
                case 4: gvp_y[3] = &v.f_du; break;
                case 5: gvp_y[4] = &v.f_da; break;
                case 6: gvp_y[5] = &v.f_db; break;
                }
        }

        // setup drawing
        int m = 64;
        int n = 256;
        int mar=0;
        gtk_drawing_area_set_content_width (area, n+10);
        gtk_drawing_area_set_content_height (area, m+6);

        g_message ("wave_preview_draw_function  %d x %d", n+2, m);
        
        cairo_translate (cr, 5.,3+ m/2);
        cairo_scale (cr, 1., 1.);
        cairo_save (cr);

        cairo_item_path *sec = new cairo_item_path (2);
        sec->set_line_width (0.5);
        sec->set_stroke_rgba (CAIRO_COLOR_BLACK);
        sec->set_xy_fast (0,0,0);
        sec->set_xy_fast (1,n-1,0);
        sec->draw (cr);
        sec->set_stroke_rgba (CAIRO_COLOR_BLACK);
        sec->set_xy_fast (0,0,(m-mar)/2);
        sec->set_xy_fast (1,n-1,(m-mar)/2);
        sec->draw (cr);
        sec->set_stroke_rgba (CAIRO_COLOR_BLACK);
        sec->set_xy_fast (0,0,-(m-mar)/2);
        sec->set_xy_fast (1,n-1,-(m-mar)/2);
        sec->draw (cr);
        sec->set_stroke_rgba (CAIRO_COLOR_GREEN);
        
        // create GVP and simulate
        for (int j=0; j<2; ++j){ // fwd, rev
                // create GVP
                int axis = 0;
                int vch = self->mover_param.wave_out_channel_xyz[wn][axis]-1;
                if (vch < 0 || vch > 5){
                        g_message ("DBG: k %d, wn %d", k, wn);
                        g_message ("DBG: gvp_ch %d", self->mover_param.wave_out_channel_xyz[k][wn]);
                        g_warning ("==> Wrong GVP DAC Channel selected: %d, fallback to ch=1 for display", vch+1);
                        vch=0;
                }

                self->create_waveform (1., 5., 2, j==0 ? 1 : -1, axis); // preview params only, scale 1V, 5ms, 2 cycles, Fwd/Rev, axis=0
                int N=RPSPMC_ControlClass->calculate_GVP_total_number_points();

                if ( N > 1){
                        cairo_item_path *gvp_wave = new cairo_item_path (N);
                        gvp_wave->set_line_width (2.0);
                        gvp_wave->set_stroke_rgba (j==0 ? CAIRO_COLOR_RED : CAIRO_COLOR_BLUE);

                        // simulate GVP
                        double t=0;
                        int pc=0;
                        int pcp=-1;
                        double ti=-1.;
                        double Tfin = RPSPMC_ControlClass->simulate_vector_program(N, &v, &pc);
                        pc=0;
                        for (int i=0; i<N; i++){
                                memset (&v, 0, sizeof(v));
                                t =  RPSPMC_ControlClass->simulate_vector_program(i, &v, &pc);
                                gvp_wave->set_xy_fast (i, n*t/Tfin, -(*gvp_y[vch]));
                                if (pc == 1 && ti < 0.) ti = t;
                                if (pcp != pc || i == N-1){ // section marks
                                        g_message ("SSS %d %d %g %g", pc, i, n*t/Tfin, t);
                                        sec->set_xy_fast (0,n*t/Tfin,(m-mar)/2);
                                        sec->set_xy_fast (1,n*t/Tfin,-(m-mar)/2);
                                        sec->draw (cr);
                                        pcp = pc;
                                }
                        }
                                
                        double skl = -(m-mar)/2.;

                        // auto scaling, draw wave
                        double gvpy_amax;
                        gvpy_amax = gvp_wave->auto_range_y (skl);
                        gvp_wave->draw (cr);
                        delete gvp_wave;

                        cairo_item_text *tms = new cairo_item_text ();
                        tms->set_font_face_size ("Ununtu", 8.);
                        tms->set_anchor (CAIRO_ANCHOR_W);
                        tms->set_stroke_rgba (CAIRO_COLOR_BLACK);
                        const gchar* lab=g_strdup_printf("%.2g ms",1e3*ti/2);
                        tms->set_text (n*ti/Tfin, (m-mar)/2-8, lab);
                        tms->draw (cr);
                        g_free (lab);
                        lab=g_strdup_printf("%.2g ms",1e3*(t-ti)/2);
                        tms->set_text (n*t/Tfin/2, (m-mar)/2-8, lab);
                        tms->draw (cr);
                        g_free (lab);
                        delete tms;

                }
        }
        delete sec;
}

void GVPMoverControl::updateAxisCounts (GtkWidget* w, int idx, int cmd){
        if (idx >=0 && idx < GVP_AFMMOV_MODES){
                Gtk_EntryControl *eaxis[3];
                eaxis[0] = (Gtk_EntryControl*) g_object_get_data( G_OBJECT (w), "AXIS-X");
                eaxis[1] = (Gtk_EntryControl*) g_object_get_data( G_OBJECT (w), "AXIS-Y");
                eaxis[2] = (Gtk_EntryControl*) g_object_get_data( G_OBJECT (w), "AXIS-Z");

                if (eaxis[0] && eaxis[1] && eaxis[2]){
                        double axis[3];
                        gchar *info;
#if 0
                        sranger_common_hwi->RTQuery ("A", axis[0], axis[1], axis[2]);
                        // g_message ("GVPMoverControl::updateAxisCounts idx=%d cmd=%d  [%g, %g, %g]", idx, cmd,  axis[0], axis[1], axis[2]);
                        for (int i=0; i<3; ++i){
                                if (cmd == 0){
                                        // relative counts pre tab
                                        mover_param.axis_counts[idx][i] += (int)axis[i] - mover_param.axis_counts_ref[idx][i];
                                        eaxis[i]->Put_Value ();
                                        mover_param.axis_counts_ref[idx][i] = (int)axis[i];
                                } else {
                                        mover_param.axis_counts_ref[idx][i] = (int)axis[i];
                                }
                                info = g_strdup_printf (" [%.0f]", axis[i]);
                                eaxis[i]->set_info (info);
                                g_free (info);
                        }
#endif
                }
        }
}

int GVPMoverControl::CmdAction(GtkWidget *widget, GVPMoverControl *self){
	int idx=-1;
	int cmd;
	PI_DEBUG (DBG_L2, "MoverCrtl::CmdAction " ); 

	if(IS_MOVER_CTRL)
		idx = GPOINTER_TO_INT(g_object_get_data( G_OBJECT (widget), "MoverNo"));

        // not GPIO on RPSPMC, so no selection (idx)
        // ...
        
	cmd = GPOINTER_TO_INT(g_object_get_data( G_OBJECT (widget), "GVP_cmd"));

        g_message ("MOVER CMD ACTION: on mover id %d (no selection yet via RPSPMC) CMD: %d", idx, cmd);
        
        switch (cmd){
        case GVP_CMD_AFM_MOV_XM:
                self->mover_param.MOV_axis = 0;
                self->mover_param.MOV_pointing = -1.;
                self->mover_param.MOV_angle = 180.;
                break;
        case GVP_CMD_AFM_MOV_XP:
                self->mover_param.MOV_axis = 0;
                self->mover_param.MOV_pointing = 1.;
                self->mover_param.MOV_angle = 0.;
                break;
        case GVP_CMD_AFM_MOV_YM:
                self->mover_param.MOV_axis = 1;
                self->mover_param.MOV_pointing = -1.;
                self->mover_param.MOV_angle = -90.;
                break;
        case GVP_CMD_AFM_MOV_YP:
                self->mover_param.MOV_axis = 1;
                self->mover_param.MOV_pointing = 1.;
                self->mover_param.MOV_angle = 90.;
                break;
        case GVP_CMD_AFM_MOV_ZM:
                self->mover_param.MOV_axis = 3;
                self->mover_param.MOV_pointing = -1.;
                self->mover_param.MOV_angle = -200.;
                break;
        case GVP_CMD_AFM_MOV_ZP:
                self->mover_param.MOV_axis = 3;
                self->mover_param.MOV_pointing = 1.;
                self->mover_param.MOV_angle = 200.;
                break;
        default:
                self->mover_param.MOV_axis = 3;
                self->mover_param.MOV_pointing = 1.;
                self->mover_param.MOV_angle = 200.;
                break;
        }
        
        self->updateAxisCounts (widget, idx, cmd);

        if(cmd>0){
		// self->ExecCmd(GVP_CMD_GPIO_SETUP); // no GPIO on RPSPMC available
		self->ExecCmd(cmd);
	}
	return 0;
}

int GVPMoverControl::StopAction(GtkWidget *widget, GVPMoverControl *self){
	PI_DEBUG (DBG_L2, "GVPMoverControl::StopAction" );

	// GVP STOP
        rpspmc_hwi->GVP_abort_vector_program ();
        RPSPMC_ControlClass->GVP_zero_all_smooth ();
         
        //self->updateAxisCounts (widget, idx, 0);

	return 0;
}

int GVPMoverControl::RampspeedUpdate(GtkWidget *widget, GVPMoverControl *self){

        int idx = GPOINTER_TO_INT(g_object_get_data( G_OBJECT (widget), "MoverNo"));
        self->updateDSP(idx);

        PI_DEBUG_GP (DBG_L2, "GVPMoverControl::RampspeedUpdate t1=%f  t2=%f  duration=%f  amp=%f  idx=%d \n", 
                                        self->mover_param.time_delay_1, 
                                        self->mover_param.time_delay_2, 
                                        self->mover_param.AFM_Speed, 
                                        self->mover_param.AFM_Amp,
                                        idx);

        // new besocke ramp speed
        gchar *tmp = g_strdup_printf ("%.2f V/ms",
                                      (self->mover_param.AFM_Amp/2)
                                      / ((self->mover_param.AFM_Speed - 2*self->mover_param.time_delay_1 - self->mover_param.time_delay_2)/2));
        gtk_label_set_text (GTK_LABEL(self->mc_rampspeed_label), tmp);
        g_free (tmp);

        return 0;
}

void GVPMoverControl::ChangedNotify(Param_Control* pcs, gpointer self){
	int idx=-1;
	gchar *us=pcs->Get_UsrString();
	PI_DEBUG (DBG_L2, "MoverCrtl:: Param Changed: " << us );
	g_free(us);

	if(IS_MOVER_CTRL)
		idx = GPOINTER_TO_INT(pcs->GetEntryData("MoverNo"));

        ((GVPMoverControl*)self)->updateDSP(idx);
}

void GVPMoverControl::ExecCmd(int cmd){
	PI_DEBUG (DBG_L2, "GVPMoverControl::ExecCmd ==> >" << cmd);

        create_waveform (mover_param.AFM_Amp, mover_param.AFM_Speed, mover_param.AFM_Steps, mover_param.MOV_pointing, mover_param.MOV_axis);
        rpspmc_hwi->start_data_read (0, 0,0,0,0, NULL,NULL,NULL,NULL);

}
