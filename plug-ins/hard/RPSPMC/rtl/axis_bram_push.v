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


module axis_bram_push #(
    parameter integer BRAM_DATA_WIDTH = 32,
    parameter integer BRAM_ADDR_WIDTH = 16
)
(
    input wire [32-1:0] ch1s,
    input wire [32-1:0] ch2s,
    input wire push_next,
    input wire BR_wpos_reset,
    
    // INPUT
    (* X_INTERFACE_PARAMETER = "FREQ_HZ 125000000, ASSOCIATED_CLKEN a2_clk, ASSOCIATED_BUSIF ch1s:ch2s" *)
    input a2_clk, // double a_clk used for BRAM (125MHz)
    
    // BRAM PORT A
    (* X_INTERFACE_PARAMETER = "FREQ_HZ 125000000, ASSOCIATED_CLKEN BRAM_PORTA_clka, ASSOCIATED_BUSIF BRAM_PORTA" *)
    output wire                        BRAM_PORTA_clka,
    output wire [BRAM_ADDR_WIDTH-1:0]  BRAM_PORTA_addra,
    output wire [BRAM_DATA_WIDTH-1:0]  BRAM_PORTA_dina,
    // input  wire [BRAM_DATA_WIDTH-1:0]  BRAM_PORTA_rddata,
    // output wire       BRAM_PORTA_rst,
    output wire       BRAM_PORTA_ena,
    output wire [0:0] BRAM_PORTA_wea,

    output wire [16-1:0]  last_write_addr,
    output wire ready
    );
    
    reg [2:0] bramwr_sms=3'd0;
    //reg [2:0] bramwr_sms_next=3'd0;
    reg [2:0] bramwr_sms_start=1'b0;
    reg bram_wren = 1'b0;
    reg bram_wren_next = 1'b0;
    reg [BRAM_ADDR_WIDTH-1:0] bram_addr=0, bram_addr_next=0;
    reg [BRAM_DATA_WIDTH-1:0] bram_data, bram_data_next;

    //reg [64-1:0] ch1_buffer;
    reg [32-1:0] ch2_buffer;


    reg status_ready=1;

    assign BRAM_PORTA_clka = a2_clk;
    // assign BRAM_PORTA_rst = ~a_resetn;
    assign BRAM_PORTA_wea = bram_wren;
    assign BRAM_PORTA_ena = bram_wren;
    assign BRAM_PORTA_addra = bram_addr;
    assign BRAM_PORTA_dina = bram_data;

    assign last_write_addr = {{(16-BRAM_ADDR_WIDTH){1'b0}}, bram_addr[BRAM_ADDR_WIDTH-1:0]};
        
    assign ready = status_ready;

    
    // BRAM writer at own a2_clk
    always @(posedge a2_clk)
    begin
        bram_addr <= bram_addr_next;
        bram_data <= bram_data_next;
        bram_wren <= bram_wren_next;
        // bramwr_sms <= (push_next && bramwr_sms == 0)  ? 3'd1 : reset ? 3'd0 : bramwr_sms_next; // SMS Control: start, reset, next
      
        // BRAM STORE MACHINE
        case(BR_wpos_reset ? 0 : bramwr_sms)
            0:    // Begin/reset/wait state
            begin
                if (BR_wpos_reset)
                begin
                    bram_wren_next  <= 1'b0;
                    bram_addr_next  <= {(BRAM_ADDR_WIDTH){1'b0}}; // idle, reset addr pointer
                end
                if (push_next) // buffer data and start write cycle
                begin
                    //ch1_buffer <= ch1s;
                    ch2_buffer <= ch2s;
                    status_ready <= 0;
                    //bramwr_sms <= 3'd1; // start write cycle
                    bram_data_next  <= ch1s; // Store CH1
                    bramwr_sms <= 3'd2;
                end
                else
                begin
                    status_ready <= 1;
                    bramwr_sms <= 3'd0; // wait for next data
                end
            end
            //1:    // Store CH1 -- included in setup step above
            //begin
            //    status_ready    <= 0;
            //    bram_data_next  <= ch1_buffer;
            //    bramwr_sms <= 3'd2;
            //end
            2:    // Write
            begin
                bram_wren_next  <= 1'b1;
                bramwr_sms <= 3'd3;
            end
            3:    // Store CH2
            begin
                bram_wren_next  <= 1'b0;
                bram_addr_next  <= bram_addr + 1;
                bram_data_next  <= ch2_buffer;
                bramwr_sms <= 3'd4;
            end
            4:    // Write
            begin
                bram_wren_next  <= 1'b1;
                bramwr_sms <= 3'd5;
            end
            5:    // Store completed
            begin
                bram_addr_next <= bram_addr + 1;
                bram_wren_next  <= 2'd0;
                status_ready <= 1;
                bramwr_sms <= 3'd0; // idle next
            end
        endcase
    end
endmodule
