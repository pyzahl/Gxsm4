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
// Create Date: 11/26/2017 12:54:20 AM
// Design Name:    part of RPSPMC
// Module Name:    ScaleAndAdjust
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


module ScaleAndAdjust #(
    parameter S_AXIS_DATA_WIDTH = 32,
    parameter M_AXIS_DATA_WIDTH = 16,
    parameter GAIN_DATA_WIDTH   = 32,
    parameter GAIN_DATA_Q       = 16,
    parameter ENABLE_ADC_OUT    = 1
)
(
   (* X_INTERFACE_PARAMETER = "ASSOCIATED_CLKEN a_clk" *)
   (* X_INTERFACE_PARAMETER = "ASSOCIATED_BUSIF S_AXIS:M_AXIS" *)
   input a_clk,
   input wire [S_AXIS_DATA_WIDTH-1:0]  S_AXIS_tdata,
   input wire                          S_AXIS_tvalid,
   input wire [GAIN_DATA_WIDTH-1:0]    gain,
   //input wire [7:0]                     pre_shr,
   output [M_AXIS_DATA_WIDTH-1:0]      M_AXIS_tdata,
   output                              M_AXIS_tvalid
);

    reg signed [S_AXIS_DATA_WIDTH-1:0] x1=0;
    reg signed [S_AXIS_DATA_WIDTH-1:0] x=0;
    reg signed [GAIN_DATA_WIDTH-1:0] v1=0;
    reg signed [GAIN_DATA_WIDTH-1:0] v=0;
    //reg signed [M_AXIS_DATA_WIDTH-1:0] w=0;
    reg signed [S_AXIS_DATA_WIDTH+GAIN_DATA_WIDTH-1:0] y=0;
    
    reg [1:0] rdecii = 0;

    always @ (posedge a_clk)
    begin
        rdecii <= rdecii+1;
    end

    always @ (posedge rdecii[1])
    begin
       if (ENABLE_ADC_OUT)
       begin
           if (S_AXIS_tvalid)
           begin
               x <= x1;
               v <= v1;
               x1 <= S_AXIS_tdata;
               v1 <= gain;
               y <= v*x; // Volume Q15 or Q(VAXIS_DATA_Q-1)
           end
        end
    end
    assign M_AXIS_tdata  = {y[GAIN_DATA_WIDTH+S_AXIS_DATA_WIDTH-1], y[M_AXIS_DATA_WIDTH+GAIN_DATA_Q-1 : GAIN_DATA_Q]}; //-GAIN_DATA_Q-
    assign M_AXIS_tvalid = S_AXIS_tvalid;
   
endmodule
