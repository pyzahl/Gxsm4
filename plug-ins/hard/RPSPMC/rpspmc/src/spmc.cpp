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

#define PACPLL_CFG0_OFFSET 0
#define PACPLL_CFG1_OFFSET 32
#define PACPLL_CFG2_OFFSET 64
#define PACPLL_CFG3_OFFSET 96


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

#define SPMC_BASE                    (PACPLL_CFG3_OFFSET + 0)
#define SPMC_GVP_CONTROL              0 // 0: reset 1: setvec
#define SPMC_GVP_VECTOR_DATA          1 // 1..16 // 512 bits (16x32)

#define SPMC_CFG_AD5791_DAC_AXIS_DATA 17 // 32bits
#define SPMC_CFG_AD5791_DAC_CONTROL   18 // bits 0,1,2: axis; bit 3: config mode; bit 4: send config data, MUST reset "send bit in config mode to resend next, on hold between"


// SPMC Transformations Core
#define SPMC_ROTM_XX             20  // cos(Alpha)
#define SPMC_ROTM_XY             21  // sin(Alpha)
//#define SPMC_ROTM_YX = -XY = -sin(Alpha)
//#define SPMC_ROTM_YY =  XX =  cos(Alpha)
#define SPMC_SLOPE_X             22
#define SPMC_SLOPE_Y             23
#define SPMC_OFFSET_X            24
#define SPMC_OFFSET_Y            25
#define SPMC_OFFSET_Z            26

extern int verbose;

extern CDoubleParameter  SPMC_BIAS_MONITOR;
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
void rp_spmc_AD5791_set_configuration_mode (bool cmode=true, bool send=false, int axis=-1){
        static int rp_spmc_AD5791_configuration_axis=0;
        static bool rp_spmc_AD5791_configuration_mode=false;
        static bool rp_spmc_AD5791_configuration_send=false;
        
        rp_spmc_AD5791_configuration_mode = cmode;
        rp_spmc_AD5791_configuration_send = send;
        if (axis >= 0)
                rp_spmc_AD5791_configuration_axis = axis; // change axis
        
        set_gpio_cfgreg_int32 (SPMC_CFG_AD5791_DAC_CONTROL,
                               (  rp_spmc_AD5791_configuration_axis & 3)
                               | (rp_spmc_AD5791_configuration_mode ? (1<<3):0)
                               | (rp_spmc_AD5791_configuration_send ? (1<<4):0)
                               );
}

// enable AD5791 FPGA to stream mdoe and flow contol (fast)
void rp_spmc_AD5791_set_stream_mode (){
        rp_spmc_AD5791_set_configuration_mode (false); // disable configuration mode, enable FPGA level control
}

// puts ALL AD5791 DACs on hold and set axis dac for config data to load, does not send it yet, all axis are loaded serially
void rp_spmc_AD5791_set_axis_data (int axis, int data){
        rp_spmc_AD5791_set_configuration_mode (true, false, axis);  // enter/stay in config mode, set axis
        set_gpio_cfgreg_int32 (SPMC_CFG_AD5791_DAC_AXIS_DATA, data); // set configuration data
}

// puts ALL AD5791 DACs on hold and into configuration mode, sets axiss data and sends alwasy all axis cfg data as set last
void rp_spmc_AD5791_send_axis_data (int axis, int data){
        rp_spmc_AD5791_set_configuration_mode (true, false, axis);  // enter/stay in config mode, set axis
        set_gpio_cfgreg_int32 (SPMC_CFG_AD5791_DAC_AXIS_DATA, data); // set configuration data
        rp_spmc_AD5791_set_configuration_mode (true, true); // enter/stay in config mode, enter send data mode, will go into hold mode after data send
}

// initialize/reset all AD5791 channels
void rp_spmc_AD5791_init (){
        rp_spmc_AD5791_set_axis_data (0, 0);
        rp_spmc_AD5791_set_axis_data (1, 0);
        rp_spmc_AD5791_set_axis_data (2, 0);
        rp_spmc_AD5791_send_axis_data (3, 0); // sends all axis dat out at once
}

void rp_spmc_set_bias (double bias){
        if (verbose > 1) fprintf(stderr, "##Configure mode set AD5971 AXIS3 (Bias) to %g V\n", bias);
        rp_spmc_AD5791_send_axis_data (3, (int)round(Q24*bias/SPMC_AD5791_REFV));
}

void rp_spmc_set_xyz (double ux, double uy, double uz){
        if (verbose > 1) fprintf(stderr, "##Configure mode set AD5971 AXIS0,1,2 (XYZ) to %g, %g, %g V\n", ux, uy, uz);
        rp_spmc_AD5791_set_axis_data (0, (int)round(Q24*ux/SPMC_AD5791_REFV));
        rp_spmc_AD5791_set_axis_data (1, (int)round(Q24*uy/SPMC_AD5791_REFV));
        rp_spmc_AD5791_send_axis_data (2, (int)round(Q24*uz/SPMC_AD5791_REFV));
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
        set_gpio_cfgreg_int32 (SPMC_CFG_Z_SERVO_MODE, mode);
        set_gpio_cfgreg_int32 (SPMC_CFG_Z_SERVO_ZSETPOINT, (int)round (Q31*z_setpoint/SPMC_AD5791_REFV)); // => +/-5V range in Q31
        set_gpio_cfgreg_int32 (SPMC_CFG_Z_SERVO_LEVEL, (int)round (Q31*level/SPMC_AD5791_REFV)); // => +/-5V range in Q31
}

// RPSPMC GVP Engine Management
void rp_spmc_gvp_config (bool reset=true, bool program=false){
        set_gpio_cfgreg_int32 (SPMC_GVP_CONTROL,
                               (reset ? 1:0) | (program ? 2:0)
                               );
}

void rp_spmc_set_gvp_vector (CFloatSignal &vector){
        rp_spmc_gvp_config (); // put in reset/hold mode
        usleep(10);
        // write GVP-Vector [vector[0]] components
        //                      du        dz        dy        dx     Next       Nrep,   Options,     nii,      N,    [Vadr]
        //data = {192'd0, 32'd0000, 32'd0000, -32'd0002, -32'd0002,  32'd0, 32'd0000,   32'h001, 32'd0128, 32'd005, 32'd00 };
        for (int i=0; i<16; ++i){
                int x=0;
                if (i < 10){
                        if (i > 4) // delta components in Volts
                                x = (int)round(Q31*vector[i]/SPMC_AD5791_REFV);  // => 23.1 S23Q8 @ +/-5V range in Q31
                        else
                                x = (int)round(vector[i]);
                }
                set_gpio_cfgreg_int32 (SPMC_GVP_VECTOR_DATA+i, x);
        }
        usleep(10);
        rp_spmc_gvp_config (true, true); // load vector
        usleep(10);
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
        return SPMC_AD5791_REFV*(double)read_gpio_reg_int32 (7,0) / Q31;
}
double rp_spmc_read_X_Monitor(){
        return SPMC_AD5791_REFV*(double)read_gpio_reg_int32 (7,1) / Q31;
}
double rp_spmc_read_Y_Monitor(){
        return SPMC_AD5791_REFV*(double)read_gpio_reg_int32 (8,0) / Q31;
}
double rp_spmc_read_Z_Monitor(){
        return SPMC_AD5791_REFV*(double)read_gpio_reg_int32 (8,1) / Q31;
}

void rp_spmc_update_readings (){
        SPMC_BIAS_MONITOR.Value () = rp_spmc_read_Bias_Monitor();
        SPMC_X_MONITOR.Value () = rp_spmc_read_X_Monitor();
        SPMC_Y_MONITOR.Value () = rp_spmc_read_Y_Monitor();
        SPMC_Z_MONITOR.Value () = rp_spmc_read_Z_Monitor();
}
