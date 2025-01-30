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
// Create Date: 08/16/2020 12:14:13 PM
// Design Name:    part of RPSPMC
// Module Name: axis_FIR
// Project Name:   RPSPMC 4 GXSM
// Target Devices: Zynq z7020
// Tool Versions:  Vivado 2023.1
// Description:    FIR filter
// 
// Dependencies: 
// 
// Revision:
// Revision 0.01 - File Created
// Additional Comments:
// 
//////////////////////////////////////////////////////////////////////////////////


module axis_FIR #(
    parameter SAXIS_TDATA_WIDTH = 32,
    parameter MAXIS_TDATA_WIDTH = 32,
    parameter FIR_DECI   = 64, // FIR LEN
    parameter FIR_DECI_L = 6 // extra bits to accomodate FIR_DECI number 
)
(
    (* X_INTERFACE_PARAMETER = "ASSOCIATED_CLKEN a_clk, ASSOCIATED_BUSIF S_AXIS:M_AXIS:M_AXIS2" *)
    input a_clk,
    input next_data,
    
    input wire [SAXIS_TDATA_WIDTH-1:0]  S_AXIS_tdata,
    input wire                          S_AXIS_tvalid,
    output wire [MAXIS_TDATA_WIDTH-1:0] M_AXIS_tdata,
    output wire                         M_AXIS_tvalid,
    output wire [MAXIS_TDATA_WIDTH-1:0] M_AXIS2_tdata,
    output wire                         M_AXIS2_tvalid
    );
    
    reg [FIR_DECI_L-1:0] i = 0;
    reg signed [SAXIS_TDATA_WIDTH+FIR_DECI_L-1:0] sum = 0;
    reg signed [SAXIS_TDATA_WIDTH-1:0] fir_buffer[FIR_DECI-1:0];
    reg signed [SAXIS_TDATA_WIDTH-1:0] data_in;
    reg signed [MAXIS_TDATA_WIDTH-1:0] buffer; 

    reg run=1;
    
    integer k;
    initial begin
        for (k=0; k<FIR_DECI; k=k+1)
            fir_buffer[k] = 0;
    end
    
    //always @ (posedge next_data)
    
    always @ (posedge a_clk)
    begin
        if (next_data) // run at decimated rate as given by axis_decii_clk
        begin
            if (S_AXIS_tvalid)
                data_in <= $signed(S_AXIS_tdata);
            else
                data_in <= 0;
            run <= 1;
        end
        else
            run <= 0;

        if (run)
        begin
            //data_in       <= $signed(S_AXIS_tdata);
            sum           <= sum - fir_buffer[i] + data_in;
            fir_buffer[i] <= data_in;
            i <= i+1;
        end
        buffer <= sum[SAXIS_TDATA_WIDTH+FIR_DECI_L-1:FIR_DECI_L+SAXIS_TDATA_WIDTH-MAXIS_TDATA_WIDTH];
    end
    
    assign M_AXIS_tdata  = buffer;
    assign M_AXIS_tvalid = S_AXIS_tvalid;
    assign M_AXIS2_tdata  = buffer;
    assign M_AXIS2_tvalid = S_AXIS_tvalid;
    
endmodule
