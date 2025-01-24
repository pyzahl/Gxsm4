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


module cfg_buffer_reg #(
    parameter CFG_WIDTH = 1024
)
(
    (* X_INTERFACE_PARAMETER = "ASSOCIATED_CLKEN a_clk" *)
    input		   a_clk,
    input [CFG_WIDTH-1:0]  cfg,
    output [CFG_WIDTH-1:0] cfg_buf
);

    // buffer in register to relaxtiming requiremnets for distributed placing
   reg [CFG_WIDTH-1:0]	   cfg_reg[1:0];

    always @ (posedge a_clk)
    begin
       cfg_reg[0] <= cfg;
       cfg_reg[1] <= cfg_reg[0];
     end

    assign cfg_buf = cfg_reg[1];

endmodule
