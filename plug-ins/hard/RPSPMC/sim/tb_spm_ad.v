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
    reg [512-1:0] data; // [VAdr], [N, NII, Nrep, Options, Next, dx, dy, dz, du] ** full vector data set block **

    wire [31:0] wx; // vector components
    wire [31:0] wy; // ..
    wire [31:0] wz; // ..
    wire [31:0] wu; // ..
    wire [31:0] wopt;  // section options: FB, ...
    wire [31:0] wsec;  // section count
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
        //                  decii       du        dz        dy        dx     Next       Nrep,   Options,     nii,      N,    [Vadr]
        data = {32'd0001, 160'd0, 32'd0004, 32'd0003, 32'd0064, 32'd0001,  32'd0, 32'd0000,   32'h000, 32'd0010, 32'd0005, 32'd00 }; // 000
        #10 prg=1; #10 prg=0; #10
        data = {32'd0001, 160'd0, -32'd0004, -32'd0003, -32'd0064, -32'd0001,  32'd0, 32'd0000,   32'h000, 32'd020, 32'd005, 32'd01 }; // END
        #10 prg=1; #10 prg=0; #10
        data = {32'd0001, 160'd0, -32'd0000, -32'd0000, -32'd0000, -32'd0000,  32'd0, 32'd0000,   32'h000, 32'd000, 32'd000, 32'd02 }; // END
        #10 prg=1; #10 prg=0; #10

        data = {32'd0000, 160'd0, 32'd0000, 32'd0000, 32'd0000, 32'd0000,  32'd0, 32'd0000,   32'h000, 32'd000, 32'd000, 32'd03 }; #2
        prg=1; #10 prg=0; #10
        data = {32'd0000, 160'd0, 32'd0000, 32'd0000, 32'd0000, 32'd0000,  32'd0, 32'd0000,   32'h000, 32'd000, 32'd000, 32'd04 }; #2
        prg=1; #10 prg=0; #10
        data = {32'd0000, 160'd0, 32'd0000, 32'd0000, 32'd0000, 32'd0000,  32'd0, 32'd0000,   32'h000, 32'd000, 32'd000, 32'd05 }; #2
        prg=1; #10 prg=0; #10
        data = {32'd0000, 160'd0, 32'd0000, 32'd0000, 32'd0000, 32'd0000,  32'd0, 32'd0000,   32'h000, 32'd000, 32'd000, 32'd06 }; #2
        prg=1; #10 prg=0; #10
        data = {32'd0000, 160'd0, 32'd0000, 32'd0000, 32'd0000, 32'd0000,  32'd0, 32'd0000,   32'h000, 32'd000, 32'd000, 32'd07 }; #2
        prg=1; #10 prg=0; #10


        r=0; // release reset to run
        wait (fin);

        #20
        r=1; // reset to hold


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
        data = {32'd0024, 160'd0, 32'd0000, 32'd0000, 32'd0000, 32'd0256,  32'd0, 32'd0000,   32'h001, 32'd128, 32'd010, 32'd00 }; // X+ ==>>
        prg=1; #10 prg=0; #10
        data = {32'd0024, 160'd0, 32'd0000, 32'd0000, 32'd0000, -32'sd0256,  32'd0, 32'd0000,   32'h001, 32'd128, 32'd010, 32'd01 }; // X- <==
        prg=1; #10 prg=0; #10
        data = {32'd0024, 160'd0, 32'd0000, 32'd0000, 32'd0064, 32'd0000,  -32'sd2, 32'd0010,   32'h001, 32'd128, 32'd001, 32'd02 }; // repeat PC-2 10x
        prg=1; #10 prg=0; #10
        data = {32'd0000, 160'd0, 32'd0000, 32'd0000, 32'd0000, 32'd0000,  32'd0, 32'd0000,   32'h000, 32'd000, 32'd000, 32'd03 }; #2
        prg=1; #10 prg=0; #10
        data = {32'd0000, 160'd0, 32'd0000, 32'd0000, 32'd0000, 32'd0000,  32'd0, 32'd0000,   32'h000, 32'd000, 32'd000, 32'd04 }; #2
        prg=1; #10 prg=0; #10
        data = {32'd0000, 160'd0, 32'd0000, 32'd0000, 32'd0000, 32'd0000,  32'd0, 32'd0000,   32'h000, 32'd000, 32'd000, 32'd05 }; #2
        prg=1; #10 prg=0; #10
        data = {32'd0000, 160'd0, 32'd0000, 32'd0000, 32'd0000, 32'd0000,  32'd0, 32'd0000,   32'h000, 32'd000, 32'd000, 32'd06 }; #2
        prg=1; #10 prg=0; #10
        data = {32'd0000, 160'd0, 32'd0000, 32'd0000, 32'd0000, 32'd0000,  32'd0, 32'd0000,   32'h000, 32'd000, 32'd000, 32'd07 }; #2
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
        .vp_set(data), // [VAdr], [N, NII, Nrep, Options, Next, dx, dy, dz, du] ** full vector data set block **
        .x(wx), // vector components
        .y(wy), // ..
        .z(wz), // ..
        .u(wu), // ..
        .options(wopt),  // section options: FB, ...
        .section(wsec),  // section count
        .pause(pause),
        .store_data(sto), // trigger to store data:: 2: full vector header, 1: data sources
        .gvp_finished(fin)      // finished 
);


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
    .M_AXIS_UMON_tdata(mru)
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
    



endmodule

