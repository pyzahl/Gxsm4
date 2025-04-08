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
// Create Date: 11/26/2017 08:20:47 PM
// Design Name:    part of RPSPMC
// Module Name:    McBSP io connect
// Project Name:   RPSPMC 4 GXSM
// Target Devices: Zynq z7020
// Tool Versions:  Vivado 2023.1
// Description:    IO connect
//  -- obsoleted -- manage hi speed serial connection to MK3 DSP
// Dependencies: 
// 
// Revision:
// Revision 0.01 - File Created
// Additional Comments:
// 
//////////////////////////////////////////////////////////////////////////////////


module McBSP_io_connect #(
    parameter USE_RP_DIGITAL_IO_N = 0,
    parameter USE_RP_DIGITAL_IO_P = 0
)
(
    // inout logic [ 8-1:0] exp_p_io,
    inout  [8-1:0] exp_p_io,
    inout  [8-1:0] exp_n_io,
    input  McBSP_clkr, // CLKR: clock return
    input  McBSP_tx,   // TX: data transmit
    input  McBSP_fsx,  // optional, debug: data frame start FSX
    input  McBSP_frm,  // optional, debug: data frame
(* X_INTERFACE_PARAMETER = "FREQ_HZ 20000000" *)
    output McBSP_clk,  // McBSP clock
    output McBSP_fs,   // McBSP FS (frame start)
    output McBSP_rx,   // McBSP RX (data receive)
    output McBSP_nrx,  // optional: Nth/other RedPitaya slaves data TX forwarded on scheme TDB
    output [8-1:0] RP_exp_in,
    output [4-1:0] McBSP_pass
    // output [4-1:0] McBSP_dbg
);

//  [Place 30-574] Poor placement for routing between an IO pin and BUFG. If this sub optimal condition is acceptable for this design, you may use the CLOCK_DEDICATED_ROUTE constraint in the .xdc file to demote this message to a WARNING. However, the use of this override is highly discouraged. These examples can be used directly in the .xdc file to override this clock rule.
//  set_property CLOCK_DEDICATED_ROUTE FALSE [get_nets system_i/PS_data_transport/McBSP_io_connect_0/inst/clk_iobuf/O]
// => complains about using non clk dedicated IO pin for McBSP Clk.
// https://www.xilinx.com/support/documentation/sw_manuals/xilinx2012_2/ug953-vivado-7series-libraries.pdfhttps://www.xilinx.com/support/documentation/sw_manuals/xilinx2012_2/ug953-vivado-7series-libraries.pdf

//[Place 30-574] Poor placement for routing between an IO pin and BUFG. This is normally an ERROR but the CLOCK_DEDICATED_ROUTE constraint is set to FALSE allowing your design to continue. The use of this override is highly discouraged as it may lead to very poor timing results. It is recommended that this error condition be corrected in the design.
//
//	system_i/PS_data_transport/McBSP_io_connect_0/inst/clk_iobuf/IBUF (IBUF.O) is locked to IOB_X0Y68
//	 and system_i/PS_data_transport/McBSP_io_connect_0/inst/McBSP_clk_BUFG_inst (BUFG.I) is provisionally placed by clockplacer on BUFGCTRL_X0Y3


// Re: WARNING: [Constraints 18-550] Could not create 'IOSTANDARD' constraint because net 'McBSP_clk' is not directly connected to top level port. 'IOSTANDARD' is ignored by Vivado but preserved for

// IOBUF macro, .T(0) : Output direction. IO = I, O = I (passed)
// IOBUF macro, .T(1) : Input direction.  IO = Z (high imp), O = IO (passed), I=X

// V1.0 interface McBSP on exp_p_io[], IO-in on exp_n_io
// ==========================================================================
/*
IOBUF clk_iobuf (.O(McBSP_clk),      .IO(exp_p_io[0]), .I(0),         .T(1) );
IOBUF fs_iobuf  (.O(McBSP_fs),       .IO(exp_p_io[1]), .I(0),         .T(1) );
IOBUF rx_iobuf  (.O(McBSP_rx),       .IO(exp_p_io[2]), .I(0),         .T(1) );
IOBUF tx_iobuf  (.O(McBSP_pass[0]),  .IO(exp_p_io[3]), .I(McBSP_tx),  .T(0) );
IOBUF fsx_iobuf (.O(McBSP_pass[1]),  .IO(exp_p_io[4]), .I(McBSP_fsx), .T(0) );
IOBUF frm_iobuf (.O(McBSP_pass[2]),  .IO(exp_p_io[5]), .I(McBSP_frm), .T(0) );
IOBUF clkr_iobuf(.O(McBSP_pass[3]),  .IO(exp_p_io[6]), .I(McBSP_clkr),.T(0) );
IOBUF nrx_iobuf (.O(McBSP_nrx),      .IO(exp_p_io[7]), .I(0),         .T(1) );

if (USE_RP_DIGITAL_IO)
begin
    IOBUF exp_in_iobuf[8-1:0] (.O(RP_exp_in[8-1:0]), .IO(exp_n_io[8-1:0]), .I(8'b00000000),    .T(8'b11111111) );
end
// = V1 =====================================================================
*/
// ---------------------------------------- OR -------------------------------

// V2.0 interface McBSP on exp_n_io[], IO-in on exp_p_io (swapped pins)
// ===========================================================================
if (USE_RP_DIGITAL_IO_N)
begin
    IOBUF clk_iobuf (.O(McBSP_clk),      .IO(exp_n_io[0]), .I(0),         .T(1) );
    IOBUF fs_iobuf  (.O(McBSP_fs),       .IO(exp_n_io[1]), .I(0),         .T(1) );
    IOBUF rx_iobuf  (.O(McBSP_rx),       .IO(exp_n_io[2]), .I(0),         .T(1) );
    IOBUF tx_iobuf  (.O(McBSP_pass[0]),  .IO(exp_n_io[3]), .I(McBSP_tx),  .T(0) );
    IOBUF fsx_iobuf (.O(McBSP_pass[1]),  .IO(exp_n_io[4]), .I(McBSP_fsx), .T(0) );
    IOBUF frm_iobuf (.O(McBSP_pass[2]),  .IO(exp_n_io[5]), .I(McBSP_frm), .T(0) );
    IOBUF clkr_iobuf(.O(McBSP_pass[3]),  .IO(exp_n_io[6]), .I(McBSP_clkr),.T(0) );
    IOBUF nrx_iobuf (.O(McBSP_nrx),      .IO(exp_n_io[7]), .I(0),         .T(1) );
end
else
begin
    assign McBSP_clk = 0;
    assign McBSP_fs = 0;
    assign McBSP_rx = 0;
    assign McBSP_nrx = 0;
    assign McBSP_pass = 0;
end
    
if (USE_RP_DIGITAL_IO_P)
begin
    IOBUF exp_in_iobuf[8-1:0] (.O(RP_exp_in[8-1:0]), .IO(exp_p_io[8-1:0]), .I(8'b00000000),    .T(8'b11111111) );
end
else
begin
    assign RP_exp_in = 0;
end
// = V2 =====================================================================

// assign McBSP_dbg = { McBSP_clk, McBSP_fs, McBSP_rx, McBSP_nrx };


endmodule
