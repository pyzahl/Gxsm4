`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 11/26/2017 08:20:47 PM
// Design Name: 
// Module Name: signal_combine
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


module signal_combine #(
    parameter ADC_WIDTH = 14,
    parameter ADC_DATA_WIDTH = 16,
    parameter AXIS_TDATA_WIDTH = 32
)
(
    (* X_INTERFACE_PARAMETER = "FREQ_HZ 62500000" *)
    input s1_clk,
    input s2_clk,
    input wire [AXIS_TDATA_WIDTH-1:0]  S_AXIS_PORT1_tdata,
    input wire                         S_AXIS_PORT1_tvalid,
    input wire [AXIS_TDATA_WIDTH-1:0]  S_AXIS_PORT2_tdata,
    input wire                         S_AXIS_PORT2_tvalid,
    output [AXIS_TDATA_WIDTH-1:0]       M_AXIS_tdata,
    output                              M_AXIS_tvalid
);
    reg signed [AXIS_TDATA_WIDTH-1:0] s1 = 32'h00000000;
    reg signed [AXIS_TDATA_WIDTH-1:0] s2 = 32'h00000000;
    
    always @ (posedge s1_clk)
    begin
        if (S_AXIS_PORT1_tvalid)
        begin
            s1 <= S_AXIS_PORT1_tdata;
        end
    end

    always @ (posedge s2_clk)
    begin
        if (S_AXIS_PORT2_tvalid)
        begin
            s2 <= S_AXIS_PORT2_tdata;
        end
    end
        
//    assign M_AXIS_PORT1_tdata = {{(AXIS_TDATA_WIDTH-ADC_DATA_WIDTH+1){S_AXIS_tdata[ADC_DATA_WIDTH-1]}},S_AXIS_tdata[ADC_DATA_WIDTH-1:0]};
//    assign M_AXIS_PORT2_tdata = {{(AXIS_TDATA_WIDTH-ADC_DATA_WIDTH+1){S_AXIS_tdata[AXIS_TDATA_WIDTH-1]}},S_AXIS_tdata[AXIS_TDATA_WIDTH-1:ADC_DATA_WIDTH]};
    assign M_AXIS_tdata = {
        {(ADC_DATA_WIDTH-ADC_WIDTH){s1[AXIS_TDATA_WIDTH-1]}}, s1[AXIS_TDATA_WIDTH-1:AXIS_TDATA_WIDTH-ADC_WIDTH],
        {(ADC_DATA_WIDTH-ADC_WIDTH){s2[AXIS_TDATA_WIDTH-1]}}, s2[AXIS_TDATA_WIDTH-1:AXIS_TDATA_WIDTH-ADC_WIDTH]
    };
    assign M_AXIS_tvalid = 1;

endmodule
