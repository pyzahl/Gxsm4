/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 8 c-style: "K&R" -*- */

/* Gnome gxsm - Gnome X Scanning Microscopy
 * universal STM/AFM/SARLS/SPALEED/... controlling and
 * data analysis software
 *
 * Gxsm Plugin Name: rpspmc_pacpll.C
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


#ifndef __RPSPMC_PACPLL_JSON_DATA_H
#define __RPSPMC_PACPLL_JSON_DATA_H

#include <glib.h>

struct JSON_parameter {
        const gchar *js_varname;
        double *value;
        gboolean ro;
};

struct JSON_signal {
        const gchar *js_varname;
        int size;
        double *value;
};

struct PACPLL_parameters {
        // RP-PAC-PLL module parameters
        double dc_offset;
        double exec_amplitude_monitor;
        double dds_frequency_monitor;
        double volume_monitor;
        double phase_monitor;
        double control_dfreq_monitor;
        double cpu_load;
        double free_ram;
        double counter;

        double parameter_period;
        double signal_period;
        double bram_write_adr;
        double bram_sample_pos;
        double bram_finished;

        double qc_gain;
        double qc_phase;
        double pac_dctau;
        double pactau;
        double pacatau;
        double frequency_manual; // manual f reset
        double frequency_center; // manual f reference
        double aux_scale;
        double volume_manual;
        double operation;
        double pacverbose;
        double transport_decimation;
        double transport_mode;
        double transport_ch3;
        double transport_ch4;
        double transport_ch5;
        double shr_dec_data;
        double gain1;
        double gain2;
        double gain3;
        double gain4;
        double gain5;
        double tune_dfreq;
        double tune_span;
        double center_amplitude; // center value of tune
        double center_frequency; // center value of tune
        double center_phase; // center value of tune
        double amplitude_fb_setpoint;
        double amplitude_fb_invert;
        double amplitude_fb_cp;
        double amplitude_fb_ci;
        double amplitude_fb_cp_db;
        double amplitude_fb_ci_db;
        double exec_fb_upper;
        double exec_fb_lower;
        double amplitude_controller;
        double phase_fb_setpoint;
        double phase_fb_invert;
        double phase_fb_cp;
        double phase_fb_ci;
        double phase_fb_cp_db;
        double phase_fb_ci_db;
        double phase_hold_am_noise_limit; 
        double freq_fb_upper;
        double freq_fb_lower;
        double qcontrol;
        double phase_controller;
        double phase_unwrapping_always;
        double lck_amplitude;
        double lck_phase;

        // RP-PAC-PLL / extra dFreq controller module parameters
        double dfreq_monitor;
        double dfreq_fb_setpoint;
        double dfreq_fb_invert;
        double dfreq_fb_cp_db;
        double dfreq_fb_ci_db;
        double dfreq_fb_cp;
        double dfreq_fb_ci;
        double dfreq_controller;
        double control_dfreq_fb_upper;
        double control_dfreq_fb_lower;
        
        // RP-PAC-PLL / extra PLL signal phase aligned pulse former
        double pulse_form_bias0, pulse_form_bias1;
        double pulse_form_phase0, pulse_form_phase1;
        double pulse_form_width0, pulse_form_width1;
        double pulse_form_width0if, pulse_form_width1if;
        double pulse_form_height0, pulse_form_height1;
        double pulse_form_height0if, pulse_form_height1if;
        double pulse_form_shapexw, pulse_form_shapexwif;
        double pulse_form_shapex, pulse_form_shapexif;
        gboolean pulse_form_enable;
 
        // RP-PAC-PLL / DSP/McBSP level PLL signal transport controls
        double set_singleshot_transport_trigger;
        double transport_tau[4];
};

struct PACPLL_signals {
        double signal_ch1[1024];
        double signal_ch2[1024];
        double signal_ch3[1024];
        double signal_ch4[1024];
        double signal_ch5[1024];
        double signal_frq[1024]; // in tune mode
        double signal_phase[1024]; // in tne mode, local
        double signal_ampl[1024]; // in tune mode, local
        double signal_gpiox[16];
};


struct SPMC_parameters {
// RP-SPMC module parameters
        double bias;
        double z_servo_mode;
        double z_servo_setpoint;
        double z_servo_cp;
        double z_servo_ci;
        double z_servo_upper;
        double z_servo_lower;
        double z_servo_setpoint_cz;
        double z_servo_level;

        double gvp_execute;
        double gvp_pause;
        double gvp_stop;
        double gvp_program;
        double gvp_status;
        
        double alpha;
        double slope_dzx;
        double slope_dzy;

        double set_offset_x;
        double set_offset_y;
        double set_offset_z;

        double v[16];
        
        // RP SPMC Monitors
        double bias_monitor;
        double signal_monitor;
        double x_monitor;
        double y_monitor;
        double z_monitor;
        double x0_monitor;
        double y0_monitor;
        double z0_monitor;
};

#define MAX_GVP_VECTORS   32
#define GVP_VECTOR_SIZE   16 // 10 components used (1st is index, then: N, nii, Options, Nrep, Next, dx, dy, dz, du, fill w zero to 16)

struct SPMC_signals {
        double gvp_vector[GVP_VECTOR_SIZE];
};

#endif
