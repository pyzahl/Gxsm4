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
// Create Date: 01/11/2025 12:04:22 AM
// Design Name:    part of RPSPMC
// Module Name:    readback_configuration via GPIO
// Project Name:   RPSPMC 4 GXSM
// Target Devices: Zynq z7020
// Tool Versions:  Vivado 2023.1
// Description:    configuration or data reader by module address
// 
// Dependencies: 
// 
// Revision:
// Revision 0.01 - File Created
// Additional Comments:
// 
//////////////////////////////////////////////////////////////////////////////////


module readback_configuration#(
    /* module readback register addresses */
    parameter readback_Z_reg_address          = 100001,
    parameter readback_Bias_reg_address       = 100002,
    parameter readback_GVPBias_reg_address    = 100003,
    parameter readback_PMD_DA56_reg_address   = 100004,
    parameter readback_Z_SERVO_RB_reg_address = 100005,
    parameter readback_AMC_FMC_reg_address    = 100006,
    parameter readback_SRCS_MUX_reg_address   = 100010,
    parameter readback_IN_MUX_reg_address     = 100011,
    parameter readback_AD463x_reg_address     = 100100,
    parameter readback_uptime_address         = 101900,
    parameter readbackTimingTest_reg_address  = 101999,
    parameter readbackTimingReset_reg_address = 102000,
    parameter readback_RPSPMC_PACPLL_Version  = 199997,
    parameter readback_system_state           = 199999,
    parameter readbackX_reg_address = 100999
    )(
    input aclk,
    
    input  [32-1:0] config_addr,
    output [32-1:0] gpio_dataA,
    output [32-1:0] gpio_dataB,

    input wire [32-1:0] Z_GVP_mon,
    input wire [32-1:0] Z_slope_mon,

    input wire [32-1:0] Bias_SUM_mon,    // Total Bias Sum: U0+GVP+Mod
    input wire [32-1:0] Bias_U0BIAS_mon, // GXSM Bias Set Value

    input wire [32-1:0] Bias_GVP_mon,    // GVP genertae Bias Offset
    input wire [32-1:0] Bias_MOD_mon,    // Bias AUX/Modifiers, LockIn,...

    input wire [32-1:0] PMD_DA_5A,
    input wire [32-1:0] PMD_DA_6B,

    input wire [32-1:0] GVP_AMC,
    input wire [32-1:0] GVP_FMC,

    input wire [32-1:0] AD463x_CH1,
    input wire [32-1:0] AD463x_CH2,

    input wire [32-1:0] Z_SERVO_RB_A,
    input wire [32-1:0] Z_SERVO_RB_B,

    input wire [32-1:0] SRCS_MUX_SEL,
    input wire [32-1:0] IN_MUX_SEL,

    input wire [32-1:0] rbXa,
    input wire [32-1:0] rbXb,
    
    output wire [32-1:0] clock_sec,
    output wire [32-1:0] clock_8ns_tics
);

    reg [32-1:0]		reg_A=0;
    reg [32-1:0]		reg_B=0;
   
    reg [32-1:0]        reg_system_state=0;
    reg                 reg_system_startup=1;
    reg                 once=0;

    reg [32-1:0]        seconds_deci=0;
    reg [32-1:0]        seconds_up=0;
        
    assign gpio_dataA = reg_A;
    assign gpio_dataB = reg_B;

    always @(posedge aclk)
    begin
        // module readback configuration
        case (config_addr)
        readback_Z_reg_address:
        begin
            reg_A <= Z_GVP_mon;
            reg_B <= Z_slope_mon;
	    end

        readback_Bias_reg_address:
        begin
            reg_A <= Bias_SUM_mon;
            reg_B <= Bias_U0BIAS_mon;
	    end
	  
        readback_GVPBias_reg_address:
        begin
            reg_A <= Bias_GVP_mon;
            reg_B <= Bias_MOD_mon;
	    end

        readback_PMD_DA56_reg_address: 
        begin
            reg_A <= PMD_DA_5A;
            reg_B <= PMD_DA_6B;
	    end

        readback_Z_SERVO_RB_reg_address: 
        begin
            reg_A <= Z_SERVO_RB_A;
            reg_B <= Z_SERVO_RB_B;
	    end
	    
        readback_AMC_FMC_reg_address:
        begin
            reg_A <= GVP_AMC;
            reg_B <= GVP_FMC;
	    end
        
        readback_SRCS_MUX_reg_address: 
        begin
            reg_A <= SRCS_MUX_SEL;
            reg_B <= IN_MUX_SEL;
	    end

        readback_IN_MUX_reg_address: 
        begin
            reg_A <= IN_MUX_SEL;
            reg_B <= 0;
	    end

        readback_AD463x_reg_address: 
        begin
            reg_A <= AD463x_CH1;
            reg_B <= AD463x_CH2;
	    end
	  
        readbackX_reg_address:
	    begin
            reg_A <= rbXa;
            reg_B <= rbXb;
	    end
        readbackTimingReset_reg_address:
	    begin
            reg_A <= 0; 
            reg_B <= 0; 
	    end
	    readback_uptime_address:
	    begin
            reg_A <= seconds_up;   // FPGA uptime in seconds or monotonouse clock (~136 years before cycling)
            reg_B <= seconds_deci; // FPGA uptime fraction seconds in 8ns (1/125MHz)
	    end
        readbackTimingTest_reg_address:
	    begin
            reg_A <= 125000000; 
            reg_B <= reg_A; 
	    end
	    readback_RPSPMC_PACPLL_Version:
	    begin
            reg_A <= 32'hEC010099; 
            reg_B <= 32'h20250501;
            
            if (reg_system_startup) // once ever after FPGA init (=1 inititially) and version read
                reg_system_startup <= 0; // never again

	    end
	    
	    readback_system_state: // system cold start detection logic: control of system_state register
	    begin
	       reg_A <= reg_system_state;
	       reg_B <= { 31'h0, reg_system_startup };

	       if (once)
	       begin
	           reg_system_state   <= reg_system_state+1; // increment at each readback_system_state reading
	           once               <= 0;
            end           
        end
        default:
        begin
            reg_A <= reg_A+1;
            reg_B <= reg_A+13;
            once  <= 1;
        end
        endcase

        // FPGA uptime and clock base
        if (seconds_deci == 0)
        begin       
            seconds_deci <= 32'd124999999;
            seconds_up   <= seconds_up + 1;
        end 
        else
        begin
            seconds_deci <= seconds_deci-1;
        end
    end    

    assign clock_sec      = seconds_up;
    assign clock_8ns_tics = seconds_deci;
    
endmodule
