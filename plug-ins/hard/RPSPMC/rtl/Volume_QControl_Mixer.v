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
// Create Date: 11/26/2017 12:54:20 AM
// Design Name:    part of RPSPMC
// Module Name:    Volume_QControl_Mixer
// Project Name:   RPSPMC 4 GXSM
// Target Devices: Zynq z7020
// Tool Versions:  Vivado 2023.1
// Description:    Q-Control Mixer
// 
// Dependencies: 
// 
// Revision:
// Revision 0.01 - File Created
// Additional Comments:
// 
//////////////////////////////////////////////////////////////////////////////////

//SineOut=(int)(((long long)cr1 * (long long)volumeSine)>>22); // Volume Q22


module Volume_QControl_Mixer #(
    parameter ADC_WIDTH        = 14,
    parameter SIGNAL_QS_WIDTH  = 16,
    parameter AXIS_DATA_WIDTH  = 16,
    parameter AXIS_TDATA_WIDTH = 32,
    parameter VAXIS_DATA_WIDTH = 16,
    parameter VAXIS_DATA_Q     = 14
)
(
    //(* X_INTERFACE_PARAMETER = "FREQ_HZ 62500000" *)
    (* X_INTERFACE_PARAMETER = "ASSOCIATED_CLKEN a_clk, ASSOCIATED_BUSIF S_AXIS_VS:S_AXIS_QS:S_AXIS_SIGNAL_M" *)
    input a_clk,
    input wire [AXIS_TDATA_WIDTH-1:0]  S_AXIS_VS_tdata,
    input wire                         S_AXIS_VS_tvalid,
    
    input wire [SIGNAL_QS_WIDTH-1:0]   S_AXIS_QS_tdata,
    input wire                         S_AXIS_QS_tvalid,

    (* X_INTERFACE_PARAMETER = "ASSOCIATED_CLKEN adc_clk, ASSOCIATED_BUSIF M_AXIS" *)
     input adc_clk,
    //(* X_INTERFACE_PARAMETER = "FREQ_HZ 125000000" *)
    output [AXIS_TDATA_WIDTH-1:0]      M_AXIS_tdata,
    output                             M_AXIS_tvalid
);

    reg signed [VAXIS_DATA_WIDTH-1:0] reg_QC_signal=0;
    
    always @ (posedge a_clk)
    begin
        reg_QC_signal <= $signed(S_AXIS_QS_tdata[SIGNAL_QS_WIDTH-1:0]) + $signed({S_AXIS_VS_tdata[AXIS_TDATA_WIDTH-1 : AXIS_TDATA_WIDTH-AXIS_DATA_WIDTH]}); // add QControl signal and Exec Volume Signal
    end

    assign M_AXIS_tdata = {{(AXIS_DATA_WIDTH-ADC_WIDTH){reg_QC_signal[VAXIS_DATA_WIDTH-1]}}, {reg_QC_signal[VAXIS_DATA_Q-1:0]}, S_AXIS_VS_tdata[AXIS_DATA_WIDTH-1 : 0]};
    assign M_AXIS_tvalid = 1;
   
endmodule
