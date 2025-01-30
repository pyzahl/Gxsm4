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
// Create Date: 01/03/2025 03:33:49 PM
// Design Name:    part of RPSPMC
// Module Name:    axis_iir_filter
// Project Name:   RPSPMC 4 GXSM
// Target Devices: Zynq z7020
// Tool Versions:  Vivado 2023.1
// Description:    IIR filter
// 
// Dependencies: 
// 
// Revision:
// Revision 0.01 - File Created
// Additional Comments:
// 
//////////////////////////////////////////////////////////////////////////////////


module axis_iir_filter #
(
    parameter S_AXIS_DATA_WIDTH = 32,
    parameter M_AXIS_DATA_WIDTH = 32,
    parameter IIR_TAU_Q = 31
)
(
    (* X_INTERFACE_PARAMETER = "ASSOCIATED_CLKEN aclk, ASSOCIATED_BUSIF S_AXIS:M_AXIS_IIR" *)
    input aclk,
    input wire [S_AXIS_DATA_WIDTH-1:0]  S_AXIS_tdata,
    input wire                          S_AXIS_tvalid,

    input signed [31:0] iir_tau, // Q31 tau DC iir at cos-sin zero x

    output wire [M_AXIS_DATA_WIDTH-1:0] M_AXIS_IIR_tdata,
    output wire                         M_AXIS_IIR_tvalid
);

    wire signed [32-1:0] rt;  // Q31 time const tau for iir  (tau, fraction new)
    wire signed [32-1:0] rtH; // Q31 time const tau for iir (1-tau)


    reg signed [M_AXIS_DATA_WIDTH-1:0] m=0;
    reg signed [2*M_AXIS_DATA_WIDTH-1:0] iir_hist=0;

    reg [1:0] rdecii = 0;

    localparam integer ONEQ31 = ((1<<31)-1);

    assign rt  = iir_tau; // Q31 tau iir
    assign rtH = ONEQ31-iir_tau; // Q31 tau iir


    always @ (posedge aclk)
    begin
        rdecii <= rdecii+1; // rdecii 00 01 *10 11 00 ...
        if (rdecii[1]==1)
        begin
            m   <= $signed(S_AXIS_tdata);
            iir_hist <= rt * m  +  rtH * iir_hist; 
        end
    end
    
    assign M_AXIS_IIR_tdata = iir_hist >>> IIR_TAU_Q;
    assign M_AXIS_IIR_tvalid = 1;

endmodule
