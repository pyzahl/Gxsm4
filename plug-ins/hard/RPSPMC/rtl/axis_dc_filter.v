`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
/* Gxsm - Gnome X Scanning Microscopy
 * universal STM/AFM/SARLS/SPALEED/... controlling and
 * data analysis software
 * 
 * Copyright (C) 1999,2000,2001,2002,2003 Percy Zahl
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
// Create Date: 11/26/2017 09:10:43 PM
// Design Name: 
// Module Name: axis_decimaor
// Project Name: 
// Target Devices: 
// Tool Versions: 
// Description: 
// 
// Dependencies: 
// 
// Revision:
// Revision 0.01 - File Created
// Additional Comments:
// 
//////////////////////////////////////////////////////////////////////////////////


module axis_dc_filter #
(
    parameter S_AXIS_DATA_WIDTH = 16,
    parameter S_AXIS_SIGNAL_SIGNIFICANT_DATA_WIDTH = 16, // 14, now decimated 4x to 16 bits
    parameter M_AXIS_DATA_WIDTH = 32,
    parameter LMS_DATA_WIDTH = 26,
    parameter LMS_Q_WIDTH = 22  // do not change
)
(
    // (* X_INTERFACE_PARAMETER = "FREQ_HZ 62500000" *)
    (* X_INTERFACE_PARAMETER = "ASSOCIATED_CLKEN aclk" *)
    (* X_INTERFACE_PARAMETER = "ASSOCIATED_BUSIF S_AXIS:M_AXIS_AC_LMS:M_AXIS_AC16:M_AXIS_ACDC" *)
    input aclk,
    input wire [S_AXIS_DATA_WIDTH-1:0]  S_AXIS_tdata,
    input wire                          S_AXIS_tvalid,

    input sc_zero,
    input signed [31:0] dc_tau, // Q31 tau DC iir at cos-sin zero x
    input signed [31:0] dc, // Q22


     // Master side
    // AC signal of CH0 prepared at LMS width
    output wire [M_AXIS_DATA_WIDTH-1:0] M_AXIS_AC_LMS_tdata,
    output wire                         M_AXIS_AC_LMS_tvalid,
    // AC signal 16bit (14 significant as input)
    output wire [S_AXIS_DATA_WIDTH-1:0] M_AXIS_AC16_tdata,
    output wire                         M_AXIS_AC16_tvalid,

    // AC signal 32bit
    output wire [32-1:0]                M_AXIS_ACDC_tdata,
    output wire                         M_AXIS_ACDC_tvalid,

    output wire [31:0] dbg_m,
    output wire [31:0] dbg_mdc
    
);

    reg reg_sc_zero;
    
    reg signed [31:0] reg_dc_tau; // Q31 tau DC iir at cos-sin zero x
    reg signed [LMS_DATA_WIDTH-1:0] reg_dc; // Q22


    reg signed [LMS_DATA_WIDTH-1:0] m=0;
    reg signed [LMS_DATA_WIDTH-1:0] mdc=0;
    reg signed [LMS_DATA_WIDTH-1:0] mdc_mue_e1=0;
    reg signed [LMS_DATA_WIDTH-1:0] mdc_mue_e2=0;
    reg signed [LMS_DATA_WIDTH-1:0] mdc_mue_e3=0;
    reg signed [LMS_DATA_WIDTH-1:0] mdc_mue_e4=0;
    reg signed [LMS_DATA_WIDTH+2-1:0] mdc_mue_sum=0;
    reg signed [LMS_DATA_WIDTH+32-1:0] mdc1=0;
    reg signed [LMS_DATA_WIDTH+32-1:0] mdc2=0;
    reg signed [LMS_DATA_WIDTH+32-1:0] mdc_mue=0;
    reg signed [LMS_DATA_WIDTH-1:0] ac_signal=0;

    reg [1:0] rdecii = 0;

    always @ (posedge aclk)
    begin
        rdecii <= rdecii+1;
    end

    always @ (posedge rdecii[1])
    begin
        reg_sc_zero <= sc_zero;
        reg_dc_tau  <= dc_tau; // Q31 tau DC iir at cos-sin zero x
        reg_dc      <= {dc[LMS_DATA_WIDTH-1], dc[LMS_DATA_WIDTH-2:0]}; // Q22 -> QLMS (26.Q22)

        m <= {{(LMS_DATA_WIDTH-LMS_Q_WIDTH-1){S_AXIS_tdata[S_AXIS_SIGNAL_SIGNIFICANT_DATA_WIDTH-1]}}, // signum bit 13 extend to LMS_DATA_WIDTH
                                             {S_AXIS_tdata[S_AXIS_SIGNAL_SIGNIFICANT_DATA_WIDTH-1 : 0]}, // 14bit ADC -now-> 16bit decimated ADC data bits: 15..0
              {(LMS_Q_WIDTH+1-S_AXIS_SIGNAL_SIGNIFICANT_DATA_WIDTH){1'b0}} // fill remeining Q 0
             }; // = LMS data size : 4Q22 from Q16 data = { S 4 expand, D16, fill 6 remaing Q bits w 0 }

        //m <= {{(LMS_Q_WIDTH-S_AXIS_DATA_WIDTH){S_AXIS_tdata[S_AXIS_DATA_WIDTH-1]}}, // signum extend to LMS_DATA_Q_WIDTH
        //      S_AXIS_tdata, // 16
        //      {(LMS_DATA_WIDTH-LMS_Q_WIDTH){1'b0}} // fill 0  (4 bits prec)
        //      };
                 
        //  special IIR DC filter DC error on average of samples at 0,90,180,270
        // **** Consider a 4rth order IIR LP ****
        if (sc_zero)
        begin
            // mdc_mue <= (m-mdc) * dc_tau;
            mdc_mue_e1 <= m-mdc; // prepare for moving sum over period at 0,90,180,270
            mdc_mue_e2 <= mdc_mue_e1;
            mdc_mue_e3 <= mdc_mue_e2;
            mdc_mue_e4 <= mdc_mue_e3;
            mdc_mue_sum  <= mdc_mue_e1+mdc_mue_e2+mdc_mue_e3+mdc_mue_e4 + $signed(2);
            mdc_mue <= (mdc_mue_sum>>>2) * reg_dc_tau;
            mdc1 <= mdc2 + mdc_mue; // + $signed(58'sh80000000);
        end
        else
        begin
            mdc2 <= mdc1;
            mdc  <= mdc1[LMS_DATA_WIDTH+32-1:32]; // (LMS_DATA_WIDTH-LMS_Q_WIDTH) Q LMS_Q_WIDTH = 22Q4
        end
    
        ac_signal <= m - $signed(reg_dc_tau[31] ? reg_dc : mdc); // auto IIR dc or manual dc
    end

    assign M_AXIS_AC_LMS_tdata = { {(M_AXIS_DATA_WIDTH-LMS_DATA_WIDTH){ac_signal[LMS_DATA_WIDTH-1]}}, ac_signal[LMS_DATA_WIDTH-1:0] }; // LMS [22.4] expanded to 32
    assign M_AXIS_AC_LMS_tvalid = 1;

    assign M_AXIS_AC16_tdata = { ac_signal[LMS_DATA_WIDTH-1] , ac_signal[LMS_DATA_WIDTH-LMS_Q_WIDTH+15-1 : LMS_DATA_WIDTH-LMS_Q_WIDTH] }; // "original 16 of AC signal"
    assign M_AXIS_AC16_tvalid = 1;
    
    assign M_AXIS_ACDC_tdata = { mdc[LMS_Q_WIDTH-1 : LMS_Q_WIDTH-16]  , // M DC 16", 
                                 ac_signal[LMS_Q_WIDTH-1 : LMS_Q_WIDTH-16] }; // "original AC 16"
    assign M_AXIS_ACDC_tvalid = 1;

    assign dbg_m   = {{(32-LMS_DATA_WIDTH){m[LMS_DATA_WIDTH-1]}}, m};     // Q LMS Sf[22.4] M value (input signal, 'decimated')
    assign dbg_mdc = {{(32-LMS_DATA_WIDTH){mdc[LMS_DATA_WIDTH-1]}}, mdc}; // Q LMS Sf[22.4] MDC value (DC value, filtered)


endmodule
