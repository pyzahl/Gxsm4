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


module led_connect #(
    parameter NUM_LEDS = 8
)
(
    input led1,
    input led2,
    input led3,
    input led4,
    input led5,
    input led6,
    input led7,
    input led8,
    
    output [NUM_LEDS-1:0] leds
);
    assign leds = { led8, led7, led6, led5, led4, led3, led2, led1 };

endmodule
