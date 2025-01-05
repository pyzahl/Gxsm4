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
    parameter AM2_DATA_WIDTH  = 48, // 48 for amplitude squared
    parameter LCK_BUFFER_LEN2 = 10, // **** need to decimate 100Hz min: log2(1<<44 / (1<<24)) => 20 *** LCK DDS IDEAL Freq= 100 Hz [16777216, N2=24, M=1048576]  Actual f=119.209 Hz  Delta=19.2093 Hz  (using power of 2 phase_inc)
    parameter DECII2_MAX = 32,
    parameter configuration_address = 999
)
(
    (* X_INTERFACE_PARAMETER = "ASSOCIATED_CLKEN a_clk, ASSOCIATED_BUSIF S_AXIS_SC:S_AXIS_SIGNAL:S_AXIS_GAIN:S_AXIS_DDS_N2:M_AXIS_A2:M_AXIS_X:M_AXIS_Y:M_AXIS_Sref:M_AXIS_SDref:M_AXIS_SignalOut:M_AXIS_i" *)
    input a_clk,

    input [32-1:0]  config_addr,
    input [512-1:0] config_data,
    
    // SIGNAL
    input wire [S_AXIS_SIGNAL_TDATA_WIDTH-1:0]  S_AXIS_SIGNAL_tdata,
    input wire                                  S_AXIS_SIGNAL_tvalid,
    // digial gain
    input wire [S_AXIS_SIGNAL_TDATA_WIDTH-1:0]  S_AXIS_GAIN_tdata, // S31Q24
    input wire                                  S_AXIS_GAIN_tvalid,
    
    // SC Lock-In Reference and controls
    input wire [S_AXIS_SC_TDATA_WIDTH-1:0]  S_AXIS_SC_tdata,
    input wire                              S_AXIS_SC_tvalid,
    input wire [16-1:0]  S_AXIS_DDS_N2_tdata,
    input wire           S_AXIS_DDS_N2_tvalid,
    
    // XY Output
    output wire [AM2_DATA_WIDTH-1:0]  M_AXIS_A2_tdata,
    output wire                       M_AXIS_A2_tvalid,
    output wire [LCK_CORRSUM_WIDTH-1:0]  M_AXIS_X_tdata,
    output wire                          M_AXIS_X_tvalid,
    output wire [LCK_CORRSUM_WIDTH-1:0]  M_AXIS_Y_tdata,
    output wire                          M_AXIS_Y_tvalid,
    
    // S ref out
    output wire [32-1:0]  M_AXIS_Sref_tdata,
    output wire           M_AXIS_Sref_tvalid,
    // SDeci ref out
    output wire [32-1:0]  M_AXIS_SDref_tdata,
    output wire           M_AXIS_SDref_tvalid,
    // Signal out
    output wire [32-1:0]  M_AXIS_SignalOut_tdata,
    output wire           M_AXIS_SignalOut_tvalid,
    // i out
    output wire [32-1:0]  M_AXIS_i_tdata,
    output wire           M_AXIS_i_tvalid
    );

    // Lock-In
    // ========================================================
    // Calculated and internal width and sizes 
    localparam integer LCK_BUFFER_LEN   = (1<<LCK_BUFFER_LEN2);
    localparam integer LCK_SUM_WIDTH    = (LCK_CORRSUM_WIDTH+LCK_BUFFER_LEN2);
    localparam integer Q24 = ((1<<24)-1);
       
    reg signed [SC_DATA_WIDTH-1:0] s=0; // Q SC (25Q24)
    reg signed [SC_DATA_WIDTH-1:0] c=0; // Q SC (25Q24)
    reg signed [31:0] sd=0; // dbg only

    reg signed [31:0] gain = Q24;
    reg signed [SC_DATA_WIDTH-1:0] signal_in  = 0;        // input signal
    reg signed [SC_DATA_WIDTH+24-1:0] sig_in_x_gain = 0;  // x gain tmp
    reg signed [SC_DATA_WIDTH-1:0] sig_in     = 0;        // ready for pre-processing/decimation
    reg signed [SC_DATA_WIDTH+DECII2_MAX-1:0] signal_dec  = 0; // decimation tmp
    reg signed [SC_DATA_WIDTH+DECII2_MAX-1:0] signal_dec2 = 0; // decimation tmp
    reg signed [SC_DATA_WIDTH-1:0] signal     = 0;        // ready for correleation

    reg signed [2*SC_DATA_WIDTH-1:0] LckXcorrpW=0;
    reg signed [2*SC_DATA_WIDTH-1:0] LckYcorrpW=0; // Q SC^2
    reg signed [LCK_CORRSUM_WIDTH-1:0] LckXcorrp=0;
    reg signed [LCK_CORRSUM_WIDTH-1:0] LckYcorrp=0; // Q SC^2

    reg signed [LCK_SUM_WIDTH-1:0] LckX_sum = 0;
    reg signed [LCK_SUM_WIDTH-1:0] LckY_sum = 0;
    reg signed [LCK_CORRSUM_WIDTH-1:0] LckX_mem [LCK_BUFFER_LEN-1:0];
    reg signed [LCK_CORRSUM_WIDTH-1:0] LckY_mem [LCK_BUFFER_LEN-1:0];

    reg signed [LCK_CORRSUM_WIDTH-1:0] a=0;
    reg signed [LCK_CORRSUM_WIDTH-1:0] b=0;
    reg [AM2_DATA_WIDTH-1:0] ampl2=0; // Q LMS Squared 
    reg [2*(LCK_CORRSUM_WIDTH-1)+1-1:0] a2=0; 
    reg [2*(LCK_CORRSUM_WIDTH-1)+1-1:0] b2=0; 
    reg signed [LCK_CORRSUM_WIDTH+2-1:0] x=0; 
    reg signed [LCK_CORRSUM_WIDTH+2-1:0] y=0; 

    reg [16-1 : 0] dds_n2=0;
    reg [LCK_BUFFER_LEN2-1 : 0] ddsdec_nM=0;
    reg [LCK_BUFFER_LEN2-1 : 0] i=0;
    reg [6-1:0] ddsdec_n2 = 0;

    reg signed [8-1:0] decii2 = 0;
    reg signed [8-1:0] decii2_last = 0;
    reg [DECII2_MAX-1:0] decii  = 0;
    reg [DECII2_MAX-1:0] rdecii = 0;

    reg [4-1:0] finishthis=0;

    reg [31:0] lck_config=0;
    reg [31:0] lck_gain=Q24;

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
    
        // module configuration
        if (config_addr == configuration_address) // BQ configuration, and auto reset
        begin
            lck_config <= config_data[1*32-1 : 0*32]; // options: Bit0: use gain control, Bit1: use gain programmed
            lck_gain   <= config_data[1*32-2 : 1*32]; // programmed gain, Q24
        end
        
        if (lck_config[0])
            gain <= S_AXIS_GAIN_tdata;
        else if (lck_config[1])
            gain <= lck_gain;
        else      
            gain <= Q24;
            
        // signal
        signal_in <= $signed(S_AXIS_SIGNAL_tdata[S_AXIS_SIGNAL_TDATA_WIDTH-1:S_AXIS_SIGNAL_TDATA_WIDTH-SC_DATA_WIDTH]);   
        sig_in_x_gain <= gain * signal_in;
        sig_in <= sig_in_x_gain >>> 24;  
    end

    
    always @ (posedge a_clk)
    begin
        // Sin, Cos
        c <= S_AXIS_SC_tdata[                        SC_DATA_WIDTH-1 :                       0];  // 25Q24 full dynamic range, proper rounding   24: 0
        s <= S_AXIS_SC_tdata[S_AXIS_SC_TDATA_WIDTH/2+SC_DATA_WIDTH-1 : S_AXIS_SC_TDATA_WIDTH/2];  // 25Q24 full dynamic range, proper rounding   56:32
                
        // DDS N / cycle
        dds_n2 <= S_AXIS_DDS_N2_tdata;
        decii2 <= 44 - LCK_BUFFER_LEN2 - $signed(dds_n2);
        if (decii2_last != decii2 || finishthis > 0)
        begin
            decii2_last <= decii2;
            if (finishthis == 0)        
                finishthis <= 7;
            else        
                finishthis <= finishthis-1;
            if (decii2 > 0)
            begin
                decii  <= (1 <<< decii2) - 1; 
                rdecii <= 0;
                ddsdec_n2 <= LCK_BUFFER_LEN2;
                ddsdec_nM  <= (1 <<< LCK_BUFFER_LEN2) - 1;
            end else begin
                decii2 <= 0;
                decii  <= 0; 
                rdecii <= 0;
                ddsdec_n2 <= 44-dds_n2;
                ddsdec_nM  <= (1 <<< (44-dds_n2)) - 1;
            end
            signal_dec <= sig_in; // start new sum
        end 
        else
        begin if (rdecii == decii)
            begin
                rdecii <= 0; // reset
                // 1
                signal <= signal_dec >>> decii2;
                signal_dec <= sig_in;; // start new sum
            end
            else
            begin
                rdecii <= rdecii+1; // next
                signal_dec <= signal_dec + sig_in; // sum

                // now we have time steps to run the calculation pipeline... but only one shot
                // LockIn ====
                case (rdecii)
                0: begin
                    // Quad Correlation Products
                    sd <= signal; // dbg purpose only
                    LckXcorrpW <= s * signal; // SC_Q_WIDTH x LMS_DATA_WIDTH , LMS_Q_WIDTH
                    LckYcorrpW <= c * signal; //
                end
                1: begin               
                    // Normalize to LCK_CORRSUM_WIDTH
                    LckXcorrp <= LckXcorrpW >>> (2*SC_DATA_WIDTH-LCK_CORRSUM_WIDTH);
                    LckYcorrp <= LckYcorrpW >>> (2*SC_DATA_WIDTH-LCK_CORRSUM_WIDTH);
                end
                2: begin // calculate moving "integral" running over exactly the last period ++ add new point -- sub the memorized one dropping out
                    LckX_sum <= LckX_sum + LckXcorrp - LckX_mem [i]; // current [i] has oldest in ring buffer
                    LckY_sum <= LckY_sum + LckYcorrp - LckY_mem [i];
                    LckX_mem [i] <= LckXcorrp; // store value to current [i] slot (to be removed at end)
                    LckY_mem [i] <= LckYcorrp;
                    i <= (i+1) & ddsdec_nM;
                end
                3: begin             
                    a <= LckX_sum >>> ddsdec_n2;
                    b <= LckY_sum >>> ddsdec_n2;
                end
                4: begin             
                    a2 <= a*a;
                    b2 <= b*b;
                    y  <= a - b;
                    x  <= a + b;
                end
                5: begin                    
                    ampl2 <= (a2 + b2) >>> (2*(LCK_CORRSUM_WIDTH-1)+1 - AM2_DATA_WIDTH); // Q48 for SQRT
                end
                endcase
            end     
        end
    end
    assign M_AXIS_A2_tdata  = ampl2;
    assign M_AXIS_A2_tvalid = 1;
    assign M_AXIS_X_tdata  = x;
    assign M_AXIS_X_tvalid = 1;
    assign M_AXIS_Y_tdata  = y;
    assign M_AXIS_Y_tvalid = 1;

    assign M_AXIS_Sref_tdata = {{(32-SC_DATA_WIDTH){c[SC_DATA_WIDTH-1]}}, {c[SC_DATA_WIDTH-1:0]}};
    assign M_AXIS_Sref_tvalid = 1;
    // test, dec s
    assign M_AXIS_SDref_tdata = a;
    assign M_AXIS_SDref_valid = 1;
    // dec signal
    assign M_AXIS_SignalDec_tdata = {{(32-SC_DATA_WIDTH){signal[SC_DATA_WIDTH-1]}}, {signal[SC_DATA_WIDTH-1:0]}};
    assign M_AXIS_SignalDec_tvalid = 1;
    // i out
    assign M_AXIS_i_tdata  = i;
    assign M_AXIS_i_tvalid = 1;


endmodule
