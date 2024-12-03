`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 11/26/2017 12:54:20 AM
// Design Name: 
// Module Name: QControl
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


module QControl #(
    parameter ADC_WIDTH        = 14,
    parameter SIGNAL_M_WIDTH   = 16,
    parameter AXIS_DATA_WIDTH  = 16,
    parameter AXIS_TDATA_WIDTH = 16,
    parameter VAXIS_DATA_WIDTH = 16,
    parameter VAXIS_DATA_Q     = 14,
    parameter QC_PHASE_LEN2    = 13
)
(
    //(* X_INTERFACE_PARAMETER = "FREQ_HZ 62500000" *)
    (* X_INTERFACE_PARAMETER = "ASSOCIATED_CLKEN a_clk" *)
    (* X_INTERFACE_PARAMETER = "ASSOCIATED_BUSIF S_AXIS_SIGNAL_M" *)
    input a_clk,
    input wire [SIGNAL_M_WIDTH-1:0]    S_AXIS_SIGNAL_M_tdata,
    input wire                         S_AXIS_SIGNAL_M_tvalid,

    input wire QC_enable,
    input wire [16-1:0]  QC_gain,
    input wire [16-1:0]  QC_delay,

    (* X_INTERFACE_PARAMETER = "ASSOCIATED_CLKEN adc_clk" *)
    (* X_INTERFACE_PARAMETER = "ASSOCIATED_BUSIF M_AXIS" *)
     input adc_clk,
    //(* X_INTERFACE_PARAMETER = "FREQ_HZ 125000000" *)
    output [AXIS_TDATA_WIDTH-1:0]      M_AXIS_tdata,
    output                             M_AXIS_tvalid
);

    reg reg_QC_enable;

    reg signed [ADC_WIDTH-1:0] x=0;
    reg signed [VAXIS_DATA_Q-1:0] v=0;
    reg signed [VAXIS_DATA_Q+ADC_WIDTH-1:0] regy_vx=0;
    reg signed [VAXIS_DATA_Q+ADC_WIDTH-1:0] regy_vx_QC=0;
    reg signed [VAXIS_DATA_Q+ADC_WIDTH-1:0] reg_QC_signal=0;
    reg signed [16-1:0] reg_QC_gain=0;
    reg signed [SIGNAL_M_WIDTH-1:0] signal=0;
    reg [QC_PHASE_LEN2-1:0] reg_QC_delay=0;
    reg signed [SIGNAL_M_WIDTH-1:0] delayline [(1<<QC_PHASE_LEN2)-1 : 0];
    reg [QC_PHASE_LEN2-1:0] i=0;
    reg [QC_PHASE_LEN2-1:0] id=0;
    
    reg [1:0] rdecii = 0;

    always @ (posedge a_clk)
    begin
        rdecii <= rdecii+1;
        if (rdecii[1])
        begin
            reg_QC_enable <= QC_enable;
            reg_QC_gain   <= QC_gain;
            reg_QC_delay  <= QC_delay[QC_PHASE_LEN2-1:0];
            // Q-Control Mixer
            
            signal   <= {S_AXIS_SIGNAL_M_tdata[SIGNAL_M_WIDTH-1 : 0]};
            delayline[i] <= signal;
            
            // qc_delay = 4096 - #delaysampels
            // Q-Control + PAC-PLL Volume Control Mixer
            reg_QC_signal <= reg_QC_enable ? reg_QC_gain * $signed(delayline[id]) : 0;
            // regy_vx_QC    <= regy_vx + reg_QC_signal; // Volume Q15 or Q(VAXIS_DATA_Q-1)
            id <= i + reg_QC_delay;
            i  <= i+1;
        end
    end


    assign M_AXIS_tdata  = {{(AXIS_DATA_WIDTH-ADC_WIDTH){reg_QC_signal[VAXIS_DATA_Q+ADC_WIDTH-1]}}, {reg_QC_signal[VAXIS_DATA_Q+ADC_WIDTH-1:VAXIS_DATA_Q-1]}};
    assign M_AXIS_tvalid = 1;
   
endmodule
