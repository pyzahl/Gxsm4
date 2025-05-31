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

#include "rpspmc_pacpll_json_data.h"

PACPLL_parameters pacpll_parameters;
PACPLL_signals pacpll_signals;

SPMC_parameters spmc_parameters;
SPMC_signals spmc_signals;


JSON_parameter PACPLL_JSON_parameters[] = {
        { "DC_OFFSET", &pacpll_parameters.dc_offset, true },
        { "CPU_LOAD", &pacpll_parameters.cpu_load, true },
        { "COUNTER", &pacpll_parameters.counter, true },
        { "FREE_RAM", &pacpll_parameters.free_ram, true },
        { "EXEC_MONITOR", &pacpll_parameters.exec_amplitude_monitor, true },
        { "DDS_FREQ_MONITOR", &pacpll_parameters.dds_frequency_monitor, true },
        { "VOLUME_MONITOR", &pacpll_parameters.volume_monitor, true },
        { "PHASE_MONITOR", &pacpll_parameters.phase_monitor, true },
        { "DFREQ_MONITOR", &pacpll_parameters.dfreq_monitor, true },
        { "CONTROL_DFREQ_MONITOR", &pacpll_parameters.control_dfreq_monitor, true },

        { "PARAMETER_PERIOD", &pacpll_parameters.parameter_period, false },
        { "SIGNAL_PERIOD", &pacpll_parameters.signal_period, false },
        { "BRAM_WRITE_ADR", &pacpll_parameters.bram_write_adr, true },
        { "BRAM_SAMPLE_POS", &pacpll_parameters.bram_sample_pos, true },
        { "BRAM_FINISHED", &pacpll_parameters.bram_finished, true },

        { "SHR_DEC_DATA", &pacpll_parameters.shr_dec_data, false },
        { "GAIN1", &pacpll_parameters.gain1, false },
        { "GAIN2", &pacpll_parameters.gain2, false },
        { "GAIN3", &pacpll_parameters.gain3, false },
        { "GAIN4", &pacpll_parameters.gain4, false },
        { "GAIN5", &pacpll_parameters.gain5, false },
        { "PAC_DCTAU", &pacpll_parameters.pac_dctau, false },
        { "PACTAU", &pacpll_parameters.pactau, false },
        { "PACATAU", &pacpll_parameters.pacatau, false },

        { "QCONTROL", &pacpll_parameters.qcontrol, false },
        { "QC_GAIN", &pacpll_parameters.qc_gain, false },
        { "QC_PHASE", &pacpll_parameters.qc_phase, false },

        { "LCK_AMPLITUDE", &pacpll_parameters.lck_amplitude, false },
        { "LCK_PHASE", &pacpll_parameters.lck_phase, false },

        { "FREQUENCY_MANUAL", &pacpll_parameters.frequency_manual, false }, // manual/tune frequency
        { "FREQUENCY_CENTER", &pacpll_parameters.frequency_center, false }, // center frequency -- used as offset for AUX
        { "VOLUME_MANUAL", &pacpll_parameters.volume_manual, false },
        { "OPERATION", &pacpll_parameters.operation, false },
        { "PACVERBOSE", &pacpll_parameters.pacverbose, false },
        { "TRANSPORT_DECIMATION", &pacpll_parameters.transport_decimation, false },
        { "TRANSPORT_MODE", &pacpll_parameters.transport_mode, false }, // CH12
        { "TRANSPORT_CH3", &pacpll_parameters.transport_ch3, false },
        { "TRANSPORT_CH4", &pacpll_parameters.transport_ch4, false },
        { "TRANSPORT_CH5", &pacpll_parameters.transport_ch5, false },
        { "TUNE_DFREQ", &pacpll_parameters.tune_dfreq, false },
        { "TUNE_SPAN", &pacpll_parameters.tune_span, false },
        { "CENTER_AMPLITUDE", &pacpll_parameters.center_amplitude, false },
        { "CENTER_PHASE", &pacpll_parameters.center_phase, false },
        { "CENTER_FREQUENCY", &pacpll_parameters.center_frequency, false },
        
        { "AMPLITUDE_FB_SETPOINT", &pacpll_parameters.amplitude_fb_setpoint, false },
        { "AMPLITUDE_FB_CP", &pacpll_parameters.amplitude_fb_cp, false },
        { "AMPLITUDE_FB_CI", &pacpll_parameters.amplitude_fb_ci, false },
        { "EXEC_FB_UPPER", &pacpll_parameters.exec_fb_upper, false },
        { "EXEC_FB_LOWER", &pacpll_parameters.exec_fb_lower, false },
        { "AMPLITUDE_CONTROLLER", &pacpll_parameters.amplitude_controller, false },
        
        { "PHASE_FB_SETPOINT", &pacpll_parameters.phase_fb_setpoint, false },
        { "PHASE_FB_CP", &pacpll_parameters.phase_fb_cp, false },
        { "PHASE_FB_CI", &pacpll_parameters.phase_fb_ci, false },
        { "FREQ_FB_UPPER", &pacpll_parameters.freq_fb_upper, false },
        { "FREQ_FB_LOWER", &pacpll_parameters.freq_fb_lower, false },
        { "PHASE_HOLD_AM_NOISE_LIMIT",  &pacpll_parameters.phase_hold_am_noise_limit, false },
        { "PHASE_CONTROLLER", &pacpll_parameters.phase_controller, false },
        { "PHASE_UNWRAPPING_ALWAYS", &pacpll_parameters.phase_unwrapping_always, false },
        { "PAC_ROT_AB", &pacpll_parameters.phase_rot_ab, false },
        { "SET_SINGLESHOT_TRANSPORT_TRIGGER", &pacpll_parameters.set_singleshot_transport_trigger, false },
        
        { "DFREQ_FB_SETPOINT", &pacpll_parameters.dfreq_fb_setpoint, false },
        { "DFREQ_FB_CP", &pacpll_parameters.dfreq_fb_cp, false },
        { "DFREQ_FB_CI", &pacpll_parameters.dfreq_fb_ci, false },
        { "DFREQ_FB_UPPER", &pacpll_parameters.control_dfreq_fb_upper, false },
        { "DFREQ_FB_LOWER", &pacpll_parameters.control_dfreq_fb_lower, false },
        { "DFREQ_CONTROLLER", &pacpll_parameters.dfreq_controller, false },
        { "DFREQ_CONTROL_Z", &pacpll_parameters.Zdfreq_control, false },
        { "DFREQ_CONTROL_U", &pacpll_parameters.Udfreq_control, false },

        { "PULSE_FORM_BIAS0", &pacpll_parameters.pulse_form_bias0, false },
        { "PULSE_FORM_BIAS1", &pacpll_parameters.pulse_form_bias1, false },
        { "PULSE_FORM_PHASE0", &pacpll_parameters.pulse_form_phase0, false },
        { "PULSE_FORM_PHASE1", &pacpll_parameters.pulse_form_phase1, false },
        { "PULSE_FORM_WIDTH0", &pacpll_parameters.pulse_form_width0, false },
        { "PULSE_FORM_WIDTH0IF", &pacpll_parameters.pulse_form_width0if, false },
        { "PULSE_FORM_WIDTH1", &pacpll_parameters.pulse_form_width1, false },
        { "PULSE_FORM_WIDTH1IF", &pacpll_parameters.pulse_form_width1if, false },
        { "PULSE_FORM_HEIGHT0", &pacpll_parameters.pulse_form_height0, false },
        { "PULSE_FORM_HEIGHT0IF", &pacpll_parameters.pulse_form_height0if, false },
        { "PULSE_FORM_HEIGHT1", &pacpll_parameters.pulse_form_height1, false },
        { "PULSE_FORM_HEIGHT1IF", &pacpll_parameters.pulse_form_height1if, false },
        { "PULSE_FORM_SHAPEXW", &pacpll_parameters.pulse_form_shapexw, false },
        { "PULSE_FORM_SHAPEXWIF", &pacpll_parameters.pulse_form_shapexwif, false },
        { "PULSE_FORM_SHAPEX", &pacpll_parameters.pulse_form_shapex, false },
        { "PULSE_FORM_SHAPEXIF", &pacpll_parameters.pulse_form_shapexif, false },

        { "RPSPMC_DMA_PULL_INTERVAL", &spmc_parameters.rpspmc_dma_pull_interval, false },

        { "SPMC_BIAS", &spmc_parameters.bias, false },

        { "SPMC_GVP_STREAM_MUX", &spmc_parameters.gvp_stream_mux, false },

        { "SPMC_Z_POLARITY", &spmc_parameters.z_polarity, false },

        { "SPMC_Z_SERVO_MODE", &spmc_parameters.z_servo_mode, false },
        { "SPMC_Z_SERVO_SETPOINT", &spmc_parameters.z_servo_setpoint, false },
        { "SPMC_Z_SERVO_CP", &spmc_parameters.z_servo_cp, false },
        { "SPMC_Z_SERVO_CI", &spmc_parameters.z_servo_ci, false },
        { "SPMC_Z_SERVO_UPPER", &spmc_parameters.z_servo_upper, false },
        { "SPMC_Z_SERVO_LOWER", &spmc_parameters.z_servo_lower, false },
        { "SPMC_Z_SERVO_SETPOINT_CZ", &spmc_parameters.z_servo_setpoint_cz, false },
        { "SPMC_Z_SERVO_IN_OFFSETCOMP", &spmc_parameters.z_servo_in_offsetcomp, false },
        { "SPMC_Z_SERVO_SRC_MUX", &spmc_parameters.z_servo_src_mux, false },


        { "SPMC_GVP_EXECUTE", &spmc_parameters.gvp_control, 0 },
        //{ "SPMC_GVP_EXECUTE", &spmc_parameters.gvp_execute, false },
        //{ "SPMC_GVP_PAUSE", &spmc_parameters.gvp_pause, false },
        //{ "SPMC_GVP_STOP", &spmc_parameters.gvp_stop, false },
        //{ "SPMC_GVP_PROGRAM", &spmc_parameters.gvp_program, false },
        { "SPMC_GVP_RESET_OPTIONS", &spmc_parameters.gvp_reset_options, false },
        { "SPMC_GVP_STATUS", &spmc_parameters.gvp_status, true },

        // NO READ BACK OF VECTORS
        { "SPMC_GVP_VECTOR_PC", &spmc_parameters.v[0], false }, // INT Vector[PC] to set
        { "SPMC_GVP_VECTOR__N", &spmc_parameters.v[1], false }, // INT # points
        { "SPMC_GVP_VECTOR__O", &spmc_parameters.v[2], false }, // INT options [Z-Servo Hold, ... , SRCS bits]
        { "SPMC_GVP_VECTORSRC", &spmc_parameters.v[3], false }, // INT options [Z-Servo Hold, ... , SRCS bits]
        { "SPMC_GVP_VECTORNRP", &spmc_parameters.v[4], false }, // INT # repetitions (0=none, i.e. execute and proceed with to next vector)
        { "SPMC_GVP_VECTORNXT", &spmc_parameters.v[5], false }, // INT # loop jump rel to PC to next vector
        { "SPMC_GVP_VECTOR_DX", &spmc_parameters.v[6], false }, // Float: DX in Volts total length of vector component
        { "SPMC_GVP_VECTOR_DY", &spmc_parameters.v[7], false }, // Float: DY in Volts total length of vector component
        { "SPMC_GVP_VECTOR_DZ", &spmc_parameters.v[8], false }, // Float: DZ in Volts total length of vector component
        { "SPMC_GVP_VECTOR_DU", &spmc_parameters.v[9], false }, // Float: DU (Bias) adjust rel to Bias ref in Volts total length of vector component
        { "SPMC_GVP_VECTOR_AA", &spmc_parameters.v[10], false }, // Float: AA (Aux Channel ADC #5) -- reserved
        { "SPMC_GVP_VECTOR_BB", &spmc_parameters.v[11], false }, // Float: BB (Aux Channel ADC #6) -- reserved
        { "SPMC_GVP_VECTOR_AM", &spmc_parameters.v[12], false }, // Float: AM RF=AM control
        { "SPMC_GVP_VECTOR_FM", &spmc_parameters.v[13], false }, // Float: FM RF=FM control
        { "SPMC_GVP_VECTORSLW", &spmc_parameters.v[14], false }, // Float: slew rate in #points / sec

        { "SPMC_GVP_XVECTOR_OPCD", &spmc_parameters.v[15], false }, // INT: VECX OPCODE
        { "SPMC_GVP_XVECTOR_RCHI", &spmc_parameters.v[16], false }, // INT: VECX Ref CH Index
        { "SPMC_GVP_XVECTOR_JMPR", &spmc_parameters.v[17], false }, // INT: VECX JumpRel Dist
        { "SPMC_GVP_XVECTOR_CMPV", &spmc_parameters.v[18], false }, // INT: VECX Compare Value

        
        { "SPMC_ALPHA", &spmc_parameters.alpha, false },
        
        { "SPMC_SLOPE_DZX", &spmc_parameters.slope_dzx, false },
        { "SPMC_SLOPE_DZY", &spmc_parameters.slope_dzy, false },
        { "SPMC_SLOPE_SLEW", &spmc_parameters.slope_slew, false },

        { "SPMC_SET_SCANPOS_X", &spmc_parameters.set_scanpos_x, false },
        { "SPMC_SET_SCANPOS_Y", &spmc_parameters.set_scanpos_y, false },
        { "SPMC_SET_SCANPOS_SLEW", &spmc_parameters.set_scanpos_slew, false },
        { "SPMC_SET_SCANPOS_OPTS", &spmc_parameters.set_scanpos_opts, false },

        { "SPMC_SET_OFFSET_X", &spmc_parameters.set_offset_x, false },
        { "SPMC_SET_OFFSET_Y", &spmc_parameters.set_offset_y, false },
        { "SPMC_SET_OFFSET_Z", &spmc_parameters.set_offset_z, false },

        { "SPMC_SET_OFFSET_XY_SLEW", &spmc_parameters.set_offset_xy_slew, false },
        { "SPMC_SET_OFFSET_Z_SLEW", &spmc_parameters.set_offset_z_slew, false },
        
        // RP SPMC Monitors -- GPIO
        { "SPMC_BIAS_MONITOR", &spmc_parameters.bias_monitor, true }, // GPIO MONITOR: U0, aka Bias as set by GXSM



        { "SPMC_SIGNAL_MONITOR", &spmc_parameters.signal_monitor, true }, // Z servo input signal (current, ... -- processed)
        { "SPMC_AD463X_CH1_MONITOR", &spmc_parameters.ad463x_monitor[0], true }, // AD463X_CH1
        { "SPMC_AD463X_CH2_MONITOR", &spmc_parameters.ad463x_monitor[1], true }, // AD463X_CH2

        { "SPMC_X_MONITOR", &spmc_parameters.x_monitor, true }, // FINAL XYZ POS at DACs
        { "SPMC_Y_MONITOR", &spmc_parameters.y_monitor, true },
        { "SPMC_Z_MONITOR", &spmc_parameters.z_monitor, true },

        { "SPMC_X0_MONITOR", &spmc_parameters.x0_monitor, true }, // OFFSETS
        { "SPMC_Y0_MONITOR", &spmc_parameters.y0_monitor, true },
        { "SPMC_Z0_MONITOR", &spmc_parameters.z0_monitor, true },
        
        { "SPMC_XS_MONITOR", &spmc_parameters.xs_monitor, true }, // SCAN POS
        { "SPMC_YS_MONITOR", &spmc_parameters.ys_monitor, true },
        { "SPMC_ZS_MONITOR", &spmc_parameters.zs_monitor, true },

        { "SPMC_GVP_DATA_POSITION", &spmc_parameters.gvp_data_position, true },

        // FPGA Module Register Monitors
        { "SPMC_BIAS_REG_MONITOR", &spmc_parameters.bias_reg_monitor, true }, // Bias Sum, OUT4
        { "SPMC_BIAS_SET_MONITOR", &spmc_parameters.bias_set_monitor, true }, // U0, aka Bias as set by GXSM

        { "SPMC_GVPU_MONITOR", &spmc_parameters.gvpu_monitor, true },
        { "SPMC_GVPA_MONITOR", &spmc_parameters.gvpa_monitor, true },
        { "SPMC_GVPB_MONITOR", &spmc_parameters.gvpb_monitor, true },
        
        { "SPMC_GVPAMC_MONITOR", &spmc_parameters.gvpamc_monitor, true },
        { "SPMC_GVPFMC_MONITOR", &spmc_parameters.gvpfmc_monitor, true },

        { "SPMC_MUX_MONITOR", &spmc_parameters.mux_monitor, true },
        
        { "SPMC_UPTIME_SECONDS", &spmc_parameters.uptime_seconds, true },
        
        // RP SPMC Lock-In
        { "SPMC_SC_LCK_FREQUENCY", &spmc_parameters.sc_lck_frequency, false }, // manual/tune frequency
        { "SPMC_SC_LCK_VOLUME",    &spmc_parameters.sc_lck_volume, false },    // amplitude
        { "SPMC_SC_LCK_TARGET",    &spmc_parameters.sc_lck_target, false },    // mixing to target
        { "SPMC_SC_LCK_MODE",      &spmc_parameters.sc_lck_mode, false },      // digital Lck & RF Gen mode
        { "SPMC_SC_LCK_AMSCALE",   &spmc_parameters.sc_lck_amscale, false },   // digital AM Mod scale control
        { "SPMC_SC_LCK_FMSCALE",   &spmc_parameters.sc_lck_fmscale, false },   // digital FM Mod scale control
        { "SPMC_SC_LCK_FILTER_MODE", &spmc_parameters.sc_lck_filter_mode, false },    // time const

        { "SPMC_SC_LCK_BQ_COEF_B0",  &spmc_parameters.sc_lck_bq_coef[0], false },    // BQCOEFS
        { "SPMC_SC_LCK_BQ_COEF_B1",  &spmc_parameters.sc_lck_bq_coef[1], false },    // BQCOEFS
        { "SPMC_SC_LCK_BQ_COEF_B2",  &spmc_parameters.sc_lck_bq_coef[2], false },    // BQCOEFS
        { "SPMC_SC_LCK_BQ_COEF_A0",  &spmc_parameters.sc_lck_bq_coef[3], false },    // BQCOEFS
        { "SPMC_SC_LCK_BQ_COEF_A1",  &spmc_parameters.sc_lck_bq_coef[4], false },    // BQCOEFS
        { "SPMC_SC_LCK_BQ_COEF_A2",  &spmc_parameters.sc_lck_bq_coef[5], false },    // BQCOEFS
        { "SPMC_SC_LCK_F0BQ_TAU",  &spmc_parameters.sc_lck_bq_tau, false },    // time const
        { "SPMC_SC_LCK_F0BQ_IIR",  &spmc_parameters.sc_lck_iir_tau, false },   // IIR time const
        { "SPMC_SC_LCK_F0BQ_Q",    &spmc_parameters.sc_lck_q, false },         // Q for IIR

        
        { "SPMC_SC_LCK_RF_FREQUENCY", &spmc_parameters.sc_lck_rf_frequency, false }, // manual/tune RF Gen frequency
        { "SPMC_RF_GEN_OUT_MUX", &spmc_parameters.rf_gen_out_mux, false },

        
        { "RPSPMC_SERVER_VERSION",   &spmc_parameters.rpspmc_version, false },
        { "RPSPMC_SERVER_DATE",      &spmc_parameters.rpspmc_date, false },
        { "RPSPMC_FPGAIMPL_VERSION", &spmc_parameters.rpspmc_fpgaimpl, false },
        { "RPSPMC_FPGAIMPL_DATE",    &spmc_parameters.rpspmc_fpgaimpl_date, false },
        { "RPSPMC_FPGA_STARTUP",     &spmc_parameters.rpspmc_fpgastartup, false },
        { "RPSPMC_FPGA_STARTUPCNT",  &spmc_parameters.rpspmc_fpgastartupcnt, false },

        { "RPSPMC_INITITAL_TRANSFER_ACK",  &spmc_parameters.rpspmc_initial_transfer_ack, false },

        
        { NULL, NULL, true }
};

JSON_signal PACPLL_JSON_signals[] = {
        { "SIGNAL_CH1", 1024, pacpll_signals.signal_ch1 },
        { "SIGNAL_CH2", 1024, pacpll_signals.signal_ch2 },
        { "SIGNAL_CH3", 1024, pacpll_signals.signal_ch3 },
        { "SIGNAL_CH4", 1024, pacpll_signals.signal_ch4 },
        { "SIGNAL_CH5", 1024, pacpll_signals.signal_ch5 },
        { "SIGNAL_FRQ", 1024, pacpll_signals.signal_frq },
        { "SIGNAL_TUNE_PHASE", 1024, pacpll_signals.signal_phase },
        { "SIGNAL_TUNE_AMPL",  1024, pacpll_signals.signal_ampl },
        { "SIGNAL_GPIOX",  16, pacpll_signals.signal_gpiox },

        { "SPMC_GVP_VECTOR", GVP_VECTOR_SIZE, spmc_signals.gvp_vector},

        { "SIGNAL_XYZ_METER", 9, spmc_signals.xyz_meter},

        { NULL, 0, NULL }
};




