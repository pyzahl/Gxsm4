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


module axis_bram_stream_srcs #(
    parameter integer BRAM_DATA_WIDTH = 32,
    parameter integer BRAM_ADDR_WIDTH = 14
)
(
    (* X_INTERFACE_PARAMETER = "FREQ_HZ 125000000, ASSOCIATED_CLKEN a2_clk, ASSOCIATED_BUSIF BRAM_PORTA" *)
                             // CH      MASK
    input a2_clk, // double a_clk used for BRAM (125MHz)
    input wire [32-1:0] ch1s, // XS      0x0001  X in Scan coords
    input wire [32-1:0] ch2s, // YS      0x0002  Y in Scan coords
    input wire [32-1:0] ch3s, // ZS      0x0004  Z
    input wire [32-1:0] ch4s, // U       0x0008  Bias
    input wire [32-1:0] ch5s, // IN1     0x0010  IN1 RP (Signal)
    input wire [32-1:0] ch6s, // IN2     0x0020  IN2 RP (Current)
    input wire [32-1:0] ch7s, // IN3     0x0040  reserved, N/A at this time
    input wire [32-1:0] ch8s, // IN4     0x0080  reserved, N/A at this time
    input wire [32-1:0] ch9s, // DFREQ   0x0100  via PACPLL FIR
    input wire [32-1:0] chAs, // EXEC    0x0200  via PACPLL FIR
    input wire [32-1:0] chBs, // PHASE   0x0400  via PACPLL FIR
    input wire [32-1:0] chCs, // AMPL    0x0800  via PACPLL FIR
    input wire [32-1:0] chDs, // LockInA 0x1000  LockIn X (ToDo)
    input wire [32-1:0] chEs, // LockInB 0x2000  LocKin R (ToDo)
    // from below
    // gvp_time[32-1: 0]      // TIME  0x4000 // lower 32
    // gvp_time[48-1:32]      // TIME  0x8000 // upper 32 (16 lower only)
    input wire [48-1:0] gvp_time,  // time since GVP start in 1/125MHz units
    input wire [32-1:0] srcs,      // data selection mask and options
    input wire [32-1:0] index,     // index starting at N-1 down to 0
    input wire [2-1:0]  push_next, // frame header/data point trigger control
    input wire reset,
    
    // BRAM PORT A


    // [IP_Flow 19-4751] Bus Interface 'BRAM_PORTA_clk': FREQ_HZ bus parameter is missing for output clock interface.

    //(* X_INTERFACE_INFO = "BRAM_PORTA_clk CLK" *)
    //(* X_INTERFACE_PARAMETER = "XIL_INTERFACENAME BRAM_PORTA, ASSOCIATED_BUSIF BRAM_PORTA, ASSOCIATED_CLKEN BRAM_PORTA_clk, FREQ_HZ 125000000" *)

    
    (* X_INTERFACE_PARAMETER = "FREQ_HZ 125000000, ASSOCIATED_CLKEN BRAM_PORTA_clk" *)
    output wire                        BRAM_PORTA_clk,
    output wire [BRAM_ADDR_WIDTH-1:0]  BRAM_PORTA_addr,
    output wire [BRAM_DATA_WIDTH-1:0]  BRAM_PORTA_din,
    // input  wire [BRAM_DATA_WIDTH-1:0]  BRAM_PORTA_rddata,
    // output wire                        BRAM_PORTA_rst,
    // output wire                        BRAM_PORTA_en,
    output wire                        BRAM_PORTA_we,

    output wire [16-1:0]  last_write_addr,
    output wire ready
    );
    
    reg [2:0] bramwr_sms=3'd0;
    //reg [2:0] bramwr_sms_next=3'd0;
    reg [2:0] bramwr_sms_start=1'b0;
    reg bram_wr = 1'b0;
    reg bram_wr_next = 1'b0;
    reg [BRAM_ADDR_WIDTH-1:0] bram_addr=14'h200;
    reg [BRAM_ADDR_WIDTH-1:0] bram_addr_next=14'h200;
    reg [BRAM_ADDR_WIDTH-1:0] position=0;
    reg [BRAM_DATA_WIDTH-1:0] bram_data=0;
    reg [BRAM_DATA_WIDTH-1:0] bram_data_next=0;

    reg [24-1:0] srcs_mask=0;
    reg [4-1:0] channel=0;
    reg [32-1:0] stream_buffer [16-1:0];

    reg once=0;
    reg status_ready=1;
    reg [2-1:0] push_mode=0;
    reg r=0;
    reg [2-1:0] hdr_type=0;

    reg clk12=0;
    reg clk122=0;

    assign BRAM_PORTA_clk = ~a2_clk;
    // assign BRAM_PORTA_rst = ~a_resetn;
    assign BRAM_PORTA_we = bram_wr;
    assign BRAM_PORTA_addr = bram_addr;
    assign BRAM_PORTA_din = bram_data;

    assign last_write_addr = {{(16-BRAM_ADDR_WIDTH){0'b0}}, position}; 
        
    assign ready = status_ready;   
      
    always @(posedge a2_clk)
    begin
        clk12 <= ~clk12;
    end    
    
    always @(posedge clk12)
    begin
        clk122 <= ~clk122;
    end    
    
    // BRAM writer at a2_clk
    always @(posedge a2_clk)
    begin
        //bram_addr <= bram_addr_next; // delay one more!
        
        // silly patch of unresolved adressing issue 0-,4+
        if ((bram_addr_next & 14'h00f) == 4'hf)
            bram_addr <= bram_addr_next - 14'h040; // fix addresses @ 0xXXX0 : -0x40
        else 
        begin 
            if ((bram_addr_next & 14'h00f) == 1)
                bram_addr <= bram_addr_next + 14'h040; // fix addresses @ 0xXXX2 : +0x40
            else     
                bram_addr <= bram_addr_next; // all other addresses are OK
        end
        
        bram_data <= bram_data_next;
        bram_wr <= bram_wr_next;
        push_mode <= push_next;
        r <= reset;
        if (r)
        begin
            bram_wr_next  <= 1'b0;
            //bram_addr_next  <= -1; //{(BRAM_ADDR_WIDTH){-1'b1}}; // idle, reset addr pointer
            bram_addr_next  <= 14'h100-2;  // idle, reset addr pointer
            bramwr_sms <= 0;
            once <= 1;
        end
        else
        begin
            position <= bram_addr_next;
            // BRAM STORE MACHINE
            case(bramwr_sms)
                0:    // Begin/reset/wait state
                begin
                    if (push_mode && once) // buffer data and start write cycle
                    begin
                        hdr_type <= push_mode;
                        status_ready <= 0;
                        bram_wr_next  <= 1'b0;
                        channel <= 0;
                        once <= 0; // only one push!
                        // buffer all data
                        stream_buffer[0] <= ch1s;
                        stream_buffer[1] <= ch2s;
                        stream_buffer[2] <= ch3s;
                        stream_buffer[3] <= ch4s;
                        stream_buffer[4] <= ch5s;
                        stream_buffer[5] <= ch6s;
                        stream_buffer[6] <= ch7s;
                        stream_buffer[7] <= ch8s;
                        stream_buffer[8] <= ch9s;
                        stream_buffer[9] <= chAs;
                        stream_buffer[10] <= chBs;
                        stream_buffer[11] <= chCs;
                        stream_buffer[12] <= chDs;
                        stream_buffer[13] <= chEs;
                        stream_buffer[14] <= gvp_time[32-1:0];
                        stream_buffer[15] <= { 16'd0, gvp_time[48-1:32] };
                        bramwr_sms <= 3'd1; // write frame start info, then data
                    end
                    else
                    begin
                        status_ready <= 1;
                        bram_wr_next  <= 1'b0;
                        if (!push_next)
                            once <= 1; // push_next is clear now, wait for next push!
                        bramwr_sms <= 3'd0; // wait for next data
                    end
                end
                1:    // Data prepare cycle HEADER
                begin
                    bram_wr_next  <= 1'b1;
                    // prepare header -- full or normal point header
                    case (hdr_type)
                        1: 
                        begin // normal data set as of srcs
                            srcs_mask       <= srcs[32-1:8];
                            bram_data_next  <= { index[16-1:0], srcs[32-8-1:8] }; // frame info: mask and type (header or data)
                        end
                        2:
                        begin // full header info, all signals
                            srcs_mask       <= 24'h0ffff;// ALL 16
                            bram_data_next  <= { index[16-1:0], 16'hffff }; // frame info: mask and type (header or data)
                        end
                        3:
                        begin // full header info, all signals, + END MARKING
                            stream_buffer[12] <= 32'hffffeeee;
                            stream_buffer[13] <= 32'hfefecdcd;
                            srcs_mask       <= 24'hfffff;// ALL 16
                            bram_data_next  <= { 16'hfefe, 16'hfefe }; // frame info: END MARK, full vector position list follows
                        end
                    endcase
                    bram_addr_next <= bram_addr_next + 1;
                    bramwr_sms <= 3'd2; // start pushing selected channels
                end
                2:    // Data prepare cycle DATA
                begin
                    if (srcs_mask & (1<<channel))
                    begin
                        bram_wr_next  <= 1'b1; // 0
                        //bram_data_next <= channel; // DATA ALIGNMENT TEST
                        bram_data_next <= stream_buffer[channel];
                        bram_addr_next <= bram_addr_next + 1; // next adr
                        if (channel == 4'd15) // check if no more data to push or last 
                            bramwr_sms <= 3'd0; //4 3 write last and finish frame
                        else
                            bramwr_sms <= 3'd2; //3 2 write and then repeat here
                    end
                    else
                    begin
                        bram_wr_next  <= 1'b1; // 0
                        if ((srcs_mask >> channel) == 0 || channel == 15) // check if no more data to push or last 
                            bramwr_sms <= 3'd0; //6 0 finish frame
                        else
                            bramwr_sms <= 3'd2; //2 1 repeat here
                    end
                    channel <= channel + 1; // check next channel, no mode change or write
                end
            endcase
        end
   end 
endmodule
