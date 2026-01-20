/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 8 c-style: "K&R" -*- */

/* Gnome gxsm - Gnome X Scanning Microscopy
 * universal STM/AFM/SARLS/SPALEED/... controlling and
 * data analysis software
 *
 * Gxsm Plugin Name: spm_scancontrol.h
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

#pragma once

#define DMA_SIZE         0x40000            // 20bit count of 32bit words ==> 1MB DMA Block:  2 x 0x80000 bytes
#define EXPAND_MULTIPLES 32

#define MAX_PROGRAM_VECTORS 32
#define i_X 0
#define i_Y 1
#define i_Z 2



#define CPN(N) ((double)(1LL<<(N))-1.)


#define CPN(N) ((double)(1LL<<(N))-1.)

#define RP_FPGA_QEXEC 31 // Q EXEC READING Controller        -- 1V/(2^RP_FPGA_QEXEC-1)
#define RP_FPGA_QSQRT 23 // Q CORDIC SQRT Amplitude Reading  -- 1V/(2^RP_FPGA_QSQRT-1)
#define RP_FPGA_QATAN 21 // Q CORDIC ATAN Phase Reading      -- 180deg/(PI*(2^RP_FPGA_QATAN-1))
#define RP_FPGA_QFREQ 44 // Q DIFF FREQ READING              -- 125MHz/(2^RP_FPGA_QFREQ-1) well number should not exceed 32bit 

#define DSP32Qs15dot16TO_Volt (50/(32767.*(1<<16)))

#define SPMC_AD5791_REFV 5.0 // DAC AD5791 Reference Volatge is 5.000000V (+/-5V Range)
#define SPMC_AD5791_to_volts (SPMC_AD5791_REFV / QN(31))
#define SPMC_RPIN12_REFV 1.0 // RP RF DACs Reference Voltage is 1.0V (+/-1V Range)
#define SPMC_RPIN12_to_volts (SPMC_RPIN12_REFV / QN(31))
#define SPMC_RPIN34_REFV 5.0 // RP AD463-24 DACs Reference Voltage is 5.0V (Differential +/-5V Range)
#define SPMC_RPIN34_to_volts (SPMC_RPIN34_REFV / QN(31))


