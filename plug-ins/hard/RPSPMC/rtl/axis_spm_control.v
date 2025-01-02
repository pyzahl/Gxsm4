`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 08/14/2023 10:22:53 PM
// Design Name: 
// Module Name: axis_spm_control
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

module axis_spm_control#(
    parameter SAXIS_TDATA_WIDTH = 32,
    parameter QROTM = 28,
    parameter QSLOPE = 31,
    parameter S_AXIS_SC_TDATA_WIDTH = 64,
    parameter SC_DATA_WIDTH  = 25,  // SC 25Q24
    parameter SC_Q_WIDTH     = 24,  // SC 25Q24
    parameter RDECI  = 5   // reduced rate decimation bits 1= 1/2 ...
)
(
    (* X_INTERFACE_PARAMETER = "ASSOCIATED_CLKEN a_clk, ASSOCIATED_BUSIF S_AXIS_Xs:S_AXIS_Ys:S_AXIS_Zs:S_AXIS_U:S_AXIS_SC:S_AXIS_Z:M_AXIS1:M_AXIS2:M_AXIS3:M_AXIS4:M_AXIS_XSMON:M_AXIS_YSMON:M_AXIS_ZSMON:M_AXIS_X0MON:M_AXIS_Z_SLOPE:M_AXIS_Y0MON:M_AXIS_Z0MON:M_AXIS_UrefMON:M_AXIS_SC" *)
    input a_clk,
    // GVP/SCAN COMPONENTS, ROTATED RELATIVE COORDS TO SCAN CENTER
    input wire [SAXIS_TDATA_WIDTH-1:0]  S_AXIS_Xs_tdata,
    input wire                          S_AXIS_Xs_tvalid,
    input wire [SAXIS_TDATA_WIDTH-1:0]  S_AXIS_Ys_tdata,
    input wire                          S_AXIS_Ys_tvalid,
    input wire [SAXIS_TDATA_WIDTH-1:0]  S_AXIS_Zs_tdata,
    input wire                          S_AXIS_Zs_tvalid,
    // Z Feedback Servo
    input  wire [SAXIS_TDATA_WIDTH-1:0] S_AXIS_Z_tdata,
    input  wire                         S_AXIS_Z_tvalid,
    // Bias
    input wire [SAXIS_TDATA_WIDTH-1:0]  S_AXIS_U_tdata,
    input wire                          S_AXIS_U_tvalid,
    // two future control components using optional (DAC #5, #6) "Motor1, Motor2"
    //input wire [SAXIS_TDATA_WIDTH-1:0]  S_AXIS_M1_tdata,
    //input wire                          S_AXIS_M1_tvalid,
    //input wire [SAXIS_TDATA_WIDTH-1:0]  S_AXIS_M2_tdata,
    //input wire                          S_AXIS_M2_tvalid,
    
    // SC Lock-In Reference and controls
    input wire [S_AXIS_SC_TDATA_WIDTH-1:0]  S_AXIS_SC_tdata,
    input wire                              S_AXIS_SC_tvalid,
    input signed [32-1:0] modulation_volume, // volume for modulation Q31
    input [32-1:0] modulation_target, // target signal for mod (#XYZUAB)
    

    // scan rotation (yx=-xy, yy=xx)
    input signed [32-1:0] rotmxx, // =cos(alpha)
    input signed [32-1:0] rotmxy, // =sin(alpha)

    // slope -- always applied in global XY plane ???
    input signed [32-1:0] slope_x, // SQSLOPE (31)
    input signed [32-1:0] slope_y, // SQSLOPE (31)

    // SCAN OFFSET / POSITION COMPONENTS, ABSOLUTE COORDS
    input signed [32-1:0] x0, // vector components
    input signed [32-1:0] y0, // ..
    input signed [32-1:0] z0, // ..
    input signed [32-1:0] u0, // Bias Reference
    input signed [32-1:0] xy_offset_step, // @Q31 => Q31 / 120M => [18 sec full scale swin @ step 1 decii = 0]  x RDECI
    input signed [32-1:0] z_offset_step, // @Q31 => Q31 / 120M => [18 sec full scale swin @ step 1 decii = 0]  x RDECI

    output wire [SAXIS_TDATA_WIDTH-1:0]  M_AXIS1_tdata,
    output wire                          M_AXIS1_tvalid,
    output wire [SAXIS_TDATA_WIDTH-1:0]  M_AXIS2_tdata,
    output wire                          M_AXIS2_tvalid,
    output wire [SAXIS_TDATA_WIDTH-1:0]  M_AXIS3_tdata,
    output wire                          M_AXIS3_tvalid,
    output wire [SAXIS_TDATA_WIDTH-1:0]  M_AXIS4_tdata,
    output wire                          M_AXIS4_tvalid,

    output wire [SAXIS_TDATA_WIDTH-1:0]  M_AXIS_XSMON_tdata,
    output wire                          M_AXIS_XSMON_tvalid,
    output wire [SAXIS_TDATA_WIDTH-1:0]  M_AXIS_YSMON_tdata,
    output wire                          M_AXIS_YSMON_tvalid,
    output wire [SAXIS_TDATA_WIDTH-1:0]  M_AXIS_ZSMON_tdata,
    output wire                          M_AXIS_ZSMON_tvalid,

    output wire [SAXIS_TDATA_WIDTH-1:0]  M_AXIS_X0MON_tdata,
    output wire                          M_AXIS_X0MON_tvalid,
    output wire [SAXIS_TDATA_WIDTH-1:0]  M_AXIS_Y0MON_tdata,
    output wire                          M_AXIS_Y0MON_tvalid,
    
    output wire [SAXIS_TDATA_WIDTH-1:0]  M_AXIS_Z0MON_tdata,
    output wire                          M_AXIS_Z0MON_tvalid,
    
    output wire [SAXIS_TDATA_WIDTH-1:0]  M_AXIS_Z_SLOPE_tdata,
    output wire                          M_AXIS_Z_SLOPE_tvalid,
    
    output wire [SAXIS_TDATA_WIDTH-1:0]  M_AXIS_UrefMON_tdata,
    output wire                          M_AXIS_UrefMON_tvalid,

    output wire [S_AXIS_SC_TDATA_WIDTH-1:0]  M_AXIS_SC_tdata,
    output wire                              M_AXIS_SC_tvalid
    );
    
    // Xr  =   rotmxx*xs + rotmxy*ys
    // Yr  =  -rotmxy*xs + rotmxx*ys
    // X   = X0 + Xr
    // X   = Y0 + Yr 
    // Zsxy = slope_x * Xr + slope_y * Yr 
    // Z    = Z0 + z + Zsxy

    reg signed [32-1:0] xy_move_step = 32;
    reg signed [32-1:0] z_move_step = 1;
    
    reg signed [32-1:0] mx0s = 0;
    reg signed [32-1:0] my0s = 0;
    reg signed [32-1:0] mz0s = 0;
    reg signed [32-1:0] mu0s = 0;

    reg signed [32+1-1:0] mx0p = 0;
    reg signed [32+1-1:0] my0p = 0;
    reg signed [32+1-1:0] mz0p = 0;
    reg signed [32+1-1:0] mx0m = 0;
    reg signed [32+1-1:0] my0m = 0;
    reg signed [32+1-1:0] mz0m = 0;
    reg signed [32-1:0] mx0 = 0;
    reg signed [32-1:0] my0 = 0;
    reg signed [32-1:0] mz0 = 0;

    reg signed [32-1:0] mxx=0; // Q20
    reg signed [32-1:0] mxy=1<<20; // Q20


    reg signed [32-1:0] x=0;
    reg signed [32-1:0] y=0;
    reg signed [32-1:0] u=0;
    
    reg signed [32+QROTM+2-1:0] rrx=0;
    reg signed [32+QROTM+2-1:0] rry=0;

    reg signed [32+2-1:0] rx=0;
    reg signed [32+2-1:0] ry=0;
    //reg signed [32-1:0] rz=0;
    reg signed [32+2-1:0] ru=0;

    reg signed [32-1:0] slx=0; // SQ31
    reg signed [32-1:0] sly=0; // SQ31
    
    reg signed [32-1:0] z_servo=0;
    reg signed [32-1:0] dZx=0;
    reg signed [32-1:0] dZx_p=0;
    reg signed [32-1:0] dZx_m=0;
    reg signed [32-1:0] dZy=0;
    reg signed [32-1:0] dZy_p=0;
    reg signed [32-1:0] dZy_m=0;
    reg signed [32+1-1:0] z_slope=0;
    reg signed [32+1-1:0] z_gvp=0;
    reg signed [32+1-1:0] z_scan=0;
    reg signed [32-1:0] z_offset=0;
    reg signed [36-1:0] z_sum=0;

    reg signed [32+QSLOPE+1-1:0] dZmx=0;
    reg signed [32+QSLOPE+1-1:0] dZmy=0;
    
    
    reg signed [SC_DATA_WIDTH-1:0] s=0; // Q SC (25Q24)
    reg signed [SC_DATA_WIDTH-1:0] c=0; // Q SC (25Q24)
    reg signed [SC_DATA_WIDTH-1:0] mv=0;
    reg signed [3-1:0] mt=0;

    reg signed [2*SC_DATA_WIDTH-1:0] mod_tmp=0;
    reg signed [32-1:0] modulation=0;
   
    reg [RDECI:0] rdecii = 0;


// Value adjuster
`define ADJUSTER(REG, XP, XM, STEP, TARGET) \
    XP <= REG + STEP;  \
    XM <= REG - STEP;  \
    if (TARGET > XP)   \
        REG <= XP;     \
    else begin if (TARGET < XM) \
        REG <= XM;     \
    else               \
        REG <= TARGET; \
    end

// Saturated result to 32bit
`define SATURATE_32(REG) (REG > 33'sd2147483647 ? 32'sd2147483647 : REG < -33'sd2147483647 ? -32'sd2147483647 : REG[32-1:0]) 

    //always @ (posedge rdecii[RDECI])
    always @ (posedge a_clk)
    begin
        rdecii <= rdecii+1; // rdecii 00 01 *10 11 00 ...
        if (rdecii == 0)
        begin
            // LockIn Sin, Cos from DDS
            c <= S_AXIS_SC_tdata[                        SC_DATA_WIDTH-1 : 0]; // 25Q24 full dynamic range, proper rounding   24: 0
            s <= S_AXIS_SC_tdata[S_AXIS_SC_TDATA_WIDTH/2+SC_DATA_WIDTH-1 : S_AXIS_SC_TDATA_WIDTH/2]; // 25Q24 full dynamic range, proper rounding   56:32
            mv <= modulation_volume[32-1 : 32-SC_DATA_WIDTH];
            mt <= modulation_target[3-1:0];
            mod_tmp    <= mv * s;
            modulation <= mod_tmp >>> SC_Q_WIDTH; // remap to default 32
            
            // always buffer locally
            xy_move_step <= xy_offset_step; // XY offset adjuster speed limit (max step)
            z_move_step  <= z_offset_step; // Z offset / slope comp. speed limit (max step) when adjusting

            x <= S_AXIS_Xs_tdata[SAXIS_TDATA_WIDTH-1:0];
            y <= S_AXIS_Ys_tdata[SAXIS_TDATA_WIDTH-1:0];
            z_gvp <= S_AXIS_Zs_tdata[SAXIS_TDATA_WIDTH-1:0];
            u <= S_AXIS_U_tdata[SAXIS_TDATA_WIDTH-1:0];
            
            mxx <= rotmxx;
            mxy <= rotmxy;
    
            slx <= slope_x;
            sly <= slope_y;
    
            // XYZ Offset Adjusters -- Zoffset curretnly not used/obsolete
            mx0s <= x0;
            my0s <= y0;
            mz0s <= z0;
            mu0s <= u0;
            
             // MUST ASSURE mx0+/-xy_move_step never exceeds +/-Q31 -- exta bit used + saturation as assign -- to avoid over flow else a PBC jump will happen! 
            `ADJUSTER (mx0, mx0p, mx0m, xy_move_step, mx0s)
            `ADJUSTER (my0, my0p, my0m, xy_move_step, my0s)
            `ADJUSTER (mz0, mz0p, mz0m, z_move_step, mz0s)
                        
            // slope_x, y adjusters for smooth op
            `ADJUSTER (dZx, dZx_p, dZx_m, z_move_step, slx)
            `ADJUSTER (dZy, dZy_p, dZy_m, z_move_step, sly)

            // Bias set
            ru <= mu0s + u + (mt == 4 ? modulation : 0);
    
            // Scan Rotation
            rrx <=  mxx*x + mxy*y;
            rry <= -mxy*x + mxx*y;
            
            rx <= (rrx >>> QROTM) + mx0 + (mt == 1 ? modulation : 0); // final global X pos
            ry <= (rry >>> QROTM) + my0 + (mt == 2 ? modulation : 0); // final global Y pos
            
            // Z and slope comensation in global X,Y and in non rot coords. sys 0,0=invariant point
            z_servo  <= S_AXIS_Z_tdata;

            // Z slope calculation (plane)
            dZmx <= dZx * rx;
            dZmy <= dZy * ry;
            
            z_slope <= (dZmx >>> QSLOPE) + (dZmy >>> QSLOPE);
            z_scan  <= z_gvp + z_servo + (mt == 3 ? modulation : 0);
            z_sum   <= z_gvp + z_servo + (mt == 3 ? modulation : 0) + mz0;
            //z_sum    <= mz0 + z_gvp + z_servo + Z_slope;
        end
    end    
    
    assign M_AXIS1_tdata  = `SATURATE_32 (rx);
    assign M_AXIS1_tvalid = 1;
    assign M_AXIS_X0MON_tdata  = mx0;
    assign M_AXIS_X0MON_tvalid = 1;
    assign M_AXIS_XSMON_tdata  = x;
    assign M_AXIS_XSMON_tvalid = 1;
    
    assign M_AXIS2_tdata  = `SATURATE_32 (ry);
    assign M_AXIS2_tvalid = 1;
    
    assign M_AXIS_Y0MON_tdata  = my0;
    assign M_AXIS_Y0MON_tvalid = 1;
    assign M_AXIS_YSMON_tdata  = y;
    assign M_AXIS_YSMON_tvalid = 1;
    
    assign M_AXIS3_tdata  = `SATURATE_32 (z_sum);
    assign M_AXIS3_tvalid = 1;

    assign M_AXIS_ZSMON_tdata  = `SATURATE_32 (z_scan);
    assign M_AXIS_ZSMON_tvalid = 1;

    assign M_AXIS_Z0MON_tdata  = mz0; // Z Offset aka Z0
    assign M_AXIS_Z0MON_tvalid = 1;

    assign M_AXIS_Z_SLOPE_tdata  = `SATURATE_32 (z_slope); // slope compensation signal to be added saturation to z_sum before out
    assign M_AXIS_Z_SLOPE_tvalid = 1;

    
    assign M_AXIS4_tdata  = `SATURATE_32 (ru);
    assign M_AXIS4_tvalid = 1;
    
    assign M_AXIS_UrefMON_tdata  = mu0s;
    assign M_AXIS_UrefMON_tvalid = 1;

    // pass to LockIn 
    assign M_AXIS_SC_tdata  = S_AXIS_SC_tdata;
    assign M_AXIS_SC_tvalid = S_AXIS_SC_tvalid; 

    
endmodule
