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
// Module Name:    axis bram stream srcs
// Project Name:   RPSPMC 4 GXSM
// Target Devices: Zynq z7020
// Tool Versions:  Vivado 2023.1
// Description:    prepare GVP data stream, with header and marks
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
    parameter integer DMA_DATA_WIDTH = 32,
    parameter integer DMA_ADDR_WIDTH = 32
)
(
    // BRAM PORT A
    // [IP_Flow 19-4751] Bus Interface 'DMA_PORTA_clk': FREQ_HZ bus parameter is missing for output clock interface.
    //(* X_INTERFACE_INFO = "DMA_PORTA_clk CLK" *)
    //(* X_INTERFACE_PARAMETER = "XIL_INTERFACENAME DMA_PORTA, ASSOCIATED_BUSIF DMA_PORTA, ASSOCIATED_CLKEN DMA_PORTA_clk, FREQ_HZ 125000000" *)
    
    (* X_INTERFACE_PARAMETER = "FREQ_HZ 125000000, ASSOCIATED_CLKEN dma_data_clk, ASSOCIATED_BUSIF M_AXIS" *)
    output wire                        dma_data_clk,
    output wire                        [DMA_DATA_WIDTH-1:0] M_AXIS_tdata,
    output wire                        M_AXIS_tvalid,
    output wire                        M_AXIS_tlast,
    //output wire                        [3:0] M_AXIS_tkeep, // ???
    input wire                         M_AXIS_tready,

    output wire                        dma_fifo_resetn,
    
                             // CH      MASK
    (* X_INTERFACE_PARAMETER = "ASSOCIATED_CLKEN a_clk, ASSOCIATED_BUSIF S_AXIS_ch1s:S_AXIS_ch2s:S_AXIS_ch3s:S_AXIS_ch4s:S_AXIS_ch5s:S_AXIS_ch6s:S_AXIS_ch7s:S_AXIS_ch8s:S_AXIS_ch9s:S_AXIS_chAs:S_AXIS_chBs:S_AXIS_chCs:S_AXIS_chDs:S_AXIS_chEs:S_AXIS_gvp_time:S_AXIS_srcs:S_AXIS_index" *)

    input a2_clk, // double a_clk used for BRAM (125MHz)
    input wire [32-1:0] S_AXIS_ch1s_tdata, // XS      0x0001  X in Scan coords
    input wire          S_AXIS_ch1s_tvalid,
    input wire [32-1:0] S_AXIS_ch2s_tdata, // YS      0x0002  Y in Scan coords
    input wire          S_AXIS_ch2s_tvalid,
    input wire [32-1:0] S_AXIS_ch3s_tdata, // ZS      0x0004  Z
    input wire          S_AXIS_ch3s_tvalid,
    input wire [32-1:0] S_AXIS_ch4s_tdata, // U       0x0008  Bias
    input wire          S_AXIS_ch4s_tvalid,
    input wire [32-1:0] S_AXIS_ch5s_tdata, // IN1     0x0010  IN1 RP (Signal) -- PLL Signal
    input wire          S_AXIS_ch5s_tvalid,
    input wire [32-1:0] S_AXIS_ch6s_tdata, // IN2     0x0020  IN2 RP (Current -- after FIR, OFFSET COMP in SQ8.24)
    input wire          S_AXIS_ch6s_tvalid,
    input wire [32-1:0] S_AXIS_ch7s_tdata, // IN3     0x0040  reserved, N/A at this time [NOW: IN2-Z-SERVO-PASS]
    input wire          S_AXIS_ch7s_tvalid,
    input wire [32-1:0] S_AXIS_ch8s_tdata, // IN4     0x0080  reserved, N/A at this time [NOW: Z-SLOPE]
    input wire          S_AXIS_ch8s_tvalid,
    input wire [32-1:0] S_AXIS_ch9s_tdata, // DFREQ   0x0100  via PACPLL FIR1 ** via transport / decimation selector  | MUXABLE in Future
    input wire          S_AXIS_ch9s_tvalid,
    input wire [32-1:0] S_AXIS_chAs_tdata, // EXEC    0x0200  via PACPLL FIR2 ** via transport / decimation selector  | MUXABLE in Future
    input wire          S_AXIS_chAs_tvalid,
    input wire [32-1:0] S_AXIS_chBs_tdata, // PHASE   0x0400  via PACPLL FIR3 ** via transport / decimation selector  | MUXABLE in Future
    input wire          S_AXIS_chBs_tvalid,
    input wire [32-1:0] S_AXIS_chCs_tdata, // AMPL    0x0800  via PACPLL FIR4 ** via transport / decimation selector  | MUXABLE in Future
    input wire          S_AXIS_chCs_tvalid,
    input wire [32-1:0] S_AXIS_chDs_tdata, // LockInA 0x1000  LockIn X (ToDo) | MUXABLE in Future [NOW: IN2 NO FIR]
    input wire          S_AXIS_chDs_tvalid,
    input wire [32-1:0] S_AXIS_chEs_tdata, // LockInB 0x2000  LocKin R (ToDo) | MUXABLE in Future [NOW: dFreq CTRL]
    input wire          S_AXIS_chEs_tvalid,
    // from below
    // gvp_time[32-1: 0]      // TIME  0x4000 // lower 32
    // gvp_time[48-1:32]      // TIME  0x8000 // upper 32 (16 lower only)
    input wire [48-1:0] S_AXIS_gvp_time_tdata,  // time since GVP start in 1/125MHz units
    input wire          S_AXIS_gvp_time_tvalid,
    input wire [32-1:0] S_AXIS_srcs_tdata,      // data selection mask and options
    input wire          S_AXIS_srcs_tvalid,
    input wire [32-1:0] S_AXIS_index_tdata,     // index starting at N-1 down to 0
    input wire          S_AXIS_index_tvalid,
    input wire [2-1:0]  push_next, // frame header/data point trigger control
    input wire reset,
    

    output wire [32-1:0]  last_write_addr,
    output wire stall
    );
    
    reg [2:0] bramwr_sms=3'd0;
    //reg [2:0] bramwr_sms_next=3'd0;
    reg [2:0] bramwr_sms_start=1'b0;
    reg dma_wr = 1'b0;
    reg dma_wr_next = 1'b0;
    reg [DMA_DATA_WIDTH-1:0] dma_data=0;
    reg [DMA_DATA_WIDTH-1:0] dma_data_next=0;
    reg [DMA_ADDR_WIDTH-1:0] dma_addr=0;
    reg [DMA_ADDR_WIDTH-1:0] dma_addr_next=0;
    reg [DMA_ADDR_WIDTH-1:0] position=0;

    reg [24-1:0] srcs_mask=0;
    reg [4-1:0] channel=0;
    reg [32-1:0] stream_buffer [16-1:0];

    reg once=0;
    reg status_ready=1;
    reg [2-1:0] push_mode=0;
    reg r=0;
    reg [2-1:0] hdr_type=0;

    reg test_mode=0;
    reg [32-1:0] test_count=0;

    reg last_packet=0; // last element of data block
    reg last=0; // last of whole transmission

    reg fifo_ready=0;

    reg [15:0] padding=0;
    
    reg [2:0] loop_fix=0;

//    reg [1:0] rdecii = 0;
//    always @ (posedge a2_clk)
//    begin
//        rdecii <= rdecii+1;
//    end

    assign dma_data_clk = a2_clk;
//    assign dma_data_clk = rdecii[1];
    // assign DMA_PORTA_rst = ~a_resetn;
    assign M_AXIS_tdata  = dma_data;
    assign M_AXIS_tvalid = dma_wr;
    assign M_AXIS_tlast  = last; // *** packet mode -- not DMA, last indicates end of DMA

    assign dma_fifo_resetn = ~reset;

    assign last_write_addr = {{(32-DMA_ADDR_WIDTH){0'b0}}, position}; 
        
    assign stall = ~fifo_ready; // && status_ready;
      

    // BRAM writer at a2_clk
    //always @ (posedge rdecii[1])
    always @(posedge a2_clk)
    begin
        
        if ((dma_addr_next == 32'h40001 || dma_addr_next == 32'h80001 || dma_addr_next == 32'hc0001)  
            && loop_fix < 2) // looping, need to send one dummy data word (getting lost)
        begin
            case (loop_fix) // inject one dumm data value to get lost...
                0:
                begin
                    dma_data  <= 32'h12345678;
                    dma_addr  <= dma_addr_next;
                    dma_wr    <= 1'b1;
                    loop_fix <= 1;
                end
                1:
                begin
                    dma_wr <= 1'b0;
                    loop_fix <= 2;
                    if (dma_addr_next == 32'hc0001)
                        dma_addr_next <= 1;
                end
            endcase
        end
        else
        begin
            push_mode <= push_next;
            r <= reset;
            fifo_ready <= M_AXIS_tready;
    
            dma_wr    <= dma_wr_next;
            
            if (test_mode)
                dma_data  <= dma_addr_next; // send a simple count only
            else
                dma_data  <= dma_data_next;
                
            dma_addr  <= dma_addr_next;
    
            if (r)
            begin
                dma_wr_next    <= 1'b0;
                dma_data_next  <= 0;
                dma_addr_next  <= 0;
                bramwr_sms <= 0;
                last <= 0;
                once <= 1;
                position <= 0;
                test_count <= 0;
                loop_fix <= 0;
            end
            else
            begin
                if (fifo_ready)
                begin
                    position <= dma_addr;
                    // BRAM STORE MACHINE
                    case(bramwr_sms)
                        0:    // Begin/reset/wait state
                        begin
                            dma_wr_next <= 1'b0;
                            last_packet <= 0;                 
                            if (push_mode && once) // buffer data and start write cycle
                            begin
                                hdr_type <= push_mode;
                                status_ready <= 0;
                                channel <= 0;
                                once <= 0; // only one push!
                                // buffer all data in order [] of srcs bits in mask
                                stream_buffer[0]  <= S_AXIS_ch1s_tdata[32-1:0]; // X
                                stream_buffer[1]  <= S_AXIS_ch2s_tdata[32-1:0]; // Y
                                stream_buffer[2]  <= S_AXIS_ch3s_tdata[32-1:0]; // Z
                                stream_buffer[3]  <= S_AXIS_ch4s_tdata[32-1:0]; // U
                                stream_buffer[4]  <= S_AXIS_ch5s_tdata[32-1:0]; // IN1
                                stream_buffer[5]  <= S_AXIS_ch6s_tdata[32-1:0]; // IN2
                                stream_buffer[6]  <= S_AXIS_ch7s_tdata[32-1:0]; // IN3
                                stream_buffer[7]  <= S_AXIS_ch8s_tdata[32-1:0]; // IN4
                                stream_buffer[8]  <= S_AXIS_ch9s_tdata[32-1:0];  // PACPLL DFREQ | MUXABLE in Future
                                stream_buffer[9]  <= S_AXIS_chAs_tdata[32-1:0];  // PACPLL EXEC  | MUXABLE in Future
                                stream_buffer[10] <= S_AXIS_chBs_tdata[32-1:0]; // PACPLL PHASE | MUXABLE in Future
                                stream_buffer[11] <= S_AXIS_chCs_tdata[32-1:0]; // PACPLL AMPL  | MUXABLE in Future
                                stream_buffer[12] <= S_AXIS_chDs_tdata[32-1:0]; // LCK-X        | MUXABLE in Future
                                stream_buffer[13] <= S_AXIS_chEs_tdata[32-1:0]; // LCK-R        | MUXABLE in Future
                                stream_buffer[14] <= S_AXIS_gvp_time_tdata[32-1:0]; // TIME lower 32
                                stream_buffer[15] <= { 16'd0, S_AXIS_gvp_time_tdata[48-1:32] }; // TIME upper
                                test_mode <= S_AXIS_srcs_tdata[7];
                                test_count <= test_count+1;
                                bramwr_sms <= 3'd1; // write frame start info, then data
                            end
                            else
                            begin
                                status_ready <= 1;
                                if (!push_next)
                                    once <= 1; // push_next is clear now, wait for next push!
                                bramwr_sms <= 3'd0; // wait for next data
                            end
                        end
                        1:    // Data prepare cycle HEADER
                        begin
                            dma_wr_next <= 1'b1;
                            // prepare header -- full or normal point header
                            case (hdr_type)
                                1: 
                                begin // normal data set as of srcs
                                    srcs_mask <= S_AXIS_srcs_tdata[32-1:8]; // 24 SRCS bits, 8 Option bits
                                    dma_data_next <= { S_AXIS_index_tdata[16-1:0], S_AXIS_srcs_tdata[32-8-1:8] }; // frame info: mask and type (header or data)
                                end
                                2:
                                begin // full header info, all signals
                                    srcs_mask <= 24'h0ffff;// ALL 16
                                    dma_data_next <= { S_AXIS_index_tdata[16-1:0], 16'hffff }; // frame info: mask and type (header or data)
                                end
                                3:
                                begin // full header info, all signals, + END MARKING
                                    test_mode <= 0; // disable test mode
                                    srcs_mask <= 24'hfffff;// ALL 16
                                    dma_data_next <= { 16'hfefe, 16'hfefe }; // frame info: END MARK, full vector position list follows
                                end
                            endcase
                            dma_addr_next <= dma_addr_next + 1; //) & 16'h3fff;
                            loop_fix <= 0;
                            bramwr_sms <= 3'd2; // start pushing selected channels
                        end
                        2:    // Data prepare cycle DATA
                        begin
                            if (srcs_mask & (1<<channel))
                            begin
                                //dma_data_next <= channel; // DATA ALIGNMENT TEST
                                dma_data_next <= stream_buffer[channel];
                                dma_addr_next <= dma_addr_next + 1; //(..) & 16'h3fff; // next adr
                                loop_fix <= 0;
                                dma_wr_next <= 1'b1; // 1: selected data in this channel
                                if (channel == 4'd15) // check if no more data to push or last 
                                begin
                                    last_packet <= 1;
                                    if (hdr_type == 3) // END MARK HDR
                                    begin
                                        padding <= 32;
                                        bramwr_sms <= 3'd3; //6 0 add fifo padding to flush DMA buffers (32 long)
                                    end 
                                    else
                                    begin
                                        bramwr_sms <= 3'd0; //4 3 write last and finish frame
                                    end
                                end
                                else
                                begin
                                    bramwr_sms <= 3'd2; //3 2 write and then repeat here
                                end
                            end
                            else
                            begin
                                dma_wr_next <= 1'b0; // 0: no data in this channel
                                if ((srcs_mask >> channel) == 0 || channel == 15) // check if no more data to push or last 
                                begin
                                    last_packet <= 1;          
                                    bramwr_sms <= 3'd0; //6 0 finish frame
                                end                     
                                else
                                begin
                                    bramwr_sms <= 3'd2; //2 1 repeat here
                                end                         
                            end
                            channel <= channel + 1; // check next channel, no mode change or write
                        end
                        3:    // post data set stream padding
                        begin
                            if (padding > 0)
                            begin
                                if (padding > 16)                 
                                    dma_data_next <= 32'hffffeeee;
                                else
                                    dma_data_next <= padding;
                                dma_addr_next <= dma_addr_next + 1; // ***
                                loop_fix <= 0;
                                padding <= padding - 1;
                            end
                            else
                            begin
                                last <= 1; // set TLAST
                                dma_wr_next <= 1'b0; // 0: data to write done next this set
                                bramwr_sms <= 3'd0; //6 0 finish
                            end             
                        end
                    endcase
                end
           end
      end
    end   
endmodule
