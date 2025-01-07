`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 11/26/2017 12:54:20 AM
// Design Name: 
// Module Name: Volume_QControl_Mixer
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
//////////////////////////////////////////////////////////////////////////////////

//SineOut=(int)(((long long)cr1 * (long long)volumeSine)>>22); // Volume Q22


module Volume_QControl_Mixer #(
    parameter ADC_WIDTH        = 14,
    parameter SIGNAL_QS_WIDTH  = 16,
    parameter AXIS_DATA_WIDTH  = 16,
    parameter AXIS_TDATA_WIDTH = 32,
    parameter VAXIS_DATA_WIDTH = 16,
    parameter VAXIS_DATA_Q     = 14
)
(
    //(* X_INTERFACE_PARAMETER = "FREQ_HZ 62500000" *)
    (* X_INTERFACE_PARAMETER = "ASSOCIATED_CLKEN a_clk, ASSOCIATED_BUSIF S_AXIS_VS:S_AXIS_QS:S_AXIS_SIGNAL_M" *)
    input a_clk,
    input wire [AXIS_TDATA_WIDTH-1:0]  S_AXIS_VS_tdata,
    input wire                         S_AXIS_VS_tvalid,
    
    input wire [SIGNAL_QS_WIDTH-1:0]   S_AXIS_QS_tdata,
    input wire                         S_AXIS_QS_tvalid,

    (* X_INTERFACE_PARAMETER = "ASSOCIATED_CLKEN adc_clk, ASSOCIATED_BUSIF M_AXIS" *)
     input adc_clk,
    //(* X_INTERFACE_PARAMETER = "FREQ_HZ 125000000" *)
    output [AXIS_TDATA_WIDTH-1:0]      M_AXIS_tdata,
    output                             M_AXIS_tvalid
);

    reg signed [VAXIS_DATA_WIDTH-1:0] reg_QC_signal=0;
    
    always @ (posedge a_clk)
    begin
        reg_QC_signal <= $signed(S_AXIS_QS_tdata[SIGNAL_QS_WIDTH-1:0]) + $signed({S_AXIS_VS_tdata[AXIS_TDATA_WIDTH-1 : AXIS_TDATA_WIDTH-AXIS_DATA_WIDTH]}); // add QControl signal and Exec Volume Signal
    end

    assign M_AXIS_tdata = {{(AXIS_DATA_WIDTH-ADC_WIDTH){reg_QC_signal[VAXIS_DATA_WIDTH-1]}}, {reg_QC_signal[VAXIS_DATA_Q-1:0]}, S_AXIS_VS_tdata[AXIS_DATA_WIDTH-1 : 0]};
    assign M_AXIS_tvalid = 1;
   
endmodule
