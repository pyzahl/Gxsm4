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
// Create Date: 11/01/2017 07:29:30 PM
// Design Name:    part of RPSPMC
// Module Name:    split_signal
// Project Name:   RPSPMC 4 GXSM
// Target Devices: Zynq z7020
// Tool Versions:  Vivado 2023.1
// Description:    Split axis signal into two
// 
// Dependencies: 
// 
// Revision:
// Revision 0.01 - File Created
// Additional Comments:
// 
//////////////////////////////////////////////////////////////////////////////////


module split_signal #
(
    parameter ADC_DATA_WIDTH = 14,
    parameter S_AXIS_TDATA_WIDTH = 32,
    parameter M_AXIS_TDATA_WIDTH = 16
)
(
    (* X_INTERFACE_PARAMETER = "FREQ_HZ 125000000" *)
    input [S_AXIS_TDATA_WIDTH-1:0]        S_AXIS_tdata,
    input                                 S_AXIS_tvalid,
    (* X_INTERFACE_PARAMETER = "FREQ_HZ 125000000" *)
    output wire [M_AXIS_TDATA_WIDTH-1:0]  M_AXIS_PORT1_tdata,
    output wire                           M_AXIS_PORT1_tvalid,
    (* X_INTERFACE_PARAMETER = "FREQ_HZ 125000000" *)
    output wire [M_AXIS_TDATA_WIDTH-1:0]  M_AXIS_PORT2_tdata,
    output wire                           M_AXIS_PORT2_tvalid,
    (* X_INTERFACE_PARAMETER = "FREQ_HZ 125000000" *)
    output wire [S_AXIS_TDATA_WIDTH-1:0]  M_AXIS_PASS_tdata, // (Sine, Cosine) vector pass through
    output wire                           M_AXIS_PASS_tvalid
    );
        
    assign M_AXIS_PORT1_tdata = {{(M_AXIS_TDATA_WIDTH-ADC_DATA_WIDTH+1){S_AXIS_tdata[ADC_DATA_WIDTH-1]}},S_AXIS_tdata[ADC_DATA_WIDTH-1:0]};
    assign M_AXIS_PORT2_tdata = {{(M_AXIS_TDATA_WIDTH-ADC_DATA_WIDTH+1){S_AXIS_tdata[S_AXIS_TDATA_WIDTH-1]}},S_AXIS_tdata[S_AXIS_TDATA_WIDTH-1:ADC_DATA_WIDTH]};
    assign M_AXIS_PORT1_tvalid = S_AXIS_tvalid;
    assign M_AXIS_PORT2_tvalid = S_AXIS_tvalid;

    assign M_AXIS_PASS_tdata  = S_AXIS_tdata; // pass
    assign M_AXIS_PASS_tvalid = S_AXIS_tvalid; // pass

endmodule

