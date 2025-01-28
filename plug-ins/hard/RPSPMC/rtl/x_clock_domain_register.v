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


module x_clock_domain_register #(
    parameter DATA_WIDTH = 1024
)
(
    (* X_INTERFACE_PARAMETER = "ASSOCIATED_CLKEN a_clk" *)
    input  in_clk,
    input  out_clk,
    input [DATA_WIDTH-1:0]  in_data,
    output [DATA_WIDTH-1:0] out_data
);

   reg [DATA_WIDTH-1:0]	   in_reg[2:0]; // pipeline for integrity check
   reg [DATA_WIDTH-1:0]	   oreg;

    always @ (posedge in_clk)
    begin
       in_reg[0] <= in_data;
       in_reg[1] <= in_reg[0];
       in_reg[2] <= in_reg[1];
    end

    always @ (posedge out_clk)
    begin
	 if (in_reg[1] == in_reg[2])
	       oreg <= in_reg[2];
    end

    assign out_data = oreg;

endmodule
