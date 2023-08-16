//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 10/30/2017 08:20:38 PM
// Design Name: 
// Module Name: SineSDB64_tb
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

//def adjust (hz)
//    fclk = 1e6 
//    dphi = 2.*pi*(hz/fclk)
//    dRe = int (Q31 * cos (dphi))
//    dIm = int (Q31 * sin (dphi))
//    return (dRe, dIm)
//Q31 = 0x7FFFFFFF # ((1<<31)-1);
//Q32 = 0xFFFFFFFF # ((1<<32)-1);
//cr = Q31 # ((1<<32)-1);
//ci = 0;


`timescale 1ns / 1ps


module SineSDB64_tb();     
    parameter AXIS_TDATA_WIDTH = 64;
    parameter signed CR_INIT = 32'h7FFFFFFF;  // ((1<<31)-1)
    parameter signed CI_INIT = 32'h00000000;
    parameter signed ERROR_E_INIT = 64'h3FFFFFFFFFFFFFFF; //4611686018427387903
    reg clock;
    reg signed [31:0] deltasRe;
    reg signed [31:0] deltasIm;
    reg signed [AXIS_TDATA_WIDTH-1:0] deltasReIm;
    wire signed [AXIS_TDATA_WIDTH-1:0] sinecosine_out;
    wire scv;
 

    SineSDB64 s_sdb64 (
              .aclk(clock),
              .S_AXIS_DELTAS_tdata(deltasReIm),
              .S_AXIS_DELTAS_tvalid(1),
              .M_AXIS_SC_tdata(sinecosine_out),
              .M_AXIS_SC_tvalid(scv)
              );

    initial begin
        clock = 0;
        deltasRe = 2145957801;
        deltasIm = 80939050;
        deltasReIm = { deltasRe, deltasIm };
        forever #1 clock = ~clock;
    end
endmodule
    