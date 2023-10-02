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

#include <DataManager.h>
#include <CustomParameters.h>

#include "fpga_cfg.h"
#include "spmc.h"

#include "spmc_stream_server.h"

// RPSPMC
//RDECI=4
//    reg [RDECI:0] rdecii = 0;
//    always @ (posedge a_clk) # 125 MHZ
//    begin
//        rdecii <= rdecii+1;
//    end
//    always @ (posedge rdecii[RDECI]) ...

//RDECI=   1               2         3           4
//[1:0] => 2 bits => 1/4   3 => 1/8  4 => 1/16   5 => 1/32
//00 01 10 11  00 01 10 11  ...
// X  X  X  X   X  X  X  X
//       R            R

#define SPMC_ACLK_MHZ   125 // RP Analog Clock Base in MHz
#define SPMC_RDECI      4
#define SPMC_CLK        ((double)SPMC_ACLK_MHZ*1e6/(1<<(SPMC_RDECI+1)))
#define SPMC_GVP_CLK    ((double)SPMC_ACLK_MHZ*1e6/2)

#define MAX_NUM_PROGRAN_VECTORS 16
#define Q_XYPRECISION Q28


// Controller Core (Servos) Relative Block Offsets:
#define SPMC_CFG_SET   0
#define SPMC_CFG_CP    1
#define SPMC_CFG_CI    2
#define SPMC_CFG_UPPER 3 // 3,4 64bit
#define SPMC_CFG_LOWER 5 // 5,6 64bit

// [CFG0]+10 AMPL Controller
// [CFG0]+20 PHASE Controller
// [CFG1]+00 dFREQ Controller   
// +0 Set, +2 CP, +4 CI, +6 UPPER, +8 LOWER

#define SPMC_AD5791_REFV 5.0 // DAC AD5791 Reference Volatge is 5.000000V (+/-5V Range)
#define QZSCOEF Q31 // Q Z-Servo Controller

// SPM Z-CONTROL SERVO
#define SPMC_CFG_Z_SERVO_CONTROLLER         (PACPLL_CFG1_OFFSET + 10) // 10:16
#define SPMC_CFG_Z_SERVO_ZSETPOINT          (PACPLL_CFG1_OFFSET + 17) // 17
#define SPMC_CFG_Z_SERVO_LEVEL              (PACPLL_CFG1_OFFSET + 18) // 18
#define SPMC_CFG_Z_SERVO_MODE               (PACPLL_CFG1_OFFSET + 19) // 19: SERVO CONTROL (enable) Bit0, ...

// CFG DATA REGISTER 3 [1023:0]
// SPMControl Core

#define SPMC_BASE                     PACPLL_CFG3_OFFSET
#define SPMC_GVP_CONTROL              (SPMC_BASE + 0) // 0: reset 1: setvec
#define SPMC_GVP_VECTOR_DATA          (SPMC_BASE + 1) // 1..16 // 512 bits (16x32)

// GVP VCETOR COMPONETS IN ARRAY AT OFFESTS
//                   decii      du        dz        dy        dx     Next       Nrep,   Options,     nii,      N,    [Vadr]
#define GVP_VEC_VADR   0
#define GVP_VEC_N      1
#define GVP_VEC_NII    2
#define GVP_VEC_OPT    3
#define GVP_VEC_NREP   4
#define GVP_VEC_NEXT   5
#define GVP_VEC_DX     6
#define GVP_VEC_DY     7
#define GVP_VEC_DZ     8
#define GVP_VEC_DU     9
#define GVP_VEC_DA    10
#define GVP_VEC_DB    11
#define GVP_VEC_012   12
#define GVP_VEC_013   13
#define GVP_VEC_014   14
#define GVP_VEC_DECII 15

#define SPMC_CFG_AD5791_DAC_AXIS_DATA (SPMC_BASE + 17) // 32bits
#define SPMC_CFG_AD5791_DAC_CONTROL   (SPMC_BASE + 18) // bits 0,1,2: axis; bit 3: config mode; bit 4: send config data, MUST reset "send bit in config mode to resend next, on hold between"


// SPMC Transformations Core
#define SPMC_ROTM_XX             (SPMC_BASE + 20)  // cos(Alpha)
#define SPMC_ROTM_XY             (SPMC_BASE + 21)  // sin(Alpha)
//#define SPMC_ROTM_YX = -XY = -sin(Alpha)
//#define SPMC_ROTM_YY =  XX =  cos(Alpha)
#define SPMC_SLOPE_X             (SPMC_BASE + 22)
#define SPMC_SLOPE_Y             (SPMC_BASE + 23)
#define SPMC_OFFSET_X            (SPMC_BASE + 24)
#define SPMC_OFFSET_Y            (SPMC_BASE + 25)
#define SPMC_OFFSET_Z            (SPMC_BASE + 26)
#define SPMC_BIAS_REF            (SPMC_BASE + 27)

#define SPMC_XY_MOVE_STEP        (SPMC_BASE + 28)
#define SPMC_Z_MOVE_STEP         (SPMC_BASE + 29)

extern int verbose;

extern CIntParameter     SPMC_GVP_STATUS;
extern CDoubleParameter  SPMC_BIAS_MONITOR;
extern CDoubleParameter  SPMC_SIGNAL_MONITOR;

extern CDoubleParameter  SPMC_X_MONITOR;
extern CDoubleParameter  SPMC_Y_MONITOR;
extern CDoubleParameter  SPMC_Z_MONITOR;

extern CDoubleParameter  SPMC_XS_MONITOR;
extern CDoubleParameter  SPMC_YS_MONITOR;
extern CDoubleParameter  SPMC_ZS_MONITOR;

extern CDoubleParameter  SPMC_X0_MONITOR;
extern CDoubleParameter  SPMC_Y0_MONITOR;
extern CDoubleParameter  SPMC_Z0_MONITOR;

extern int stream_server_control;
extern spmc_stream_server spmc_stream_server_instance;

int x0_buf = 0;
int y0_buf = 0;

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


/* ****
       // TEST AD SERIAL OUT
        dac_send = 0;      
        dac_axis = 0;      
        dac_cfg = 0;
        dac_cfgv = 1;
        #10;

        dac_axis = 0;      
        dac_cfg = 32;
        #10;
        dac_send = 1;      
        #128;
        dac_send = 0;      
        dac_cmode = 0;      

        r=1;
        #20

        // TEST GVP SCAN
        prg=0;
        #20
        // move to start point
        //                  du        dz        dy        dx     Next       Nrep,   Options,     nii,      N,    [Vadr]
        data = {192'd0, 32'd0000, 32'd0000, -32'd0002, -32'd0002,  32'd0, 32'd0000,   32'h001, 32'd0128, 32'd005, 32'd00 };
        #2
        prg=1;
        #20
        prg=0;
        #20

        data = {192'd0, 32'd0004, 32'd0003, 32'd0002, 32'd0001,  32'd0, 32'd0000,   32'h000, 32'd000, 32'd000, 32'd01 }; // END
        #2
        prg=1;
        #20
        prg=0;
        #20

        r=0; // release reset to run
        #20

        wait (fin);

        r=1; // put into reset/hold
        #20

        prg=0;
        #20

        // scan procedure
        // GVP Vector [512 bit max, 10x32 used currently per vector, Num Vectors=8 Currently ]
        // [32bit]-000-fill    9,        8,       7,         6,       5,        4,         3,       2,       1       0
        //                  du        dz        dy        dx     Next       Nrep,   Options,     nii,      N,    [Vadr]
        data = {192'd0, 32'd0000, 32'd0000, 32'd0000, 32'd0256,  32'd0, 32'd0000,   32'h001, 32'd128, 32'd010, 32'd00 };
        #2
        prg=1;
        #20
        prg=0;
        #20
        
        data = {192'd0, 32'd0000, 32'd0000, 32'd0000, -32'sd0256,  32'd0, 32'd0000,   32'h001, 32'd128, 32'd010, 32'd01 };
        #2
        prg=1;
        #20
        prg=0;
        #20

        data = {192'd0, 32'd0000, 32'd0000, 32'd0064, 32'd0000,  -32'sd2, 32'd0010,   32'h001, 32'd128, 32'd001, 32'd02 };
        #2
        prg=1;
        #20
        prg=0;
        #20

        data = {192'd0, 32'd0004, 32'd0003, 32'd0064, 32'd0001,  32'd0, 32'd0000,   32'h000, 32'd000, 32'd000, 32'd03 }; // END
        #2
        prg=1;
        #20
        prg=0;
        #20

        r=0; // release reset to run
        wait (fin);
******** */


/*
 * RedPitaya A9 FPGA Control and Data Transfer
 * ------------------------------------------------------------
 */

/* RP SPMC FPGA Engine Configuration and Control */

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
        if (verbose > 1) fprintf(stderr, "##  FPGA AD5781 to %s mode, send %d, axis %d\n", cmode?"config":"streaming", send?1:0, axis);
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

inline double ad5791_dac_to_volts (int value){ return SPMC_AD5791_REFV*(double)value / Q19; }
inline int volts_to_ad5791_dac (double volts){ return round(Q19*volts/SPMC_AD5791_REFV); }

inline double rpspmc_to_volts (int value){ return SPMC_AD5791_REFV*(double)value / Q31; }
inline int volts_to_rpspmc (double volts){ return round(Q31*volts/SPMC_AD5791_REFV); }


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

        if (verbose >= 0){
                char s32[33]; memset(s32, 0, 33);
                fprintf(stderr, "##ad5791setdac: %g V in Q19 int = %08x 0b%s\n", volts, u.i, int_to_binary(s32, u.i, 32)); 
                //fprintf(stderr, "##ad5791setdac: %g V in Q19 int = %08x 0b%s\n", volts, u.ui, uint_to_binary(s32, u.ui, 32)); 
        }
                
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

// initialize/reset all AD5791 channels
void rp_spmc_AD5791_init (){
        rp_spmc_set_rotation (0.0);

        // power up one by one
        ad5791_setup(0,0);
        ad5791_setup(1,0);
        ad5791_setup(2,0);
        ad5791_setup(3,0);

        usleep(10000);

        // send 0V
        ad5791_prepare_dac_value (0, 0.0);
        ad5791_prepare_dac_value (1, 0.0);
        ad5791_prepare_dac_value (2, 0.0);
        ad5791_set_dac_value (3, 0.0);

        usleep(10000);

        rp_spmc_AD5791_set_stream_mode (); // enable streaming for AD5791 DAC's from FPGA now!
}

void rp_spmc_set_bias (double bias){
        if (verbose > 1) fprintf(stderr, "##Configure mode set AD5971 AXIS3 (Bias) to %g V\n", bias);
        set_gpio_cfgreg_int32 (SPMC_BIAS_REF, volts_to_rpspmc (bias));
}

// WARNING: FOR TEST/CALIBARTION ONLY, direct DAC write via config mode!!!
void rp_spmc_set_xyzu (double ux, double uy, double uz, double bias){
        if (verbose > 1) fprintf(stderr, "##Configure mode set AD5971 AXIS0,1,2,3 (XYZU) to %g V, %g V, %g V, %g V\n", ux, uy, uz, bias);

        ad5791_prepare_dac_value (0, ux);
        ad5791_prepare_dac_value (1, uy);
        ad5791_prepare_dac_value (2, uz);
        ad5791_set_dac_value     (3, bias);

        if (verbose > 1){
                fprintf(stderr, "Set AD5971 AXIS3 (Bias) to %g V\n", bias);
                fprintf(stderr, "MON-UXYZ: %8g  %8g  %8g  %8g V  STATUS: %08X PASS: %8g V\n",
                        rpspmc_to_volts (read_gpio_reg_int32 (8,0)),
                        rpspmc_to_volts (read_gpio_reg_int32 (8,1)),
                        rpspmc_to_volts (read_gpio_reg_int32 (9,0)),
                        rpspmc_to_volts (read_gpio_reg_int32 (9,1)),
                        read_gpio_reg_int32 (3,1),
                        1.0*(double)read_gpio_reg_int32 (7,1) / Q15
                        );            // (3,1) X6 STATUS, (7,1) X14 SIGNAL PASS
        }

}


// Main SPM Z Feedback Servo Control
// CONTROL[32] OUT[32]   m[24]  x  c[32]  = 56 M: 24{Q32},  P: 44{Q14}
void rp_spmc_set_zservo_controller (double setpoint, double cp, double ci, double upper, double lower){
        if (verbose > 1) fprintf(stderr, "##Configure RP SPMC Z-Servo Controller: set= %g  cp=%g ci=%g upper=%g lower=%g\n", setpoint, cp, ci, upper, lower); 
        set_gpio_cfgreg_int32 (SPMC_CFG_Z_SERVO_CONTROLLER + SPMC_CFG_SET,   volts_to_rpspmc (setpoint));
        set_gpio_cfgreg_int32 (SPMC_CFG_Z_SERVO_CONTROLLER + SPMC_CFG_CP,    (int)round (QZSCOEF * cp)); // Q31
        set_gpio_cfgreg_int32 (SPMC_CFG_Z_SERVO_CONTROLLER + SPMC_CFG_CI,    (int)round (QZSCOEF * ci)); // Q31
        set_gpio_cfgreg_int32 (SPMC_CFG_Z_SERVO_CONTROLLER + SPMC_CFG_UPPER, volts_to_rpspmc (upper));
        set_gpio_cfgreg_int32 (SPMC_CFG_Z_SERVO_CONTROLLER + SPMC_CFG_LOWER, volts_to_rpspmc (lower));
}

void rp_spmc_set_zservo_gxsm_speciality_setting (int mode, double z_setpoint, double level){
        if (verbose > 1) fprintf(stderr, "##Configure RP SPMC Z-Servo Controller: mode= %d  Zset=%g level=%g\n", mode, z_setpoint, level); 
        set_gpio_cfgreg_int32 (SPMC_CFG_Z_SERVO_MODE, mode);
        set_gpio_cfgreg_int32 (SPMC_CFG_Z_SERVO_ZSETPOINT, volts_to_rpspmc (z_setpoint));
        set_gpio_cfgreg_int32 (SPMC_CFG_Z_SERVO_LEVEL, volts_to_rpspmc (level));
}

//#define DBG_SETUPGVP

// RPSPMC GVP Engine Management
void rp_spmc_gvp_config (bool reset=true, bool program=false, bool pause=false, int reset_options=-1){
        static int r_options = 0;
        
        if (reset_options >= 0)
                r_options = reset_options&0xffff;
        
        if (!reset){
                rp_spmc_AD5791_set_stream_mode (); // enable streaming for AD5791 DAC's from FPGA now!
                //usleep(1000);
        }
        int cfg = (reset ? 1:0) | (program ? 2:0) | (pause ? 4:0) | (r_options << 16);
        set_gpio_cfgreg_int32 (SPMC_GVP_CONTROL, cfg);
        
        usleep(1000);
#ifdef DBG_SETUPGVP
        if (verbose > 1){
                char s8[9]; memset(s8, 0, 9);
                fprintf(stderr, "##Configure GVP: %08x => [Paus Prg Res] %s\n", cfg, int_to_binary(s8, cfg, 3)); 
                rp_spmc_update_readings ();
        }
#endif
}

void rp_spmc_gvp_init (){
        rp_spmc_gvp_config (); // assure reset+hold mode

        for (int pc=0; pc<8; ++pc){
                if (verbose > 1) fprintf(stderr, "Init Vector[PC=%03d] to zero, decii=32\n", pc);
                // write GVP-Vector [vector[0]] components
                //                   decii      du        dz        dy        dx     Next       Nrep,   Options,     nii,      N,    [Vadr]
                //data = {160'd0, 32'd0064, 32'd0000, 32'd0000, -32'd0002, -32'd0002,  32'd0, 32'd0000,   32'h001, 32'd0128, 32'd005, 32'd00 };
                set_gpio_cfgreg_int32 (SPMC_GVP_VECTOR_DATA + GVP_VEC_VADR, pc);
                for (int i=1; i<16; ++i)
                        set_gpio_cfgreg_int32 (SPMC_GVP_VECTOR_DATA + i, 0);
                set_gpio_cfgreg_int32 (SPMC_GVP_VECTOR_DATA + GVP_VEC_DECII, 32);  // decimation
                rp_spmc_gvp_config (true, true); // load vector
                rp_spmc_gvp_config (true, false);
        }
}

void rp_spmc_set_gvp_vector (int pc, int n, unsigned int opts, int nrp, int nxt,
                             double dx, double dy, double dz, double du,
                             double da, double db,
                             double slew, bool update_life=false){
        if (pc >= MAX_NUM_PROGRAN_VECTORS || pc < 0){
                fprintf(stderr, "ERROR: Vector[PC=%03d] out of valif vector program space. Ignoring. GVP is put into RESET mode now.", pc);
                return;
        }
        unsigned int nii   = 0;
        unsigned int decii = 32;
        double Nsteps = 1.0;

        if (n > 0 && slew > 0.0){
                // find smallest non null delta component
                double dt = 0.0;
                double deltas[6] = { dx,dy,dz,du,da,db };
                double dmin = 10.0; // Volts here, +/-5V is max range
                double dmax =  0.0; // Volts here, +/-5V is max range
                for (int i=0; i<6; ++i){
                        double x = fabs(deltas[i]);
                        if (x > 1e-8 && dmin > x)
                                dmin = x;
                        if (dmax < x)
                                dmax = x;
                }
                double ddmin = dmin/n; // smallest point distance in volts at full step
                double ddmax = dmax/n; // smallest point distance in volts at full step
                double Vstep_prec_Q31 = 5.0/Q31;  // GVP precison:  5/(1<<31)*1e6 = ~0.00232830643653869629 uV
                                                  // DAC precision: 5/(1<<19)*1e6 = 9.5367431640625  uV
                
                //int ddminQ31  = (int)round(Q31*ddmin/SPMC_AD5791_REFV); // smallest point distance in Q31 for DAC in S19Q12  (32 bit total precision fixed point)
                // Error:
                //double ddminE = (double)ddminQ31 - Q31*ddmin/SPMC_AD5791_REFV; // smallest point distance in Q31 for DAC in S19Q12  (32 bit total precision fixed point)

                // SPMC_GVP_CLK = 125/2 MHz
                // slew in V/s
                // => time / point: 1/slew

                dt = 1.0/slew;
                // => NII total = dt*SPMC_GVP_CLK
                double NII_total = dt*SPMC_GVP_CLK;

                // problem: find best factors
                // NII_total = decii * nii

                // 100 A/V  x 20 -> 2000 A/V or 0.5mV/A or 5uV/pm assumptions for max decci step 1pm

                /*
                nii = 1;
                if (ddmin > 2e-6) // 1uV steps min
                        nii = (int)round(ddmin * 1e6);
                */
                
                nii = 3+(unsigned int)round(ddmin/Vstep_prec_Q31)/1e6; // Error < relative 1e6

                decii = (unsigned int)round(NII_total / nii);

                // check for limits, auto adjust
                if (decii < 32){
                        decii = 32;
                        nii = (unsigned int)round(NII_total / decii);
                        if (nii < 2)
                                nii=3;
                }
                //double deciiE = (double)decii - NII_total / nii;

                // total vector steps:
                Nsteps = nii * n;

                if (verbose > 1) fprintf(stderr, "Auto calc decii: slew=%g pts/s, dmin=%g V, dt=%g s, ##=%g, Nsteps=%g {ddmin=%g uV #%g ddmax=%g uV #%g}\n",
                                         slew, dmin, dt, NII_total, Nsteps, ddmin, ddmin/Vstep_prec_Q31, ddmax, ddmax/Vstep_prec_Q31);

                
        }

        if (!update_life)
                rp_spmc_gvp_config (); // assure reset+hold mode // NOT REQUIRED
        else
                if (verbose > 1) fprintf(stderr, "Life Vector Update is ON\n");

        // *** IMPORTANT GVP BEHAVIOR PER DESIGN ***
        // N=5 ii=2 ==> 3 inter vec add steps (2 extra waits), 6 (points) x3 (inter) delta vectors added for section
        
        
        // write GVP-Vector [vector[0]] components
        //           decii  ******     du        dz        dy        dx       Next      Nrep,   Options,     nii,      N,  [Vadr]
        // data = {32'd016, 160'd0, 32'd0004, 32'd0003, 32'd0064, 32'd0001,  32'd0, 32'd0000,   32'h0100, 32'd02, 32'd005, 32'd00 }; // 000

        if (verbose > 1) fprintf(stderr, "Write Vector[PC=%03d] init", pc);
        
        int idv[16];
        if (verbose > 1) fprintf(stderr, "Write Vector[PC=%03d] = [", pc);
        set_gpio_cfgreg_int32 (SPMC_GVP_VECTOR_DATA + GVP_VEC_VADR, idv[0]=pc);

        if (verbose > 1) fprintf(stderr, "%04d, ", n);
        set_gpio_cfgreg_int32 (SPMC_GVP_VECTOR_DATA + GVP_VEC_N, idv[1]=n > 0 ? n-1 : 0); // *** see above note

        if (verbose > 1) fprintf(stderr, "%04d, ", nii);
        set_gpio_cfgreg_uint32 (SPMC_GVP_VECTOR_DATA + GVP_VEC_NII, idv[2]=nii > 0 ? nii-1 : 0); // *** see above note nii > 1 for normal vector, need that time for logic

        if (verbose > 1) fprintf(stderr, "%04d, ", opts);
        set_gpio_cfgreg_uint32 (SPMC_GVP_VECTOR_DATA + GVP_VEC_OPT, idv[3]=opts);

        if (verbose > 1) fprintf(stderr, "%04d, ", nrp);
        set_gpio_cfgreg_int32 (SPMC_GVP_VECTOR_DATA + GVP_VEC_NREP, idv[4]=nrp);

        if (verbose > 1) fprintf(stderr, "%04d, ", nxt);
        set_gpio_cfgreg_int32 (SPMC_GVP_VECTOR_DATA + GVP_VEC_NEXT, idv[5]=nxt);

        if (verbose > 1) fprintf(stderr, "dX %8.10g uV, ", 1e6*dx/Nsteps);
        set_gpio_cfgreg_int32 (SPMC_GVP_VECTOR_DATA + GVP_VEC_DX, idv[6]=(int)round(Q31*dx/Nsteps/SPMC_AD5791_REFV));  // => 23.1 S23Q8 @ +/-5V range in Q31

        if (verbose > 1) fprintf(stderr, "dY %8.10g uV, ", 1e6*dy/Nsteps);
        set_gpio_cfgreg_int32 (SPMC_GVP_VECTOR_DATA + GVP_VEC_DY, idv[7]=(int)round(Q31*dy/Nsteps/SPMC_AD5791_REFV));  // => 23.1 S23Q8 @ +/-5V range in Q31

        if (verbose > 1) fprintf(stderr, "dZ %8.10g uV, ", 1e6*dz/Nsteps);
        set_gpio_cfgreg_int32 (SPMC_GVP_VECTOR_DATA + GVP_VEC_DZ, idv[8]=(int)round(Q31*dz/Nsteps/SPMC_AD5791_REFV));  // => 23.1 S23Q8 @ +/-5V range in Q31

        if (verbose > 1) fprintf(stderr, "dU %8.10g uV, ", 1e6*du/Nsteps);
        set_gpio_cfgreg_int32 (SPMC_GVP_VECTOR_DATA + GVP_VEC_DU, idv[9]=(int)round(Q31*du/Nsteps/SPMC_AD5791_REFV));  // => 23.1 S23Q8 @ +/-5V range in Q31

        if (verbose > 1) fprintf(stderr, "dA %8.10g uV, ", 1e6*da/Nsteps);
        set_gpio_cfgreg_int32 (SPMC_GVP_VECTOR_DATA + GVP_VEC_DA, idv[10]=(int)round(Q31*da/Nsteps/SPMC_AD5791_REFV));  // => 23.1 S23Q8 @ +/-5V range in Q31
        
        if (verbose > 1) fprintf(stderr, "dB %8.10g uV, ", 1e6*db/Nsteps);
        set_gpio_cfgreg_int32 (SPMC_GVP_VECTOR_DATA + GVP_VEC_DB, idv[11]=(int)round(Q31*db/Nsteps/SPMC_AD5791_REFV));  // => 23.1 S23Q8 @ +/-5V range in Q31
        
        set_gpio_cfgreg_int32 (SPMC_GVP_VECTOR_DATA + GVP_VEC_012, idv[12]=0);  // clear bits -- not yet used componets
        set_gpio_cfgreg_int32 (SPMC_GVP_VECTOR_DATA + GVP_VEC_013, idv[13]=0);  // clear bits
        set_gpio_cfgreg_int32 (SPMC_GVP_VECTOR_DATA + GVP_VEC_014, idv[14]=0);  // clear bits

        if (verbose > 1) fprintf(stderr, "0,0,0, decii=%d]\n", idv[15]=decii); // last vector component is decii
        set_gpio_cfgreg_uint32 (SPMC_GVP_VECTOR_DATA + GVP_VEC_DECII, decii);  // decimation

        //<< std::setfill('0') << std::hex << std::setw(8) << data
        
        if (verbose > 0){
                std::stringstream vector_def;
                if (pc == 0)
                        //             vector =   {32'd0, -32'd0, -32'd0, -32'd0, -32'd0, -32'd0, -32'd0, -32'd0, -32'd0, -32'd0, -32'd0, -32'd000, 32'h1001, -32'd0, -32'd0,  32'd1}; // Vector #1
                        vector_def << "VectorDef: { Decii, 0,0,0, dB,dA,dU,dZ,dY,dX, nxt,reps,opts,nii,n, pc}\n";
                vector_def << "vector = {32'd" << ((unsigned int)idv[15]); // decii
                for (int i=14; i>=0; i--){
                        switch (i){
                        case 3:
                                vector_def << std::setfill('0') << std::setw(8) << std::hex << ", 32'h" << ((unsigned int)idv[i]); // SRCS, OPTIONS
                                break;
                        default:
                                vector_def << ((idv[i] < 0) ?", -":",  ") << std::setfill('0') << std::dec << "32'd" << (abs(idv[i]));
                                break;
                        }
                }
                vector_def << "}; // Vector #" << pc;
                spmc_stream_server_instance.add_vector (vector_def.str());
        }

        
        if (update_life){
                rp_spmc_gvp_config (false, true);  // set vector
                rp_spmc_gvp_config (false, false); // continue
        } else {
                rp_spmc_gvp_config (true, true); // load vector
                rp_spmc_gvp_config (true, false);
        }
}


// RPSPMC Location and Geometry
void rp_spmc_set_rotation (double alpha){
        set_gpio_cfgreg_int32 (SPMC_ROTM_XX, (int)round (Q_XYPRECISION*cos(alpha)));
        set_gpio_cfgreg_int32 (SPMC_ROTM_XY, (int)round (Q_XYPRECISION*sin(alpha)));
}

void rp_spmc_set_slope (double dzx, double dzy){
        set_gpio_cfgreg_int32 (SPMC_SLOPE_X, (int)round (Q_XYPRECISION*dzx));
        set_gpio_cfgreg_int32 (SPMC_SLOPE_Y, (int)round (Q_XYPRECISION*dzy));
}

void rp_spmc_set_offsets (double x0, double y0, double z0, double xy_move_slew=-1., double z_move_slew=-1.){
        static int xy_step = 1+(int)(volts_to_rpspmc (0.5/SPMC_CLK));
        static int z_step = 1+(int)(volts_to_rpspmc (0.2/SPMC_CLK));

        // adjust rate is SPMC_CLK in Hz
        if (xy_move_slew >= 0.0)
                xy_step = 1+(int)(volts_to_rpspmc (xy_move_slew/SPMC_CLK));
        if (z_move_slew >= 0.0)
                z_step = 1+(int)(volts_to_rpspmc (z_move_slew/SPMC_CLK));

        if (verbose > 1) fprintf(stderr, "** set offsets [%g %g %g]V {%d %d}/cycle\n", x0, y0, z0, xy_step, z_step);

        set_gpio_cfgreg_int32 (SPMC_OFFSET_X, x0_buf = volts_to_rpspmc (x0));
        set_gpio_cfgreg_int32 (SPMC_OFFSET_Y, y0_buf = volts_to_rpspmc (y0));
        set_gpio_cfgreg_int32 (SPMC_OFFSET_Z, volts_to_rpspmc (z0));
        set_gpio_cfgreg_int32 (SPMC_XY_MOVE_STEP, xy_step);
        set_gpio_cfgreg_int32 (SPMC_Z_MOVE_STEP, z_step);
}

void rp_spmc_set_scanpos (double xs, double ys, double slew){
        // setup GVP to do so...
        
        rp_spmc_gvp_config (); // reset any GVP now!

        // read current xy scan pos
        double x = rpspmc_to_volts (read_gpio_reg_int32 (1,0));
        double y = rpspmc_to_volts (read_gpio_reg_int32 (1,1));

        // make vector to new scan pos requested:
        rp_spmc_set_gvp_vector (0, 400, 0, 0, 0,
                                xs-x, ys-y, 0.0, 0.0,
                                0.0, 0.0,
                                slew, false);
        rp_spmc_set_gvp_vector (0, 0, 0, 0, 0,
                                0.0, 0.0, 0.0, 0.0,
                                0.0, 0.0,
                                0.0, false);
        // and execute
        rp_spmc_gvp_config (false);
}


/*
  Signal Monitoring via GPIO:
  U-Mon: GPIO 7 X15
  X-Mon: GPIO 7 X16
  Y-Mon: GPIO 8 X17
  Z-Mon: GPIO 8 X18
 */


void rp_spmc_update_readings (){

        SPMC_GVP_STATUS.Value () = read_gpio_reg_int32 (3,1);
        //assign dbg_status = { GVP-STATUS, 0,0,0,0,  0, GVP-hold, GVP-finished, z-servo };
        //                      |= { sec[32-3], reset, pause, ~finished }[23:0]
        
        SPMC_BIAS_MONITOR.Value () = rpspmc_to_volts (read_gpio_reg_int32 (8,0));

        SPMC_X_MONITOR.Value () = rpspmc_to_volts (read_gpio_reg_int32 (8,1));
        SPMC_Y_MONITOR.Value () = rpspmc_to_volts (read_gpio_reg_int32 (9,0));
        SPMC_Z_MONITOR.Value () = rpspmc_to_volts (read_gpio_reg_int32 (9,1));

        SPMC_XS_MONITOR.Value () = rpspmc_to_volts (read_gpio_reg_int32 (1,0));
        SPMC_YS_MONITOR.Value () = rpspmc_to_volts (read_gpio_reg_int32 (1,1));
        SPMC_ZS_MONITOR.Value () = rpspmc_to_volts (read_gpio_reg_int32 (10,0));

        SPMC_X0_MONITOR.Value () = rpspmc_to_volts (x0_buf); // ** mirror
        SPMC_Y0_MONITOR.Value () = rpspmc_to_volts (y0_buf); // ** mirror
        SPMC_Z0_MONITOR.Value () = rpspmc_to_volts (read_gpio_reg_int32 (10,1));

        SPMC_SIGNAL_MONITOR.Value () = rpspmc_to_volts (read_gpio_reg_int32 (7,1));
}

