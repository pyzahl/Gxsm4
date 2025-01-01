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

    parameter AXIS_TDATA_WIDTH = 64;
    parameter signed CR_INIT = 32'h7FFFFFFF;  // ((1<<31)-1)
    parameter signed CI_INIT = 32'h00000000;
    parameter signed ERROR_E_INIT = 64'h3FFFFFFFFFFFFFFF; //4611686018427387903
    reg clock;
    reg signed [31:0] deltasRe;
    reg signed [31:0] deltasIm;
    reg signed [AXIS_TDATA_WIDTH-1:0] deltasReIm;
    wire signed [AXIS_TDATA_WIDTH-1:0] SinCos;
    wire scv;
 
   
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

    reg dma_ready=1;

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

    wire spm_rxv;
    wire spm_ryv;
    wire spm_rzv;
    wire spm_ruv;

    wire spm_mrxv;
    wire spm_mryv;
    wire spm_mrzv;
    wire spm_mruv;

    wire [31:0] spm_Zslope;
    wire spm_Zslopev;


    wire [31:0] spm_mx0;   
    wire spm_mx0v;  
    wire [31:0] spm_my0;   
    wire spm_my0v;  
            
    wire [31:0] spm_mz0;   
    wire spm_mz0v;  
            
    wire [31:0] spm_Zslope;
    wire spm_Zslopev;
            
    wire [31:0] spm_Urm;   
    wire spm_Urmv;  
            
    wire [63:0] SCp;       
    wire SCpv;       


    wire [31:0] mrx; // vector components
    wire [31:0] mry; // ..
    wire [31:0] mrz; // ..
    wire [31:0] mru; // ..
   
    wire rxv;
    wire ryv;
    wire rzv;
    wire ruv;

    wire clkbra;
    wire [32-1:0] dma_tdata;
    wire dma_tvalid;
    wire dma_tlast;
    wire dma_stall;
    

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

    reg [31:0] sig=0;
    reg [63:0] sc=0;
    reg [15:0] ddsn2=30;
    reg [47:0] dphiQ44; 
    //##Configure: LCK DDS IDEAL Freq= 10000 Hz [1073741824, N2=30, M=16384]  Actual f=7629.39 Hz  Delta=-2370.61 Hz  (using power of 2 phase_inc)

    reg [47:0] ddsph=0;

    initial 
    begin       
        tb_ACLK = 1'b0;
    end
    
    //------------------------------------------------------------------------
    // Simple Clock Generator
    //------------------------------------------------------------------------
       
    always begin
        pclk = 1; #1;
        pclk = 0; #1;
        ddsph = ddsph + dphiQ44; 
        sc  = { SinCos[63:32]>>7, SinCos[31:0]>>7 };
        sig = SinCos[63:32];
    end

    always begin
        sclk = 1; #1;
        sclk = 0; #1;
    end


    initial
    begin
    
        $display ("running the tb");
/*
#!/usr/bin/python
import numpy
import math

Q31 = 0x7FFFFFFF # ((1<<31)-1);
Q32 = 0xFFFFFFFF # ((1<<32)-1);
Q44 = 1<<44

def adjust (hz):
    fclk = 125e6
    dphi = 2.*math.pi*(hz/fclk)
    dRe = int (Q31 * math.cos (dphi))
    dIm = int (Q31 * math.sin (dphi))

    print ('fclk: ', fclk, ' Hz')
    print ('dphi = ', dphi, ' rad')
    print ('dphiQ44 = ', round(Q44*dphi/(2*math.pi)))
    print ('deltasRe = ', dRe)
    print ('deltasIm = ', dIm)
    
    return (dRe, dIm)

print (adjust (100))
*/
        // SDB init
        clock = 0;
        
        //fclk:  125000000.0  Hz
        //dphi =  5.026548245743669e-06  rad
        dphiQ44 =  14073749;
        deltasRe =  2147483646;
        deltasIm =  10794;

        
        deltasRe = 2145957801;
        deltasIm = 80939050;
        deltasReIm = { deltasRe, deltasIm };


        // INIT GVP
        r=1; #10 prg=0; #10
        //                  decii                                      du      dz      dy      dx    Next      Nrep,   Options,    nii,      N,    [Vadr]
        vector = {32'd16,  32'd0,  32'd0,  32'd0,  32'd0,  32'd0,  32'd10,  32'd0,  32'd0,  32'd0,  32'd0,  32'd000, 32'hc00801,  32'd02,  32'd4, -32'd0}; // Vector #0
        //vector = {32'd16, -32'd0, -32'd0, -32'd0, -32'd0, -32'd0,  32'd10, -32'd0, -32'd0, -32'd0, -32'd0, -32'd000, 32'hc0801,  32'd02,  32'd16, -32'd0}; // Vector #0
        #10 prg=1; #10 prg=0; #10
        vector = {32'd16, -32'd0, -32'd0, -32'd0, -32'd0, -32'd0, -32'd10, -32'd0, -32'd0, -32'd0, -32'd0, -32'd000, 32'hc00801,  32'd02,  32'd16,  32'd1}; // Vector #1
        #10 prg=1; #10 prg=0; #10
        vector = {32'd16, -32'd0, -32'd0, -32'd0, -32'd0, -32'd0, -32'd0, -32'd0, -32'd0, -32'd0, -32'd0, -32'd000, 32'h1, -32'd0, -32'd0,  32'd2}; // Vector #2
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
        .stall(dma_stall),
        .store_data(sto), // trigger to store data:: 2: full vector header, 1: data sources
        .gvp_finished(fin)      // finished 
);

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
        .M_AXIS_tready(dma_ready),
        .stall(dma_stall)
    );


axis_py_lockin lockin
    (
    .a_clk(pclk),    // clocking up to aclk
    .S_AXIS_SIGNAL_tdata(sig),
    .S_AXIS_SIGNAL_tvalid(1),
    
    // SC Lock-In Reference and controls
    .S_AXIS_SC_tdata(sc),
    .S_AXIS_SC_tvalid(1),
    .S_AXIS_DDS_N2_tdata(ddsn2),
    .S_AXIS_DDS_N2_tvalid(1),
    
    // XY Output
    .M_AXIS_A2_tdata(la2),
    .M_AXIS_A2_tvalid(la2v),
    .M_AXIS_X_tdata(lx),
    .M_AXIS_X_tvalid(lxv),
    .M_AXIS_Y_tdata(ly),
    .M_AXIS_Y_tvalid(lyv),
    
    // S ref out
    .M_AXIS_Sref_tdata(lsref),
    .M_AXIS_Sref_tvalid(lsrv),
    // SDeci ref out
    .M_AXIS_SDref_tdata(lsd),
    .M_AXIS_SDref_tvalid(lsdv)
);    




    

/*
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

/*
axis_spm_control axis_spm_control_tb
(
    .a_clk(pclk),
    .S_AXIS_Xs_tdata(wx),
    .S_AXIS_Xs_tvalid(1),
    .S_AXIS_Ys_tdata(wy),
    .S_AXIS_Ys_tvalid(q),
    .S_AXIS_Zs_tdata(0),
    .S_AXIS_Zs_tvalid(1),
    
    .S_AXIS_Z_tdata(0),
    .S_AXIS_Z_tvalid(1),
    
    .S_AXIS_U_tdata(0),
    .S_AXIS_U_tvalid(1),
    // two future control components using optional (DAC #5, #6) "Motor1, Motor2"
    //input wire [SAXIS_TDATA_WIDTH-1:0]  S_AXIS_M1_tdata,
    //input wire                          S_AXIS_M1_tvalid,
    //input wire [SAXIS_TDATA_WIDTH-1:0]  S_AXIS_M2_tdata,
    //input wire                          S_AXIS_M2_tvalid,
    
    // SC Lock-In Reference and controls
    .S_AXIS_SC_tdata(SinCos),
    .S_AXIS_SC_tvalid(1),
    .modulation_volume(10000), // volume for modulation Q31
    .modulation_target(4), // target signal for mod (#XYZUAB)
    

    // scan rotation (yx=-xy, yy=xx)
    .rotmxx(1), // =cos(alpha)
    .rotmxy(0), // =sin(alpha)

    // slope -- always applied in global XY plane ???
    .slope_x(1000), // SQ28
    .slope_y(1000), // SQ28

    // SCAN OFFSET / POSITION COMPONENTS, ABSOLUTE COORDS
    .x0(0), // vector components
    .y0(0), // ..
    .z0(0), // ..
    .u0(0), // Bias Reference
    .xy_offset_step(100), // @Q31 => Q31 / 120M => [18 sec full scale swin @ step 1 decii = 0]  x RDECI
    .z_offset_step(100), // @Q31 => Q31 / 120M => [18 sec full scale swin @ step 1 decii = 0]  x RDECI

    .M_AXIS1_tdata (spm_rx),  
    .M_AXIS1_tvalid(spm_rxv), 
    .M_AXIS2_tdata (spm_ry),  
    .M_AXIS2_tvalid(spm_ryv), 
    .M_AXIS3_tdata (spm_rz),  
    .M_AXIS3_tvalid(spm_rzv), 
    .M_AXIS4_tdata (spm_ru),  
    .M_AXIS4_tvalid(spm_ruv), 

    .M_AXIS_XSMON_tdata (spm_mrx),  
    .M_AXIS_XSMON_tvalid(spm_mrxv),
    .M_AXIS_YSMON_tdata (spm_mry),  
    .M_AXIS_YSMON_tvalid(spm_mryv),
    .M_AXIS_ZSMON_tdata (spm_mrz),  
    .M_AXIS_ZSMON_tvalid(spm_mrzv),

    .M_AXIS_X0MON_tdata (spm_mx0), 
    .M_AXIS_X0MON_tvalid(spm_mx0v),
    .M_AXIS_Y0MON_tdata (spm_my0),
    .M_AXIS_Y0MON_tvalid(spm_my0v),
    
    .M_AXIS_Z0MON_tdata (spm_mz0),
    .M_AXIS_Z0MON_tvalid(spm_mz0v),
    
  .M_AXIS_Z_SLOPE_tdata (spm_Zslope),
  .M_AXIS_Z_SLOPE_tvalid(spm_Zslopev),
    
  .M_AXIS_UrefMON_tdata (spm_Urm),
  .M_AXIS_UrefMON_tvalid(spm_Urmv),

       .M_AXIS_SC_tdata (SCp),
      .M_AXIS_SC_tvalid (SCpv)
);
*/

//def adjust (hz)
//    fclk = 1e6 
//    dphi = 2.*pi*(hz/fclk)
//    dRe = int (Q31 * cos (dphi))
//    dIm = int (Q31 * sin (dphi))
//    return (dRe, dIm)
//Q31 = 0x7FFFFFFF # ((1<<31)-1);
//Q32 = 0xFFFFFFFF # ((1<<32)-1);
//cr = Q31 # ((1<<32)-1);
//ci = 0;


SineSDB64 SDB_tb
(
    .aclk (pclk),
    .S_AXIS_DELTAS_tdata(deltasReIm), // deltas(Re,Im) vector
    .S_AXIS_DELTAS_tvalid(1),
    .M_AXIS_SC_tdata(SinCos), // (Sine, Cosine) vector
    .M_AXIS_SC_tvalid(SCtv)
);

endmodule

