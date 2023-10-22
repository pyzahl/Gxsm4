`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 08/16/2023 08:44:21 AM
// Design Name: 
// Module Name: tb_spm_ad
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


module tb_spm_ad(

    );

    reg tb_ACLK;
    reg tb_ARESETn;
   
    wire temp_clk;
    wire temp_rstn; 
   
    reg [31:0] read_data;
    wire [3:0] leds;
    reg resp;
    
    reg pclk=0;
    reg sclk=0;
    reg r=1;
    reg prg=0;
    reg pause=0;
    reg [512-1:0] vector; // [VAdr], [N, NII, Nrep, Options, Next, dx, dy, dz, du] ** full vector data set block **

    wire [31:0] wx; // vector components
    wire [31:0] wy; // ..
    wire [31:0] wz; // ..
    wire [31:0] wu; // ..
    wire [47:0] gvp_time;
    wire [31:0] gvp_index;
    wire [31:0] wopt;  // section options: FB, ...
    wire [1:0] sto; // trigger to store data:: 2: full vector header, 1: data sources
    wire fin;  // finished 

    wire [31:0] spm_rx; // vector components
    wire [31:0] spm_ry; // ..
    wire [31:0] spm_rz; // ..
    wire [31:0] spm_ru; // ..
    wire [31:0] spm_mrx; // vector components
    wire [31:0] spm_mry; // ..
    wire [31:0] spm_mrz; // ..
    wire [31:0] spm_mru; // ..

    wire [31:0] mrx; // vector components
    wire [31:0] mry; // ..
    wire [31:0] mrz; // ..
    wire [31:0] mru; // ..
   
    wire rxv;
    wire ryv;
    wire rzv;
    wire ruv;

    reg [31:0] dac_cfg=0;
    reg dac_cfgv=1;
    
    //inout  [8-1:0] p_iob;
    //inout  [8-1:0] n_iob;
    reg  [8-1:0] p_iob;
    reg  [8-1:0] n_iob;

    reg dac_cmode = 1;
    reg [2:0] dac_axis = 0;
    reg dac_send = 0;

    wire ad_ready;

    initial 
    begin       
        tb_ACLK = 1'b0;
    end
    
    //------------------------------------------------------------------------
    // Simple Clock Generator
    //------------------------------------------------------------------------
    
    always #10 tb_ACLK = !tb_ACLK;
       
    always begin
        pclk = 1; #1;
        pclk = 0; #1;
    end

    always begin
        sclk = 1; #1;
        sclk = 0; #1;
    end


    initial
    begin
    
        $display ("running the tb");


        // INIT GVP
        r=1; #10 prg=0; #10
        //                  decii                                           du      dz      dy      dx    Next      Nrep,   Options,     nii,      N,    [Vadr]
        vector = {32'd04, -32'd0, -32'd0, -32'd0, -32'd0, -32'd0,  32'd998121, -32'd0, -32'd0, -32'd0, -32'd0, -32'd000, 32'hc0801,  32'd03,  32'd256, -32'd0}; // Vector #0
        #10 prg=1; #10 prg=0; #10
        vector = {32'd04, -32'd0, -32'd0, -32'd0, -32'd0, -32'd0, -32'd998830, -32'd0, -32'd0, -32'd0, -32'd0, -32'd000, 32'hc0801,  32'd03,  32'd256,  32'd1}; // Vector #1
        #10 prg=1; #10 prg=0; #10
        vector = {32'd012, -32'd0, -32'd0, -32'd0, -32'd0, -32'd0, -32'd0, -32'd0, -32'd0, -32'd0, -32'd0, -32'd000, 32'h1, -32'd0, -32'd0,  32'd2}; // Vector #2
        #10 prg=1; #10 prg=0; #10
        r=0; // release reset to run
        wait (fin);
        #2000
        r=1; // reset to hold
        #2000
    


        // INIT GVP
        r=1; #10 prg=0; #10
        //                  decii       du        dz        dy        dx     Next       Nrep,   Options,     nii,      N,    [Vadr]
        vector = {32'd016, 160'd0, 32'd0004, 32'd0003, 32'd002, 32'd0001,  32'd0, 32'd0000,   32'h0801, 32'd02, 32'd005, 32'd00 }; // 000 Bias Mask "08xx"
        #10 prg=1; #10 prg=0; #10
        vector = {32'd016, 160'd0, -32'd0004,-32'd0003,-32'd002,-32'd0001, 32'd0, 32'd0000,   32'h0901, 32'd02, 32'd005, 32'd01 }; // END Bias + X mask "09xx"
        #10 prg=1; #10 prg=0; #10
        vector = {32'd016, 160'd0, -32'd0000,-32'd0000,-32'd0000,-32'd0000, 32'd0, 32'd0000,   32'h0000, 32'd00, 32'd000, 32'd02 }; // END
        #10 prg=1; #10 prg=0; #10

        vector = {32'd016, 160'd0, 32'd0000, 32'd0000, 32'd0000, 32'd0000,  32'd0, 32'd0000,   32'h0000, 32'd000, 32'd000, 32'd03 }; #2
        prg=1; #10 prg=0; #10
        vector = {32'd0016, 160'd0, 32'd0000, 32'd0000, 32'd0000, 32'd0000,  32'd0, 32'd0000,   32'h000, 32'd000, 32'd000, 32'd04 }; #2
        prg=1; #10 prg=0; #10
        vector = {32'd0016, 160'd0, 32'd0000, 32'd0000, 32'd0000, 32'd0000,  32'd0, 32'd0000,   32'h000, 32'd000, 32'd000, 32'd05 }; #2
        prg=1; #10 prg=0; #10
        vector = {32'd0016, 160'd0, 32'd0000, 32'd0000, 32'd0000, 32'd0000,  32'd0, 32'd0000,   32'h000, 32'd000, 32'd000, 32'd06 }; #2
        prg=1; #10 prg=0; #10
        vector = {32'd0016, 160'd0, 32'd0000, 32'd0000, 32'd0000, 32'd0000,  32'd0, 32'd0000,   32'h000, 32'd000, 32'd000, 32'd07 }; #2
        prg=1; #10 prg=0; #10

        r=0; // release reset to run
        wait (fin);
        #20
        r=1; // reset to hold
        #20
        r=0; // release reset to run again
        wait (fin);
        #20
        r=1; // reset to hold
        #20

        // INIT GVP
        r=1; #10 prg=0; #10
        //            decii       **     **     **     BB     AA     du     dz     dy     dx       Next    Nrep,  Options, nii,   N,  [Vadr]
        vector = {32'd250, -32'd0, -32'd0, -32'd0, -32'd0, -32'd0,  32'd8589935, -32'd0, -32'd0,  32'd858993, -32'd0, -32'd000, 32'h801,  32'd4,  32'd9, -32'd0}; // Vector #0
        prg=1; #10 prg=0; #10
        vector = {32'd250, -32'd0, -32'd0, -32'd0, -32'd0, -32'd0, -32'd8589935, -32'd0, -32'd0, -32'd858993, -32'd0, -32'd000, 32'h801,  32'd4,  32'd9,  32'd1}; // Vector #1
        prg=1; #10 prg=0; #10
        vector = {32'd0, 32'd0, 32'd0, 32'd0, 32'd0, 32'd0, 32'd0, 32'd0, 32'd0, 32'd0, 32'd0, 32'd0, 32'd1, 32'd0, 32'd0, 32'd2}; // Vector #2
        prg=1; #10 prg=0; #10
        vector = {32'd0, 32'd0, 32'd0, 32'd0, 32'd0, 32'd0, 32'd0, 32'd0, 32'd0, 32'd0, 32'd0, 32'd0, 32'd1, 32'd0, 32'd0, 32'd3}; // Vector #3
        prg=1; #10 prg=0; #10
        vector = {32'd0, 32'd0, 32'd0, 32'd0, 32'd0, 32'd0, 32'd0, 32'd0, 32'd0, 32'd0, 32'd0, 32'd0, 32'd1, 32'd0, 32'd0, 32'd4}; // Vector #4
        prg=1; #10 prg=0; #10

        r=0; // release reset to run
        wait (fin);
        #20
        r=1; // reset to hold
        #20
        r=0; // release reset to run again
        wait (fin);
        #20
        r=1; // reset to hold
        #20

        // TEST AD SERIAL OUT
        dac_cmode = 1; // DACs put in config mode (hold) unless reprogrammed      
        dac_send = 0; // SEND CFG DATA bit
        dac_axis = 0; // DAC AXIS (Channel)
        dac_cfg = 0;  // DAC CFG DATA
        dac_cfgv = 1; // DATA VALID
        #20;

        dac_axis = 3; // ADC0      
        dac_cfg = 128; // =32
        #20;
        dac_axis = 2; // ADC0      
        dac_cfg = 64; // =32
        #10;
        dac_axis = 1; // ADC0      
        dac_cfg = 32; // =32
        #20;
        dac_axis = 0; // ADC0      
        dac_cfg = 16; // =32
        #20;
        dac_send = 1; // Send
        #(20);
        dac_send = 0;      
        #(20);
        wait (ad_ready);  

        #(20);
        dac_cmode = 0;      

        // TEST GVP SCAN LOOP
        r=1; #10  prg=0; #10
        // move to start point
        //         decii                du        dz        dy        dx     Next       Nrep,   Options,     nii,      N,    [Vadr]
        vector = {32'd0024, 160'd0, 32'd0000, 32'd0000, 32'd0000, 32'd0256,  32'd0, 32'd0000,   32'h001, 32'd128, 32'd010, 32'd00 }; // X+ ==>>
        prg=1; #10 prg=0; #10
        vector = {32'd0024, 160'd0, 32'd0000, 32'd0000, 32'd0000, -32'sd0256,  32'd0, 32'd0000,   32'h001, 32'd128, 32'd010, 32'd01 }; // X- <==
        prg=1; #10 prg=0; #10
        vector = {32'd0024, 160'd0, 32'd0000, 32'd0000, 32'd0064, 32'd0000,  -32'sd2, 32'd0010,   32'h001, 32'd128, 32'd001, 32'd02 }; // repeat PC-2 10x
        prg=1; #10 prg=0; #10
        vector = {32'd0000, 160'd0, 32'd0000, 32'd0000, 32'd0000, 32'd0000,  32'd0, 32'd0000,   32'h000, 32'd000, 32'd000, 32'd03 }; #2
        prg=1; #10 prg=0; #10
        vector = {32'd0000, 160'd0, 32'd0000, 32'd0000, 32'd0000, 32'd0000,  32'd0, 32'd0000,   32'h000, 32'd000, 32'd000, 32'd04 }; #2
        prg=1; #10 prg=0; #10
        vector = {32'd0000, 160'd0, 32'd0000, 32'd0000, 32'd0000, 32'd0000,  32'd0, 32'd0000,   32'h000, 32'd000, 32'd000, 32'd05 }; #2
        prg=1; #10 prg=0; #10
        vector = {32'd0000, 160'd0, 32'd0000, 32'd0000, 32'd0000, 32'd0000,  32'd0, 32'd0000,   32'h000, 32'd000, 32'd000, 32'd06 }; #2
        prg=1; #10 prg=0; #10
        vector = {32'd0000, 160'd0, 32'd0000, 32'd0000, 32'd0000, 32'd0000,  32'd0, 32'd0000,   32'h000, 32'd000, 32'd000, 32'd07 }; #2
        prg=1; #10 prg=0; #10

        r=0; // release reset to run
        wait (fin);

        #20
        r=1; // reset to hold

        $display ("Simulation 2 completed");
        $stop;
    end


    assign temp_clk = tb_ACLK;
    assign temp_rstn = tb_ARESETn;




gvp gvp_1
    (
        .a_clk(pclk),    // clocking up to aclk
        .reset(r),  // put into reset mode (hold)
        .setvec(prg), // program vector data using vp_set data
        .vp_set(vector), // [VAdr], [N, NII, Nrep, Options, Next, dx, dy, dz, du] ** full vector data set block **
        .reset_options(1), // option (fb hold/go, srcs... to pass when idle or in reset mode
        .M_AXIS_X_tdata(wx), // vector components
        .M_AXIS_Y_tdata(wy), // ..
        .M_AXIS_Z_tdata(wz), // ..
        .M_AXIS_U_tdata(wu), // ..
        .M_AXIS_index_tdata(gvp_index),
        .M_AXIS_gvp_time_tdata(gvp_time),
        .options(wopt),  // section options: FB, ...
        .pause(pause),
        .store_data(sto), // trigger to store data:: 2: full vector header, 1: data sources
        .gvp_finished(fin)      // finished 
);

wire clkbra;
wire [32-1:0] dma_tdata;
wire dma_tvalid;
wire dma_tlast;

axis_bram_stream_srcs axis_bram_stream_srcs_tb
(                             // CH      MASK
        .S_AXIS_ch1s_tdata(wx), // XS      0x0001  X in Scan coords
        .S_AXIS_ch2s_tdata(wy), // YS      0x0002  Y in Scan coords
        .S_AXIS_ch3s_tdata(wz), // ZS      0x0004  Z
        .S_AXIS_ch4s_tdata(wu), // U       0x0008  Bias
        .S_AXIS_ch5s_tdata(5), // IN1     0x0010  IN1 RP (Signal)
        .S_AXIS_ch6s_tdata(6), // IN2     0x0020  IN2 RP (Current)
        .S_AXIS_ch7s_tdata(7), // IN3     0x0040  reserved, N/A at this time
        .S_AXIS_ch8s_tdata(8), // IN4     0x0080  reserved, N/A at this time
        .S_AXIS_ch9s_tdata(9), // DFREQ   0x0100  via PACPLL FIR
        .S_AXIS_chAs_tdata(10), // EXEC    0x0200  via PACPLL FIR
        .S_AXIS_chBs_tdata(11), // PHASE   0x0400  via PACPLL FIR
        .S_AXIS_chCs_tdata(12), // AMPL    0x0800  via PACPLL FIR
        .S_AXIS_chDs_tdata(13), // LockInA 0x1000  LockIn X (ToDo)
        .S_AXIS_chEs_tdata(14), // LockInB 0x2000  LocKin R (ToDo)
        .S_AXIS_gvp_time_tdata(gvp_time),  // time since GVP start in 1/125MHz units
        .S_AXIS_srcs_tdata(wopt),      // data selection mask and options
        .S_AXIS_index_tdata(gvp_index),     // index starting at N-1 down to 0
        .push_next(sto), // frame header/data point trigger control
	    .reset(r),
        .a2_clk(pclk), // double a_clk used for BRAM (125MHz)
        .dma_data_clk(clkbbra),
        .M_AXIS_tdata(dma_tdata),
        .M_AXIS_tvalid(dma_tvalid),
        .M_AXIS_tlast(dma_tlast),
        .M_AXIS_tready(1)
    );

/*
blk_mem_gen_spmc bram_store_inst (
  .clka(clkbra),    // input wire clka
  .wea(wr_en),      // input wire [0 : 0] wea
  .addra(addr_wr),  // input wire [13 : 0] addra
  .dina(data_wr),    // input wire [31 : 0] dina
  .clkb(clk),    // input wire clkb
  .enb(rd_en),      // input wire enb
  .addrb(addr_rd),  // input wire [13 : 0] addrb
  .doutb(data_rd)  // output wire [31 : 0] doutb
);
*/

/*
axis_spm_control axis_spm_control_1
(
    .rotmxy(0),
    .rotmxx(1),

    .slope_x(0),
    .slope_y(0),
    
    .S_AXIS_Z_tdata(0),
    .S_AXIS_Z_tvalid(1),
    // SCAN COMPONENTS, ROTATED RELATIVE COORDS
    .xs(wx), // vector components
    .ys(wy), // ..
    .zs(wz), // ..
    .u(wu), // ..

    // SCAN POSITION COMPONENTS, ABSOLUTE COORDS
    .x0(0), // vector components
    .y0(0),
    .z0(0),

    .a_clk(pclk),
    .M_AXIS1_tdata(spm_rx),
    .M_AXIS1_tvalid(spm_rxv),
    .M_AXIS2_tdata(spm_ry),
    .M_AXIS2_tvalid(spm_ryv),
    .M_AXIS3_tdata(spm_rz),
    .M_AXIS3_tvalid(spm_rzv),
    .M_AXIS4_tdata(spm_ru),
    .M_AXIS4_tvalid(spm_ruv),


    .M_AXIS_XSMON_tdata(mrx),
    //.M_AXIS_XSMON_tvalid,
    .M_AXIS_YSMON_tdata(mry),
    //.M_AXIS_YSMON_tvalid,

    //.M_AXIS_XMON_tdata,
    //.M_AXIS_XMON_tvalid,
    //.M_AXIS_YMON_tdata,
    //.M_AXIS_YMON_tvalid,
    //.M_AXIS_ZMON_tdata,
    //.M_AXIS_ZMON_tvalid,
    //.M_AXIS_UMON_tvalid

    );


    


axis_AD5791 axis_AD5791_1 
(
    .a_clk(pclk),
    .S_AXIS1_tdata(spm_rx),
    .S_AXIS1_tvalid(spm_rxv),
    .S_AXIS2_tdata(spm_ry),
    .S_AXIS2_tvalid(spm_ryv),
    .S_AXIS3_tdata(spm_rz),
    .S_AXIS3_tvalid(spm_rzv),
    .S_AXIS4_tdata(spm_ru),
    .S_AXIS4_tvalid(spm_ruv),

    .S_AXISCFG_tdata(dac_cfg),
    .S_AXISCFG_tvalid(dac_cfgv),

    .configuration_mode(dac_cmode),
    .configuration_axis(dac_axis),
    .configuration_send(dac_send),
    
    .ready(ad_ready)
    //.exp_p_io(p_iob),
    //.exp_n_io(n_iob)
    );
    
*/


endmodule

