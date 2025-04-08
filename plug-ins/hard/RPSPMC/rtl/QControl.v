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
// Module Name:    QControl
// Project Name:   RPSPMC 4 GXSM
// Target Devices: Zynq z7020
// Tool Versions:  Vivado 2023.1
// Description:    Q-Control
// 
// Dependencies: 
// 
// Revision:
// Revision 0.01 - File Created
// Additional Comments:
// 
//////////////////////////////////////////////////////////////////////////////////

//SineOut=(int)(((long long)cr1 * (long long)volumeSine)>>22); // Volume Q22


module QControl #(
    parameter qcontrol_address = 20030,
    parameter ADC_WIDTH        = 14,
    parameter SIGNAL_M_WIDTH   = 16,
    parameter AXIS_DATA_WIDTH  = 16,
    parameter AXIS_TDATA_WIDTH = 16,
    parameter VAXIS_DATA_WIDTH = 16,
    parameter VAXIS_DATA_Q     = 14,
    parameter QC_PHASE_LEN2    = 13
)
(
    input [32-1:0]  config_addr,
    input [512-1:0] config_data,
    (* X_INTERFACE_PARAMETER = "ASSOCIATED_CLKEN a_clk, ASSOCIATED_BUSIF S_AXIS_SIGNAL_M" *)
    input a_clk,
    input wire [SIGNAL_M_WIDTH-1:0]    S_AXIS_SIGNAL_M_tdata,
    input wire                         S_AXIS_SIGNAL_M_tvalid,

    (* X_INTERFACE_PARAMETER = "ASSOCIATED_CLKEN adc_clk, ASSOCIATED_BUSIF M_AXIS" *)
    input adc_clk,
    output [AXIS_TDATA_WIDTH-1:0]      M_AXIS_tdata,
    output                             M_AXIS_tvalid
);

    reg reg_QC_enable=0;

    reg signed [ADC_WIDTH-1:0] x=0;
    reg signed [VAXIS_DATA_Q-1:0] v=0;
    reg signed [VAXIS_DATA_Q+ADC_WIDTH-1:0] regy_vx=0;
    reg signed [VAXIS_DATA_Q+ADC_WIDTH-1:0] regy_vx_QC=0;
    reg signed [VAXIS_DATA_Q+ADC_WIDTH-1:0] reg_QC_signal=0;
    reg signed [16-1:0] reg_QC_gain=0;
    reg signed [SIGNAL_M_WIDTH-1:0] signal=0;
    reg [QC_PHASE_LEN2-1:0] reg_QC_delay=0;
    reg signed [SIGNAL_M_WIDTH-1:0] delayline [(1<<QC_PHASE_LEN2)-1 : 0];
    reg [QC_PHASE_LEN2-1:0] i=0;
    reg [QC_PHASE_LEN2-1:0] id=0;
    
    reg [1:0] rdecii = 0;

    always @ (posedge a_clk)
    begin
    
        // module configuration
        if (config_addr == qcontrol_address) // QC configuration
        begin
            reg_QC_enable <= config_data[0*32      : 0*32]; //QC_enable;
            reg_QC_gain   <= config_data[1*32+16-1 : 1*32]; //QC_gain;
            reg_QC_delay  <= config_data[2*32+16-1 : 2*32]; //QC_delay[QC_PHASE_LEN2-1:0];
        end
    
        rdecii <= rdecii+1;
        if (rdecii[1])
        begin
            // Q-Control Mixer
            signal   <= {S_AXIS_SIGNAL_M_tdata[SIGNAL_M_WIDTH-1 : 0]};
            delayline[i] <= signal;
            
            // qc_delay = 4096 - #delaysampels
            // Q-Control + PAC-PLL Volume Control Mixer
            reg_QC_signal <= reg_QC_enable ? reg_QC_gain * $signed(delayline[id]) : 0;
            // regy_vx_QC    <= regy_vx + reg_QC_signal; // Volume Q15 or Q(VAXIS_DATA_Q-1)
            id <= i + reg_QC_delay;
            i  <= i+1;
        end
    end


    assign M_AXIS_tdata  = {{(AXIS_DATA_WIDTH-ADC_WIDTH){reg_QC_signal[VAXIS_DATA_Q+ADC_WIDTH-1]}}, {reg_QC_signal[VAXIS_DATA_Q+ADC_WIDTH-1:VAXIS_DATA_Q-1]}};
    assign M_AXIS_tvalid = 1;
   
endmodule
