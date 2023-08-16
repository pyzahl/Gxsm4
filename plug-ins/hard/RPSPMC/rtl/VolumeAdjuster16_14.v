`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 11/26/2017 12:54:20 AM
// Design Name: 
// Module Name: VolumeAdjuster
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


module VolumeAdjuster16_14 #(
    parameter ADC_WIDTH        = 14,
    parameter SIGNAL_M_WIDTH   = 16,
    parameter AXIS_DATA_WIDTH  = 16,
    parameter AXIS_TDATA_WIDTH = 32,
    parameter VAXIS_DATA_WIDTH = 16,
    parameter VAXIS_DATA_Q     = 14
)
(
    //(* X_INTERFACE_PARAMETER = "FREQ_HZ 62500000" *)
    (* X_INTERFACE_PARAMETER = "ASSOCIATED_CLKEN a_clk" *)
    (* X_INTERFACE_PARAMETER = "ASSOCIATED_BUSIF S_AXIS:SV_AXIS:S_AXIS_SIGNAL_M" *)
    input a_clk,
    input wire [AXIS_TDATA_WIDTH-1:0]  S_AXIS_tdata,
    input wire                         S_AXIS_tvalid,
    
    input wire [VAXIS_DATA_WIDTH-1:0]  SV_AXIS_tdata,
    input wire                         SV_AXIS_tvalid,

    (* X_INTERFACE_PARAMETER = "ASSOCIATED_CLKEN adc_clk" *)
    (* X_INTERFACE_PARAMETER = "ASSOCIATED_BUSIF M_AXIS" *)
     input adc_clk,
    //(* X_INTERFACE_PARAMETER = "FREQ_HZ 125000000" *)
    output [AXIS_TDATA_WIDTH-1:0]      M_AXIS_tdata,
    output                             M_AXIS_tvalid
);

    reg signed [ADC_WIDTH-1:0] x=0;
    reg signed [VAXIS_DATA_Q-1:0] v=0;
    reg signed [VAXIS_DATA_Q+ADC_WIDTH-1:0] regy_vx=0;
    
    //reg [1:0] rdecii = 0;

    always @ (posedge a_clk)
    begin
        //rdecii <= rdecii+1;
        x <= {S_AXIS_tdata[AXIS_TDATA_WIDTH-AXIS_DATA_WIDTH+ADC_WIDTH-1 : AXIS_TDATA_WIDTH-AXIS_DATA_WIDTH]};
        v <= {SV_AXIS_tdata[VAXIS_DATA_WIDTH-1 : VAXIS_DATA_WIDTH-VAXIS_DATA_Q]};
        regy_vx <= v*x;
    end
    
    //always @ (posedge rdecii[1])

    assign M_AXIS_tdata = {{(AXIS_DATA_WIDTH-ADC_WIDTH){regy_vx[VAXIS_DATA_Q+ADC_WIDTH-1]}}, {regy_vx[VAXIS_DATA_Q+ADC_WIDTH-1:VAXIS_DATA_Q-1]}, S_AXIS_tdata[AXIS_DATA_WIDTH-1 : 0]};
    assign M_AXIS_tvalid = S_AXIS_tvalid && SV_AXIS_tvalid;
   
endmodule
