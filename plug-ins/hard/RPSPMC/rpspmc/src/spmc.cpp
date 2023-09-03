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

#include <DataManager.h>
#include <CustomParameters.h>

#include "fpga_cfg.h"
#include "spmc.h"

// CONFIGURATION (CFG) DATA REGISTER 0 [1023:0] x 4 = 4k
// PAC-PLL Control Core

// general control paging (future options)
//#define PACPLL_CFG_PAGE_CONTROL  30   // 32bit wide
//#define PACPLL_CFG_PAGE          31   // 32bit wide


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

extern int verbose;

extern CIntParameter     SPMC_GVP_STATUS;
extern CDoubleParameter  SPMC_BIAS_MONITOR;
extern CDoubleParameter  SPMC_SIGNAL_MONITOR;
extern CDoubleParameter  SPMC_X_MONITOR;
extern CDoubleParameter  SPMC_Y_MONITOR;
extern CDoubleParameter  SPMC_Z_MONITOR;


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
	uint8_t write_command[3] = {0, 0, 0};
	uint32_t spi_word = 0;
	//int8_t status = 0;

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
	uint8_t write_command[3] = {0, 0, 0};
	uint32_t spi_word = 0;
	//int8_t status = 0;

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
#if 0
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
	uint32_t old_ctrl = 0;
	uint32_t new_ctrl = 0;
	int32_t status = 0;
#if 0
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

	//AD5791_LDAC_LOW;
	status = ad5791_set_register_value (axis,
                                            AD5791_REG_DAC,
                                            (uint32_t)((unsigned int)round(Q19*volts/SPMC_AD5791_REFV)));
	//AD5791_LDAC_HIGH;

	return status;
}
int32_t ad5791_prepare_dac_value(int axis,
                                 double volts)
{
	int32_t status = 0;

	//AD5791_LDAC_LOW;
	status = ad5791_prepare_register_value (axis,
                                                AD5791_REG_DAC,
                                                (uint32_t)((unsigned int)round(Q19*volts/SPMC_AD5791_REFV)));
	//AD5791_LDAC_HIGH;

	return status;
}

inline double ad5791_dac_to_volts (uint32_t value){ return SPMC_AD5791_REFV*(double)value / Q19; }


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
	new_ctrl = old_ctrl | setup_word;
	status = ad5791_set_register_value(axis,
					   AD5791_REG_CTRL,
					   new_ctrl);

	return status;
}

// initialize/reset all AD5791 channels
void rp_spmc_AD5791_init (){


 	//AD5791_RESET_OUT;
	//AD5791_RESET_HIGH;
	//AD5791_LDAC_OUT;
	//AD5791_LDAC_HIGH;
	//AD5791_CLR_OUT;
	//AD5791_CLR_HIGH;
        
        // power up one by one
        ad5791_setup(0,0);
        ad5791_setup(1,0);
        ad5791_setup(2,0);
        ad5791_setup(3,0);

        // send 0V
        ad5791_prepare_dac_value (0, 0.0);
        ad5791_prepare_dac_value (1, 0.0);
        ad5791_prepare_dac_value (2, 0.0);
        ad5791_set_dac_value (3, 0.0);
}

void rp_spmc_set_bias (double bias){
        if (verbose > 1) fprintf(stderr, "##Configure mode set AD5971 AXIS3 (Bias) to %g V\n", bias);
        ad5791_set_dac_value (3, bias);

        fprintf(stderr, "Set AD5971 AXIS3 (Bias) to %g V\n", bias);
        fprintf(stderr, "MON-UXYZ: %8g  %8g  %8g  %8g V  STATUS: %08X [%g] PASS: %8g V\n",
                ad5791_dac_to_volts (read_gpio_reg_int32 (8,0)),
                ad5791_dac_to_volts (read_gpio_reg_int32 (8,1)),
                ad5791_dac_to_volts (read_gpio_reg_int32 (9,0)),
                ad5791_dac_to_volts (read_gpio_reg_int32 (9,1)),
                read_gpio_reg_int32 (3,1),
                ad5791_dac_to_volts ((read_gpio_reg_int32 (3,1)&0b00001111111111111111111100000000)>>8),
                1.0*(double)read_gpio_reg_int32 (7,1) / Q15
                );            // (3,1) X6 STATUS, (7,1) X14 SIGNAL PASS

        
}

void rp_spmc_set_xyz (double ux, double uy, double uz){
        if (verbose > 1) fprintf(stderr, "##Configure mode set AD5971 AXIS0,1,2 (XYZ) to %g, %g, %g V\n", ux, uy, uz);
        ad5791_prepare_dac_value (0, ux);
        ad5791_prepare_dac_value (1, uy);
        ad5791_set_dac_value     (2, uz);
}


// Main SPM Z Feedback Servo Control
// CONTROL[32] OUT[32]   m[24]  x  c[32]  = 56 M: 24{Q32},  P: 44{Q14}
void rp_spmc_set_zservo_controller (double setpoint, double cp, double ci, double upper, double lower){
        if (verbose > 1) fprintf(stderr, "##Configure RP SPMC Z-Servo Controller: set= %g  cp=%g ci=%g upper=%g lower=%g\n", setpoint, cp, ci, upper, lower); 
        set_gpio_cfgreg_int32 (SPMC_CFG_Z_SERVO_CONTROLLER + SPMC_CFG_SET,   (int)round (Q31*setpoint/SPMC_AD5791_REFV)); // => 23.1 S23Q8 @ +/-5V range in Q31
        set_gpio_cfgreg_int32 (SPMC_CFG_Z_SERVO_CONTROLLER + SPMC_CFG_CP,    (int)round (QZSCOEF * cp)); // Q31
        set_gpio_cfgreg_int32 (SPMC_CFG_Z_SERVO_CONTROLLER + SPMC_CFG_CI,    (int)round (QZSCOEF * ci)); // Q31
        set_gpio_cfgreg_int32 (SPMC_CFG_Z_SERVO_CONTROLLER + SPMC_CFG_UPPER, (int)round (Q31*upper/SPMC_AD5791_REFV)); // => 23.1 S23Q8 @ +/-5V range in Q31
        set_gpio_cfgreg_int32 (SPMC_CFG_Z_SERVO_CONTROLLER + SPMC_CFG_LOWER, (int)round (Q31*lower/SPMC_AD5791_REFV));
}

void rp_spmc_set_zservo_gxsm_speciality_setting (int mode, double z_setpoint, double level){
        if (verbose > 1) fprintf(stderr, "##Configure RP SPMC Z-Servo Controller: mode= %d  Zset=%g level=%g\n", mode, z_setpoint, level); 
        set_gpio_cfgreg_int32 (SPMC_CFG_Z_SERVO_MODE, mode);
        set_gpio_cfgreg_int32 (SPMC_CFG_Z_SERVO_ZSETPOINT, (int)round (Q31*z_setpoint/SPMC_AD5791_REFV)); // => +/-5V range in Q31
        set_gpio_cfgreg_int32 (SPMC_CFG_Z_SERVO_LEVEL, (int)round (Q31*level/SPMC_AD5791_REFV)); // => +/-5V range in Q31
}

// RPSPMC GVP Engine Management
void rp_spmc_gvp_config (bool reset=true, bool program=false, bool pause=false){
        if (!reset)
                rp_spmc_AD5791_set_stream_mode (); // enable streaming for AD5791 DAC's from FPGA now!

        usleep(10000);
                
        set_gpio_cfgreg_int32 (SPMC_GVP_CONTROL,
                               (reset ? 1:0) | (program ? 2:0) | (pause ? 4:0)
                               );
        if (verbose > 1) fprintf(stderr, "##Configure GVP: %08x", (reset ? 1:0) | (program ? 2:0) | (pause ? 4:0)); 
}

//void rp_spmc_set_gvp_vector (CFloatSignal &vector){
void rp_spmc_set_gvp_vector (int pc, int n, int nii, unsigned int opts, int nrp, int nxt, int decii,
                             double dx, double dy, double dz, double du){
        rp_spmc_gvp_config (); // put in reset/hold mode
        usleep(10000);
        // write GVP-Vector [vector[0]] components
        //                      du        dz        dy        dx     Next       Nrep,   Options,     nii,      N,    [Vadr]
        //data = {192'd0, 32'd0000, 32'd0000, -32'd0002, -32'd0002,  32'd0, 32'd0000,   32'h001, 32'd0128, 32'd005, 32'd00 };
        fprintf(stderr, "Write Vector[PC=%03d] = ", pc);
        int i=0;
        set_gpio_cfgreg_int32 (SPMC_GVP_VECTOR_DATA+i++, pc);

        fprintf(stderr, "%04d ", n);
        set_gpio_cfgreg_int32 (SPMC_GVP_VECTOR_DATA+i++, n);

        fprintf(stderr, "%04d ", nii);
        set_gpio_cfgreg_int32 (SPMC_GVP_VECTOR_DATA+i++, nii);

        fprintf(stderr, "%04d ", opts);
        set_gpio_cfgreg_uint32 (SPMC_GVP_VECTOR_DATA+i++, opts);

        fprintf(stderr, "%04d ", nrp);
        set_gpio_cfgreg_int32 (SPMC_GVP_VECTOR_DATA+i++, nrp);

        fprintf(stderr, "%04d ", nxt);
        set_gpio_cfgreg_int32 (SPMC_GVP_VECTOR_DATA+i++, nxt);

        fprintf(stderr, "%8.10g mV ", 1000.*dx);
        set_gpio_cfgreg_int32 (SPMC_GVP_VECTOR_DATA+i++, (int)round(Q31*dx/SPMC_AD5791_REFV));  // => 23.1 S23Q8 @ +/-5V range in Q31

        fprintf(stderr, "%8.10g mV ", 1000.*dy);
        set_gpio_cfgreg_int32 (SPMC_GVP_VECTOR_DATA+i++, (int)round(Q31*dy/SPMC_AD5791_REFV));  // => 23.1 S23Q8 @ +/-5V range in Q31

        fprintf(stderr, "%8.10g mV ", 1000.*dz);
        set_gpio_cfgreg_int32 (SPMC_GVP_VECTOR_DATA+i++, (int)round(Q31*dz/SPMC_AD5791_REFV));  // => 23.1 S23Q8 @ +/-5V range in Q31

        fprintf(stderr, "%8.10g mV ", 1000.*du);
        set_gpio_cfgreg_int32 (SPMC_GVP_VECTOR_DATA+i++, (int)round(Q31*du/SPMC_AD5791_REFV));  // => 23.1 S23Q8 @ +/-5V range in Q31

        fprintf(stderr, "decii=%d ", decii); // last vector componet
        set_gpio_cfgreg_int32 (SPMC_GVP_VECTOR_DATA+15, decii);  // decimation

        fprintf(stderr, "\n" );
        usleep(10000);
        rp_spmc_gvp_config (true, true); // load vector
        usleep(10000);
        rp_spmc_gvp_config (true, false);
}


// RPSPMC Location and Geometry
void rp_spmc_set_rotation (double alpha){
        set_gpio_cfgreg_int32 (SPMC_ROTM_XX, (int)round (Q31*cos(alpha)));
        set_gpio_cfgreg_int32 (SPMC_ROTM_XY, (int)round (Q31*sin(alpha)));
}

void rp_spmc_set_slope (double dzx, double dzy){
        set_gpio_cfgreg_int32 (SPMC_SLOPE_X, (int)round (Q31*dzx));
        set_gpio_cfgreg_int32 (SPMC_SLOPE_Y, (int)round (Q31*dzy));
}

void rp_spmc_set_offsets (double x0, double y0, double z0){
        set_gpio_cfgreg_int32 (SPMC_OFFSET_X, (int)round (Q31*x0/SPMC_AD5791_REFV));
        set_gpio_cfgreg_int32 (SPMC_OFFSET_Y, (int)round (Q31*y0/SPMC_AD5791_REFV));
        set_gpio_cfgreg_int32 (SPMC_OFFSET_Z, (int)round (Q31*z0/SPMC_AD5791_REFV));
}


/*
  Signal Monitoring via GPIO:
  U-Mon: GPIO 7 X15
  X-Mon: GPIO 7 X16
  Y-Mon: GPIO 8 X17
  Z-Mon: GPIO 8 X18
 */

double rp_spmc_read_Bias_Monitor(){
        return SPMC_AD5791_REFV*(double)read_gpio_reg_int32 (8,0) / Q31;
}
double rp_spmc_read_X_Monitor(){
        return SPMC_AD5791_REFV*(double)read_gpio_reg_int32 (8,1) / Q31;
}
double rp_spmc_read_Y_Monitor(){
        return SPMC_AD5791_REFV*(double)read_gpio_reg_int32 (9,0) / Q31;
}
double rp_spmc_read_Z_Monitor(){
        return SPMC_AD5791_REFV*(double)read_gpio_reg_int32 (9,1) / Q31;
}

double rp_spmc_read_Signal_Monitor(){
        return 0.0; //SPMC_AD5791_REFV*(double)read_gpio_reg_int32 (7,1) / Q31;
}

void rp_spmc_update_readings (){
        int gvpstatus = read_gpio_reg_int32 (3,1);
        int gvpsec = read_gpio_reg_int32 (10,0);
        int gvpi   = read_gpio_reg_int32 (10,1);
        SPMC_GVP_STATUS.Value () = gvpstatus;
        fprintf(stderr, "## GVP status: %d S: %x  Sec=%d i%d\n", gvpstatus>>8, gvpstatus&0xff, gvpsec, gvpi);

        SPMC_BIAS_MONITOR.Value () = rp_spmc_read_Bias_Monitor();
        SPMC_X_MONITOR.Value () = rp_spmc_read_X_Monitor();
        SPMC_Y_MONITOR.Value () = rp_spmc_read_Y_Monitor();
        SPMC_Z_MONITOR.Value () = rp_spmc_read_Z_Monitor();

        SPMC_SIGNAL_MONITOR.Value () = rp_spmc_read_Signal_Monitor();

}
