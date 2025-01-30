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
// Module Name:    AD463x io connect to RP modules
// Project Name:   RPSPMC 4 GXSM
// Target Devices: Zynq z7020
// Tool Versions:  Vivado 2023.1
// Description:    IO connection assignments
// Company: 
// Engineer: 
// 
// Dependencies: 
// 
// Revision:
// Revision 0.01 - File Created
// Additional Comments:
// 
//////////////////////////////////////////////////////////////////////////////////


module AD463x_io_connect #(
    parameter NUM_DACS  = 2,
    parameter NUM_LANES = 1,
    parameter USE_RP_DIGITAL_IO = 1
)
(
    inout [8-1:0]  exp_p_io,
    (* X_INTERFACE_PARAMETER = "FREQ_HZ 30000000, ASSOCIATED_CLKEN SPI_sck, ASSOCIATED_BUSIF SPI" *)
    input SPI_sck,
    input SPI_cs,
    output  SPI_busy,
    input SPI_cnv,
    input SPI_reset,
    input  SPI_sdi,
    output [NUM_LANES*NUM_DACS-1:0] SPI_sdn
);

   wire [8-1:0] RP_exp_out;
   

// IOBUF macro, .T(0) : Output direction. IO = I, O = I (passed)
// IOBUF macro, .T(1) : Input direction.  IO = Z (high imp), O = IO (passed), I=X

// AD463x pin assignments to RP DnP IO
// ===================================================================================
// SD0 .. SD7:   
   // ADC1 (SD0, SD1*) => D0P, *D2P  * up top two lane when configuring adapter and hard wireing RESET and CS pins
   // ADC2 (SD4, SD5*) => D1P, *D3P
// CS    <= D3P  (*)
// BUSY  => D4P 
// CNV   <= D5P
// SDI   <= D6P
// SCK   <= D7P
// RESET <= D2P  (*)

// V1.0 interface AD5791 x 4
// ===========================================================================

// FIXED ASSIGNMNETS for one data lane each on SD0, SD4 => D0P, D1P
IOBUF dac_read_iobuf_AD_CH0  (.O(SPI_sdn[0]),  .IO(exp_p_io[0]), .I(0), .T(1) );
IOBUF dac_read_iobuf_AD_CH1  (.O(SPI_sdn[1]),  .IO(exp_p_io[1]), .I(0), .T(1) );
IOBUF dac_read_iobuf_AD_Busy (.O(SPI_busy),    .IO(exp_p_io[4]), .I(0), .T(1) );
   
IOBUF rst_iobuf  (.O(RP_exp_out[2]),   .IO(exp_p_io[2]), .I(SPI_reset),    .T(0) );
IOBUF cs_iobuf   (.O(RP_exp_out[3]),   .IO(exp_p_io[3]), .I(SPI_cs),       .T(0) );
IOBUF cnv_iobuf  (.O(RP_exp_out[5]),   .IO(exp_p_io[5]), .I(SPI_cnv),      .T(0) );
IOBUF sdi_iobuf  (.O(RP_exp_out[6]),   .IO(exp_p_io[6]), .I(SPI_sdi),      .T(0) );
IOBUF sck_iobuf  (.O(RP_exp_out[7]),   .IO(exp_p_io[7]), .I(SPI_sck),      .T(0) );

//IOBUF dac_read_iobuf (.O(PMD_dac_data_read),   .IO(exp_p_io[7]), .I(0), .T(1) );

endmodule
