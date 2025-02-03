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
// Create Date: 12/27/2024 07:43:40 PM
// Design Name:    part of RPSPMC
// Module Name:    axis_selector2_1 (MUX)
// Project Name:   RPSPMC 4 GXSM
// Target Devices: Zynq z7020
// Tool Versions:  Vivado 2023.1
// Description:    MUX for multiple AXIS to AXIS
// 
// Dependencies: 
// 
// Revision:
// Revision 0.01 - File Created
// Additional Comments:
// 
//////////////////////////////////////////////////////////////////////////////////

module axis_selector2_1 #(
    parameter SAXIS_TDATA_WIDTH = 32,
    parameter MAXIS_TDATA_WIDTH = 32,
    parameter configuration_address = 2000
)(
    // (* X_INTERFACE_PARAMETER = "FREQ_HZ 125000000" *)
    (* X_INTERFACE_PARAMETER = "ASSOCIATED_CLKEN a_clk, ASSOCIATED_BUSIF S_AXIS_00:S_AXIS_01:M_AXIS_1" *)
    input a_clk,
    input [32-1:0]  config_addr,
    input [512-1:0] config_data,
    
    input wire [SAXIS_TDATA_WIDTH-1:0]  S_AXIS_00_tdata,
    input wire                          S_AXIS_00_tvalid,
    input wire [SAXIS_TDATA_WIDTH-1:0]  S_AXIS_01_tdata,
    input wire                          S_AXIS_01_tvalid,

    output wire [MAXIS_TDATA_WIDTH-1:0] M_AXIS_1_tdata,
    output wire                         M_AXIS_1_tvalid,

    output wire [32-1:0] mux_ch

    );
   
    
// use buffer           
reg [SAXIS_TDATA_WIDTH-1:0] ALL_AXIS_tdata[1:0];
reg ALL_AXIS_tvalid[1:0];

reg [32-1:0] mux_axis_select=32'h00ba3210; // # AXIS S to connect 00..15 to AXIS M 1..6:  M1=# M2=... M6=# 4 bits each: bits 0..23
reg [32-1:0] axis_test=0;
reg [32-1:0] axis_test_value=0;


    always @ (posedge a_clk)
    begin
        // module configuration
        if (config_addr == configuration_address) // 16 -> 6 MUX configuration and test modes
        begin
            mux_axis_select  <= config_data[1*32-1 : 0*32]; // options: Bit0: use gain control, Bit1: use gain programmed
            axis_test        <= config_data[2*32-2 : 1*32]; // programmed test mode
            axis_test_value  <= config_data[3*32-2 : 2*32]; // programmed test value
        end
    end

    always @ (posedge a_clk)
    begin
        ALL_AXIS_tdata[0]  <= S_AXIS_00_tdata;
        ALL_AXIS_tdata[1]  <= S_AXIS_01_tdata;

        ALL_AXIS_tvalid[0]  <= S_AXIS_00_tvalid;
        ALL_AXIS_tvalid[1]  <= S_AXIS_01_tvalid;
    end

assign  M_AXIS_1_tdata = axis_test == 1 ? axis_test_value : ALL_AXIS_tdata [mux_axis_select[1-1:0]];

assign  M_AXIS_1_tvalid = ALL_AXIS_tvalid [mux_axis_select[1-1:0]];

assign mux_ch = mux_axis_select;

endmodule
