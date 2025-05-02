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

    parameter gvp_control_reg_address = 5001;
    parameter gvp_reset_options_reg_address = 5002;
    parameter gvp_vector_programming_reg_address  = 5003;
    parameter gvp_vector_set_reg_address  = 5004;
    parameter gvp_vectorX_programming_reg_address  = 5005;
    //parameter gvp_vectorX_programming_reg_address  = 500;


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
    reg [31:0] confaddr;
    reg [512-1:0] vector; // [VAdr], [N, NII, Nrep, Options, Next, dx, dy, dz, du] ** full vector data set block **

    reg dma_ready=1;

    reg [16:0] spi_waits=(2*32+10)*32;

    wire [31:0] xvalue;
    wire [3:0] Xgvpch;




    wire [31:0] wx; // vector components
    wire [31:0] wy; // ..
    wire [31:0] wz; // ..
    wire [31:0] wu; // ..
    wire [31:0] wa; // ..
    wire [31:0] wb; // ..
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

    reg [64:0] clicks=0;
    reg [15:0] amptest=4;





reg [31:0] mux00td;    
reg        mux00tv;   
reg [31:0] mux01td;    
reg        mux01tv;   
reg [31:0] mux02td;    
reg        mux02tv;   
reg [31:0] mux03td;    
reg        mux03tv;   
reg [31:0] mux04td;    
reg        mux04tv;   
reg [31:0] mux05td;    
reg        mux05tv;   
reg [31:0] mux06td;    
reg        mux06tv;   
reg [31:0] mux07td;    
reg        mux07tv;   
reg [31:0] mux08td;    
reg        mux08tv;   
reg [31:0] mux09td;    
reg        mux09tv;   
reg [31:0] mux10td;    
reg        mux10tv;   
reg [31:0] mux11td;    
reg        mux11tv;   
reg [31:0] mux12td;    
reg        mux12tv;   
reg [31:0] mux13td;    
reg        mux13tv;   
reg [31:0] mux14td;    
reg        mux14tv;   
reg [31:0] mux15td;    
reg        mux15tv; 
              
wire [31:0] mux_out01td; 
wire        mux_out01tv;
wire [31:0] mux_out02td;
wire        mux_out02tv;
wire [31:0] mux_out03td;
wire        mux_out03tv;
wire [31:0] mux_out04td;
wire        mux_out04tv;
wire [31:0] mux_out05td;
wire        mux_out05tv;
wire [31:0] mux_out06td;
wire        mux_out06tv;

wire [31:0]FIR_wu;
wire       FIR_twu;
wire [31:0]FIR2_wu;
wire       FIR2_twu;


wire [31:0] ad_ch1;
wire ad_ch1v;
wire [31:0] ad_ch2;
wire ad_ch2v;
    
wire w_spi_sck;
wire w_spi_cs;
wire w_spi_sdi;
wire w_spi_reset;
wire w_spi_cnv;
reg  w_spi_busy=0;
reg [7:0] w_spi_sdn=8'hff;

wire [31:0] ad_mon0;
wire [31:0] ad_mon1;

wire [7:0] ad_bt_dbg;
wire ad_ready;


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
        sc  = { SinCos[63:32]>>>7, SinCos[31:0]>>>7 };
        
        clicks <= clicks+1;
        
        if (clicks > 331072)
            amptest <= 5;
        sig = $signed(SinCos[63:32])>>>amptest;
    end

    always begin
        sclk = 1; #1;
        sclk = 0; #1;
    end
        
    always begin
        w_spi_busy <= ad_bt_dbg > 8'hf0 ? 1:0; #2;
    end


    reg  signed [31:0] servo_set =40000000;
    reg  signed [31:0] servo_setP=40000010;
    reg  signed [31:0] servo_setM=39999990;
    reg  signed [31:0] servo_sig =40000000;
    reg  signed [31:0] servo_in  =40000000;
    reg  signed [31:0] servo_d   =2;
    reg         servo_in_tvalid=1;
    wire signed [31:0] servo_ctrl=0;   

    // PI Controller aka Servo test
    reg  [15:0] sdec=0; 
    always begin
        if (sdec==0)
        begin
            sdec <= 10;
            if (servo_sig > servo_setP)
                servo_d <= -1;
            if (servo_sig < servo_setM)
                servo_d <= 1;
            servo_sig <= servo_sig + servo_d;
            servo_in_tvalid <= 1;
        end           
        else
        begin
            servo_in_tvalid <= 0;
            sdec <= sdec-1;
        end
        servo_in <= servo_sig;
        #1;
    end




    always begin
        mux00td <= wx;
        mux01td <= wy;
        mux02td <= wz;
        mux03td <= wu;
        mux04td <= wa;
        mux05td <= wb;
        mux06td <= wb;
        mux07td <= wb;
        mux08td <= wb;
        mux09td <= wb;
        mux10td <= wb;
        mux11td <= wb;
        mux12td <= wb;
        mux13td <= wb;
        mux14td <= wb;
        mux15td <= wb;
        mux00tv <= 1;
        mux01tv <= 1;
        mux02tv <= 1;
        mux03tv <= 1;
        mux04tv <= 1;
        mux05tv <= 1;
        mux06tv <= 1;
        mux07tv <= 1;
        mux08tv <= 1;
        mux09tv <= 1;
        mux10tv <= 1;
        mux11tv <= 1;
        mux12tv <= 1;
        mux13tv <= 1;
        mux14tv <= 1;
        mux15tv <= 1;
        #1;
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
        //dphiQ44 =  17179869184 ; // @Q44 phase
        
        //deltasRe =  2147443221 ; // SDB
        //deltasIm =  13176711 ; // SDB
        //ddsn2   =  34 ;

        dphiQ44 =  134217728 ; // @Q44 phase
        ddsn2   =  27 ;
        deltasRe =  2147483644 ; // SDB
        deltasIm =  102943 ; // SDB
        

        deltasReIm = { deltasRe, deltasIm };

        // MODUL CONFIG
        confaddr=0; #10 vector = 123456789;#1 confaddr=99999; #10 confaddr=0; #10 // module config cycle

        // MUX SETUP
        confaddr=0; #10 vector = 32'h00543021; #1 confaddr=2000; #10 confaddr=0; #10 // module config cycle
        //parameter gvp_control_reg_address = 5001;
        //parameter gvp_reset_options_reg_address = 5002;
        //parameter gvp_vector_programming_reg_address  = 5003;
        //parameter gvp_vector_set_reg_address  = 5004;
        //parameter gvp_vectorX_programming_reg_address  = 500;

        // INIT AD463x
        // RESET MARK
        vector = {32'd8, 32'd0,  32'h0000000, 32'h00 }; #1; confaddr=50000; #10; confaddr=0; #50 // make sure reset deasserted
        vector = {32'd8, 32'd0,  32'h0000000, 32'h80 }; #1; confaddr=50000; #10; confaddr=0; #50 // assert RESET
        vector = {32'd8, 32'd0,  32'h0000000, 32'h00 }; #1; confaddr=50000; #10; confaddr=0; #50 // release RESET

        #100;


        // ENTER CONF MODE WRITE -- INIT  W: { 1'b0, 0x7FFF (Address 16'b) , 0x00 (data 8'b)}
        vector = {32'd3, 32'd24, 32'h00a00000, 32'h03 }; #1; confaddr=50000; #10; confaddr=0; #spi_waits // let SPI finish
        // READ TIMING 0x2000
        vector = {32'd3, 32'd24, 32'h00A000aa, 32'h03 }; #1; confaddr=50000; #10; confaddr=0; #spi_waits // let SPI finish
        // WRITE SCRATCHPAD 0x000A
        vector = {32'd3, 32'd24, 32'h00000Aaa, 32'h03 }; #1; confaddr=50000; #10; confaddr=0; #spi_waits // let SPI finish
        // READ SCRATCHPAD 0x000A
        vector = {32'd3, 32'd24, 32'h00800Aaa, 32'h03 }; #1; confaddr=50000; #10; confaddr=0; #spi_waits // let SPI finish

        // WRITE MODES 0x0020
        //vector = {32'd8, 32'd24, 32'h00000Aaa, 32'h03 }; #1; confaddr=50000; #10; confaddr=0; #spi_waits // let SPI finish

        // CNV, READ SINGLE SAMPLE
        // CNV MANUAL 8    STREAM-READ 4    AXI 0x10
        //vector = {32'd3, 32'd32, 32'h00000000, 32'h09 }; #1; confaddr=50000; #10; confaddr=0; #spi_waits // let SPI finish
        vector = {32'd3, 32'd24, 32'h00000000, 32'h09 }; #1; confaddr=50000; #10; confaddr=0; #spi_waits // let SPI finish
        //vector = {32'd3, 32'd24, 32'h00000000, 32'h0d }; #1; confaddr=50000; #10; confaddr=0; #spi_waits // let SPI finish
        //vector = {32'd3, 32'd24, 32'h00000000, 32'h0d }; #1; confaddr=50000; #10; confaddr=0; #spi_waits // let SPI finish
        //vector = {32'd3, 32'd32, 32'h00000000, 32'h0d }; #1; confaddr=50000; #10; confaddr=0; #spi_waits // let SPI finish
        //vector = {32'd3, 32'd32, 32'h00000000, 32'h0d }; #1; confaddr=50000; #10; confaddr=0; #spi_waits // let SPI finish
        //vector = {32'd3, 32'd24, 32'h00000000, 32'h10 }; #1; confaddr=50000; #10; confaddr=0; #spi_waits // let SPI finish
        vector = {32'd3, 32'd24, 32'h00000000, 32'h14 }; #1; confaddr=50000; #10; confaddr=0; #spi_waits // let SPI finish
        //vector = {32'd3, 32'd24, 32'h00000000, 32'h14 }; #1; confaddr=50000; #10; confaddr=0; #spi_waits // let SPI finish
        //vector = {32'd3, 32'd32, 32'h00000000, 32'h14 }; #1; confaddr=50000; #10; confaddr=0; #spi_waits // let SPI finish
        //vector = {32'd3, 32'd32, 32'h00000000, 32'h14 }; #1; confaddr=50000; #10; confaddr=0; #spi_waits // let SPI finish
        #10000;
        vector = {32'd1, 32'd24, 32'h00000000, 32'h14 }; #1; confaddr=50000; #10; confaddr=0; #spi_waits // let SPI finish
        #10000;
        vector = {32'd2, 32'd24, 32'h00000000, 32'h14 }; #1; confaddr=50000; #10; confaddr=0; #spi_waits // let SPI finish
        #10000;
        vector = {32'd3, 32'd24, 32'h00000000, 32'h14 }; #1; confaddr=50000; #10; confaddr=0; #spi_waits // let SPI finish
        #10000;
        vector = {32'd4, 32'd24, 32'h00000000, 32'h14 }; #1; confaddr=50000; #10; confaddr=0; #spi_waits // let SPI finish
        #10000;

        // RESET MARK
        vector = {32'd8, 32'd0,  32'h00000000, 32'h00 }; #1; confaddr=50000; #10; confaddr=0; #50 // make sure reset deasserted
        vector = {32'd8, 32'd0,  32'h00000000, 32'h80 }; #1; confaddr=50000; #10; confaddr=0; #30 // assert RESET
        vector = {32'd8, 32'd0,  32'h00000000, 32'h00 }; #1; confaddr=50000; #10; confaddr=0; #50 // release RESET
        // ====

        // CONF MODE WRITE -- INIT  W: { 1'b0, 0x7FFF (Address 16'b) , 0x00 (data 8'b)}
        vector = {32'd3, 32'd24, 32'h00a00001, 32'h03 }; #1; confaddr=50000; #10; confaddr=0; #spi_waits // let SPI finish

       // CONF MODE READ TEST a0a000
        vector = {32'd3, 32'd24,  32'h0001a000, 32'h03 }; #1; confaddr=50000; #10; confaddr=0; #spi_waits // let SPI finish

        // CNV, READ SINGLE SAMPLE
        // CNV MANUAL 8    STREAM-READ 4    AXI 0x10
        vector = {32'd3, 32'd24,  32'h00000, 32'h14 }; #1; confaddr=50000; #10; confaddr=0; #spi_waits // module config cycle done
        #500;
        vector = {32'd3, 32'd24,  32'h00000, 32'h14 }; #1; confaddr=50000; #10; confaddr=0; #spi_waits // module config cycle done
        #500;

        vector = {32'd3, 32'd32,  32'h00000, 32'h14 }; #1; confaddr=50000; #10; confaddr=0; #spi_waits // module config cycle done
        #1500;
        vector = {32'd3, 32'd32,  32'h00000, 32'h14 }; #1; confaddr=50000; #10; confaddr=0; #spi_waits // module config cycle done
        #1500;

        vector = {32'd3, 32'd24,  32'h00000, 32'h14 }; #1; confaddr=50000; #10; confaddr=0; #spi_waits // module config cycle done
        #1500;
        vector = {32'd3, 32'd24,  32'h00000, 32'h14 }; #1; confaddr=50000; #10; confaddr=0; #spi_waits // module config cycle done

        #1500;

        // AXI STREAMING
        vector = {32'd3, 32'd24,  32'h00000, 32'h14 }; #1; confaddr=50000; #10; confaddr=0; #spi_waits // module config cycle done
        #1500;


        // **********************

        // INIT GVP
        confaddr=0; #10 vector = 1;#1 confaddr=gvp_control_reg_address; #10 confaddr=0; #10 // reset to GVP

        //         decii                               db       da       du       dz       dy       dx     Next     Nrep,    Options,    nii,      N,    [Vadr]
        vector = {32'd16,  32'd0,  32'd0,  32'd0,  32'd30,  32'd20,  32'd10,  32'd03,  32'd02,  32'd01,  32'd00, 32'd000, 32'hc00801,  32'd02,  32'd20,   32'd0}; // Vector #0
        //vector = {32'd16, -32'd0, -32'd0, -32'd0, -32'd0, -32'd0,  32'd10, -32'd0, -32'd0, -32'd0, -32'd0, -32'd000, 32'hc0801,  32'd02,  32'd16, -32'd0}; // Vector #0
        #10 confaddr=gvp_vector_programming_reg_address; #10 confaddr=0; #10; // vec-set
        //
        vector = {32'd16,  32'd0,  32'd0,  32'd0,  32'd01,  32'd02, -32'd10,  -32'd03,  32'd00,  -32'd01, -32'd01, 32'd005, 32'hc00881,  32'd02,  32'd20,  32'd1}; // Vector #1
        #10 confaddr=gvp_vector_programming_reg_address; #10 confaddr=0; #10; // vec-set
        vector = {32'd0,  32'd0,  32'd0,  32'd0,  32'd0,  32'd0, 32'd0,  32'd0,  32'd0,  32'd0, 32'd0, 32'd0, 32'd0,  32'h22,  32'd2,  32'd1}; // VectorX #1   XXXXXXXXXXXXXXXXX
        #10 confaddr=gvp_vectorX_programming_reg_address; #10 confaddr=0; #10; // vec-set
        //
        vector = {32'd16,  32'd0,  32'd0,  32'd0,  32'd30,  32'd20,  32'd20,  -32'd04,  32'd02,  32'd01,  32'd00, 32'd000, 32'hc00801,  32'd02,  32'd20,   32'd2}; // Vector #2
        #10 confaddr=gvp_vector_programming_reg_address; #10 confaddr=0; #10; // vec-set
        //
        vector = {32'd16,  32'd0,  32'd0,  32'd0,  32'd00,  32'd00,  32'd00,  32'd00,  32'd00,  32'd00,  32'd00, 32'd000, 32'h000000,  32'd00,  32'h00,  32'd3}; // END
        #10 confaddr=gvp_vector_programming_reg_address; #10 confaddr=0; #10 // vec-set
        vector = {32'd16,  32'd0,  32'd0,  32'd0,  32'd00,  32'd00,  32'd00,  32'd00,  32'd00,  32'd00,  32'd00, 32'd000, 32'h000000,  32'd00,  32'h00,  32'd4}; // 000
        #10 confaddr=gvp_vector_programming_reg_address; #10 confaddr=0; #10 // vec-set

        #10 confaddr=0; #10 vector = 0;#1 confaddr=gvp_control_reg_address; #10 confaddr=0; #10 // run to GVP (out of reset)
        wait (fin);
        #2000
        #10 confaddr=0; #10 vector = 2;#1 confaddr=gvp_control_reg_address; #10 confaddr=0; #10 // pause to GVP
        #2000
        #10 confaddr=0; #10 vector = 1;#1 confaddr=gvp_control_reg_address; #10 confaddr=0; #10 // reset to GVP


        // INIT GVP
        #10 confaddr=0; #10 vector = 1; #1 confaddr=gvp_control_reg_address; #10 confaddr=0; #10 // reset to GVP
        //         decii                              db       da       du       dz       dy       dx    Next      Nrep,   Options,    nii,      N,    [Vadr]
        vector = {32'd16,  32'd0,  32'd0,  32'd0,  32'd30,  32'd20,  32'd10,  32'd03,  32'd02,  32'd01,  32'd0,  32'd000, 32'hc00801,  32'd02,  32'd20, 32'd0}; // Vector #0
        #10 confaddr=gvp_vector_programming_reg_address; #10 confaddr=0; #10; // vec-set
        vector = {32'd16,  32'd0,  32'd0,  32'd0, -32'sd30, -32'sd20, -32'sd10, -32'sd03, -32'd02, -32'sd01, -32'sd1,  32'd002, 32'hc00801,  32'd02,  32'd20, 32'd1}; // Vector #0
        #10 confaddr=gvp_vector_programming_reg_address; #10 confaddr=0; #10; // vec-set
        vector = {32'd16,  32'd0,  32'd0,  32'd0,  32'd00,  32'd00,  32'd00,  32'd00,  32'd00,  32'd00,  32'd00, 32'd000, 32'h000000,  32'd00,  32'h00, 32'd2}; // END
        #10 confaddr=gvp_vector_programming_reg_address; #10 confaddr=0; #10; // vec-set
        vector = {32'd16,  32'd0,  32'd0,  32'd0,  32'd00,  32'd00,  32'd00,  32'd00,  32'd00,  32'd00,  32'd00, 32'd000, 32'h000000,  32'd00,  32'h00, 32'd3}; // 000
        #10 confaddr=gvp_vector_programming_reg_address; #10 confaddr=0; #10 // vec-set

        #10 vector = 0; #1 confaddr=gvp_control_reg_address; #10 confaddr=0; #10 // run to GVP (out of reset)
        #2000

        #10 vector = 1;#1 confaddr=gvp_control_reg_address; #10 confaddr=0; #10 // reset to GVP
        wait (fin);

        // INIT GVP
        vector = 1;#1 confaddr=gvp_control_reg_address; #10 confaddr=0; #10 // reset to GVP
        //         decii                              db       da       du         dz       dy       dx        Next      Nrep,   Options,    nii,    N,   [Vadr]
        vector = {32'd250, -32'd0, -32'd0, -32'd0, -32'd0, -32'd0,  32'd8589935, -32'd0, -32'd0,  32'd858993, -32'd0, -32'd000, 32'h801,  32'd4,  32'd9, -32'd0}; // Vector #0
        confaddr=gvp_vector_programming_reg_address;#10 confaddr=0; #10; // vec-set
        vector = {32'd250, -32'd0, -32'd0, -32'd0, -32'd0, -32'd0, -32'sd16589935, -32'd0, -32'd0, -32'sd858993, -32'd0, -32'd000, 32'h801,  32'd4,  32'd9,  32'd1}; // Vector #1
        confaddr=gvp_vector_programming_reg_address;#10 confaddr=0; #10; // vec-set
        vector = {32'd0, 32'd0, 32'd0, 32'd0, 32'd0, 32'd0, 32'd0, 32'd0, 32'd0, 32'd0, 32'd0, 32'd0, 32'd1, 32'd0, 32'd0, 32'd2}; // Vector #2
        confaddr=gvp_vector_programming_reg_address;#10 confaddr=0; #10; // vec-set
        vector = {32'd0, 32'd0, 32'd0, 32'd0, 32'd0, 32'd0, 32'd0, 32'd0, 32'd0, 32'd0, 32'd0, 32'd0, 32'd1, 32'd0, 32'd0, 32'd3}; // Vector #3
        confaddr=gvp_vector_programming_reg_address;#10 confaddr=0; #10; // vec-set
        vector = {32'd0, 32'd0, 32'd0, 32'd0, 32'd0, 32'd0, 32'd0, 32'd0, 32'd0, 32'd0, 32'd0, 32'd0, 32'd1, 32'd0, 32'd0, 32'd4}; // Vector #4
        confaddr=gvp_vector_programming_reg_address;#10 confaddr=0; #10; // vec-set

        confaddr=gvp_vector_programming_reg_address;#10 confaddr=0; #10; // vec-set

        vector = 0;#1 confaddr=gvp_control_reg_address; #10 confaddr=0; #10 // run to GVP (out of reset)
        #2000
        wait (fin);

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
        vector = {32'd0000, 1600'd0, 32'd0000, 32'd0000, 32'd0000, 32'd0000,  32'd0, 32'd0000,   32'h000, 32'd000, 32'd000, 32'd06 }; #2
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
        .config_addr(confaddr),
        .config_data(vector),
        //.reset(r),  // put into reset mode (hold)
        //.setvec(prg), // program vector data using vp_set data
        //.vp_set(vector), // [VAdr], [N, NII, Nrep, Options, Next, dx, dy, dz, du] ** full vector data set block **
        //.reset_options(1), // option (fb hold/go, srcs... to pass when idle or in reset mode
        .M_AXIS_X_tdata(wx), // vector components
        .M_AXIS_Y_tdata(wy), // ..
        .M_AXIS_Z_tdata(wz), // ..
        .M_AXIS_U_tdata(wu), // ..
        .M_AXIS_A_tdata(wa), // ..
        .M_AXIS_B_tdata(wb), // ..
        .M_AXIS_index_tdata(gvp_index),
        .M_AXIS_gvp_time_tdata(gvp_time),
        .options(wopt),  // section options: FB, ...
        //.pause(pause),
        .stall(dma_stall),
        .store_data(sto), // trigger to store data:: 2: full vector header, 1: data sources
        .gvp_finished(fin),      // finished 
        .gvp_hold(gvphold),      // finished 
        .x_gvp_ch(Xgvpch),
        .S_AXIS_XV_tdata(xvalue),
        .reset_state(gvpres)
);



axis_FIR axis_FIR_tb
(
    .a_clk(pclk),    // clocking up to aclk
    .next_data(sto),
    .S_AXIS_tdata(wu),
    .S_AXIS_tvalid(1),
    .M_AXIS_tdata(FIR_wu),
    .M_AXIS_tvalid(FIR_twu),
    .M_AXIS2_tdata(FIR2_wu),
    .M_AXIS2_tvalid(FIR2_twu)
);

axis_bram_stream_srcs axis_bram_stream_srcs_tb
(                             // CH      MASK
        .M_AXIS_gvp_x_value_tdata(xvalue),
        .gvp_x_chindex(Xgvpch),
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

    wire [31:0] la2;   
    wire la2v;  
    wire [31:0] lx;   
    wire lxv;  
    wire [31:0] ly;   
    wire lyv;  
    wire [31:0] lsref;   
    wire lsrefv;  
    wire [31:0] lsd2;   
    wire lsdv;  
    wire [31:0] lso2;   
    wire lsov;  
    wire [31:0] li;   
    wire liv;  
    wire ldclk;

axis_py_lockin lockin_tb
    (
    .a_clk(pclk),    // clocking up to aclk
    .S_AXIS_SIGNAL_tdata(sig),
    .S_AXIS_SIGNAL_tvalid(1),
    
    // SC Lock-In Reference and controls
    .S_AXIS_SC_tdata(sc),
    .S_AXIS_SC_tvalid(1),
    
    // XY Output
    .M_AXIS_A2_tdata(la2),
    .M_AXIS_A2_tvalid(la2v),
    .M_AXIS_X_tdata(lx),
    .M_AXIS_X_tvalid(lxv),
    .M_AXIS_Y_tdata(ly),
    .M_AXIS_Y_tvalid(lyv),
    
    // S ref out
    .M_AXIS_Sref_tdata(lsref),
    .M_AXIS_Sref_tvalid(lsrefv),
    // SDeci ref out
    .M_AXIS_SDref_tdata(lsd),
    .M_AXIS_SDref_tvalid(lsdv),
    // Signal out
    .M_AXIS_SignalOut_tdata(lso),
    .M_AXIS_SignalOut_tvalid(lsov),
    // i out
    .M_AXIS_i_tdata(li),
    .M_AXIS_i_tvalid(liv),
    
    .axis_deci_clk(ldclk)
);    

    wire [31:0] bqd;
    wire bqdv; 
    wire [31:0] bqsp;
    wire bqspv; 

axis_biquad_iir_filter biquad(
    .aclk(pclk),
    
    .config_addr(confaddr),
    .config_data(vector),
    
    .S_AXIS_in_tdata(lso),
    .S_AXIS_in_tvalid(lsov),

    .axis_decii_clk(ldclk),
    
    .M_AXIS_out_tdata(bqd),
    .M_AXIS_out_tvalid(bqdv),
    
    .M_AXIS_pass_tdata(bqsp),
    .M_AXIS_pass_tvalid(bqspv)
);    


axis_selector mux(
    .a_clk(pclk),
    .config_addr(confaddr),
    .config_data(vector),
    
    .S_AXIS_00_tdata(mux00td),
    .S_AXIS_00_tvalid(mux00tv),
    .S_AXIS_01_tdata(mux01td),
    .S_AXIS_01_tvalid(mux01tv),
    .S_AXIS_02_tdata(mux02td),
    .S_AXIS_02_tvalid(mux02tv),
    .S_AXIS_03_tdata(mux03td),
    .S_AXIS_03_tvalid(mux03tv),
    .S_AXIS_04_tdata(mux04td),
    .S_AXIS_04_tvalid(mux04tv),
    .S_AXIS_05_tdata(mux05td),
    .S_AXIS_05_tvalid(mux05tv),
    .S_AXIS_06_tdata(mux06td),
    .S_AXIS_06_tvalid(mux06tv),
    .S_AXIS_07_tdata(mux07td),
    .S_AXIS_07_tvalid(mux07tv),
    .S_AXIS_08_tdata(mux08td),
    .S_AXIS_08_tvalid(mux08tv),
    .S_AXIS_09_tdata(mux09td),
    .S_AXIS_09_tvalid(mux09tv),
    .S_AXIS_10_tdata(mux10td),
    .S_AXIS_10_tvalid(mux10tv),
    .S_AXIS_11_tdata(mux11td),
    .S_AXIS_11_tvalid(mux11tv),
    .S_AXIS_12_tdata(mux12td),
    .S_AXIS_12_tvalid(mux12tv),
    .S_AXIS_13_tdata(mux13td),
    .S_AXIS_13_tvalid(mux13tv),
    .S_AXIS_14_tdata(mux14td),
    .S_AXIS_14_tvalid(mux14tv),
    .S_AXIS_15_tdata(mux15td),
    .S_AXIS_15_tvalid(mux15tv),

    .M_AXIS_1_tdata(mux_out01td),
    .M_AXIS_1_tvalid(mux_out01tv),
    .M_AXIS_2_tdata(mux_out02td),
    .M_AXIS_2_tvalid(mux_out02tv),
    .M_AXIS_3_tdata(mux_out03td),
    .M_AXIS_3_tvalid(mux_out03tv),
    .M_AXIS_4_tdata(mux_out04td),
    .M_AXIS_4_tvalid(mux_out04tv),
    .M_AXIS_5_tdata(mux_out05td),
    .M_AXIS_5_tvalid(mux_out05tv),
    .M_AXIS_6_tdata(mux_out06td),
    .M_AXIS_6_tvalid(mux_out06tv)
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

axis_AD463x axis_AD463_tb
(
    .a_clk(pclk),
    .config_addr(confaddr),
    .config_data(vector),

    .M_AXIS1_tdata(ad_ch1),
    .M_AXIS1_tvalid(ad_ch1v),
    .M_AXIS2_tdata(ad_ch2),
    .M_AXIS2_tvalid(ad_ch2v),
    
    .wire_SPI_sck(w_spi_sck),
    .wire_SPI_cs(w_spi_cs),
    .wire_SPI_sdi(w_spi_sdi),
    .wire_SPI_reset(w_spi_reset),
    .wire_SPI_cnv(w_spi_cnv),
    .wire_SPI_busy(w_spi_busy),
    .wire_SPI_sdn(w_spi_sdn),
    
    .mon0(ad_mon0),
    .mon1(ad_mon1),
    
    .bt_dbg(ad_bt_dbg) // UNCOMMENT DBG PORT IN IP TOI SIMULATE A DELAY!!
    );
    
axis_spm_control axis_spm_control_tb
(
    .a_clk(pclk),
    .config_addr(confaddr),
    .config_data(vector),

    .S_AXIS_Xs_tdata(wu),
    .S_AXIS_Xs_tvalid(1),
    .S_AXIS_Ys_tdata(wy),
    .S_AXIS_Ys_tvalid(1),
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
    .S_AXIS_SREF_tdata(SinCos[31:0]),
    .S_AXIS_SREF_tvalid(1),

    .M_AXIS1_tdata  (spm_rx),  
    .M_AXIS1_tvalid (spm_rxv), 
    .M_AXIS2_tdata  (spm_ry),  
    .M_AXIS2_tvalid (spm_ryv), 
    .M_AXIS3_tdata  (spm_rz),  
    .M_AXIS3_tvalid (spm_rzv), 
    .M_AXIS4_tdata  (spm_ru),  
    .M_AXIS4_tvalid (spm_ruv), 

    .M_AXIS_XSMON_tdata  (spm_mrx),  
    .M_AXIS_XSMON_tvalid (spm_mrxv),
    .M_AXIS_YSMON_tdata  (spm_mry),  
    .M_AXIS_YSMON_tvalid (spm_mryv),
    .M_AXIS_ZSMON_tdata  (spm_mrz),  
    .M_AXIS_ZSMON_tvalid (spm_mrzv),

    .M_AXIS_X0MON_tdata  (spm_mx0), 
    .M_AXIS_X0MON_tvalid (spm_mx0v),
    .M_AXIS_Y0MON_tdata  (spm_my0),
    .M_AXIS_Y0MON_tvalid (spm_my0v),
    
    .M_AXIS_ZGVPMON_tdata  (spm_mz0),
    .M_AXIS_ZGVPMON_tvalid (spm_mz0v),
    
  .M_AXIS_Z_SLOPE_tdata  (spm_Zslope),
  .M_AXIS_Z_SLOPE_tvalid (spm_Zslopev),
    
  .M_AXIS_UGVPMON_tdata  (spm_Urm),
  .M_AXIS_UGVPMON_tvalid (spm_Urmv)
);


//def adjust reg (hz)
//    fclk = 1e6 
//    dphi = 2.*pi*reg (hz/fclk)
//    dRe = int reg (Q31 * cos reg (dphi))
//    dIm = int reg (Q31 * sin reg (dphi))
//    return reg (dRe, dIm)
//Q31 = 0x7FFFFFFF # reg (reg (1<<31)-1);
//Q32 = 0xFFFFFFFF # reg (reg (1<<32)-1);
//cr = Q31 # reg (reg (1<<32)-1);
//ci = 0;


SineSDB64 SDB_tb
(
    .aclk (pclk),
    .S_AXIS_DELTAS_tdata (deltasReIm), // deltasreg (Re,Im) vector
    .S_AXIS_DELTAS_tvalid (1),
    .M_AXIS_SC_tdata (SinCos), // reg (Sine, Cosine) vector
    .M_AXIS_SC_tvalid (SCtv)
);



controller_pi#(
        .AXIS_TDATA_WIDTH(32),  
        .M_AXIS_CONTROL_TDATA_WIDTH(32),
        .M_AXIS_CONTROL2_TDATA_WIDTH(32),
        .M_AXIS_CONTROL2_TDATA_WIDTH(32),
        .COEF_Q(31),
        .COEF_W(32),
        .IN_Q(31),
        .IN_W(32),
        .CONTROL_Q(31),
        .CONTROL_W(32),
        .CONTROL2_W(32),
        .FUZZY_CONST_CONTROL_MODE(1),
        .Rdeci(8),
        .Rdecii(0)
    ) servo_tb
/*
(
    parameter AXIS_TDATA_WIDTH = 32,            // INPUT AXIS DATA WIDTH                          PH: 32  AM: 24
    parameter M_AXIS_CONTROL_TDATA_WIDTH = 48,  // SERVO AXIS CONTROL DATA WIDTH OF AXIS          PH: 48  AM: 16
    parameter M_AXIS_CONTROL2_TDATA_WIDTH = 48, // INTERNAL CONTROl DATA WIDTH MAPPED             PH: 48  AM: 32
		                                // TO AXIS FOR READOUT not including extend
    parameter IN_Q = 22,       // Q of Controller Input Signal                                    PH: 22  AM: 22
    parameter IN_W = 23,       // Width of Controller Input Signal                                PH: 23  AM: 23
    parameter COEF_Q = 31,     // Q of CP, CI's                                                   PH: 31  AM: 31
    parameter COEF_W = 32,     // Width of CP, CI's                                               PH: 31  AM: 31
    parameter CONTROL_Q = 31,  // Q of Controlvalue                                               PH: 31  AM: 15
    parameter CONTROL_W = 44,  // Significant Width of Control Output Data                        PH: 44  AM: 16
    parameter CONTROL2_W = 32, // max passed outside control width, must be <= CONTROL2_WIDTH     PH: 32  AM: 16
    parameter AMCONTROL_ALLOW_NEG_SPECIAL = 0, // special AM controller behavior                       0      1
    parameter AUTO_RESET_AT_LIMIT  = 0,        // optional behavior instead of saturation at limits,   X      0
		                                       //  push control back to reset value
    parameter USE_RESET_DATA_INPUT = 1,        // has reset value AXIS input,                          1      1
    parameter FUZZY_CONST_CONTROL_MODE = 0,    // if enabled trys to maintain "reset" value if not exceeding setpoint as a limit, if so, normal regulation operation 

    parameter RDECI  = 1,  // reduced rate decimation bits 1= 1/2 ...
    parameter RDECII = 1   // reduced rate decimation bits 1= 1/2 ...
    // Calculated and fixed Parameters -- ignore and do not change in bogus GUI: zW_XXXXX -- or better hand caclulate and check! Unclear/false behavior :( 
    //parameter zW_ERROR         = IN_W + 1,               // ACTUAL CONTROL ERROR WIDTH REQUIRED as of significant data range
    //parameter zW_EXTEND        = 1,                      // FOR SATURATION CHECK PURPOSE 
    //parameter zW_CONTROL_INT   = COEF_W+IN_W+zW_EXTEND  // INTERNAL CONTROL INTEGRATOR WIDTH
)
*/
(
    .aclk(pclk),
    .S_AXIS_tdata(servo_in), // Controller data input
    .S_AXIS_tvalid(servo_in_tvalid),

    .setpoint(servo_set), // Controller Setpoint
    .cp(2147484), // Controller Parameter Proportional // -60dB
    .ci(2147484), // Controller Parameter Integral
    .limit_upper( 2100000000), // Upper Control Limit
    .limit_lower(-2100000000), // Lower Control Limit

    .S_AXIS_reset_tdata(0), // Controller Reset/Start, i.e. disabled control output value
    .S_AXIS_reset_tvalid(1),
    .enable(1),
    .control_hold(0),
    .fuzzy_cr_mode(0), // reset value is use as "Z" target unles setpoint limit is reached

    .M_AXIS_CONTROL_tdata(servo_ctrl)  // control output, may be less precision

/*    
    .M_AXIS_PASS_tdata, // passed input
    .M_AXIS_PASS_tvalid,
    .M_AXIS_PASS2_tdata, // passed input
    .M_AXIS_PASS2_tvalid,
    .M_AXIS_CONTROL_tdata,  // control output, may be less precision
    .M_AXIS_CONTROL_tvalid,
    .M_AXIS_CONTROL2_tdata, // control output, full precision without over flow bit
    .M_AXIS_CONTROL2_tvalid,
    .M_AXIS_CONTROL3_tdata, // control output, full precision without over flow bit
    .M_AXIS_CONTROL3_tvalid,

    .mon_signal,
    .mon_error,
    .mon_control,
    .mon_control_lower32,
    .mon_control_B,

    .status_max,
    .status_min
    */
    );




endmodule

