`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 11/26/2017 12:48:03 AM
// Design Name: 
// Module Name: ArrithmeticShift
// Project Name: 
// Target Devices: 
// Tool Versions: 
// Description: 
// 
// Dependencies: 
// 
// Revision:
// Revision 0.01 - File Created
// Additional Comments: assign y = {{(22-14){q[15]}},q[14:0]};
// 
//////////////////////////////////////////////////////////////////////////////////


module Signed16toQ22(
    input clk,
    input t_valid,
    input signed [23:0] x,
    output signed [22:0] y
    );
    
    reg signed [23:0] q=0;
    
    always @ (posedge clk)
    begin
        if (t_valid)
        begin
            q <= x;
        end
    end

    assign y = {q[23], q[22:1]};
endmodule
