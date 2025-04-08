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
// Create Date: 11/26/2017 08:20:47 PM
// Design Name:    part of RPSPMC
// Module Name:    axis ctrlsrc_select
// Project Name:   RPSPMC 4 GXSM
// Target Devices: Zynq z7020
// Tool Versions:  Vivado 2023.1
// Description:    select control axis with offset correct
// 
// Dependencies: 
// 
// Revision:
// Revision 0.01 - File Created
// Additional Comments:
// 
//////////////////////////////////////////////////////////////////////////////////

// 32 -> SQ8.24 (with switch to select log path: ln(1+abs(x+offset) vs. linear (x+offset))
module axis_ctrlsrc_select #(
    parameter SAXIS_DATA_WIDTH = 32,
    parameter MAXIS_DATA_WIDTH = 32,
    parameter ADD_OFFSET = 1
)
(
    (* X_INTERFACE_PARAMETER = "ASSOCIATED_CLKEN a_clk, ASSOCIATED_BUSIF S_AXIS:S_AXIS_LN:M_AXIS_ABS:M_AXIS_MON:M_AXIS" *)
    input a_clk,
    input wire [SAXIS_DATA_WIDTH-1:0]  S_AXIS_tdata,
    input wire                         S_AXIS_tvalid,
    input wire [SAXIS_DATA_WIDTH-1:0]  signal_offset,

    input wire [32-1:0]  S_AXIS_LN_tdata,
    input wire           S_AXIS_LN_tvalid,

    input wire [1:0]  selection_ln,

    output [32-1:0]      M_AXIS_ABS_tdata,
    output               M_AXIS_ABS_tvalid,

    output [MAXIS_DATA_WIDTH-1:0]      M_AXIS_tdata,
    output                             M_AXIS_tvalid,

    output [MAXIS_DATA_WIDTH-1:0]      M_AXIS_MON_tdata,
    output                             M_AXIS_MON_tvalid
);

    reg signed [SAXIS_DATA_WIDTH-1:0] x;
    reg signed [SAXIS_DATA_WIDTH-1:0] y;

    always @ (posedge a_clk)
    begin
        if (ADD_OFFSET)
            x <= ($signed(S_AXIS_tdata) >>> 8 ) + ($signed(signal_offset) >>> 8); // REMOVE SIGNAL OFFSET
        else     
            x <= $signed(S_AXIS_tdata) >>> 8;
        y <= x[SAXIS_DATA_WIDTH-1] ? -x : x; // ABS
    end


    // Consider, if you have a data values i_data coming in with IWID (input width) bits, and you want to create a data value with OWID bits, why not just grab the top OWID bits?
    // assign	w_tozero = i_data[(IWID-1):0] + { {(OWID){1'b0}}, i_data[(IWID-1)], {(IWID-OWID-1){!i_data[(IWID-1)]}}};

    assign M_AXIS_MON_tdata = x; // SQ8.24
    assign M_AXIS_MON_tvalid = S_AXIS_tvalid;
    assign M_AXIS_tdata = selection_ln ? S_AXIS_LN_tdata : x;
    assign M_AXIS_tvalid = S_AXIS_tvalid;
    assign M_AXIS_ABS_tdata = y + 1; //ln_offset; // fixed extra offset from zero
    assign M_AXIS_ABS_tvalid = S_AXIS_tvalid;

endmodule
