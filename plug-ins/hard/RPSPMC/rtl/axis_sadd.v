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
    parameter POS_SATURATION_LIMIT =  36'sd2147483648,
    parameter NEG_SATURATION_LIMIT = -36'sd2147483647,
    parameter POS_SATURATION_VALUE =  32'sd2147483648,
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
    
    reg signed [SAXIS_TDATA_WIDTH+2-1:0] a=0;
    reg signed [SAXIS_TDATA_WIDTH+2-1:0] b=0;
    reg signed [SAXIS_TDATA_WIDTH+2-1:0] sum=0;
    reg signed [MAXIS_TDATA_WIDTH-1:0]   res=0;

    always @ (posedge a_clk)
    begin
        if (S_AXIS_A_tvalid && S_AXIS_B_tvalid)
        begin
            a      <= S_AXIS_A_tdata;
            b      <= S_AXIS_B_tdata;
            sum    <= a+b;
            if (sum > POS_SATURATION_LIMIT)
            begin
                res <= POS_SATURATION_VALUE;
            end     
            else
            begin     
                if (sum < NEG_SATURATION_LIMIT)
                begin
                    res <= NEG_SATURATION_VALUE;
                end 
                else
                begin
                    res <= {sum[SAXIS_TDATA_WIDTH+2-1], sum[MAXIS_TDATA_WIDTH-2:0]};
                end
            end         
        end
    end
    
    assign M_AXIS_SUM_tdata  = res;
    assign M_AXIS_SUM_tvalid = S_AXIS_A_tvalid && S_AXIS_A_tvalid;
    
endmodule  