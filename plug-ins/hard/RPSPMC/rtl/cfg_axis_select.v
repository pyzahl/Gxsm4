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


module cfg_axis_select #(
    parameter SRC_ADDR = 0,
    parameter CFG_WIDTH = 1024,
    parameter CFG_SWBIT = 0,
    parameter AXIS_TDATA_WIDTH = 32
)
(
    input [CFG_WIDTH-1:0] cfg,
    input a_clk,
    (* X_INTERFACE_PARAMETER = "FREQ_HZ 125000000" *)
    input [AXIS_TDATA_WIDTH-1:0]      S1_AXIS_tdata,
    input                             S1_AXIS_tvalid,
    (* X_INTERFACE_PARAMETER = "FREQ_HZ 125000000" *)
    input [AXIS_TDATA_WIDTH-1:0]      S2_AXIS_tdata,
    input                             S2_AXIS_tvalid,
    (* X_INTERFACE_PARAMETER = "FREQ_HZ 125000000" *)
    output [AXIS_TDATA_WIDTH-1:0]      M_AXIS_tdata,
    output                             M_AXIS_tvalid,
    
    output wire status
);

    assign M_AXIS_tdata  = (cfg[SRC_ADDR*32+CFG_SWBIT]) ? S2_AXIS_tdata  : S1_AXIS_tdata;
    assign M_AXIS_tvalid = (cfg[SRC_ADDR*32+CFG_SWBIT]) ? S2_AXIS_tvalid : S1_AXIS_tvalid;
    assign status = cfg[SRC_ADDR*32+CFG_SWBIT];

endmodule
