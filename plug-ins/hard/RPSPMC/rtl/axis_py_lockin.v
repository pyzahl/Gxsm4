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
// Create Date: 12/28/2024 09:20:10 PM
// Design Name:    part of RPSPMC
// Module Name:    axis_py_lockin
// Project Name:   RPSPMC 4 GXSM
// Target Devices: Zynq z7020
// Tool Versions:  Vivado 2023.1
// Description:    LockIn (C) by Py variant w dedicated corr sum over one exact period 
// 
// Dependencies: 
// 
// Revision:
// Revision 0.01 - File Created
// Additional Comments:
// 
//////////////////////////////////////////////////////////////////////////////////


module axis_py_lockin#(
    parameter S_AXIS_SIGNAL_TDATA_WIDTH = 32, 
    parameter LCK_CORRSUM_WIDTH = 32,  // Final Precision
    parameter LCK_CORRSUM_Q_WIDTH = 31,  // Final Precision
    parameter S_AXIS_SC_TDATA_WIDTH = 64,
    parameter SC_DATA_WIDTH  = 25,  // SC 25Q24
    parameter SC_Q_WIDTH     = 24,  // SC 25Q24
    parameter LCK_Q_WIDTH    = 24,  // SC 25Q24
    parameter DPHASE_WIDTH    = 44, // 44 for delta phase width
    parameter AM2_DATA_WIDTH  = 48, // 48 for amplitude squared
    parameter LCK_BUFFER_LEN2 = 10, // **** need to decimate 100Hz min: log2(1<<44 / (1<<24)) => 20 *** LCK DDS IDEAL Freq= 100 Hz [16777216, N2=24, M=1048576]  Actual f=119.209 Hz  Delta=19.2093 Hz  (using power of 2 phase_inc)
    parameter DECII2_MAX = 32,
    parameter configuration_address = 999
)
(
    (* X_INTERFACE_PARAMETER = "ASSOCIATED_CLKEN a_clk, ASSOCIATED_BUSIF S_AXIS_SC:S_AXIS_SIGNAL:S_AXIS_AMC:S_AXIS_FMC:M_AXIS_DDSPhaseInc:M_AXIS_A2:M_AXIS_X:M_AXIS_Y:M_AXIS_Sref:M_AXIS_SDref:M_AXIS_SignalOut:M_AXIS_i" *)
    input a_clk,

    input [32-1:0]  config_addr,
    input [512-1:0] config_data,
    
    // SIGNAL
    input wire [S_AXIS_SIGNAL_TDATA_WIDTH-1:0]  S_AXIS_SIGNAL_tdata,
    input wire                                  S_AXIS_SIGNAL_tvalid,
    // digial AM
    input wire [S_AXIS_SIGNAL_TDATA_WIDTH-1:0]  S_AXIS_AMC_tdata, // S31Q24
    input wire                                  S_AXIS_AMC_tvalid,
    // digial FM
    input wire [S_AXIS_SIGNAL_TDATA_WIDTH-1:0]  S_AXIS_FMC_tdata, // S31Q24
    input wire                                  S_AXIS_FMC_tvalid,
    
    // SC Lock-In Reference and controls
    input wire [S_AXIS_SC_TDATA_WIDTH-1:0]  S_AXIS_SC_tdata,
    input wire                              S_AXIS_SC_tvalid,
    
    // DDS PhaseInc Control
    output wire [48-1:0]  M_AXIS_DDSPhaseInc_tdata,
    output wire           M_AXIS_DDSPhaseInc_tvalid,

    // XY Output
    output wire [AM2_DATA_WIDTH-1:0]  M_AXIS_A2_tdata,
    output wire                       M_AXIS_A2_tvalid,
    output wire [LCK_CORRSUM_WIDTH-1:0]  M_AXIS_X_tdata,
    output wire                          M_AXIS_X_tvalid,
    output wire [LCK_CORRSUM_WIDTH-1:0]  M_AXIS_Y_tdata,
    output wire                          M_AXIS_Y_tvalid,
    
    // S ref out
    output wire [32-1:0]  M_AXIS_Sref_tdata,
    output wire           M_AXIS_Sref_tvalid,
    // SDeci ref out
    output wire [32-1:0]  M_AXIS_SDref_tdata,
    output wire           M_AXIS_SDref_tvalid,
    // Signal out
    output wire [32-1:0]  M_AXIS_SignalOut_tdata,
    output wire           M_AXIS_SignalOut_tvalid,
    // i out
    output wire [32-1:0]  M_AXIS_i_tdata,
    output wire           M_AXIS_i_tvalid,

    (* X_INTERFACE_PARAMETER = "FREQ_HZ 2000000, ASSOCIATED_CLKEN axis_deci_clk" *)
    output wire           axis_deci_clk

    );

    localparam integer Q20onek = 1000*(1<<20);

    // DDS Control
    reg signed [31:0] amc = 0;
    reg signed [31:0] fmc = 0;
    reg signed [31:0] dds_FM_Scale = Q20onek; // 1kHz/V
    reg signed [63:0] fmc_scaled=0;
    reg [48-1:0] dds_PhaseIncRef = 1;
    reg [48-1:0] dds_FM = 0;
    reg [48-1:0] dds_PhaseInc = 1;

    // Lock-In
    // ========================================================
    // Calculated and internal width and sizes 
    localparam integer LCK_BUFFER_LEN   = (1<<LCK_BUFFER_LEN2);
    localparam integer LCK_SUM_WIDTH    = (LCK_CORRSUM_WIDTH+LCK_BUFFER_LEN2);
    localparam integer Q24 = ((1<<24)-1);
       
    reg signed [SC_DATA_WIDTH-1:0] s=0; // Q SC (25Q24)
    reg signed [SC_DATA_WIDTH-1:0] c=0; // Q SC (25Q24)
    reg signed [SC_DATA_WIDTH-1:0] SigRef=0; // Q SC (25Q24)

    reg signed [31:0] gain = Q24;
    reg signed [SC_DATA_WIDTH-1:0] signal_in  = 0;        // input signal
    reg signed [SC_DATA_WIDTH+24-1:0] sig_in_x_gain = 0;  // x gain tmp
    reg signed [SC_DATA_WIDTH-1:0] sig_in     = 0;        // ready for pre-processing/decimation
    reg signed [SC_DATA_WIDTH+DECII2_MAX-1:0] signal_dec  = 0; // decimation tmp
    reg signed [SC_DATA_WIDTH+DECII2_MAX-1:0] signal_dec2 = 0; // decimation tmp
    reg signed [SC_DATA_WIDTH-1:0] signal     = 0;        // ready for correleation

    reg signed [2*SC_DATA_WIDTH-1:0] LckXcorrpW=0;
    reg signed [2*SC_DATA_WIDTH-1:0] LckYcorrpW=0; // Q SC^2
    reg signed [LCK_CORRSUM_WIDTH-1:0] LckXcorrp=0;
    reg signed [LCK_CORRSUM_WIDTH-1:0] LckYcorrp=0; // Q SC^2

    reg signed [LCK_SUM_WIDTH-1:0] LckX_sum = 0;
    reg signed [LCK_SUM_WIDTH-1:0] LckY_sum = 0;
    reg signed [LCK_CORRSUM_WIDTH-1:0] LckX_mem [LCK_BUFFER_LEN-1:0];
    reg signed [LCK_CORRSUM_WIDTH-1:0] LckY_mem [LCK_BUFFER_LEN-1:0];

    reg signed [LCK_CORRSUM_WIDTH-1:0] a=0;
    reg signed [LCK_CORRSUM_WIDTH-1:0] b=0;
    reg [AM2_DATA_WIDTH-1:0] ampl2=0; // Q LMS Squared 
    reg [2*(LCK_CORRSUM_WIDTH-1)+1-1:0] a2=0; 
    reg [2*(LCK_CORRSUM_WIDTH-1)+1-1:0] b2=0; 
    reg signed [LCK_CORRSUM_WIDTH+2-1:0] x=0; 
    reg signed [LCK_CORRSUM_WIDTH+2-1:0] y=0; 

    reg [16-1 : 0] dds_n2=0;
    reg [LCK_BUFFER_LEN2-1 : 0] ddsdec_nM=0;
    reg [LCK_BUFFER_LEN2-1 : 0] i=0;
    reg [6-1:0] ddsdec_n2 = 0;

    reg signed [8-1:0] decii2 = 0;
    reg signed [8-1:0] decii2_last = 0;
    reg [DECII2_MAX-1:0] decii  = 0;
    reg [DECII2_MAX-1:0] rdecii = 0;

    reg decii_clk=0;

    reg [4-1:0] finishthis=0;

    reg [8-1:0] lck_config=0;
    reg [31:0] lck_gain=Q24;

    integer ii;
    initial begin
        for (ii=0; ii<LCK_BUFFER_LEN; ii=ii+1)
        begin
            LckX_mem [ii] = 0;
            LckY_mem [ii] = 0;
        end     
    end


    always @ (posedge a_clk)
    begin
    
        // module configuration
        if (config_addr == configuration_address) // BQ configuration, and auto reset
        begin
            lck_config      <= config_data[8-1       : 0];    // options: Bit0: AM, Bit2: FM control
            gain            <= config_data[2*32-1    : 1*32]; // programmed gain, Q24
            dds_n2          <= config_data[2*32+16-1 : 2*32];       // Phase Inc N2 lower 16 bits
            dds_PhaseIncRef <= config_data[3*32+48-1 : 3*32]; // Phase Inc Width: 48 bits
            dds_FM_Scale    <= config_data[6*32-1    : 5*32]; // Phase Inc FM Scale for FM -- Q20
        end
        
        if (lck_config[0]) // AM mod?
            amc <= (S_AXIS_AMC_tdata * 5) >>> 7; // "1V" => Q24 => 1V/5V * Q31 * x = Q24
        else
            amc <= Q24;

        if (lck_config[2]) // FM mod?
        begin
            fmc <= S_AXIS_FMC_tdata;
            fmc_scaled <= dds_FM_Scale*fmc; // scale fmc to range
            dds_FM <= fmc_scaled >>> 20; // FM-Scale: Q20
        end else
            dds_FM <= 0; 

        dds_PhaseInc <= dds_PhaseIncRef + dds_FM; 
            
        // signal
        sig_in <= $signed(S_AXIS_SIGNAL_tdata[S_AXIS_SIGNAL_TDATA_WIDTH-1:S_AXIS_SIGNAL_TDATA_WIDTH-SC_DATA_WIDTH]);   

        // Sin, Cos
        c <= S_AXIS_SC_tdata[                        SC_DATA_WIDTH-1 :                       0];  // 25Q24 full dynamic range, proper rounding   24: 0
        s <= S_AXIS_SC_tdata[S_AXIS_SC_TDATA_WIDTH/2+SC_DATA_WIDTH-1 : S_AXIS_SC_TDATA_WIDTH/2];  // 25Q24 full dynamic range, proper rounding   56:32
                
            
        SigRef <= (c * amc) >>> 24; // GeneratedSignal Out with AM and FM mod
                     
        // DDS N / cycle
        // dds_n2 <= S_AXIS_DDS_N2_tdata;
        decii2 <= 44 - LCK_BUFFER_LEN2 - dds_n2;
        if (decii2_last != decii2 || finishthis > 0)
        begin
            decii2_last <= decii2;
            if (finishthis == 0)        
                finishthis <= 7;
            else        
                finishthis <= finishthis-1;
            if (decii2 > 0)
            begin
                decii  <= (1 <<< decii2) - 1; 
                rdecii <= 0;
                ddsdec_n2 <= LCK_BUFFER_LEN2;
                ddsdec_nM  <= (1 <<< LCK_BUFFER_LEN2) - 1;
            end else begin
                decii2 <= 0;
                decii  <= 0; 
                rdecii <= 0;
                ddsdec_n2 <= 44-dds_n2;
                ddsdec_nM  <= (1 <<< (44-dds_n2)) - 1;
            end
            signal_dec <= sig_in; // start new sum
        end 
        else
        begin if (rdecii == decii)
            begin
                rdecii <= 0; // reset
                // 1
                signal <= signal_dec >>> decii2;
                signal_dec <= sig_in; // start new sum
            end
            else
            begin
                rdecii <= rdecii+1; // next
                signal_dec <= signal_dec + sig_in; // sum

                // now we have time steps to run the calculation pipeline... but only one shot
                // LockIn ====
                case (rdecii) // single shot "pipeline" triggered after each decii completed
                0: begin
                    decii_clk <= 0;
                    // Quad Correlation Products
                    LckXcorrpW <= s * signal; // SC_Q_WIDTH x LMS_DATA_WIDTH , LMS_Q_WIDTH
                    LckYcorrpW <= c * signal; //
                end
                1: begin               
                    // Normalize to LCK_CORRSUM_WIDTH
                    LckXcorrp <= LckXcorrpW >>> (2*SC_Q_WIDTH-LCK_CORRSUM_Q_WIDTH); // reduce product width to CK_CORRSUM_WIDTH (32bit)
                    LckYcorrp <= LckYcorrpW >>> (2*SC_Q_WIDTH-LCK_CORRSUM_Q_WIDTH);
                end
                2: begin // calculate moving "integral" running over exactly the last period ++ add new point -- sub the memorized one dropping out
                    LckX_sum <= LckX_sum + LckXcorrp - LckX_mem [i]; // current [i] has oldest in ring buffer
                    LckY_sum <= LckY_sum + LckYcorrp - LckY_mem [i];
                    LckX_mem [i] <= LckXcorrp; // store value to current [i] slot (to be removed at end)
                    LckY_mem [i] <= LckYcorrp;
                    i <= (i+1) & ddsdec_nM;
                end
                3: begin             
                    a <= LckX_sum >>> ddsdec_n2;
                    b <= LckY_sum >>> ddsdec_n2;
                end
                4: begin             
                    a2 <= a*a;
                    b2 <= b*b;
                    y  <= a - b;
                    x  <= a + b;
                end
                5: begin                    
                    decii_clk <= 1;
                    ampl2 <= (a2 + b2) >>> (2*(LCK_CORRSUM_WIDTH-1)+1 - AM2_DATA_WIDTH); // Q48 for SQRT
                end
                default:
                    decii_clk <= 0;
                endcase
            end     
        end
    end
    
    assign M_AXIS_DDSPhaseInc_tdata  = dds_PhaseInc;
    assign M_AXIS_DDSPhaseInc_tvalid = 1;
    
    assign M_AXIS_A2_tdata  = ampl2;
    assign M_AXIS_A2_tvalid = 1;
    assign M_AXIS_X_tdata  = x;
    assign M_AXIS_X_tvalid = 1;
    assign M_AXIS_Y_tdata  = y;
    assign M_AXIS_Y_tvalid = 1;

    assign M_AXIS_Sref_tdata = {{(32-SC_DATA_WIDTH){SigRef[SC_DATA_WIDTH-1]}}, {SigRef[SC_DATA_WIDTH-1:0]}};
    assign M_AXIS_Sref_tvalid = 1;
    // test, dec s
    assign M_AXIS_SDref_tdata = {{(32-SC_DATA_WIDTH){sig_in[SC_DATA_WIDTH-1]}}, {sig_in[SC_DATA_WIDTH-1:0]}};
    assign M_AXIS_SDref_tvalid = 1;
    // dec signal
    assign M_AXIS_SignalOut_tdata = {{(32-SC_DATA_WIDTH){signal[SC_DATA_WIDTH-1]}}, {signal[SC_DATA_WIDTH-1:0]}};
    assign M_AXIS_SignalOut_tvalid = 1;
    // i out
    assign M_AXIS_i_tdata  = i;
    assign M_AXIS_i_tvalid = 1;

    assign axis_deci_clk = decii_clk;

endmodule
