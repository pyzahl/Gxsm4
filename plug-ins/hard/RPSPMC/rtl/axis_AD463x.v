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
// Create Date: 01/11/2025 12:04:22 AM
// Design Name:    part of RPSPMC
// Module Name:    axis AD463x
// Project Name:   RPSPMC 4 GXSM
// Target Devices: Zynq z7020
// Tool Versions:  Vivado 2023.1
// Description:    AD463x SPI interface logic and streaming to AXIS
// 
// Dependencies: 
// 
// Revision:
// Revision 0.01 - File Created
// Additional Comments:
// 
//////////////////////////////////////////////////////////////////////////////////


module axis_AD463x #(
    parameter AD463x_address = 50000,
    parameter NUM_DAC = 2,
    parameter DAC_DATA_WIDTH = 24,
    parameter DAC_WORD_WIDTH = 32,
    parameter SAXIS_TDATA_WIDTH = 32
)
(
    input [32-1:0]  config_addr,
    input [512-1:0] config_data,
    (* X_INTERFACE_PARAMETER = "ASSOCIATED_CLKEN a_clk, ASSOCIATED_BUSIF M_AXIS1:M_AXIS2" *)
    input a_clk,

    output wire [SAXIS_TDATA_WIDTH-1:0]  S_AXIS1_tdata,
    output wire                          S_AXIS1_tvalid,
    output wire [SAXIS_TDATA_WIDTH-1:0]  S_AXIS2_tdata,
    output wire                          S_AXIS2_tvalid,
    
    (* X_INTERFACE_PARAMETER = "FREQ_HZ 30000000, ASSOCIATED_CLKEN wire_SPI_sck, ASSOCIATED_BUSIF wire_SPI" *)
    output wire wire_SPI_sck,
    output wire wire_SPI_cs,
    output wire wire_SPI_sdi,
    output wire wire_SPI_reset,
    output wire wire_SPI_cnv,
    input wire  wire_SPI_busy,
    input wire [NUM_DAC-1:0] wire_SPI_sdn,
    
    output wire [31:0] mon0,
    output wire [31:0] mon1,
    
    output wire ready
    );
    
    reg configuration_mode=0;
    reg configuration_send=0;
    
    
    reg SPI_sck=0;   // SPI CLK pin AD463x
    reg SPI_sdi=1;   // SPI SDI pin AD463x
    reg SPI_cs=1;    // SPI CS  pin AD463x
    reg SPI_cnv=0;   // SPI CVN pin AD463x
    reg SPI_reset=0; // SPI RESET pin AD463x
    reg SPI_sdn [NUM_DAC-1:0]; // SPI DATA ADC[1..2] pin AD463x
    
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
            SPI_sdn[i]=0;
      end     
    end

    // Latch data from AXIS when new
    always @(posedge a_clk) // @*   posedge PMD_clk)   a_clk = 125MHz
    begin

        // module configuration
        case (config_addr)
        AD463x_address:
        begin
            configuration_mode <= config_data[0*32 : 0*32]; // up to 32bit 
            configuration_send <= config_data[1*32 : 1*32];
        end   
        endcase

        // WORK IN PROGRESS: HOOK UP SPI, CREATE CONFIGURATION/TEST OP MODE AND STREAMING TO AXIS 
        if (0)
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
                    ;
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
                end
            end     

            4: // always @(posedge PMD_clk)  // SPI transmit: data setup edge
            begin
                SPI_sck <= 1;
                //SPI_sync   <= sync;
                //PMD_dac[0] <= reg_dac_data_buf[0][frame_bit_counter];
                //PMD_dac[1] <= reg_dac_data_buf[1][frame_bit_counter];
            end
            
            0:   // always @(negedge PMD_clk) // SPI clock, prepare data, manage
            begin
                SPI_sck <= 0;

                // clkr <= 0; // generate clkr
                // Detect Frame Sync
                case (state_load_dacs)
                    0: // reset mode, wait for new data
                    begin
                        sync <= 1;
                        if (reg_dac_data_buf[0] != reg_dac_data[0] || reg_dac_data_buf[1] != reg_dac_data[1])
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
    end

    
assign ready = (state_load_dacs == 1'b0000);

assign wire_SPI_sck   = SPI_sck;
assign wire_SPI_sync  = SPI_cs;
assign wire_SPI_reset = SPI_reset;
assign wire_SPI_cnv   = SPI_cnv;
assign wire_SPI_sdi   = SPI_sdi;
  
assign mon0 = { reg_dac_data_buf[0][19:0], {(32-24){1'b0}}, reg_dac_data_buf[0][23:20] };
assign mon1 = { reg_dac_data_buf[1][19:0], {(32-24){1'b0}}, reg_dac_data_buf[1][23:20] };
    
endmodule
