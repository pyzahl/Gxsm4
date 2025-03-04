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
    parameter NUM_LANES = 4,
    parameter DAC_DATA_WIDTH = 24,
    parameter DAC_WORD_WIDTH = 32,
    parameter SAXIS_TDATA_WIDTH = 32
)
(
    input [32-1:0]  config_addr,
    input [512-1:0] config_data,
    (* X_INTERFACE_PARAMETER = "ASSOCIATED_CLKEN a_clk, ASSOCIATED_BUSIF M_AXIS1:M_AXIS2" *)
    input a_clk,

    output wire [SAXIS_TDATA_WIDTH-1:0]  M_AXIS1_tdata,
    output wire                          M_AXIS1_tvalid,
    output wire [SAXIS_TDATA_WIDTH-1:0]  M_AXIS2_tdata,
    output wire                          M_AXIS2_tvalid,
    
    (* X_INTERFACE_PARAMETER = "FREQ_HZ 30000000, ASSOCIATED_CLKEN wire_SPI_sck, ASSOCIATED_BUSIF wire_SPI" *)
    output wire wire_SPI_sck,
    output wire wire_SPI_cs,
    output wire wire_SPI_sdi,
    output wire wire_SPI_reset,
    output wire wire_SPI_cnv,
    input wire  wire_SPI_busy,
    input wire [NUM_DAC*NUM_LANES-1:0] wire_SPI_sdn,
    
    output wire [31:0] mon0,
    output wire [31:0] mon1,
    output wire [31:0] cfg_word,
    
    output wire ready
    );
    
    reg configuration_mode=0;
    reg configuration_rw=0;
    reg [32-1:0] configuration_addr_data_write=0;
    reg [32-1:0] configuration_addr_data_read=0;
    reg manual_cnv=0;
    reg stream_cnv=0;
    reg run=0;
    
    reg SPI_sck=0;   // SPI CLK pin AD463x
    reg sck=0;       // SPI CLOCK dly
    reg sck1=0;       // SPI CLOCK dly
    reg SPI_sdi=0;   // SPI SDI pin AD463x
    reg SPI_cs=1;    // SPI CS  pin AD463x
    reg cs=0;
    reg cs1=0;
    reg SPI_cnv=0;   // SPI CVN pin AD463x
    reg SPI_reset=0; // SPI RESET pin AD463x
    
    reg [DAC_WORD_WIDTH-1:0] reg_dac_data_ser[NUM_DAC-1:0];
    reg [DAC_WORD_WIDTH-1:0] reg_dac_data[NUM_DAC-1:0];
    reg [64-1:0] reg_data_in;
    reg [7:0] n_bits=24;
    
    reg cfg_mode_job=1;
    reg [5-1:0] frame_bit_counter=0;
    reg [4-1:0] state_rw=0;
    reg [4-1:0] state_single=0;
    reg [4-1:0] state_stream=0;
    
    reg rdecii = 0; // MAX SCK 80MHz
    reg [8:0] rdecii_cfg_n = 16;
    reg [8:0] rdecii_cfg = 0; // MAX SCK 20MHz 3:0    1 2 4 8

    integer i;
    initial begin
      for (i=0;i<NUM_DAC;i=i+1)
      begin
            reg_dac_data[i] = 0;
      end     
    end

    // Latch data from AXIS when new
    always @(posedge a_clk) // @*   posedge PMD_clk)   a_clk = 125MHz
    begin

        // module configuration
        if (config_addr == AD463x_address)
        begin
            configuration_mode       <= config_data[0:0]; //  1: activate configuration mode 
            configuration_rw         <= config_data[1:1]; //  2: read/write config
                                                          // enter configuration mode by CS=0, SCK start: SDI=1,0,1 min 4 SCK POS EDGES (up to 24 ok) = READ 01xxx
                                                          // exit configuration mode by writing 0x01 to Register Address 0x0014
            manual_cnv               <= config_data[3:3]; //  8: trigger one manual convert
            stream_cnv               <= config_data[4:4]; // 10: start auto convert and stream to AXI
            SPI_reset                <= ~config_data[7:7]; // 80: manual reset
            configuration_addr_data_write <= config_data[1*32+32-1 : 1*32]; // Config Read/Write Address: Bit 23 is "A15": R=1 (send first), Bit 22:8A are A14..A0  (16 BIT total), Then 8bit DATA in on SD0 from SCK17 .. 24
            //;                                                             // Config Read/Write Address: Bit 23 is "A15": W=0 (send first), Bit 22:8: are A14..A0, (16 BIT total), Then 8Bit DATA 7:0 are data D7:0 (24 BIT total Addr + Data)
            n_bits                   <= config_data[2*32+8-1  : 2*32]; // number bytes to read/write in config mode => in bits
            rdecii_cfg_n             <= config_data[3*32+8-1  : 3*32] ? config_data[3*32+8-1  : 3*32] : 10;

            if (config_data[0:0]) 
                cfg_mode_job <= 1;
            if (~config_data[4:4]) 
                state_stream <= 0;
            
            cs <= 1;
        end
        else
        begin      
            if (configuration_mode && !rdecii && !state_rw && !state_single && !state_stream && cfg_mode_job)
            begin
                cfg_mode_job <= 0; // only once, until reset via new config request for next op!
                cs <= 0; // initiate and assert CS
                
                if (manual_cnv)
                begin
                    state_stream <= 0; // disable streaming
                    state_single <= 1;
                end
                else
                begin
                    state_stream <= stream_cnv ? 1:0; // enable streaming
                end
                
                if (configuration_rw)
                begin
                    state_rw <= 1;
                    frame_bit_counter <= n_bits-1; // send whole word 24/32 bits
                    reg_data_in <= 0; // clear data
                    reg_dac_data[0] <= 0;
                end
                rdecii <= 1; // start with setup data
                rdecii_cfg <= rdecii_cfg_n; // 1
            end
            else
            begin
                if (configuration_mode || state_stream)
                begin
                    rdecii_cfg <= rdecii_cfg-1;
                    if (rdecii_cfg == 0)
                    begin
                        rdecii_cfg <= rdecii_cfg_n;
                        rdecii <= ~rdecii; // 0,1,0,1,...

                        sck1 <= sck;
                        SPI_sck <= sck1; // delay SCK to setup
                        SPI_cs <= cs;    // delay 1/2 SCK
                       
                        case (rdecii)
                            1: // @(posedge SCK)  // SPI transmit/receive: data setup edge
                            begin
                                if (!cs && (state_rw || state_single))
                                begin
                                    sck <= 1; // gen SPI SCK, action state to latch data in/out
                                end
                                else                                 
                                    cs <= 1; // deassert
                            end
                            
                            0:   // @(negedge SCK) // SPI clock, prepare data, manage
                            begin
                                sck <= 0; // gen SPI SCK, setup state
                
                                if (state_rw)
                                begin
                                    cs  <= 0; // assert
                                    SPI_sdi <= configuration_addr_data_write [frame_bit_counter];
                                    reg_data_in[frame_bit_counter] <= wire_SPI_sdn[0];
                                    if (!frame_bit_counter)
                                    begin
                                        reg_dac_data[0] <= { reg_data_in[32-1:1], wire_SPI_sdn[0] };
                                        configuration_addr_data_read <= { reg_data_in[32-1:1], wire_SPI_sdn[0] };
                                        state_rw <= 0; // completed
                                    end
                                    else
                                        frame_bit_counter <= frame_bit_counter - 1;
                                end
                                else if (state_stream && !state_single)
                                begin
                                    state_single <= 1;
                                end
                                else
                                begin              
                                    case (state_single) // start single conversion and read
                                        1: // initiate, clear data
                                        begin
                                            cs  <= 0; // assert
                                            SPI_cnv <= 1; // start CNV
                                            reg_dac_data_ser[0] <= 0;
                                            reg_dac_data_ser[1] <= 0;
                                            state_single <= 2;
                                        end                 
                                        2:  // wait for conversion result
                                        begin
                                            SPI_cnv <= 0; // wait busy
                                            if (!wire_SPI_busy)
                                            begin
                                                frame_bit_counter  <= n_bits-1;
                                                state_single <= 3;
                                            end
                                        end                 
                                        3: // serialize read address data
                                        begin
                                            reg_dac_data_ser[frame_bit_counter][0] <= wire_SPI_sdn[0];
                                            reg_dac_data_ser[frame_bit_counter][1] <= wire_SPI_sdn[1];
                                            if (!frame_bit_counter)
                                            begin
                                                // completed, store data -- use latest last bit now!
                                                reg_dac_data[0] <= { reg_dac_data_ser[0], wire_SPI_sdn[0] }; // complete now!
                                                reg_dac_data[1] <= { reg_dac_data_ser[1], wire_SPI_sdn[1] };
                                                state_single <= 4; // completed
                                            end
                                            else                
                                                frame_bit_counter <= frame_bit_counter - 1;
                                        end
                                        4:  // quiet period
                                        begin
                                            state_single <= 0;
                                        end
                                    endcase
                                end
                            end
                        endcase
                    end                 
                end
                else 
                begin
                    SPI_sdi <= 0;
                    rdecii_cfg <= 0;
                    rdecii <= ~rdecii; // 0,1,0,1,...
                end
            end             
        end
    end
    
assign M_AXIS1_tdata  = reg_dac_data[0];
assign M_AXIS1_tvalid = state_stream;
assign M_AXIS2_tdata  = reg_dac_data[1];
assign M_AXIS2_tvalid = state_stream;
    
assign ready = (!state_rw && !state_single && !state_stream);

assign wire_SPI_sck   = SPI_sck;
assign wire_SPI_cs    = SPI_cs;
assign wire_SPI_reset = SPI_reset;
assign wire_SPI_cnv   = SPI_cnv;
assign wire_SPI_sdi   = SPI_sdi;
  
assign mon0 = reg_dac_data[0];
assign mon1 = reg_dac_data[1];
assign cfg_word = configuration_addr_data_read;
    
endmodule
