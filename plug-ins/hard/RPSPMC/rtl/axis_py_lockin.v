`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 12/28/2024 09:20:10 PM
// Design Name: 
// Module Name: axis_py_lockin
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


module axis_py_lockin#(
    parameter S_AXIS_SIGNAL_TDATA_WIDTH = 32,  // SC 25Q24
    parameter LCK_CORRSUM_WIDTH = 32,  // Final Precision
    parameter S_AXIS_SC_TDATA_WIDTH = 64,
    parameter SC_DATA_WIDTH  = 25,  // SC 25Q24
    parameter SC_Q_WIDTH     = 24,  // SC 25Q24
    parameter LCK_Q_WIDTH    = 24,  // SC 25Q24
    parameter DPHASE_WIDTH    = 44, // 44 for delta phase width
    parameter LCK_BUFFER_LEN2 = 10, // **** need to decimate 100Hz min: log2(1<<44 / (1<<24)) => 20 *** LCK DDS IDEAL Freq= 100 Hz [16777216, N2=24, M=1048576]  Actual f=119.209 Hz  Delta=19.2093 Hz  (using power of 2 phase_inc)
    parameter DECII2_MAX = 16
)
(
    (* X_INTERFACE_PARAMETER = "ASSOCIATED_CLKEN a_clk, ASSOCIATED_BUSIF S_AXIS_SC:S_AXIS_SIGNAL:S_AXIS_DDS_N2:M_AXIS_X:M_AXIS_Y:M_AXIS_Sref" *)
    input a_clk,
    
    // SIGNAL
    input wire [S_AXIS_SIGNAL_TDATA_WIDTH-1:0]  S_AXIS_SIGNAL_tdata,
    input wire                                  S_AXIS_SIGNAL_tvalid,
    
    // SC Lock-In Reference and controls
    input wire [S_AXIS_SC_TDATA_WIDTH-1:0]  S_AXIS_SC_tdata,
    input wire                              S_AXIS_SC_tvalid,
    input wire [16-1:0]  S_AXIS_DDS_N2_tdata,
    input wire           S_AXIS_DDS_N2_tvalid,
    
    // XY Output
    output wire [LCK_CORRSUM_WIDTH-1:0]  M_AXIS_X_tdata,
    output wire                          M_AXIS_X_tvalid,
    output wire [LCK_CORRSUM_WIDTH-1:0]  M_AXIS_Y_tdata,
    output wire                          M_AXIS_Y_tvalid,
    
    // S ref out
    output wire [32-1:0]  M_AXIS_Sref_tdata,
    output wire           M_AXIS_Sref_tvalid
    );

    // Lock-In
    // ========================================================
    // Calculated and internal width and sizes 
    localparam integer LCK_BUFFER_LEN   = (1<<LCK_BUFFER_LEN2);
    localparam integer LCK_SUM_WIDTH    = (LCK_CORRSUM_WIDTH+LCK_BUFFER_LEN);
       
    reg signed [SC_DATA_WIDTH-1:0] s=0; // Q SC (25Q24)
    reg signed [SC_DATA_WIDTH-1:0] c=0; // Q SC (25Q24)

    reg signed [LCK_Q_WIDTH-1:0] sig_in     = 0;
    reg signed [LCK_Q_WIDTH-1:0] signal     = 0;
    reg signed [LCK_Q_WIDTH+DECII2_MAX-1:0] signal_dec  = 0;
    reg signed [LCK_Q_WIDTH+DECII2_MAX-1:0] signal_dec2 = 0;

    reg signed [LCK_Q_WIDTH+SC_DATA_WIDTH-1:0] LckXcorrpW=0;
    reg signed [LCK_Q_WIDTH+SC_DATA_WIDTH-1:0] LckYcorrpW=0; // Q SC (25Q24)
    reg signed [LCK_CORRSUM_WIDTH-1:0] LckXcorrp=0;
    reg signed [LCK_CORRSUM_WIDTH-1:0] LckYcorrp=0; // Q SC (25Q24)

    reg signed [LCK_SUM_WIDTH-1:0] LckX_sum = 0;
    reg signed [LCK_SUM_WIDTH-1:0] LckY_sum = 0;
    reg signed [LCK_CORRSUM_WIDTH-1:0] LckX_mem [LCK_BUFFER_LEN-1:0];
    reg signed [LCK_CORRSUM_WIDTH-1:0] LckY_mem [LCK_BUFFER_LEN-1:0];

    reg [16-1 : 0] dds_n2=8;
    reg [LCK_BUFFER_LEN2-1 : 0] dds_n=16;
    reg [LCK_BUFFER_LEN2+1-1 : 0] i=0;

    reg [6-1:0] decii2 = 5;
    reg [DECII2_MAX-1:0] decii  = (1 << 5) - 1;
    reg [DECII2_MAX-1:0] rdecii = 0;

    
    always @ (posedge a_clk)
    begin
        sig_in <= S_AXIS_SIGNAL_tdata[S_AXIS_SIGNAL_TDATA_WIDTH-1:S_AXIS_SIGNAL_TDATA_WIDTH-LCK_Q_WIDTH];
        
        // DDS N / cycle
        dds_n2 <= S_AXIS_DDS_N2_tdata;

        if (dds_n2 > LCK_BUFFER_LEN2)
        begin
            decii2 <= dds_n2 - LCK_BUFFER_LEN2;
            decii  <= (1 << decii2) - 1; 
            dds_n  <= (1 << LCK_BUFFER_LEN2) - 1;
        end else begin
            decii2 <= 0;
            decii  <= 0; 
            dds_n  <= (1 << dds_n2) - 1;
        end
        
        if (rdecii == decii)
        begin
            rdecii <= 0;
            
            signal_dec <= sig_in; // start new sum
            signal <= signal_dec >>> decii2;
            
            // LockIn ====
            // Sin, Cos
            c <= S_AXIS_SC_tdata[                        SC_DATA_WIDTH-1 :                       0];  // 25Q24 full dynamic range, proper rounding   24: 0
            s <= S_AXIS_SC_tdata[S_AXIS_SC_TDATA_WIDTH/2+SC_DATA_WIDTH-1 : S_AXIS_SC_TDATA_WIDTH/2];  // 25Q24 full dynamic range, proper rounding   56:32
            
            signal   <= S_AXIS_SIGNAL_tdata[S_AXIS_SIGNAL_TDATA_WIDTH-1:S_AXIS_SIGNAL_TDATA_WIDTH-LCK_Q_WIDTH];

            // Quad Correlation Products
            LckXcorrpW <= s * signal; // SC_Q_WIDTH x LMS_DATA_WIDTH , LMS_Q_WIDTH
            LckYcorrpW <= c * signal; //

            // Normalize to LCK_CORRSUM_WIDTH
            LckXcorrp <= LckXcorrpW >>> (LCK_Q_WIDTH+SC_DATA_WIDTH-LCK_CORRSUM_WIDTH);
            LckYcorrp <= LckYcorrpW >>> (LCK_Q_WIDTH+SC_DATA_WIDTH-LCK_CORRSUM_WIDTH);

            // calcule special aligned averaging FIR (moving window over last period from "now")
            LckX_sum <= LckX_sum + LckXcorrp; // calculate moving "integral" running over exactly the last period ++ add new point
            LckY_sum <= LckY_sum + LckYcorrp;

            LckX_sum <= LckX_sum - LckX_mem [(i+dds_n) & dds_n]; // calculate moving "integral" running over exactly the last period -- remove dropping off points
            LckY_sum <= LckY_sum - LckY_mem [(i+dds_n) & dds_n];

            LckX_mem [i] <= LckXcorrp; // store value (to be removed at end)
            LckY_mem [i] <= LckYcorrp;

            i <= (i+1) & dds_n;
    
        end
        else
        begin
            rdecii <= rdecii+1;
            signal_dec <= signal_dec + sig_in; // sum
        end     
    end
    
    assign M_AXIS_X_tdata  = LckX_sum;
    assign M_AXIS_X_tvalid = 1;
    assign M_AXIS_Y_tdata  = LckY_sum;
    assign M_AXIS_Y_tvalid = 1;

    assign M_AXIS_Sref     = { {(32-SC_DATA_WIDTH){s[SC_DATA_WIDTH-1]}}, s[SC_DATA_WIDTH-1:0] };

endmodule
