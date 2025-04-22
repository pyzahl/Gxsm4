`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company:  BNL
// Engineer: Percy Zahl
// 
/* Gxsm - Gnome X Scanning Microscopy 
 * ** FPGA Implementaions RPSPMC aka RedPitaya Scanning Probe Control **
 * universal STM/AFM/SARLS/SPALEED/... controlling and
 * data analysis software
 * 
 * Copyright (C) 1999-2025 by Percy Zahl
 *
 * Authors: Percy Zahl <zahl@users.sf.net>
 * WWW Home: http://gxsm.sf.net
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 */
// 
// Create Date: 08/14/2023 10:22:53 PM
// Design Name:    part of RPSPMC
// Module Name:    axis_spm_control
// Project Name:   RPSPMC 4 GXSM
// Target Devices: Zynq z7020
// Tool Versions:  Vivado 2023.1
// Description:    Main SPM control logics for:
//                 signal routing, offsets control, bias, Z-plane, scan rotation, etc.
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
    parameter QROTM = 24,
    parameter QSLOPE = 24,
    parameter QSIGNALS = 31,
    parameter S_AXIS_SREF_TDATA_WIDTH = 32,
    parameter SREF_DATA_WIDTH  = 25,  // SC 25Q24
    parameter SREF_Q_WIDTH     = 24,  // SC 25Q24
    parameter RDECI  = 5,   // reduced rate decimation bits 1= 1/2 ...
    parameter xyzu_offset_reg_address = 1100,
    parameter rotm_reg_address = 1101,
    parameter slope_reg_address = 1102,
    parameter modulation_reg_address = 1103,
    parameter bias_reg_address = 1104,
    parameter z_polarity_configuration = 1105
)(
    (* X_INTERFACE_PARAMETER = "ASSOCIATED_CLKEN a_clk, ASSOCIATED_BUSIF S_AXIS_Xs:S_AXIS_Ys:S_AXIS_Zs:S_AXIS_U:S_AXIS_A:S_AXIS_B:S_AXIS_SREF:S_AXIS_Z:S_AXIS_dFC:M_AXIS1:M_AXIS2:M_AXIS3:M_AXIS4:M_AXIS3:M_AXIS5:M_AXIS3:M_AXIS6:M_AXIS_XSMON:M_AXIS_YSMON:M_AXIS_ZSMON:M_AXIS_X0MON:M_AXIS_Z_SLOPE:M_AXIS_Y0MON:M_AXIS_ZGVPMON:M_AXIS_UGVPMON:M_AXIS_U0BIASMON" *)
    input a_clk,
    input [32-1:0]  config_addr,
    input [512-1:0] config_data,
    //output wire [32-1:0] config_readback, // default = 0; or 'Z' ??? possible

    // GVP/SCAN COMPONENTS, ROTATED RELATIVE COORDS TO SCAN CENTER
    input wire [SAXIS_TDATA_WIDTH-1:0]  S_AXIS_Xs_tdata,
    input wire                          S_AXIS_Xs_tvalid,
    input wire [SAXIS_TDATA_WIDTH-1:0]  S_AXIS_Ys_tdata,
    input wire                          S_AXIS_Ys_tvalid,
    input wire [SAXIS_TDATA_WIDTH-1:0]  S_AXIS_Zs_tdata,
    input wire                          S_AXIS_Zs_tvalid,
    // Z Feedback Servo STM
    input  wire [SAXIS_TDATA_WIDTH-1:0] S_AXIS_Z_tdata,
    input  wire                         S_AXIS_Z_tvalid,
    // Z Feedback Servo dFREQ
    input  wire [SAXIS_TDATA_WIDTH-1:0] S_AXIS_dFC_tdata,
    input  wire                         S_AXIS_dFC_tvalid,
    // Bias
    input wire [SAXIS_TDATA_WIDTH-1:0]  S_AXIS_U_tdata,
    input wire                          S_AXIS_U_tvalid,
    // A,B vector channels
    input wire [SAXIS_TDATA_WIDTH-1:0]  S_AXIS_A_tdata,
    input wire                          S_AXIS_A_tvalid,
    input wire [SAXIS_TDATA_WIDTH-1:0]  S_AXIS_B_tdata,
    input wire                          S_AXIS_B_tvalid,
    
    // SC Lock-In Reference and controls
    input wire [S_AXIS_SREF_TDATA_WIDTH-1:0]  S_AXIS_SREF_tdata,
    input wire                                S_AXIS_SREF_tvalid,

    output wire [SAXIS_TDATA_WIDTH-1:0]  M_AXIS1_tdata,
    output wire                          M_AXIS1_tvalid,
    output wire [SAXIS_TDATA_WIDTH-1:0]  M_AXIS2_tdata,
    output wire                          M_AXIS2_tvalid,
    output wire [SAXIS_TDATA_WIDTH-1:0]  M_AXIS3_tdata,
    output wire                          M_AXIS3_tvalid,
    output wire [SAXIS_TDATA_WIDTH-1:0]  M_AXIS4_tdata,
    output wire                          M_AXIS4_tvalid,
    output wire [SAXIS_TDATA_WIDTH-1:0]  M_AXIS5_tdata,
    output wire                          M_AXIS5_tvalid,
    output wire [SAXIS_TDATA_WIDTH-1:0]  M_AXIS6_tdata,
    output wire                          M_AXIS6_tvalid,

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
    output wire [SAXIS_TDATA_WIDTH-1:0]  M_AXIS_ZGVPMON_tdata,
    output wire                          M_AXIS_ZGVPMON_tvalid,
    
    output wire [SAXIS_TDATA_WIDTH-1:0]  M_AXIS_Z_SLOPE_tdata,
    output wire                          M_AXIS_Z_SLOPE_tvalid,
    
    output wire z_polarity,

    output wire [SAXIS_TDATA_WIDTH-1:0]  M_AXIS_UGVPMON_tdata,
    output wire                          M_AXIS_UGVPMON_tvalid,
    output wire [SAXIS_TDATA_WIDTH-1:0]  M_AXIS_U0BIASMON_tdata,
    output wire                          M_AXIS_U0BIASMON_tvalid
    );
    
    //reg [32-1:0] reg_config_readback = 0;
    
    
    // Xr  =   rotmxx*xs + rotmxy*ys
    // Yr  =  -rotmxy*xs + rotmxx*ys
    // X   = X0 + Xr
    // X   = Y0 + Yr 
    // Zsxy = slope_x * Xr + slope_y * Yr 
    // Z    = Z0 + z + Zsxy

    reg signed [32-1:0] modulation_volume=0; // volume for modulation Q31
    reg [4-1:0] modulation_target=0; // target signal for mod (#XYZUAB)
    reg [4-1:0] dFControl_target=0;  // target for dFControl: B0: Z, B1: Bias

    reg negate_z=0;

    // scan rotation (yx=-xy; yy=xx)
    reg signed [QROTM+1-1:0] rotmxx=0; // =cos(alpha) Q20
    reg signed [QROTM+1-1:0] rotmxy=1<<QROTM; // =sin(alpha) Q20

    // slope -- always applied in global XY plane ???
    reg signed [QSLOPE+1-1:0] slope_x=0; // SQSLOPE (31)
    reg signed [QSLOPE+1-1:0] slope_y=0; // SQSLOPE (31)

    // SCAN OFFSET / POSITION COMPONENTS; ABSOLUTE COORDS
    reg signed [32-1:0] x0=0; // vector components
    reg signed [32-1:0] y0=0; // ..
    reg signed [32-1:0] z0=0; // ..
    reg signed [32-1:0] u0_bias=0; // Bias Reference, GXSM Bias Set Value
    reg signed [32-1:0] xy_offset_step=32; // @Q31 => Q31 / 120M => [18 sec full scale swin @ step 1 decii = 0]  x RDECI
    reg signed [32-1:0] z_offset_step=32; // @Q31 => Q31 / 120M => [18 sec full scale swin @ step 1 decii = 0]  x RDECI

    reg signed [32-1:0] xy_move_step = 32;
    reg signed [32-1:0] z_move_step = 1;
    
    // adjuster tmp's
    reg signed [32+1-1:0] mx0p = 0;
    reg signed [32+1-1:0] my0p = 0;
    reg signed [32+1-1:0] mz0p = 0;
    reg signed [32+1-1:0] mx0m = 0;
    reg signed [32+1-1:0] my0m = 0;
    reg signed [32+1-1:0] mz0m = 0;
    reg signed [32+1-1:0] mx0 = 0;
    reg signed [32+1-1:0] my0 = 0;
    reg signed [32+1-1:0] mz0 = 0;



    // GVP controls
    reg signed [32-1:0] x=0;
    reg signed [32-1:0] y=0;
    reg signed [32-1:0] z_gvp=0;
    reg signed [32-1:0] u_gvp=0;
    
    reg signed [32+QROTM+2-1:0] rrxxx=0;
    reg signed [32+QROTM+2-1:0] rryxy=0;
    reg signed [32+QROTM+2-1:0] rrxyx=0;
    reg signed [32+QROTM+2-1:0] rryyy=0;

    reg signed [32+2-1:0] srx=0;
    reg signed [32+2-1:0] sry=0;
    reg signed [32+2-1:0] rx=0;
    reg signed [32+2-1:0] ry=0;
    //reg signed [32-1:0] rz=0;
    //reg signed [32+2-1:0] ru2=0;
    reg signed [32+2-1:0] ru=0;

    reg signed [32-1:0] AXa=0;
    reg signed [32-1:0] AXb=0;
    reg signed [32+1-1:0] rA=0;
    reg signed [32+1-1:0] rB=0;


    reg signed [32-1:0] slx=0; // SQ31
    reg signed [32-1:0] sly=0; // SQ31
    
    reg signed [32-1:0] z_servo_stm=0;
    reg signed [32-1:0] servo_dFC=0;
    reg signed [32-1:0] dZx=0;
    reg signed [32-1:0] dZx_p=0;
    reg signed [32-1:0] dZx_m=0;
    reg signed [32-1:0] dZy=0;
    reg signed [32-1:0] dZy_p=0;
    reg signed [32-1:0] dZy_m=0;
    reg signed [32+1-1:0] z_slope=0;
    reg signed [36-1:0] z_sum=0;

    reg signed [32+QSLOPE+1-1:0] dZmx=0;
    reg signed [32+QSLOPE+1-1:0] dZmy=0;
    
    
    reg signed [SREF_DATA_WIDTH-1:0] s=0; // Q SC (25Q24)
    reg signed [SREF_DATA_WIDTH-1:0] mv=0;
    reg [4-1:0] mt=0;

    reg signed [2*SREF_DATA_WIDTH-1:0] mod_tmp=0;
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

//assign config_readback = reg_config_readback;

    always @ (posedge a_clk)
    begin
        // module configuration
        case (config_addr) // manage MAIN SPM CONTROL configuration registers
        bias_reg_address: // Bias only
        begin
            u0_bias <= $signed(config_data[1*32-1 : 0*32]); // Gxsm Bias Reference
        end

        xyzu_offset_reg_address:
        begin
            // SCAN OFFSET / POSITION COMPONENTS, ABSOLUTE COORDS
            x0 <= $signed(config_data[1*32-1 : 0*32]); // vector components
            y0 <= $signed(config_data[2*32-1 : 1*32]); // ..
            z0 <= $signed(config_data[3*32-1 : 2*32]); // ..
            u0_bias <= $signed(config_data[4*32-1 : 3*32]); // Gxsm Bias Reference
            
            xy_offset_step <= $signed(config_data[5*32-1 : 4*32]); // @Q31 => Q31 / 120M => [18 sec full scale swin @ step 1 decii = 0]  x RDECI
            z_offset_step  <= $signed(config_data[6*32-1 : 5*32]); // @Q31 => Q31 / 120M => [18 sec full scale swin @ step 1 decii = 0]  x RDECI
        end


        rotm_reg_address:
        begin
            // scan rotation (yx=-xy, yy=xx)
            rotmxx <= $signed(config_data[0*32+QROTM+1-1 : 0*32]); // =cos(alpha) // QROTM
            rotmxy <= $signed(config_data[1*32+QROTM+1-1 : 1*32]); // =sin(alpha)
        end

        slope_reg_address:
        begin
            // slope -- always applied in global XY plane ???
            slope_x <= $signed(config_data[0*32+QSLOPE+1-1 : 0*32]); // SQSLOPE (31)
            slope_y <= $signed(config_data[1*32+QSLOPE+1-1 : 1*32]); // SQSLOPE (31)
        end

        modulation_reg_address:
        begin
            // modulation control
            modulation_volume <= $signed(config_data[1*32-1 : 0*32]); // volume for modulation Q31
            modulation_target <= config_data[1*32+4-1 : 1*32]; // target signal for mod (#XYZUAB)
            dFControl_target  <= config_data[2*32+4-1 : 2*32]; // target for dFControl: B0: Z, B1: Bias
        end

        z_polarity_configuration:
        begin
            negate_z <= config_data[0*32 : 0*32];  // WARNING: Adjusting this will casue a Z jump depending on Z+Slope been non zero. Do once at startup ONLY when Z+Slope = 0! Default: -1
        end
        endcase


        rdecii <= rdecii+1; // rdecii 00 01 *10 11 00 ...
        if (rdecii == 0)
        begin
            // LockIn Sin Ref from DDS
            s  <= $signed (S_AXIS_SREF_tdata[SREF_DATA_WIDTH-1:0]); // SRef is 32bit wide, but only SREF_DATA_WIDTH-1:0 used
            mv <= $signed (modulation_volume[32-1 : 32-SREF_DATA_WIDTH]); // SQ31 value, truncating.
            mt <= modulation_target[4-1:0];
            mod_tmp    <= mv * s;
            modulation <= mod_tmp >>> (SREF_Q_WIDTH - (QSIGNALS-SREF_Q_WIDTH)); // remap to default 32:   2*SREF_Q>> << SIGNALS_Q = >>> 24 - 31-24
            
            // buffer locally
            xy_move_step <= xy_offset_step; // XY offset adjuster speed limit (max step)
            z_move_step  <= z_offset_step; // Z offset / slope comp. speed limit (max step) when adjusting

            // GVP CONTROL COMPONENTS:
            x     <= S_AXIS_Xs_tdata[SAXIS_TDATA_WIDTH-1:0];
            y     <= S_AXIS_Ys_tdata[SAXIS_TDATA_WIDTH-1:0];
            z_gvp <= S_AXIS_Zs_tdata[SAXIS_TDATA_WIDTH-1:0];
            u_gvp <= S_AXIS_U_tdata[SAXIS_TDATA_WIDTH-1:0];
            AXa   <= S_AXIS_A_tdata[SAXIS_TDATA_WIDTH-1:0];
            AXb   <= S_AXIS_B_tdata[SAXIS_TDATA_WIDTH-1:0];

            // Z Servo Control Components
            z_servo_stm  <= S_AXIS_Z_tdata;    // STM Control on Current
            servo_dFC    <= S_AXIS_dFC_tdata;  // ZFM Control on dFreq or Bias Control on dFreq

            // XYZ Offset Adjusters -- Zoffset currenly not used/obsolete

            // MUST ASSURE mx0+/-xy_move_step never exceeds +/-Q31 -- exta bit used + saturation as assign -- to avoid over flow else a PBC jump will happen! 
            `ADJUSTER (mx0, mx0p, mx0m, xy_move_step, x0)
            `ADJUSTER (my0, my0p, my0m, xy_move_step, y0)
            `ADJUSTER (mz0, mz0p, mz0m, z_move_step, z0)
                        
            // slope_x, y adjusters for smooth op
            `ADJUSTER (dZx, dZx_p, dZx_m, z_move_step, slope_x)
            `ADJUSTER (dZy, dZy_p, dZy_m, z_move_step, slope_y)

            // Scan Rotation
            //rrx <=  rotmxx*x + rotmxy*y;
            //rry <= -rotmxy*x + rotmxx*y;
            rrxxx <= rotmxx*x;
            rryxy <= rotmxy*y;
            rrxyx <= rotmxy*x;
            rryyy <= rotmxx*y;
            srx <= ( rrxxx+rryxy) >>> QROTM;
            sry <= (-rrxyx+rryyy) >>> QROTM;
            rx <= srx + mx0 + (mt == 1 ? modulation : 0); // final global X pos
            ry <= sry + my0 + (mt == 2 ? modulation : 0); // final global Y pos
            
            // Z slope calculation (plane)
            dZmx <= dZx * rx;
            dZmy <= dZy * ry;
            
            // Z and slope comensation in global X,Y and in non rot coords. sys 0,0=invariant point
            z_slope <= (dZmx >>> QSLOPE) + (dZmy >>> QSLOPE);
            z_sum   <= z_gvp + z_servo_stm + (dFControl_target[0] ? servo_dFC : 0) + (mt == 3 ? modulation : 0) + mz0;
            //z_sum   <= z_gvp + z_servo + Z_slope; // slope is SAT-added externally later

            // Bias := u0_bias (User Bias by GXSM) + u_gvp (GVP bias manipulation component) + LockIn Mod 
            ru  <= u0_bias + u_gvp + (dFControl_target[1] ? servo_dFC : 0) + ((mt == 4 || mt == 7 )? modulation : 0);
    

            // AUX AXIS A,B
            rA  <= AXa + (mt == 5 ? modulation : 0) + (mt == 7 ? mv >>> 2 : 0);
            rB  <= AXb + (mt == 6 ? modulation : 0);
        end
    end    

// X    
    assign M_AXIS1_tdata  = `SATURATE_32 (rx);
    assign M_AXIS1_tvalid = 1;
    assign M_AXIS_X0MON_tdata  = mx0;
    assign M_AXIS_X0MON_tvalid = 1;
    assign M_AXIS_XSMON_tdata  = x;
    assign M_AXIS_XSMON_tvalid = 1;
    
// Y    
    assign M_AXIS2_tdata  = `SATURATE_32 (ry);
    assign M_AXIS2_tvalid = 1;
    assign M_AXIS_Y0MON_tdata  = my0;
    assign M_AXIS_Y0MON_tvalid = 1;
    assign M_AXIS_YSMON_tdata  = y;
    assign M_AXIS_YSMON_tvalid = 1;

// Z    
    assign M_AXIS3_tdata  = `SATURATE_32 (z_sum); // Z total
    assign M_AXIS3_tvalid = 1;
    assign M_AXIS_ZGVPMON_tdata  = z_gvp;
    assign M_AXIS_ZGVPMON_tvalid = 1;
    assign M_AXIS_ZSMON_tdata  = `SATURATE_32 (z_sum);
    assign M_AXIS_ZSMON_tvalid = 1;

    assign M_AXIS_Z_SLOPE_tdata  = `SATURATE_32 (z_slope); // slope compensation signal to be added saturation to z_sum before out
    assign M_AXIS_Z_SLOPE_tvalid = 1;

    assign z_polarity = negate_z; // Z Polarity. Applied in S_ADD to both: +/-(slope+Z)
    
// U/BIAS    
    assign M_AXIS4_tdata  = `SATURATE_32 (ru); // Bias total
    assign M_AXIS4_tvalid = 1;
    
    assign M_AXIS_UGVPMON_tdata  = u_gvp;
    assign M_AXIS_UGVPMON_tvalid = 1;
    assign M_AXIS_U0BIASMON_tdata  = u0_bias;
    assign M_AXIS_U0BIASMON_tvalid = 1;

// A
    assign M_AXIS5_tdata  = `SATURATE_32 (rA);
    assign M_AXIS5_tvalid = S_AXIS_A_tvalid;
// B
    assign M_AXIS6_tdata  = `SATURATE_32 (rB);
    assign M_AXIS6_tvalid = S_AXIS_B_tvalid;
    
endmodule
