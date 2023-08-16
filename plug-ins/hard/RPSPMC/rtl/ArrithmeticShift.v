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
// Additional Comments:
// 
//////////////////////////////////////////////////////////////////////////////////


module ArrithmeticShift(
    input signed [31:0] x,
    output signed [22:0] y
    );
    reg signed [22:0] tmp;
    
    always @ (*)
    begin
        tmp <= x <<< (22-14);
    end
    assign y = tmp;
endmodule
