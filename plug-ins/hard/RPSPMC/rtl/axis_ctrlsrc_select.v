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
// Description: 
// 
// Dependencies: 
// 
// Revision:
// Revision 0.01 - File Created
// Additional Comments:
// 
//////////////////////////////////////////////////////////////////////////////////


module axis_ctrlsrc_select #(
    parameter SAXIS_DATA_WIDTH = 32,
    parameter MAXIS_DATA_WIDTH = 32,
    parameter ADD_OFFSET = 1
)
(
    (* X_INTERFACE_PARAMETER = "ASSOCIATED_CLKEN a_clk" *)
    (* X_INTERFACE_PARAMETER = "ASSOCIATED_BUSIF S_AXIS:S_AXIS_LN:M_AXIS_ABS:M_AXIS" *)
    input a_clk,
    input wire [SAXIS_DATA_WIDTH-1:0]  S_AXIS_tdata,
    input wire                         S_AXIS_tvalid,
    input wire [SAXIS_DATA_WIDTH-1:0]  signal_offset,

    input wire [32-1:0]  S_AXIS_LN_tdata,
    input wire           S_AXIS_LN_tvalid,

    input wire [1:0]  selection_ln,

    output [32-1:0]      M_AXIS_ABS_tdata,
    output               M_AXIS_ABS_tvalid,

    output [MAXIS_DATA_WIDTH-1:0]      M_AXIS_tdata,
    output                             M_AXIS_tvalid
);

    reg signed [SAXIS_DATA_WIDTH-1:0] x;
    reg signed [SAXIS_DATA_WIDTH-1:0] y;

    always @ (posedge a_clk)
    begin
        if (ADD_OFFSET)
            x <= ($signed(S_AXIS_tdata) >>> 8 ) + ($signed(signal_offset) >>> 8); // REMOVE SIGNAL OFFSET
        else     
            x <= $signed(S_AXIS_tdata) >>> 8;
        y <= x[SAXIS_DATA_WIDTH-1] ? -x : x; // ABS
    end


    // Consider, if you have a data values i_data coming in with IWID (input width) bits, and you want to create a data value with OWID bits, why not just grab the top OWID bits?
    // assign	w_tozero = i_data[(IWID-1):0] + { {(OWID){1'b0}}, i_data[(IWID-1)], {(IWID-OWID-1){!i_data[(IWID-1)]}}};

    assign M_AXIS_tdata = selection_ln ? S_AXIS_LN_tdata : x;
    assign M_AXIS_tvalid = S_AXIS_tvalid;
    assign M_AXIS_ABS_tdata = y + 1; //ln_offset; // fixed extra offset from zero
    assign M_AXIS_ABS_tvalid = S_AXIS_tvalid;

endmodule
