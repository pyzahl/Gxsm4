`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
/* Gxsm - Gnome X Scanning Microscopy
 * universal STM/AFM/SARLS/SPALEED/... controlling and
 * data analysis software
 * 
 * Copyright (C) 1999,2000,2001,2002,2003 Percy Zahl
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
// Create Date: 11/26/2017 09:10:43 PM
// Design Name: 
// Module Name: lms_phase_amplitude_detector
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


module controller_pi #(
    parameter AXIS_TDATA_WIDTH = 32,            // INPUT AXIS DATA WIDTH                          PH: 32  AM: 24
    parameter M_AXIS_CONTROL_TDATA_WIDTH = 48,  // SERVO AXIS CONTROL DATA WIDTH OF AXIS          PH: 48  AM: 16
    parameter M_AXIS_CONTROL2_TDATA_WIDTH = 48, // INTERNAL CONTROl DATA WIDTH MAPPED             PH: 48  AM: 32
		                                // TO AXIS FOR READOUT not including extend
    parameter IN_Q = 22,       // Q of Controller Input Signal                                    PH: 22  AM: 22
    parameter IN_W = 23,       // Width of Controller Input Signal                                PH: 23  AM: 23
    parameter COEF_Q = 31,     // Q of CP, CI's                                                   PH: 31  AM: 31
    parameter COEF_W = 32,     // Width of CP, CI's                                               PH: 31  AM: 31
    parameter CONTROL_Q = 31,  // Q of Controlvalue                                               PH: 31  AM: 15
    parameter CONTROL_W = 44,  // Significant Width of Control Output Data                        PH: 44  AM: 16
    parameter CONTROL2_W = 32, // max passed outside control width, must be <= CONTROL2_WIDTH     PH: 32  AM: 16
    parameter AMCONTROL_ALLOW_NEG_SPECIAL = 0, // special AM controller behavior                       0      1
    parameter AUTO_RESET_AT_LIMIT  = 0,        // optional behavior instead of saturation at limits,   X      0
		                                       //  push control back to reset value
    parameter USE_RESET_DATA_INPUT = 1,        // has reset value AXIS input,                          1      1

    parameter RDECI = 1   // reduced rate decimation bits 1= 1/2 ...
    // Calculated and fixed Parameters -- ignore and do not change in bogus GUI: zW_XXXXX -- or better hand caclulate and check! Unclear/false behavior :( 
    //parameter zW_ERROR         = IN_W + 1,               // ACTUAL CONTROL ERROR WIDTH REQUIRED as of significant data range
    //parameter zW_EXTEND        = 1,                      // FOR SATURATION CHECK PURPOSE 
    //parameter zW_CONTROL_INT   = COEF_W+IN_W+zW_EXTEND  // INTERNAL CONTROL INTEGRATOR WIDTH
)
(
    (* X_INTERFACE_PARAMETER = "ASSOCIATED_CLKEN aclk" *)
    (* X_INTERFACE_PARAMETER = "ASSOCIATED_BUSIF S_AXIS:S_AXIS_reset:M_AXIS_PASS:M_AXIS_PASS2:M_AXIS_CONTROL:M_AXIS_CONTROL2:M_AXIS_CONTROL3" *)
    input aclk,
    input wire [AXIS_TDATA_WIDTH-1:0]  S_AXIS_tdata, // Controller data input
    input wire                         S_AXIS_tvalid,

    input wire signed [AXIS_TDATA_WIDTH-1:0] setpoint, // Controller Setpoint
    input wire signed [COEF_W-1:0]  cp, // Controller Parameter Proportional
    input wire signed [COEF_W-1:0]  ci, // Controller Parameter Integral
    input wire signed [M_AXIS_CONTROL_TDATA_WIDTH-1:0]  limit_upper, // Upper Control Limit
    input wire signed [M_AXIS_CONTROL_TDATA_WIDTH-1:0]  limit_lower, // Lower Control Limit

    input wire signed [M_AXIS_CONTROL_TDATA_WIDTH-1:0]  S_AXIS_reset_tdata, // Controller Reset/Start, i.e. disabled control output value
    input wire                                          S_AXIS_reset_tvalid,
    input wire enable,
    input wire control_hold,
    
    output wire [AXIS_TDATA_WIDTH-1:0] M_AXIS_PASS_tdata, // passed input
    output wire                        M_AXIS_PASS_tvalid,
    
    output wire [AXIS_TDATA_WIDTH-1:0] M_AXIS_PASS2_tdata, // passed input
    output wire                        M_AXIS_PASS2_tvalid,
    
    output wire [M_AXIS_CONTROL_TDATA_WIDTH-1:0] M_AXIS_CONTROL_tdata,  // control output, may be less precision
    output wire                                  M_AXIS_CONTROL_tvalid,
    
    output wire [M_AXIS_CONTROL2_TDATA_WIDTH-1:0] M_AXIS_CONTROL2_tdata, // control output, full precision without over flow bit
    output wire                                   M_AXIS_CONTROL2_tvalid,

    output wire [M_AXIS_CONTROL2_TDATA_WIDTH-1:0] M_AXIS_CONTROL3_tdata, // control output, full precision without over flow bit
    output wire                                   M_AXIS_CONTROL3_tvalid,

    output wire signed [31:0] mon_signal,
    output wire signed [31:0] mon_error,
    output wire signed [31:0] mon_control,
    output wire signed [31:0] mon_control_lower32,
    output wire signed [31:0] mon_control_B,

    output wire status_max,
    output wire status_min
    );
    
    // Calculated and fixed Parameters 
    localparam integer zW_ERROR         = IN_W + 1;               // ACTUAL CONTROL ERROR WIDTH REQUIRED as of significant data range
    localparam integer zW_EXTEND        = 1;                      // FOR SATURATION CHECK PURPOSE 
    localparam integer zW_CONTROL_INT   = COEF_W+IN_W+zW_EXTEND;  // INTERNAL CONTROL INTEGRATOR WIDTH


    reg signed [zW_ERROR-1:0] m=0;
    reg signed [zW_ERROR-1:0] error=0;
    reg signed [zW_ERROR-1:0] error_next=0;
    reg signed [zW_ERROR-1:0] reg_setpoint;
    reg signed [zW_CONTROL_INT-1:0] upper, lower, reset;
    reg signed [zW_CONTROL_INT-1:0] control=0;
    reg signed [zW_CONTROL_INT-1:0] control_next=0;
    reg signed [zW_CONTROL_INT-1:0] control_cie=0;
    reg signed [zW_CONTROL_INT-1:0] control_cpe=0;
    reg signed [zW_CONTROL_INT-1:0] controlint=0;
    reg signed [zW_CONTROL_INT-1:0] controlint_next=0;
    reg signed [COEF_W-1:0]  reg_cp;
    reg signed [COEF_W-1:0]  reg_ci;
    reg control_max;
    reg control_min;
    reg reg_enable;
    reg reg_hold;

    assign M_AXIS_PASS_tdata  = S_AXIS_tdata; // pass
    assign M_AXIS_PASS_tvalid = S_AXIS_tvalid; // pass
    assign M_AXIS_PASS2_tdata  = S_AXIS_tdata; // pass
    assign M_AXIS_PASS2_tvalid = S_AXIS_tvalid; // pass
   
    assign status_max = control_max;
    assign status_min = control_min;

/*
assign	w_convergent = i_data[(IWID-1):0]
			+ { {(OWID){1'b0}},
				i_data[(IWID-OWID)],
				{(IWID-OWID-1){!i_data[(IWID-OWID)]}}};
always @(posedge i_clk)
	o_convergent <= w_convergent[(IWID-1):(IWID-OWID)];
*/     

    reg [RDECI:0] rdecii = 0;

    always @ (posedge aclk)
    begin
        rdecii <= rdecii+1;
    end

    always @ (posedge rdecii[RDECI])
    begin
        upper <= {{(zW_EXTEND){       limit_upper[CONTROL_W-1]}},        limit_upper[CONTROL_W-1:0], {(zW_CONTROL_INT-CONTROL_W-zW_EXTEND){1'b0}}};  // sign extend and pad on right to control int width
        lower <= {{(zW_EXTEND){       limit_lower[CONTROL_W-1]}},        limit_lower[CONTROL_W-1:0], {(zW_CONTROL_INT-CONTROL_W-zW_EXTEND){1'b0}}};  // sign extend and pad on right to control int width
        if(USE_RESET_DATA_INPUT)
        begin
            reset <= {{(zW_EXTEND){S_AXIS_reset_tdata[CONTROL_W-1]}}, S_AXIS_reset_tdata[CONTROL_W-1:0], {(zW_CONTROL_INT-CONTROL_W-zW_EXTEND){1'b0}}};  // sign extend and pad on right to control int width
        end else begin
            reset <= 0;
        end
        error <= error_next;

        reg_cp <= cp;
        reg_ci <= ci;

        reg_enable <= enable;
        reg_hold   <= control_hold;

        // limit to range, in control mode
        if (reg_enable && control_next > upper)
        begin
            if (AUTO_RESET_AT_LIMIT)
            begin
                control      <= reset;
                controlint   <= reset;
            end
            else
            begin
                control      <= upper;
                controlint   <= upper;
            end
            control_max  <= 1;
            control_min  <= reg_enable? 0:1;
        end
        else 
        begin
            if (reg_enable && control_next < lower)
            begin
                if (AUTO_RESET_AT_LIMIT)
                begin
                    control      <= reset;
                    controlint   <= reset;
                end
                else
                begin
                    control      <= lower;
                    controlint   <= lower;
                end
                control_max  <= reg_enable? 0:1;
                control_min  <= 1;
            end 
            else
            begin
                if (AMCONTROL_ALLOW_NEG_SPECIAL)
                begin
                    if (error_next > $signed(0) && control_next < $signed(0)) // auto reset condition for amplitude control to preven negative phase, but allow active "damping"
                    begin
                        control      <= 0;
                        controlint   <= 0;
                    end
                    else
                    begin
                        control      <= control_next;
                        controlint   <= controlint_next;
                    end
                    control_max  <= reg_enable? 0:1;
                    control_min  <= reg_enable? 0:1;
                end 
                else
                begin
                    control      <= control_next;
                    controlint   <= controlint_next;
                    control_max  <= reg_enable? 0:1;
                    control_min  <= reg_enable? 0:1;
                end
            end
        end

        // prepare data to internal error data width zW_ERROR, signum extended
        m            <= {{(zW_ERROR-IN_W+1){S_AXIS_tdata[AXIS_TDATA_WIDTH-1]}},   S_AXIS_tdata[IN_W-1:0]};
        reg_setpoint <= {{(zW_ERROR-IN_W+1){setpoint[AXIS_TDATA_WIDTH-1]}},       setpoint[IN_W-1:0]};
        // calculate error
        error_next   <= reg_setpoint - m; // IN_W + 1  (zW_ERROR)

        if (reg_enable && !reg_hold) // run controller, integrate and prepare control output
        begin
            control_cie <= reg_ci*error; // ciX*error // saturation via extended range and limiter // Q64.. += Q31 x Q22
            control_cpe <= reg_cp*error; // cpX*error // saturation via extended range and limiter // Q64.. += Q31 x Q22
            controlint_next <= controlint + control_cie; // saturation via extended range and limiter // Q64.. += Q31 x Q22
            control_next    <= controlint + control_cpe; // 
        end 
        else // pass reset value as control
        begin
            if (!reg_enable)
            begin
                controlint_next <= reset;
                control_next    <= reset;
            end
        end
    end
 
    //assign M_AXIS_CONTROL_tdata   = {control[zW_CONTROL_INT-1], control[zW_CONTROL_INT-zW_EXTEND-2 : zW_CONTROL_INT-zW_EXTEND-CONTROL_W]}; // strip extension
    assign M_AXIS_CONTROL_tdata   = {{(M_AXIS_CONTROL_TDATA_WIDTH-CONTROL_W){control[zW_CONTROL_INT-1]}}, control[zW_CONTROL_INT-zW_EXTEND-1 : zW_CONTROL_INT-zW_EXTEND-CONTROL_W]}; // strip extension, expand to TDATA WIDTH
    assign M_AXIS_CONTROL_tvalid  = 1'b1;
    //assign M_AXIS_CONTROL2_tdata  = {control[zW_CONTROL_INT-1], control[zW_CONTROL_INT-zW_EXTEND-2 : zW_CONTROL_INT-zW_EXTEND-CONTROL2_W]}; // strip extension
    assign M_AXIS_CONTROL2_tdata  = {{(M_AXIS_CONTROL_TDATA_WIDTH-CONTROL2_W){control[zW_CONTROL_INT-1]}}, control[zW_CONTROL_INT-zW_EXTEND-1 : zW_CONTROL_INT-zW_EXTEND-CONTROL2_W]}; // strip extension, expand to TDATA WIDTH
    assign M_AXIS_CONTROL2_tvalid = 1'b1;
    //3rd out
    assign M_AXIS_CONTROL3_tdata  = {{(M_AXIS_CONTROL_TDATA_WIDTH-CONTROL2_W){control[zW_CONTROL_INT-1]}}, control[zW_CONTROL_INT-zW_EXTEND-1 : zW_CONTROL_INT-zW_EXTEND-CONTROL2_W]}; // strip extension, expand to TDATA WIDTH
    assign M_AXIS_CONTROL3_tvalid = 1'b1;

    assign mon_signal  = {{(32-zW_ERROR){m[zW_ERROR-1]}}, m[zW_ERROR-1:0]};
    assign mon_error   = {{(32-zW_ERROR){error[zW_ERROR-1]}}, error[zW_ERROR-1:0]};
    assign mon_control         = {control[zW_CONTROL_INT-1], control[zW_CONTROL_INT-zW_EXTEND-2 : zW_CONTROL_INT-zW_EXTEND-32]};
    assign mon_control_B       = {control[zW_CONTROL_INT-1], control[zW_CONTROL_INT-zW_EXTEND-2 : zW_CONTROL_INT-zW_EXTEND-32]};
    // assign mon_control_lower32 = {{control[zW_CONTROL_INT-zW_EXTEND-32-1 : ((zW_CONTROL_INT-zW_EXTEND) >= 64? zW_CONTROL_INT-zW_EXTEND-32-1-31:0)]}, {((zW_CONTROL_INT-zW_EXTEND) >= 64? 0:(64-zW_EXTEND-zW_CONTROL_INT)){1'b0}}}; // signed, lower 31 -- in case control_int_width is > 64
    // assign mon_control_lower32 = {{control[zW_CONTROL_INT-zW_EXTEND-32-1 : ((zW_CONTROL_INT-zW_EXTEND) >= 64? zW_CONTROL_INT-zW_EXTEND-32-1-31:0)]}, {((zW_CONTROL_INT-zW_EXTEND-2) >= 64? 0:(64-zW_EXTEND-zW_CONTROL_INT+2)){1'b0}}}; // signed, lower 31 -- in case control_int_width is > 64
    assign mon_control_lower32 = {control[zW_CONTROL_INT-zW_EXTEND-32-1 : 0], {(64-zW_EXTEND-zW_CONTROL_INT+2){1'b0}}}; // lower 32 (or less) bits left aligned

//                                              57 - 1 - 32 - 1 = 23    : 0 [24]  ,  64 - 1 - 57 + 2 = 8 => 32
//  0111001011111111
//0111000010111111
//      001001010000 250 DDS SOLL
//    XX0010010100    94 DDS READ BACL FPGA
endmodule
