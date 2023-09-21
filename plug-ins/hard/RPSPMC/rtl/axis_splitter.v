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


module axis_splitter #(
    parameter SAXIS_TDATA_WIDTH = 32,
    parameter MAXIS_TDATA_WIDTH = 32
)
(
    // (* X_INTERFACE_PARAMETER = "FREQ_HZ 125000000" *)
    (* X_INTERFACE_PARAMETER = "ASSOCIATED_CLKEN a_clk" *)
    (* X_INTERFACE_PARAMETER = "ASSOCIATED_BUSIF S_AXIS:M_AXIS:M_AXIS2" *)
    input a_clk,
    
    input wire [SAXIS_TDATA_WIDTH-1:0]  S_AXIS_tdata,
    input wire                          S_AXIS_tvalid,
    output wire [MAXIS_TDATA_WIDTH-1:0] M_AXIS_tdata,
    output wire                         M_AXIS_tvalid,
    output wire [MAXIS_TDATA_WIDTH-1:0] M_AXIS2_tdata,
    output wire                         M_AXIS2_tvalid,
    output wire [MAXIS_TDATA_WIDTH-1:0] monitor
    );
    
    reg signed [SAXIS_TDATA_WIDTH-1:0] data_in;
    reg signed [MAXIS_TDATA_WIDTH-1:0] buffer;
     
    always @ (posedge a_clk)
    begin
        if (S_AXIS_tvalid)
        begin
            data_in   <= $signed(S_AXIS_tdata);
            buffer    <= data_in;
        end
        else
        begin
            buffer  <= 0;
        end
    end
    
    assign monitor = buffer;
    assign M_AXIS_tdata  = buffer;
    assign M_AXIS_tvalid = S_AXIS_tvalid;
    assign M_AXIS2_tdata  = buffer;
    assign M_AXIS2_tvalid = S_AXIS_tvalid;
    
endmodule
