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
// Description: get top most bits from config
// 
// Dependencies: 
// 
// Revision:
// Revision 0.01 - File Created
// Additional Comments:
// 
//////////////////////////////////////////////////////////////////////////////////


module cfg_to_axis #(
    parameter SRC_ADDR = 0,
    parameter SRC_BITS = 32,
    parameter CFG_WIDTH = 1024,
    parameter DST_WIDTH = 32,
    parameter MAXIS_TDATA_WIDTH = 32
)
(
    (* X_INTERFACE_PARAMETER = "ASSOCIATED_CLKEN a_clk" *)
    (* X_INTERFACE_PARAMETER = "ASSOCIATED_BUSIF M_AXIS" *)
    input  a_clk,
    input [CFG_WIDTH-1:0] cfg,
    output [MAXIS_TDATA_WIDTH-1:0]      M_AXIS_tdata,
    output                              M_AXIS_tvalid,
    
    output wire [DST_WIDTH-1:0] data
);
    assign M_AXIS_tdata = {
        {(MAXIS_TDATA_WIDTH-DST_WIDTH){cfg[SRC_ADDR*32+SRC_BITS-1]}}, cfg[SRC_ADDR*32+SRC_BITS-1:SRC_ADDR*32+SRC_BITS-DST_WIDTH]
    };
    assign M_AXIS_tvalid = 1;
    assign data = cfg[SRC_ADDR*32+SRC_BITS-1:SRC_ADDR*32+SRC_BITS-DST_WIDTH];

endmodule
