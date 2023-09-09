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

#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/sysinfo.h>
#include <vector>
#include <sys/mman.h>
#include <fcntl.h>
#include <pthread.h>

#include <DataManager.h>
#include <CustomParameters.h>


#include "fpga_cfg.h"

#define FPGA_GPIO_SIZE    0x1000     // 0x1000

extern void *FPGA_PACPLL_bram;
extern void *FPGA_PACPLL_cfg1;
extern void *FPGA_PACPLL_cfg2;
extern void *FPGA_PACPLL_gpio;

extern int verbose;

inline uint8_t* cfg_reg_adr(int cfg_slot){
        size_t off;
        off = cfg_slot * 4;
        if (cfg_slot < 128)
                return ((uint8_t*)FPGA_PACPLL_cfg1 + off);
        else{
                off = (cfg_slot-128) * 4;
                return ((uint8_t*)FPGA_PACPLL_cfg2 + off);
        }
}

inline void set_gpio_cfgreg_int32 (int cfg_slot, int value){
        *((int32_t *)(cfg_reg_adr(cfg_slot))) = value;

        //if (verbose > 2) fprintf(stderr, "set_gpio32[CFG%d] int32 %04x = %08x %d\n", cfg_slot, off, value, value);
}

inline void set_gpio_cfgreg_uint32 (int cfg_slot, unsigned int value){
        *((int32_t *)(cfg_reg_adr(cfg_slot))) = value;
        //size_t off = cfg_slot * 4;

        //if (verbose > 2) fprintf(stderr, "set_gpio32[CFG%d] uint32 %04x = %08x %d\n", cfg_slot, off, value, value);
}

inline void set_gpio_cfgreg_int16_int16 (int cfg_slot, gint16 value_1, gint16 value_2){
        union { struct { gint16 hi, lo; } hl; int ww; } mem;
        mem.hl.hi = value_1;
        mem.hl.lo = value_2;

        *((int32_t *)(cfg_reg_adr(cfg_slot))) = mem.ww;
        
        //if (verbose > 2) fprintf(stderr, "set_gpio32[CFG%d] int16|16 %04x = %04x | %04x\n", cfg_slot, off, value_1, value_2);
}

inline void set_gpio_cfgreg_uint16_uint16 (int cfg_slot, guint16 value_1, guint16 value_2){
        union { struct { guint16 hi, lo; } hl; int ww; } mem;
        mem.hl.hi = value_1;
        mem.hl.lo = value_2;

        *((int32_t *)(cfg_reg_adr(cfg_slot))) = mem.ww;

        //if (verbose > 2) fprintf(stderr, "set_gpio32[CFG%d] uint16|16 %04x = %04x | %04x\n", cfg_slot, off, value_1, value_2);
}

inline void set_gpio_cfgreg_int64 (int cfg_slot, long long value){
        unsigned long long uv = (unsigned long long)value;
        *((int32_t *)(cfg_reg_adr(cfg_slot)))   = uv & 0xffffffff;
        *((int32_t *)(cfg_reg_adr(cfg_slot+1))) = uv >> 32;
}

 
// 48bit in 64cfg reg from top -- eventual precision bits=0
inline void set_gpio_cfgreg_int48 (int cfg_slot, unsigned long long value){
        set_gpio_cfgreg_int64 (cfg_slot, value << 16);
}
 

inline int32_t read_gpio_reg_int32_t (int gpio_block, int pos){
        size_t offset = (gpio_block-1) * FPGA_GPIO_SIZE + pos * 8;
        return *((int32_t *)((uint8_t*)FPGA_PACPLL_gpio + offset));
}

inline int read_gpio_reg_int32 (int gpio_block, int pos){
        size_t offset = (gpio_block-1) * FPGA_GPIO_SIZE + pos * 8;
        int x = *((int32_t *)((uint8_t*)FPGA_PACPLL_gpio + offset));
        return x;
}

inline unsigned int read_gpio_reg_uint32 (int gpio_block, int pos){
        size_t offset = (gpio_block-1) * FPGA_GPIO_SIZE + pos * 8;
        unsigned int x = *((int32_t *)((uint8_t*)FPGA_PACPLL_gpio + offset));
        return x;
}

