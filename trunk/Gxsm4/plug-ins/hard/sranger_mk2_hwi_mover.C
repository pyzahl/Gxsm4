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



#include <locale.h>
#include <libintl.h>

#include <time.h>

#include "core-source/glbvars.h"
#include "modules/dsp.h"
#include <fcntl.h>
#include <sys/ioctl.h>

#include "core-source/gxsm_app.h"
#include "core-source/gxsm_window.h"
#include "core-source/action_id.h"

#include "xsmcmd.h"
#include "sranger_mk2_hwi_control.h"
#include "sranger_mk23common_hwi.h"
#include "modules/sranger_mk2_ioctl.h"
#include "../common/pyremote.h"

#define UTF8_DEGREE    "\302\260"
#define UTF8_MU        "\302\265"
#define UTF8_ANGSTROEM "\303\205"

extern GxsmPlugin sranger_mk2_hwi_pi;
extern DSPControl *DSPControlClass;
extern sranger_common_hwi_dev *sranger_common_hwi; // instance of the HwI derived XSM_Hardware class
DSPMoverControl *this_mover_control=NULL;	 
extern DSPPACControl *DSPPACClass;

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
        //{ MOV_WAVE_CYCLO_IPL, "Inv. Cycloide+", "Support for stick-slip motion based on a cycloidic signal, inverted - positve values.", 1 },
        //{ MOV_WAVE_CYCLO_IMI, "Inv. Cycloide-", "Support for stick-slip motion based on a cycloidic signal, inverted - negative values.", 1 },
        { MOV_WAVE_KOALA, "Koala", "Support for Koala type STM heads.", 2 },
        { MOV_WAVE_BESOCKE, "Besocke", "Support for bettle type STM heads with 3fold segmented piezos for the coarse approach without inner z-electrode.", 3 },
        { MOV_WAVE_PULSE, "Pulse",	 "Support for arbitrary rectangular pulses.", 1 },

        { MOV_WAVE_STEP, "Stepfunc", "Support for step function.", 1 },
        // Pulses are generated as arbitrary wave forms (stm, 08.10.2010)
        // positive pluse: base line is zero, pulse as height given by amplitude
        // ratio between on/off: 1:1
        // total time = ton + toff

        { MOV_WAVE_STEPPERMOTOR, "Stepper Motor", "Support for stepper motor (2 channel).", 2 },
        { MOV_WAVE_GPIO, "(none) GPIO Mode", "Custom GPIO", 0 },
        { -1, NULL, NULL, 0 },
};


class MOV_GUI_Builder : public BuildParam{
public:
        MOV_GUI_Builder (GtkWidget *build_grid=NULL, GSList *ec_list_start=NULL, GSList *ec_remote_list=NULL) :
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




static void DSPMoverControl_Freeze_callback (gpointer x){	 
	if (this_mover_control)	 
		this_mover_control->freeze ();	 
}	 
static void DSPMoverControl_Thaw_callback (gpointer x){	 
	if (this_mover_control)	 
		this_mover_control->thaw ();	 
}

DSPMoverControl::DSPMoverControl ()
{
	XsmRescourceManager xrm("sranger_mk2_hwi_control");

        hwi_settings = g_settings_new (GXSM_RES_BASE_PATH_DOT".hwi.sranger-mk23-mover");
        mover_param.AFM_GPIO_setting = g_settings_get_int (hwi_settings, "mover-gpio-last");
       
	PI_DEBUG (DBG_L2, "DSPMoverControl::DSPMoverControl xrm read mk2 mover settings");

	xrm.Get("AFM_Amp", &mover_param.AFM_Amp, "1");
	xrm.Get("AFM_Speed", &mover_param.AFM_Speed, "3");
	xrm.Get("AFM_Steps", &mover_param.AFM_Steps, "10");
	// xrm.Get("AFM_GPIO_setting", &mover_param.AFM_GPIO_setting, "0");

  	// defaults presets storage for "Besocke" style coarse motions
	// [0..5] Besocke XY, Besocke Rotation, Besocke PSD, Besocke Lens, Auto, Stepper Motor

        for (int i=0; i< DSP_AFMMOV_MODES; ++i){
                gchar *id=g_strdup_printf ("AFM_usrAmp%d", i);
                xrm.Get(id, &mover_param.AFM_usrAmp[i], "1.0"); g_free (id);
                id=g_strdup_printf ("AFM_usrSpeed%d", i);
                xrm.Get(id, &mover_param.AFM_usrSpeed[i], "3"); g_free (id);
                id=g_strdup_printf ("AFM_usrSteps%d", i);
                xrm.Get(id, &mover_param.AFM_usrSteps[i], "10"); g_free (id);
                id=g_strdup_printf ("AFM_GPIO_setting%d", i);
                xrm.Get(id, &mover_param.AFM_GPIO_usr_setting[i], "0"); g_free (id);

                mover_param.axis_counts[i][0] = 0;
                mover_param.axis_counts[i][1] = 0;
                mover_param.axis_counts[i][2] = 0;
        }

        mover_param.wave_out_channels_used=6;
	for (int k=0; k<mover_param.wave_out_channels_used; ++k)
                mover_param.wave_out_channel_xyz[k][0]=mover_param.wave_out_channel_xyz[k][1]=mover_param.wave_out_channel_xyz[k][2]=0;

        mover_param.wave_out_channel_xyz[0][0]=3, mover_param.wave_out_channel_xyz[0][1]=4, mover_param.wave_out_channel_xyz[0][2]=3;

        xrm.Get("MOV_output", &mover_param.MOV_output, "0");
	xrm.Get("MOV_waveform_id", &mover_param.MOV_waveform_id, "0");

        mover_param.MOV_waveform_id = g_settings_get_int (hwi_settings, "mover-waveform-id");

        xrm.Get("AUTO_final_delay", &mover_param.final_delay, "50");
	xrm.Get("AUTO_max_settling_time", &mover_param.max_settling_time, "1000");
	xrm.Get("InchWorm_phase", &mover_param.inch_worm_phase, "0");
        
        xrm.Get("timedelay_2", &mover_param.time_delay_2, "0.09");
        xrm.Get("timedelay_1", &mover_param.time_delay_1, "0.09");
        xrm.Get("z_Rate", &mover_param.z_Rate, "0.1");
	xrm.Get("Wave_space", &mover_param.Wave_space, "0.0");
	xrm.Get("Wave_offset", &mover_param.Wave_offset, "0.0");

	xrm.Get("GPIO_on", &mover_param.GPIO_on, "0");
	xrm.Get("GPIO_off", &mover_param.GPIO_off, "0");
	xrm.Get("GPIO_reset", &mover_param.GPIO_reset, "0");
	xrm.Get("GPIO_scan", &mover_param.GPIO_scan, "0");
	xrm.Get("GPIO_tmp1", &mover_param.GPIO_tmp1, "0");
	xrm.Get("GPIO_tmp2", &mover_param.GPIO_tmp2, "0");
	xrm.Get("GPIO_direction", &mover_param.GPIO_direction, "0");
	xrm.Get("GPIO_delay", &mover_param.GPIO_delay, "250.0");

	xrm.Get("Z0_speed", &Z0_speed, "500.");
	xrm.Get("Z0_adjust", &Z0_adjust, "500.");
	xrm.Get("Z0_goto", &Z0_goto, "0.");

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
	PI_DEBUG (DBG_L2, "DSPMoverControl::DSPMoverControl create folder");
	create_folder ();
	PI_DEBUG (DBG_L2, "... OK");
	this_mover_control=this;	 
	sranger_mk2_hwi_pi.app->ConnectPluginToStartScanEvent (DSPMoverControl_Freeze_callback);	 
	sranger_mk2_hwi_pi.app->ConnectPluginToStopScanEvent (DSPMoverControl_Thaw_callback);
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

// duration in ms
// amp in V SR out (+/-2.05V max)
int DSPMoverControl::create_waveform (double amp, double duration, int space){
        gint space_len = 0;
#define SR_VFAC    (32767./10.00) // A810 max Volt out is 10V
        for (int i=0; i < MOV_MAXWAVELEN; ++i)
		mover_param.MOV_waveform[i] = 0;

        double pointing = duration > 0. ? 1. : -1.;
        gint channels = 1;

        // check and find wave valid wave form id "k"
        int k=0;
        for (k=0; wave_form_options[k].wave_form_label; ++k)
                if (mover_param.MOV_waveform_id == wave_form_options[k].curve_id)
                        break;
        // lookup num waves to compute
        channels = wave_form_options[k].num_waves;

        PI_DEBUG_GP(DBG_L2, "DSPMoverControl::create_waveform ID=%d  %s #CH=%d\n",
                    mover_param.MOV_waveform_id,  wave_form_options[k].wave_form_label,
                    channels);

        if (space >= 0)
                space_len = channels * space;
        else
                space_len = channels * (gint)round ( DSPControlClass->frq_ref* mover_param.Wave_space*1e-3); 

        PI_DEBUG_GP(DBG_L2, "DSPMoverControl::create_waveform (frq_ref, %f\n)", DSPControlClass->frq_ref);       

	mover_param.MOV_wave_len = channels * (gint)round (DSPControlClass->frq_ref*fabs (duration)*1e-3);
	mover_param.MOV_wave_speed_fac = 1;

	while (mover_param.MOV_wave_len/mover_param.MOV_wave_speed_fac >= MOV_MAXWAVELEN)
		++mover_param.MOV_wave_speed_fac;

	mover_param.MOV_wave_len /= mover_param.MOV_wave_speed_fac;
	space_len /= mover_param.MOV_wave_speed_fac;

	if (mover_param.MOV_wave_len < 2*channels)
		mover_param.MOV_wave_len = 2*channels;

	PI_DEBUG_GP (DBG_L2, "DSPMoverControl::create_waveform (total samples, all %d channels): %d x %d, p=%g ms A=%g V \n",
                     channels,
                     mover_param.MOV_wave_len + space_len,
                     mover_param.MOV_wave_speed_fac,
                     duration, 
                     amp
                     );        

        int    imax = mover_param.MOV_wave_len-channels;
        int    imax1 = mover_param.MOV_wave_len-channels;
	double kn = (double)mover_param.MOV_wave_len;
	double n = kn / channels;
	double n2 = n/2.;
	double t=0.;

        double phase = mover_param.inch_worm_phase/360.;
        int   iphase = (int)(phase*kn);

	switch (mover_param.MOV_waveform_id){
	case MOV_WAVE_SAWTOOTH:
                PI_DEBUG_GP (DBG_L2, " ** SAWTOOTH WAVEFORM CALC\n");
                if (pointing > 0) // wave for forward direction
                        for (int i=0; i < mover_param.MOV_wave_len; i += channels, t+=1.){
                                for (int k=0; k<channels; ++k)
                                        mover_param.MOV_waveform[i+k] = (short)round (SR_VFAC*amp*(((double)(t<n2? t : t-n)/n2)+mover_param.Wave_offset));
                        }
                else // wave for reverse direction
                        for (int i=0; i < mover_param.MOV_wave_len; i += channels, t+=1.){
                                for (int k=0; k<channels; ++k)
                                        mover_param.MOV_waveform[i+k] = (short)round (SR_VFAC*amp*(((double)(t<n2? -t : n-t)/n2)+mover_param.Wave_offset));
                        }
		break;
	case MOV_WAVE_SINE:
                PI_DEBUG_GP (DBG_L2, " ** SINE WAVEFORM CALC\n");
                if (pointing > 0) // wave for forward direction
                        for (int i=0; i < mover_param.MOV_wave_len; i += channels, t+=1.)
                                for (int k=0; k<channels; ++k)
                                        mover_param.MOV_waveform[i+k] = (short)round (SR_VFAC*amp*((sin (k*phase*2.0*M_PI + (double)t*2.*M_PI/n))+mover_param.Wave_offset));
                else
                        for (int i=0; i < mover_param.MOV_wave_len; i += channels, t+=1.)
                                for (int k=0; k<channels; ++k)
                                        mover_param.MOV_waveform[i+k] = (short)round (SR_VFAC*amp*((sin (k*phase*2.0*M_PI - (double)t*2.*M_PI/n))+mover_param.Wave_offset));
		break;
	case MOV_WAVE_CYCLO:
	case MOV_WAVE_CYCLO_PL:
	case MOV_WAVE_CYCLO_MI:
                PI_DEBUG_GP (DBG_L2, " ** CYCLO* CALC\n");
                if (pointing < 0)
                        amp = -amp; // invert
 		for (int i=0; i < mover_param.MOV_wave_len; i += channels){
			double dt = 1./(n - 1);
			switch (mover_param.MOV_waveform_id){
			case MOV_WAVE_CYCLO_PL:
                                for (int k=0; k<channels; ++k){
                                short v = 0;
                                if(pointing > 0)
                                        v = (short)round(SR_VFAC*amp*(t*t*t*t));
                                else
                                        v = (short)round(-SR_VFAC*amp*(1.-t*t*t*t));
                                mover_param.MOV_waveform[i+k] = v;
                                }
				t += dt;
				break;	
			case MOV_WAVE_CYCLO_MI:
                                for (int k=0; k<channels; ++k){
                                short v = 0;                                
                                if(pointing > 0)
                                        v = (short)round(-SR_VFAC*amp*(t*t*t*t));
                                else
                                        v = (short)round(SR_VFAC*amp*(1.-t*t*t*t));
                                mover_param.MOV_waveform[i+k] = v;
                                }
				t += dt;
				break;
			default:
                                if (i == mover_param.MOV_wave_len/2)
                                        amp = -amp; // jump
				dt = M_PI/2./(mover_param.MOV_wave_len - 1.)*2;
                                for (int k=0; k<channels; ++k){
                                        short v = (short)round(SR_VFAC*amp*( (1.-cos (t))));
                                        mover_param.MOV_waveform[i+k] = v;
                                }
				if (i < (mover_param.MOV_wave_len/2))
					t += dt;
				else
					t -= dt;
				break;
			}
		}
		break;
// removed inverse cycloidic waveforms as the are not used by anyone. (stm, 2.4.2018)
#if 0
	case MOV_WAVE_CYCLO_IPL:
	case MOV_WAVE_CYCLO_IMI:
                PI_DEBUG_GP (DBG_L2, " ** CYCLO_I* CALC\n");
		for (int i=0; i < mover_param.MOV_wave_len; i += channels){
			double dt = pointing/(n - 1.);
			double a = 1.;
			switch (mover_param.MOV_waveform_id){
			case MOV_WAVE_CYCLO_IPL:
                                for (int k=0; k<channels; ++k){
                                        short v = (short)round (SR_VFAC*amp*a*( (t*t*t*t) - 1 ));
                                        if (pointing > 0) // wave for forward direction 
                                                mover_param.MOV_waveform[i+k] = v;
                                        else
                                                mover_param.MOV_waveform[imax-i+k] = v;
                                }
				break;	
			case MOV_WAVE_CYCLO_IMI:
                                for (int k=0; k<channels; ++k){
                                        short v = (short)round (SR_VFAC*amp*a*( (t*t*t*t)));
                                        if (pointing > 0) // wave for forward direction 
                                                mover_param.MOV_waveform[i+k] = v;
                                        else
                                                mover_param.MOV_waveform[imax-i+k] = v;
                                }
				break;
			}
			t += dt;
		}
		break;
#endif
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
        }

        PI_DEBUG_GP (DBG_L2, " ** ADDING SPACE\n");
	// terminate with spacing using 1st sample
	for (int i = mover_param.MOV_wave_len; i < mover_param.MOV_wave_len+space_len; i += channels)
                for (int k=0; k < channels; ++k)
                        if ((i+k) < MOV_MAXWAVELEN)
                                mover_param.MOV_waveform[i+k] = mover_param.MOV_waveform[k];

	mover_param.MOV_wave_len += space_len;

        if (mover_param.MOV_wave_len >= MOV_MAXWAVELEN)
                mover_param.MOV_wave_len = MOV_MAXWAVELEN-1;
	
        return channels;
}


DSPMoverControl::~DSPMoverControl (){        
        XsmRescourceManager xrm("sranger_mk2_hwi_control");        
        xrm.Put("MOV_waveform_id", mover_param.MOV_waveform_id);
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

static GActionEntry win_DSPMover_popup_entries[] = {
        { "dsp-mover-configure", DSPMoverControl::configure_callback, NULL, "true", NULL },
};

void DSPMoverControl::configure_callback (GSimpleAction *action, GVariant *parameter, 
                                          gpointer user_data){
        DSPMoverControl *mc = (DSPMoverControl *) user_data;
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
                          (GFunc) DSPMoverControl::show_tab_to_configure, NULL
                          );
        } else  {
                g_slist_foreach
                        ( mc->mov_bp->get_configure_list_head (),
                          (GFunc) App::hide_w, NULL
                          );
                g_slist_foreach
                        ( mc->mov_bp->get_config_checkbutton_list_head (),
                          (GFunc) DSPMoverControl::show_tab_as_configured, NULL
                          );
        }
        if (!action){
                g_variant_unref (new_state);
        }
}

void DSPMoverControl::AppWindowInit(const gchar *title){
        if (title) { // stage 1
                PI_DEBUG (DBG_L2, "DSPMoverControl::AppWindowInit -- header bar");

                app_window = gxsm4_app_window_new (GXSM4_APP (gapp->get_application ()));
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
                PI_DEBUG (DBG_L2, "DSPMoverControl::AppWindowInit -- header bar -- stage two, hook configure menu");

                win_DSPMover_popup_entries[0].state = g_settings_get_boolean (hwi_settings, "configure-mode") ? "true":"false"; // get last state
                g_action_map_add_action_entries (G_ACTION_MAP (app_window),
                                                 win_DSPMover_popup_entries, G_N_ELEMENTS (win_DSPMover_popup_entries),
                                                 this);

                // create window PopUp menu  ---------------------------------------------------------------------
                mc_popup_menu = gtk_popover_menu_new_from_model (G_MENU_MODEL (gapp->get_hwi_mover_popup_menu ()));

                // attach popup menu configuration to tool button --------------------------------
                GtkWidget *header_menu_button = gtk_menu_button_new ();
                gtk_menu_button_set_icon_name (GTK_MENU_BUTTON (header_menu_button), "applications-utilities-symbolic");
                gtk_menu_button_set_popover (GTK_MENU_BUTTON (header_menu_button), mc_popup_menu);
                gtk_header_bar_pack_end (GTK_HEADER_BAR (header_bar), header_menu_button);
                gtk_widget_show (header_menu_button);
        }
}

void DSPMoverControl::create_folder (){
	GSList *EC_list=NULL;

	Gtk_EntryControl *ec;
	GtkWidget *tg, *fg, *fg2, *frame_param, *frame_param2;
	GtkWidget *input;
	GtkWidget *notebook;
	GtkWidget *MoverCrtl;
	GtkWidget *button, *img, *lab;
        // FIX-ME GTK4
	//GtkAccelGroup *accel_group=NULL;

        //Gtk_EntryControl *ec_phase;
 
        PI_DEBUG (DBG_L2, "DSPMoverControl::create_folder");

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
	const char *pcs_tab_remote_key_prefix[] = { "dspmover-xy-", "dspmover-rot-", "dspmover-psd-", "dspmover-lens-", "dspmover-auto-", "dspmover-sm-", "dspmover-z0-", "dspmover-config-", NULL};

	Gtk_EntryControl *Ampl, *Spd, *Stp, *GPIO_on, *GPIO_off, *GPIO_reset, *GPIO_scan, *GPIO_dir, *Wave_out0, *Wave_out1;
	int i,itab;

        mov_bp = new MOV_GUI_Builder (v_grid);
        mov_bp->set_error_text ("Invalid Value.");
        mov_bp->set_input_width_chars (10);
        mov_bp->set_no_spin ();

        // =======================================
        //const gchar* signal_name_button_pressed  = "clicked";  //"activate"; // "pressed" in GTK3
        //const gchar* signal_name_button_released = "released"; // "released" in GTK3
        // =======================================
        //g_message ("MOVER: settings signals for MV control button signals: Pressed: '%s', Released: '%s'",signal_name_button_pressed, signal_name_button_released);
        
        for(itab=i=0; MoverNames[i]; ++i){                
                Gtk_EntryControl *ec_axis[3];
                GtkGesture *gesture;

                if (gapp->xsm->Inst->OffsetMode() != OFM_ANALOG_OFFSET_ADDING && i == 5) continue;
		if (IS_SLIDER_CTRL && i < 4 ) continue;
		PI_DEBUG (DBG_L2, "DSPMoverControl::Mover:" << MoverNames[i]);

                mov_bp->start_notebook_tab (notebook, MoverNames[i], mover_tab_key[i], hwi_settings);
                itab++;
                        
                mov_bp->set_pcs_remote_prefix (pcs_tab_remote_key_prefix[i]);

                if (i==6){ // Z0 Tab
                        mov_bp->new_grid_with_frame ("Z-Offset Control");
			mov_bp->set_default_ec_change_notice_fkt (DSPMoverControl::ChangedNotify, this);

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
			g_object_set_data( G_OBJECT (button), "DSP_cmd", GINT_TO_POINTER (DSP_CMD_Z0_STOP));
			g_object_set_data( G_OBJECT (button), "MoverNo", GINT_TO_POINTER (99));
			g_signal_connect ( G_OBJECT (button), "clicked",
                                           G_CALLBACK (DSPMoverControl::CmdAction),
                                           this);
                        
			{ // remote hook
				remote_action_cb *ra = g_new( remote_action_cb, 1);
				ra -> cmd = g_strdup_printf("DSP_CMD_STOP_Z0");
				ra -> RemoteCb = (void (*)(GtkWidget*, void*))DSPMoverControl::CmdAction;
				ra -> widget = button;
				ra -> data = this;
				gapp->RemoteActionList = g_slist_prepend ( gapp->RemoteActionList, ra );
				gchar *help = g_strconcat ("Remote example: action (\"", ra->cmd, "\"", NULL);
				gtk_widget_set_tooltip_text (button, help);
			}

                        // UP arrow (back)
			mov_bp->set_xy (3,11);
                        mov_bp->grid_add_widget (button = gtk_button_new_from_icon_name ("seek-backward-symbolic"));
			g_object_set_data( G_OBJECT (button), "DSP_cmd", GINT_TO_POINTER (DSP_CMD_Z0_P));
			g_object_set_data( G_OBJECT (button), "MoverNo", GINT_TO_POINTER (99));
#if 0
			g_signal_connect ( G_OBJECT (button), signal_name_button_pressed,
                                           G_CALLBACK (DSPMoverControl::CmdAction),
                                           this);
			g_signal_connect ( G_OBJECT (button), signal_name_button_released,
                                           G_CALLBACK (DSPMoverControl::StopAction),
                                           this);
#endif
                        gesture = gtk_gesture_click_new ();
			g_object_set_data( G_OBJECT (button), "DSP_cmd", GINT_TO_POINTER (DSP_CMD_Z0_P));
			g_object_set_data( G_OBJECT (button), "MoverNo", GINT_TO_POINTER (99));
			g_object_set_data( G_OBJECT (gesture), "Button", button);
                        //gtk_gesture_single_set_button (GTK_GESTURE_SINGLE (gesture), 1);
                        g_signal_connect (gesture, "pressed", G_CALLBACK (DSPMoverControl::direction_button_pressed_cb), this);
                        g_signal_connect (gesture, "released", G_CALLBACK (DSPMoverControl::direction_button_released_cb), this);
                        gtk_widget_add_controller (button, GTK_EVENT_CONTROLLER (gesture));

                        // FIX-ME GTK4 ??
			//g_signal_connect( G_OBJECT(v_grid), "key_press_event", 
                        //                  G_CALLBACK(create_window_key_press_event_lcb), this);

			{ // remote hook
				remote_action_cb *ra = g_new( remote_action_cb, 1);
				ra -> cmd = g_strdup_printf("DSP_CMD_UP_Z0");
				ra -> RemoteCb = (void (*)(GtkWidget*, void*))DSPMoverControl::CmdAction;
				ra -> widget = button;
				ra -> data = this;
				gapp->RemoteActionList = g_slist_prepend ( gapp->RemoteActionList, ra );
				gchar *help = g_strconcat ("Remote example: action (\"", ra->cmd, "\"", NULL);
				gtk_widget_set_tooltip_text (button, help);
			}


			// DOWN arrow (forward)
			mov_bp->set_xy (3,13);
                        mov_bp->grid_add_widget (button = gtk_button_new_from_icon_name ("seek-forward-symbolic"));
			g_object_set_data( G_OBJECT (button), "DSP_cmd", GINT_TO_POINTER (DSP_CMD_Z0_M));
			g_object_set_data( G_OBJECT (button), "MoverNo", GINT_TO_POINTER (99));
#if 0
			g_signal_connect ( G_OBJECT (button), signal_name_button_pressed,
                                           G_CALLBACK (DSPMoverControl::CmdAction),
                                           this);
			g_signal_connect ( G_OBJECT (button), signal_name_button_released,
                                           G_CALLBACK (DSPMoverControl::StopAction),
                                           this);
#endif
                        gesture = gtk_gesture_click_new ();
			g_object_set_data( G_OBJECT (button), "DSP_cmd", GINT_TO_POINTER (DSP_CMD_Z0_P));
			g_object_set_data( G_OBJECT (button), "MoverNo", GINT_TO_POINTER (99));
			g_object_set_data( G_OBJECT (gesture), "Button", button);
                        //gtk_gesture_single_set_button (GTK_GESTURE_SINGLE (gesture), 1);
                        g_signal_connect (gesture, "pressed", G_CALLBACK (DSPMoverControl::direction_button_pressed_cb), this);
                        g_signal_connect (gesture, "released", G_CALLBACK (DSPMoverControl::direction_button_released_cb), this);
                        gtk_widget_add_controller (button, GTK_EVENT_CONTROLLER (gesture));

			{ // remote hook
				remote_action_cb *ra = g_new( remote_action_cb, 1);
				ra -> cmd = g_strdup_printf("DSP_CMD_DOWN_Z0");
				ra -> RemoteCb = (void (*)(GtkWidget*, void*))DSPMoverControl::CmdAction;
				ra -> widget = button;
				ra -> data = this;
				gapp->RemoteActionList = g_slist_prepend ( gapp->RemoteActionList, ra );
				gchar *help = g_strconcat ("Remote example: action (\"", ra->cmd, "\"", NULL);
				gtk_widget_set_tooltip_text (button, help);
			}

                        // approach (connect, disconnect)
			mov_bp->set_xy (6,11);
                        mov_bp->grid_add_widget (button = gtk_button_new_from_icon_name ("goto-center-symbolic"));
			g_object_set_data( G_OBJECT (button), "DSP_cmd", GINT_TO_POINTER (DSP_CMD_Z0_AUTO));
			g_object_set_data( G_OBJECT (button), "MoverNo", GINT_TO_POINTER (99));
			g_signal_connect ( G_OBJECT (button), "clicked",
                                           G_CALLBACK (DSPMoverControl::CmdAction),
                                           this);
			{ // remote hook
				remote_action_cb *ra = g_new( remote_action_cb, 1);
				ra -> cmd = g_strdup_printf("DSP_CMD_AUTOCENTER_Z0");
				ra -> RemoteCb = (void (*)(GtkWidget*, void*))DSPMoverControl::CmdAction;
				ra -> widget = button;
				ra -> data = this;
				gapp->RemoteActionList = g_slist_prepend ( gapp->RemoteActionList, ra );
				gchar *help = g_strconcat ("Remote example: action (\"", ra->cmd, "\"", NULL);
				gtk_widget_set_tooltip_text (button, help);
			}


			mov_bp->set_xy (6,12);
                        mov_bp->grid_add_widget (button = gtk_button_new_from_icon_name ("goto-home-symbolic"));
			g_object_set_data( G_OBJECT (button), "DSP_cmd", GINT_TO_POINTER (DSP_CMD_Z0_CENTER));
			g_object_set_data( G_OBJECT (button), "MoverNo", GINT_TO_POINTER (99));
			g_signal_connect ( G_OBJECT (button), "clicked",
                                           G_CALLBACK (DSPMoverControl::CmdAction),
                                           this);
			{ // remote hook
				remote_action_cb *ra = g_new( remote_action_cb, 1);
				ra -> cmd = g_strdup_printf("DSP_CMD_HOME_Z0");
				ra -> RemoteCb = (void (*)(GtkWidget*, void*))DSPMoverControl::CmdAction;
				ra -> widget = button;
				ra -> data = this;
				gapp->RemoteActionList = g_slist_prepend ( gapp->RemoteActionList, ra );
				gchar *help = g_strconcat ("Remote example: action (\"", ra->cmd, "\"", NULL);
				gtk_widget_set_tooltip_text (button, help);
			}


                        mov_bp->set_xy (6,13);
                        mov_bp->grid_add_widget (button = gtk_button_new_from_icon_name ("goto-position-symbolic"));
			g_object_set_data( G_OBJECT (button), "DSP_cmd", GINT_TO_POINTER (DSP_CMD_Z0_GOTO));
			g_object_set_data( G_OBJECT (button), "MoverNo", GINT_TO_POINTER (99));
			g_signal_connect ( G_OBJECT (button), "clicked",
                                           G_CALLBACK (DSPMoverControl::CmdAction),
                                           this);
			{ // remote hook
				remote_action_cb *ra = g_new( remote_action_cb, 1);
				ra -> cmd = g_strdup_printf("DSP_CMD_GOTO_Z0");
				ra -> RemoteCb = (void (*)(GtkWidget*, void*))DSPMoverControl::CmdAction;
				ra -> widget = button;
				ra -> data = this;
				gapp->RemoteActionList = g_slist_prepend ( gapp->RemoteActionList, ra );
				gchar *help = g_strconcat ("Remote example: action (\"", ra->cmd, "\"", NULL);
				gtk_widget_set_tooltip_text (button, help);
			}


                        
			mov_bp->notebook_tab_show_all ();
			continue;
		}

                // ================================================== CONFIG TAB
		if (i==7){ // config tab
                        mov_bp->new_grid_with_frame ("Output Configuration");
			mov_bp->set_default_ec_change_notice_fkt (DSPMoverControl::ChangedNotify, this);

                        mov_bp->new_grid_with_frame ("Waveform selection");

                        GtkWidget *cbx;
                        mov_bp->grid_add_widget (cbx = gtk_combo_box_text_new ());
                        // fill with options
                        for(int j=0; wave_form_options[j].wave_form_label; j++){
                                 gchar *id = g_strdup_printf ("%d",  wave_form_options[j].curve_id);
                                 gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (cbx), id, wave_form_options[j].wave_form_label);
                                 g_free (id);
                        }
                        /* connect with signal-handler if selected */
                        g_signal_connect (G_OBJECT (cbx), "changed", G_CALLBACK (DSPMoverControl::config_waveform), this);

                        {
                                gchar *id = g_strdup_printf ("%d", mover_param.MOV_waveform_id);
                                gtk_combo_box_set_active_id (GTK_COMBO_BOX (cbx), id);
                                g_free (id);
                        }
                        mov_bp->new_line ();

                        mov_bp->grid_add_ec ("Space", Time, &mover_param.Wave_space, 0., 1000., ".3f", 1., 10., "Wave-Space");
			g_object_set_data( G_OBJECT (mov_bp->input), "Wavespace ", GINT_TO_POINTER (i));
			gtk_widget_set_tooltip_text (mov_bp->input, "Wave form spacing ");
                        g_signal_connect (G_OBJECT (mov_bp->input), "changed", G_CALLBACK (DSPMoverControl::config_waveform), this);
                        mov_bp->new_line ();

                        mov_bp->grid_add_ec ("Offset", Unity, &mover_param.Wave_offset, -10., 10., ".2f", 1., 10., "Wave-Offset");
			g_object_set_data( G_OBJECT (mov_bp->input), "Waveoffset ", GINT_TO_POINTER (i));
			gtk_widget_set_tooltip_text (mov_bp->input, "Wave form relative DC Offset (in Amplitude Volts) ");
                        g_signal_connect (G_OBJECT (mov_bp->input), "changed", G_CALLBACK (DSPMoverControl::config_waveform), this);
                        mov_bp->new_line ();

                        // config options, managed -- moved here
	                mov_bp->set_configure_hide_list_b_mode_on ();
                        mov_bp->grid_add_ec ("Phase", Phase, &mover_param.inch_worm_phase, 0., 360., "3.0f", 1., 60., "IW-Phase");
	                gtk_widget_set_tooltip_text (mov_bp->input,
                                                     "Generic Phase value may be used by custom wave form generation.\n"
                                                     "Used by Sine and Pulse\n");
                        g_signal_connect (G_OBJECT (mov_bp->input), "changed", G_CALLBACK (DSPMoverControl::config_waveform), this);
                        mov_bp->new_line ();
                        mov_bp->set_configure_hide_list_b_mode_off ();;

                        mov_bp->set_configure_hide_list_b_mode_on ();
                        mov_bp->grid_add_ec ("z-jump/xy-amplitude", Unity, &mover_param.z_Rate, 0., 1., ".2f", 0.01, 0.1, "besocke-z-jump-ratio");
                        gtk_widget_set_tooltip_text (mov_bp->input, "Relative amplitude of the z-jump in respect to the amplitude of the xy-slide");
                        g_signal_connect (G_OBJECT (mov_bp->input), "changed", G_CALLBACK (DSPMoverControl::config_waveform), this);
                        mov_bp->new_line ();

                        mov_bp->grid_add_ec ("settling time t1", Time, &mover_param.time_delay_1, 0., 1., ".3f", 0.001, 0.01, "besocke-t1");
                        gtk_widget_set_tooltip_text (mov_bp->input, "Time delay of the signals to settle the tip before and after the z-jump");
                        g_signal_connect (G_OBJECT (mov_bp->input), "changed", G_CALLBACK (DSPMoverControl::config_waveform), this);
                        mov_bp->new_line ();

                        mov_bp->grid_add_ec ("period of fall t2", Time, &mover_param.time_delay_2, 0., 1., ".3f", 0.001, 0.01, "besocke-t2");
                        gtk_widget_set_tooltip_text (mov_bp->input, "Time between the z-jump and the xy-slide.");
                        g_signal_connect (G_OBJECT (mov_bp->input), "changed", G_CALLBACK (DSPMoverControl::config_waveform), this);
                        mov_bp->new_line ();
                        mov_bp->set_configure_hide_list_b_mode_off ();

                        mov_bp->pop_grid ();
                        
                        // ===
                        mov_bp->new_line ();
                        // ======================================== Wave Output Setup ========================================
			mov_bp->new_grid_with_frame ("#wave/direction(xyz)->DAC(0-6) ... preview");
                        gtk_widget_set_tooltip_text (mov_bp->frame,"rows: waveform, columns: direction, cells: output channel; channel 7 does not work as it is overwritten by another signal."); 

                        mov_bp->new_line ();

                        mov_bp->set_configure_hide_list_b_mode_on ();
                        mov_bp->set_input_width_chars (3);
                        for (int k=0; k<6; ++k){
                                gchar *wchlab= g_strdup_printf("Wave %d: X", k);
                                gchar *wchid = g_strdup_printf("wave-out%d-ch-x", k);
                                mov_bp->grid_add_ec (wchlab, Unity, &mover_param.wave_out_channel_xyz[k][0], 0, 17, ".0f", wchid);
                                gtk_widget_set_tooltip_text (mov_bp->input, "map wave N onto DAC channel 0-6 for X direction move action.\n MK2: add 10 for adding mode. 10=CH0, 11=CH1,.. with adding wave to current output signal");
                                g_free (wchid);
                                wchid = g_strdup_printf("wave-out%d-ch-y", k);
                                mov_bp->grid_add_ec ("Y", Unity, &mover_param.wave_out_channel_xyz[k][1], 0, 17, ".0f", wchid);
                                gtk_widget_set_tooltip_text (mov_bp->input, "map wave N onto DAC channel 0-6 for Y direction move action.\n MK2: add 10 for adding mode. 10=CH0, 11=CH1,.. with adding wave to current output signal");
                                g_free (wchid);
                                wchid = g_strdup_printf("wave-out%d-ch-z", k);
                                mov_bp->grid_add_ec ("Z", Unity, &mover_param.wave_out_channel_xyz[k][2], 0, 17, ".0f", wchid);
                                gtk_widget_set_tooltip_text (mov_bp->input, "map wave N onto DAC channel 0-6 for Z direction move action.\n MK2: add 10 for adding mode. 10=CH0, 11=CH1,.. with adding wave to current output signal");
                                g_free (wchid);
                                g_free (wchlab);

                                GtkWidget *wave_preview_area = gtk_drawing_area_new ();
                                gtk_drawing_area_set_content_width (GTK_DRAWING_AREA (wave_preview_area), 128);
                                gtk_drawing_area_set_content_height (GTK_DRAWING_AREA (wave_preview_area), 34);
                                gtk_drawing_area_set_draw_func (GTK_DRAWING_AREA (wave_preview_area), wave_preview_draw_function, this, NULL);
                                g_object_set_data  (G_OBJECT (wave_preview_area), "wave_ch", GINT_TO_POINTER (k));
                                mov_bp->grid_add_widget (wave_preview_area);

                                mov_bp->new_line ();
                        }
                        mov_bp->set_configure_hide_list_b_mode_off ();
                        // g_object_set_data(G_OBJECT (mov_bp->button), "WAVE-CHXY-LIST", mov_bp->get_configure_hide_list_b_head ());
                                
                        mov_bp->set_default_ec_change_notice_fkt (DSPMoverControl::ChangedNotify, this);

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

			mov_bp->notebook_tab_show_all ();

                        configure_waveform (cbx); // update now

			continue;
		}

		if (i<DSP_AFMMOV_MODES){ // modes stores 0..5
                        mov_bp->new_grid_with_frame ("Mover Timing");
                        mov_bp->set_default_ec_change_notice_fkt (DSPMoverControl::ChangedNotify, this);

                        mov_bp->grid_add_ec ("Max. Steps", Unity, &mover_param.AFM_usrSteps[i], 1., 5000., "4.0f", 1., 10., "max-steps"); //mov_bp->new_line ();
                        g_object_set_data( G_OBJECT (mov_bp->input), "MoverNo", GINT_TO_POINTER (i));
                        
                        mov_bp->set_configure_list_mode_on ();
                        mov_bp->grid_add_ec ("Amplitude", Volt, &mover_param.AFM_usrAmp[i], -20., 20., "5.2f", 0.1, 1., "amplitude"); mov_bp->new_line ();
                        g_object_set_data( G_OBJECT (mov_bp->input), "MoverNo", GINT_TO_POINTER (i));

                        mov_bp->grid_add_ec ("Duration", Time, &mover_param.AFM_usrSpeed[i], 0.1, 110., "4.3f", 0.1, 1., "duration"); //mov_bp->new_line ();
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
	        				    G_CALLBACK (DSPMoverControl::RampspeedUpdate),
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
			// STOP
			mov_bp->set_xy (3,2);
                        mov_bp->grid_add_widget (button = gtk_button_new_from_icon_name ("process-stopall-symbolic"));
                        g_object_set_data( G_OBJECT (button), "AXIS-X", ec_axis[0]);
                        g_object_set_data( G_OBJECT (button), "AXIS-Y", ec_axis[1]);
                        g_object_set_data( G_OBJECT (button), "AXIS-Z", ec_axis[2]);
			g_signal_connect (G_OBJECT (button), "clicked",
					    G_CALLBACK (DSPMoverControl::StopAction),
					    this);
                        if (i!=4)
                        {
                                // UP
	        		mov_bp->set_xy (3,1);
                                mov_bp->grid_add_widget (button = gtk_button_new_from_icon_name ("seek-backward-symbolic"));
	        		g_object_set_data( G_OBJECT (button), "DSP_cmd", GINT_TO_POINTER (DSP_CMD_AFM_MOV_YP));
	        		g_object_set_data( G_OBJECT (button), "MoverNo", GINT_TO_POINTER (i));
                                g_object_set_data( G_OBJECT (button), "AXIS-X", ec_axis[0]);
                                g_object_set_data( G_OBJECT (button), "AXIS-Y", ec_axis[1]);
                                g_object_set_data( G_OBJECT (button), "AXIS-Z", ec_axis[2]);
#if 0
	        		g_signal_connect (G_OBJECT (button), signal_name_button_pressed,
	        				    G_CALLBACK (DSPMoverControl::CmdAction),
	        				    this);
	        		g_signal_connect (G_OBJECT (button), signal_name_button_released,
	        				    G_CALLBACK (DSPMoverControl::StopAction),
	        				    this);
#endif
                                gesture = gtk_gesture_click_new ();
                                g_object_set_data( G_OBJECT (button), "DSP_cmd", GINT_TO_POINTER (DSP_CMD_Z0_P));
                                g_object_set_data( G_OBJECT (button), "MoverNo", GINT_TO_POINTER (99));
                                g_object_set_data( G_OBJECT (gesture), "Button", button);
                                //gtk_gesture_single_set_button (GTK_GESTURE_SINGLE (gesture), 1);
                                g_signal_connect (gesture, "pressed", G_CALLBACK (DSPMoverControl::direction_button_pressed_cb), this);
                                g_signal_connect (gesture, "released", G_CALLBACK (DSPMoverControl::direction_button_released_cb), this);
                                gtk_widget_add_controller (button, GTK_EVENT_CONTROLLER (gesture));

                                /*
	        		g_signal_connect(G_OBJECT(v_grid *** box), "key_press_event", 
	        				   G_CALLBACK(create_window_key_press_event_lcb), this);
	        		*/
	        		{ // pyremote hook
	        			remote_action_cb *ra = g_new( remote_action_cb, 1);
	        			ra -> cmd = g_strdup_printf("DSP_CMD_MOV-YP_%s",MoverNames[i]);
	        			ra -> RemoteCb = (void (*)(GtkWidget*, void*))DSPMoverControl::CmdAction;
	        			ra -> widget = button;
	        			ra -> data = this;
	        			gapp->RemoteActionList = g_slist_prepend ( gapp->RemoteActionList, ra );
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
                                mov_bp->grid_add_widget (button = gtk_button_new_from_icon_name ("seek-backward-symbolic"));
	        		g_object_set_data( G_OBJECT (button), "DSP_cmd", GINT_TO_POINTER (DSP_CMD_AFM_MOV_ZP));
	        		g_object_set_data( G_OBJECT (button), "MoverNo", GINT_TO_POINTER (i));
                                g_object_set_data( G_OBJECT (button), "AXIS-X", ec_axis[0]);
                                g_object_set_data( G_OBJECT (button), "AXIS-Y", ec_axis[1]);
                                g_object_set_data( G_OBJECT (button), "AXIS-Z", ec_axis[2]);
#if 0
	        		g_signal_connect (G_OBJECT (button), signal_name_button_pressed,
	        				    G_CALLBACK (DSPMoverControl::CmdAction),
	        				    this);
	        		g_signal_connect (G_OBJECT (button), signal_name_button_released,
	        				    G_CALLBACK (DSPMoverControl::StopAction),
	        				    this);
#endif
                                gesture = gtk_gesture_click_new ();
                                g_object_set_data( G_OBJECT (button), "DSP_cmd", GINT_TO_POINTER (DSP_CMD_Z0_P));
                                g_object_set_data( G_OBJECT (button), "MoverNo", GINT_TO_POINTER (99));
                                g_object_set_data( G_OBJECT (gesture), "Button", button);
                                //gtk_gesture_single_set_button (GTK_GESTURE_SINGLE (gesture), 1);
                                g_signal_connect (gesture, "pressed", G_CALLBACK (DSPMoverControl::direction_button_pressed_cb), this);
                                g_signal_connect (gesture, "released", G_CALLBACK (DSPMoverControl::direction_button_released_cb), this);
                                gtk_widget_add_controller (button, GTK_EVENT_CONTROLLER (gesture));
                              

	        		/*
	        		g_signal_connect(G_OBJECT(v_grid *** box), "key_press_event", 
	        				   G_CALLBACK(create_window_key_press_event_lcb), this);
	        		*/
	        		{ // pyremote hook
	        			remote_action_cb *ra = g_new( remote_action_cb, 1);
	        			ra -> cmd = g_strdup_printf("DSP_CMD_MOV-ZP_%s",MoverNames[i]);
	        			ra -> RemoteCb = (void (*)(GtkWidget*, void*))DSPMoverControl::CmdAction;
	        			ra -> widget = button;
	        			ra -> data = this;
	        			gapp->RemoteActionList = g_slist_prepend ( gapp->RemoteActionList, ra );
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
				// LEFT
                                mov_bp->set_xy (2,2);
                                mov_bp->grid_add_widget (button = gtk_button_new_from_icon_name ("seek-left-symbolic"));
				g_object_set_data( G_OBJECT (button), "DSP_cmd", GINT_TO_POINTER (DSP_CMD_AFM_MOV_XM));
				g_object_set_data( G_OBJECT (button), "MoverNo", GINT_TO_POINTER (i));
                                g_object_set_data( G_OBJECT (button), "AXIS-X", ec_axis[0]);
                                g_object_set_data( G_OBJECT (button), "AXIS-Y", ec_axis[1]);
                                g_object_set_data( G_OBJECT (button), "AXIS-Z", ec_axis[2]);
#if 0
				g_signal_connect (G_OBJECT (button), signal_name_button_pressed,
						    G_CALLBACK (DSPMoverControl::CmdAction),
						    this);
				g_signal_connect (G_OBJECT (button), signal_name_button_released,
						    G_CALLBACK (DSPMoverControl::StopAction),
						    this);
#endif

                                gesture = gtk_gesture_click_new ();
                                g_object_set_data( G_OBJECT (button), "DSP_cmd", GINT_TO_POINTER (DSP_CMD_Z0_P));
                                g_object_set_data( G_OBJECT (button), "MoverNo", GINT_TO_POINTER (99));
                                g_object_set_data( G_OBJECT (gesture), "Button", button);
                                //gtk_gesture_single_set_button (GTK_GESTURE_SINGLE (gesture), 1);
                                g_signal_connect (gesture, "pressed", G_CALLBACK (DSPMoverControl::direction_button_pressed_cb), this);
                                g_signal_connect (gesture, "released", G_CALLBACK (DSPMoverControl::direction_button_released_cb), this);
                                gtk_widget_add_controller (button, GTK_EVENT_CONTROLLER (gesture));

				{ // pyremote hook
					remote_action_cb *ra = g_new( remote_action_cb, 1);
					ra -> cmd = g_strdup_printf("DSP_CMD_MOV-XM_%s",MoverNames[i]);
					ra -> RemoteCb = (void (*)(GtkWidget*, void*))DSPMoverControl::CmdAction;
					ra -> widget = button;
					ra -> data = this;
					gapp->RemoteActionList = g_slist_prepend ( gapp->RemoteActionList, ra );
					gchar *help = g_strconcat ("Remote example: action (\"", ra->cmd, "\"", NULL);
					gtk_widget_set_tooltip_text (button, help);
				}
	//      gtk_widget_add_accelerator (button, "pressed", accel_group,
	//                                  GDK_F1+4*i, (GdkModifierType)0,
	//                                  GTK_ACCEL_VISIBLE);

				// RIGHT
                                // button = gtk_button_new_from_icon_name ("seek-right-symbolic");
                                mov_bp->set_xy (4,2);
                                mov_bp->grid_add_widget (button = gtk_button_new_from_icon_name ("media-seek-forward-symbolic"));
				g_object_set_data( G_OBJECT (button), "DSP_cmd", GINT_TO_POINTER (DSP_CMD_AFM_MOV_XP));
				g_object_set_data( G_OBJECT (button), "MoverNo", GINT_TO_POINTER (i));
                                g_object_set_data( G_OBJECT (button), "AXIS-X", ec_axis[0]);
                                g_object_set_data( G_OBJECT (button), "AXIS-Y", ec_axis[1]);
                                g_object_set_data( G_OBJECT (button), "AXIS-Z", ec_axis[2]);
#if 0
				g_signal_connect (G_OBJECT (button), signal_name_button_pressed,
						    G_CALLBACK (DSPMoverControl::CmdAction),
						    this);
				g_signal_connect (G_OBJECT (button), signal_name_button_released,
						    G_CALLBACK (DSPMoverControl::StopAction),
						    this);
#endif
                                gesture = gtk_gesture_click_new ();
                                g_object_set_data( G_OBJECT (button), "DSP_cmd", GINT_TO_POINTER (DSP_CMD_Z0_P));
                                g_object_set_data( G_OBJECT (button), "MoverNo", GINT_TO_POINTER (99));
                                g_object_set_data( G_OBJECT (gesture), "Button", button);
                                //gtk_gesture_single_set_button (GTK_GESTURE_SINGLE (gesture), 1);
                                g_signal_connect (gesture, "pressed", G_CALLBACK (DSPMoverControl::direction_button_pressed_cb), this);
                                g_signal_connect (gesture, "released", G_CALLBACK (DSPMoverControl::direction_button_released_cb), this);
                                gtk_widget_add_controller (button, GTK_EVENT_CONTROLLER (gesture));

				{ // pyremote hook
					remote_action_cb *ra = g_new( remote_action_cb, 1);
					ra -> cmd = g_strdup_printf("DSP_CMD_MOV-XP_%s",MoverNames[i]);
					ra -> RemoteCb = (void (*)(GtkWidget*, void*))DSPMoverControl::CmdAction;
					ra -> widget = button;
					ra -> data = this;
					gapp->RemoteActionList = g_slist_prepend ( gapp->RemoteActionList, ra );
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
                                mov_bp->grid_add_widget (button = gtk_button_new_from_icon_name ("seek-forward-symbolic"));
	        		g_object_set_data( G_OBJECT (button), "DSP_cmd", GINT_TO_POINTER (DSP_CMD_AFM_MOV_YM));
	        		g_object_set_data( G_OBJECT (button), "MoverNo", GINT_TO_POINTER (i));
                                g_object_set_data( G_OBJECT (button), "AXIS-X", ec_axis[0]);
                                g_object_set_data( G_OBJECT (button), "AXIS-Y", ec_axis[1]);
                                g_object_set_data( G_OBJECT (button), "AXIS-Z", ec_axis[2]);
#if 0
	        		g_signal_connect (G_OBJECT (button), signal_name_button_pressed,
	        				    G_CALLBACK (DSPMoverControl::CmdAction),
	        				    this);
	        		g_signal_connect (G_OBJECT (button), signal_name_button_released,
	        				    G_CALLBACK (DSPMoverControl::StopAction),
	        				    this);
#endif
                                gesture = gtk_gesture_click_new ();
                                g_object_set_data( G_OBJECT (button), "DSP_cmd", GINT_TO_POINTER (DSP_CMD_Z0_P));
                                g_object_set_data( G_OBJECT (button), "MoverNo", GINT_TO_POINTER (99));
                                g_object_set_data( G_OBJECT (gesture), "Button", button);
                                //gtk_gesture_single_set_button (GTK_GESTURE_SINGLE (gesture), 1);
                                g_signal_connect (gesture, "pressed", G_CALLBACK (DSPMoverControl::direction_button_pressed_cb), this);
                                g_signal_connect (gesture, "released", G_CALLBACK (DSPMoverControl::direction_button_released_cb), this);
                                gtk_widget_add_controller (button, GTK_EVENT_CONTROLLER (gesture));
                                
	        		{ // pyremote hook
	        			remote_action_cb *ra = g_new( remote_action_cb, 1);
                                        ra -> cmd = g_strdup_printf("DSP_CMD_MOV-YM_%s",MoverNames[i]);
	        			ra -> RemoteCb = (void (*)(GtkWidget*, void*))DSPMoverControl::CmdAction;
	        			ra -> widget = button;
	        			ra -> data = this;
	        			gapp->RemoteActionList = g_slist_prepend ( gapp->RemoteActionList, ra );
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
                                mov_bp->grid_add_widget (button = gtk_button_new_from_icon_name ("seek-forward-symbolic"));
	        		g_object_set_data( G_OBJECT (button), "DSP_cmd", GINT_TO_POINTER (DSP_CMD_AFM_MOV_ZM));
	        		g_object_set_data( G_OBJECT (button), "MoverNo", GINT_TO_POINTER (i));
                                g_object_set_data( G_OBJECT (button), "AXIS-X", ec_axis[0]);
                                g_object_set_data( G_OBJECT (button), "AXIS-Y", ec_axis[1]);
                                g_object_set_data( G_OBJECT (button), "AXIS-Z", ec_axis[2]);
#if 0
	        		g_signal_connect (G_OBJECT (button), "pressed",
	        				    G_CALLBACK (DSPMoverControl::CmdAction),
	        				    this);
	        		g_signal_connect (G_OBJECT (button), "released",
	        				    G_CALLBACK (DSPMoverControl::StopAction),
	        				    this);
#endif
                                gesture = gtk_gesture_click_new ();
                                g_object_set_data( G_OBJECT (button), "DSP_cmd", GINT_TO_POINTER (DSP_CMD_Z0_P));
                                g_object_set_data( G_OBJECT (button), "MoverNo", GINT_TO_POINTER (99));
                                g_object_set_data( G_OBJECT (gesture), "Button", button);
                                //gtk_gesture_single_set_button (GTK_GESTURE_SINGLE (gesture), 1);
                                g_signal_connect (gesture, "pressed", G_CALLBACK (DSPMoverControl::direction_button_pressed_cb), this);
                                g_signal_connect (gesture, "released", G_CALLBACK (DSPMoverControl::direction_button_released_cb), this);
                                gtk_widget_add_controller (button, GTK_EVENT_CONTROLLER (gesture));

	        		{ // pyremote hook
	        			remote_action_cb *ra = g_new( remote_action_cb, 1);
                                        ra -> cmd = g_strdup_printf("DSP_CMD_MOV-ZM_%s", MoverNames[i]);
	        			ra -> RemoteCb = (void (*)(GtkWidget*, void*))DSPMoverControl::CmdAction;
	        			ra -> widget = button;
	        			ra -> data = this;
	        			gapp->RemoteActionList = g_slist_prepend ( gapp->RemoteActionList, ra );
	        			gchar *help = g_strconcat ("Remote example: action (\"", ra->cmd, "\"", NULL);
	        			gtk_widget_set_tooltip_text (button, help);
	        		}
                        }
//      gtk_widget_add_accelerator (button, "pressed", accel_group,
//                                  GDK_F2+4*i, (GdkModifierType)0,
//                                  GTK_ACCEL_VISIBLE);

    
		}
    
		if(i==4){
			// ========================================
			// Auto App. Control Buttons  GTK_STOCK_GOTO_TOP  GTK_STOCK_GOTO_BOTTOM GTK_STOCK_MEDIA_STOP
			mov_bp->set_xy (6,1); mov_bp->grid_add_label ("Auto Control");

                        mov_bp->set_xy (6,2);
                        mov_bp->grid_add_widget (button = gtk_button_new_from_icon_name ("approach-symbolic"));
			gtk_widget_set_tooltip_text (button, "Start Auto Approaching. GPIO [Rot, Coarse Mode]");
			if(IS_MOVER_CTRL)
				g_object_set_data( G_OBJECT (button), "DSP_cmd", GINT_TO_POINTER (DSP_CMD_APPROCH_MOV_XP));
			else
				g_object_set_data( G_OBJECT (button), "DSP_cmd", GINT_TO_POINTER (DSP_CMD_APPROCH));

			g_object_set_data( G_OBJECT (button), "MoverNo", GINT_TO_POINTER (i));
                        g_object_set_data( G_OBJECT (button), "AXIS-X", ec_axis[0]);
                        g_object_set_data( G_OBJECT (button), "AXIS-Y", ec_axis[1]);
                        g_object_set_data( G_OBJECT (button), "AXIS-Z", ec_axis[2]);
			g_signal_connect (G_OBJECT (button), "clicked",
					    G_CALLBACK (DSPMoverControl::CmdAction),
					    this);
      
			{ // remote hook
				remote_action_cb *ra = g_new( remote_action_cb, 1);
				ra -> cmd = g_strdup_printf("DSP_CMD_AUTOAPP");
				ra -> RemoteCb = (void (*)(GtkWidget*, void*))DSPMoverControl::CmdAction;
				ra -> widget = button;
				ra -> data = this;
				gapp->RemoteActionList = g_slist_prepend ( gapp->RemoteActionList, ra );
				gchar *help = g_strconcat ("Remote example: action (\"", ra->cmd, "\"", NULL);
				gtk_widget_set_tooltip_text (button, help);
			}

                        mov_bp->set_xy (6,3);
                        mov_bp->grid_add_widget (button = gtk_button_new_from_icon_name ("system-off-symbolic"));
			gtk_widget_set_tooltip_text (button, "Cancel Auto Approach.");

			g_object_set_data( G_OBJECT (button), "DSP_cmd", GINT_TO_POINTER (DSP_CMD_CLR_PA));
			g_object_set_data( G_OBJECT (button), "MoverNo", GINT_TO_POINTER (i));
                        g_object_set_data( G_OBJECT (button), "AXIS-X", ec_axis[0]);
                        g_object_set_data( G_OBJECT (button), "AXIS-Y", ec_axis[1]);
                        g_object_set_data( G_OBJECT (button), "AXIS-Z", ec_axis[2]);
			g_signal_connect (G_OBJECT (button), "clicked",
					    G_CALLBACK (DSPMoverControl::CmdAction),
					    this);
      
			{ // remote hook
				remote_action_cb *ra = g_new( remote_action_cb, 1);
				ra -> cmd = g_strdup_printf("DSP_CMD_STOPALL");
				ra -> RemoteCb = (void (*)(GtkWidget*, void*))DSPMoverControl::CmdAction;
				ra -> widget = button;
				ra -> data = this;
				gapp->RemoteActionList = g_slist_prepend ( gapp->RemoteActionList, ra );
				gchar *help = g_strconcat ("Remote example: action (\"", ra->cmd, "\"", NULL);
				gtk_widget_set_tooltip_text (button, help);
			}

                        mov_bp->set_xy (5,3);
                        mov_bp->grid_add_widget (button = gtk_button_new_from_icon_name ("xymode-symbolic"));
			gtk_widget_set_tooltip_text (button, "Switch GPIO to XY Scan (Main) Mode.");

			g_object_set_data( G_OBJECT (button), "DSP_cmd", GINT_TO_POINTER (DSP_CMD_CLR_PA));
			g_object_set_data( G_OBJECT (button), "MoverNo", GINT_TO_POINTER (100));
			g_signal_connect (G_OBJECT (button), "clicked",
					    G_CALLBACK (DSPMoverControl::CmdAction),
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
                                           G_CALLBACK (DSPMoverControl::CmdAction),
                                           this);

			if(IS_MOVER_CTRL)
				g_object_set_data( G_OBJECT (button), "DSP_cmd", GINT_TO_POINTER (DSP_CMD_APPROCH_MOV_XP));
			else
				g_object_set_data( G_OBJECT (button), "DSP_cmd", GINT_TO_POINTER (DSP_CMD_APPROCH));

      
			{ // remote hook
				remote_action_cb *ra = g_new( remote_action_cb, 1);
				ra -> cmd = g_strdup_printf("DSP_CMD_AUTOAPP_STEPPER");
				ra -> RemoteCb = (void (*)(GtkWidget*, void*))DSPMoverControl::CmdAction;
				ra -> widget = button;
				ra -> data = this;
				gapp->RemoteActionList = g_slist_prepend ( gapp->RemoteActionList, ra );
				gchar *help = g_strconcat ("Remote example: action (\"", ra->cmd, "\"", NULL);
				gtk_widget_set_tooltip_text (button, help);
			}

			mov_bp->set_xy (6,3);
                        mov_bp->grid_add_widget (button = gtk_button_new_from_icon_name ("system-off-symbolic"));
			gtk_widget_set_tooltip_text (button, "Stop Auto Approaching.");
			g_object_set_data( G_OBJECT (button), "DSP_cmd", GINT_TO_POINTER (DSP_CMD_CLR_PA));
			g_object_set_data( G_OBJECT (button), "MoverNo", GINT_TO_POINTER (i));
                        g_object_set_data( G_OBJECT (button), "AXIS-X", ec_axis[0]);
                        g_object_set_data( G_OBJECT (button), "AXIS-Y", ec_axis[1]);
                        g_object_set_data( G_OBJECT (button), "AXIS-Z", ec_axis[2]);
			g_signal_connect ( G_OBJECT (button), "clicked",
                                           G_CALLBACK (DSPMoverControl::CmdAction),
                                           this);
      
			{ // remote hook
				remote_action_cb *ra = g_new( remote_action_cb, 1);
				ra -> cmd = g_strdup_printf("DSP_CMD_STOPALL_STEPPER");
				ra -> RemoteCb = (void (*)(GtkWidget*, void*))DSPMoverControl::CmdAction;
				ra -> widget = button;
				ra -> data = this;
				gapp->RemoteActionList = g_slist_prepend ( gapp->RemoteActionList, ra );
				gchar *help = g_strconcat ("Remote example: action (\"", ra->cmd, "\"", NULL);
				gtk_widget_set_tooltip_text (button, help);
			}
		}

                mov_bp->notebook_tab_show_all ();
	}
  
	// ============================================================
	// save List away...
        g_object_set_data( G_OBJECT (window), "MOVER_EC_list", mov_bp->get_ec_list_head ());
        sranger_mk2_hwi_pi.app->RemoteEntryList = g_slist_concat (sranger_mk2_hwi_pi.app->RemoteEntryList, mov_bp->get_remote_list_head ());
        configure_callback (NULL, NULL, this); // configure "false"
        
        AppWindowInit (NULL); // stage two
        set_window_geometry ("dsp-mover-control");
}

void DSPMoverControl::update(){
	g_slist_foreach
		( (GSList*) g_object_get_data( G_OBJECT (window), "MOVER_EC_list"),
		  (GFunc) App::update_ec, NULL
			);
}

void DSPMoverControl::updateDSP(int sliderno){
	PI_DEBUG (DBG_L2, "Hallo DSP ! Mover No:" << sliderno );

	if (sliderno == 100){
	        // auto offset ZERO
		sranger_common_hwi->SetOffset (0,0); // set
		sranger_common_hwi->SetOffset (0,0); // wait for finish
		if (mover_param.AFM_GPIO_setting != mover_param.GPIO_scan){
			if ( mover_param.GPIO_tmp1 ){ // unmask special bit from value
				mover_param.AFM_GPIO_setting = mover_param.GPIO_scan | mover_param.GPIO_tmp1;
				ExecCmd(DSP_CMD_GPIO_SETUP);
				mover_param.AFM_GPIO_setting = mover_param.GPIO_scan;
				ExecCmd(DSP_CMD_GPIO_SETUP);
			} else if ( mover_param.GPIO_tmp2 ){ // invert action unmask special bit from value
			        mover_param.AFM_GPIO_setting = mover_param.GPIO_scan;
				ExecCmd(DSP_CMD_GPIO_SETUP);
				mover_param.AFM_GPIO_setting = mover_param.GPIO_scan | mover_param.GPIO_tmp2;
				ExecCmd(DSP_CMD_GPIO_SETUP);
			} else {
 			        mover_param.AFM_GPIO_setting = mover_param.GPIO_scan;
			}
		}
                g_settings_set_int (hwi_settings, "mover-gpio-last", mover_param.AFM_GPIO_setting);
		return;
	}

	if (mover_param.AFM_GPIO_setting == mover_param.GPIO_scan 
	    && ((sliderno >= 0 && sliderno < DSP_AFMMOV_MODES)? mover_param.AFM_GPIO_usr_setting[sliderno] : mover_param.MOV_GPIO_setting) != mover_param.GPIO_scan){
	        // auto offset ZERO
		sranger_common_hwi->MovetoXY (0,0); // set
		sranger_common_hwi->SetOffset (0,0); // set
		sranger_common_hwi->SetOffset (0,0); // wait for finish
		
		if ( mover_param.GPIO_tmp1 ){ // unmask special bit from value
		        mover_param.AFM_GPIO_setting = mover_param.GPIO_scan | mover_param.GPIO_tmp1;
			ExecCmd(DSP_CMD_GPIO_SETUP);
			mover_param.AFM_GPIO_setting = mover_param.GPIO_scan;
			ExecCmd(DSP_CMD_GPIO_SETUP);
		} else if ( mover_param.GPIO_tmp2 ){ // invert action unmask special bit from value
		        mover_param.AFM_GPIO_setting = mover_param.GPIO_scan;
			ExecCmd(DSP_CMD_GPIO_SETUP);
			mover_param.AFM_GPIO_setting = mover_param.GPIO_scan | mover_param.GPIO_tmp2;
			ExecCmd(DSP_CMD_GPIO_SETUP);
		} else {
		        mover_param.AFM_GPIO_setting = mover_param.GPIO_scan;
		}
	}
	if (sliderno >= 0 && sliderno < DSP_AFMMOV_MODES){
	        // auto scan XY and offset to ZERO
                sranger_common_hwi->MovetoXY (0,0);
		sranger_common_hwi->SetOffset (0,0); // set
		sranger_common_hwi->SetOffset (0,0); // wait for finish
		mover_param.AFM_Amp   = mover_param.AFM_usrAmp  [sliderno];
		mover_param.AFM_Speed = mover_param.AFM_usrSpeed[sliderno];
		mover_param.AFM_Steps = mover_param.AFM_usrSteps[sliderno];

		if ( mover_param.GPIO_tmp1 ){ // unmask special bit from value
		        if ( mover_param.AFM_GPIO_setting != mover_param.AFM_GPIO_usr_setting[sliderno] ){
			        mover_param.AFM_GPIO_setting = mover_param.AFM_GPIO_usr_setting[sliderno] | mover_param.GPIO_tmp1;
				ExecCmd(DSP_CMD_GPIO_SETUP);
				mover_param.AFM_GPIO_setting = mover_param.AFM_GPIO_usr_setting[sliderno];
			        ExecCmd(DSP_CMD_GPIO_SETUP);
			}
		} else if ( mover_param.GPIO_tmp2 ){ // invert action unmask special bit from value
		        if ( mover_param.AFM_GPIO_setting != mover_param.AFM_GPIO_usr_setting[sliderno] | mover_param.GPIO_tmp2 ){
			        mover_param.AFM_GPIO_setting = mover_param.AFM_GPIO_usr_setting[sliderno];
				ExecCmd(DSP_CMD_GPIO_SETUP);
				mover_param.AFM_GPIO_setting = mover_param.AFM_GPIO_usr_setting[sliderno] | mover_param.GPIO_tmp2;
				ExecCmd(DSP_CMD_GPIO_SETUP);
			}
		} else {
		        mover_param.AFM_GPIO_setting = mover_param.AFM_GPIO_usr_setting[sliderno];
		}
	}
        g_settings_set_int (hwi_settings, "mover-gpio-last", mover_param.AFM_GPIO_setting);
}


int DSPMoverControl::config_waveform(GtkWidget *widget, DSPMoverControl *dspc){
        return dspc->configure_waveform (widget);
}

int DSPMoverControl::configure_waveform(GtkWidget *widget){
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
                gtk_widget_hide ((GtkWidget*) g_slist_nth_data (wc, 6+q));
                gtk_widget_hide ((GtkWidget*) g_slist_nth_data (wc, 7+q));
        }        

        return 1;
}


void DSPMoverControl::wave_preview_draw_function (GtkDrawingArea *area, cairo_t *cr,
                                                  int             width,
                                                  int             height,
                                                  DSPMoverControl *dspc){
        int wn = GPOINTER_TO_INT (g_object_get_data  (G_OBJECT (area), "wave_ch"));
        int nch = dspc->create_waveform (1.,1., 1); // preview params only, full scale, 1ms equiv. points -- Fwd. -->
        int n =  dspc->mover_param.MOV_wave_len/nch; // samples/ch
        gtk_drawing_area_set_content_width (area, n+2);
        gtk_drawing_area_set_content_height (area, 34);

        cairo_translate (cr, 1., 17.);
        cairo_scale (cr, 1., 1.);
        double yr=-16./(SR_VFAC+fabs(SR_VFAC*dspc->mover_param.Wave_offset));
        cairo_save (cr);

        cairo_item_path *wave = new cairo_item_path (2);
        wave->set_line_width (0.5);
        wave->set_stroke_rgba (CAIRO_COLOR_BLACK);
        wave->set_xy_fast (0,0,0);
        wave->set_xy_fast (1,n-1,0);
        wave->draw (cr);
        delete wave;

        wave = new cairo_item_path (n);
        wave->set_line_width (2.0);
        wave->set_stroke_rgba (CAIRO_COLOR_RED);
        for (int k=0; k<n; ++k) wave->set_xy_fast (k,k,yr*dspc->mover_param.MOV_waveform[k*nch+wn]);
        wave->draw (cr);

        nch = dspc->create_waveform (1.,-1., 1); // preview params only, full scale, 1ms equiv. points -- Rev. <--
        wave->set_stroke_rgba (CAIRO_COLOR_BLUE);
        for (int k=0; k<n; ++k) wave->set_xy_fast (k,k,yr*dspc->mover_param.MOV_waveform[k*nch+wn]);
        wave->draw (cr);

        delete wave;
}

void DSPMoverControl::updateAxisCounts (GtkWidget* w, int idx, int cmd){
        if (idx >=0 && idx < DSP_AFMMOV_MODES){
                Gtk_EntryControl *eaxis[3];
                eaxis[0] = (Gtk_EntryControl*) g_object_get_data( G_OBJECT (w), "AXIS-X");
                eaxis[1] = (Gtk_EntryControl*) g_object_get_data( G_OBJECT (w), "AXIS-Y");
                eaxis[2] = (Gtk_EntryControl*) g_object_get_data( G_OBJECT (w), "AXIS-Z");

                if (eaxis[0] && eaxis[1] && eaxis[2]){
                        double axis[3];
                        gchar *info;
                        sranger_common_hwi->RTQuery ("A", axis[0], axis[1], axis[2]);
                        // g_message ("DSPMoverControl::updateAxisCounts idx=%d cmd=%d  [%g, %g, %g]", idx, cmd,  axis[0], axis[1], axis[2]);
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
                }
        }
}

int DSPMoverControl::CmdAction(GtkWidget *widget, DSPMoverControl *dspc){
	int idx=-1;
	int cmd;
	PI_DEBUG (DBG_L2, "MoverCrtl::CmdAction " ); 

	if(IS_MOVER_CTRL)
	{
		idx = GPOINTER_TO_INT(g_object_get_data( G_OBJECT (widget), "MoverNo"));
	}

	if (idx < 10 || idx == 100)
	{
		dspc->updateDSP(idx);
	}

        
	cmd = GPOINTER_TO_INT(g_object_get_data( G_OBJECT (widget), "DSP_cmd"));

        g_message ("MOVER CMD ACTION: %d CMD: %d", idx, cmd);
        
        switch (cmd){
        case DSP_CMD_AFM_MOV_XM:
                dspc->mover_param.MOV_angle = 180.;
                break;
        case DSP_CMD_AFM_MOV_XP:
                dspc->mover_param.MOV_angle = 0.;
                break;
        case DSP_CMD_AFM_MOV_YM:
                dspc->mover_param.MOV_angle = (-90.);
                break;
        case DSP_CMD_AFM_MOV_YP:
                dspc->mover_param.MOV_angle = 90.;
                break;
        case DSP_CMD_AFM_MOV_ZM:
                dspc->mover_param.MOV_angle = (-200.);
                break;
        case DSP_CMD_AFM_MOV_ZP:
        default:
                dspc->mover_param.MOV_angle = 200.;
                break;
        }
        

        dspc->updateAxisCounts (widget, idx, cmd);

        if(cmd>0){
		dspc->ExecCmd(DSP_CMD_GPIO_SETUP);
		dspc->ExecCmd(cmd);
	}
	PI_DEBUG (DBG_L2, "cmd=" << cmd << " Mover=" << idx );
	return 0;
}

int DSPMoverControl::StopAction(GtkWidget *widget, DSPMoverControl *dspc){
	PI_DEBUG (DBG_L2, "DSPMoverControl::StopAction" );

	int idx = GPOINTER_TO_INT(g_object_get_data( G_OBJECT (widget), "MoverNo"));
	if (idx == 99)
		dspc->ExecCmd(DSP_CMD_Z0_STOP);
	else
		dspc->ExecCmd(DSP_CMD_CLR_PA);

        dspc->updateAxisCounts (widget, idx, 0);

	return 0;
}

int DSPMoverControl::RampspeedUpdate(GtkWidget *widget, DSPMoverControl *dspc){

        int idx = GPOINTER_TO_INT(g_object_get_data( G_OBJECT (widget), "MoverNo"));
        dspc->updateDSP(idx);

        PI_DEBUG_GP (DBG_L2, "DSPMoverControl::RampspeedUpdate t1=%f  t2=%f  duration=%f  amp=%f  idx=%d \n", 
                                        dspc->mover_param.time_delay_1, 
                                        dspc->mover_param.time_delay_2, 
                                        dspc->mover_param.AFM_Speed, 
                                        dspc->mover_param.AFM_Amp,
                                        idx);

        // new besocke ramp speed
        gchar *tmp = g_strdup_printf ("%.2f V/ms",
                                      (dspc->mover_param.AFM_Amp/2)
                                      / ((dspc->mover_param.AFM_Speed - 2*dspc->mover_param.time_delay_1 - dspc->mover_param.time_delay_2)/2));
        gtk_label_set_text (GTK_LABEL(dspc->mc_rampspeed_label), tmp);
        g_free (tmp);

        return 0;
}

void DSPMoverControl::ChangedNotify(Param_Control* pcs, gpointer dspc){
	int idx=-1;
	gchar *us=pcs->Get_UsrString();
	PI_DEBUG (DBG_L2, "MoverCrtl:: Param Changed: " << us );
	g_free(us);

	if(IS_MOVER_CTRL)
		idx = GPOINTER_TO_INT(pcs->GetEntryData("MoverNo"));

        ((DSPMoverControl*)dspc)->updateDSP(idx);
}

void DSPMoverControl::ExecCmd(int cmd){
	PI_DEBUG (DBG_L2, "DSPMoverControl::ExecCmd ==> >" << cmd);

        //<< " Amp=" << sranger_common_hwi->mover_param.AFM_Amp
        //<< " Speed=" << sranger_common_hwi->mover_param.AFM_Speed
        //<< " Steps=" << sranger_common_hwi->mover_param.AFM_Steps
        //<< " GPIO=0x" << std::hex << sranger_common_hwi->mover_param.AFM_GPIO_setting << std::dec
                
        sranger_common_hwi->ExecCmd (cmd);
}
