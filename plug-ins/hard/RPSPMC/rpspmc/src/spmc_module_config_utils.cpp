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

#include <time.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/sysinfo.h>
#include <vector>
#include <sys/mman.h>
#include <mutex>

std::mutex cfg_mutex; // Create a mutex object to prohibit threadded interleaving of FPGA configuration bus rescource access mixups


#include "fpga_cfg.h"
#include "spmc_module_config_utils.h"

/*
 * RedPitaya A9 FPGA Control and Data Transfer
 * ------------------------------------------------------------
 */

/* RP SPMC FPGA Engine Configuration and Control */


// SPMC module configuration, mastering CONFIG BUS

void rp_spmc_module_start_config (){
        cfg_mutex.lock(); // claim and lock mutex  ** MUST FINISH WITH module_write_config_data **

        set_gpio_cfgreg_int32 (SPMC_MODULE_CONFIG_ADDR, 0);    // make sure all modules configs are disabled
        usleep(MODULE_ADDR_SETTLE_TIME);
}

void rp_spmc_module_write_config_data (int addr){
        usleep(MODULE_ADDR_SETTLE_TIME);
        set_gpio_cfgreg_int32 (SPMC_MODULE_CONFIG_ADDR, addr); // set address, will fetch data
        usleep(MODULE_ADDR_SETTLE_TIME);
        set_gpio_cfgreg_int32 (SPMC_MODULE_CONFIG_ADDR, 0);    // and disable config bus again

        cfg_mutex.unlock(); // release mutex ** UNLOCK from module_start_config **
}

void rp_spmc_module_config_int32 (int addr, int data, int pos=MODULE_START_VECTOR){
        if (pos > 15 || pos < 0) return;
        if (pos == MODULE_START_VECTOR) rp_spmc_module_start_config ();
        set_gpio_cfgreg_int32 (SPMC_MODULE_CONFIG_DATA+pos, data); // set config data at pos
        if (addr) rp_spmc_module_write_config_data (addr); // and finish write
}

void rp_spmc_module_config_uint32 (int addr, unsigned int data, int pos=MODULE_START_VECTOR){
        if (pos > 15 || pos < 0) return;
        if (pos == MODULE_START_VECTOR) rp_spmc_module_start_config ();
        set_gpio_cfgreg_uint32 (SPMC_MODULE_CONFIG_DATA+pos, data); // set config data at pos
        if (addr) rp_spmc_module_write_config_data (addr); // and finish write
}

void rp_spmc_module_config_int64 (int addr, long long data, int pos=MODULE_START_VECTOR){
        if (pos > 14 || pos < 0) return;
        if (pos == MODULE_START_VECTOR) rp_spmc_module_start_config ();
        set_gpio_cfgreg_int64 (SPMC_MODULE_CONFIG_DATA+pos, data); // set config data at pos
        if (addr) rp_spmc_module_write_config_data (addr); // and finish write
}

void rp_spmc_module_config_int48_16 (int addr,  unsigned long long value, unsigned int n, int pos=MODULE_START_VECTOR){
        if (pos > 14 || pos < 0) return;
        if (pos == MODULE_START_VECTOR) rp_spmc_module_start_config ();
        set_gpio_cfgreg_int48_16 (SPMC_MODULE_CONFIG_DATA+pos, value, n); // set config data at pos
        if (addr) rp_spmc_module_write_config_data (addr); // and finish write
}

void rp_spmc_module_config_Qn (int addr, double data, int pos=MODULE_START_VECTOR, double Qn=Q31){
        if (pos > 15 || pos < 0) return;
        if (pos == MODULE_START_VECTOR) rp_spmc_module_start_config ();
        set_gpio_cfgreg_int32 (SPMC_MODULE_CONFIG_DATA+pos,  int(round(Qn*data))); // set config data at pos
        if (addr) rp_spmc_module_write_config_data (addr); // and finish write
}

void rp_spmc_module_config_vector_Qn (int addr, double data[16], int n, double Qn=Q31){
        if (n > 16 || n < 1) return;
        rp_spmc_module_start_config ();
        for (int i=0; i<n; i++)
                set_gpio_cfgreg_int32 (SPMC_MODULE_CONFIG_DATA+i, int(round(Qn*data[i]))); // set config data -- only 1st 32 of 512
        if (addr) rp_spmc_module_write_config_data (addr); // and finish write
}

void rp_spmc_module_config_int16_int16 (int addr, gint16 dataH, gint16 dataL, int pos=MODULE_START_VECTOR){
        if (pos > 15 || pos < 0) return;
        if (pos == MODULE_START_VECTOR) rp_spmc_module_start_config ();
        set_gpio_cfgreg_int16_int16 (SPMC_MODULE_CONFIG_DATA+pos, dataH, dataL); // set config data at pos
        if (addr) rp_spmc_module_write_config_data (addr); // and finish write
}

void rp_spmc_module_config_uint16_uint16 (int addr, guint16 dataH, guint16 dataL, int pos=MODULE_START_VECTOR){
        if (pos > 15 || pos < 0) return;
        if (pos == MODULE_START_VECTOR) rp_spmc_module_start_config ();
        set_gpio_cfgreg_uint16_uint16 (SPMC_MODULE_CONFIG_DATA+pos, dataH, dataL); // set config data at pos
        if (addr) rp_spmc_module_write_config_data (addr); // and finish write
}



// Readback Module Configuration Data, 2x 32bit/address via regA, regB

void rp_spmc_module_read_config_data (int addr, int *regA, int *regB){
        cfg_mutex.lock(); // ** claim mutex and lock FPGA config bus access **

        set_gpio_cfgreg_int32 (SPMC_MODULE_CONFIG_ADDR, addr); // set address, will prepare data to fetch
        usleep(MODULE_ADDR_SETTLE_TIME);
        *regA = read_gpio_reg_int32 (10,0); // GPIO X19 <= RegA (addr)
        *regB = read_gpio_reg_int32 (10,1); // GPIO X20 <= RegB (addr)
        set_gpio_cfgreg_int32 (SPMC_MODULE_CONFIG_ADDR, 0);    // and disable config bus again

        cfg_mutex.unlock(); // ** unlock FPGA config bus ** 
}

void rp_spmc_module_read_config_data_timing_test (){
        int i=0;
        int regA[200], regB[200];
        clock_t t=clock();
        clock_t tlim=t+CLOCKS_PER_SEC/10;
        clock_t t1, t2, t3, t4;
        int     i1, i2, i3, i4;
        clock_t regT[200];
        
        cfg_mutex.lock(); // ** claim mutex and lock FPGA config bus access **

        regA[i] = read_gpio_reg_int32 (10,0); // GPIO X19 <= RegA (addr)
        regB[i] = read_gpio_reg_int32 (10,1); // GPIO X20 <= RegB (addr)
        regT[i] = clock();
        regA[++i] = read_gpio_reg_int32 (10,0); // GPIO X19 <= RegA (addr)
        regB[i] = read_gpio_reg_int32 (10,1); // GPIO X20 <= RegB (addr)
        regT[i] = clock();
        regA[++i] = read_gpio_reg_int32 (10,0); // GPIO X19 <= RegA (addr)
        regB[i] = read_gpio_reg_int32 (10,1); // GPIO X20 <= RegB (addr)
        regT[i] = clock();
        regA[++i] = read_gpio_reg_int32 (10,0); // GPIO X19 <= RegA (addr)
        regB[i] = read_gpio_reg_int32 (10,1); // GPIO X20 <= RegB (addr)
        regT[i] = clock();
        regA[++i] = read_gpio_reg_int32 (10,0); // GPIO X19 <= RegA (addr)
        regB[i] = read_gpio_reg_int32 (10,1); // GPIO X20 <= RegB (addr)
        regT[i] = clock(); 

        set_gpio_cfgreg_int32 (SPMC_MODULE_CONFIG_ADDR, 0);    // and disable config bus again
        usleep(MODULE_ADDR_SETTLE_TIME);

        regA[++i] = read_gpio_reg_int32 (10,0); // GPIO X19 <= RegA (addr)
        regB[i]   = read_gpio_reg_int32 (10,1); // GPIO X20 <= RegB (addr)
        regT[i] = clock();

        i1=i; t1 = clock();
        set_gpio_cfgreg_int32 (SPMC_MODULE_CONFIG_ADDR, SPMC_READBACK_TEST_RESET_REG);
        do {
                regA[++i] = read_gpio_reg_int32 (10,0); // GPIO X19 <= RegA (addr)
                regB[i]   = read_gpio_reg_int32 (10,1); // GPIO X20 <= RegB (addr)
                regT[i] = clock();
        } while ((regA[i] || regB[i]) && clock() < tlim && i < 190);
        t2 = clock(); i2=i;
        
        set_gpio_cfgreg_int32 (SPMC_MODULE_CONFIG_ADDR, SPMC_READBACK_TEST_VALUE_REG);
        do {
                regA[++i] = read_gpio_reg_int32 (10,0); // GPIO X19 <= RegA (addr)
                regB[i]   = read_gpio_reg_int32 (10,1); // GPIO X20 <= RegB (addr)
                regT[i] = clock();
        } while ((regA[i]==0 && regB[i]==0) && clock() < tlim && i < 190);
        t3 = clock(); i3=i;
        
        regA[++i] = read_gpio_reg_int32 (10,0); // GPIO X19 <= RegA (addr)
        regB[i]   = read_gpio_reg_int32 (10,1); // GPIO X20 <= RegB (addr)
        regT[i] = clock();

        set_gpio_cfgreg_int32 (SPMC_MODULE_CONFIG_ADDR, 0);    // and disable config bus again

        regA[++i] = read_gpio_reg_int32 (10,0); // GPIO X19 <= RegA (addr)
        regB[i]   = read_gpio_reg_int32 (10,1); // GPIO X20 <= RegB (addr)
        regT[i] = clock();
        t4 = clock(); i4=i;

        regA[++i] = read_gpio_reg_int32 (10,0); // GPIO X19 <= RegA (addr)
        regB[i]   = read_gpio_reg_int32 (10,1); // GPIO X20 <= RegB (addr)
        regT[i] = clock();
        regA[++i] = read_gpio_reg_int32 (10,0); // GPIO X19 <= RegA (addr)
        regB[i]   = read_gpio_reg_int32 (10,1); // GPIO X20 <= RegB (addr)
        regT[i] = clock();
        regA[++i] = read_gpio_reg_int32 (10,0); // GPIO X19 <= RegA (addr)
        regB[i]   = read_gpio_reg_int32 (10,1); // GPIO X20 <= RegB (addr)
        regT[i] = clock();

        cfg_mutex.unlock(); // ** unlock FPGA config bus access **
        
        fprintf(stderr, "\n *** rp_spmc_module_read_config_data_timing_test result ***\n");
        fprintf(stderr, "T1[%02d] = %g us\n", i1, 1e6*(t1 - t)/CLOCKS_PER_SEC);
        fprintf(stderr, "T2[%02d] = %g us\n", i2, 1e6*(t2 - t)/CLOCKS_PER_SEC);
        fprintf(stderr, "T2-T1 until set = %g us\n", 1e6*(t2 - t1)/CLOCKS_PER_SEC);
        fprintf(stderr, "T3[%02d] = %g us\n", i3, 1e6*(t3 - t)/CLOCKS_PER_SEC);
        fprintf(stderr, "T3-T2 until set = %g us\n", 1e6*(t3 - t2)/CLOCKS_PER_SEC);
        fprintf(stderr, "T4[%02d] = %g us\n", i4, 1e6*(t4 - t)/CLOCKS_PER_SEC);

        int ra,rb;
        ra=rb=-1;
        fprintf(stderr, "INDEX\t TIME in us\t RegA     \t RegB\t diffs...\n");
        for (int j=0; j <= i; ++j){
                if (ra == regA[j] && rb == regB[j]) continue;
                int da = regA[j]-ra;
                ra=regA[j], rb=regB[j];
                fprintf(stderr, "%05d\t %10g\t %08d\t %08d\t dt %04d\t %g us\n", j, 1e6*(regT[j] - t)/CLOCKS_PER_SEC, ra, rb, fabs(da) < 10000?da:0, fabs(da) < 10000?da/125.0:0);
        }
        fprintf(stderr, "***\n");
}

