`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 01/11/2025 12:04:22 AM
// Design Name: 
// Module Name: z_servo_configuration
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


module controller_configuration#(
    /* config address */
    parameter controller_reg_address = 99998,
    parameter controller_modes_reg_address = 99999,
    parameter width_limits = 32, // up to 64
    parameter width_consts = 32,
    parameter width_setpoint = 32,
    parameter width_threshold = 32
    )(
    (* X_INTERFACE_PARAMETER = "ASSOCIATED_CLKEN aclk, ASSOCIATED_BUSIF M_AXIS_setpoint:M_AXIS_reset:M_AXIS_threshold" *)
    input aclk,
    
    input [32-1:0]  config_addr,
    input [512-1:0] config_data,
    
    output wire [width_setpoint-1:0] M_AXIS_setpoint_tdata,
    output wire                      M_AXIS_setpoint_tvalid,
    output wire [width_consts-1:0] cp,
    output wire [width_consts-1:0] ci,
    output wire [width_limits-1:0] upper,
    output wire [width_limits-1:0] lower,

    output wire [width_threshold-1:0] M_AXIS_threshold_tdata,
    output wire                       M_AXIS_threshold_tvalid,

    output wire [width_limits-1:0] M_AXIS_reset_tdata,
    output wire                    M_AXIS_reset_tvalid,
    output wire controller_enable,
    output wire controller_mode,
    output wire controller_option_uw,
    output wire controller_option_th,
    output wire controller_hold);
        
    reg [width_setpoint-1:0] r_control_setpoint = 0;
    reg [width_limits-1:0] r_reset_value = 0;
    reg [width_consts-1:0] r_cp = 0;
    reg [width_consts-1:0] r_ci = 0;
    reg [width_limits-1:0] r_upper = 0;
    reg [width_limits-1:0] r_lower = 0;
    reg [width_threshold-1:0] r_threshold;

    reg [32-1:0] r_controller_mode = 0;


    assign M_AXIS_setpoint_tdata  = r_control_setpoint;
    assign M_AXIS_setpoint_tvalid = 1;
    assign cp = r_cp;
    assign ci = r_ci;
    assign upper = r_upper;
    assign lower = r_lower;
    assign M_AXIS_threshold_tdata = r_threshold;
    assign M_AXIS_threshold_tvalid = 1;

    assign M_AXIS_reset_tdata  = r_reset_value;
    assign M_AXIS_reset_tvalid = 1;

    assign controller_enable = r_controller_mode[0:0];
    assign controller_hold   = r_controller_mode[1:1];

    assign controller_option_uw = r_controller_mode[2:2];
    assign controller_option_th = r_controller_mode[3:3];

    always @(posedge aclk)
    begin
        // module configuration
        case (config_addr)
        controller_reg_address:
        begin
            r_control_setpoint <= config_data[2*32-1 : 2*32-width_setpoint]; //cfg[SRC_ADDR*32+SRC_BITS-1:SRC_ADDR*32+SRC_BITS-DST_WIDTH]
            r_cp               <= config_data[4*32-1 : 4*32-width_consts];
            r_ci               <= config_data[6*32-1 : 6*32-width_consts];
            r_upper            <= config_data[8*32-1 : 8*32-width_limits];
            r_lower            <= config_data[10*32-1: 10*32-width_limits];
        end   
          
        controller_modes_reg_address:
        begin
            r_reset_value        <= config_data[2*32-1 : 2*32-width_limits];
            r_controller_mode    <= config_data[4*32-1 : 3*32];
            r_threshold          <= config_data[6*32-1 : 6*32-width_threshold];  
        end     
        endcase
        
    end    
    
endmodule
