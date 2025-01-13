`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 01/11/2025 12:04:22 AM
// Design Name: 
// Module Name: readback_configuration
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


module readback_configuration#(
    /* module readback register addresses */
    parameter readback_Z_reg_address    = 100001,
    parameter readback_Bias_reg_address = 100002,
    parameter readbackTimingTest_reg_address  = 101999,
    parameter readbackTimingReset_reg_address = 102000,
    parameter readbackX_reg_address = 100999
    )(
    input aclk,
    
    input  [32-1:0] config_addr,
    output [32-1:0] gpio_dataA,
    output [32-1:0] gpio_dataB,

    input wire [32-1:0] Z_GVP_mon,
    input wire [32-1:0] Z_slope_mon,

    input wire [32-1:0] Bias_mon,
    input wire [32-1:0] Bias_GVP_mon,


    input wire [32-1:0] rbXa,
    input wire [32-1:0] rbXb
);

    reg [32-1:0]		reg_A=0;
    reg [32-1:0]		reg_B=0;
   
        
    assign gpio_dataA = reg_A;
    assign gpio_dataB = reg_B;

    always @(posedge aclk)
    begin
        // module readback configuration
        case (config_addr)
        readback_Z_reg_address:
        begin
            reg_A <= Z_GVP_mon;
            reg_B <= Z_slope_mon;
	    end

        readback_Bias_reg_address:
        begin
            reg_A <= Bias_mon;
            reg_B <= Bias_GVP_mon;
	    end
	  
        readbackX_reg_address:
	    begin
            reg_A <= rbXa;
            reg_B <= rbXb;
	    end
        readbackTimingReset_reg_address:
	    begin
            reg_A <= 0; 
            reg_B <= 0; 
	    end
        readbackTimingTest_reg_address:
	    begin
            reg_A <= 125000000; 
            reg_B <= reg_A; 
	    end
	default:
	  begin
            reg_A <= reg_A+1;
            reg_B <= reg_A+13;
	  end
        endcase
    end    
    
endmodule
