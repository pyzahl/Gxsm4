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
        { "DC_OFFSET", &pacpll_parameters.dc_offset, true, "mV" },
        { "CPU_LOAD", &pacpll_parameters.cpu_load, true, "PC" },
        { "COUNTER", &pacpll_parameters.counter, true, "1" },
        { "FREE_RAM", &pacpll_parameters.free_ram, true, "MB" },
        { "EXEC_MONITOR", &pacpll_parameters.exec_amplitude_monitor, true, "mV" },
        { "DDS_FREQ_MONITOR", &pacpll_parameters.dds_frequency_monitor, true, "Hz" },
        { "VOLUME_MONITOR", &pacpll_parameters.volume_monitor, true, "mV" },
        { "PHASE_MONITOR", &pacpll_parameters.phase_monitor, true, "deg" },
        { "DFREQ_MONITOR", &pacpll_parameters.dfreq_monitor, true, "Hz" },
        { "CONTROL_DFREQ_MONITOR", &pacpll_parameters.control_dfreq_monitor, true, "mV1" },

        { "PARAMETER_PERIOD", &pacpll_parameters.parameter_period, false, "ms" },
        { "SIGNAL_PERIOD", &pacpll_parameters.signal_period, false, "ms" },
        { "BRAM_WRITE_ADR", &pacpll_parameters.bram_write_adr, true, "1" },
        { "BRAM_SAMPLE_POS", &pacpll_parameters.bram_sample_pos, true, "1" },
        { "BRAM_FINISHED", &pacpll_parameters.bram_finished, true, "1" },

        { "SHR_DEC_DATA", &pacpll_parameters.shr_dec_data, false, "1" },
        { "GAIN1", &pacpll_parameters.gain1, false, "1" },
        { "GAIN2", &pacpll_parameters.gain2, false, "1" },
        { "GAIN3", &pacpll_parameters.gain3, false, "1" },
        { "GAIN4", &pacpll_parameters.gain4, false, "1" },
        { "GAIN5", &pacpll_parameters.gain5, false, "1" },
        { "PAC_DCTAU", &pacpll_parameters.pac_dctau, false, "ms" },
        { "PACTAU", &pacpll_parameters.pactau, false, "us" },
        { "PACATAU", &pacpll_parameters.pacatau, false, "us" },

        { "QCONTROL", &pacpll_parameters.qcontrol, false, "1" },
        { "QC_GAIN", &pacpll_parameters.qc_gain, false, "1" },
        { "QC_PHASE", &pacpll_parameters.qc_phase, false, "deg" },

        { "LCK_AMPLITUDE", &pacpll_parameters.lck_amplitude, false, "mV1" },
        { "LCK_PHASE", &pacpll_parameters.lck_phase, false, "deg" },

        { "FREQUENCY_MANUAL", &pacpll_parameters.frequency_manual, false, "Hz" }, // manual/tune frequency
        { "FREQUENCY_CENTER", &pacpll_parameters.frequency_center, false, "Hz" }, // center frequency -- used as offset for AUX
        { "VOLUME_MANUAL", &pacpll_parameters.volume_manual, false, "mV1" },
        { "OPERATION", &pacpll_parameters.operation, false, "1" },
        { "PACVERBOSE", &pacpll_parameters.pacverbose, false, "1" },
        { "TRANSPORT_DECIMATION", &pacpll_parameters.transport_decimation, false, "1" },
        { "TRANSPORT_MODE", &pacpll_parameters.transport_mode, false, "1" }, // CH12
        { "TRANSPORT_CH3", &pacpll_parameters.transport_ch3, false, "1" },
        { "TRANSPORT_CH4", &pacpll_parameters.transport_ch4, false, "1" },
        { "TRANSPORT_CH5", &pacpll_parameters.transport_ch5, false, "1" },
        { "TUNE_DFREQ", &pacpll_parameters.tune_dfreq, false, "Hz" },
        { "TUNE_SPAN", &pacpll_parameters.tune_span, false, "Hz" },
        { "CENTER_AMPLITUDE", &pacpll_parameters.center_amplitude, false, "mV1" },
        { "CENTER_PHASE", &pacpll_parameters.center_phase, false, "deg" },
        { "CENTER_FREQUENCY", &pacpll_parameters.center_frequency, false, "Hz" },
        
        { "AMPLITUDE_FB_SETPOINT", &pacpll_parameters.amplitude_fb_setpoint, false, "mV1" },
        { "AMPLITUDE_FB_CP", &pacpll_parameters.amplitude_fb_cp, false, "1" },
        { "AMPLITUDE_FB_CI", &pacpll_parameters.amplitude_fb_ci, false, "1" },
        { "EXEC_FB_UPPER", &pacpll_parameters.exec_fb_upper, false, "mV1" },
        { "EXEC_FB_LOWER", &pacpll_parameters.exec_fb_lower, false, "mV1" },
        { "AMPLITUDE_CONTROLLER", &pacpll_parameters.amplitude_controller, false, "bool" },
        
        { "PHASE_FB_SETPOINT", &pacpll_parameters.phase_fb_setpoint, false, "deg" },
        { "PHASE_FB_CP", &pacpll_parameters.phase_fb_cp, false, "1" },
        { "PHASE_FB_CI", &pacpll_parameters.phase_fb_ci, false, "1" },
        { "FREQ_FB_UPPER", &pacpll_parameters.freq_fb_upper, false, "Hz" },
        { "FREQ_FB_LOWER", &pacpll_parameters.freq_fb_lower, false, "Hz" },
        { "PHASE_HOLD_AM_NOISE_LIMIT",  &pacpll_parameters.phase_hold_am_noise_limit, false, "mV1" },
        { "PHASE_CONTROLLER", &pacpll_parameters.phase_controller, false, "bool" },
        { "PHASE_UNWRAPPING_ALWAYS", &pacpll_parameters.phase_unwrapping_always, false, "bool" },
        { "PAC_ROT_AB", &pacpll_parameters.phase_rot_ab, false, "1" },
        { "SET_SINGLESHOT_TRANSPORT_TRIGGER", &pacpll_parameters.set_singleshot_transport_trigger, false, "bool" },
        
        { "DFREQ_FB_SETPOINT", &pacpll_parameters.dfreq_fb_setpoint, false, "Hz" },
        { "DFREQ_FB_CP", &pacpll_parameters.dfreq_fb_cp, false, "1" },
        { "DFREQ_FB_CI", &pacpll_parameters.dfreq_fb_ci, false, "1" },
        { "DFREQ_FB_UPPER", &pacpll_parameters.control_dfreq_fb_upper, false, "mV1" },
        { "DFREQ_FB_LOWER", &pacpll_parameters.control_dfreq_fb_lower, false, "mV1" },
        { "DFREQ_CONTROLLER", &pacpll_parameters.dfreq_controller, false, "bool" },
        { "DFREQ_CONTROL_Z", &pacpll_parameters.Zdfreq_control, false, "bool" },
        { "DFREQ_CONTROL_U", &pacpll_parameters.Udfreq_control, false, "bool" },

        { "PULSE_FORM_BIAS0", &pacpll_parameters.pulse_form_bias0, false, "mV1" },
        { "PULSE_FORM_BIAS1", &pacpll_parameters.pulse_form_bias1, false, "mV1" },
        { "PULSE_FORM_PHASE0", &pacpll_parameters.pulse_form_phase0, false, "deg" },
        { "PULSE_FORM_PHASE1", &pacpll_parameters.pulse_form_phase1, false, "deg" },
        { "PULSE_FORM_WIDTH0", &pacpll_parameters.pulse_form_width0, false, "us" },
        { "PULSE_FORM_WIDTH0IF", &pacpll_parameters.pulse_form_width0if, false, "us" },
        { "PULSE_FORM_WIDTH1", &pacpll_parameters.pulse_form_width1, false, "us" },
        { "PULSE_FORM_WIDTH1IF", &pacpll_parameters.pulse_form_width1if, false, "us" },
        { "PULSE_FORM_HEIGHT0", &pacpll_parameters.pulse_form_height0, false, "mV1" },
        { "PULSE_FORM_HEIGHT0IF", &pacpll_parameters.pulse_form_height0if, false, "mV1" },
        { "PULSE_FORM_HEIGHT1", &pacpll_parameters.pulse_form_height1, false, "mV1" },
        { "PULSE_FORM_HEIGHT1IF", &pacpll_parameters.pulse_form_height1if, false, "mV1" },
        { "PULSE_FORM_SHAPEXW", &pacpll_parameters.pulse_form_shapexw, false, "1" },
        { "PULSE_FORM_SHAPEXWIF", &pacpll_parameters.pulse_form_shapexwif, false, "1" },
        { "PULSE_FORM_SHAPEX", &pacpll_parameters.pulse_form_shapex, false, "1" },
        { "PULSE_FORM_SHAPEXIF", &pacpll_parameters.pulse_form_shapexif, false, "1" },
        { "PULSE_FORM_TRIGGER_SELECT", &pacpll_parameters.pulse_form_trigger_select, false, "1" },
        { "PULSE_FORM_DPOS0", &pacpll_parameters.pulse_form_dpt[0], false, "1" },
        { "PULSE_FORM_DPOS1", &pacpll_parameters.pulse_form_dpt[1], false, "1" },
        { "PULSE_FORM_DPOS2", &pacpll_parameters.pulse_form_dpt[2], false, "1" },
        { "PULSE_FORM_DPOS3", &pacpll_parameters.pulse_form_dpt[3], false, "1" },
        { "PULSE_FORM_DPVAL0",&pacpll_parameters.pulse_form_dpv[0], false, "1" },
        { "PULSE_FORM_DPVAL1",&pacpll_parameters.pulse_form_dpv[1], false, "1" },
        { "PULSE_FORM_DPVAL2",&pacpll_parameters.pulse_form_dpv[2], false, "1" },
        { "PULSE_FORM_DPVAL3",&pacpll_parameters.pulse_form_dpv[3], false, "1" },

        { "RPSPMC_DMA_PULL_INTERVAL", &spmc_parameters.rpspmc_dma_pull_interval, false, "ms" },

        { "SPMC_BIAS", &spmc_parameters.bias, false, "V" },

        { "SPMC_GVP_STREAM_MUX_0", &spmc_parameters.gvp_stream_mux_0, false, "V" },
        { "SPMC_GVP_STREAM_MUX_1", &spmc_parameters.gvp_stream_mux_1, false, "V" },

        { "SPMC_Z_POLARITY", &spmc_parameters.z_polarity, false, "V" },

        { "SPMC_Z_SERVO_MODE", &spmc_parameters.z_servo_mode, false, "1" },
        { "SPMC_Z_SERVO_SETPOINT", &spmc_parameters.z_servo_setpoint, false, "V" },
        { "SPMC_Z_SERVO_CP", &spmc_parameters.z_servo_cp, false, "1" },
        { "SPMC_Z_SERVO_CI", &spmc_parameters.z_servo_ci, false, "1" },
        { "SPMC_Z_SERVO_UPPER", &spmc_parameters.z_servo_upper, false, "V" },
        { "SPMC_Z_SERVO_LOWER", &spmc_parameters.z_servo_lower, false, "V" },
        { "SPMC_Z_SERVO_SETPOINT_CZ", &spmc_parameters.z_servo_setpoint_cz, false, "V" },
        { "SPMC_Z_SERVO_IN_OFFSETCOMP", &spmc_parameters.z_servo_in_offsetcomp, false, "V" },
        { "SPMC_Z_SERVO_SRC_MUX", &spmc_parameters.z_servo_src_mux, false, "X" },


        { "SPMC_GVP_EXECUTE", &spmc_parameters.gvp_control, 0, "X" },
        //{ "SPMC_GVP_EXECUTE", &spmc_parameters.gvp_execute, false, "X" },
        //{ "SPMC_GVP_PAUSE", &spmc_parameters.gvp_pause, false, "X" },
        //{ "SPMC_GVP_STOP", &spmc_parameters.gvp_stop, false, "X" },
        //{ "SPMC_GVP_PROGRAM", &spmc_parameters.gvp_program, false, "X" },
        { "SPMC_GVP_RESET_OPTIONS", &spmc_parameters.gvp_reset_options, false, "X" },
        { "SPMC_GVP_STATUS", &spmc_parameters.gvp_status, true, "X" },

        // NO READ BACK OF VECTORS
        { "SPMC_GVP_VECTOR_PC", &spmc_parameters.v[0], false, "1" }, // INT Vector[PC] to set
        { "SPMC_GVP_VECTOR__N", &spmc_parameters.v[1], false, "1" }, // INT # points
        { "SPMC_GVP_VECTOR__O", &spmc_parameters.v[2], false, "1" }, // INT options [Z-Servo Hold, ... , SRCS bits]
        { "SPMC_GVP_VECTORSRC", &spmc_parameters.v[3], false, "1" }, // INT options [Z-Servo Hold, ... , SRCS bits]
        { "SPMC_GVP_VECTORNRP", &spmc_parameters.v[4], false, "1" }, // INT # repetitions (0=none, i.e. execute and proceed with to next vector)
        { "SPMC_GVP_VECTORNXT", &spmc_parameters.v[5], false, "1" }, // INT # loop jump rel to PC to next vector
        { "SPMC_GVP_VECTOR_DX", &spmc_parameters.v[6], false, "V" }, // Float: DX in Volts total length of vector component
        { "SPMC_GVP_VECTOR_DY", &spmc_parameters.v[7], false, "V" }, // Float: DY in Volts total length of vector component
        { "SPMC_GVP_VECTOR_DZ", &spmc_parameters.v[8], false, "V" }, // Float: DZ in Volts total length of vector component
        { "SPMC_GVP_VECTOR_DU", &spmc_parameters.v[9], false, "V" }, // Float: DU (Bias) adjust rel to Bias ref in Volts total length of vector component
        { "SPMC_GVP_VECTOR_AA", &spmc_parameters.v[10], false, "V" }, // Float: AA (Aux Channel ADC #5) -- reserved
        { "SPMC_GVP_VECTOR_BB", &spmc_parameters.v[11], false, "V" }, // Float: BB (Aux Channel ADC #6) -- reserved
        { "SPMC_GVP_VECTOR_AM", &spmc_parameters.v[12], false, "V" }, // Float: AM RF=AM control
        { "SPMC_GVP_VECTOR_FM", &spmc_parameters.v[13], false, "V" }, // Float: FM RF=FM control
        { "SPMC_GVP_VECTORSLW", &spmc_parameters.v[14], false, "ptsps" }, // Float: slew rate in #points / sec

        { "SPMC_GVP_XVECTOR_OPCD", &spmc_parameters.v[15], false, "1" }, // INT: VECX OPCODE
        { "SPMC_GVP_XVECTOR_RCHI", &spmc_parameters.v[16], false, "1" }, // INT: VECX Ref CH Index
        { "SPMC_GVP_XVECTOR_JMPR", &spmc_parameters.v[17], false, "1" }, // INT: VECX JumpRel Dist
        { "SPMC_GVP_XVECTOR_CMPV", &spmc_parameters.v[18], false, "V" }, // INT: VECX Compare Value

        
        { "SPMC_ALPHA", &spmc_parameters.alpha, false, "deg" },
        
        { "SPMC_SLOPE_DZX", &spmc_parameters.slope_dzx, false, "1" }, // ratio Ang Z / Ang X
        { "SPMC_SLOPE_DZY", &spmc_parameters.slope_dzy, false, "1" },
        { "SPMC_SLOPE_SLEW", &spmc_parameters.slope_slew, false, "1" },

        { "SPMC_SET_SCANPOS_X", &spmc_parameters.set_scanpos_x, false, "1" },
        { "SPMC_SET_SCANPOS_Y", &spmc_parameters.set_scanpos_y, false, "1" },
        { "SPMC_SET_SCANPOS_SLEW", &spmc_parameters.set_scanpos_slew, false, "1" },
        { "SPMC_SET_SCANPOS_OPTS", &spmc_parameters.set_scanpos_opts, false, "1" },

        { "SPMC_SET_OFFSET_X", &spmc_parameters.set_offset_x, false, "V" },
        { "SPMC_SET_OFFSET_Y", &spmc_parameters.set_offset_y, false, "V" },
        { "SPMC_SET_OFFSET_Z", &spmc_parameters.set_offset_z, false, "V" },

        { "SPMC_SET_OFFSET_XY_SLEW", &spmc_parameters.set_offset_xy_slew, false, "Vps" }, // V/s -- check
        { "SPMC_SET_OFFSET_Z_SLEW", &spmc_parameters.set_offset_z_slew, false, "1" },
        
        // RP SPMC Monitors -- GPIO
        { "SPMC_BIAS_MONITOR", &spmc_parameters.bias_monitor, true, "V" }, // GPIO MONITOR: U0, aka Bias as set by GXSM



        { "SPMC_SIGNAL_MONITOR", &spmc_parameters.signal_monitor, true, "V" }, // Z servo input signal (current, ... -- processed)
        { "SPMC_AD463X_CH1_MONITOR", &spmc_parameters.ad463x_monitor[0], true, "V" }, // AD463X_CH1
        { "SPMC_AD463X_CH2_MONITOR", &spmc_parameters.ad463x_monitor[1], true, "V" }, // AD463X_CH2

        { "SPMC_X_MONITOR", &spmc_parameters.x_monitor, true, "V" }, // FINAL XYZ POS at DACs
        { "SPMC_Y_MONITOR", &spmc_parameters.y_monitor, true, "V" },
        { "SPMC_Z_MONITOR", &spmc_parameters.z_monitor, true, "V" },

        { "SPMC_X0_MONITOR", &spmc_parameters.x0_monitor, true, "V" }, // OFFSETS
        { "SPMC_Y0_MONITOR", &spmc_parameters.y0_monitor, true, "V" },
        { "SPMC_Z0_MONITOR", &spmc_parameters.z0_monitor, true, "V" },
        
        { "SPMC_XS_MONITOR", &spmc_parameters.xs_monitor, true, "V" }, // SCAN POS
        { "SPMC_YS_MONITOR", &spmc_parameters.ys_monitor, true, "V" },
        { "SPMC_ZS_MONITOR", &spmc_parameters.zs_monitor, true, "V" },

        { "SPMC_GVP_DATA_POSITION", &spmc_parameters.gvp_data_position, true, "1" },

        // FPGA Module Register Monitors
        { "SPMC_BIAS_REG_MONITOR", &spmc_parameters.bias_reg_monitor, true, "V" }, // Bias Sum, OUT4
        { "SPMC_BIAS_SET_MONITOR", &spmc_parameters.bias_set_monitor, true, "V" }, // U0, aka Bias as set by GXSM

        { "SPMC_GVPU_MONITOR", &spmc_parameters.gvpu_monitor, true, "V" },
        { "SPMC_GVPA_MONITOR", &spmc_parameters.gvpa_monitor, true, "V" },
        { "SPMC_GVPB_MONITOR", &spmc_parameters.gvpb_monitor, true, "V" },
        
        { "SPMC_GVPAMC_MONITOR", &spmc_parameters.gvpamc_monitor, true, "V" },
        { "SPMC_GVPFMC_MONITOR", &spmc_parameters.gvpfmc_monitor, true, "V" },

        { "SPMC_MUX_MONITOR", &spmc_parameters.mux_monitor, true, "X" },
        
        { "SPMC_UPTIME_SECONDS", &spmc_parameters.uptime_seconds, true, "s" },
        
        // RP SPMC Lock-In
        { "SPMC_LCK_MODE",      &spmc_parameters._lck_mode, false, "1" },      // digital Lck & RF Gen mode
        { "SPMC_LCK_FREQUENCY", &spmc_parameters.lck_frequency, false, "Hz" }, // manual/tune frequency
        { "SPMC_LCK_VOLUME",    &spmc_parameters.lck_volume, false, "V" },    // amplitude
        { "SPMC_LCK_TARGET",    &spmc_parameters.lck_target, false, "1" },    // mixing to target
        { "SPMC_LCK_GAIN",      &spmc_parameters.lck_gain, false, "1" },     // LCK gain

        { "SPMC_LCK1_BQ2_MAG_MONITOR",  &spmc_parameters.lck1_bq2_mag_monitor, false, "V" },  // Monitor Lck1 magnitude
        { "SPMC_LCK1_BQ2_PH_MONITOR",   &spmc_parameters.lck1_bq2_ph_monitor, false, "deg" },  // Monitor Lck1 phase
        

        // RF-GEN
        { "SPMC_RF_GEN_MODE",      &spmc_parameters._rf_gen_mode, false, "X" },      // digital Lck & RF Gen mode
        { "SPMC_RF_GEN_FREQUENCY", &spmc_parameters.rf_gen_frequency, false, "Hz" }, // manual/tune RF Gen frequency
        { "SPMC_RF_GEN_FMSCALE",   &spmc_parameters.rf_gen_fmscale, false, "1" },   // digital FM Mod scale control
        { "SPMC_RF_GEN_OUT_MUX",   &spmc_parameters.rf_gen_out_mux, false, "X" },


        // BQ's
        { "SPMC_SC_FILTER_MODE",   &spmc_parameters.sc_filter_mode, false, "X" },    // time const
        { "SPMC_SC_FILTER_SELECT", &spmc_parameters.sc_filter_mode, false, "X" },    // time const
        { "SPMC_SC_BQ_COEF_B0",  &spmc_parameters.sc_bq_coef[0], false, "1" },    // BQCOEFS
        { "SPMC_SC_BQ_COEF_B1",  &spmc_parameters.sc_bq_coef[1], false, "1" },    // BQCOEFS
        { "SPMC_SC_BQ_COEF_B2",  &spmc_parameters.sc_bq_coef[2], false, "1" },    // BQCOEFS
        { "SPMC_SC_BQ_COEF_A0",  &spmc_parameters.sc_bq_coef[3], false, "1" },    // ** SECTION #ID
        { "SPMC_SC_BQ_COEF_A1",  &spmc_parameters.sc_bq_coef[4], false, "1" },    // BQCOEFS
        { "SPMC_SC_BQ_COEF_A2",  &spmc_parameters.sc_bq_coef[5], false, "1" },    // BQCOEFS

        { "SPMC_LCK_ACLKS_MONITOR", &spmc_parameters.lck_aclocks_per_sample_monitor, false, "1" },
        { "SPMC_LCK_ILEN_MONITOR",  &spmc_parameters.lck_ilen_monitor, false, "1" },
        { "SPMC_LCK_DECII_MONITOR", &spmc_parameters.lck_decii_monitor, false, "1" },
        
        { "RPSPMC_SERVER_VERSION",   &spmc_parameters.rpspmc_version, false, "hex" },
        { "RPSPMC_SERVER_DATE",      &spmc_parameters.rpspmc_date, false, "hex" },
        { "RPSPMC_FPGAIMPL_VERSION", &spmc_parameters.rpspmc_fpgaimpl, false, "hex" },
        { "RPSPMC_FPGAIMPL_DATE",    &spmc_parameters.rpspmc_fpgaimpl_date, false, "hex" },
        { "RPSPMC_FPGA_STARTUP",     &spmc_parameters.rpspmc_fpgastartup, false, "1" },
        { "RPSPMC_FPGA_STARTUPCNT",  &spmc_parameters.rpspmc_fpgastartupcnt, false, "1" },

        { "RPSPMC_INITITAL_TRANSFER_ACK",  &spmc_parameters.rpspmc_initial_transfer_ack, false, "1" },

        { "SPMC_AD5791_GLC_CH",  &spmc_parameters.rpspmc_ad5791_glc_ch, false, "1" },  // PMOD# / DAC Channel
        { "SPMC_AD5791_GLC_BIT", &spmc_parameters.rpspmc_ad5791_glc_bit, false, "1" }, // Bit
        { "SPMC_AD5791_GLC_VALPOS", &spmc_parameters.rpspmc_ad5791_glc_valpos, false, "1" }, // Value Pos 
        { "SPMC_AD5791_GLC_VALNEG", &spmc_parameters.rpspmc_ad5791_glc_valneg, false, "1" }, // Value Neg

        { "SPMC_GVP_RAW_VECTOR_WRITE", &spmc_parameters.rpspmc_gvp_raw_vector_write_mode, false, "1" }, // GVP raw write mode -- direct components write
        { "SPMC_GVP_VECTOR_RAW_NII",   &spmc_parameters.rpspmc_gvp_raw_nii, false, "1" },   // GVP nii for raw write mode only
        { "SPMC_GVP_VECTOR_RAW_DECII", &spmc_parameters.rpspmc_gvp_raw_decii, false, "1" }, // GVP Decii for raw write mode only
        
        { NULL, NULL, true, "" }
};

JSON_signal PACPLL_JSON_signals[] = {
        { "SIGNAL_CH1", 1024, pacpll_signals.signal_ch1, "V" },
        { "SIGNAL_CH2", 1024, pacpll_signals.signal_ch2, "V" },
        { "SIGNAL_CH3", 1024, pacpll_signals.signal_ch3, "V" },
        { "SIGNAL_CH4", 1024, pacpll_signals.signal_ch4, "V" },
        { "SIGNAL_CH5", 1024, pacpll_signals.signal_ch5, "V" },
        { "SIGNAL_FRQ", 1024, pacpll_signals.signal_frq, "V" },
        { "SIGNAL_TUNE_PHASE", 1024, pacpll_signals.signal_phase, "deg" },
        { "SIGNAL_TUNE_AMPL",  1024, pacpll_signals.signal_ampl, "mV1" },
        { "SIGNAL_GPIOX",  16, pacpll_signals.signal_gpiox, "1" },

        { "SPMC_GVP_VECTOR", GVP_VECTOR_SIZE, spmc_signals.gvp_vector, "1" },

        { "SIGNAL_XYZ_METER", 9, spmc_signals.xyz_meter, "V" },

        { "SIGNAL_MON_FIFO_T", 1024, spmc_signals.mon_fifo_t, "s" },
        { "SIGNAL_MON_FIFO_X", 1024, spmc_signals.mon_fifo_x, "V" },
        { "SIGNAL_MON_FIFO_Y", 1024, spmc_signals.mon_fifo_y, "V" },
        { "SIGNAL_MON_FIFO_Z", 1024, spmc_signals.mon_fifo_z, "V" },
        { "SIGNAL_MON_FIFO_U", 1024, spmc_signals.mon_fifo_u, "V" },
        //{ "SIGNAL_MON_FIFO_A", 1024, spmc_signals.mon_fifo_a, "V" },
        //{ "SIGNAL_MON_FIFO_B", 1024, spmc_signals.mon_fifo_b, "V" },
        { "SIGNAL_MON_FIFO_IN3", 1024, spmc_signals.mon_fifo_in3, "V" },
        { "SIGNAL_MON_FIFO_IN4", 1024, spmc_signals.mon_fifo_in4, "V" },
        { "SIGNAL_MON_FIFO_DF", 1024, spmc_signals.mon_fifo_df, "Hz" },
        //{ "SIGNAL_MON_FIFO_AMP", 1024, spmc_signals.mon_fifo_amp, "V" },
        
        { NULL, 0, NULL, "" }
};




