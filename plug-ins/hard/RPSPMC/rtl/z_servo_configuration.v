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
// Design Name: part of RPSPMC
// Module Name: z_servo_configuration
// Project Name:   RPSPMC 4 GXSM
// Target Devices: Zynq z7020
// Tool Versions:  Vivado 2023.1
// Description: 
// 
// Dependencies: 
// 
// Revision:
// Revision 0.01 - File Created
// Additional Comments:
// 
//////////////////////////////////////////////////////////////////////////////////


module z_servo_configuration#(
    /* config address */
    parameter z_servo_control_reg_address = 100,
    parameter z_servo_modes_reg_address = 101,
    parameter z_servo_select_rb_setpoint_modes_address = 110,
    parameter z_servo_select_rb_cpi_address = 111,
    parameter z_servo_select_rb_limits_address = 112
    )(
    (* X_INTERFACE_PARAMETER = "ASSOCIATED_CLKEN aclk, ASSOCIATED_BUSIF M_AXIS_setpoint:M_AXIS_zreset" *)
    input aclk,
    
    input [32-1:0]  config_addr,
    input [512-1:0] config_data,
    
    input [32-1:0] gvp_options,
    
    output wire [32-1:0] M_AXIS_setpoint_tdata,
    output wire          M_AXIS_setpoint_tvalid,
    output wire [32-1:0] cp,
    output wire [32-1:0] ci,
    output wire [32-1:0] upper,
    output wire [32-1:0] lower,

    output wire [32-1:0] signal_offset,
    output wire [32-1:0] control_signal_offset,
    output wire [32-1:0] M_AXIS_zreset_tdata,
    output wire          M_AXIS_zreset_tvalid,

    output wire [32-1:0] z_servo_rb_A,
    output wire [32-1:0] z_servo_rb_B,

    output wire servo_enable,
    output wire servo_log,
    output wire servo_fcz,
    output wire servo_hold,
    output wire [1:0] FIR_ctrl
    );
        

    reg [32-1:0] r_control_setpoint = 0;
    reg [32-1:0] r_cp = 0;
    reg [32-1:0] r_ci = 0;
    reg [32-1:0] r_upper = 0;
    reg [32-1:0] r_lower = 0;

    reg signed [32-1:0] r_signal_offset = 0;
    reg signed [32-1:0] r_control_signal_offset = 0;
    reg signed [32-1:0] r_z_setpoint = 0;
    reg [32-1:0] r_transfer_mode = 0;
    
    reg [32-1:0] r_gvp_options = 0;

    reg [32-1:0] z_servo_rb_A_reg = 0;
    reg [32-1:0] z_servo_rb_B_reg = 0;

    reg [1:0] r_fir_ctrl=0;


    assign z_servo_rb_A = z_servo_rb_A_reg;
    assign z_servo_rb_B = z_servo_rb_B_reg;

    assign M_AXIS_setpoint_tdata  = r_control_setpoint;
    assign M_AXIS_setpoint_tvalid = 1;
    assign cp = r_cp;
    assign ci = r_ci;
    assign upper = r_upper;
    assign lower = r_lower;

    assign signal_offset = r_signal_offset;
    assign control_signal_offset = r_control_signal_offset;
    assign M_AXIS_zreset_tdata  = r_z_setpoint;
    assign M_AXIS_zreset_tvalid = 1;

    assign servo_enable = r_transfer_mode[0:0];
    assign servo_log    = r_transfer_mode[1:1];
    assign servo_fcz    = r_transfer_mode[2:2];

    assign servo_hold   = r_transfer_mode[3:3] | r_gvp_options[0:0];

    assign FIR_ctrl = r_fir_ctrl;

    always @(posedge aclk)
    begin
        // module configuration
        case (config_addr)
        z_servo_control_reg_address:
        begin
            r_control_setpoint <= config_data[1*32-1 : 0*32];
            r_cp               <= config_data[2*32-1 : 1*32];
            r_ci               <= config_data[3*32-1 : 2*32];
            r_upper            <= config_data[4*32-1 : 3*32];
            r_lower            <= config_data[5*32-1 : 4*32];
        end   
          
        z_servo_modes_reg_address:
        begin
            r_signal_offset    <= config_data[1*32-1 : 0*32];
            r_control_signal_offset <= config_data[2*32-1 : 1*32]; // normally 0 
            r_z_setpoint       <= config_data[3*32-1 : 2*32];
            r_transfer_mode    <= config_data[4*32-1 : 3*32];
            r_fir_ctrl         <= config_data[4*32+2-1: 4*32];
        end     

        z_servo_select_rb_setpoint_modes_address: // read back config
        begin
            z_servo_rb_A_reg <= r_control_setpoint;
            z_servo_rb_B_reg <= r_transfer_mode;
        end     
        z_servo_select_rb_limits_address: // read back config
        begin
            z_servo_rb_A_reg <= r_upper;
            z_servo_rb_B_reg <= r_lower;
        end     
        z_servo_select_rb_cpi_address: // read back config
        begin
            z_servo_rb_A_reg <= r_cp;
            z_servo_rb_B_reg <= r_ci;
        end     

        endcase
        
        r_gvp_options <= gvp_options; // buffer
    end    
    
endmodule
