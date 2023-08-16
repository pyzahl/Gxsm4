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


module SignedQ31toQ22(
    input signed [31:0] x,
    output signed [22:0] y
    );
    assign y = {x[31],x[21:0]};
endmodule
