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
// Module Name:    axis_spm_control
// Engineer: 
// 
// Create Date: 08/16/2020 12:14:13 PM
// Design Name:    part of RPSPMC
// Module Name:    axis_splitter
// Project Name:   RPSPMC 4 GXSM
// Target Devices: Zynq z7020
// Tool Versions:  Vivado 2023.1// Company: 
// Description:    Branch AXIS signal into two
// 
// Dependencies: 
// 
// Revision:
// Revision 0.01 - File Created
// Additional Comments:
// 
//////////////////////////////////////////////////////////////////////////////////


module axis_splitter #(
    parameter SAXIS_TDATA_WIDTH = 32,
    parameter MAXIS_TDATA_WIDTH = 32
)
(
    // (* X_INTERFACE_PARAMETER = "FREQ_HZ 125000000" *)
    (* X_INTERFACE_PARAMETER = "ASSOCIATED_CLKEN a_clk, ASSOCIATED_BUSIF S_AXIS:M_AXIS:M_AXIS2" *)
    input a_clk,
    
    input wire [SAXIS_TDATA_WIDTH-1:0]  S_AXIS_tdata,
    input wire                          S_AXIS_tvalid,
    output wire [MAXIS_TDATA_WIDTH-1:0] M_AXIS_tdata,
    output wire                         M_AXIS_tvalid,
    output wire [MAXIS_TDATA_WIDTH-1:0] M_AXIS2_tdata,
    output wire                         M_AXIS2_tvalid,
    output wire [MAXIS_TDATA_WIDTH-1:0] monitor
    );
    
    /*
    reg signed [SAXIS_TDATA_WIDTH-1:0] data_in;
    reg signed [MAXIS_TDATA_WIDTH-1:0] buffer;
     
    always @ (posedge a_clk)
    begin
        if (S_AXIS_tvalid)
        begin
            data_in   <= $signed(S_AXIS_tdata);
            if (SAXIS_TDATA_WIDTH < MAXIS_TDATA_WIDTH)
                buffer    <= data_in <<< (MAXIS_TDATA_WIDTH - SAXIS_TDATA_WIDTH);
            else
                buffer    <= data_in;
        end
        else
        begin
            buffer  <= 0;
        end
    end
    
    assign monitor = buffer;
    assign M_AXIS_tdata  = buffer;
    assign M_AXIS_tvalid = S_AXIS_tvalid;
    assign M_AXIS2_tdata  = buffer;
    assign M_AXIS2_tvalid = S_AXIS_tvalid;
    */
    
    // ROUNDING down - -consider for DAC
    // Consider, if you have a data values i_data coming in with IWID (input width) bits, and you want to create a data value with OWID bits, why not just grab the top OWID bits?
    // assign	w_tozero = i_data[(IWID-1):0] + { {(OWID){1'b0}}, i_data[(IWID-1)], {(IWID-OWID-1){!i_data[(IWID-1)]}}};
    // assign monitor = S_AXIS_tdata[SAXIS_TDATA_WIDTH-1:0] + { {(MAXIS_TDATA_WIDTH){1'b0}}, S_AXIS_tdata[SAXIS_TDATA_WIDTH-1], {(SAXIS_TDATA_WIDTH-MAXIS_TDATA_WIDTH-1){!S_AXIS_tdata[SAXIS_TDATA_WIDTH-1]}}};
    
    // EXPAND / COPY:
    assign monitor        = {{S_AXIS_tdata[SAXIS_TDATA_WIDTH-1:0]}, {(MAXIS_TDATA_WIDTH-SAXIS_TDATA_WIDTH){1'b0}}};
    assign M_AXIS_tdata   = {{S_AXIS_tdata[SAXIS_TDATA_WIDTH-1:0]}, {(MAXIS_TDATA_WIDTH-SAXIS_TDATA_WIDTH){1'b0}}};
    assign M_AXIS_tvalid  = S_AXIS_tvalid;
    assign M_AXIS2_tdata  = {{S_AXIS_tdata[SAXIS_TDATA_WIDTH-1:0]}, {(MAXIS_TDATA_WIDTH-SAXIS_TDATA_WIDTH){1'b0}}};
    assign M_AXIS2_tvalid = S_AXIS_tvalid;
    
    
    
endmodule
