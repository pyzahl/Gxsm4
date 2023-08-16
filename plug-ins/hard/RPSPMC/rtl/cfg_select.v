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


module cfg_select #(
    parameter SRC_ADDR = 0,
    parameter CFG_WIDTH = 1024,
    parameter CFG_SWBIT = 0
)
(
    input [CFG_WIDTH-1:0] cfg,
    output wire status
);

    assign status = cfg[SRC_ADDR*32+CFG_SWBIT];

endmodule
