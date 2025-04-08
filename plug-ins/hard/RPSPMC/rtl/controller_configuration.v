`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company:  BNL
// Engineer: Percy Zahl
// 
/* Gxsm - Gnome X Scanning Microscopy 
 * ** FPGA Implementaions RPSPMC aka RedPitaya Scanning Probe Control **
 * universal STM/AFM/SARLS/SPALEED/... controlling and
 * data analysis software
 * 
 * Copyright (C) 1999-2025 by Percy Zahl
 *
 * Authors: Percy Zahl <zahl@users.sf.net>
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
// 
// Create Date: 01/11/2025 12:04:22 AM
// Design Name:    part of RPSPMC
// Module Name:    z_servo_configuration
// Project Name:   RPSPMC 4 GXSM
// Target Devices: Zynq z7020
// Tool Versions:  Vivado 2023.1
// Description:    module configuration block for controller core
// 
// Dependencies: 
// 
// Revision:
// Revision 0.01 - File Created
// Additional Comments:
// 
//////////////////////////////////////////////////////////////////////////////////


module controller_configuration#(
    /* config address */
    parameter controller_reg_address = 99998,
    parameter controller_modes_reg_address = 99999,
    parameter width_limits = 32, // up to 64
    parameter width_consts = 32,
    parameter width_setpoint = 32,
    parameter width_threshold = 32
    )(
    (* X_INTERFACE_PARAMETER = "ASSOCIATED_CLKEN aclk, ASSOCIATED_BUSIF M_AXIS_setpoint:M_AXIS_reset:M_AXIS_threshold" *)
    input aclk,
    
    input [32-1:0]  config_addr,
    input [512-1:0] config_data,
    
    output wire [width_setpoint-1:0] M_AXIS_setpoint_tdata,
    output wire                      M_AXIS_setpoint_tvalid,
    output wire [width_consts-1:0] cp,
    output wire [width_consts-1:0] ci,
    output wire [width_limits-1:0] upper,
    output wire [width_limits-1:0] lower,

    output wire [width_threshold-1:0] M_AXIS_threshold_tdata,
    output wire                       M_AXIS_threshold_tvalid,

    output wire [width_limits-1:0] M_AXIS_reset_tdata,
    output wire                    M_AXIS_reset_tvalid,
    output wire controller_enable,
    output wire controller_mode,
    output wire controller_option_uw,
    output wire controller_option_th,
    output wire controller_hold);
        
    reg [width_setpoint-1:0] r_control_setpoint = 0;
    reg [width_limits-1:0] r_reset_value = 0;
    reg [width_consts-1:0] r_cp = 0;
    reg [width_consts-1:0] r_ci = 0;
    reg [width_limits-1:0] r_upper = 0;
    reg [width_limits-1:0] r_lower = 0;
    reg [width_threshold-1:0] r_threshold;

    reg [32-1:0] r_controller_mode = 0;


    assign M_AXIS_setpoint_tdata  = r_control_setpoint;
    assign M_AXIS_setpoint_tvalid = 1;
    assign cp = r_cp;
    assign ci = r_ci;
    assign upper = r_upper;
    assign lower = r_lower;
    assign M_AXIS_threshold_tdata = r_threshold;
    assign M_AXIS_threshold_tvalid = 1;

    assign M_AXIS_reset_tdata  = r_reset_value;
    assign M_AXIS_reset_tvalid = 1;

    assign controller_enable = r_controller_mode[0:0];
    assign controller_hold   = r_controller_mode[1:1];

    assign controller_option_uw = r_controller_mode[2:2];
    assign controller_option_th = r_controller_mode[3:3];

    always @(posedge aclk)
    begin
        // module configuration
        case (config_addr)
        controller_reg_address:
        begin
            r_control_setpoint <= config_data[0*32+width_setpoint-1 : 0*32]; // up to 32bit
            r_cp               <= config_data[1*32+width_consts-1   : 1*32];
            r_ci               <= config_data[2*32+width_consts-1   : 2*32];
            r_upper            <= config_data[3*32+width_limits-1   : 3*32]; // up to 64bit
            r_lower            <= config_data[5*32+width_limits-1   : 5*32]; // up to 64bit
        end   
          
        controller_modes_reg_address:
        begin
            r_reset_value        <= config_data[0*32+width_limits-1 : 0*32]; // up to 64bit
            r_controller_mode    <= config_data[3*32-1 : 2*32];
            r_threshold          <= config_data[3*32+width_threshold-1 : 3*32];  
        end     
        endcase
        
    end    
    
endmodule
