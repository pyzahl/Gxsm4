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
// Create Date: 11/26/2017 08:20:47 PM
// Module Name:    AD5791 io connect for up to 6 PMODs
// Company: GXSM
// Engineer: Percy Zahl
// 
// Create Date: 12/31/2022 08:50:50 PM
// Design Name:    part of RPSPMC
// Module Name:    fill clock gaps
// Project Name:   RPSPMC 4 GXSM
// Target Devices: Zynq z7020
// Tool Versions:  Vivado 2023.1
// Description:    post CDC filling gaps/keep
// 
// Dependencies: 
// 
// Revision:
// Revision 0.01 - File Created
// Additional Comments:
// 
//////////////////////////////////////////////////////////////////////////////////


module fill_clock_gaps #(
    parameter DATA_WIDTH = 1024
)
(
    (* X_INTERFACE_PARAMETER = "ASSOCIATED_CLKEN a_clk, ASSOCIATED_BUSIF S_AXIS_in" *)
    input  a_clk,
    input [DATA_WIDTH-1:0]  S_AXIS_in_tdata,
    input                   S_AXIS_in_tvalid,
    output                  S_AXIS_in_tready,
    output [DATA_WIDTH-1:0] out_data
);

    assign S_AXIS_in_tready = 1;

    // buffer in register to relaxtiming requiremnets for distributed placing
   reg [DATA_WIDTH-1:0]	   oreg;

    always @ (posedge a_clk)
    begin
        if (S_AXIS_in_tvalid)
            oreg <= S_AXIS_in_tdata;
    end

    assign out_data = oreg;

endmodule
