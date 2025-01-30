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
// Create Date: 12/22/2024 10:05:33 PM
// Design Name:    part of RPSPMC
// Module Name:    axis_sadd
// Project Name:   RPSPMC 4 GXSM
// Target Devices: Zynq z7020
// Tool Versions:  Vivado 2023.1
// Description:    Saturated AXIS signal ADD
// 
// Dependencies: 
// 
// Revision:
// Revision 0.01 - File Created
// Additional Comments:
// 
//////////////////////////////////////////////////////////////////////////////////


module axis_sadd #(
    parameter SAXIS_TDATA_WIDTH = 32,
    parameter MAXIS_TDATA_WIDTH = 32,
    parameter POS_SATURATION_LIMIT =  2147483646,
    parameter POS_SATURATION_VALUE =  2147483646,
    parameter NEG_SATURATION_LIMIT = -2147483646,
    parameter NEG_SATURATION_VALUE = -2147483646
    )
    (
    // (* X_INTERFACE_PARAMETER = "FREQ_HZ 125000000" *)
    (* X_INTERFACE_PARAMETER = "ASSOCIATED_CLKEN a_clk" *)
    (* X_INTERFACE_PARAMETER = "ASSOCIATED_BUSIF S_AXIS_A:S_AXIS_B:M_AXIS_SUM" *)
    input a_clk,
    input wire [SAXIS_TDATA_WIDTH-1:0]  S_AXIS_A_tdata,
    input wire                          S_AXIS_A_tvalid,
    input wire [SAXIS_TDATA_WIDTH-1:0]  S_AXIS_B_tdata,
    input wire                          S_AXIS_B_tvalid,
    output wire [MAXIS_TDATA_WIDTH-1:0] M_AXIS_SUM_tdata,
    output wire                         M_AXIS_SUM_tvalid
    );
    

// Saturated result to 32bit
`define SATURATE(REG) (REG  > POS_SATURATION_LIMIT ? POS_SATURATION_VALUE : REG < NEG_SATURATION_LIMIT ? NEG_SATURATION_VALUE : REG[MAXIS_TDATA_WIDTH-1:0]) 

// Saturated result to 32bit
//`define SATURATE_32(REG) (REG > 33'sd2147483647 ? 32'sd2147483647 : REG < -33'sd2147483647 ? -32'sd2147483647 : REG[32-1:0]) 

    reg signed [SAXIS_TDATA_WIDTH+1-1:0] a=0;
    reg signed [SAXIS_TDATA_WIDTH+1-1:0] b=0;
    reg signed [SAXIS_TDATA_WIDTH+2-1:0] sum=0;

    always @ (posedge a_clk)
    begin
        a <= $signed(S_AXIS_A_tdata);
        b <= $signed(S_AXIS_B_tdata);
        sum <= a + b;
    end
    
    assign M_AXIS_SUM_tdata  = `SATURATE(sum);  // sum > POS_SATURATION_LIMIT ? POS_SATURATION_VALUE : sum < NEG_SATURATION_LIMIT ? NEG_SATURATION_VALUE : sum[MAXIS_TDATA_WIDTH-1:0]; // Saturate
    assign M_AXIS_SUM_tvalid = S_AXIS_A_tvalid && S_AXIS_B_tvalid;
    
endmodule  