`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 08/16/2020 12:14:13 PM
// Design Name: 
// Module Name: axis_FIR
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


module axis_FIR #(
    parameter SAXIS_TDATA_WIDTH = 32,
    parameter MAXIS_TDATA_WIDTH = 32,
    parameter FIR_DECI   = 64, // FIR LEN
    parameter FIR_DECI_L = 6 // extra bits to accomodate FIR_DECI number 
)
(
    // (* X_INTERFACE_PARAMETER = "FREQ_HZ 125000000" *)
    (* X_INTERFACE_PARAMETER = "ASSOCIATED_CLKEN a_clk" *)
    (* X_INTERFACE_PARAMETER = "ASSOCIATED_BUSIF S_AXIS:M_AXIS:M_AXIS2" *)
    input a_clk,
    input next_dv,
    
    input wire [SAXIS_TDATA_WIDTH-1:0]  S_AXIS_tdata,
    input wire                          S_AXIS_tvalid,
    output wire [MAXIS_TDATA_WIDTH-1:0] M_AXIS_tdata,
    output wire                         M_AXIS_tvalid,
    output wire [MAXIS_TDATA_WIDTH-1:0] M_AXIS2_tdata,
    output wire                         M_AXIS2_tvalid
    );
    
    reg [FIR_DECI_L-1:0] i = 0;
    reg signed [SAXIS_TDATA_WIDTH+FIR_DECI_L-1:0] sum = 0;
    reg signed [SAXIS_TDATA_WIDTH-1:0] fir_buffer[FIR_DECI-1:0];
    reg signed [SAXIS_TDATA_WIDTH-1:0] data_in;
    reg signed [MAXIS_TDATA_WIDTH-1:0] buffer; 
    
    integer i;
    initial begin
        for (i=0; i<FIR_DECI; i=i+1)
            fir_buffer[i] = 0;
    end
    
    always @ (posedge next_dv)
    begin
        if (S_AXIS_tvalid)
        begin
            data_in       <= $signed(S_AXIS_tdata);
            sum           <= sum - fir_buffer[i] + data_in;
            fir_buffer[i] <= data_in;
        end
        else
        begin
            fir_buffer[i] <= 0;
            sum           <= 0;
            data_in       <= 0;
        end
        i <= i+1;
        buffer <= sum[SAXIS_TDATA_WIDTH+FIR_DECI_L-1:FIR_DECI_L+SAXIS_TDATA_WIDTH-MAXIS_TDATA_WIDTH];
    end
    
    assign M_AXIS_tdata  = buffer;
    assign M_AXIS_tvalid = S_AXIS_tvalid;
    assign M_AXIS2_tdata  = buffer;
    assign M_AXIS2_tvalid = S_AXIS_tvalid;
    
endmodule
