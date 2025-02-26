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
// AD5791_io_connect
    parameter NUM_DAC = 6,
// AD463x_io_connect
    parameter NUM_DACS  = 2,
    parameter NUM_LANES = 4,
    parameter USE_RP_DIGITAL_IO = 1
)
(
// EXPANSION CONNECTOR IO
    inout [10:0]  exp_p_io,
    inout [10:0]  exp_n_io,
    
// AD463x_io_connect
    (* X_INTERFACE_PARAMETER = "FREQ_HZ 60000000, ASSOCIATED_CLKEN SPI_sck, ASSOCIATED_BUSIF SPI" *)
    input SPI_sck,
    input SPI_cs,
    output  SPI_busy,
    input SPI_cnv,
    input SPI_reset,
    input  SPI_sdi,
    output [NUM_LANES*NUM_DACS-1:0] SPI_sdn,

// AD5791_io_connect
    (* X_INTERFACE_PARAMETER = "FREQ_HZ 30000000, ASSOCIATED_CLKEN PMD_clk, ASSOCIATED_BUSIF PMD" *)
    input PMD_clk,
    input PMD_sync,
    input [NUM_DAC-1:0] PMD_dac,

// dummy
   output [7:0] RP_exp_out,
   output [4:0] RP_exp_out_p

);

   //wire [7:0] RP_exp_out;
   //wire [4:0] RP_exp_out_p;


// PMODS: AD5791_io_connect
// ===========================================================================

// IOBUF macro, .T(0) : Output direction. IO = I, O = I (passed)
// IOBUF macro, .T(1) : Input direction.  IO = Z (high imp), O = IO (passed), I=X

// PMOD AD
// 7N CLK
// 6N SYNC
// 0..5N SDATA DAC1..6
// 7P SDATA OUT common

// V1.0 interface AD5791 x 4..6
// ===========================================================================

IOBUF dac0_iobuf (.O(RP_exp_out[0]),   .IO(exp_n_io[0]), .I(PMD_dac[0]), .T(0) );
IOBUF dac1_iobuf (.O(RP_exp_out[1]),   .IO(exp_n_io[1]), .I(PMD_dac[1]), .T(0) );
IOBUF dac2_iobuf (.O(RP_exp_out[2]),   .IO(exp_n_io[2]), .I(PMD_dac[2]), .T(0) );
IOBUF dac3_iobuf (.O(RP_exp_out[3]),   .IO(exp_n_io[3]), .I(PMD_dac[3]), .T(0) );
IOBUF dac4_iobuf (.O(RP_exp_out[4]),   .IO(exp_n_io[4]), .I(PMD_dac[4]), .T(0) );
IOBUF dac5_iobuf (.O(RP_exp_out[5]),   .IO(exp_n_io[5]), .I(PMD_dac[5]), .T(0) );
IOBUF sync_iobuf (.O(RP_exp_out[6]),   .IO(exp_n_io[6]), .I(PMD_sync),   .T(0) );
IOBUF clk_iobuf  (.O(RP_exp_out[7]),   .IO(exp_n_io[7]), .I(PMD_clk),    .T(0) );

//IOBUF dac_read_iobuf (.O(PMD_dac_data_read),   .IO(exp_p_io[7]), .I(0), .T(1) );


// ***** need to merge ******


// AD463x_io_connect
// ===========================================================================


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
IOBUF dac_read_iobuf_AD_CH0   (.O(SPI_sdn[0]),  .IO(exp_p_io[0]), .I(0), .T(1) );
IOBUF dac_read_iobuf_AD_CH01  (.O(SPI_sdn[2]),  .IO(exp_n_io[10]),.I(0), .T(1) );
IOBUF dac_read_iobuf_AD_CH02  (.O(SPI_sdn[4]),  .IO(exp_n_io[9]), .I(0), .T(1) );
IOBUF dac_read_iobuf_AD_CH03  (.O(SPI_sdn[6]),  .IO(exp_n_io[8]), .I(0), .T(1) );

IOBUF dac_read_iobuf_AD_CH1   (.O(SPI_sdn[1]),  .IO(exp_p_io[1]), .I(0), .T(1) );
IOBUF dac_read_iobuf_AD_CH11  (.O(SPI_sdn[3]),  .IO(exp_p_io[8]), .I(0), .T(1) );
IOBUF dac_read_iobuf_AD_CH12  (.O(SPI_sdn[5]),  .IO(exp_p_io[9]), .I(0), .T(1) );
IOBUF dac_read_iobuf_AD_CH13  (.O(SPI_sdn[7]),  .IO(exp_p_io[10]),.I(0), .T(1) );

IOBUF dac_read_iobuf_AD_Busy  (.O(SPI_busy),    .IO(exp_p_io[4]), .I(0), .T(1) );
   
IOBUF rst_iobuf  (.O(RP_exp_out_p[0]),   .IO(exp_p_io[2]), .I(SPI_reset),    .T(0) );
IOBUF cs_iobuf   (.O(RP_exp_out_p[1]),   .IO(exp_p_io[3]), .I(SPI_cs),       .T(0) );
IOBUF cnv_iobuf  (.O(RP_exp_out_p[2]),   .IO(exp_p_io[5]), .I(SPI_cnv),      .T(0) );
IOBUF sdi_iobuf  (.O(RP_exp_out_p[3]),   .IO(exp_p_io[6]), .I(SPI_sdi),      .T(0) );
IOBUF sck_iobuf  (.O(RP_exp_out_p[4]),   .IO(exp_p_io[7]), .I(SPI_sck),      .T(0) );

//IOBUF dac_read_iobuf (.O(PMD_dac_data_read),   .IO(exp_p_io[7]), .I(0), .T(1) );

endmodule
