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
    parameter M_AXIS_XY_TDATA_WIDTH = 32,  // SC 25Q24
    parameter S_AXIS_SC_TDATA_WIDTH = 64,
    parameter SC_DATA_WIDTH  = 25,  // SC 25Q24
    parameter SC_Q_WIDTH     = 24,  // SC 25Q24
    parameter LCK_Q_WIDTH    = 24,  // SC 25Q24
    parameter DPHASE_WIDTH    = 44, // 44 for delta phase width
    parameter LCK_BUFFER_LEN2 = 13, // NEED 13 
    parameter RDECI  = 5   // reduced rate decimation bits 1= 1/2 ...
)
(
    (* X_INTERFACE_PARAMETER = "ASSOCIATED_CLKEN a_clk, ASSOCIATED_BUSIF S_AXIS_SC:S_AXIS_SIGNAL:M_AXIS_X:M_AXIS_Y" *)
    input a_clk,
    
    // SIGNAL
    input wire [S_AXIS_SIGNAL_TDATA_WIDTH-1:0]  S_AXIS_SIGNAL_tdata,
    input wire                                  S_AXIS_SIGNAL_tvalid,
    
    // SC Lock-In Reference and controls
    input wire [S_AXIS_SC_TDATA_WIDTH-1:0]  S_AXIS_SC_tdata,
    input wire                              S_AXIS_SC_tvalid,
    input wire [48-1:0]  S_AXIS_DDS_dphi_tdata,
    input wire           S_AXIS_DDS_dphi_tvalid,
    
    // XY Output
    output wire [M_AXIS_XY_TDATA_WIDTH-1:0]  M_AXIS_X_tdata,
    output wire                              M_AXIS_X_tvalid,
    output wire [M_AXIS_XY_TDATA_WIDTH-1:0]  M_AXIS_Y_tdata,
    output wire                              M_AXIS_Y_tvalid
    );

    // Lock-In
    // ========================================================
    // Calculated and fixed Parameters 
    localparam integer LCK_PH_SHIFT     = 22; // phase reduction
    localparam integer LCK_INT_PH_WIDTH = DPHASE_WIDTH - LCK_PH_SHIFT; // +1  = 22
    localparam integer LCK_INT_WIDTH    = M_AXIS_XY_TDATA_WIDTH + LCK_INT_PH_WIDTH + 10; // =  26 22 10 = 58
    //localparam integer LCK_NORM           = DPHASE_WIDTH-PH_SHIFT-PH_WIDTH_CUT; // "2pi" is the norm, splitup  

       
    reg signed [SC_DATA_WIDTH-1:0] s=0; // Q SC (25Q24)
    reg signed [SC_DATA_WIDTH-1:0] c=0; // Q SC (25Q24)

    reg signed [LCK_Q_WIDTH-1:0] signal = 0;

    reg signed [LCK_Q_WIDTH+SC_DATA_WIDTH-1:0] LckXcorrp1=0;
    reg signed [LCK_Q_WIDTH+SC_DATA_WIDTH-1:0] LckYcorrp1=0; // Q SC (25Q24)

    reg [LCK_INT_PH_WIDTH+LCK_INT_WIDTH+LCK_INT_WIDTH-1:0]  LckDXYdphi_mem [(1<<LCK_BUFFER_LEN2)-1:0]; // LckDXYdphi_mem

    reg [1:0] rdecii = 0;

    
    always @ (posedge a_clk)
    begin
        rdecii <= rdecii+1;
        // rdecii 00 01 *10 11 00 ...
        if (rdecii == 3)
        begin
            // LockIn 
            // Sin, Cos
            c <= S_AXIS_SC_tdata[                        SC_DATA_WIDTH-1 :                       0];  // 25Q24 full dynamic range, proper rounding   24: 0
            s <= S_AXIS_SC_tdata[S_AXIS_SC_TDATA_WIDTH/2+SC_DATA_WIDTH-1 : S_AXIS_SC_TDATA_WIDTH/2];  // 25Q24 full dynamic range, proper rounding   56:32
            
            signal   <= S_AXIS_SIGNAL_tdata[S_AXIS_SIGNAL_TDATA_WIDTH-1:S_AXIS_SIGNAL_TDATA_WIDTH-LCK_Q_WIDTH];

            LckXcorrp1 <= s * signal; // SC_Q_WIDTH x LMS_DATA_WIDTH , LMS_Q_WIDTH
            LckYcorrp1 <= c * signal; //

            // Classic LockIn and Correlation moving Integral over one period
/*    
            // Store in ring buffer
            // LckDdphi [Lck_i] <= LckDdphi4; // LCK_INT_PH_WIDTH 
            // LckXdphi [Lck_i] <= LckXdphi4; // LCK_INT_WIDTH
            // LckYdphi [Lck_i] <= LckYdphi4;
            LckDXYdphi_mem [Lck_i] <= { LckDdphi4, LckXdphi4, LckYdphi4 }; // LCK_INT_PH_WIDTH 

            // STEP 1: Correlelation Product
            LckDdphi1  <= S_AXIS_DDS_dphi_tdata[DPHASE_WIDTH-1:0] >>> 2; // convert to LCK_INT_PH_WIDTH  -- account for 4x decimation 

            // STEP 2 
            LckX2 <= (LckXcorrp1 + SCQHALF) >>> SC_Q_WIDTH; // Q22  LMS_DATA_WIDTH , LMS_Q_WIDTH
            LckY2 <= (LckYcorrp1 + SCQHALF) >>> SC_Q_WIDTH; // Q22
            LckDdphi2 <= LckDdphi1 >>> LCK_PH_SHIFT; 

            // STEP 3: Scale to Phase-Signal Volume // **** DDS_dphi limited from principal 48bit to LCK_DPH_WIDTH *****
            LckXdphi4 <= LckX2 * LckDdphi2; // S_AXIS_DDS_dphi_tdata[LCK_DPH_WIDTH-1:0]; // LMS_DATA_WIDTH + LCK_DPH_WIDTH - >>>PH_SHIFT  == LCK_INT_WIDTH   [26]Q22 + Q44 [assume 30kHz range renorm by 12 now (4096) --  4166 is 30kHz spp] 22+44-12=54
            LckYdphi4 <= LckY2 * LckDdphi2; // S_AXIS_DDS_dphi_tdata[LCK_DPH_WIDTH-1:0]; // Q22 [16 sig -> 6 spare] + Q44 [12 spare @ 30kHz] 44+22-6-12
            LckDdphi4 <= LckDdphi2[LCK_INT_PH_WIDTH-1:0];
*/
            
        end     
    end
    
    assign M_AXIS_X_tdata  = LckXcorrp1 [LCK_Q_WIDTH+SC_DATA_WIDTH-1 : (LCK_Q_WIDTH+SC_DATA_WIDTH)-M_AXIS_XY_TDATA_WIDTH-1];
    assign M_AXIS_X_tvalid = 1;
    assign M_AXIS_Y_tdata  = LckYcorrp1 [LCK_Q_WIDTH+SC_DATA_WIDTH-1 : (LCK_Q_WIDTH+SC_DATA_WIDTH)-M_AXIS_XY_TDATA_WIDTH-1];
    assign M_AXIS_Y_tvalid = 1;

endmodule
