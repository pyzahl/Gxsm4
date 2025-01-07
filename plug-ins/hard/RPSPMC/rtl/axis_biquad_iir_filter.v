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
    parameter inout_width = 32,
    parameter inout_decimal_width = 31,
    parameter coefficient_width = 32,
    parameter coefficient_decimal_width = 31,
    parameter internal_width = 32,
    parameter internal_decimal_width = 31,
    /* coefficients */
    parameter signed b0 = 0,
    parameter signed b1 = 0,
    parameter signed b2 = 0,
    parameter signed a1 = 0,
    parameter signed a2 = 0,
    /* config address */
    parameter configuration_address = 999
    )(
    (* X_INTERFACE_PARAMETER = "ASSOCIATED_CLKEN aclk, ASSOCIATED_BUSIF S_AXIS:M_AXIS" *)
    input aclk,
    
    input [32-1:0]  config_addr,
    input [512-1:0] config_data,
    
    /* slave axis interface */
    input [inout_width-1:0] S_AXIS_tdata,
    input S_AXIS_tvalid,
    
    /* master axis interface */
    output [inout_width-1:0] M_AXIS_tdata,
    output M_AXIS_tvalid
    );
    
    localparam inout_integer_width = inout_width - inout_decimal_width; /* compute integer width */
    localparam coefficient_integer_width = coefficient_width - coefficient_decimal_width; /* compute integer width */
    localparam internal_integer_width = internal_width - internal_decimal_width; /* compute integer width */
    localparam Q31 = ((1<<31)-1);
    
    reg resetn=0;
    
    reg signed [internal_width-1:0] input_int; /* input data internal size */ // was wire
    
    reg signed [internal_width-1:0] b0_int=Q31; /* coefficient internal size */
    reg signed [internal_width-1:0] b1_int=0; /* coefficient internal size */
    reg signed [internal_width-1:0] b2_int=0; /* coefficient internal size */
    reg signed [internal_width-1:0] a1_int=0; /* coefficient internal size */
    reg signed [internal_width-1:0] a2_int=0; /* coefficient internal size */

    //wire signed [internal_width-1:0] output_int; /* output internal size */
    
    reg signed [internal_width-1:0] input_pipe1=0; /* input data pipeline */
    reg signed [internal_width-1:0] input_pipe2=0; /* input data pipeline */
    reg signed [internal_width-1:0] output_pipe1=0; /* output data pipeline */
    reg signed [internal_width-1:0] output_pipe2=0; /* output data pipeline */
    
    reg signed [internal_width + internal_width-1:0] input_b0=0; /* product input */
    reg signed [internal_width + internal_width-1:0] input_b1=0; /* product input */
    reg signed [internal_width + internal_width-1:0] input_b2=0; /* product input */
    reg signed [internal_width + internal_width-1:0] output_a1=0; /* product output */
    reg signed [internal_width + internal_width-1:0] output_a2=0; /* product output */
    //reg signed [internal_width + internal_width-1:0] output_2int=0; /* adder output */
    
    assign M_AXIS_tvalid = S_AXIS_tvalid;



    /* pipeline registers */
    always @(posedge aclk)
    begin
        // module configuration
        if (config_addr == configuration_address) // BQ configuration, and auto reset
        begin
            b0_int <= { {(internal_integer_width-coefficient_integer_width){b0[coefficient_width-1]}}, 
                        config_data[1*32-1 : 0*32], // b0   <= config_32[0] 
                        {(internal_decimal_width-coefficient_decimal_width){1'b0}} };
            b1_int <= { {(internal_integer_width-coefficient_integer_width){b1[coefficient_width-1]}},
                        config_data[2*32-1 : 1*32], // b1   <= config_32[1]
                        {(internal_decimal_width-coefficient_decimal_width){1'b0}} };
            b2_int <= { {(internal_integer_width-coefficient_integer_width){b2[coefficient_width-1]}},
                        config_data[3*32-1 : 2*32], // b2   <= config_32[2]
                        {(internal_decimal_width-coefficient_decimal_width){1'b0}} };
            a1_int <= { {(internal_integer_width-coefficient_integer_width){a1[coefficient_width-1]}},
                        config_data[4*32-1 : 3*32], // a1   <= config_32[3]
                        {(internal_decimal_width-coefficient_decimal_width){1'b0}} };
            a2_int <= { {(internal_integer_width-coefficient_integer_width){a2[coefficient_width-1]}},
                        config_data[5*32-1 : 4*32], // a2    <= config_32[4]
                        {(internal_decimal_width-coefficient_decimal_width){1'b0}} };
            resetn <= 0;                     
        end
        else // run
            resetn <= 1;                     
    end    

    
    // pipeline of registers
    always @(posedge aclk)
    begin
        if (!resetn) begin
          input_pipe1 <= 0;
          input_pipe2 <= 0;
          output_pipe1 <= 0;
          output_pipe2 <= 0;
        end
        else if (S_AXIS_tvalid) 
        begin

            /* resize signals to internal width */
            input_int    <= { {(internal_integer_width-inout_integer_width){S_AXIS_tdata[inout_width-1]}},
                               S_AXIS_tdata,
                              {(internal_decimal_width-inout_decimal_width){1'b0}} };
            input_pipe1  <= input_int;
            input_pipe2  <= input_pipe1;
            output_pipe1 <= (input_b0 + input_b1 + input_b2 - output_a1 - output_a2) >>> (internal_decimal_width); //output_int;
            output_pipe2 <= output_pipe1;
        end
            
    // compute IIR kernels
        if (!resetn) begin
            input_b0  <= 0;
            input_b1  <= 0;
            input_b2  <= 0;
            output_a1 <= 0;
            output_a2 <= 0;
        end
        else begin
            input_b0  <= b0_int * input_int;   // IIR 1st stage (n)
            input_b1  <= b1_int * input_pipe1; // IIR 1st stage (n-1)
            input_b2  <= b2_int * input_pipe2;
            output_a1 <= a1_int * output_pipe1;
            output_a2 <= a2_int * output_pipe2;    
            //output_2int <= (input_b0 + input_b1 + input_b2 - output_a1 - output_a2);
            //output_int <= output_2int >>> (internal_decimal_width);
        end
    end
        
    //assign output_2int = input_b0 + input_b1 + input_b2 - output_a1 - output_a2;
    //assign output_int = output_2int >>> (internal_decimal_width);
   
    assign M_AXIS_tdata = output_pipe1 >>> (internal_decimal_width-inout_decimal_width);
    
endmodule
