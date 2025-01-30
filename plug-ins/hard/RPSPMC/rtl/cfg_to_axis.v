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
// Module Name: signal_combine
// Project Name:   RPSPMC 4 GXSM
// Target Devices: Zynq z7020
// Tool Versions:  Vivado 2023.1
// Description: get top most bits from config
// 
// Dependencies: 
// 
// Revision:
// Revision 0.01 - File Created
// Additional Comments:
// 
//////////////////////////////////////////////////////////////////////////////////


module cfg_to_axis #(
    parameter SRC_ADDR = 0,
    parameter SRC_BITS = 32,
    parameter CFG_WIDTH = 1024,
    parameter DST_WIDTH = 32,
    parameter MAXIS_TDATA_WIDTH = 32
)
(
    (* X_INTERFACE_PARAMETER = "ASSOCIATED_CLKEN a_clk, ASSOCIATED_BUSIF M_AXIS" *)
    input  a_clk,
    input [CFG_WIDTH-1:0] cfg,
    output [MAXIS_TDATA_WIDTH-1:0]      M_AXIS_tdata,
    output                              M_AXIS_tvalid,
    
    output wire [DST_WIDTH-1:0] data
);

    // buffer in register to relaxtiming requiremnets for distributed placing
    reg [DST_WIDTH-1:0] cfg_dst_reg;

    always @ (posedge a_clk)
    begin
        cfg_dst_reg <= cfg[SRC_ADDR*32+SRC_BITS-1:SRC_ADDR*32+SRC_BITS-DST_WIDTH];
    end

    assign M_AXIS_tdata = {
        {(MAXIS_TDATA_WIDTH-DST_WIDTH){cfg[SRC_ADDR*32+SRC_BITS-1]}}, cfg_dst_reg
        //{(MAXIS_TDATA_WIDTH-DST_WIDTH){cfg[SRC_ADDR*32+SRC_BITS-1]}}, cfg[SRC_ADDR*32+SRC_BITS-1:SRC_ADDR*32+SRC_BITS-DST_WIDTH]
    };
    assign M_AXIS_tvalid = 1;
    assign data = cfg_dst_reg;
    //assign data = cfg[SRC_ADDR*32+SRC_BITS-1:SRC_ADDR*32+SRC_BITS-DST_WIDTH];

endmodule
