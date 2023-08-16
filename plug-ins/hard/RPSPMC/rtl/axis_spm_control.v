`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 08/14/2023 10:22:53 PM
// Design Name: 
// Module Name: axis_spm_control
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


module axis_spm_control#(
    parameter SAXIS_TDATA_WIDTH = 32
)
(
    input [32-1:0] rotm[4], // ..
    
    // SCAN COMPONENTS, ROTATED RELATIVE COORDS
    input [32-1:0] xs, // vector components
    input [32-1:0] ys, // ..
    input [32-1:0] zs, // ..
    input [32-1:0] u, // ..

    // SCAN POSITION COMPONENTS, ABSOLUTE COORDS
    input [32-1:0] x0, // vector components
    input [32-1:0] y0, // ..
    input [32-1:0] z0, // ..

    (* X_INTERFACE_PARAMETER = "ASSOCIATED_CLKEN a_clk" *)
    (* X_INTERFACE_PARAMETER = "ASSOCIATED_BUSIF S_AXIS1:S_AXIS2:S_AXIS3:S_AXIS4" *)
    input a_clk,
    output wire [SAXIS_TDATA_WIDTH-1:0]  S_AXIS1_tdata,
    output wire                          S_AXIS1_tvalid,
    output wire [SAXIS_TDATA_WIDTH-1:0]  S_AXIS2_tdata,
    output wire                          S_AXIS2_tvalid,
    output wire [SAXIS_TDATA_WIDTH-1:0]  S_AXIS3_tdata,
    output wire                          S_AXIS3_tvalid,
    output wire [SAXIS_TDATA_WIDTH-1:0]  S_AXIS4_tdata,
    output wire                          S_AXIS4_tvalid,

    output [32-1:0] xs_mon, // vector components
    output [32-1:0] ys_mon, // ..
    output [32-1:0] zs_mon, // ..
    output [32-1:0] u_mon // ..

    );
    
    assign S_AXIS1_tdata  = x0+xs;
    assign S_AXIS1_tvalid = 1;
    
    assign S_AXIS1_tdata  = y0+ys;
    assign S_AXIS2_tvalid = 1;
    
    assign S_AXIS1_tdata  = z0+zs;
    assign S_AXIS3_tvalid = 1;
    
    assign S_AXIS1_tdata  = u;
    assign S_AXIS4_tvalid = 1;

    assign xs_mon = xs;
    assign ys_mon = ys;
    assign zs_mon = zs;
    assign u_mon  = u;
    
    
endmodule
