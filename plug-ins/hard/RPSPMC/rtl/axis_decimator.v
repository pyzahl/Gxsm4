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


module axis_decimator #
(
    parameter decimation = 2,
    parameter AXIS_SIGNAL_TDATA_WIDTH = 32,
    parameter AXIS_SIGNAL_DATA_WIDTH = 16,
    parameter AXIS_SIGNAL_SIGNIFICANT_DATA_WIDTH = 14
)
(
    // Source side
    (* X_INTERFACE_PARAMETER = "ASSOCIATED_CLKEN adc_clk, ASSOCIATED_BUSIF S_AXIS_SIGNAL" *)
    input adc_clk,
    input wire [AXIS_SIGNAL_TDATA_WIDTH-1:0]  S_AXIS_SIGNAL_tdata,
    input wire                                S_AXIS_SIGNAL_tvalid,

    // Master side
    (* X_INTERFACE_PARAMETER = "ASSOCIATED_CLKEN aclk, ASSOCIATED_BUSIF M_AXIS_S0:M_AXIS_S1:M_AXIS_S01" *)
    input aclk,

    //(* X_INTERFACE_PARAMETER = "ASSOCIATED_CLKEN aclk" *)
    //(* X_INTERFACE_PARAMETER = "FREQ_HZ 62500000" *)
    output wire [AXIS_SIGNAL_DATA_WIDTH-1:0] M_AXIS_S0_tdata,
    output wire                              M_AXIS_S0_tvalid,
    //(* X_INTERFACE_PARAMETER = "ASSOCIATED_CLKEN aclk" *)
    //(* X_INTERFACE_PARAMETER = "FREQ_HZ 62500000" *)
    output wire [AXIS_SIGNAL_DATA_WIDTH-1:0] M_AXIS_S1_tdata,
    output wire                              M_AXIS_S1_tvalid,
    //(* X_INTERFACE_PARAMETER = "ASSOCIATED_CLKEN aclk" *)
    //(* X_INTERFACE_PARAMETER = "FREQ_HZ 62500000" *)
    output wire [AXIS_SIGNAL_TDATA_WIDTH-1:0] M_AXIS_S01_tdata,
    output wire                              M_AXIS_S01_tvalid
);

    //reg [2-1:0] dec_clk = 0;

    reg signed [AXIS_SIGNAL_DATA_WIDTH-1:0] x=0;
    reg signed [AXIS_SIGNAL_DATA_WIDTH-1:0] x1=0;
    reg signed [AXIS_SIGNAL_DATA_WIDTH-1:0] x2=0;
    reg signed [AXIS_SIGNAL_DATA_WIDTH-1:0] x3=0;
    reg signed [AXIS_SIGNAL_DATA_WIDTH-1:0] x4=0;
    reg signed [AXIS_SIGNAL_DATA_WIDTH-1:0] y=0;
    reg signed [AXIS_SIGNAL_DATA_WIDTH-1:0] y1=0;
    reg signed [AXIS_SIGNAL_DATA_WIDTH-1:0] y2=0;
    reg signed [AXIS_SIGNAL_DATA_WIDTH-1:0] y3=0;
    reg signed [AXIS_SIGNAL_DATA_WIDTH-1:0] y4=0;

    // Consider, if you have a data values i_data coming in with IWID (input width) bits, and you want to create a data value with OWID bits, why not just grab the top OWID bits?
    // assign	w_tozero = i_data[(IWID-1):0] + { {(OWID){1'b0}}, i_data[(IWID-1)], {(IWID-OWID-1){!i_data[(IWID-1)]}}};

    always @ (posedge adc_clk)
    begin
        //dec_clk <= dec_clk+1; // decimated analog clock
        
        x4 <= x3;
        x3 <= x2;
        x2 <= x1;
        x1 <= {{(AXIS_SIGNAL_DATA_WIDTH-AXIS_SIGNAL_SIGNIFICANT_DATA_WIDTH){S_AXIS_SIGNAL_tdata[AXIS_SIGNAL_SIGNIFICANT_DATA_WIDTH-1]}}, // signum bit 13 extend to 16
               {S_AXIS_SIGNAL_tdata[AXIS_SIGNAL_SIGNIFICANT_DATA_WIDTH-1 : 0]} // 14bit ADC data bits 13..0
              };

        y4 <= y3;
        y3 <= y2;
        y2 <= y1;
        y1 <= {{(AXIS_SIGNAL_DATA_WIDTH-AXIS_SIGNAL_SIGNIFICANT_DATA_WIDTH){S_AXIS_SIGNAL_tdata[AXIS_SIGNAL_DATA_WIDTH+AXIS_SIGNAL_SIGNIFICANT_DATA_WIDTH-1]}}, // signum bit 13 extend to 16
               {S_AXIS_SIGNAL_tdata[AXIS_SIGNAL_DATA_WIDTH+AXIS_SIGNAL_SIGNIFICANT_DATA_WIDTH-1 : AXIS_SIGNAL_DATA_WIDTH]} // 14bit ADC data bits 13..0
              };

        //if (aclk[0])
        //begin
        //    x <= (x4 + x3 + x2 + x1 + $signed(2)); // >>> 2; // input data is 14bits only, so 4x fits 16!
        //    y <= (y4 + y3 + y2 + y1 + $signed(2)); // >>> 2; 
        //end
    end

    always @ (posedge aclk)
    begin
        x <= (x4 + x3 + x2 + x1); // >>> 2; // input data is 14bits only, so 4x fits 16!
        y <= (y4 + y3 + y2 + y1); // >>> 2; 
    end


    assign M_AXIS_S0_tdata = x; // x[(16-1):0] + { {(14){1'b0}}, x[(16-1)], {(16-14-1){!x[(16-1)]}}}; // x
    assign M_AXIS_S0_tvalid = S_AXIS_SIGNAL_tvalid;
    assign M_AXIS_S1_tdata = y;
    assign M_AXIS_S1_tvalid = S_AXIS_SIGNAL_tvalid;
    assign M_AXIS_S01_tdata = S_AXIS_SIGNAL_tdata;
    assign M_AXIS_S01_tvalid = S_AXIS_SIGNAL_tvalid;

endmodule
