`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 05/06/2018 09:30:07 PM
// Design Name: 
// Module Name: controller_pi_tb
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


module controller_pi_tb();
    parameter AXIS_TDATA_WIDTH = 32; // INPUT AXIS DATA WIDTH
    parameter M_AXIS_CONTROL_TDATA_WIDTH = 32; // SERVO CONTROL DATA WIDTH OF AXIS
    parameter CONTROL_WIDTH = 32; // SERVO CONTROL DATA WIDTH
    parameter M_AXIS_CONTROL2_TDATA_WIDTH = 32; // INTERNAL CONTROl DATA WIDTH MAPPED TO AXIS FOR READOUT not including extend
    parameter CONTROL2_WIDTH = 50; // INTERNAL CONTROl DATA WIDTH not including extend **** COEFQ+AXIS_TDATA_WIDTH == CONTROL2_WIDTH
    parameter CONTROL2_OUT_WIDTH = 32; // max passed outside control width, must be <= CONTROL2_WIDTH
    parameter COEF_WIDTH = 32; // CP, CI WIDTH
    parameter QIN = 22; // Q In Signal
    parameter QCOEF = 31; // Q CP, CI's
    parameter QCONTROL = 31; // Q Controlvalue
    parameter CEXTEND = 4; // room for saturation check
    parameter DEXTEND = 1;  // data, erorr extend
    parameter AMCONTROL_ALLOW_NEG_SPECIAL = 1;
    
    reg [63:0] simtime;
    reg clock;
    reg signed [AXIS_TDATA_WIDTH-1:0] in_data;
    reg in_tvalid;
    reg signed [AXIS_TDATA_WIDTH-1:0] setpoint;
    reg signed [COEF_WIDTH-1:0]  cp;
    reg signed [COEF_WIDTH-1:0]  ci;
    reg signed [M_AXIS_CONTROL_TDATA_WIDTH-1:0]  limit_upper;
    reg signed [M_AXIS_CONTROL_TDATA_WIDTH-1:0]  limit_lower;    
    reg [M_AXIS_CONTROL_TDATA_WIDTH-1:0]  S_AXIS_reset_tdata;
    reg S_AXIS_reset_tvalid;
    reg enable;
    reg signed [AXIS_TDATA_WIDTH-1:0] q;

    wire [AXIS_TDATA_WIDTH-1:0] M_AXIS_PASS_tdata;
    wire                        M_AXIS_PASS_tvalid;
    wire [AXIS_TDATA_WIDTH-1:0] M_AXIS_PASS2_tdata;
    wire                        M_AXIS_PASS2_tvalid;
    wire [M_AXIS_CONTROL_TDATA_WIDTH-1:0] M_AXIS_CONTROL_tdata;  // may be less precision
    wire                                  M_AXIS_CONTROL_tvalid;
    wire [M_AXIS_CONTROL2_TDATA_WIDTH-1:0] M_AXIS_CONTROL2_tdata; // full precision without over flow bit
    wire                                   M_AXIS_CONTROL2_tvalid;
    wire signed [31:0] mon_signal;
    wire signed [31:0] mon_error;
    wire signed [31:0] mon_control;
    wire signed [31:0] mon_control_lower32;
    wire status_max;
    wire status_min;
    

    controller_pi Controller_PI (
              //.AMCONTROL_ALLOW_NEG_SPECIAL(1),
              //.CONTROL2_WIDTH(50),
              .aclk(clock),
              .S_AXIS_tdata(in_data),
              .S_AXIS_tvalid(in_tvalid),
              .setpoint(setpoint),
              .cp(cp),
              .ci(ci),
              .limit_upper(limit_upper),
              .limit_lower(limit_lower),
              .S_AXIS_reset_tdata(S_AXIS_reset_tdata),
              .S_AXIS_reset_tvalid(S_AXIS_reset_tvalid),
              .enable(enable),
              .M_AXIS_PASS_tdata(M_AXIS_PASS_tdata),
              .M_AXIS_PASS_tvalid(M_AXIS_PASS_tvalid),
              .M_AXIS_PASS2_tdata(M_AXIS_PASS2_tdata),
              .M_AXIS_PASS2_tvalid(M_AXIS_PASS2_tvalid),
              .M_AXIS_CONTROL_tdata(M_AXIS_CONTROL_tdata),  // may be less precision
              .M_AXIS_CONTROL_tvalid(M_AXIS_CONTROL_tvalid),
              .M_AXIS_CONTROL2_tdata(M_AXIS_CONTROL2_tdata), // full precision without over flow bit
              .M_AXIS_CONTROL2_tvalid(M_AXIS_CONTROL2_tvalid),
              .mon_signal(mon_signal),
              .mon_error(mon_error),
              .mon_control(mon_control),
              .mon_control_lower32(mon_control_lower32),
              .status_max(status_max),
              .status_min(status_min)
              );

    initial begin
        q=10;
        simtime=0;
        clock = 0;
        in_data = 23;
        in_tvalid = 1;
        setpoint = 20;
        // **** controlint_next <= controlint + ((ci*error) >>> (COEFQ));
        cp = 30000; // 0.0000001*(1<<QCOEF);
        ci = 20000; // 0.0000001*(1<<QCOEF);
        limit_upper = 200000;
        limit_lower = -200000;
        S_AXIS_reset_tdata = 0;
        S_AXIS_reset_tvalid = 1;
        enable = 1;
        
        forever #1 begin 
            clock = ~clock;
            simtime = simtime+1;
            if (simtime == 100) in_data = 15;
            if (simtime == 500) in_data = 19;
            if (simtime == 800) in_data = 21;
            if (simtime > 1000) in_data = M_AXIS_CONTROL2_tdata;
            //if (in_data < -400) q=1;
            //if (in_data >  400) q=-1;
            //in_data = in_data + q;
            // if (M_AXIS_CONTROL2_tdata < -100) q=1;
            // in_data = M_AXIS_CONTROL2_tdata*2;
        end
    end

endmodule
