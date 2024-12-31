`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 12/27/2024 07:43:40 PM
// Design Name: 
// Module Name: axis_selector
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
// Design for a half-adder
module ha ( input   a, b,
            output  sum, cout);

  assign sum  = a ^ b;
  assign cout = a & b;
endmodule

// A top level design that contains N instances of half adder
module my_design
	#(parameter N=4)
		(	input [N-1:0] a, b,
			output [N-1:0] sum, cout);

	// Declare a temporary loop variable to be used during
	// generation and won't be available during simulation
	genvar i;

	// Generate for loop to instantiate N times
	generate
		for (i = 0; i < N; i = i + 1) begin
          ha u0 (a[i], b[i], sum[i], cout[i]);
		end
	endgenerate
endmodule
*/


module axis_selector #(
    parameter SAXIS_TDATA_WIDTH = 32,
    parameter MAXIS_TDATA_WIDTH = 32
)
(
    // (* X_INTERFACE_PARAMETER = "FREQ_HZ 125000000" *)
    (* X_INTERFACE_PARAMETER = "ASSOCIATED_CLKEN a_clk" *)
    (* X_INTERFACE_PARAMETER = "ASSOCIATED_BUSIF S_AXIS_00:S_AXIS_01:S_AXIS_02:S_AXIS_03:S_AXIS_04:S_AXIS_05:S_AXIS_06:S_AXIS_07:S_AXIS_08:S_AXIS_09:S_AXIS_10:S_AXIS_11:S_AXIS_12:S_AXIS_13:S_AXIS_14:S_AXIS_15:M_AXIS_1:M_AXIS_2:M_AXIS_3:M_AXIS_4:M_AXIS_5:M_AXIS_6" *)
    input a_clk,
    
    input wire [SAXIS_TDATA_WIDTH-1:0]  S_AXIS_00_tdata,
    input wire                          S_AXIS_00_tvalid,
    input wire [SAXIS_TDATA_WIDTH-1:0]  S_AXIS_01_tdata,
    input wire                          S_AXIS_01_tvalid,
    input wire [SAXIS_TDATA_WIDTH-1:0]  S_AXIS_02_tdata,
    input wire                          S_AXIS_02_tvalid,
    input wire [SAXIS_TDATA_WIDTH-1:0]  S_AXIS_03_tdata,
    input wire                          S_AXIS_03_tvalid,
    input wire [SAXIS_TDATA_WIDTH-1:0]  S_AXIS_04_tdata,
    input wire                          S_AXIS_04_tvalid,
    input wire [SAXIS_TDATA_WIDTH-1:0]  S_AXIS_05_tdata,
    input wire                          S_AXIS_05_tvalid,
    input wire [SAXIS_TDATA_WIDTH-1:0]  S_AXIS_06_tdata,
    input wire                          S_AXIS_06_tvalid,
    input wire [SAXIS_TDATA_WIDTH-1:0]  S_AXIS_07_tdata,
    input wire                          S_AXIS_07_tvalid,
    input wire [SAXIS_TDATA_WIDTH-1:0]  S_AXIS_08_tdata,
    input wire                          S_AXIS_08_tvalid,
    input wire [SAXIS_TDATA_WIDTH-1:0]  S_AXIS_09_tdata,
    input wire                          S_AXIS_09_tvalid,
    input wire [SAXIS_TDATA_WIDTH-1:0]  S_AXIS_10_tdata,
    input wire                          S_AXIS_10_tvalid,
    input wire [SAXIS_TDATA_WIDTH-1:0]  S_AXIS_11_tdata,
    input wire                          S_AXIS_11_tvalid,
    input wire [SAXIS_TDATA_WIDTH-1:0]  S_AXIS_12_tdata,
    input wire                          S_AXIS_12_tvalid,
    input wire [SAXIS_TDATA_WIDTH-1:0]  S_AXIS_13_tdata,
    input wire                          S_AXIS_13_tvalid,
    input wire [SAXIS_TDATA_WIDTH-1:0]  S_AXIS_14_tdata,
    input wire                          S_AXIS_14_tvalid,
    input wire [SAXIS_TDATA_WIDTH-1:0]  S_AXIS_15_tdata,
    input wire                          S_AXIS_15_tvalid,
    input wire [32-1:0]                 axis_selector, // # AXIS S to connect 1...10 to AXIS M 1..4:  M1=# M2=# M3=# M4=# 4 bits each 0..15, 4+4+4+4 = 16 bits
    output wire [MAXIS_TDATA_WIDTH-1:0] M_AXIS_1_tdata,
    output wire                         M_AXIS_1_tvalid,
    output wire [MAXIS_TDATA_WIDTH-1:0] M_AXIS_2_tdata,
    output wire                         M_AXIS_2_tvalid,
    output wire [MAXIS_TDATA_WIDTH-1:0] M_AXIS_3_tdata,
    output wire                         M_AXIS_3_tvalid,
    output wire [MAXIS_TDATA_WIDTH-1:0] M_AXIS_4_tdata,
    output wire                         M_AXIS_4_tvalid,
    output wire [MAXIS_TDATA_WIDTH-1:0] M_AXIS_5_tdata,
    output wire                         M_AXIS_5_tvalid,
    output wire [MAXIS_TDATA_WIDTH-1:0] M_AXIS_6_tdata,
    output wire                         M_AXIS_6_tvalid
    );
   
   /* 
    reg [SAXIS_TDATA_WIDTH-1:0] data1 = 0;
    reg valid1 = 0;
    reg [SAXIS_TDATA_WIDTH-1:0] data2 = 0;
    reg valid2 = 0;
    reg [SAXIS_TDATA_WIDTH-1:0] data3 = 0;
    reg valid3 = 0;
    reg [SAXIS_TDATA_WIDTH-1:0] data4 = 0;
    reg valid4 = 0;
    reg [SAXIS_TDATA_WIDTH-1:0] data5 = 0;
    reg valid5 = 0;
    reg [SAXIS_TDATA_WIDTH-1:0] data6 = 0;
    reg valid6 = 0;
    
// my SELECTOR
// connects input N to output x stream

`define MY_SELECTOR(N, xdata, xvalid) \
    case(N) \
        0: \
        begin \
            xdata  <= S_AXIS_00_tdata; \
            xvalid <= S_AXIS_00_tvalid; \
        end \
        1: \
        begin \
            xdata  <= S_AXIS_01_tdata; \
            xvalid <= S_AXIS_01_tvalid; \
        end \
        2: \
        begin \
            xdata  <= S_AXIS_02_tdata; \
            xvalid <= S_AXIS_02_tvalid; \
        end \
        3: \
        begin \
            xdata  <= S_AXIS_03_tdata; \
            xvalid <= S_AXIS_03_tvalid; \
        end \
        4: \
        begin \
            xdata  <= S_AXIS_04_tdata; \
            xvalid <= S_AXIS_04_tvalid; \
        end \
        5: \
        begin \
            xdata  <= S_AXIS_05_tdata; \
            xvalid <= S_AXIS_05_tvalid; \
        end  \
        6: \
        begin \
            xdata  <= S_AXIS_06_tdata; \
            xvalid <= S_AXIS_06_tvalid; \
        end  \
        7: \
        begin \
            xdata  <= S_AXIS_07_tdata; \
            xvalid <= S_AXIS_07_tvalid; \
        end  \
        8: \
        begin \
            xdata  <= S_AXIS_08_tdata; \
            xvalid <= S_AXIS_08_tvalid; \
        end  \
        9: \
        begin \
            xdata  <= S_AXIS_09_tdata; \
            xvalid <= S_AXIS_09_tvalid; \
        end  \
        10: \
        begin \
            xdata  <= S_AXIS_10_tdata; \
            xvalid <= S_AXIS_10_tvalid; \
        end  \
        11: \
        begin \
            xdata  <= S_AXIS_11_tdata; \
            xvalid <= S_AXIS_11_tvalid; \
        end  \
        12: \
        begin \
            xdata  <= S_AXIS_12_tdata; \
            xvalid <= S_AXIS_12_tvalid; \
        end  \
        13: \
        begin \
            xdata  <= S_AXIS_13_tdata; \
            xvalid <= S_AXIS_13_tvalid; \
        end  \
        14: \
        begin \
            xdata  <= S_AXIS_14_tdata; \
            xvalid <= S_AXIS_14_tvalid; \
        end  \
        15: \
        begin \
            xdata  <= S_AXIS_15_tdata; \
            xvalid <= S_AXIS_15_tvalid; \
        end  \
    endcase \

    always @ (posedge a_clk)
    begin

        `MY_SELECTOR (axis_selector[4-1:0], data1, valid1);
        `MY_SELECTOR (axis_selector[8-1:4], data2, valid2);
        `MY_SELECTOR (axis_selector[12-1:8], data3, valid3);
        `MY_SELECTOR (axis_selector[16-1:12], data4, valid4);
        `MY_SELECTOR (axis_selector[20-1:16], data5, valid5);
        `MY_SELECTOR (axis_selector[24-1:20], data6, valid6);
    end

    assign M_AXIS_1_tdata  = data1;
    assign M_AXIS_1_tvalid = valid1;
    assign M_AXIS_2_tdata  = data2;
    assign M_AXIS_2_tvalid = valid2;
    assign M_AXIS_3_tdata  = data3;
    assign M_AXIS_3_tvalid = valid3;
    assign M_AXIS_4_tdata  = data4;
    assign M_AXIS_4_tvalid = valid4;
    assign M_AXIS_5_tdata  = data5;
    assign M_AXIS_5_tvalid = valid5;
    assign M_AXIS_6_tdata  = data6;
    assign M_AXIS_6_tvalid = valid6;
*/  

// my SELECTOR MUX
// connects input N to output x stream


`define MY_SELECTOR_DATA(N, xdata) \
    assign xdata = N ==  0? S_AXIS_00_tdata \
                 : N ==  1? S_AXIS_01_tdata \
                 : N ==  2? S_AXIS_02_tdata \
                 : N ==  3? S_AXIS_03_tdata \
                 : N ==  4? S_AXIS_04_tdata \
                 : N ==  5? S_AXIS_05_tdata \
                 : N ==  6? S_AXIS_06_tdata \
                 : N ==  7? S_AXIS_07_tdata \
                 : N ==  8? S_AXIS_08_tdata \
                 : N ==  9? S_AXIS_09_tdata \
                 : N == 10? S_AXIS_10_tdata \
                 : N == 11? S_AXIS_11_tdata \
                 : N == 11? S_AXIS_12_tdata \
                 : N == 12? S_AXIS_13_tdata \
                 : N == 13? S_AXIS_13_tdata \
                 : N == 14? S_AXIS_14_tdata \
                 : N == 15? S_AXIS_15_tdata : 0 \
                 

`define MY_SELECTOR_VALID(N, xvalid) \
    assign xvalid = N ==  0? S_AXIS_00_tvalid \
                  : N ==  1? S_AXIS_01_tvalid \
                  : N ==  2? S_AXIS_02_tvalid \
                  : N ==  3? S_AXIS_03_tvalid \
                  : N ==  4? S_AXIS_04_tvalid \
                  : N ==  5? S_AXIS_05_tvalid \
                  : N ==  6? S_AXIS_06_tvalid \
                  : N ==  7? S_AXIS_07_tvalid \
                  : N ==  8? S_AXIS_08_tvalid \
                  : N ==  9? S_AXIS_09_tvalid \
                  : N == 10? S_AXIS_10_tvalid \
                  : N == 11? S_AXIS_11_tvalid \
                  : N == 11? S_AXIS_12_tvalid \
                  : N == 12? S_AXIS_13_tvalid \
                  : N == 13? S_AXIS_13_tvalid \
                  : N == 14? S_AXIS_14_tvalid \
                  : N == 15? S_AXIS_15_tvalid : 0 \
                 

`MY_SELECTOR_DATA (axis_selector[4-1:0], M_AXIS_1_tdata);
`MY_SELECTOR_DATA (axis_selector[8-1:4], M_AXIS_2_tdata);
`MY_SELECTOR_DATA (axis_selector[12-1:8], M_AXIS_3_tdata);
`MY_SELECTOR_DATA (axis_selector[16-1:12], M_AXIS_4_tdata);
`MY_SELECTOR_DATA (axis_selector[20-1:16], M_AXIS_5_tdata);
`MY_SELECTOR_DATA (axis_selector[24-1:20], M_AXIS_6_tdata);

`MY_SELECTOR_VALID (axis_selector[4-1:0], M_AXIS_1_tvalid);
`MY_SELECTOR_VALID (axis_selector[8-1:4], M_AXIS_2_tvalid);
`MY_SELECTOR_VALID (axis_selector[12-1:8], M_AXIS_3_tvalid);
`MY_SELECTOR_VALID (axis_selector[16-1:12], M_AXIS_4_tvalid);
`MY_SELECTOR_VALID (axis_selector[20-1:16], M_AXIS_5_tvalid);
`MY_SELECTOR_VALID (axis_selector[24-1:20], M_AXIS_6_tvalid);


endmodule
