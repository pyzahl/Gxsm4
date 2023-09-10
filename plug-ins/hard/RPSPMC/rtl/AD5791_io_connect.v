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


module AD5791_io_connect #(
    parameter NUM_DAC = 4,
    parameter USE_RP_DIGITAL_IO = 1
)
(
    inout [8-1:0]  exp_n_io,
    (* X_INTERFACE_PARAMETER = "FREQ_HZ 30000000" *)
    (* X_INTERFACE_PARAMETER = "ASSOCIATED_CLKEN PMD_clk" *)
    (* X_INTERFACE_PARAMETER = "ASSOCIATED_BUSIF PMD_sync:PMD_dac" *)
    input PMD_clk,
    input PMD_sync,
    input [NUM_DAC-1:0] PMD_dac,
    output [8-1:0] RP_exp_out
);


// IOBUF macro, .T(0) : Output direction. IO = I, O = I (passed)
// IOBUF macro, .T(1) : Input direction.  IO = Z (high imp), O = IO (passed), I=X

// PMOD AD
// 7N CLK
// 6N SYNC
// 0..5N SDATA DAC1..6
// 7P SDATA OUT common

// V1.0 interface AD5791 x 4
// ===========================================================================

IOBUF dac0_iobuf (.O(RP_exp_out[0]),   .IO(exp_n_io[0]), .I(PMD_dac[0]), .T(0) );
IOBUF dac1_iobuf (.O(RP_exp_out[1]),   .IO(exp_n_io[1]), .I(PMD_dac[1]), .T(0) );
IOBUF dac2_iobuf (.O(RP_exp_out[2]),   .IO(exp_n_io[2]), .I(PMD_dac[2]), .T(0) );
IOBUF dac3_iobuf (.O(RP_exp_out[3]),   .IO(exp_n_io[3]), .I(PMD_dac[3]), .T(0) );
//IOBUF dac4_iobuf (.O(RP_exp_out[4]),   .IO(exp_n_io[4]), .I(PMD_dac[4]), .T(0) );
//IOBUF dac5_iobuf (.O(RP_exp_out[5]),   .IO(exp_n_io[5]), .I(PMD_dac[5]), .T(0) );
IOBUF sync_iobuf (.O(RP_exp_out[6]),   .IO(exp_n_io[6]), .I(PMD_sync),   .T(0) );
IOBUF clk_iobuf  (.O(RP_exp_out[7]),   .IO(exp_n_io[7]), .I(PMD_clk),    .T(0) );

//IOBUF dac_read_iobuf (.O(PMD_dac_data_read),   .IO(exp_p_io[7]), .I(0), .T(1) );

endmodule
