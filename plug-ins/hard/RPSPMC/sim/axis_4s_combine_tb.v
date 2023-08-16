`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 05/26/2018 05:00:42 PM
// Design Name: 
// Module Name: axis_4s_combine_tb
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


module axis_4s_combine_tb();
    parameter SAXIS_1_DATA_WIDTH  = 16;
    parameter SAXIS_1_TDATA_WIDTH = 32;
    parameter SAXIS_2_DATA_WIDTH  = 32;
    parameter SAXIS_2_TDATA_WIDTH = 32;
    parameter SAXIS_3_DATA_WIDTH  = 48;
    parameter SAXIS_3_TDATA_WIDTH = 48;
    parameter SAXIS_45_DATA_WIDTH  = 24;
    parameter SAXIS_45_TDATA_WIDTH = 24;
    parameter SAXIS_6_DATA_WIDTH   = 32;
    parameter SAXIS_6_TDATA_WIDTH  = 32;
    parameter SAXIS_78_DATA_WIDTH  = 32;
    parameter SAXIS_78_TDATA_WIDTH = 32;
    parameter integer BRAM_DATA_WIDTH = 64;
    parameter integer BRAM_ADDR_WIDTH = 15;
    parameter integer DECIMATED_WIDTH = 32;
    
    reg [63:0] simtime;
    reg clock;

    wire a_clk;
    wire [SAXIS_1_TDATA_WIDTH-1:0]  S_AXIS1_tdata;
    wire                          S_AXIS1_tvalid;
    wire [SAXIS_2_TDATA_WIDTH-1:0]  S_AXIS2_tdata;
    wire                          S_AXIS2_tvalid;
    wire [SAXIS_3_TDATA_WIDTH-1:0]  S_AXIS3_tdata;
    wire                            S_AXIS3_tvalid;
    wire [SAXIS_45_TDATA_WIDTH-1:0]  S_AXIS4_tdata;
    wire                          S_AXIS4_tvalid;
    wire [SAXIS_45_TDATA_WIDTH-1:0]  S_AXIS5_tdata;
    wire                          S_AXIS5_tvalid;
    wire [SAXIS_78_TDATA_WIDTH-1:0]  S_AXIS7_tdata; // FIR Ampl
    wire                          S_AXIS7_tvalid;
    wire [SAXIS_78_TDATA_WIDTH-1:0]  S_AXIS8_tdata; // FIR Phase
    wire                          S_AXIS8_tvalid;
    wire signed [SAXIS_3_DATA_WIDTH-1:0] axis3_center;  
    wire [7:0]     rp_digital_in;
    wire [32-1:0]  operation; // 0..7 control bits 0: 1=start 1: 1=single shot/0=fifo loop 2: 4: 1=init/reset , [31:8] DATA ACCUMULATOR (64) SHR
    wire [32-1:0]  ndecimate;
    wire [32-1:0]  nsamples;
    wire [32-1:0]  channel_selector;
    wire          finished_state;
    wire          init_state;
    wire [32-1:0] writeposition;
    wire [32-1:0] debug;
    wire [64-1:0]  M_AXIS_aux_tdata;
    wire           M_AXIS_aux_tvalid;
    wire                        bram_porta_clk;
    wire [BRAM_ADDR_WIDTH-1:0]  bram_porta_addr;
    wire [BRAM_DATA_WIDTH-1:0]  bram_porta_wrdata;
    wire                        bram_porta_en;
    wire                        bram_porta_we;

    reg  tv;
    reg  signed [SAXIS_1_TDATA_WIDTH-1:0]d1;
    reg  signed [SAXIS_2_TDATA_WIDTH-1:0]d2;
    reg  signed [SAXIS_3_TDATA_WIDTH-1:0]d3;
    reg  signed [SAXIS_45_TDATA_WIDTH-1:0]d4;
    reg  signed [SAXIS_45_TDATA_WIDTH-1:0]d5;
    reg  signed [SAXIS_78_TDATA_WIDTH-1:0]d7;
    reg  signed [SAXIS_78_TDATA_WIDTH-1:0]d8;
    reg  signed [SAXIS_3_DATA_WIDTH-1:0] center;
    reg [7:0]     d_in;
    reg [32-1:0]  op; // 0..7 control bits 0: 1=start 1: 1=single shot/0=fifo loop 2: 4: 1=init/reset , [31:8] DATA ACCUMULATOR (64) SHR
    reg [32-1:0]  ndeci;
    reg [32-1:0]  ns;
    reg [32-1:0]  ch_s;

    reg [32-1:0] shr;
    reg updn=0;

    axis_4s_combine Axis_4S_Combine (
        .a_clk(clock),
        .S_AXIS1_tdata(d1),
        .S_AXIS1_tvalid(tv),
        .S_AXIS2_tdata(d2),
        .S_AXIS2_tvalid(tv),
        .S_AXIS3_tdata(d3),
        .S_AXIS3_tvalid(tv),
        .S_AXIS4_tdata(d4),
        .S_AXIS4_tvalid(tv),
        .S_AXIS5_tdata(d5),
        .S_AXIS5_tvalid(tv),
        .S_AXIS7_tdata(d7),
        .S_AXIS7_tvalid(tv),
        .S_AXIS8_tdata(d8),
        .S_AXIS8_tvalid(tv),
        .axis3_center(center), 
        .rp_digital_in(d_in),
        .operation(op),
        .ndecimate(ndeci),
        .nsamples(ns),
        .channel_selector(ch_s)
        );

   initial begin
        simtime=0;
        clock = 0;
        d1 = 0; // {In1, IN2}
        d2 = 100; // ExecAmpl
        d3 = 202; // Frequency (DDS Phase Inc)
        d4 = 0; // Phase
        d5 = 0; // Amplitude
        d7 = 0; // --
        d8 = 0; // --
        tv = 1;
        
        center = 200; // Frequency-Center (DDS Phase Inc)

        d_in = 0; // Digital In Bits
        
        shr=4<<8;
        op=8+shr; // 0..7 control bits 0: 1=start 1: 2=single shot/0=fifo loop 2: 4: 8=init/reset , [31:8] DATA ACCUMULATOR (64) SHR
        ndeci=16;
        ns=8;
        ch_s=4;  // Mapping AXIS 2,3
        updn = 1;
        
        forever #1 begin 
            clock = ~clock;
            simtime = simtime+1;
            if (simtime < 100) begin 
                op = 16+shr; // 1 2 4 8
            end 
            if (simtime == 100) begin 
                op = 1+2+shr; // Single
//                op = 1+0+shr; // Loop
            end 
            if (simtime == 800) begin 
                op = 16+shr; // Init
            end 
            if (simtime == 810) begin 
                op = 1+2+shr; // Single
            end 
            if (simtime == 1500) begin 
                op = 16+shr; // Init
            end 
            if (simtime == 1510) begin 
                op = 1+0+shr; // Loop
            end 
            if (simtime > 400 && d3 > 200 && updn) begin 
                updn=0;
            end 
            if (simtime > 400 && d3 < -200 && ~updn) begin 
                updn=1;
            end 
            if (simtime > 400 && updn) begin 
                d3 = d3+1;
            end 
            if (simtime > 400 && ~updn) begin 
                d3 = d3-1;
            end 
        end
    end

endmodule

