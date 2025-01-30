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
// Create Date: 11/01/2017 01:04:34 AM
// Design Name:    part of RPSPMC
// Module Name:    SineSDB64
// Project Name:   RPSPMC 4 GXSM
// Target Devices: Zynq z7020
// Tool Versions:  Vivado 2023.1
// Description: pipelined SineSDB64 -- 5 steps, means delta cr/ci is (5+1) delta
// 
// Dependencies: 
// 
// Revision:
// Revision 0.01 - File Created
// Additional Comments:
// 
//////////////////////////////////////////////////////////////////////////////////


module SineSDB64 #(
    parameter AXIS_TDATA_WIDTH = 64,
    parameter signed CR_INIT = 32'h7FFFFFFF,  // ((1<<31)-1)
    parameter signed CI_INIT = 32'h00000000,
    parameter signed ERROR_E_INIT = 64'h3FFFFFFFFFFFFFFF //4611686018427387903
)
(
    (* X_INTERFACE_PARAMETER = "FREQ_HZ 125000000" *)
    input aclk,
    (* X_INTERFACE_PARAMETER = "FREQ_HZ 125000000" *)
    input wire [AXIS_TDATA_WIDTH-1:0]  S_AXIS_DELTAS_tdata, // deltas(Re,Im) vector
    input wire                         S_AXIS_DELTAS_tvalid,
    (* X_INTERFACE_PARAMETER = "FREQ_HZ 125000000" *)
    output wire [AXIS_TDATA_WIDTH-1:0] M_AXIS_SC_tdata, // (Sine, Cosine) vector
    output wire                        M_AXIS_SC_tvalid
);
    
    reg signed [31:0] deltasRe;
    reg signed [31:0] deltasIm;
    
    reg signed [31:0] cr = CR_INIT;
    reg signed [31:0] ci = CI_INIT;
    
    reg signed [63:0] RealP;
    reg signed [63:0] ImaP;
    
    reg signed [31:0] RealP_Hi;
    reg signed [31:0] ImaP_Hi;
    reg signed [31:0] Error_int;

// N-pipeline, one shot: 
// a_n = f(x)
// b_n = g(a_n-1), ..
// c_n = h(a_n-2, b_n-1), ..
// ..

//    always @ (posedge aclk)
//    begin
//        if (S_AXIS_DELTAS_tvalid)
//        begin
//            deltasRe <= {S_AXIS_DELTAS_tdata[AXIS_TDATA_WIDTH-1 : AXIS_TDATA_WIDTH/2]};
//            deltasIm <= {S_AXIS_DELTAS_tdata[AXIS_TDATA_WIDTH/2-1 : 0]};
//        end
//    end

    // *********** Sinus calculation
    always @ (posedge aclk) 
    begin
        deltasRe = {S_AXIS_DELTAS_tdata[AXIS_TDATA_WIDTH-1 : AXIS_TDATA_WIDTH/2]};
        deltasIm = {S_AXIS_DELTAS_tdata[AXIS_TDATA_WIDTH/2-1 : 0]};

        //    Real part calculation : crdr - cidi
        //  Q31*Q31 - Q31*Q31=Q62
        //  For cr=2147483647 and deltasRe=2129111627
        //      ci=0 and  deltasIm=280302863
        //  crdr - cidi = 2129111626 Q31 (RealP_Hi)
        //  RealP=4572232401620063669

        //    Imaginary part calculation : drci + crdi
        //  Q31*Q31 - Q31*Q31=Q62
        //  drci - crdi= -280302863 Q31 (ImaP_Hi)
        //  ImaP=-601945814499781361

        RealP  = (cr * deltasRe) - (ci * deltasIm);
        ImaP   = (ci * deltasRe) + (cr * deltasIm);
        
        RealP_Hi = RealP >>> 31; //Q62 to Q31
        ImaP_Hi  = ImaP >>> 31; //Q62 to Q31

        //  e=Error/2 = (1 - (cr^2+ci^2))/2
        //  e=(Q62 - Q62 - Q62))>>32 --> Q30 (so Q31/2)
        //  4611686018427387903 - (-280302863*-280302863) - (2129111626*2129111626)
        //  Error_e=4611686018427387903 - 78569695005996769 - 4533116315968363876
        //  Error_e = 7453027258
        //  Error_int = 1

        // Error_e   = ERROR_E_INIT - (ImaP_Hi * ImaP_Hi) - (RealP_Hi * RealP_Hi);
        // Error_int = (Error_e>>32); //Q62 to Q31 plus /2
        // Error_int = (ERROR_E_INIT - (RealP[62:31] * RealP[62:31]) - (ImaP[62:31] * ImaP[62:31])) >>> 32;
        Error_int = (ERROR_E_INIT - (RealP_Hi * RealP_Hi) - (ImaP_Hi * ImaP_Hi)) >>> 32;

        //    Update Cre and Cim
        //  Cr=Cr(1-e)=Cr-Cr*e // Q62 - Q31*Q31
        //  Ci=Ci(1-e)=Ci-Ci*e
        //  ImaP=-601945814499781361 - 1*-280302863 = -601945814219478498
        //  ci[0]=-280302862
        //  RealP=4572232401620063669 - 1*2129111626 = 4572232399490952043
        //  cr[0]=2129111625

        //ImaP    = ImaP + (ImaP_Hi*Error_int);
        //ImaP_Hi = (ImaP + (ImaP_Hi*Error_int)) >> 31; //Q62 to Q31
        //ImaP_Hi  = (ImaP  + ((ImaP>>>31)  * Error_int)) >>> 31; //Q62 to Q31
        ImaP_Hi  = (ImaP  + ((ImaP_Hi)  * Error_int)) >>> 31; //Q62 to Q31
        //RealP    = RealP + (RealP_Hi*Error_int);
        //RealP_Hi = (RealP + (RealP_Hi[i+1]*Error_int[i+2])) >> 31; //Q62 to Q31
        //RealP_Hi = (RealP + ((RealP>>>31) * Error_int)) >>> 31; //Q62 to Q31
        RealP_Hi = (RealP + ((RealP_Hi) * Error_int)) >>> 31; //Q62 to Q31

        if (ImaP_Hi > 2147483647) 
            ci = 2147483647;
        else if (ImaP_Hi < -2147483647)
            ci = -2147483647;
        else ci = ImaP_Hi;
        
        if (RealP_Hi > 2147483647)
            cr = 2147483647;
        else if (RealP_Hi < -2147483647)
            cr = -2147483647;
        else cr = RealP_Hi;
    end
 
    assign M_AXIS_SC_tdata = { cr[31:0], ci[31:0] }; // wire (cr, ci) to (sine, cosine) output vector 
    assign M_AXIS_SC_tvalid = S_AXIS_DELTAS_tvalid; // just pass
    
endmodule
