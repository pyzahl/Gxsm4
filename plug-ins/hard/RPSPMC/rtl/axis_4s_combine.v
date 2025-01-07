`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
/* Gxsm - Gnome X Scanning Microscopy
 * universal STM/AFM/SARLS/SPALEED/... controlling and
 * data analysis software
 * 
 * Copyright (C) 1999,2000,2001,2002,2003 Percy Zahl
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
// Create Date: 11/26/2017 08:20:47 PM
// Design Name: 
// Module Name: signal_combine
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
/*
 *  TRANSPORT FPGA MODULE to BRAM:
 *  S_AXIS1: M-AXIS-SIGNAL from LMS CH0 (input), CH1 (not used) -- pass through from ADC 14 bit
 *  S_AXIS2: M-AXIS-CONTROL Amplitude: output/monitor from Ampl. Controller (32 bit)
 *  S_AXIS3: M-AXIS-CONTROL Phase: output/monitor from Phase. Controller (48 bit)
 *  S_AXIS4: M-AXIS-CONTROL Phase pass, monitor Phase. from CORDIC ATAN X/Y (24 bit)
 *  S_AXIS5: M-AXIS-CONTROL Amplitude pass, monitor Amplitude from CORDIC SQRT (24 bit)
 *
 *  TRANSPORT MODE:
 *  BLOCK modes singel shot, fill after start, stop when full:
 *  0: S_AXIS1 CH1, CH2 decimated, aver aged (sum)
 *  1: S_AXIS4,5  decimated, averaged (sum)
 *  2: A_AXIS1 CH1 decimated, averaged (sum), ADDRreg
 *  3: TEST -- ch1n <= 32'h01234567, ch2n <= int_decimate_count;
 *  CONTINEOUS STEAM FIFO operation (loop, overwrite)
 *  ... modes to be added for FIFO contineous operation mode
 */

//////////////////////////////////////////////////////////////////////////////////



module axis_4s_combine #(
    parameter SAXIS_1_DATA_WIDTH  = 16,
    parameter SAXIS_1_TDATA_WIDTH = 32,
    parameter SAXIS_2_DATA_WIDTH  = 32,
    parameter SAXIS_2_TDATA_WIDTH = 32,
    parameter SAXIS_3_DATA_WIDTH  = 48,
    parameter SAXIS_3_TDATA_WIDTH = 48,
    parameter SAXIS_4_DATA_WIDTH  = 32,
    parameter SAXIS_4_TDATA_WIDTH = 32,
    parameter SAXIS_5_DATA_WIDTH  = 24,
    parameter SAXIS_5_TDATA_WIDTH = 24,
    parameter SAXIS_6_DATA_WIDTH   = 32,
    parameter SAXIS_6_TDATA_WIDTH  = 32,
    parameter SAXIS_78_DATA_WIDTH  = 32,
    parameter SAXIS_78_TDATA_WIDTH = 32,
    parameter MAXIS_TDATA_WIDTH = 32,
    parameter integer BRAM_DATA_WIDTH = 64,
    parameter integer BRAM_ADDR_WIDTH = 15,
    parameter integer DECIMATED_WIDTH = 32,
    parameter integer LCK_ENABLE = 0
)
(
    // (* X_INTERFACE_PARAMETER = "FREQ_HZ 125000000" *)
    (* X_INTERFACE_PARAMETER = "ASSOCIATED_CLKEN a_clk, ASSOCIATED_BUSIF S_AXIS1:S_AXIS1S:S_AXIS2:S_AXIS3:S_AXIS4:S_AXIS5:S_AXIS6:S_AXIS7:S_AXIS8:M_AXIS_CH1:M_AXIS_CH2:M_AXIS_CH3:M_AXIS_CH4:M_AXIS_DFREQ_DEC:M_AXIS_DFREQ_DEC2" *)
    input a_clk,
    // input a_resetn
    
    input wire [SAXIS_1_TDATA_WIDTH-1:0]  S_AXIS1_tdata, // IN1, IN2
    input wire                            S_AXIS1_tvalid,
    input wire [SAXIS_1_TDATA_WIDTH-1:0]  S_AXIS1S_tdata, // IN1, IN2
    input wire                            S_AXIS1S_tvalid,
    input wire [SAXIS_2_TDATA_WIDTH-1:0]  S_AXIS2_tdata, // AM Control2
    input wire                            S_AXIS2_tvalid,
    input wire [SAXIS_3_TDATA_WIDTH-1:0]  S_AXIS3_tdata, // PH Control2
    input wire                            S_AXIS3_tvalid,
    input wire [SAXIS_4_TDATA_WIDTH-1:0]  S_AXIS4_tdata, // PH Pass2
    input wire                            S_AXIS4_tvalid,
    input wire [SAXIS_5_TDATA_WIDTH-1:0]  S_AXIS5_tdata, // AM Pass2
    input wire                            S_AXIS5_tvalid,
    input wire [SAXIS_6_TDATA_WIDTH-1:0]  S_AXIS6_tdata, // DCFilter: AC,DC
    input wire                            S_AXIS6_tvalid,
    input wire [SAXIS_78_TDATA_WIDTH-1:0]  S_AXIS7_tdata, // Lck-X
    input wire                             S_AXIS7_tvalid,
    input wire [SAXIS_78_TDATA_WIDTH-1:0]  S_AXIS8_tdata, // Lck-Y
    input wire                             S_AXIS8_tvalid,

    input wire [SAXIS_3_DATA_WIDTH-1:0] axis3_center,
    input wire [2:0] zero_spcp,

    input wire [32-1:0]  operation, // 0..7 control bits 0: 1=start 1: 1=single shot/0=fifo loop 2, 4: 16=init/reset , [31:8] DATA ACCUMULATOR (64) SHR   [0] start/stop, [1] ss/loop, [2], [3], [4] reset
    input wire [32-1:0]  ndecimate,
    input wire [32-1:0]  nsamples,
    input wire [32-1:0]  channel_selector,
    input wire           ext_trigger, // =0 for free run, auto trigger, 1= to hold trigger (until next pix, etc.)
    
    output wire          finished_state,
    output wire          init_state,
    output wire [32-1:0] writeposition,
    
    // DOWN SAMPLED CH1-4
    output wire [MAXIS_TDATA_WIDTH-1:0]  M_AXIS_CH1_tdata,
    output wire                          M_AXIS_CH1_tvalid,
    output wire [MAXIS_TDATA_WIDTH-1:0]  M_AXIS_CH2_tdata,
    output wire                          M_AXIS_CH2_tvalid,
    output wire [MAXIS_TDATA_WIDTH-1:0]  M_AXIS_CH3_tdata,
    output wire                          M_AXIS_CH3_tvalid,
    output wire [MAXIS_TDATA_WIDTH-1:0]  M_AXIS_CH4_tdata,
    output wire                          M_AXIS_CH4_tvalid,

    output wire [MAXIS_TDATA_WIDTH-1:0]  M_AXIS_DFREQ_DEC_tdata,
    output wire                          M_AXIS_DFREQ_DEC_tvalid,
    output wire [MAXIS_TDATA_WIDTH-1:0]  M_AXIS_DFREQ_DEC2_tdata,
    output wire                          M_AXIS_DFREQ_DEC2_tvalid,

    output wire [32-1:0] delta_frequency_monitor,
    output wire [32-1:0] delta_frequency_direct,

    output wire FIR_next,

    // BRAM PUSH INTERFACE
    output wire [32-1:0] BR_ch1s,
    output wire [32-1:0] BR_ch2s,
    output wire BR_next,
    output wire BR_reset,
    input  wire BR_ready,
    input  wire [15:0] BR_wpos,
    output wire pulse_arm_single,
    output wire pulse_run    
    );
    
    reg [2:0] dec_sms=3'd0;
    reg [2:0] dec_sms_next=3'd0;

    reg [32-1:0] decimate_count=0; 
    reg [32-1:0] sample_count=0;

    reg signed [64-1:0] ch1, ch2, chPH, chDF, chAM, chEX;
    reg signed [64-1:0] ch1s, ch2s, chPHs, chDFs, chAMs, chEXs;
    reg signed [64-1:0] reg_freq_center;
    reg signed [64-1:0] reg_freq;
    reg signed [64-1:0] reg_delta_freq;

    reg [ 8-1:0] reg_operation;
    reg [24-1:0] reg_shift;
    reg [32-1:0] reg_ndecimate;
    reg [32-1:0] reg_nsamples;
    reg [ 4-1:0] reg_channel_selector;
    reg          reg_ext_trigger;


    reg finished=1'b0;
    reg trigger=1'b0;
    reg trigger_next=1'b0;
    
    reg fir_next=0;
    
    reg bram_reset=1;
    reg bram_next=0;
    reg bram_retry=0;

    reg reg_pulse_arm_single=0;
    reg reg_pulse_run=1;

    assign pulse_arm_single = reg_pulse_arm_single;
    assign pulse_run = reg_pulse_run;

    //wire signed [64-1:0] Q31HALF = { {(64-31){1'b0}}, 1'b1, {(31-1){1'b0}} };

    
    assign init_state = operation[0];
    assign finished_state = finished;
    assign writeposition = { sample_count[15:0], finished, BR_wpos[14:0] };   
    
    assign FIR_next = fir_next;
    
    // BRAM INTERFACE CONNECT
    assign BR_next  = bram_next;       
    assign BR_reset = bram_reset;       
    assign BR_ch1s  = ch1s[31:0];
    //assign BR_ch1s  = reg_tau_dfreq[31] ? ch1s[31:0] : ch1s_lp;
    assign BR_ch2s  = ch2s[31:0];

    reg [1:0] rdecii = 0;

    reg i_zero_spcp1 = 0;
    reg zero_x_s = 0;


    always @ (posedge a_clk)
    begin
        rdecii <= rdecii+1; // rdecii 00 01 *10 11 00 ...
        if (rdecii == 1)
        begin
            // buffer in local register
            reg_channel_selector <= channel_selector[4-1:0];
            reg_ext_trigger <= ext_trigger;
            reg_operation <= operation[7:0];
            reg_shift     <= operation[31:8];
            reg_ndecimate <= ndecimate;
            reg_nsamples  <= nsamples+1;
    
            // map data sources and calculate delta Freq
            reg_freq_center <= {{(64-SAXIS_3_DATA_WIDTH){1'b0}},  axis3_center[SAXIS_3_DATA_WIDTH-1:0]}; // expand to 64 bit, signed but always pos
            reg_freq        <= {{(64-SAXIS_3_DATA_WIDTH){1'b0}}, S_AXIS3_tdata[SAXIS_3_DATA_WIDTH-1:0]}; // expand to 64 bit, signed but always pos
            reg_delta_freq  <= reg_freq - reg_freq_center; // compute delta frequency -- should need way less than actual 48bits now! But here we go will full range. Deciamtion may overrun is delta is way way off normal.
        end

        if (i_zero_spcp1 != zero_spcp[1])
        begin
            i_zero_spcp1 <= zero_spcp[1];
            if (zero_spcp[1] > 0)
                zero_x_s <= 1;
        end

        // ===============================================================================
        // MANAGE EXT CONTROLS [reset, operation, trigger] FOR DECIMATING STATE MACHINE
        // ===============================================================================

        // reg_operation[0] 1: start auto,  0: no change
        // reg_operation[1] 1: Single Shot, 0: Auto Rearm
        // reg_operation[2] not used
        // reg_operation[3] not used
        // reg_operation[4] put in reset

        if(reg_operation[4]) // reset operation mode set
        begin
            finished <= 1'b0; // reset finished
            dec_sms  <= 3'd0; // stay in reset mode
            trigger_next <= 1'b1; // set software trigger
        end else begin
            dec_sms <= dec_sms_next; // go to next mode
        end
        
            
        // ===============================================================================
        // DECIMATING STATE MACHINE
        // ===============================================================================

        case (dec_sms) // Decimation State Machine Status
            // ===============================================================================================
            0: // RESET STATE
            begin
                zero_x_s      <= 0;
                bram_reset    <= 1'b1;
                finished      <= 1'b0;
                dec_sms_next  <= 3'd1; // ready, now wait for release of reset operation to enagage trigger and arm
                sample_count  <= 32'd0;
                fir_next      <= ~fir_next; // clock FIR for rest cycle
            end
            // ===============================================================================================
            1:    // Wait for trigger
            begin
            
                // trigger and pulse arm logic
                if(reg_operation[0]) // operation mode: tigger start running
                begin
                    if (reg_operation[1]) // Scope Manual Single Shot Mode 
                    begin
                        if (zero_x_s) // wait for ref zero crossing
                        begin
                            zero_x_s <= 0;
                            if (!trigger)
                            begin
                                reg_pulse_arm_single <= 1; // arm simple pulse
                                reg_pulse_run <= 0;
                            end                     
                            trigger <= trigger_next;    // GPIO TRIGGER: VIA SOFTWARE, AUTO RUN
                        end
                        else
                        begin
                            trigger <= 0; // wait
                            reg_pulse_arm_single <= 0; // disable pulse
                            reg_pulse_run <= 0;
                        end
                    end
                    else // Scope Auto Mode, but auto trigger on S
                    begin
                        if (zero_x_s) // wait for ref zero crossing
                        begin
                            zero_x_s <= 0;
                            if (!trigger)
                            begin
                                reg_pulse_arm_single <= 0; // arm simple pulse
                                reg_pulse_run <= 1;
                            end                     
                            trigger <= trigger_next;    // GPIO TRIGGER: VIA SOFTWARE, AUTO RUN
                        end
                        else
                        begin
                            trigger <= 0; // wait
                            reg_pulse_arm_single <= 0;
                            reg_pulse_run <= 1; // keep pulses running 
                        end
                    end         
                end else begin
                    trigger <= reg_ext_trigger; // PL TRIGGER: (McBSP hardware trigger mode)
                    reg_pulse_arm_single <= 0;
                    reg_pulse_run <= 1;
                end
            
                if(trigger) // start job
                begin
                    sample_count <= 32'd0;
                    fir_next     <= 0;      // FIR start and hold
                    bram_reset   <= 1'b0;   // take BRAM controller out of reset
                    bram_next    <= 0;      // hold BRAM write next
                    finished     <= 1'b0;
                    dec_sms_next <= 3'd2; // go, init decimation
                    decimate_count      <= reg_ndecimate; // initialize samples for deciamtion first
                end
            end
            // ===============================================================================================
            2:    // Initiate Measuring Sum Values | Box Carr of Summing Data
            // ===============================================================================================
            begin   
                 // Summing: Measure, (Box Carr Average) / Decimate
                decimate_count <= decimate_count + 1; // next sample

                if (decimate_count < reg_ndecimate)
                begin
                    // decimate sum for all PAC-PLL channels
                    chPH <= chPH + $signed(S_AXIS4_tdata[SAXIS_4_DATA_WIDTH-1:0]);  // PHC Phase (24) =>  32
                    chDF <= chDF + reg_delta_freq;                                  // Freq (48) - Center (48)
                    chAM <= chAM + $signed(S_AXIS5_tdata[SAXIS_5_DATA_WIDTH-1:0]);  // AMC Amplitude (24) =>  32
                    chEX <= chEX + $signed(S_AXIS2_tdata[SAXIS_2_DATA_WIDTH-1:0]);  // AMC Exec Amplitude (32)
                    
                    case (reg_channel_selector) // channel selector
                        0:
                        begin
                            ch1 <= ch1 + $signed(S_AXIS1_tdata[SAXIS_1_DATA_WIDTH-1                       :                     0]); // IN1
                            ch2 <= ch2 + $signed(S_AXIS1_tdata[SAXIS_1_TDATA_WIDTH/2+SAXIS_1_DATA_WIDTH-1 : SAXIS_1_TDATA_WIDTH/2]); // IN2
                        end
                        1:
                        begin
                            ch1 <= ch1 + $signed(S_AXIS6_tdata[15:0]);    // IN1 AC Signal 16bit
                            ch2 <= ch2 + $signed(S_AXIS6_tdata[31:16]);   // IN1 DC Signal from Filter
                        end
                        6:
                        begin                                                             // dFreq Controller Test
                            ch2 <= ch2 + $signed(S_AXIS1S_tdata[32-1:0]);                         // DFC Control Value (32)
                        end
                        7: // DDR in #2 -> Hi16, #1 -> Lo16
                        begin
                            ch1[32-1:16] <= S_AXIS1_tdata[SAXIS_1_TDATA_WIDTH-1 : 0];
                            ch2[32-1:16] <= S_AXIS1_tdata[SAXIS_1_TDATA_WIDTH/2+SAXIS_1_DATA_WIDTH-1 : SAXIS_1_TDATA_WIDTH/2];
                        end
                        8: // LockIn X,Y
                        begin
                            if (LCK_ENABLE)
                            begin
                                ch1 <= ch1 + $signed(S_AXIS7_tdata[SAXIS_78_DATA_WIDTH-1:0]); // X
                                ch2 <= ch2 + $signed(S_AXIS8_tdata[SAXIS_78_DATA_WIDTH-1:0]); // Y
                                //ch1[15:0]    <= 0; //reg_McBSPbits; // DDR in #2 -> Hi16, #1 -> Lo16
                                //ch2[32-1:16] <= 0; //S_AXIS1_tdata[SAXIS_1_TDATA_WIDTH/2+SAXIS_1_DATA_WIDTH-1 : SAXIS_1_TDATA_WIDTH/2];
                            end
                        end
                    endcase

                    bram_next <= 0; // hold BRAM write next
                    fir_next  <= 0; // hold FIR
                end
                else begin
                    // normalize and store summed data data 
                    chPHs <= (chPH >>> reg_shift);   // PHC Phase (24) =>  32 *** not used ***
                    chDFs <= (chDF >>> reg_shift);   // Freq (48) - Center (48)
                    chAMs <= (chAM >>> reg_shift);   // AMPL ch3s
                    chEXs <= (chEX >>> reg_shift);   // EXEC ch4s

                    bram_next <= 1; // initate BRAM write data cycles
                    fir_next  <= 1; // load next into FIR
                    sample_count <= sample_count + 1;
                  
                    // check on sample count if in single shot mode, else continue  
                    if(reg_operation[1]) // Single Shot mode?
                    begin
                        // Single Shot, check for # samples
                        if (sample_count >= reg_nsamples) // run mode 1 single shot, finish, else LOOP/FIFO mode for ever
                        begin
                            dec_sms_next  <= 3'd4; // finished, needs reset to restart and re-arm.
                            dec_sms       <= 3'd4; // finished, needs reset to restart and re-arm.
                        end
                    end

                    // Initiate Measuring next Sum Values
                    decimate_count <= 32'd1; // first sample again
    
                    // initiate decimate sum for all PAC-PLL channels
                    chPH <= $signed(S_AXIS4_tdata[SAXIS_4_DATA_WIDTH-1:0]);  // PHC Phase (24) =>  32
                    chDF <= reg_delta_freq;                                  // Freq (48) - Center (48)
                    chAM <= $signed(S_AXIS5_tdata[SAXIS_5_DATA_WIDTH-1:0]);  // AMC Amplitude (24) =>  32
                    chEX <= $signed(S_AXIS2_tdata[SAXIS_2_DATA_WIDTH-1:0]);  // AMC Exec Amplitude (32)

                    // initialize and store pending custom CH1, CH2 selections
                    case (reg_channel_selector) // channels selector
                        0:
                        begin
                            ch1s  <= (ch1 >>> reg_shift);
                            ch2s  <= (ch2 >>> reg_shift);
                            ch1 <= $signed(S_AXIS1_tdata[SAXIS_1_DATA_WIDTH-1                       :                     0]); // IN1
                            ch2 <= $signed(S_AXIS1_tdata[SAXIS_1_TDATA_WIDTH/2+SAXIS_1_DATA_WIDTH-1 : SAXIS_1_TDATA_WIDTH/2]); // IN2
                        end
                        1:
                        begin
                            ch1s  <= (ch1 >>> reg_shift);
                            ch2s  <= (ch2 >>> reg_shift);
                            ch1 <= $signed(S_AXIS6_tdata[15:0]);    // IN1 AC Signal from DC Filter (16bit)
                            ch2 <= $signed(S_AXIS6_tdata[31:16]);   // IN1 DC Signal from DC Filter
                        end
                        2:
                        begin                                                       // AMC opt configuration   
                            ch1s  <= (chAM >>> reg_shift);                          // AMC Amplitude (24) =>  32
                            ch2s  <= (chEX >>> reg_shift);                          // AMC Exec Amplitude (32)
                        end
                        3:
                        begin                                                       // PHC opt configuration
                            ch1s  <= (chPH >>> reg_shift);                          // PHC Phase (24)
                            ch2s  <= (chDF >>> reg_shift);                          // PHC Freq (48) - Center (48)
                        end
                        4:
                        begin                                                       // Tuning Configuration
                            ch1s  <= (chPH >>> reg_shift);                          // PHC Phase (24)
                            ch2s  <= (chAM >>> reg_shift);                          // AMC Amplitude (24)
                        end
                        5:
                        begin                                                        // Operation for scanning
                            ch1s  <= (chPH >>> reg_shift);                          // PHC Phase (24)
                            ch2s  <= (chDF >>> reg_shift);                          // Freq (48) - Center (48)
                        end
                        6:
                        begin
                            ch1s  <= (chDF >>> reg_shift);                          // Freq (48) - Center (48)
                            ch2s  <= (ch2 >>> reg_shift);                           // store
                            ch2   <= $signed(S_AXIS1S_tdata[32-1:0]);               // dFreq Control Value 32
                        end
                        7: // DDR in #2 -> Hi16, #1 -> Lo16 -- ONLY GOOD WITH DEC=2 (external: DEC=1, SHR=0)
                        begin
                            ch1s  <= (ch1 >>> reg_shift);
                            ch2s  <= (ch2 >>> reg_shift);
                            ch1[16-1:0] <= S_AXIS1_tdata[SAXIS_1_TDATA_WIDTH-1 : 0];
                            ch2[16-1:0] <= S_AXIS1_tdata[SAXIS_1_TDATA_WIDTH/2+SAXIS_1_DATA_WIDTH-1 : SAXIS_1_TDATA_WIDTH/2];
                        end
                        8: // for Debug and Testing
                        begin
                            if (LCK_ENABLE)
                            begin
                                ch1s  <= (ch1 >>> reg_shift);
                                ch2s  <= (ch2 >>> reg_shift);
                                ch1 <= $signed(S_AXIS7_tdata[SAXIS_78_DATA_WIDTH-1:0]); // X
                                ch2 <= $signed(S_AXIS8_tdata[SAXIS_78_DATA_WIDTH-1:0]); // Y
                                //ch1 <= 0; // {reg_McBSPbits, reg_McBSPbits}; // DDR in #2 -> Hi16, #1 -> Lo16
                                //ch2 <= 0; // {{16'b0}, S_AXIS1_tdata[SAXIS_1_TDATA_WIDTH/2+SAXIS_1_DATA_WIDTH-1 : SAXIS_1_TDATA_WIDTH/2]};
                            end
                        end
                    endcase
                end
            end
            // ===============================================================================================
            4:    // Finished State -- wait for new operation setup
            begin
                zero_x_s     <= 0;
                bram_next    <= 0;    // hold BRAM write next
                fir_next     <= 0;    // hold FIR
                finished     <= 1'b1; // set finish flag
            end
            // ===============================================================================================
        endcase
    end
    
    // dFrequency [64] (44) =>  32 || signed, lower 31 bits, decimated -- must be in OPERATION SCANNING (=5) mode
    assign delta_frequency_monitor = chDFs[31:0]; 
    assign delta_frequency_direct  = reg_delta_freq[31:0]; 
    
    // assign CH1,2 from box carr filtering as selected and AM, EX
    assign M_AXIS_CH1_tdata  = ch1s[31:0];
    assign M_AXIS_CH1_tvalid = !reg_operation[4];
    assign M_AXIS_CH2_tdata  = ch2s[31:0];
    assign M_AXIS_CH2_tvalid = !reg_operation[4];
    assign M_AXIS_CH3_tdata  = chAMs[31:0];
    assign M_AXIS_CH3_tvalid = !reg_operation[4];
    assign M_AXIS_CH4_tdata  = chEXs[31:0];
    assign M_AXIS_CH4_tvalid = !reg_operation[4];

    // must be in STREAMING MODE for contineous op
    assign M_AXIS_DFREQ_DEC_tdata  = chDFs[31:0]; // dFrequency [64] (44) =>  32 || signed, lower 31 bits
    assign M_AXIS_DFREQ_DEC_tvalid = 1;
    assign M_AXIS_DFREQ_DEC2_tdata  = chDFs[31:0]; // dFrequency [64] (44) =>  32 || signed, lower 31 bits
    assign M_AXIS_DFREQ_DEC2_tvalid = 1;
    
endmodule
