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
// Module Name:    axis_selector_16_6 (MUX)
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

module axis_selector16_6 #(
    parameter SAXIS_TDATA_WIDTH = 32,
    parameter MAXIS_TDATA_WIDTH = 32,
    parameter configuration_address = 2000
)(
    // (* X_INTERFACE_PARAMETER = "FREQ_HZ 125000000" *)
    (* X_INTERFACE_PARAMETER = "ASSOCIATED_CLKEN a_clk, ASSOCIATED_BUSIF S_AXIS_00:S_AXIS_01:S_AXIS_02:S_AXIS_03:S_AXIS_04:S_AXIS_05:S_AXIS_06:S_AXIS_07:S_AXIS_08:S_AXIS_09:S_AXIS_10:S_AXIS_11:S_AXIS_12:S_AXIS_13:S_AXIS_14:S_AXIS_15:M_AXIS_1:M_AXIS_2:M_AXIS_3:M_AXIS_4:M_AXIS_5:M_AXIS_6" *)
    input a_clk,
    input [32-1:0]  config_addr,
    input [512-1:0] config_data,
    
    input wire [SAXIS_TDATA_WIDTH-1:0]  S_AXIS_00_tdata,
    input wire                          S_AXIS_00_tvalid,
    input wire [SAXIS_TDATA_WIDTH-1:0]  S_AXIS_01_tdata,
    input wire                          S_AXIS_01_tvalid,
    input wire [SAXIS_TDATA_WIDTH-1:0]  S_AXIS_02_tdata,
    input wire                          S_AXIS_02_tvalid,
    input wire [SAXIS_TDATA_WIDTH-1:0]  S_AXIS_03_tdata,
    input wire                          S_AXIS_03_tvalid,
    input wire [SAXIS_TDATA_WIDTH-1:0]  S_AXIS_04_tdata,
    input wire                          S_AXIS_04_tvalid,
    input wire [SAXIS_TDATA_WIDTH-1:0]  S_AXIS_05_tdata,
    input wire                          S_AXIS_05_tvalid,
    input wire [SAXIS_TDATA_WIDTH-1:0]  S_AXIS_06_tdata,
    input wire                          S_AXIS_06_tvalid,
    input wire [SAXIS_TDATA_WIDTH-1:0]  S_AXIS_07_tdata,
    input wire                          S_AXIS_07_tvalid,
    input wire [SAXIS_TDATA_WIDTH-1:0]  S_AXIS_08_tdata,
    input wire                          S_AXIS_08_tvalid,
    input wire [SAXIS_TDATA_WIDTH-1:0]  S_AXIS_09_tdata,
    input wire                          S_AXIS_09_tvalid,
    input wire [SAXIS_TDATA_WIDTH-1:0]  S_AXIS_10_tdata,
    input wire                          S_AXIS_10_tvalid,
    input wire [SAXIS_TDATA_WIDTH-1:0]  S_AXIS_11_tdata,
    input wire                          S_AXIS_11_tvalid,
    input wire [SAXIS_TDATA_WIDTH-1:0]  S_AXIS_12_tdata,
    input wire                          S_AXIS_12_tvalid,
    input wire [SAXIS_TDATA_WIDTH-1:0]  S_AXIS_13_tdata,
    input wire                          S_AXIS_13_tvalid,
    input wire [SAXIS_TDATA_WIDTH-1:0]  S_AXIS_14_tdata,
    input wire                          S_AXIS_14_tvalid,
    input wire [SAXIS_TDATA_WIDTH-1:0]  S_AXIS_15_tdata,
    input wire                          S_AXIS_15_tvalid,

    output wire [MAXIS_TDATA_WIDTH-1:0] M_AXIS_1_tdata,
    output wire                         M_AXIS_1_tvalid,
    output wire [MAXIS_TDATA_WIDTH-1:0] M_AXIS_2_tdata,
    output wire                         M_AXIS_2_tvalid,
    output wire [MAXIS_TDATA_WIDTH-1:0] M_AXIS_3_tdata,
    output wire                         M_AXIS_3_tvalid,
    output wire [MAXIS_TDATA_WIDTH-1:0] M_AXIS_4_tdata,
    output wire                         M_AXIS_4_tvalid,
    output wire [MAXIS_TDATA_WIDTH-1:0] M_AXIS_5_tdata,
    output wire                         M_AXIS_5_tvalid,
    output wire [MAXIS_TDATA_WIDTH-1:0] M_AXIS_6_tdata,
    output wire                         M_AXIS_6_tvalid,

    output wire [32-1:0] mux_ch

    );
   
/*
// my SELECTOR MUX
// connects input N to output x stream

`define MY_SELECTOR_DATA(N, xdata) \
    assign xdata = N ==  0? S_AXIS_00_tdata \
                 : N ==  1? S_AXIS_01_tdata \
                 : N ==  2? S_AXIS_02_tdata \
                 : N ==  3? S_AXIS_03_tdata \
                 : N ==  4? S_AXIS_04_tdata \
                 : N ==  5? S_AXIS_05_tdata \
                 : N ==  6? S_AXIS_06_tdata \
                 : N ==  7? S_AXIS_07_tdata \
                 : N ==  8? S_AXIS_08_tdata \
                 : N ==  9? S_AXIS_09_tdata \
                 : N == 10? S_AXIS_10_tdata \
                 : N == 11? S_AXIS_11_tdata \
                 : N == 11? S_AXIS_12_tdata \
                 : N == 12? S_AXIS_13_tdata \
                 : N == 13? S_AXIS_13_tdata \
                 : N == 14? S_AXIS_14_tdata \
                 : N == 15? S_AXIS_15_tdata : 0
                 

`define MY_SELECTOR_VALID(N, xvalid) \
    assign xvalid = N ==  0? S_AXIS_00_tvalid \
                  : N ==  1? S_AXIS_01_tvalid \
                  : N ==  2? S_AXIS_02_tvalid \
                  : N ==  3? S_AXIS_03_tvalid \
                  : N ==  4? S_AXIS_04_tvalid \
                  : N ==  5? S_AXIS_05_tvalid \
                  : N ==  6? S_AXIS_06_tvalid \
                  : N ==  7? S_AXIS_07_tvalid \
                  : N ==  8? S_AXIS_08_tvalid \
                  : N ==  9? S_AXIS_09_tvalid \
                  : N == 10? S_AXIS_10_tvalid \
                  : N == 11? S_AXIS_11_tvalid \
                  : N == 11? S_AXIS_12_tvalid \
                  : N == 12? S_AXIS_13_tvalid \
                  : N == 13? S_AXIS_13_tvalid \
                  : N == 14? S_AXIS_14_tvalid \
                  : N == 15? S_AXIS_15_tvalid : 0

// odd behaving
`MY_SELECTOR_DATA (mux_axis_select[4-1:0], M_AXIS_1_tdata);
`MY_SELECTOR_DATA (mux_axis_select[8-1:4], M_AXIS_2_tdata);
`MY_SELECTOR_DATA (mux_axis_select[12-1:8], M_AXIS_3_tdata);
`MY_SELECTOR_DATA (mux_axis_select[16-1:12], M_AXIS_4_tdata);
`MY_SELECTOR_DATA (mux_axis_select[20-1:16], M_AXIS_5_tdata);
`MY_SELECTOR_DATA (mux_axis_select[24-1:20], M_AXIS_6_tdata);

`MY_SELECTOR_VALID (mux_axis_select[4-1:0], M_AXIS_1_tvalid);
`MY_SELECTOR_VALID (mux_axis_select[8-1:4], M_AXIS_2_tvalid);
`MY_SELECTOR_VALID (mux_axis_select[12-1:8], M_AXIS_3_tvalid);
`MY_SELECTOR_VALID (mux_axis_select[16-1:12], M_AXIS_4_tvalid);
`MY_SELECTOR_VALID (mux_axis_select[20-1:16], M_AXIS_5_tvalid);
`MY_SELECTOR_VALID (mux_axis_select[24-1:20], M_AXIS_6_tvalid);

*/
    
// use buffer           
reg [SAXIS_TDATA_WIDTH-1:0] ALL_AXIS_tdata[15:0];
reg ALL_AXIS_tvalid[15:0];

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
        ALL_AXIS_tdata[2]  <= S_AXIS_02_tdata;
        ALL_AXIS_tdata[3]  <= S_AXIS_03_tdata;
        ALL_AXIS_tdata[4]  <= S_AXIS_04_tdata;
        ALL_AXIS_tdata[5]  <= S_AXIS_05_tdata;
        ALL_AXIS_tdata[6]  <= S_AXIS_06_tdata;
        ALL_AXIS_tdata[7]  <= S_AXIS_07_tdata;
        ALL_AXIS_tdata[8]  <= S_AXIS_08_tdata;
        ALL_AXIS_tdata[9]  <= S_AXIS_09_tdata;
        ALL_AXIS_tdata[10] <= S_AXIS_10_tdata;
        ALL_AXIS_tdata[11] <= S_AXIS_11_tdata;
        ALL_AXIS_tdata[12] <= S_AXIS_12_tdata;
        ALL_AXIS_tdata[13] <= S_AXIS_13_tdata;
        ALL_AXIS_tdata[14] <= S_AXIS_14_tdata;
        ALL_AXIS_tdata[15] <= S_AXIS_15_tdata;

        ALL_AXIS_tvalid[0]  <= S_AXIS_00_tvalid;
        ALL_AXIS_tvalid[1]  <= S_AXIS_01_tvalid;
        ALL_AXIS_tvalid[2]  <= S_AXIS_02_tvalid;
        ALL_AXIS_tvalid[3]  <= S_AXIS_03_tvalid;
        ALL_AXIS_tvalid[4]  <= S_AXIS_04_tvalid;
        ALL_AXIS_tvalid[5]  <= S_AXIS_05_tvalid;
        ALL_AXIS_tvalid[6]  <= S_AXIS_06_tvalid;
        ALL_AXIS_tvalid[7]  <= S_AXIS_07_tvalid;
        ALL_AXIS_tvalid[8]  <= S_AXIS_08_tvalid;
        ALL_AXIS_tvalid[9]  <= S_AXIS_09_tvalid;
        ALL_AXIS_tvalid[10] <= S_AXIS_10_tvalid;
        ALL_AXIS_tvalid[11] <= S_AXIS_11_tvalid;
        ALL_AXIS_tvalid[12] <= S_AXIS_12_tvalid;
        ALL_AXIS_tvalid[13] <= S_AXIS_13_tvalid;
        ALL_AXIS_tvalid[14] <= S_AXIS_14_tvalid;
        ALL_AXIS_tvalid[15] <= S_AXIS_15_tvalid;
    end

assign  M_AXIS_1_tdata = axis_test == 1 ? axis_test_value : ALL_AXIS_tdata [mux_axis_select[4-1:0]];
assign  M_AXIS_2_tdata = axis_test == 2 ? axis_test_value : ALL_AXIS_tdata [mux_axis_select[8-1:4]];
assign  M_AXIS_3_tdata = axis_test == 3 ? axis_test_value : ALL_AXIS_tdata [mux_axis_select[12-1:8]];
assign  M_AXIS_4_tdata = axis_test == 4 ? axis_test_value : ALL_AXIS_tdata [mux_axis_select[16-1:12]];
assign  M_AXIS_5_tdata = axis_test == 5 ? axis_test_value : ALL_AXIS_tdata [mux_axis_select[20-1:16]];
assign  M_AXIS_6_tdata = axis_test == 6 ? axis_test_value : ALL_AXIS_tdata [mux_axis_select[24-1:20]];

assign  M_AXIS_1_tvalid = ALL_AXIS_tvalid [mux_axis_select[4-1:0]];
assign  M_AXIS_2_tvalid = ALL_AXIS_tvalid [mux_axis_select[8-1:4]];
assign  M_AXIS_3_tvalid = ALL_AXIS_tvalid [mux_axis_select[12-1:8]];
assign  M_AXIS_4_tvalid = ALL_AXIS_tvalid [mux_axis_select[16-1:12]];
assign  M_AXIS_5_tvalid = ALL_AXIS_tvalid [mux_axis_select[20-1:16]];
assign  M_AXIS_6_tvalid = ALL_AXIS_tvalid [mux_axis_select[24-1:20]];

assign mux_ch = mux_axis_select;

endmodule
