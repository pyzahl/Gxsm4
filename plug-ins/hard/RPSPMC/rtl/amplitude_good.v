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
// Module Name: amplitude goood check
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


module amplitude_good#(
    parameter AXIS_TDATA_WIDTH = 32
)
(
    (* X_INTERFACE_PARAMETER = "ASSOCIATED_CLKEN aclk" *)
    (* X_INTERFACE_PARAMETER = "ASSOCIATED_BUSIF S_AXIS_AM:S_AXIS_AMTHR" *)
    input aclk,
    input wire [AXIS_TDATA_WIDTH-1:0]  S_AXIS_AM_tdata, // Amplitude Reading
    input wire                         S_AXIS_AM_tvalid,
    input wire [AXIS_TDATA_WIDTH-1:0]  S_AXIS_AMTHR_tdata, // Amplitude Reading
    input wire                         S_AXIS_AMTHR_tvalid,

    output wire hold
    );
    
    reg [AXIS_TDATA_WIDTH-1:0] a=0;
    reg [AXIS_TDATA_WIDTH-1:0] thr=0;
    reg not_good=0;

    reg [1:0] rdecii = 0;

/*
    always @ (posedge aclk)
    begin
        rdecii <= rdecii+1;
    end
*/
    //always @ (posedge rdecii[1])
    always @ (posedge aclk)
    begin
        rdecii <= rdecii+1; // rdecii 00 01 *10 11 00 ...
        if (rdecii == 1)
        begin
            a   <= S_AXIS_AM_tdata;
            thr <= S_AXIS_AMTHR_tdata;
            not_good <= a > thr ? 0:1; // 0 means OK (no hold)
        end
    end
     
    assign hold = not_good;
endmodule
