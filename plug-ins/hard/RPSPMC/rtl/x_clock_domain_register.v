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
// Create Date: 12/26/2024 08:20:47 PM
// Design Name:    part of RPSPMC
// Module Name:    x clock domain test -- obsoleted by AXI-FIFO w independent clocks
// Project Name:   RPSPMC 4 GXSM
// Target Devices: Zynq z7020
// Tool Versions:  Vivado 2023.1
// Description:    CDC test
// 
// Dependencies: 
// 
// Revision:
// Revision 0.01 - File Created
// Additional Comments:
// 
//////////////////////////////////////////////////////////////////////////////////


module x_clock_domain_register #(
    parameter DATA_WIDTH = 1024
)
(
    (* X_INTERFACE_PARAMETER = "ASSOCIATED_CLKEN a_clk" *)
    input  in_clk,
    input  out_clk,
    input [DATA_WIDTH-1:0]  in_data,
    output [DATA_WIDTH-1:0] out_data
);

   reg [DATA_WIDTH-1:0]	   in_reg[2:0]; // pipeline for integrity check
   reg [DATA_WIDTH-1:0]	   oreg;

    always @ (posedge in_clk)
    begin
       in_reg[0] <= in_data;
       in_reg[1] <= in_reg[0];
       in_reg[2] <= in_reg[1];
    end

    always @ (posedge out_clk)
    begin
	 if (in_reg[1] == in_reg[2])
	       oreg <= in_reg[2];
    end

    assign out_data = oreg;

endmodule
