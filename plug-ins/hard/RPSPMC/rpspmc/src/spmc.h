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


#ifdef __cplusplus
extern "C" {
#endif

        void rp_spmc_AD5791_set_configuration_mode (bool cmode, bool send, int axis);
        void rp_spmc_AD5791_set_stream_mode ();
        void rp_spmc_AD5791_set_axis_data (int axis, int data);
        void rp_spmc_AD5791_send_axis_data (int axis, int data);
        void rp_spmc_AD5791_init ();
        void rp_spmc_set_bias (double bias); // WARNING -- instant setting
        void rp_spmc_set_xyz (double ux, double uy, double uz);  // WARNING -- instant setting

        void rp_spmc_set_zservo_controller (double setpoint, double cp, double ci, double upper, double lower);
        void rp_spmc_set_zservo_gxsm_speciality_setting (int mode, double z_setpoint, double level);

        void rp_spmc_gvp_config (bool reset, bool program); // taking out of reset starts GVP!
        void rp_spmc_set_gvp_vector (CFloatSignal &vector);

        void rp_spmc_set_rotation (double alpha);
        void rp_spmc_set_slope (double dzx, double dzy);
        void rp_spmc_set_offsets (double x0, double y0, double z0);

        double rp_spmc_read_Bias_Monitor ();
        double rp_spmc_read_X_Monitor ();
        double rp_spmc_read_Y_Monitor ();
        double rp_spmc_read_X_Monitor ();
        void rp_spmc_update_readings ();

#ifdef __cplusplus
}
#endif
