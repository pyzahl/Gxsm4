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
// Module Name:    axis sc28 to 14
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


module axis_sc28_to_14 #(
    parameter SRC_BITS = 26,
    parameter AUX_BITS = 16,
    parameter SAXIS_DATA_WIDTH = 32,
    parameter SAXIS_TDATA_WIDTH = 64, // two channels s,c
    parameter SAXIS_AUX_TDATA_WIDTH = 16,
    parameter SAXIS_AUX_DATA_WIDTH = 16,
    parameter ADC_WIDTH = 14,
    parameter MAXIS_DATA_WIDTH = 16,
    parameter MAXIS_TDATA_WIDTH = 32
)
(
    (* X_INTERFACE_PARAMETER = "ASSOCIATED_CLKEN a_clk, ASSOCIATED_BUSIF S_AXIS:S_AXIS_aux:M_AXIS" *)
    input a_clk,
    input wire [SAXIS_TDATA_WIDTH-1:0]  S_AXIS_tdata,
    input wire                          S_AXIS_tvalid,

    input wire [SAXIS_AUX_TDATA_WIDTH-1:0]  S_AXIS_aux_tdata,
    input wire                              S_AXIS_aux_tvalid, // if valid is set, this data is choosen for channel 2

    output [MAXIS_TDATA_WIDTH-1:0]      M_AXIS_tdata,
    output                              M_AXIS_tvalid
);


    // Consider, if you have a data values i_data coming in with IWID (input width) bits, and you want to create a data value with OWID bits, why not just grab the top OWID bits?
    // assign	w_tozero = i_data[(IWID-1):0] + { {(OWID){1'b0}}, i_data[(IWID-1)], {(IWID-OWID-1){!i_data[(IWID-1)]}}};


    assign M_AXIS_tdata = {
        {(MAXIS_DATA_WIDTH-ADC_WIDTH){S_AXIS_tdata[SRC_BITS-1]}}, 
                                      S_AXIS_tdata[SRC_BITS-1:SRC_BITS-ADC_WIDTH],
        S_AXIS_aux_tvalid ?
        {{(MAXIS_DATA_WIDTH-ADC_WIDTH){S_AXIS_aux_tdata[AUX_BITS-1]}}, 
                                       S_AXIS_aux_tdata[AUX_BITS-1:AUX_BITS-ADC_WIDTH]}
        :
        {{(MAXIS_DATA_WIDTH-ADC_WIDTH){S_AXIS_tdata[SAXIS_TDATA_WIDTH-SAXIS_DATA_WIDTH+SRC_BITS-1]}}, 
                                       S_AXIS_tdata[SAXIS_TDATA_WIDTH-SAXIS_DATA_WIDTH+SRC_BITS-1:SAXIS_TDATA_WIDTH-SAXIS_DATA_WIDTH+SRC_BITS-ADC_WIDTH]}
    };
    assign M_AXIS_tvalid = S_AXIS_tvalid;
/*    
    assign M_AXIS_pass_tdata = {
        {(MAXIS_DATA_WIDTH-ADC_WIDTH){S_AXIS_tdata[SRC_BITS-1]}}, 
                                      S_AXIS_tdata[SRC_BITS-1:SRC_BITS-ADC_WIDTH],
        S_AXIS_aux_tvalid ?
        {{(MAXIS_DATA_WIDTH-ADC_WIDTH){S_AXIS_aux_tdata[AUX_BITS-1]}}, 
                                       S_AXIS_aux_tdata[AUX_BITS-1:AUX_BITS-ADC_WIDTH]}
        :
        {{(MAXIS_DATA_WIDTH-ADC_WIDTH){S_AXIS_tdata[SAXIS_TDATA_WIDTH-SAXIS_DATA_WIDTH+SRC_BITS-1]}}, 
                                       S_AXIS_tdata[SAXIS_TDATA_WIDTH-SAXIS_DATA_WIDTH+SRC_BITS-1:SAXIS_TDATA_WIDTH-SAXIS_DATA_WIDTH+SRC_BITS-ADC_WIDTH]}
    };
    assign M_AXIS_pass_tvalid = S_AXIS_tvalid;
*/
endmodule
