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
    (* X_INTERFACE_PARAMETER = "ASSOCIATED_CLKEN a_clk, ASSOCIATED_BUSIF S_AXIS_in" *)
    input  a_clk,
    input [DATA_WIDTH-1:0]  S_AXIS_in_tdata,
    input                   S_AXIS_in_tvalid,
    output                  S_AXIS_in_tready,
    output [DATA_WIDTH-1:0] out_data
);

    assign S_AXIS_in_tready = 1;

    // buffer in register to relaxtiming requiremnets for distributed placing
   reg [DATA_WIDTH-1:0]	   oreg;

    always @ (posedge a_clk)
    begin
        if (S_AXIS_in_tvalid)
            oreg <= S_AXIS_in_tdata;
    end

    assign out_data = oreg;

endmodule
