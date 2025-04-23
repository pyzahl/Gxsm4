`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company:  BNL
// Engineer: Percy Zahl
// 
/* Gxsm - Gnome X Scanning Microscopy 
 * ** FPGA Implementaions RPSPMC aka RedPitaya Scanning Probe Control **
 * universal STM/AFM/SARLS/SPALEED/... controlling and
 * data analysis software
 * 
 * Copyright (C) 1999-2025 by Percy Zahl
 *
 * Authors: Percy Zahl <zahl@users.sf.net>
 * WWW Home: http://gxsm.sf.net
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 */
//
// Create Date: 11/26/2017 09:10:43 PM
// Design Name:    part of RPSPMC
// Module Name:    lms_phase_amplitude_detector
// Project Name:   RPSPMC 4 GXSM
// Target Devices: Zynq z7020
// Tool Versions:  Vivado 2023.1
// Description:    LMS based Convergence Phase Amplitude Detector
// 
// Dependencies: 
// 
// Revision:
// Revision 0.01 - File Created
// Additional Comments:
// 
//////////////////////////////////////////////////////////////////////////////////
/*
# s,c signal in QSC
    def phasedetect (self, signal, s, c):
        #// Sin/Cos in Q27 coeff in Q27

        #// Apply loopback filter on ref

        self.ss=int ((1.-self.iirf)*self.ss+self.iirf*s)
        self.cc=int ((1.-self.iirf)*self.cc+self.iirf*c)

        S=s-self.ss  # DelayForRef (delayRef, s, &InputRef1[0]);
        C=c-self.cc  # DelayForRef (delayRef, c, &InputRef2[0]);

        #// Compute the prediction error
        sa = S * self.a
        cb = C * self.b
        predict_err = signal - (( sa + cb + SCQH ) >> NQSC)

        #// Compute d_mu_e  
        d_mu_e = ( predict_err * self.tau_pac + LMSQH ) >> NQLMS;

        #// Compute LMS
        self.b = self.b + (( C * d_mu_e + SCQH ) >> NQSC)
        self.a = self.a + (( S * d_mu_e + SCQH ) >> NQSC)


    def ampl (self):
        return math.sqrt (self.a*self.a + self.b*self.b)/QLMS

    def phase (self):
        return math.atan2 (self.a-self.b, self.a+self.b)
*/

module lms_phase_amplitude_detector #(
    parameter lms_phase_amplitude_detector_address = 20000,
    parameter reset_lms_phase_amplitude_detector_address = 19999,
    parameter S_AXIS_SIGNAL_TDATA_WIDTH = 32, // actually used: LMS DATA WIDTH and Q
    parameter S_AXIS_SC_TDATA_WIDTH = 64,
    parameter M_AXIS_SC_TDATA_WIDTH = 64,
    parameter SC_DATA_WIDTH  = 25,  // SC 25Q24
    parameter SC_Q_WIDTH     = 24,  // SC 25Q24
    parameter LMS_DATA_WIDTH = 26,  // LMS 26Q22
    parameter LMS_Q_WIDTH    = 22,  // LMS 26Q22
    parameter M_AXIS_XY_TDATA_WIDTH = 64,
    parameter M_AXIS_AM_TDATA_WIDTH = 48,
    parameter PH_DELAY = 10,     // PHASE delay steps
    parameter DPHASE_WIDTH = 44, // 44 for delta phase width
    parameter LCK_BUFFER_LEN2 = 13, // NEED 13 
    parameter USE_DUAL_PAC = 1,
    parameter COMPUTE_LOCKIN = 1
)
(
    input [32-1:0]  config_addr,
    input [512-1:0] config_data,
    //(* X_INTERFACE_PARAMETER = "FREQ_HZ 62500000" *)
    (* X_INTERFACE_PARAMETER = "ASSOCIATED_CLKEN aclk, ASSOCIATED_BUSIF S_AXIS_SIGNAL:S_AXIS_SC:M_AXIS_SC:S_AXIS_DDS_dphi:M_AXIS_XY:M_AXIS_AM2:M_AXIS_Aout:M_AXIS_Bout:M_AXIS_LockInX:M_AXIS_LockInY" *)
    input aclk,
    input wire [S_AXIS_SIGNAL_TDATA_WIDTH-1:0]  S_AXIS_SIGNAL_tdata,
    input wire                                  S_AXIS_SIGNAL_tvalid,

    input wire [S_AXIS_SC_TDATA_WIDTH-1:0]  S_AXIS_SC_tdata,
    input wire                              S_AXIS_SC_tvalid,
    
    input wire [48-1:0]  S_AXIS_DDS_dphi_tdata,
    input wire           S_AXIS_DDS_dphi_tvalid,  

    output wire [M_AXIS_SC_TDATA_WIDTH-1:0] M_AXIS_SC_tdata, // (Sine, Cosine) vector pass through
    output wire                             M_AXIS_SC_tvalid,

    output wire [M_AXIS_XY_TDATA_WIDTH-1:0] M_AXIS_XY_tdata, // (Sine, Cosine) vector pass through
    output wire                             M_AXIS_XY_tvalid,

    output wire [M_AXIS_AM_TDATA_WIDTH-1:0] M_AXIS_AM2_tdata, // dbg
    output wire                             M_AXIS_AM2_tvalid,
    
    output wire [31:0] M_AXIS_Aout_tdata,
    output wire        M_AXIS_Aout_tvalid,
    output wire [31:0] M_AXIS_Bout_tdata,
    output wire        M_AXIS_Bout_tvalid,
    output wire [31:0] M_AXIS_LockInX_tdata,
    output wire        M_AXIS_LockInX_tvalid,
    output wire [31:0] M_AXIS_LockInY_tdata,
    output wire        M_AXIS_LockInY_tvalid,

    output wire signed [31:0] dbg_lckN,
    output wire signed [31:0] dbg_lckX,
    output wire signed [31:0] dbg_lckY,
    
    output wire sc_zero_x,
    output wire [2:0] zero_spcp
    
/*    
    // LockIn Buffer BRAM
    //output wire M_BRAM_PORT_A_addra[12:0],
    //output wire M_BRAM_PORT_A_clka,
    //output wire M_BRAM_PORT_A_dina[191:0],
    //output wire M_BRAM_PORT_A_ena,
    //output wire M_BRAM_PORT_A_wea[0:0],
        
    //output wire S_BRAM_PORT_B_addrb[12:0],
    //output wire S_BRAM_PORT_B_clkb,
    //input  wire S_BRAM_PORT_B_doutb[191:0],
    //output wire S_BRAM_PORT_B_ena,
          // Ports for BRAM
   
        
    (* X_INTERFACE_INFO = "xilinx.com:interface:bram:1.0 BRAM_PORTA CLK" *)
    output wire BRAM_PORTA_clk,
    (* X_INTERFACE_INFO = "xilinx.com:interface:bram:1.0 BRAM_PORTA WE" *)
    output wire [0 : 0] BRAM_PORTA_we,
    (* X_INTERFACE_INFO = "xilinx.com:interface:bram:1.0 BRAM_PORTA ADDR" *)
    output wire [12 : 0] BRAM_PORTA_addr,
    (* X_INTERFACE_PARAMETER = "XIL_INTERFACENAME BRAM_PORTA, MEM_SIZE 8192, MEM_WIDTH 192, MEM_ECC NONE, MASTER_TYPE OTHER, READ_WRITE_MODE WRITE_ONLY, READ_LATENCY 1" *)
    (* X_INTERFACE_INFO = "xilinx.com:interface:bram:1.0 BRAM_PORTA DIN" *)
    output wire [191 : 0] BRAM_PORTA_din,
    (* X_INTERFACE_INFO = "xilinx.com:interface:bram:1.0 BRAM_PORTB CLK" *)
    output wire BRAM_PORTB_clk,
    (* X_INTERFACE_INFO = "xilinx.com:interface:bram:1.0 BRAM_PORTB ADDR" *)
    output wire [12 : 0] BRAM_PORTB_addr,
    (* X_INTERFACE_PARAMETER = "XIL_INTERFACENAME BRAM_PORTB, MEM_SIZE 8192, MEM_WIDTH 192, MEM_ECC NONE, MASTER_TYPE OTHER, READ_WRITE_MODE READ_ONLY, READ_LATENCY 1" *)
    (* X_INTERFACE_INFO = "xilinx.com:interface:bram:1.0 BRAM_PORTB DOUT" *)
    input wire [191 : 0] BRAM_PORTB_dout
*/
    );


    wire signed [2*LMS_DATA_WIDTH-1:0] LMSQHALF             = { {(2*LMS_DATA_WIDTH-LMS_Q_WIDTH){1'b0}}, 1'b1, {(LMS_Q_WIDTH-1){1'b0}} }; // for simple rounding Q 1/2 ("=0.5")  2^(44-1)
    wire signed [SC_DATA_WIDTH+LMS_DATA_WIDTH-1:0] SCQHALF  = { {(SC_DATA_WIDTH+LMS_DATA_WIDTH-SC_Q_WIDTH){1'b0}}, 1'b1, {(SC_Q_WIDTH-1){1'b0}} }; // for simple rounding Q 1/2 ("=0.5")
    // wire [45-1:0] PHASE_2PI = {{1'b0}, {(44){1'b1}}}; // 2PI in Q44 + 1/2 dPHI @ 32kHz = 17594437844230
    // wire [45-1:0] PHASE_2PI  = 45'd17594437844230; // 17592186044417  = 2PI in Q44
    // wire [45-1:0] PHASE_2PIR = 45'd17596436316565; //30200Hz full rate
    
    //wire [45-1:0] PHASE_2PIR = 45'd4402296783253; //30200Hz 1/4 rate (DEC4)
    wire [23-1:0] PHASE_2PIR = 23'd1048708; //30200Hz 1/4 rate (DEC4) in Q22
    //wire [22-1:0] TWO_PI_Q22 = 22'd26353589; // 2pi in Q22

    reg signed [31:0] Rtau=0; // Q22 tau phase
    reg signed [31:0] RAtau=0; // Q22 tau amplitude

    reg [4-1:0] rot_ab_pi4x=1;
    reg lck_ampl=0;
    reg lck_phase=0;

    reg signed [LMS_DATA_WIDTH-1:0] signal=0;  // 26Q22   LMS  Q22
    reg signed [LMS_DATA_WIDTH-1:0] signal_1=0; // 26Q22
    reg signed [LMS_DATA_WIDTH-1:0] signal_2=0; // 26Q22
    reg signed [LMS_DATA_WIDTH-1:0] signal_3=0; // 26Q22
    reg signed [LMS_DATA_WIDTH-1:0] signal_4=0; // 26Q22
    reg signed [LMS_DATA_WIDTH-1:0] signal_5=0; // 26Q22
    reg signed [LMS_DATA_WIDTH-1:0] signal_6=0; // 26Q22
    reg signed [LMS_DATA_WIDTH-1:0] signal_7=0; // 26Q22
    reg signed [LMS_DATA_WIDTH-1:0] signal_8=0; // 26Q22
    reg signed [LMS_DATA_WIDTH-1:0] signal_9=0; // 26Q22
    reg signed [LMS_DATA_WIDTH-1:0] signal_10=0; // 26Q22
    reg signed [SC_DATA_WIDTH-1:0] s=0; // Q SC (25Q24)
    reg signed [SC_DATA_WIDTH-1:0] s1=0;
    reg signed [SC_DATA_WIDTH-1:0] s2=0;
    reg signed [SC_DATA_WIDTH-1:0] s3=0;
    reg signed [SC_DATA_WIDTH-1:0] c=0; 
    reg signed [SC_DATA_WIDTH-1:0] c1=0;
    reg signed [SC_DATA_WIDTH-1:0] c2=0;
    reg signed [SC_DATA_WIDTH-1:0] c3=0;
    reg signed [LMS_DATA_WIDTH-1:0] a=0;    // Q LMS (26Q22)
    reg signed [LMS_DATA_WIDTH-1:0] Aa=0;
    reg signed [LMS_DATA_WIDTH-1:0] b=0;
    reg signed [LMS_DATA_WIDTH-1:0] Ab=0;
    reg signed [SC_DATA_WIDTH+LMS_DATA_WIDTH-1:0] sa_1=0; // Q SC + Q LMS   (51Q46)
    reg signed [SC_DATA_WIDTH+LMS_DATA_WIDTH-1:0] cb_1=0;
    reg signed [SC_DATA_WIDTH+LMS_DATA_WIDTH-1:0] Asa_1=0;
    reg signed [SC_DATA_WIDTH+LMS_DATA_WIDTH-1:0] Acb_1=0;
    reg signed [LMS_DATA_WIDTH-1:0] predict_err_2=0; 
    reg signed [LMS_DATA_WIDTH-1:0] Apredict_err_2=0; 
    reg signed [LMS_DATA_WIDTH+LMS_DATA_WIDTH-1:0] d_mu_e_3p=0; 
    reg signed [LMS_DATA_WIDTH+LMS_DATA_WIDTH-1:0] Ad_mu_e_3p=0;
    reg signed [LMS_DATA_WIDTH+LMS_DATA_WIDTH-1:0] d_mu_e_3=0; 
    reg signed [LMS_DATA_WIDTH+LMS_DATA_WIDTH-1:0] Ad_mu_e_3=0;
    reg signed [SC_DATA_WIDTH+LMS_DATA_WIDTH-1:0] Sd_mu_e_4=0; 
    reg signed [SC_DATA_WIDTH+LMS_DATA_WIDTH-1:0] ASd_mu_e_4=0;
    reg signed [SC_DATA_WIDTH+LMS_DATA_WIDTH-1:0] Cd_mu_e_4=0; 
    reg signed [SC_DATA_WIDTH+LMS_DATA_WIDTH-1:0] ACd_mu_e_4=0;
    
    reg [2*(LMS_DATA_WIDTH-1)+1-1:0] ampl2=0; // Q LMS Squared 
    reg [2*(LMS_DATA_WIDTH-1)+1-1:0] a2=0; 
    reg [2*(LMS_DATA_WIDTH-1)+1-1:0] b2=0; 
    reg signed [LMS_DATA_WIDTH+2-1:0] x=0; 
    reg signed [LMS_DATA_WIDTH+2-1:0] y=0; 
    reg signed [LMS_DATA_WIDTH+2-1:0] tmpX=0; 
    reg signed [LMS_DATA_WIDTH+2-1:0] tmpY=0; 
    reg signed [LMS_DATA_WIDTH+2-1:0] tmpXA=0; 
    reg signed [LMS_DATA_WIDTH+2-1:0] tmpYA=0; 

    // Lock-In
    // ========================================================
    // Calculated and fixed Parameters 
    localparam integer LCK_PH_SHIFT     = 22; // phase reduction
    localparam integer LCK_INT_PH_WIDTH = DPHASE_WIDTH - LCK_PH_SHIFT; // +1  = 22
    localparam integer LCK_INT_WIDTH    = LMS_DATA_WIDTH + LCK_INT_PH_WIDTH + 10; // =  26 22 10 = 58
    //localparam integer LCK_NORM           = DPHASE_WIDTH-PH_SHIFT-PH_WIDTH_CUT; // "2pi" is the norm, splitup  

    //reg [LCK_DPH_WIDTH-1:0] dds_dphi [PH_DELAY-1:0]; // 22
    reg signed [LMS_DATA_WIDTH+SC_DATA_WIDTH-1:0] LckXcorrp1 = 0; // SC_Q_WIDTH x LMS_DATA_WIDTH , LMS_Q_WIDTH
    reg signed [LMS_DATA_WIDTH+SC_DATA_WIDTH-1:0] LckYcorrp1 = 0; // SC_Q_WIDTH x LMS_DATA_WIDTH , LMS_Q_WIDTH

    reg signed [LMS_DATA_WIDTH-1:0] LckX2=0;
    reg signed [LMS_DATA_WIDTH-1:0] LckY2=0;

    reg [LCK_INT_PH_WIDTH:0]       LckdIntPhi=0; // DDS PHASE WIDTH=44 + 1
    reg signed [LCK_INT_WIDTH-1:0] LckXInt=0;
    reg signed [LCK_INT_WIDTH-1:0] LckYInt=0;
    
    //reg signed [LCK_INT_WIDTH+LCK_DPH_WIDTH-1:0] LckXdphi3=0;
    //reg signed [LCK_INT_WIDTH+LCK_DPH_WIDTH-1:0] LckYdphi3=0;
    reg [DPHASE_WIDTH-1:0]         LckDdphi1=0;
    reg signed [LCK_INT_PH_WIDTH-1+1:0] LckDdphi2=0;
    reg [LCK_INT_PH_WIDTH-1:0]     LckDdphi3=0;
    reg [LCK_INT_PH_WIDTH-1:0]     LckDdphi4=0;
    reg signed [LCK_INT_WIDTH-1:0] LckXdphi4=0;
    reg signed [LCK_INT_WIDTH-1:0] LckYdphi4=0;
    //reg [LCK_INT_PH_WIDTH-1:0]     LckDdphi [(1<<LCK_BUFFER_LEN2)-1:0];
    //reg signed [LCK_INT_WIDTH-1:0] LckXdphi [(1<<LCK_BUFFER_LEN2)-1:0];
    //reg signed [LCK_INT_WIDTH-1:0] LckYdphi [(1<<LCK_BUFFER_LEN2)-1:0];
    reg [LCK_INT_PH_WIDTH+LCK_INT_WIDTH+LCK_INT_WIDTH-1:0]     LckDXYdphi_mem [(1<<LCK_BUFFER_LEN2)-1:0]; // LckDXYdphi_mem inferred to BRAM
    
    reg [LCK_INT_PH_WIDTH+LCK_INT_WIDTH+LCK_INT_WIDTH-1:0] LckDXYdphi_0=0;
    reg [LCK_INT_PH_WIDTH+LCK_INT_WIDTH+LCK_INT_WIDTH-1:0] LckDXYdphi_1=0;
    
    reg signed [LMS_DATA_WIDTH+LMS_Q_WIDTH-1:0] LckXSumNorm=0;
    reg signed [LMS_DATA_WIDTH+LMS_Q_WIDTH-1:0] LckYSumNorm=0;
    reg signed [LMS_DATA_WIDTH-1:0] LckXSum=0;
    reg signed [LMS_DATA_WIDTH-1:0] LckYSum=0;
    reg [LCK_BUFFER_LEN2-1:0] Lck_i=0;
    reg [LCK_BUFFER_LEN2-1:0] Lck_N=0;

    reg sp=0;
    reg cp=0;
    reg sc_zero=0;
    
    assign M_AXIS_SC_tdata  = S_AXIS_SC_tdata; // pass
    assign M_AXIS_SC_tvalid = S_AXIS_SC_tvalid; // pass
      
    integer i;
    // initial for (i=0; i<PH_DELAY; i=i+1) dds_dphi[i] = 0;
    //initial for (i=0; i<(1<<LCK_BUFFER_LEN2); i=i+1) LckDdphi[i] = 0;
    //initial for (i=0; i<(1<<LCK_BUFFER_LEN2); i=i+1) LckXdphi[i] = 0;
    //initial for (i=0; i<(1<<LCK_BUFFER_LEN2); i=i+1) LckYdphi[i] = 0;
    initial for (i=0; i<(1<<LCK_BUFFER_LEN2); i=i+1) LckDXYdphi_mem[i] = 0;

    reg [1:0] rdecii = 0;
    reg resetn = 1;

    always @ (posedge aclk)
    begin

        case (config_addr)
        lms_phase_amplitude_detector_address:
        begin
            // cfg[SRC_ADDR*32+SRC_BITS-1:SRC_ADDR*32+SRC_BITS-DST_WIDTH] // old, using DST_WIDTH upper bits form cfg reg
            Rtau        <= config_data[1*32-1 : 0*32]; //  tau: buffer to register -- Q22 tau phase
            RAtau       <= config_data[2*32-1 : 1*32]; // Atau: buffer to register -- Q22 tau amplitude
            lck_ampl    <= config_data[2*32   : 2*32];
            lck_phase   <= config_data[2*32+1 : 2*32+1];
            rot_ab_pi4x <= config_data[3*32+3 : 3*32];
        end   
        reset_lms_phase_amplitude_detector_address:
            resetn <= 0;
        default:
            resetn <= 1;
        endcase

        rdecii <= rdecii+1;
        // rdecii 00 01 *10 11 00 ...
        if (rdecii == 3)
        begin
            LckDXYdphi_1 <= LckDXYdphi_mem[Lck_i-Lck_N+1];
        end else begin
            LckDXYdphi_0 <= LckDXYdphi_mem[Lck_i-Lck_N];
        end
        if (rdecii[1]==1)
        begin
            
        //  special IIR DC filter DC error on average of samples at 0,90,180,270
            if (sc_zero)
            begin
                sc_zero <= 0; // reset zero indicator
            end
            
        // Signal Input
            signal   <= S_AXIS_SIGNAL_tdata[LMS_DATA_WIDTH-1:0];
            signal_1 <= signal;
            signal_2 <= signal_1;
            signal_3 <= signal_2;
            signal_4 <= signal_3;
            signal_5 <= signal_4;
            signal_6 <= signal_5;
            signal_7 <= signal_6;
            signal_8 <= signal_7;
            signal_9 <= signal_8;
            signal_10 <= signal_9;  // DDS step, delay signal by 10 -- check!
            
        // Sin, Cos
            //s <= {{(LMS_DATA_WIDTH-LMS_Q_WIDTH-1){ S_AXIS_SC_tdata[S_AXIS_SC_TDATA_WIDTH/2+SC_DATA_WIDTH-1]}}, S_AXIS_SC_tdata[S_AXIS_SC_TDATA_WIDTH/2+SC_DATA_WIDTH-1 : S_AXIS_SC_TDATA_WIDTH/2+SC_DATA_WIDTH-LMS_Q_WIDTH-1]};   // 26Q22 truncated
            //c <= {{(LMS_DATA_WIDTH-LMS_Q_WIDTH-1){ S_AXIS_SC_tdata[                        SC_DATA_WIDTH-1]}}, S_AXIS_SC_tdata[                        SC_DATA_WIDTH-1 :                         SC_DATA_WIDTH-LMS_Q_WIDTH-1]};   // 26Q22 truncated
            c <= S_AXIS_SC_tdata[                        SC_DATA_WIDTH-1 :                       0];  // 25Q24 full dynamic range, proper rounding   24: 0
            s <= S_AXIS_SC_tdata[S_AXIS_SC_TDATA_WIDTH/2+SC_DATA_WIDTH-1 : S_AXIS_SC_TDATA_WIDTH/2];  // 25Q24 full dynamic range, proper rounding   56:32
        // and pipelined delayed s,c
            c1 <= c;
            c2 <= c1;
            c3 <= c2;
            s1 <= s;
            s2 <= s1;
            s3 <= s2;
            
        // S,C Zero Detector
            if (s > $signed(0) && ~sp)
            begin
                sp <= 1;
                sc_zero <= 1;
            end else
            begin
                if (s<$signed(0) && sp)
                begin
                    sp <= 0;
                    sc_zero <= 1;
                end
            end
            if (c>$signed(0) && ~cp)
            begin
                cp <= 1;
                sc_zero <= 1;
            end else
            begin
                if (c<$signed(0) && cp)
                begin
                    cp <= 0;
                    sc_zero <= 1;
                end
            end
    
            if (resetn)
            begin
                // Compute the prediction
                // ### 1 pipeline level
                // predict <= (s * a + c * b + "0.5") >>> 22; // Q22
                sa_1 <= s * a; // Q SC + Q LMS 
                cb_1 <= c * b; // Q24 + Q22 => Q46
                
                // ### 2
                predict_err_2 <= signal_1 - ((sa_1 + cb_1 + SCQHALF) >>> SC_Q_WIDTH); // sum, round normalize Q SC + Q LMS to Q LMS: 46 to 22
                //# DUAL PAC or PAC+LCK -- both needs 82 DSP units out of 80 available for total design
                if (USE_DUAL_PAC)
                begin
                    //== Apredict1 <= (s * Aa + c * Ab + QHALF) >>> LMS_Q_WIDTH; // Q44
                    Asa_1 <= s * Aa; // Q SC + Q LMS 
                    Acb_1 <= c * Ab; //  Q24 + Q22 => Q46
                    Apredict_err_2 <= signal_1 - ((Asa_1 + Acb_1 + SCQHALF) >>> SC_Q_WIDTH); // Q44
                end
                
                // Compute d_mu_e    
                // ### 3
                // error = signal-predict #// Q22 - Q22 : Q22
                // d_mu_e1 <= ((signal-predict1) * tau + "0.5") >>> 22;
                //== d_mu_e3 <= ((signal1 - predict1) * Rtau + QHALF) >>> LMS_Q_WIDTH;
                //d_mu_e_3 <= (predict_err_2 * Rtau + LMSQHALF) >>> LMS_Q_WIDTH;
                //d_mu_e_3 <= (predict_err_2 * Rtau + LMSQHALF); // ) >>> LMS_Q_WIDTH;
                
                //**d_mu_e_3 <= predict_err_2 * Rtau + LMSQHALF; // ** test w o rounding
                // added one pipline stage
                d_mu_e_3p <= predict_err_2 * Rtau; // ** test w o rounding
                d_mu_e_3 <= d_mu_e_3p + LMSQHALF; // ** test w o rounding
                if (USE_DUAL_PAC)
                begin
                    //Ad_mu_e1 <= ((m1-Apredict1) * Atau + 45'sh200000) >>> 22;
                    //**Ad_mu_e_3 <= Apredict_err_2 * RAtau + LMSQHALF; //) >>> LMS_Q_WIDTH;
                    // added one pipline stage
                    Ad_mu_e_3p <= Apredict_err_2 * RAtau; //) >>> LMS_Q_WIDTH;
                    Ad_mu_e_3 <= Ad_mu_e_3p + LMSQHALF; //) >>> LMS_Q_WIDTH;
                end
                
                // Compute LMS
                // ### 4
                // b <= b + ((c2 * d_mu_e2 + "0.5") >>> 22);
                // b <= b + ((c3 * d_mu_e_3 + SCQHALF) >>> SC_Q_WIDTH);
                // b <= b + ((c3 * $signed(d_mu_e_3[LMS_Q_WIDTH+LMS_Q_WIDTH-1:LMS_Q_WIDTH]) + SCQHALF) >>> SC_Q_WIDTH);
                // Cd_mu_e_4 <= c3 * $signed(d_mu_e_3[LMS_Q_WIDTH+LMS_Q_WIDTH-1 : LMS_Q_WIDTH]) + SCQHALF;
                
                //LMS_DATA_WIDTH+LMS_DATA_WIDTH-1
                //--- Cd_mu_e_4 <= c3 * $signed(d_mu_e_3[LMS_Q_WIDTH+LMS_Q_WIDTH-1 : LMS_Q_WIDTH]);
                Cd_mu_e_4 <= c3 * $signed(d_mu_e_3[LMS_Q_WIDTH+LMS_Q_WIDTH-1 : LMS_Q_WIDTH]);
                //b <= b + $signed(Cd_mu_e_4[LMS_Q_WIDTH+SC_Q_WIDTH-1 : SC_Q_WIDTH]);
                b <= b + ((Cd_mu_e_4 + SCQHALF) >>> SC_Q_WIDTH);
                if (USE_DUAL_PAC)
                begin
                    // Ab <= Ab + ((c3 * $signed(Ad_mu_e_3[LMS_Q_WIDTH+LMS_Q_WIDTH-1:LMS_Q_WIDTH]) + SCQHALF) >>> SC_Q_WIDTH);
                    ACd_mu_e_4 <= c3 * $signed(Ad_mu_e_3[LMS_Q_WIDTH+LMS_Q_WIDTH-1 : LMS_Q_WIDTH]);
                    Ab <= Ab + ((ACd_mu_e_4 + SCQHALF) >>> SC_Q_WIDTH);
                end
                
                // a <= a + ((s2 * d_mu_e2 + "0.5") >>> 22);
                // a <= a + ((s3 * d_mu_e_3 + SCQHALF) >>> SC_Q_WIDTH);
                // a <= a + ((s3 * $signed(d_mu_e_3[LMS_Q_WIDTH+LMS_Q_WIDTH-1:LMS_Q_WIDTH]) + SCQHALF) >>> SC_Q_WIDTH);
                Sd_mu_e_4 <= s3 * $signed(d_mu_e_3[LMS_Q_WIDTH+LMS_Q_WIDTH-1 : LMS_Q_WIDTH]);
                a <= a + ((Sd_mu_e_4+SCQHALF) >>> SC_Q_WIDTH);
                if (USE_DUAL_PAC)
                begin
                    // Aa <= Aa + ((s3 * $signed(Ad_mu_e_3[LMS_Q_WIDTH+LMS_Q_WIDTH-1:LMS_Q_WIDTH]) + SCQHALF) >>> SC_Q_WIDTH);
                    ASd_mu_e_4 <= s3 * $signed(Ad_mu_e_3[LMS_Q_WIDTH+LMS_Q_WIDTH-1 : LMS_Q_WIDTH]);
                    Aa <= Aa + ((ASd_mu_e_4+SCQHALF) >>> SC_Q_WIDTH);
                end
                
                // Rot45
                // R2 = sqrt(2)
                // ar = a*R2 - b*R2 = R2*(a-b)
                // br = a*R2 + b*R2 = R2*(a+b)
                // Q22*arctan (ar/br) # +/- pi/2    
                // --> phase = Q22 arctan ((a-b)/(a+b))
                // amp = sqrt (a*a + b*b); // SQRT( Q44 )
                // ph  = atan ((a-b)/(a+b)); // Q22
                //---- amplitude and phase moved to end
        
                /*
                // amplitude squared from 2nd A-PAC 
                ampl2 <= Aa*Aa + Ab*Ab; // 1Q44
                // x,y (for phase)from 1st PAC
                y <= a-b;
                x <= a+b;
                */
                
                if (COMPUTE_LOCKIN)
                begin
            
            /*      sequencial code
                    x = signal*s;
                    y = signal*c;
                    // add new
                    sumdphi  += dphi
                    sumcorrx += x*dphi
                    sumcorry += y*dphi
                    // correct and compensate if phase window len changed
                    while (sumdphi > 2*math.pi+dphi/2):
                        sumdphi  -= corrdphi[circ(corri-corrlen)];
                        sumcorrx -= corrx[circ(corri-corrlen)];
                        sumcorry -= corry[circ(corri-corrlen)];
                        corrlen--;
        
                    // Verilog:          
                    corrdphi[corri] = dphi;
                    corrx[corri] = x*dphi;
                    corry[corri] = y*dphi;
                    corrlen++;
                    corri = circ(corri+1);
        
                    LckdIntPhi2 = LckdIntPhi1 + LckDdphi1; // one step ???? blocking works may be???
                    // single shot unrolled LockIn moving window correlation integral
                    if ((LckdIntPhi2 - LckDdphi [Lck_i-Lck_N]) >= PHASE_2PI ) // phase > 2pi (+dphi/2) // 2pi =!= 2<<44 -- ahead
                    begin
                        LckdIntPhi1 <= LckdIntPhi2 - LckDdphi [Lck_i-Lck_N] - LckDdphi [Lck_i-Lck_N+1];
                        LckXInt <= LckXInt - LckXdphi [Lck_i-Lck_N] - LckXdphi [Lck_i-Lck_N+1] + LckXdphi [Lck_i]; 
                        LckYInt <= LckYInt - LckYdphi [Lck_i-Lck_N] - LckYdphi [Lck_i-Lck_N+1] + LckYdphi [Lck_i];
                        Lck_N <= Lck_N - 1; //!!!!
                    end else 
                        begin
                        if (LckdIntPhi2 >= PHASE_2PI) // phase > 2pi (+dphi/2) // 2pi =!= 2<<44 -- right on
                        begin
                            LckdIntPhi1 <= LckdIntPhi2 - LckDdphi [Lck_i-Lck_N];
                            LckXInt <= LckXInt - LckXdphi [Lck_i-Lck_N] + LckXdphi [Lck_i];
                            LckYInt <= LckYInt - LckYdphi [Lck_i-Lck_N] + LckYdphi [Lck_i];
                        end else
                        begin // behind
                            LckXInt <= LckXInt + LckXdphi [Lck_i];
                            LckYInt <= LckYInt + LckYdphi [Lck_i];
                            Lck_N <= Lck_N + 1; //!!!!
                        end
                    end
        
        
            */
        
                    // Classic LockIn and Correlation moving Integral over one period
            
                    // Store in ring buffer
                    // LckDdphi [Lck_i] <= LckDdphi4; // LCK_INT_PH_WIDTH 
                    // LckXdphi [Lck_i] <= LckXdphi4; // LCK_INT_WIDTH
                    // LckYdphi [Lck_i] <= LckYdphi4;
                    LckDXYdphi_mem [Lck_i] <= { LckDdphi4, LckXdphi4, LckYdphi4 }; // LCK_INT_PH_WIDTH 
        
                    // STEP 1: Correlelation Product
                    LckDdphi1  <= S_AXIS_DDS_dphi_tdata[DPHASE_WIDTH-1:0] >>> 2; // convert to LCK_INT_PH_WIDTH  -- account for 4x decimation 
                    LckXcorrp1 <= s * signal_10; // SC_Q_WIDTH x LMS_DATA_WIDTH , LMS_Q_WIDTH
                    LckYcorrp1 <= c * signal_10; //
        
                    // STEP 2 
                    LckX2 <= (LckXcorrp1 + SCQHALF) >>> SC_Q_WIDTH; // Q22  LMS_DATA_WIDTH , LMS_Q_WIDTH
                    LckY2 <= (LckYcorrp1 + SCQHALF) >>> SC_Q_WIDTH; // Q22
                    LckDdphi2 <= LckDdphi1 >>> LCK_PH_SHIFT; 
        
                    // STEP 3: Scale to Phase-Signal Volume // **** DDS_dphi limited from principal 48bit to LCK_DPH_WIDTH *****
                    LckXdphi4 <= LckX2 * LckDdphi2; // S_AXIS_DDS_dphi_tdata[LCK_DPH_WIDTH-1:0]; // LMS_DATA_WIDTH + LCK_DPH_WIDTH - >>>PH_SHIFT  == LCK_INT_WIDTH   [26]Q22 + Q44 [assume 30kHz range renorm by 12 now (4096) --  4166 is 30kHz spp] 22+44-12=54
                    LckYdphi4 <= LckY2 * LckDdphi2; // S_AXIS_DDS_dphi_tdata[LCK_DPH_WIDTH-1:0]; // Q22 [16 sig -> 6 spare] + Q44 [12 spare @ 30kHz] 44+22-6-12
                    LckDdphi4 <= LckDdphi2[LCK_INT_PH_WIDTH-1:0];
        
                    //LckXdphi4 <= LckXdphi3 >>> PH_SHIFT; // LMS_DATA_WIDTH + LCK_DPH_WIDTH - >>>PH_SHIFT  == LCK_INT_WIDTH   [26]Q22 + Q44 [assume 30kHz range renorm by 12 now (4096) --  4166 is 30kHz spp] 22+44-12=54
                    //LckYdphi4 <= LckYdphi3 >>> PH_SHIFT; // Q22 [16 sig -> 6 spare] + Q44 [12 spare @ 30kHz] 44+22-6-12
                    //LckDdphi4 <= LckDdphi3;
                    
                    // SETP 4: update ring buffer and correl length
                    // update one period integral
                    // single shot unrolled LockIn moving window correlation integral
                    
                    /*
                    if ((LckdIntPhi + LckDdphi4 - LckDdphi [Lck_i-Lck_N]) >= PHASE_2PIR) // phase > 2pi (+dphi/2) // 2pi =!= 2<<44 -- ahead
                    begin
                        LckdIntPhi <= LckdIntPhi + LckDdphi4 - LckDdphi [Lck_i-Lck_N] - LckDdphi [Lck_i-Lck_N+1]; // remove two dPhi's, one shorter 
                        LckXInt    <= LckXInt    + LckXdphi4 - LckXdphi [Lck_i-Lck_N] - LckXdphi [Lck_i-Lck_N+1]; 
                        LckYInt    <= LckYInt    + LckYdphi4 - LckYdphi [Lck_i-Lck_N] - LckYdphi [Lck_i-Lck_N+1];
                        Lck_N <= Lck_N - 1; // shorter
                    end else 
                        begin
                        if (LckdIntPhi + LckDdphi4 >= PHASE_2PIR) // phase > 2pi (+dphi/2) // 2pi =!= 2<<44 -- right on
                        begin
                            LckdIntPhi <= LckdIntPhi + LckDdphi4 - LckDdphi [Lck_i-Lck_N]; // remove one dPhi, add one new dPhi (now change in length)
                            LckXInt    <= LckXInt    + LckXdphi4 - LckXdphi [Lck_i-Lck_N];
                            LckYInt    <= LckYInt    + LckYdphi4 - LckYdphi [Lck_i-Lck_N];
                            // Lck_N unchanged
                        end else
                        begin // behind
                            LckdIntPhi <= LckdIntPhi + LckDdphi4; // add new dPhi, and keep -- longer now
                            LckXInt    <= LckXInt    + LckXdphi4;
                            LckYInt    <= LckYInt    + LckYdphi4;
                            Lck_N <= Lck_N + 1; // longer
                        end
                    end
                    */
                    
                    //LckDXYdphi_0 <= LckDXYdphi_mem[Lck_i-Lck_N];
                    //LckDXYdphi_1 <= LckDXYdphi_mem[Lck_i-Lck_N+1];
                    //    reg [LCK_INT_PH_WIDTH+LCK_INT_WIDTH+LCK_INT_WIDTH-1:0]     LckDXYdphi [(1<<LCK_BUFFER_LEN2)-1:0];
                    if ((LckdIntPhi + LckDdphi4 - LckDXYdphi_0[LCK_INT_PH_WIDTH+LCK_INT_WIDTH+LCK_INT_WIDTH-1:LCK_INT_WIDTH+LCK_INT_WIDTH]) >= PHASE_2PIR) // phase > 2pi (+dphi/2) // 2pi =!= 2<<44 -- ahead
                    begin
                        LckdIntPhi <= LckdIntPhi + LckDdphi4 - LckDXYdphi_0[LCK_INT_PH_WIDTH+LCK_INT_WIDTH+LCK_INT_WIDTH-1:LCK_INT_WIDTH+LCK_INT_WIDTH] - LckDXYdphi_1[LCK_INT_PH_WIDTH+LCK_INT_WIDTH+LCK_INT_WIDTH-1:LCK_INT_WIDTH+LCK_INT_WIDTH]; // remove two dPhi's, one shorter 
                        LckXInt    <= LckXInt    + LckXdphi4 - LckDXYdphi_0[                 LCK_INT_WIDTH+LCK_INT_WIDTH-1:LCK_INT_WIDTH              ] - LckDXYdphi_1[                 LCK_INT_WIDTH+LCK_INT_WIDTH-1:LCK_INT_WIDTH              ]; 
                        LckYInt    <= LckYInt    + LckYdphi4 - LckDXYdphi_0[                               LCK_INT_WIDTH-1:0                          ] - LckDXYdphi_1[                               LCK_INT_WIDTH-1:0                          ];
                        Lck_N <= Lck_N - 1; // shorter
                    end else 
                        begin
                        if (LckdIntPhi + LckDdphi4 >= PHASE_2PIR) // phase > 2pi (+dphi/2) // 2pi =!= 2<<44 -- right on
                        begin
                            LckdIntPhi <= LckdIntPhi + LckDdphi4 - LckDXYdphi_0[LCK_INT_PH_WIDTH+LCK_INT_WIDTH+LCK_INT_WIDTH-1:LCK_INT_WIDTH+LCK_INT_WIDTH]; // remove one dPhi, add one new dPhi (now change in length)
                            LckXInt    <= LckXInt    + LckXdphi4 - LckDXYdphi_0[                 LCK_INT_WIDTH+LCK_INT_WIDTH-1:LCK_INT_WIDTH              ];
                            LckYInt    <= LckYInt    + LckYdphi4 - LckDXYdphi_0[                               LCK_INT_WIDTH-1:0                          ];
                            // Lck_N unchanged
                        end else
                        begin // behind
                            LckdIntPhi <= LckdIntPhi + LckDdphi4; // add new dPhi, and keep -- longer now
                            LckXInt    <= LckXInt    + LckXdphi4;
                            LckYInt    <= LckYInt    + LckYdphi4;
                            Lck_N <= Lck_N + 1; // longer
                        end
                    end
                    
        
        
                    Lck_i <= Lck_i + 1;
            
                    //   localparam integer LCK_INT_WIDTH      = LMS_DATA_WIDTH + DPHASE_WIDTH - PH_SHIFT; // integral over sine should not exceed 1  LMS_DATA_WIDTH + LCK_DPH_WIDTH - >>>PH_SHIFT  == LCK_INT_WIDTH
                    //   ** signed [LCK_INT_WIDTH-1:0] LckXInt=0;
        
                    //**    wire [45-1:0] PHASE_2PIR = 45'd4402296783253; //30200Hz 1/4 rate (DEC4)   *** 398599931957
                    //**    wire [22-1:0] TWO_PI_Q22 = 22'd26353589; // 2pi in Q22
                    //LckXSumNorm <= TWO_PI_Q22 * $signed(LckXInt[LCK_INT_WIDTH-1 : DPHASE_WIDTH - PH_SHIFT]); //>>> LCK_NORM; // normalize with 2pi == Q44 - PH_SHIFT (remaining bits afer partial pre norm)
                    //LckYSumNorm <= TWO_PI_Q22 * $signed(LckYInt[LCK_INT_WIDTH-1 : DPHASE_WIDTH - PH_SHIFT]); //>>> LCK_NORM; // 
                    //LckXSum <= LckXSumNorm >>> 22; //>>> LCK_NORM; // normalize with 2pi == Q44 - PH_SHIFT (remaining bits afer partial pre norm)
                    //LckYSum <= LckYSumNorm >>> 22; //>>> LCK_NORM; // 
                    //LckXSum <= LckXInt >>> (DPHASE_WIDTH-3 - LMS_Q_WIDTH + PH_SHIFT + 1 -13 ); //>>> LCK_NORM; // normalize with pi == Q44 / 4 {1/4 rate} / 2 {pi vs 2pi} = 44-3 - PH_SHIFT (remaining bits afer partial pre norm)
                    //LckYSum <= LckYInt >>> (DPHASE_WIDTH-3 - LMS_Q_WIDTH + PH_SHIFT + 1 -13); //>>> LCK_NORM; // 
                    //  x = INT__2PI { (X*SC >> SC__Q_WIDTH)*dPHI >> PH_SHIFT } / PI
                    //                  (X[22]+24)-24 + (44-22) 
                    LckXSum <= LckXInt >>> (DPHASE_WIDTH-2+1 - LCK_PH_SHIFT -2); //>>> LCK_NORM; // normalize with pi == Q44 / 4 {1/4 rate} / 2 {pi vs 2pi} = 44-3 - PH_SHIFT (remaining bits afer partial pre norm)
                    LckYSum <= LckYInt >>> (DPHASE_WIDTH-2+1 - LCK_PH_SHIFT -2); //>>> LCK_NORM; // 
                    //norm = 2 / self.PHASE_2PI / (1<<(self.LMS_Q_WIDTH)) * (1<<self.PH_SHIFT)
                    //         2^44 / 2^21 [keep LMS Q WIDTH] = 2^23  ----> 2pi * (XXX >> 23)
                    //  (44-...)=  DPHASE_WIDTH  - PH_SHIFT - LMS_Q_WIDTH     
                    // self.a = self.filter(self.a, 2pi*self.LckXSum/norm)  # 2 pi * ...
        
                end
                  
                // prepare outputs by selection        
                
                if (USE_DUAL_PAC && !COMPUTE_LOCKIN)
                begin
                    //# For Dual PAC:     
                    // amplitude squared from 2nd A-PAC 
                    a2 <= Aa*Aa;
                    b2 <= Ab*Ab;
                    //== ampl2 <= Aa*Aa + Ab*Ab; // 1Q44
                    ampl2 <= a2 + b2; // 1Q44
                    // x,y (for phase)from 1st PAC
                    case (rot_ab_pi4x)
                    0: // ZERO DEG vs. SINE
                    begin
                        y <= a;
                        x <= b;
                    end
                    1: // ORIGINAL 45 deg
                    begin
                        y <= a-b;
                        x <= a+b;
                    end
                    2: // -45
                    begin
                        y <= b-a;
                        x <= a+b;
                    end
                    3: // 90
                    begin
                        y <= b;
                        x <= a;
                    end
                    4: // -90
                    begin
                        y <= -b;
                        x <= a;
                    end
                    5: // 180
                    begin
                        y <= -a;
                        x <= b;
                    end
                    6: // -180
                    begin
                        y <= -a;
                        x <= -b;
                    end
                    endcase
                end 
                else if (COMPUTE_LOCKIN)
                begin
                    // select PAC or LockIn individually
                    if (USE_DUAL_PAC) // select from Dual PAC or LockIn options
                    begin
                        a2 <= (lck_ampl?LckXSum:Aa)*(lck_ampl?LckXSum:Aa);
                        b2 <= (lck_ampl?LckYSum:Ab)*(lck_ampl?LckYSum:Ab);
                    end else
                    begin // select from Single PAC (same tau for PH and AM) or LockIn
                        a2 <= (lck_ampl?LckXSum:a)*(lck_ampl?LckXSum:a);
                        b2 <= (lck_ampl?LckYSum:b)*(lck_ampl?LckYSum:b);
                    end
                    ampl2 <= a2 + b2; // 1Q44
                    // x,y (for phase)from PH PAC or LockIn
                    y     <= (lck_phase?LckXSum:a) - (lck_phase?LckYSum:b);
                    x     <= (lck_phase?LckXSum:a) + (lck_phase?LckYSum:b);
                    //y     <= (lck_phase? LckYSum : (a - b));
                    //x     <= (lck_phase? LckXSum : (a + b));
                end
                else
                begin
                    a2 <= a*a;
                    b2 <= b*b;
                    //== ampl2 <= Aa*Aa + Ab*Ab; // 1Q44
                    ampl2 <= a2 + b2; // 1Q44
                    // x,y (for phase)from 1st PAC
                    y <= a-b;
                    x <= a+b;
                end
            end
            else
            begin // reset LMS
                a <= 0;
                b <= 0;
                Aa <= 0;
                Ab <= 0;
            end
        end
    end
         
    //assign amplitude = sqrt (a*a+b*b); // Q22
    //assign phase = 4194303*atan ((a-b)/(a+b)); // Q22

    // atan2 cordic input w 28
    assign M_AXIS_XY_tdata    = { {(M_AXIS_XY_TDATA_WIDTH/2-(LMS_DATA_WIDTH+2)){y[LMS_DATA_WIDTH+2-1]}}, y,  
                                  {(M_AXIS_XY_TDATA_WIDTH/2-(LMS_DATA_WIDTH+2)){x[LMS_DATA_WIDTH+2-1]}}, x };
    assign M_AXIS_XY_tvalid   = 1'b1;

    assign M_AXIS_AM2_tdata   = ampl2[M_AXIS_AM_TDATA_WIDTH-1:0]; 
    assign M_AXIS_AM2_tvalid  = 1'b1;

    // LMS A,B
    assign M_AXIS_Aout_tdata  = {{(32-LMS_DATA_WIDTH){a[LMS_DATA_WIDTH-1]}}, a}; // a
    assign M_AXIS_Aout_tvalid = 1'b1;
    assign M_AXIS_Bout_tdata  = {{(32-LMS_DATA_WIDTH){b[LMS_DATA_WIDTH-1]}}, b}; // b
    assign M_AXIS_Bout_tvalid = 1'b1;

    // LockIn X,Y Sum
    assign M_AXIS_LockInX_tdata  = {{(32-LMS_DATA_WIDTH){LckXSum[LMS_DATA_WIDTH-1]}}, LckXSum};
    assign M_AXIS_LockInX_tvalid = 1'b1;
    assign M_AXIS_LockInY_tdata  = {{(32-LMS_DATA_WIDTH){LckYSum[LMS_DATA_WIDTH-1]}}, LckYSum};
    assign M_AXIS_LockInY_tvalid = 1'b1;
    
    assign sc_zero_x = sc_zero;
    assign zero_spcp = {sc_zero, sp, cp};

    //    assign dbg_lckN = { {(32-LCK_BUFFER_LEN2){1'b0}}, Lck_N }; // [LCK_BUFFER_LEN2-1:0] Lck_N=0;   14 bits
    assign dbg_lckN = { 2'b00, Lck_i, 2'b00, Lck_N }; // [LCK_BUFFER_LEN2-1:0] Lck_N=0;   14 bits
    assign dbg_lckX = {{(32-LMS_DATA_WIDTH){LckXSum[LMS_DATA_WIDTH-1]}}, LckXSum};
    assign dbg_lckY = {{(32-LMS_DATA_WIDTH){LckYSum[LMS_DATA_WIDTH-1]}}, LckYSum};

endmodule
