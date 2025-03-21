/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 8 c-style: "K&R" -*- */

/* Gxsm - Gnome X Scanning Microscopy
 * universal STM/AFM/SARLS/SPALEED/... controlling and
 * data analysis software
 * 
 * Copyright (C) 1999,2000,..2023 Percy Zahl
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


typedef unsigned short guint16;
typedef short gint16;

/* Register Map */
#define AD5791_NOP                 0  // No operation (NOP).
#define AD5791_REG_DAC             1  // DAC register.
#define AD5791_REG_CTRL            2  // Control register.
#define AD5791_REG_CLR_CODE        3  // Clearcode register.
#define AD5791_CMD_WR_SOFT_CTRL    4  // Software control register(Write only).

/* Input Shift Register bit definition. */
#define AD5791_READ                (1ul << 23)
#define AD5791_WRITE               (0ul << 23)
#define AD5791_ADDR_REG(x)         (((uint32_t)(x) & 0x7) << 20)

/* Control Register bit Definition */
#define AD5791_CTRL_LINCOMP(x)   (((x) & 0xF) << 6) // Linearity error compensation.
#define AD5791_CTRL_SDODIS       (1 << 5) // SDO pin enable/disable control.
#define AD5791_CTRL_BIN2SC       (1 << 4) // DAC register coding selection.
#define AD5791_CTRL_DACTRI       (1 << 3) // DAC tristate control.
#define AD5791_CTRL_OPGND        (1 << 2) // Output ground clamp control.
#define AD5791_CTRL_RBUF         (1 << 1) // Output amplifier configuration control.

/* Software Control Register bit definition */
#define AD5791_SOFT_CTRL_RESET      (1 << 2) // RESET function.
#define AD5791_SOFT_CTRL_CLR        (1 << 1) // CLR function.
#define AD5791_SOFT_CTRL_LDAC       (1 << 0) // LDAC function.

/* DAC OUTPUT STATES */
#define AD5791_OUT_NORMAL            0x0
#define AD5791_OUT_CLAMPED_6K        0x1
#define AD5791_OUT_TRISTATE          0x2


#ifdef __cplusplus
extern "C" {
#endif

        void rp_spmc_AD5791_set_configuration_mode (bool cmode, bool send, int axis);
        void rp_spmc_AD5791_set_stream_mode (); // AXI
        void rp_spmc_AD5791_set_axis_data (int axis, int data);
        void rp_spmc_AD5791_send_axis_data (int axis, int data);
        void rp_spmc_AD5791_init ();

        ad463x_dev* rp_spmc_AD463x_init ();
        void rp_spmc_AD463x_set_stream_mode (struct ad463x_dev *dev); // AXI
        void rp_spmc_AD463x_test (struct ad463x_dev *dev);

        void rp_spmc_set_bias (double bias);
        void rp_spmc_set_xyzu_DIRECT (double ux, double uy, double uz, double bias);  // WARNING -- instant setting in config mode (test only)
        
        void rp_spmc_set_zservo_controller (double setpoint, double cp, double ci, double upper, double lower);
        void rp_spmc_set_zservo_gxsm_speciality_setting (int mode, double z_setpoint, double in_offset_comp);

        void reset_gvp_positions_uab();
        void rp_spmc_gvp_xzy_reset (); // WARNING!! For corase/mover actions only!
        void rp_spmc_gvp_config (bool reset, bool pause, int reset_options); // taking out of reset starts GVP!
        //void rp_spmc_set_gvp_vector (CFloatSignal &vector);
        void rp_spmc_set_gvp_vector (int pc, int n, unsigned int opts, unsigned int srcs, int nrp, int nxt,
                                     double dx, double dy, double dz, double du,
                                     double da, double db, double dam, double dfm,
                                     double slew,
                                     bool update_life);
        
        void rp_set_gvp_stream_mux_selector (unsigned long selector, unsigned long test_mode, int testval);
        void rp_set_z_servo_stream_mux_selector (unsigned long selector, unsigned long test_mode, int testval);

        int rp_spmc_set_rotation (double alpha, double slew);
        void rp_spmc_set_slope (double dzx, double dzy, double dzxy_slew);
        void rp_spmc_set_offsets (double x0, double y0, double z0, double bias, double xy_move_slew, double z_move_slew);
        void rp_spmc_set_scanpos (double xs, double ys, double slew);

        void rp_spmc_set_modulation (double volume, int target, int dfc_target);

        double rp_spmc_configure_lockin (double freq, double gain, unsigned int mode, double RF_ref_freq, int LCKID);

        void rp_spmc_set_biqad_Lck_F0 (double f_cut, double Q, double Fs, int BIQID);
        void rp_spmc_set_biqad_Lck_F0_pass (int BIQID);
        void rp_spmc_set_biqad_Lck_F0_IIR (double f_cut, double Fs, int BIQID);


        void rp_spmc_update_readings ();

#ifdef __cplusplus
}
#endif
