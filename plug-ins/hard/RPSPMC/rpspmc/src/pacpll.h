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

#pragma once

// Bit Masks for TRANSPORT CONTROL REGISTER
#define PACPLL_CFG_TRANSPORT_RESET           16  // Bit 4=1
#define PACPLL_CFG_TRANSPORT_SINGLE          2   // Bit 1=1
#define PACPLL_CFG_TRANSPORT_LOOP            0   // Bit 1=0
#define PACPLL_CFG_TRANSPORT_START           1   // Bit 0=1

#define PACPLL_CFG_TRANSPORT_CONTROL         6
#define PACPLL_CFG_TRANSPORT_SAMPLES         7
#define PACPLL_CFG_TRANSPORT_DECIMATION      8
#define PACPLL_CFG_TRANSPORT_CHANNEL_SELECT  9


#define READING_MAX_VALUES 14

#define QLMS QN(22)
#define BITS_CORDICSQRT 24
#define BITS_CORDICATAN 24
#define QCORDICSQRT QN(23) // W24: 1Q23
#define QCORDICATAN QN(21) // W24: 3Q21
#define QCORDICSQRTFIR QN64(32-2)
// #define QCORDICATANFIR (QN(24-3)*1024)
#define QCORDICATANFIR QN(32-3)


#define QEXEC   Q31
#define QAMCOEF Q31
#define QFREQ   Q47
#define QPHCOEF Q31
#define QDFCOEF Q31



// FIR filter for GPIO beased readings
#define MAX_FIR_VALUES 9
#define GPIO_FIR_LEN   1024
#define GPIO_READING_DDS_X8 0
#define GPIO_READING_DDS_X9 1

// GPIO READING VECTOR ASSIGNMENTS
#define GPIO_READING_LMS_A  0          // gpio_reading_FIRV_vector[GPIO_READING_LMS_A] / GPIO_FIR_LEN / QLMS
#define GPIO_READING_LMS_B  1          // gpio_reading_FIRV_vector[GPIO_READING_LMS_B] / GPIO_FIR_LEN / QLMS
#define GPIO_READING_OFFSET 2          // / QLMS
#define GPIO_READING_AMPL   3          // gpio_reading_FIRV_vector[GPIO_READING_AMPL] / GPIO_FIR_LEN * 1000 / QCORDICSQRT => mV
#define GPIO_READING_PHASE  4          // ... 180 / QCORDICATAN / M_PI => deg
#define GPIO_READING_EXEC   5          // ... 1000 / QEXEC => mV
#define GPIO_READING_DDS_FREQ       6  // ... * dds_phaseinc_to_freq[[4,1]<<(44-32) | [5,0]>>(64-44)]
#define GPIO_READING_DFREQ          7  // ... * dds_phaseinc_to_freq()
#define GPIO_READING_CONTROL_DFREQ  8  // ... 10000/Q31






typedef unsigned short guint16;
typedef short gint16;


#ifdef __cplusplus
extern "C" {
#endif

  double dds_phaseinc (double freq);
  double dds_phaseinc_to_freq (unsigned long long ddsphaseincQ44);
  double dds_phaseinc_rel_to_freq (long long ddsphaseincQ44);
  void rp_PAC_adjust_dds (double freq);
  void rp_PAC_set_volume (double volume);
  void rp_PAC_configure_switches (int phase_ctrl, int am_ctrl, int phase_unwrap_always, int qcontrol, int lck_amp, int lck_phase, int dfreq_ctrl);
  void rp_PAC_set_qcontrol (double gain, double phase);
  void rp_PAC_set_pactau (double tau, double atau, double dc_tau);
  void rp_PAC_set_dcoff (double dc);

  void rp_PAC_auto_dc_offset_adjust ();
  void rp_PAC_auto_dc_offset_correct ();
  
  void rp_PAC_set_amplitude_controller (double setpoint, double cp, double ci, double upper, double lower);
  void rp_PAC_set_phase_controller (double setpoint, double cp, double ci, double upper, double lower, double am_threashold);
  
  void rp_PAC_set_pulse_form (double bias0, double bias1,
			      double phase0, double phase1,
			      double width0, double width0if,
			      double width1, double width1if,
			      double height0, double height0if,
			      double height1, double height1if,
			      double shapexw, double shapexwif,
			      double shapex, double shapexif);

  void rp_PAC_transport_set_control (int control);
  void rp_PAC_configure_transport (int control, int shr_dec_data, int nsamples, int decimation, int channel_select, double scale, double center);
  void rp_PAC_start_transport (int control_mode, int nsamples, int tr_mode);
  int bram_ready();
  int bram_status(int bram_status[3]);
  void rp_PAC_set_dfreq_controller (double setpoint, double cp, double ci, double upper, double lower);
  void *thread_gpio_reading_FIR(void *arg) ;
  void rp_PAC_get_single_reading_FIR (double reading_vector[READING_MAX_VALUES]);

  void *thread_gpio_reading_FIR(void *arg);

  
#ifdef __cplusplus
}
#endif
