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
    parameter readback_AD463x_address         = 100100,
    parameter readbackTimingTest_reg_address  = 101999,
    parameter readbackTimingReset_reg_address = 102000,
    parameter readback_RPSPMC_PACPLL_Version  = 199997,
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

    input wire [32-1:0] AD463x_CH1,
    input wire [32-1:0] AD463x_CH2,

    input wire [32-1:0] rbXa,
    input wire [32-1:0] rbXb
);

    reg [32-1:0]		reg_A=0;
    reg [32-1:0]		reg_B=0;
   
        
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
        readbackTimingTest_reg_address:
	    begin
            reg_A <= 125000000; 
            reg_B <= reg_A; 
	    end
	    readback_RPSPMC_PACPLL_Version:
	    begin
            reg_A <= 32'hEC010099; 
            reg_B <= 32'h20250202;
	    end
	default:
	  begin
            reg_A <= reg_A+1;
            reg_B <= reg_A+13;
	  end
        endcase
    end    
    
endmodule
