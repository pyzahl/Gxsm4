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

#define MODULE_ADDR_SETTLE_TIME 10
        
#define MODULE_SETUP 0
#define MODULE_START_VECTOR    (0)  // ALWAYS start write @POS=0, keep addr=0 for loading data block
#define MODULE_SETUP_VECTOR(N) (N)  // when last write, set addr to module target addr (>0)

        void rp_spmc_module_start_config ();
        void rp_spmc_module_write_config_data (int addr);
        void rp_spmc_module_config_int32 (int addr, int data, int pos);
        void rp_spmc_module_config_int64 (int addr, long long data, int pos);
        void rp_spmc_module_config_int48_16 (int addr,  unsigned long long value, unsigned int n, int pos);

        void rp_spmc_module_config_uint32 (int addr, unsigned int data, int pos);
        void rp_spmc_module_config_Qn (int addr, double data, int pos, double Qn);
        void rp_spmc_module_config_vector_Qn (int addr, double data[16], int n, double Qn);

        void rp_spmc_module_config_int16_int16 (int addr, gint16 dataH, gint16 dataL, int pos);
        void rp_spmc_module_config_uint16_uint16 (int addr, guint16 dataH, guint16 dataL, int pos);

        
        void rp_spmc_module_read_config_data (int addr, int *regA, int *regB);

#ifdef __cplusplus
}
#endif
