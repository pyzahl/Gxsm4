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
        const gchar *unit_id;
};

struct JSON_signal {
        const gchar *js_varname;
        int size;
        double *value;
        const gchar *unit_id;
};

struct PACPLL_parameters {
        // RP-PAC-PLL module parameters
        // ---> DO NOT REORDER from here
        double dc_offset;
        double exec_amplitude_monitor;
        double dds_frequency_monitor;
        double dds_dfreq_computed;
        double volume_monitor;
        double phase_monitor;
        double control_dfreq_monitor;
        // <--- to here
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
        double phase_rot_ab;
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
        double Zdfreq_control;
        double Udfreq_control;
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
        double pulse_form_trigger_select;
        double pulse_form_dpt[4];
        double pulse_form_dpv[4];
                        
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

        double gvp_stream_mux_0;
        double gvp_stream_mux_1;
        
        double z_polarity;
        int gxsm_z_polarity;

        // ---> DO NOT REORDER from here
        double z_servo_mode;
        double z_servo_setpoint;
        double z_servo_cp;
        double z_servo_ci;
        double z_servo_cp_db;
        double z_servo_ci_db;
        double z_servo_upper;
        double z_servo_lower;
        double z_servo_setpoint_cz;
        double z_servo_level;
        double z_servo_in_offsetcomp;
        double z_servo_src_mux;
        // <-- to here
        
        double gvp_control;
        //double gvp_execute;
        //double gvp_pause;
        //double gvp_stop;
        //double gvp_program;
        double gvp_reset_options;
        double gvp_status;
        double gvp_data_position;
        
        double rpspmc_gvp_raw_vector_write_mode;
        double rpspmc_gvp_raw_nii;
        double rpspmc_gvp_raw_decii;
        
        // ---> DO NOT REORDER from here
        double alpha;
        double slope_dzx;
        double slope_dzy;
        double slope_slew;

        double set_scanpos_x;
        double set_scanpos_y;
        double set_scanpos_slew;
        double set_scanpos_opts;
        
        double set_offset_x;
        double set_offset_y;
        double set_offset_z;

        double set_offset_xy_slew;
        double set_offset_z_slew;
        // <-- to here

        double v[24];

        // DMA rate limit control
        double rpspmc_dma_pull_interval;
        
        // RP SPMC Monitors
        // ---> DO NOT REORDER from here
        double bias_monitor;
        double bias_reg_monitor;
        double bias_set_monitor;


        double gvpu_monitor;
        double gvpa_monitor;
        double gvpb_monitor;
        double gvpamc_monitor;
        double gvpfmc_monitor;
        double mux_monitor;
        double signal_monitor;
        double ad463x_monitor[2];
        double x_monitor; // final XYZ = VecOffset + VecScan + Vec(0,0,Zslope)
        double y_monitor;
        double z_monitor;
        double x0_monitor; // Offsets
        double y0_monitor;
        double z0_monitor;
        double xs_monitor; // scan coords
        double ys_monitor;
        double zs_monitor;
        // <--- to here

        // Lock-In
        double lck1_bq2_mag_monitor;
        double lck1_bq2_ph_monitor;
        double lck_aclocks_per_sample_monitor;
        double lck_ilen_monitor;
        double lck_decii_monitor;
        double lck_bq_dec_monitor;
        //
        double lck_frequency;
        double lck_volume;
        double lck_target;
        int    lck_mode;
        double _lck_mode;
        double lck_sens;
        double lck_gain;

        // tmp for BQ config
        double sc_filter_mode;
        double sc_filter_select;
        double sc_bq_coef[6]; // b0,1,2, [a0 (per def. a0:=1), used for := section #id],1,2
        // SECTION 1
        int    sc_bq1mode;
        double sc_bq1_coef[6]; // b0,1,2, [a0 := 1],1,2

        // SECTION 2
        int    sc_bq2mode;
        double sc_bq2_coef[6]; // b0,1,2, [a0 := 1],1,2

        // ZS BQ ANTI FILTER "SECTION 10"
        int    sc_zs_bqmode;
        double sc_zs_bq_coef[6]; // b0,1,2, [a0 := 1],1,2

        // RF-GEN
        int    rf_gen_mode;
        double _rf_gen_mode;
        double rf_gen_frequency;
        double rf_gen_fmscale;
        double rf_gen_out_mux;

        
        double rpspmc_version;
        double rpspmc_date;
        double rpspmc_fpgaimpl;
        double rpspmc_fpgaimpl_date;
        double rpspmc_fpgastartup;
        double rpspmc_fpgastartupcnt;
        double uptime_seconds;
        double rpspmc_initial_transfer_ack;

        double rpspmc_ad5791_glc_ch;
        double rpspmc_ad5791_glc_bit;
        double rpspmc_ad5791_glc_valpos;
        double rpspmc_ad5791_glc_valneg;

};

//#define MAX_GVP_VECTORS   32
//#define GVP_VECTOR_SIZE   16 // 10 components used (1st is index, then: N, nii, Options, Nrep, Next, dx, dy, dz, du, fill w zero to 16)

#if 0
struct SPMC_signals {
        double gvp_vector[GVP_VECTOR_SIZE];
        double xyz_meter[10]; // { X[3], Y[3], Z[3], NOW } [actual, max , min]

        double mon_fifo_t[1024]; // SIGNAL_MON_FIFO_T
        double mon_fifo_x[1024]; // SIGNAL_MON_FIFO_X
        double mon_fifo_y[1024]; // SIGNAL_MON_FIFO_Y
        double mon_fifo_z[1024]; // SIGNAL_MON_FIFO_Z
        double mon_fifo_u[1024]; // SIGNAL_MON_FIFO_U
        //double mon_fifo_a[1024]; // SIGNAL_MON_FIFO_A
        //double mon_fifo_b[1024]; // SIGNAL_MON_FIFO_B
        double mon_fifo_in3[1024]; // SIGNAL_MON_FIFO_IN3
        double mon_fifo_in4[1024]; // SIGNAL_MON_FIFO_IN4
        double mon_fifo_df[1024]; // SIGNAL_MON_FIFO_DF
        //double mon_fifo_amp24]; // SIGNAL_MON_FIFO_AMP
};
#endif

#endif
