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
    parameter RDECI = 4   // reduced rate decimation bits 1= 1/2 ...
)
(
    (* X_INTERFACE_PARAMETER = "ASSOCIATED_CLKEN a_clk, ASSOCIATED_BUSIF S_AXIS_Xs:S_AXIS_Ys:S_AXIS_Zs:S_AXIS_U:S_AXIS_Z:M_AXIS1:M_AXIS2:M_AXIS3:M_AXIS4:M_AXIS_XSMON:M_AXIS_YSMON:M_AXIS_ZSMON:M_AXIS_X0MON:M_AXIS_Y0MON:M_AXIS_Z0MON:M_AXIS_UrefMON" *)
    input a_clk,
    // GVP/SCAN COMPONENTS, ROTATED RELATIVE COORDS TO SCAN CENTER
    input wire [SAXIS_TDATA_WIDTH-1:0]  S_AXIS_Xs_tdata,
    input wire                          S_AXIS_Xs_tvalid,
    input wire [SAXIS_TDATA_WIDTH-1:0]  S_AXIS_Ys_tdata,
    input wire                          S_AXIS_Ys_tvalid,
    input wire [SAXIS_TDATA_WIDTH-1:0]  S_AXIS_Zs_tdata,
    input wire                          S_AXIS_Zs_tvalid,
    // Z Feedback Servo
    input  wire [SAXIS_TDATA_WIDTH-1:0]  S_AXIS_Z_tdata,
    input  wire                          S_AXIS_Z_tvalid,
    // Bias
    input wire [SAXIS_TDATA_WIDTH-1:0]  S_AXIS_U_tdata,
    input wire                          S_AXIS_U_tvalid,
    // two future control components using optional (DAC #5, #6)
    // input [32-1:0] motor1, // ..
    // input [32-1:0] motor2, // ..

    // scan rotation (yx=-xy, yy=xx)
    input [32-1:0] rotmxx, // =cos(alpha)
    input [32-1:0] rotmxy, // =sin(alpha)

    // slope -- TBD local to scan or global ???
    input [32-1:0] slope_x,
    input [32-1:0] slope_y,

    // SCAN OFFSET / POSITION COMPONENTS, ABSOLUTE COORDS
    input [32-1:0] x0, // vector components
    input [32-1:0] y0, // ..
    input [32-1:0] z0, // ..
    input [32-1:0] u0, // Bias Reference
    input [32-1:0] xy_offset_step, // @Q31 => Q31 / 120M => [18 sec full scale swin @ step 1 decii = 0]  x RDECI
    input [32-1:0] z_offset_step, // @Q31 => Q31 / 120M => [18 sec full scale swin @ step 1 decii = 0]  x RDECI

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
    
    output wire [SAXIS_TDATA_WIDTH-1:0]  M_AXIS_UrefMON_tdata,
    output wire                          M_AXIS_UrefMON_tvalid

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

    reg signed [32-1:0] mx0p = 0;
    reg signed [32-1:0] my0p = 0;
    reg signed [32-1:0] mz0p = 0;
    reg signed [32-1:0] mx0m = 0;
    reg signed [32-1:0] my0m = 0;
    reg signed [32-1:0] mz0m = 0;
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

    reg signed [32-1:0] rx=0;
    reg signed [32-1:0] ry=0;
    reg signed [32-1:0] rz=0;
    reg signed [32-1:0] ru=0;
    
    reg signed [32-1:0] z_servo=0;
    reg signed [32-1:0] z_slope=0;
    reg signed [32-1:0] z_gvp=0;
    reg signed [32-1:0] z_offset=0;
    reg signed [36-1:0] z_sum=0;
    
    reg [RDECI:0] rdecii = 0;


    always @ (posedge a_clk)
    begin
        rdecii <= rdecii+1;
    end

    always @ (posedge rdecii[RDECI])
    begin
    // always buffer locally
        xy_move_step <= xy_offset_step;
        z_move_step <= z_offset_step;
        x <= S_AXIS_Xs_tdata;
        y <= S_AXIS_Ys_tdata;
        z_gvp <= S_AXIS_Zs_tdata;
        u <= S_AXIS_U_tdata;
        mxx <= rotmxx;
        mxy <= rotmxy;

        // Offset Adjusters
        mx0s <= x0;
        my0s <= y0;
        mz0s <= z0;
        mu0s <= u0;
        
        // MUST ASSURE mx0+/-xy_move_step never exceeds +/-Q31 to avoid over flow else a PBC jump will happen! 
        
        mx0p <= mx0+xy_move_step;
        mx0m <= mx0-xy_move_step;
        if (mx0s > mx0p)
            mx0 <= mx0p;
        else begin if (mx0s < mx0m)
            mx0 <= mx0m;
        else
            mx0 <= mx0s;
        end    
             
        my0p <= my0+xy_move_step;
        my0m <= my0-xy_move_step;
        if (my0s > my0p)
            my0 <= my0p;
        else begin if (my0s < my0m)
            my0 <= my0m;
        else
            my0 <= my0s;
        end
        
        mz0p <= mz0+z_move_step;
        mz0m <= mz0-z_move_step;
        if (mz0s > mz0p)
            mz0 <= mz0p;
        else begin if (mz0s < mz0m)
            mz0 <= mz0m;
        else 
            mz0 <= mz0s;
        end        

        // Bias set
        ru <= mu0s + u;

        // Scan Rotation
        rrx <=  mxx*x + mxy*y;
        rry <= -mxy*x + mxx*y;
        
        rx <= (rrx >>> QROTM) + mx0;
        ry <= (rry >>> QROTM) + my0;
        
        // Z and slope
        z_servo  <= S_AXIS_Z_tdata;
        z_slope  <= 0;
        z_sum    <= mz0 + z_gvp + z_slope + z_servo;
        if (z_sum > 36'sd2147483647)
        begin
            rz <= 32'sd2147483648;
        end     
        else
        begin     
            if (z_sum < -36'sd2147483647)
            begin
                rz <= -32'sd2147483647;
            end 
            else
            begin
                rz <= z_sum[32-1:0];
            end
        end         
    end
    
    
    assign M_AXIS1_tdata  = rx;
    assign M_AXIS1_tvalid = 1;
    assign M_AXIS_X0MON_tdata  = mx0;
    assign M_AXIS_X0MON_tvalid = 1;
    assign M_AXIS_XSMON_tdata  = x;
    assign M_AXIS_XSMON_tvalid = 1;
    
    assign M_AXIS2_tdata  = ry;
    assign M_AXIS2_tvalid = 1;
    assign M_AXIS_Y0MON_tdata  = my0;
    assign M_AXIS_Y0MON_tvalid = 1;
    assign M_AXIS_YSMON_tdata  = y;
    assign M_AXIS_YSMON_tvalid = 1;
    
    assign M_AXIS3_tdata  = rz;
    assign M_AXIS3_tvalid = 1;
    assign M_AXIS_ZSMON_tdata  = z_gvp;  // Z-GVP aka scan
    assign M_AXIS_ZSMON_tvalid = 1;
    assign M_AXIS_Z0MON_tdata  = mz0; // Z Offset aka Z0
    assign M_AXIS_Z0MON_tvalid = 1;
    
    assign M_AXIS4_tdata  = ru;
    assign M_AXIS4_tvalid = 1;
    assign M_AXIS_UrefMON_tdata  = mu0s;
    assign M_AXIS_UrefMON_tvalid = 1;
    
endmodule
