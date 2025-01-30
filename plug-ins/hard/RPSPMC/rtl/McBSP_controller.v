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
// Create Date: 11/26/2017 08:20:47 PM
// Design Name:    part of RPSPMC
// Module Name:    McBSP_controller
// Project Name:   RPSPMC 4 GXSM
// Target Devices: Zynq z7020
// Tool Versions:  Vivado 2023.1
// Description:    IO connect
//  -- obsoleted -- manage hi speed serial connection to 
//                  SignalRanger MK3 DSP
// 
// Dependencies: 
// 
// Revision:
// Revision 0.01 - File Created
// Additional Comments:
// 
/*
 */

//////////////////////////////////////////////////////////////////////////////////


module McBSP_controller #(
    parameter WORDS_PER_FRAME = 8,
    parameter BITS_PER_WORD = 32,
    parameter SAXIS_TDATA_WIDTH = 32
)
(
    (* X_INTERFACE_PARAMETER = "ASSOCIATED_CLKEN a_clk, ASSOCIATED_BUSIF S_AXIS1:S_AXIS2:S_AXIS3:S_AXIS4:S_AXIS5:S_AXIS6:S_AXIS7:S_AXIS8" *)
    input a_clk,
    input mcbsp_clk,
    input mcbsp_frame_start,
    input mcbsp_data_rx,
    input mcbsp_data_nrx,   // N-th RedPitaya Slave/Chained Link forwarding to McBSP Maser Slave -- optional

    output wire mcbsp_data_clkr, // clock return
    output wire mcbsp_data_tx,
    output wire mcbsp_data_fsx, // optional/debug FSX start
    output wire mcbsp_data_frm, // optional/debug Frame

    output wire McBSP_sending,
    // input a_resetn,
    
    output wire trigger,
    //output wire [(WORDS_PER_FRAME * BITS_PER_WORD) -1 : 0] dataset_read,
    
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
    input wire [SAXIS_TDATA_WIDTH-1:0]  S_AXIS7_tdata,
    input wire                          S_AXIS7_tvalid,
    input wire [SAXIS_TDATA_WIDTH-1:0]  S_AXIS8_tdata,
    input wire                          S_AXIS8_tvalid

    //output wire [32-1:0] dbgA,
    //output wire [32-1:0] dbgB,
    //output wire [16-1:0]  dbgC

    );
    
    reg frame_start=0;
    reg [10-1:0] frame_bit_counter;
    reg [(WORDS_PER_FRAME * BITS_PER_WORD) -1 : 0] reg_data_src;
    reg [(WORDS_PER_FRAME * BITS_PER_WORD) -1 : 0] data;
    reg [(WORDS_PER_FRAME * BITS_PER_WORD) -1 : 0] data_in;
    reg [(WORDS_PER_FRAME * BITS_PER_WORD) -1 : 0] data_read;
    reg tx=0;
    //reg clkr=0;
    reg reg_data_rx=0;

    reg rtrigger=0;
    
    // reg [7:0] data_counter=0;

    // Latch data from AXIS when new
    always @(*)
    begin
        reg_data_src[(8 * BITS_PER_WORD) - 1 : (8 * BITS_PER_WORD)-32] <= S_AXIS1_tdata;
        reg_data_src[(7 * BITS_PER_WORD) - 1 : (7 * BITS_PER_WORD)-32] <= S_AXIS2_tdata;
        reg_data_src[(6 * BITS_PER_WORD) - 1 : (6 * BITS_PER_WORD)-32] <= S_AXIS3_tdata;
        reg_data_src[(5 * BITS_PER_WORD) - 1 : (5 * BITS_PER_WORD)-32] <= S_AXIS4_tdata;
        reg_data_src[(4 * BITS_PER_WORD) - 1 : (4 * BITS_PER_WORD)-32] <= S_AXIS5_tdata;
        reg_data_src[(3 * BITS_PER_WORD) - 1 : (3 * BITS_PER_WORD)-32] <= S_AXIS6_tdata;
        reg_data_src[(2 * BITS_PER_WORD) - 1 : (2 * BITS_PER_WORD)-32] <= S_AXIS7_tdata;
        reg_data_src[(1 * BITS_PER_WORD) - 1 : (1 * BITS_PER_WORD)-32] <= S_AXIS8_tdata;
    end

    always @(negedge mcbsp_clk) // McBSP read edge
    begin
            //clkr <= 0; // generate clkr
            // Detect Frame Sync
            if (mcbsp_frame_start && ~frame_start)
            begin
                rtrigger <= 1; // set frame start and trigger
                frame_start <= 1;
                frame_bit_counter <= 10'd255; // ((WORDS_PER_FRAME * BITS_PER_WORD) - 1);
                // wait for aclk edge (very fast vs. mcbsp_clk)
                //@(posedge a_clk) // dose not synth.
                // Latch Axis Data at Frame Sync Pulse and initial data serialization
                data <= reg_data_src;
                // data[(1 * BITS_PER_WORD) - 1 : (1 * BITS_PER_WORD)-32] <= data_read[(8 * BITS_PER_WORD) - 1 : (8 * BITS_PER_WORD)-32]; // LOOP BACK TEST, index: col,row
                // data_counter <= data_counter + 1;
            end else
            begin
                if (frame_start)
                begin
                    rtrigger <= 0;
                    // read data bit
                    data_in[frame_bit_counter] <= mcbsp_data_rx;
                    reg_data_rx                <= mcbsp_data_rx;
            
                    // completed?
                    if (frame_bit_counter == 10'b0)
                    begin
                        frame_start <= 0;
                    end else
                    begin
                        // next bit
                        frame_bit_counter <= frame_bit_counter - 1;
                    end
                end else
                begin
                    data_read <= data_in; // buffer read data
                end   
            end
    end

    always @(posedge mcbsp_clk) // McBSP transmit data setup edge
     begin
        // generate clkr and setup data bit
        // clkr <= 1;
        tx   <= data[frame_bit_counter];
    end
    
    assign mcbsp_data_clkr = mcbsp_clk; // clkr;
    assign mcbsp_data_tx = tx;
    assign trigger = rtrigger;

    assign mcbsp_data_frm = frame_start;
    assign mcbsp_data_fsx = mcbsp_frame_start;

    assign McBSP_sending = frame_start;

    // testing only
    // assign dbgA = {{(32-3){1'b0}}, reg_data_rx, frame_start, clkr}; //,  {(8){1'b0}}, data_counter[7:0] }; //{{(32-3){1'b0}}, mcbsp_clk, mcbsp_frame_start, mcbsp_data_rx};
    // assign dbgB = data_read[31:0]; //S_AXIS1_tdata;
    // assign dbgC = {data_read[16-5-8-1:0], frame_bit_counter[7:0], rtrigger, tx, reg_data_rx, frame_start, clkr};

    //assign dataset_read = data_read;

endmodule


/*

[DRC PDRC-153] Gated clock check: Net system_i/PS_data_transport/McBSP_controller_0/inst/data_in_reg[2]_LDC_i_1_n_0 is a gated clock net sourced by a combinational pin system_i/PS_data_transport/McBSP_controller_0/inst/data_in_reg[2]_LDC_i_1/O, cell system_i/PS_data_transport/McBSP_controller_0/inst/data_in_reg[2]_LDC_i_1. This is not good design practice and will likely impact performance. For SLICE registers, for example, use the CE pin to control the loading of data.

[DRC PDRC-153] Gated clock check: Net system_i/PS_data_transport/McBSP_controller_0/inst/data_read_reg[0]_LDC_i_1_n_0 is a gated clock net sourced by a combinational pin system_i/PS_data_transport/McBSP_controller_0/inst/data_read_reg[0]_LDC_i_1/O, cell system_i/PS_data_transport/McBSP_controller_0/inst/data_read_reg[0]_LDC_i_1. This is not good design practice and will likely impact performance. For SLICE registers, for example, use the CE pin to control the loading of data.

[DRC PDRC-153] Gated clock check: Net system_i/PS_data_transport/McBSP_controller_0/inst/data_read_reg[1]_LDC_i_1_n_0 is a gated clock net sourced by a combinational pin system_i/PS_data_transport/McBSP_controller_0/inst/data_read_reg[1]_LDC_i_1/O, cell system_i/PS_data_transport/McBSP_controller_0/inst/data_read_reg[1]_LDC_i_1. This is not good design practice and will likely impact performance. For SLICE registers, for example, use the CE pin to control the loading of data.

[DRC PDRC-153] Gated clock check: Net system_i/PS_data_transport/McBSP_controller_0/inst/data_read_reg[2]_LDC_i_1_n_0 is a gated clock net sourced by a combinational pin system_i/PS_data_transport/McBSP_controller_0/inst/data_read_reg[2]_LDC_i_1/O, cell system_i/PS_data_transport/McBSP_controller_0/inst/data_read_reg[2]_LDC_i_1. This is not good design practice and will likely impact performance. For SLICE registers, for example, use the CE pin to control the loading of data.

[DRC PDRC-153] Gated clock check: Net system_i/PS_data_transport/McBSP_controller_0/inst/data_reg[0]_LDC_i_1_n_0 is a gated clock net sourced by a combinational pin system_i/PS_data_transport/McBSP_controller_0/inst/data_reg[0]_LDC_i_1/O, cell system_i/PS_data_transport/McBSP_controller_0/inst/data_reg[0]_LDC_i_1. This is not good design practice and will likely impact performance. For SLICE registers, for example, use the CE pin to control the loading of data.

[DRC PDRC-153] Gated clock check: Net system_i/PS_data_transport/McBSP_controller_0/inst/data_reg[100]_LDC_i_1_n_0 is a gated clock net sourced by a combinational pin system_i/PS_data_transport/McBSP_controller_0/inst/data_reg[100]_LDC_i_1/O, cell system_i/PS_data_transport/McBSP_controller_0/inst/data_reg[100]_LDC_i_1. This is not good design practice and will likely impact performance. For SLICE registers, for example, use the CE pin to control the loading of data.
...
*/