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
// Create Date: 08/14/2023 06:42:21 PM
// Design Name:    part of RPSPMC
// Module Name:    axis_AD5791
// Project Name:   RPSPMC 4 GXSM
// Target Devices: Zynq z7020
// Tool Versions:  Vivado 2023.1
// Description:    AD5791 (or generic SPI) multi channel PMOD control and to AXIS streaming
// 
// Dependencies: 
// 
// Revision:
// Revision 0.01 - File Created
// Additional Comments:
// 
//////////////////////////////////////////////////////////////////////////////////


module axis_AD5791 #(
    parameter USE_RP_DIGITAL_IO = 0,
    parameter NUM_DAC = 6,
    parameter DAC_DATA_WIDTH = 20,
    parameter DAC_WORD_WIDTH = 24,
    parameter SAXIS_TDATA_WIDTH = 32
)
(
    (* X_INTERFACE_PARAMETER = "ASSOCIATED_CLKEN a_clk, ASSOCIATED_BUSIF S_AXIS1:S_AXIS2:S_AXIS3:S_AXIS4:S_AXIS5:S_AXIS6:S_AXISCFG" *)
    input a_clk,
    input wire [SAXIS_TDATA_WIDTH-1:0]  S_AXIS1_tdata,
    input wire                          S_AXIS1_tvalid,
    input wire [SAXIS_TDATA_WIDTH-1:0]  S_AXIS2_tdata,
    input wire                          S_AXIS2_tvalid,
    input wire [SAXIS_TDATA_WIDTH-1:0]  S_AXIS3_tdata,
    input wire                          S_AXIS3_tvalid,
    input wire [SAXIS_TDATA_WIDTH-1:0]  S_AXIS4_tdata,
    input wire                          S_AXIS4_tvalid,
    input wire [SAXIS_TDATA_WIDTH-1:0]  S_AXIS5_tdata,
    input wire                          S_AXIS5_tvalid,
    input wire [SAXIS_TDATA_WIDTH-1:0]  S_AXIS6_tdata,
    input wire                          S_AXIS6_tvalid,

    input wire [SAXIS_TDATA_WIDTH-1:0]  S_AXISCFG_tdata,
    input wire                          S_AXISCFG_tvalid,
    
    input wire configuration_mode,
    input wire [2:0] configuration_axis,
    input wire configuration_send,
    
    (* X_INTERFACE_PARAMETER = "FREQ_HZ 30000000, ASSOCIATED_CLKEN wire_PMD_clk, ASSOCIATED_BUSIF wire_PMD" *)
    output wire wire_PMD_clk,
    output wire wire_PMD_sync,
    output wire [NUM_DAC-1:0] wire_PMD_dac,
    
    output wire [31:0] mon0,
    output wire [31:0] mon1,
    output wire [31:0] mon2,
    output wire [31:0] mon3,
    output wire [31:0] mon4,
    output wire [31:0] mon5,
    
    output wire ready
    );
    
    
    reg PMD_clk=0;   // PMOD SPI CLK
    reg PMD_sync=1;  // PMOD SPI SYNC
    reg PMD_dac [NUM_DAC-1:0]; // PMOD SPI DATA DAC[1..4]
    
    reg [DAC_WORD_WIDTH-1:0] reg_dac_data[NUM_DAC-1:0];
    reg [DAC_WORD_WIDTH-1:0] reg_dac_data_buf[NUM_DAC-1:0];
    
    reg cfg_mode_job=0;
    reg sync=1;
    reg start=0;
    reg [6-1:0] frame_bit_counter=0;
    reg [4-1:0] state_load_dacs=0;
    
    reg [3-1:0] rdecii = 0; //4bit 1/4clk for testing slow  ** 2bit: PMD clock 31.25MHz max = 125MHz/4

    integer i;
    initial begin
      for (i=0;i<NUM_DAC;i=i+1)
      begin
            reg_dac_data[i] = 0;
            reg_dac_data_buf[i] = 0;
            PMD_dac[i]=0;
      end     
    end

    // Latch data from AXIS when new
    always @(posedge a_clk) // @*   posedge PMD_clk)   a_clk = 125MHz
    begin
        //if (rdecii == 4) // [2] 1/4 => 30MHz  , [4] 1/16: testing slow
        //begin
        //    rdecii <= 0;
        //    PMD_clk <= ~PMD_clk;
        //end
        rdecii <= rdecii+1; // 3bits 0...7...   2bits 0...3...
        case (rdecii)
            // 0: PMD_CLK=0 ... 4: PMD_CLK=1 ... 7: rdecii=0
            1:  // load data from FPGA AXIS to buffer reg for processing. DAC is fetching this register at SYNC start, any potenial new data inbetween is ignored 
            begin
                if (configuration_mode)
                begin
                    if (S_AXISCFG_tvalid)
                    begin
                        reg_dac_data[configuration_axis[1:0]] <= S_AXISCFG_tdata[DAC_WORD_WIDTH-1:0];
                    end
                end
                else
                begin
                    if (S_AXIS1_tvalid)
                    begin            
                        //                       send to data register 24 bits:         0 001 20-bit-data
                        reg_dac_data[0] <= {{(DAC_WORD_WIDTH-DAC_DATA_WIDTH-1 ){1'b0}}, 1'b1, S_AXIS1_tdata[SAXIS_TDATA_WIDTH-1:SAXIS_TDATA_WIDTH-DAC_DATA_WIDTH]};
                    end
                    if (S_AXIS2_tvalid)
                    begin            
                        reg_dac_data[1] <= {{(DAC_WORD_WIDTH-DAC_DATA_WIDTH-1 ){1'b0}}, 1'b1, S_AXIS2_tdata[SAXIS_TDATA_WIDTH-1:SAXIS_TDATA_WIDTH-DAC_DATA_WIDTH]};
                    end
                    if (S_AXIS3_tvalid)
                    begin            
                        reg_dac_data[2] <= {{(DAC_WORD_WIDTH-DAC_DATA_WIDTH-1 ){1'b0}}, 1'b1, S_AXIS3_tdata[SAXIS_TDATA_WIDTH-1:SAXIS_TDATA_WIDTH-DAC_DATA_WIDTH]};
                    end
                    if (S_AXIS4_tvalid)
                    begin            
                        reg_dac_data[3] <= {{(DAC_WORD_WIDTH-DAC_DATA_WIDTH-1 ){1'b0}}, 1'b1, S_AXIS4_tdata[SAXIS_TDATA_WIDTH-1:SAXIS_TDATA_WIDTH-DAC_DATA_WIDTH]};
                    end
                    if (S_AXIS5_tvalid)
                    begin            
                        reg_dac_data[4] <= {{(DAC_WORD_WIDTH-DAC_DATA_WIDTH-1 ){1'b0}}, 1'b1, S_AXIS5_tdata[SAXIS_TDATA_WIDTH-1:SAXIS_TDATA_WIDTH-DAC_DATA_WIDTH]};
                    end
                    if (S_AXIS6_tvalid)
                    begin            
                        reg_dac_data[5] <= {{(DAC_WORD_WIDTH-DAC_DATA_WIDTH-1 ){1'b0}}, 1'b1, S_AXIS6_tdata[SAXIS_TDATA_WIDTH-1:SAXIS_TDATA_WIDTH-DAC_DATA_WIDTH]};
                    end
                end
            end     

            4: // always @(posedge PMD_clk)  // SPI transmit: data setup edge
            begin
                PMD_clk <= 1;
                PMD_sync   <= sync;
                PMD_dac[0] <= reg_dac_data_buf[0][frame_bit_counter];
                PMD_dac[1] <= reg_dac_data_buf[1][frame_bit_counter];
                PMD_dac[2] <= reg_dac_data_buf[2][frame_bit_counter];
                PMD_dac[3] <= reg_dac_data_buf[3][frame_bit_counter];
                PMD_dac[4] <= reg_dac_data_buf[4][frame_bit_counter];
                PMD_dac[5] <= reg_dac_data_buf[5][frame_bit_counter];
            end
            
            0:   // always @(negedge PMD_clk) // SPI clock, prepare data, manage
            begin
                PMD_clk <= 0;

                // clkr <= 0; // generate clkr
                // Detect Frame Sync
                case (state_load_dacs)
                    0: // reset mode, wait for new data
                    begin
                        sync <= 1;
                        if (reg_dac_data_buf[0] != reg_dac_data[0] || reg_dac_data_buf[1] != reg_dac_data[1] || reg_dac_data_buf[2] != reg_dac_data[2] || reg_dac_data_buf[3] != reg_dac_data[3] || reg_dac_data_buf[4] != reg_dac_data[4] || reg_dac_data_buf[5] != reg_dac_data[5])
                        begin
                            if (configuration_mode)
                            begin // load only on configuration_send pos edge
                                if (configuration_send)
                                begin
                                    if (!cfg_mode_job)
                                    begin
                                        cfg_mode_job <= 1;
                                        state_load_dacs <= 1;
                                    end
                                end
                                else
                                begin
                                    cfg_mode_job <= 0; // do not repat sending! "send bit" must be reset while in config mode
                                end
                            end
                            else // auto load and start sending on new data
                            begin
                                state_load_dacs <= 1;
                            end
                        end
                    end
                    1: // load dac start
                    begin
                        sync <= 1;
                        frame_bit_counter <= 10'd23; // 24 bits to send
            
                        // Latch Axis Data at Frame Sync Pulse and initial data serialization
                        reg_dac_data_buf[0] <= reg_dac_data[0];
                        reg_dac_data_buf[1] <= reg_dac_data[1];
                        reg_dac_data_buf[2] <= reg_dac_data[2];
                        reg_dac_data_buf[3] <= reg_dac_data[3];
                        reg_dac_data_buf[4] <= reg_dac_data[4];
                        reg_dac_data_buf[5] <= reg_dac_data[5];
        
                        state_load_dacs <= 2;
                    end
                    2:
                    begin
                        sync <= 0;
                        state_load_dacs <= 3; // one more sync high cycle
                    end
                    3:
                    begin
                        // completed?
                        if (frame_bit_counter == 10'b0)
                        begin
                            state_load_dacs <= 4; // finish
                        end else
                        begin
                            // next bit
                            frame_bit_counter <= frame_bit_counter - 1;
                        end
                    end   
                    4:
                    begin
                        sync <= 1;
                        state_load_dacs <= 0; // completed, standy next
                    end   
                endcase
            end
        endcase
    end

    
assign ready = (state_load_dacs == 1'b0000);

assign wire_PMD_clk  = PMD_clk;
assign wire_PMD_sync = PMD_sync;
assign wire_PMD_dac  = { PMD_dac[5], PMD_dac[4], PMD_dac[3], PMD_dac[2], PMD_dac[1], PMD_dac[0] };
  
assign mon0 = { reg_dac_data_buf[0][19:0], {(32-24){1'b0}}, reg_dac_data_buf[0][23:20] };
assign mon1 = { reg_dac_data_buf[1][19:0], {(32-24){1'b0}}, reg_dac_data_buf[1][23:20] };
assign mon2 = { reg_dac_data_buf[2][19:0], {(32-24){1'b0}}, reg_dac_data_buf[2][23:20] };
assign mon3 = { reg_dac_data_buf[3][19:0], {(32-24){1'b0}}, reg_dac_data_buf[3][23:20] };
assign mon4 = { reg_dac_data_buf[4][19:0], {(32-24){1'b0}}, reg_dac_data_buf[4][23:20] };
assign mon5 = { reg_dac_data_buf[5][19:0], {(32-24){1'b0}}, reg_dac_data_buf[5][23:20] };
    
endmodule
