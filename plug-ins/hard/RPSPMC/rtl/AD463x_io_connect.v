`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 11/26/2017 08:20:47 PM
// Design Name: 
// Module Name: signal_combine
// Project Name: 
// Target Devices: 
// Tool Versions: 
// Description: get top most bits from config
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
    output SPI_sck,
    output SPI_cs,
    input  SPI_busy,
    output SPI_cnv,
    output SPI_reset,
    input  SPI_SDI_data,
    output [NUM_LANES*NUM_DACS-1:0] SPI_SDn_data
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
IOBUF dac_read_iobuf (.O(SPI_SDn_data[0]),   .IO(exp_p_io[0]), .I(0), .T(1) );
IOBUF dac_read_iobuf (.O(SPI_SDn_data[1]),   .IO(exp_p_io[1]), .I(0), .T(1) );
IOBUF dac_read_iobuf (.O(SPI_busy[4]),       .IO(exp_p_io[4]), .I(0), .T(1) );
   
IOBUF rst_iobuf  (.O(RP_exp_out[2]),   .IO(exp_p_io[2]), .I(SPI_reset),    .T(0) );
IOBUF cs_iobuf   (.O(RP_exp_out[3]),   .IO(exp_p_io[3]), .I(SPI_cs),       .T(0) );
IOBUF cnv_iobuf  (.O(RP_exp_out[5]),   .IO(exp_p_io[5]), .I(SPI_cnv),      .T(0) );
IOBUF sdi_iobuf  (.O(RP_exp_out[6]),   .IO(exp_p_io[6]), .I(SPI_SDI_data), .T(0) );
IOBUF sck_iobuf  (.O(RP_exp_out[7]),   .IO(exp_p_io[7]), .I(SPI_sck),      .T(0) );

//IOBUF dac_read_iobuf (.O(PMD_dac_data_read),   .IO(exp_p_io[7]), .I(0), .T(1) );

endmodule
