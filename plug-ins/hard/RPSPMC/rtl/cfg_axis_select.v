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
// Module Name:    cfg axis select
// Project Name:   RPSPMC 4 GXSM
// Target Devices: Zynq z7020
// Tool Versions:  Vivado 2023.1
// Description:    select cfg bits and assign to axis
// 
// Dependencies: 
// 
// Revision:
// Revision 0.01 - File Created
// Additional Comments:
// 
//////////////////////////////////////////////////////////////////////////////////


module cfg_axis_select #(
    parameter SRC_ADDR = 0,
    parameter CFG_WIDTH = 1024,
    parameter CFG_SWBIT = 0,
    parameter AXIS_TDATA_WIDTH = 32
)
(
    input [CFG_WIDTH-1:0] cfg,
    input a_clk,
    (* X_INTERFACE_PARAMETER = "FREQ_HZ 125000000" *)
    input [AXIS_TDATA_WIDTH-1:0]      S1_AXIS_tdata,
    input                             S1_AXIS_tvalid,
    (* X_INTERFACE_PARAMETER = "FREQ_HZ 125000000" *)
    input [AXIS_TDATA_WIDTH-1:0]      S2_AXIS_tdata,
    input                             S2_AXIS_tvalid,
    (* X_INTERFACE_PARAMETER = "FREQ_HZ 125000000" *)
    output [AXIS_TDATA_WIDTH-1:0]      M_AXIS_tdata,
    output                             M_AXIS_tvalid,
    
    output wire status
);

    assign M_AXIS_tdata  = (cfg[SRC_ADDR*32+CFG_SWBIT]) ? S2_AXIS_tdata  : S1_AXIS_tdata;
    assign M_AXIS_tvalid = (cfg[SRC_ADDR*32+CFG_SWBIT]) ? S2_AXIS_tvalid : S1_AXIS_tvalid;
    assign status = cfg[SRC_ADDR*32+CFG_SWBIT];

endmodule
