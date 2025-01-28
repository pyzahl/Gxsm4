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
#include <fstream>
#include <iostream>
#include <set>
#include <streambuf>
#include <string>

#include "fpga_cfg.h"

// PMOD AD5791 SUPPORT

inline double ad5791_dac_to_volts (int value){ return SPMC_AD5791_REFV*(double)value / Q19; }
inline int volts_to_ad5791_dac (double volts){ return (int)round(Q19*volts/SPMC_AD5791_REFV); }


// Integer to binary string. Writes a string of n "bit" characters '1' or '0' as of x&[1<<(N-1)]] to b[0..n-1] and terminating b[n]=0.
// NOTE: b[] must be at least n+1 bytes long, termination 0 is written to b[n].
// Returns passed b pointer for convenience
// Example: { char b16[17]; printf("%s", int_to_binary(b, 1234, 16); }

const char *int_to_binary (char b[], int x, int n){
        char *p = b;
	for (int z = 1<<(n-1); z > 0; z >>= 1)
                *p++ = (x & z) ? '1' : '0';
        *p = 0;
	return b;
}

const char *uint_to_binary (char b[], unsigned int x, int n){
        char *p = b;
	for (int z = 1<<(n-1); z > 0; z >>= 1)
                *p++ = (x & z) ? '1' : '0';
        *p = 0;
	return b;
}

// RP SPMC PMOD AD5791 management functions
// THIS SETS the FPGA SPI interface to configuration mode, direct writes to AD!
void rp_spmc_AD5791_set_configuration_mode (bool cmode=true, bool send=false, int axis=-1){
        static int rp_spmc_AD5791_configuration_axis=0;
        static bool rp_spmc_AD5791_configuration_mode=false;
        static bool rp_spmc_AD5791_configuration_send=false;
        
        rp_spmc_AD5791_configuration_mode = cmode;
        rp_spmc_AD5791_configuration_send = send;
        if (axis >= 0)
                rp_spmc_AD5791_configuration_axis = axis; // change axis
        
        set_gpio_cfgreg_uint32 (SPMC_CFG_AD5791_DAC_CONTROL,
                                (  rp_spmc_AD5791_configuration_axis & 0b111)
                                | (rp_spmc_AD5791_configuration_mode ? (1<<3):0)
                                | (rp_spmc_AD5791_configuration_send ? (1<<4):0)
                                );
        usleep(10000);
        //if (verbose > 1) fprintf(stderr, "##  FPGA AD5781 to %s mode, send %d, axis %d\n", cmode?"config":"streaming", send?1:0, axis);
}

// enable AD5791 FPGA to stream mdoe and flow contol (fast)
void rp_spmc_AD5791_set_stream_mode (){
        rp_spmc_AD5791_set_configuration_mode (false); // disable configuration mode, enable FPGA level control
}

// puts ALL AD5791 DACs on hold and set axis dac for config data to load, does not send it yet, all axis are loaded serially
void rp_spmc_AD5791_set_axis_data (int axis, uint32_t data){
        rp_spmc_AD5791_set_configuration_mode (true, false, axis);  // enter/stay in config mode, set axis
        set_gpio_cfgreg_uint32 (SPMC_CFG_AD5791_DAC_AXIS_DATA, data); // set configuration data
        if (verbose > 1) fprintf(stderr, "##  Set axis data to AD5781 axis %d to %08x\n", axis, data);
}

// puts ALL AD5791 DACs on hold and into configuration mode, sets axiss data and sends alwasy all axis cfg data as set last
void rp_spmc_AD5791_send_axis_data (int axis, uint32_t data){
        rp_spmc_AD5791_set_configuration_mode (true, false, axis);  // enter/stay in config mode, set axis
        set_gpio_cfgreg_uint32 (SPMC_CFG_AD5791_DAC_AXIS_DATA, data); // set configuration data
        usleep(10000);
        rp_spmc_AD5791_set_configuration_mode (true, true); // enter/stay in config mode, enter send data mode, will go into hold mode after data send
        if (verbose > 1) fprintf(stderr, "## Send axis data to AD5781 axis %d to %08x\n", axis, data);
}



/***************************************************************************//**
 * @brief Writes data into a register.
 *
 * @param dev              - The device structure.
 * @param register_address - Address of the register.
 *                          Example:
 *                          AD5791_REG_DAC          - DAC register
 *                          AD5791_REG_CTRL         - Control register
 *                          AD5791_REG_CLR_CODE     - Clearcode register
 *                          AD5791_CMD_WR_SOFT_CTRL - Software control register
 * @param register_value   - Value of the register.
 *
 * @return Returns 0 in case of success or negative error code.
*******************************************************************************/
int32_t ad5791_set_register_value(int axis,
				  uint8_t register_address,
				  uint32_t register_value)
{
	uint32_t spi_word = 0;

	spi_word = AD5791_WRITE |
		   AD5791_ADDR_REG(register_address) |
		   (register_value & 0xFFFFF);
        
        rp_spmc_AD5791_send_axis_data (axis, spi_word); // set and send
        
	return 0;
}
int32_t ad5791_prepare_register_value(int axis,
                                      uint8_t register_address,
                                      uint32_t register_value)
{
	uint32_t spi_word = 0;

	spi_word = AD5791_WRITE |
		   AD5791_ADDR_REG(register_address) |
		   (register_value & 0xFFFFF);
        
        rp_spmc_AD5791_set_axis_data (axis, spi_word); // not sending out, but setting up FPGA channel data
        
	return 0;
}


/***************************************************************************//**
 * @brief Reads the value of a register.
 *
 * @param dev              - The device structure.
 * @param register_address - Address of the register.
 *                          Example:
 *                          AD5791_REG_DAC          - DAC register
 *                          AD5791_REG_CTRL         - Control register
 *                          AD5791_REG_CLR_CODE     - Clearcode register
 *                          AD5791_CMD_WR_SOFT_CTRL - Software control register
 *
 * @return dataRead        - The register's value or negative error code.
*******************************************************************************/
int32_t ad5791_get_register_value(int axis,
				  uint8_t register_address)
{
#if 0  // no read back serial data connected to FPGA. N/A
	uint8_t register_word[3] = {0, 0, 0};
	uint32_t data_read = 0x0;
	int8_t status = 0;

	register_word[0] = (AD5791_READ | AD5791_ADDR_REG(register_address)) >> 16;

        //rp_spmc_AD5791_send_axis_data (axis, spi_word);
        status = spi_write_and_read(axis,
				    register_word,
				    3);
	if(status != 3) {
		return -1;
	}
	register_word[0] = 0x00;
	register_word[1] = 0x00;
	register_word[2] = 0x00;
	status = spi_write_and_read(axis,
				    register_word,
				    3);
	if(status != 3) {
		return -1;
	}
	data_read = ((int32_t)register_word[0] << 16) |
		    ((int32_t)register_word[1] <<  8) |
		    ((int32_t)register_word[2] <<  0);

	return data_read;
#endif
        return 0;
}

/***************************************************************************//**
 * @brief Sets the DAC output in one of the three states.
 *
 * @param dev   - The device structure.
 * @param state - The output state.
 *                Example:
 *                AD5791_OUT_NORMAL     - normal operation mode
 *                AD5791_OUT_CLAMPED_6K - output is clamped via ~6KOhm to AGND
 *                AD5791_OUT_TRISTATE   - output is in tristate
 *
 * @return Negative error code or 0 in case of success.
*******************************************************************************/
int32_t ad5791_dac_ouput_state(int axis,
			       uint8_t state)
{
	int32_t status = 0;
#if 0
	uint32_t old_ctrl = 0;
	uint32_t new_ctrl = 0;
	status = ad5791_get_register_value(axis,
					   AD5791_REG_CTRL);
	if(status < 0) {
		return status;
	}
	old_ctrl = status;
	/* Clear DACTRI and OPGND bits. */
	old_ctrl = old_ctrl & ~(AD5791_CTRL_DACTRI | AD5791_CTRL_OPGND);
	/* Sets the new state provided by the user. */
	new_ctrl = old_ctrl |
		   ((state << 2) & (AD5791_CTRL_DACTRI | AD5791_CTRL_OPGND));
	status = ad5791_set_register_value(axis,
					   AD5791_REG_CTRL,
					   new_ctrl);
#endif
	return status;
}


/***************************************************************************//**
 * @brief Writes to the DAC register.
 *
 * @param dev   - The device structure.
 * @param value - The value to be written to DAC. In Volts, [-5:+5]V
 *
 * @return Negative error code or 0 in case of success.
*******************************************************************************/
int32_t ad5791_set_dac_value(int axis,
			     double volts)
{
	int32_t status = 0;
        union { int i; unsigned int ui; }u;

        u.i = volts_to_ad5791_dac (volts);

        /* 
        if (verbose > 0){
                char s32[33]; memset(s32, 0, 33);
                fprintf(stderr, "##ad5791setdac: %g V in Q19 int = %08x 0b%s\n", volts, u.i, int_to_binary(s32, u.i, 32)); 
                //fprintf(stderr, "##ad5791setdac: %g V in Q19 int = %08x 0b%s\n", volts, u.ui, uint_to_binary(s32, u.ui, 32)); 
        }
        */
        
	status = ad5791_set_register_value (axis,
                                            AD5791_REG_DAC,
                                            (uint32_t)u.ui);

	return status;
}
int32_t ad5791_prepare_dac_value(int axis,
                                 double volts)
{
	int32_t status = 0;
        union { int i; unsigned int ui; }u;
        
        u.i = volts_to_ad5791_dac (volts);

        if (verbose > 1){
                char s32[33]; memset(s32, 0, 33);
                fprintf(stderr, "##ad5791setdac: %g V in Q19 int = %08x 0b%s\n", volts, u.i, int_to_binary(s32, u.i, 32)); 
                //fprintf(stderr, "##ad5791setdac: %g V in Q19 int = %08x 0b%s\n", volts, u.ui, uint_to_binary(s32, u.ui, 32)); 
        }

	status = ad5791_prepare_register_value (axis,
                                                AD5791_REG_DAC,
                                                (uint32_t)u.ui);

	return status;
}



/***************************************************************************//**
 * @brief Asserts RESET, CLR or LDAC in a software manner.
 *
 * @param dev             - The device structure.
 * @param instruction_bit - A Software Control Register bit.
 *                         Example:
 *                         AD5791_SOFT_CTRL_LDAC  - Load DAC
 *                         AD5791_SOFT_CTRL_CLR   - Clear
 *                         AD5791_SOFT_CTRL_RESET - Reset
 *
 * @return Negative error code or 0 in case of success.
*******************************************************************************/
int32_t ad5791_soft_instruction(int axis,
				uint8_t instruction_bit)
{
	int32_t status = 0;

	status = ad5791_set_register_value(axis,
					   AD5791_CMD_WR_SOFT_CTRL,
					   instruction_bit);
	if(status < 0) {
		return status;
	}
	//mdelay(1);    // Wait for the instruction to take effect.

	return status;
}

/***************************************************************************//**
 * @brief Configures the output amplifier, DAC coding, SDO state and the
 *        linearity error compensation.
 *
 * @param dev        - The device structure.
 * @param setup_word - Is a 24-bit value that sets or clears the Control Register
 *                    bits : RBUF bit(AD5791_CTRL_RBUF),
 *                           BIN/2sC bit(AD5791_CTRL_BIN2SC),
 *                           SDODIS bit(AD5791_CTRL_SDODIS) and
 *                           LINCOMP bits(AD5791_CTRL_LINCOMP(x)).
 *                    Example: AD5791_CTRL_BIN2SC | AD5791_CTRL_RBUF - sets
 *                             the DAC register to use offset binary coding and
 *                             powers down the internal output amplifier.
 *
 * @return Negative error code or 0 in case of success.
*******************************************************************************/
int32_t ad5791_setup(int axis,
		     uint32_t setup_word)
{
	uint32_t old_ctrl = 0;
	uint32_t new_ctrl = 0;
	int32_t status = 0;

	/* Reads the control register in order to save the options related to the
	   DAC output state that may have been configured previously. */
	//status = ad5791_get_register_value(axis,
	//				   AD5791_REG_CTRL);
	//if(status < 0) {
	//	return status;
	//}
	//old_ctrl = status;
        old_ctrl = 0b000000000000000000111110; // power up is ---0000111110
	/* Clear LINCOMP, SDODIS, BIN2SC and RBUF bits. */
	old_ctrl = old_ctrl & ~(AD5791_CTRL_LINCOMP(-1) |
				AD5791_CTRL_SDODIS |
				AD5791_CTRL_BIN2SC |
				AD5791_CTRL_RBUF);
	/* Sets the new state provided by the user. */
	//new_ctrl = old_ctrl | setup_word;
	new_ctrl = AD5791_CTRL_SDODIS;
	status = ad5791_set_register_value(axis,
					   AD5791_REG_CTRL,
					   new_ctrl);

	return status;
}

