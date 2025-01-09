`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: axis_biquad 
// Engineer: https://github.com/controlpaths/blog/tree/main/axis_biquad
// 
// Create Date: 01/04/2025 10:18:00 AM
// Design Name: 
// Module Name: axis_biquad_iir_filter
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
/*
BiQuad Direct form 1
y[n] = b0 x[n] + b1 x[n-1] + b2 x[n-2] - a1 y[n-1] - a2 y[n-2]

x[n] ------ *b0 ---> + -------------> y[n]
        |            |            |
input   z  pipe1     |            z   output pipe1
        |            |            |
x[n-1]  --- *b1 ---> + <-- -a1* ---   y[n-1]
        |            |            |
input   z  pipe2     |            z   output pipe2
        |            |            |
x[n-2]  --- *b2 ---> + <-- -a2* ---   y[n-2]

*/

module axis_biquad_iir_filter #(
    parameter signal_width = 32,
    parameter coefficient_width = 32,
    parameter coefficient_Q = 28,
    /* config address */
    parameter configuration_address = 999
    )(
    (* X_INTERFACE_PARAMETER = "ASSOCIATED_CLKEN aclk, ASSOCIATED_BUSIF S_AXIS_in:M_AXIS_out:M_AXIS_pass" *)
    input aclk,
    
    input [32-1:0]  config_addr,
    input [512-1:0] config_data,
    
    /* slave axis interface */
    input [signal_width-1:0] S_AXIS_in_tdata,
    input                    S_AXIS_in_tvalid,

    input wire axis_decii_clk,
    
    /* master axis interface */
    output [signal_width-1:0] M_AXIS_out_tdata,
    output                    M_AXIS_out_tvalid,
    
    output [signal_width-1:0] M_AXIS_pass_tdata,
    output                    M_AXIS_pass_tvalid
    );
    
    localparam internal_extra = 4;
    localparam ONE = ((1<<coefficient_Q)-1);
    
    reg resetn=0;
    
    reg signed [signal_width-1:0] x; /* input data internal size */ // was wire
    reg signed [signal_width-1:0] y; /* input data internal size */ // was wire
    
    reg signed [coefficient_width-1:0] b0=ONE; /* coefficient internal size */
    reg signed [coefficient_width-1:0] b1=0; /* coefficient internal size */
    reg signed [coefficient_width-1:0] b2=0; /* coefficient internal size */
    reg signed [coefficient_width-1:0] a0=0; /* coefficient internal size */
    reg signed [coefficient_width-1:0] a1=0; /* coefficient internal size */
    reg signed [coefficient_width-1:0] a2=0; /* coefficient internal size */

    
    reg signed [signal_width+internal_extra-1:0] x1=0; /* input data pipeline */
    reg signed [signal_width+internal_extra-1:0] x2=0; /* input data pipeline */
    reg signed [signal_width+internal_extra-1:0] y1=0; /* output data pipeline */
    reg signed [signal_width+internal_extra-1:0] y2=0; /* output data pipeline */
    
    reg signed [signal_width+internal_extra + coefficient_width-1:0] x_b0=0; /* product input */
    reg signed [signal_width+internal_extra + coefficient_width-1:0] x_b1=0; /* product input */
    reg signed [signal_width+internal_extra + coefficient_width-1:0] x_b2=0; /* product input */
    reg signed [signal_width+internal_extra + coefficient_width-1:0] y_a1=0; /* product output */
    reg signed [signal_width+internal_extra + coefficient_width-1:0] y_a2=0; /* product output */
    
    reg decii_clk=0;
    reg run=0;
    

    assign M_AXIS_pass_tdata  = S_AXIS_in_tdata;
    assign M_AXIS_pass_tvalid = S_AXIS_in_tvalid;


    /* pipeline registers */
    always @(posedge aclk)
    begin
        // module configuration
        if (config_addr == configuration_address) // BQ configuration, and auto reset
        begin
            b0 <= config_data[1*32-1 : 0*32]; // assuming all 32bits.
            b1 <= config_data[2*32-1 : 1*32];
            b2 <= config_data[3*32-1 : 2*32];
            a0 <= config_data[4*32-1 : 3*32]; // *** not used here, template
            a1 <= config_data[5*32-1 : 4*32];
            a2 <= config_data[6*32-1 : 5*32];
            resetn <= 0;                     
        end
        else // run
            resetn <= 1;
            decii_clk <= axis_decii_clk;                     
    end    

    
    // pipeline of registers

    always @(posedge aclk)
    //always @(posedge axis_decii_clk)
    begin
        if (decii_clk) // run at decimated rate as given by axis_decii_clk
            run <= 1;
        
        if (run)
            run <= 0;
            
        if (!resetn) begin
          x1 <= 0;
          x2 <= 0;
          y1 <= 0;
          y2 <= 0;
        end
        else if (S_AXIS_in_tvalid && run) 
        begin
            x   <= S_AXIS_in_tdata;
            x1  <= x;
            x2  <= x1;
            y1  <= (x_b0 + x_b1 + x_b2 - y_a1 - y_a2) >>> (coefficient_Q);
            y2  <= y1;
            
            y <= y1 >>> (internal_extra);
        end
            
    // compute IIR kernels
        if (!resetn) begin
            x_b0  <= 0;
            x_b1  <= 0;
            x_b2  <= 0;
            y_a1 <= 0;
            y_a2 <= 0;
        end
        else if (run) 
        begin
            x_b0  <= b0 * x;   // IIR 1st stage (n)
            x_b1  <= b1 * x1; // IIR 1st stage (n-1)
            x_b2  <= b2 * x2;
            y_a1 <= a1 * y1;
            y_a2 <= a2 * y2;    
        end
    end
   
    assign M_AXIS_out_tdata  = y;
    assign M_AXIS_out_tvalid = 1;
      
endmodule
