`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 11/01/2017 07:29:30 PM
// Design Name: 
// Module Name: split_signal
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


module split_signal #
(
    parameter ADC_DATA_WIDTH = 14,
    parameter S_AXIS_TDATA_WIDTH = 32,
    parameter M_AXIS_TDATA_WIDTH = 16
)
(
    (* X_INTERFACE_PARAMETER = "FREQ_HZ 125000000" *)
    input [S_AXIS_TDATA_WIDTH-1:0]        S_AXIS_tdata,
    input                                 S_AXIS_tvalid,
    (* X_INTERFACE_PARAMETER = "FREQ_HZ 125000000" *)
    output wire [M_AXIS_TDATA_WIDTH-1:0]  M_AXIS_PORT1_tdata,
    output wire                           M_AXIS_PORT1_tvalid,
    (* X_INTERFACE_PARAMETER = "FREQ_HZ 125000000" *)
    output wire [M_AXIS_TDATA_WIDTH-1:0]  M_AXIS_PORT2_tdata,
    output wire                           M_AXIS_PORT2_tvalid,
    (* X_INTERFACE_PARAMETER = "FREQ_HZ 125000000" *)
    output wire [S_AXIS_TDATA_WIDTH-1:0]  M_AXIS_PASS_tdata, // (Sine, Cosine) vector pass through
    output wire                           M_AXIS_PASS_tvalid
    );
        
    assign M_AXIS_PORT1_tdata = {{(M_AXIS_TDATA_WIDTH-ADC_DATA_WIDTH+1){S_AXIS_tdata[ADC_DATA_WIDTH-1]}},S_AXIS_tdata[ADC_DATA_WIDTH-1:0]};
    assign M_AXIS_PORT2_tdata = {{(M_AXIS_TDATA_WIDTH-ADC_DATA_WIDTH+1){S_AXIS_tdata[S_AXIS_TDATA_WIDTH-1]}},S_AXIS_tdata[S_AXIS_TDATA_WIDTH-1:ADC_DATA_WIDTH]};
    assign M_AXIS_PORT1_tvalid = S_AXIS_tvalid;
    assign M_AXIS_PORT2_tvalid = S_AXIS_tvalid;

    assign M_AXIS_PASS_tdata  = S_AXIS_tdata; // pass
    assign M_AXIS_PASS_tvalid = S_AXIS_tvalid; // pass

endmodule

