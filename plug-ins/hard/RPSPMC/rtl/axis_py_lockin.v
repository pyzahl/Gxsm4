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
    parameter S_AXIS_SIGNAL_TDATA_WIDTH = 32, 
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
    (* X_INTERFACE_PARAMETER = "ASSOCIATED_CLKEN a_clk, ASSOCIATED_BUSIF S_AXIS_SC:S_AXIS_SIGNAL:S_AXIS_DDS_N2:M_AXIS_A2:M_AXIS_X:M_AXIS_Y:M_AXIS_Sref:M_AXIS_SDref" *)
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
    output wire [LCK_CORRSUM_WIDTH-1:0]  M_AXIS_A2_tdata,
    output wire                          M_AXIS_A2_tvalid,
    output wire [LCK_CORRSUM_WIDTH-1:0]  M_AXIS_X_tdata,
    output wire                          M_AXIS_X_tvalid,
    output wire [LCK_CORRSUM_WIDTH-1:0]  M_AXIS_Y_tdata,
    output wire                          M_AXIS_Y_tvalid,
    
    // S ref out
    output wire [32-1:0]  M_AXIS_Sref_tdata,
    output wire           M_AXIS_Sref_tvalid,
    // SDeci ref out
    output wire [32-1:0]  M_AXIS_SDref_tdata,
    output wire           M_AXIS_SDref_tvalid
    );

    // Lock-In
    // ========================================================
    // Calculated and internal width and sizes 
    localparam integer LCK_BUFFER_LEN   = (1<<LCK_BUFFER_LEN2);
    localparam integer LCK_SUM_WIDTH    = (LCK_CORRSUM_WIDTH+LCK_BUFFER_LEN2);
       
    reg signed [SC_DATA_WIDTH-1:0] s=0; // Q SC (25Q24)
    reg signed [SC_DATA_WIDTH-1:0] c=0; // Q SC (25Q24)
    reg signed [31:0] sd=0; // dbg only

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

    reg signed [LCK_CORRSUM_WIDTH-1:0] a=0;
    reg signed [LCK_CORRSUM_WIDTH-1:0] b=0;
    reg [2*(LCK_CORRSUM_WIDTH-1)+1-1:0] ampl2=0; // Q LMS Squared 
    reg [2*(LCK_CORRSUM_WIDTH-1)+1-1:0] a2=0; 
    reg [2*(LCK_CORRSUM_WIDTH-1)+1-1:0] b2=0; 
    reg signed [LCK_CORRSUM_WIDTH+2-1:0] x=0; 
    reg signed [LCK_CORRSUM_WIDTH+2-1:0] y=0; 

    reg [16-1 : 0] dds_n2=0;
    reg [16-1 : 0] dds_n2_last=0;
    reg [LCK_BUFFER_LEN2-1 : 0] dds_n=0;
    reg [LCK_BUFFER_LEN2+1-1 : 0] i=0;

    reg [6-1:0] decii2 = 5;
    reg [DECII2_MAX-1:0] decii  = 0;
    reg [DECII2_MAX-1:0] rdecii = 0;

    integer ii;
    initial begin
        for (ii=0; ii<LCK_BUFFER_LEN; ii=ii+1)
        begin
            LckX_mem [ii] = 0;
            LckY_mem [ii] = 0;
        end     
    end

    
    always @ (posedge a_clk)
    begin
        // Sin, Cos
        c <= S_AXIS_SC_tdata[                        SC_DATA_WIDTH-1 :                       0];  // 25Q24 full dynamic range, proper rounding   24: 0
        s <= S_AXIS_SC_tdata[S_AXIS_SC_TDATA_WIDTH/2+SC_DATA_WIDTH-1 : S_AXIS_SC_TDATA_WIDTH/2];  // 25Q24 full dynamic range, proper rounding   56:32
        // signal
        sig_in <= S_AXIS_SIGNAL_tdata[S_AXIS_SIGNAL_TDATA_WIDTH-1:S_AXIS_SIGNAL_TDATA_WIDTH-LCK_Q_WIDTH];   
        
        // DDS N / cycle
        dds_n2 <= S_AXIS_DDS_N2_tdata;
        if (dds_n2 != dds_n2_last)
        begin
            dds_n2_last <= dds_n2;
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
        end     
        if (rdecii == 0)
        begin
            rdecii <= decii; // reload
            signal_dec <= sig_in; // start new sum
            signal <= signal_dec >>> decii2;
            
            // LockIn ====
            
            if (i<16)
                sd <= 1<<26; // dbg purpose only
            else
                sd <= s; // dbg purpose only
            signal   <= S_AXIS_SIGNAL_tdata[S_AXIS_SIGNAL_TDATA_WIDTH-1:S_AXIS_SIGNAL_TDATA_WIDTH-LCK_Q_WIDTH]; // top bits as of LCK_Q_WIDTH

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
    
            // need to normalize:
            a <= LckX_sum >>> dds_n;
            b <= LckY_sum >>> dds_n;
            a2 <= a*a;
            b2 <= b*b;
            ampl2 <= a2 + b2; // 1Q44
            y     <= a - b;
            x     <= a + b;
   
        end
        else
        begin
            rdecii <= rdecii-1; // dec
            signal_dec <= signal_dec + sig_in; // sum
        end     
    end
    
    assign M_AXIS_A2_tdata  = ampl2;
    assign M_AXIS_A2_tvalid = 1;
    assign M_AXIS_X_tdata  = i; // x // testing
    assign M_AXIS_X_tvalid = 1;
    assign M_AXIS_Y_tdata  = y;
    assign M_AXIS_Y_tvalid = 1;

    assign M_AXIS_Sref_tdata = {{(32-SC_DATA_WIDTH){c[SC_DATA_WIDTH-1]}}, {c[SC_DATA_WIDTH-1:0]}};
    assign M_AXIS_Sref_tvalid = 1;
    assign M_AXIS_SDref_tdata = sd;
    assign M_AXIS_SDref_valid = 1;

endmodule
