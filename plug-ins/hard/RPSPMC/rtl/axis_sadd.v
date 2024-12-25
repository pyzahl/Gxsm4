`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 12/22/2024 10:05:33 PM
// Design Name: 
// Module Name: axis_sadd
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


module axis_sadd #(
    parameter SAXIS_TDATA_WIDTH = 32,
    parameter MAXIS_TDATA_WIDTH = 32,
    parameter POS_SATURATION_LIMIT =  33'sd2147483647,
    parameter NEG_SATURATION_LIMIT = -33'sd2147483647,
    parameter POS_SATURATION_VALUE =  32'sd2147483647,
    parameter NEG_SATURATION_VALUE = -32'sd2147483647
    )
    (
    // (* X_INTERFACE_PARAMETER = "FREQ_HZ 125000000" *)
    (* X_INTERFACE_PARAMETER = "ASSOCIATED_CLKEN a_clk" *)
    (* X_INTERFACE_PARAMETER = "ASSOCIATED_BUSIF S_AXIS_A:S_AXIS_B:M_AXIS_SUM" *)
    input a_clk,
    input wire [SAXIS_TDATA_WIDTH-1:0]  S_AXIS_A_tdata,
    input wire                          S_AXIS_A_tvalid,
    input wire [SAXIS_TDATA_WIDTH-1:0]  S_AXIS_B_tdata,
    input wire                          S_AXIS_B_tvalid,
    output wire [MAXIS_TDATA_WIDTH-1:0] M_AXIS_SUM_tdata,
    output wire                         M_AXIS_SUM_tvalid
    );
    
    reg signed [SAXIS_TDATA_WIDTH+1-1:0] sum=0;

// Saturated result to 32bit
`define SATURATE(REG) (REG  > POS_SATURATION_LIMIT ? POS_SATURATION_VALUE : REG < NEG_SATURATION_LIMIT ? NEG_SATURATION_VALUE : REG[MAXIS_TDATA_WIDTH-1:0]) 

    always @ (posedge a_clk)
    begin
        sum <= $signed(S_AXIS_A_tdata) + $signed(S_AXIS_B_tdata);
    end
    
    assign M_AXIS_SUM_tdata  = `SATURATE(sum);  // sum > POS_SATURATION_LIMIT ? POS_SATURATION_VALUE : sum < NEG_SATURATION_LIMIT ? NEG_SATURATION_VALUE : sum[MAXIS_TDATA_WIDTH-1:0]; // Saturate
    assign M_AXIS_SUM_tvalid = S_AXIS_A_tvalid && S_AXIS_A_tvalid;
    
endmodule  