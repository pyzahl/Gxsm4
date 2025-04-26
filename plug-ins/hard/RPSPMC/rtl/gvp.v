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
// Create Date: 11/26/2017 08:20:47 PM
// Module Name:    AD5791 io connect for up to 6 PMODs
// Company: GXSM
// Engineer: Percy Zahl
// 
// Create Date: 12/31/2022 08:50:50 PM
// Design Name:    part of RPSPMC
// Module Name:    gvp
// Project Name:   RPSPMC 4 GXSM
// Target Devices: Zynq z7020
// Tool Versions:  Vivado 2023.1
// Description:    General Vector Program Execution Core (C) P.Zahl
// 
// Dependencies: 
// 
// Revision:
// Revision 0.01 - File Created
// Additional Comments:
// 
//////////////////////////////////////////////////////////////////////////////////


module gvp #(
    parameter NUM_VECTORS_N2 = 4,
    parameter NUM_VECTORS    = 16,
    parameter control_reg_address = 5001,
    parameter reset_options_reg_address = 5002,
    parameter vector_programming_reg_address  = 5003,
    parameter vector_set_reg_address  = 5004,
    parameter vectorX_programming_reg_address  = 5005,
    parameter vector_reset_components_reg_address = 5009
)
(
    (* X_INTERFACE_PARAMETER = "ASSOCIATED_CLKEN a_clk, ASSOCIATED_BUSIF S_AXIS_XV:M_AXIS_X:M_AXIS_Y:M_AXIS_Z:M_AXIS_U:M_AXIS_A:M_AXIS_B:M_AXIS_AM:M_AXIS_FM:M_AXIS_SRCS:M_AXIS_INDEX:M_AXIS_GVP_TIME" *)
    input		 a_clk, // clocking up to aclk
    //input		 reset, // put into reset mode (set program and hold)
    //input		 pause, // put/release into/from pause mode -- always completes the "ii" nop cycles!
    //input		 setvec, // program vector data using vp_set data
    input [32-1:0]  config_addr,
    input [512-1:0] config_data,
    //input [512-1:0]	 vp_set, // [VAdr], [N, NII, Options, Nrep, Next, dx, dy, dz, du] ** full vector data set block **
    //input [16-1:0]	 reset_options, // option (fb hold/go, srcs... to pass when idle or in reset mode

    input        stall, // asserted by stream srcs when AXI DMA / FIFO is not ready to accept data -- should never happen, but prevents data loss/gap 
    input wire [32-1:0] S_AXIS_XV_tdata, // X-Value := SRCS[x_ch]
    input wire		    S_AXIS_XV_tvalid,
    
    output wire [32-1:0] M_AXIS_X_tdata, // vector components
    output wire		     M_AXIS_X_tvalid,
    output wire [32-1:0] M_AXIS_Y_tdata, // ..
    output wire		     M_AXIS_Y_tvalid,
    output wire [32-1:0] M_AXIS_Z_tdata, // ..
    output wire		     M_AXIS_Z_tvalid,
    output wire [32-1:0] M_AXIS_U_tdata, // ..
    output wire		     M_AXIS_U_tvalid,
    output wire [32-1:0] M_AXIS_A_tdata, // ..
    output wire		     M_AXIS_A_tvalid,
    output wire [32-1:0] M_AXIS_B_tdata, // ..
    output wire		     M_AXIS_B_tvalid,
    output wire [32-1:0] M_AXIS_AM_tdata, // ..
    output wire		     M_AXIS_AM_tvalid,
    output wire [32-1:0] M_AXIS_FM_tdata, // ..
    output wire		     M_AXIS_FM_tvalid,
    output wire	[32-1:0] M_AXIS_SRCS_tdata,
    output wire	         M_AXIS_SRCS_tvalid,
    output wire [32-1:0] options, // section options: FB, ... (flags) and source selections bits
    output wire [1:0 ] store_data, // trigger to store data:: 2: full vector header, 1: data sources
    output wire	 gvp_finished, // finished flag
    output wire	 gvp_hold, // on hold/pause
    output wire	[32-1:0] M_AXIS_index_tdata,
    output wire	         M_AXIS_index_tvalid,
    output wire	[48-1:0] M_AXIS_gvp_time_tdata,
    output wire	         M_AXIS_gvp_time_tvalid,
    output wire [32-1:0] dbg_status,
    output wire [4-1:0]  x_gvp_ch,
    output wire reset_state
    );

// Saturated result to 32bit
`define SATURATE_32(REG) (REG > 33'sd2147483647 ? 32'sd2147483647 : REG < -33'sd2147483647 ? -32'sd2147483647 : REG[32-1:0]) 


    reg [32-1:0]  config_addr_reg=0;
    reg [512-1:0] config_data_reg=0;

    // buffers
    reg pause=0;
    reg reset=1;
    reg reset_flg=1;  // put into reset mode (set program and hold)
    reg pause_flg=0;  // put/release into/from pause mode -- always completes the "ii" nop cycles!
    reg [4-1:0] setvec_mode=0; // program vector data using vp_set data
    reg [512-1:0] vp_set; // [VAdr], [N, NII, Options, Nrep, Next, dx, dy, dz, du] ** full vector data set block **
    reg [16-1:0] reset_options; // option (fb hold/go, srcs... to pass when idle or in reset mode
    
    //localparam integer NUM_VECTORS = 1 << NUM_VECTORS_N2;
    reg [32-1:0] decimation=0;
    reg [32-1:0] rdecii=0;
    
    // program controls
    reg [32-1:0] i=0;
    reg [32-1:0] ii=0;
    reg [32-1:0] sec=0;
    reg [4-1:0]  ik=0;
    reg xflag=0;
    reg load_next_vector=0;
    reg finished=0;
    
    reg [4-1:0]  x_rchi = 0;  // SRCS CHANNEL INDEX FOR COMPARE VECX OPERATIONS
    reg [32-1:0] X_value = 0; // SRCS CHANNEL X-Value
    reg X_GE = 0;
    reg X_LE = 0;
    
    // vector program memory/list
    // vpc control
    reg [32-1:0]  vec_i[NUM_VECTORS-1:0];
    reg [32-1:0]  vec_n[NUM_VECTORS-1:0];
    reg [32-1:0]  vec_iin[NUM_VECTORS-1:0];
    reg [32-1:0]  vec_options[NUM_VECTORS-1:0];
    reg [32-1:0]  vec_nrep[NUM_VECTORS-1:0];
    reg [32-1:0]  vec_deci[NUM_VECTORS-1:0];
    reg signed [NUM_VECTORS_N2:0] vec_next[NUM_VECTORS-1:0];

    // vector extension components -- only used/valid if X bit (bit15) set in OPTIONS
    reg [32-1:0]  vecX_opcd[NUM_VECTORS-1:0]; // VECX opcode
    reg [32-1:0]  vecX_cmpv[NUM_VECTORS-1:0]; // VECX value for X-operation
    reg [32-1:0]  vecX_rchi[NUM_VECTORS-1:0]; // VECX reference SRCS index to work with
    reg [32-1:0]  vecX_jmpn[NUM_VECTORS-1:0]; // VECX jump rel next
    // ...les

    // diff components
    reg signed [32-1:0]  vec_dx[NUM_VECTORS-1:0];
    reg signed [32-1:0]  vec_dy[NUM_VECTORS-1:0];
    reg signed [32-1:0]  vec_dz[NUM_VECTORS-1:0];
    reg signed [32-1:0]  vec_du[NUM_VECTORS-1:0];
    reg signed [32-1:0]  vec_da[NUM_VECTORS-1:0];
    reg signed [32-1:0]  vec_db[NUM_VECTORS-1:0];
    reg signed [32-1:0]  vec_dam[NUM_VECTORS-1:0];
    reg signed [32-1:0]  vec_dfm[NUM_VECTORS-1:0];

    reg signed [NUM_VECTORS_N2:0] pvc=0; // program counter. 0...NUM_VECTORS-1 "+ signuma bit"
  
    integer k;
    initial begin
        for (k=0; k<NUM_VECTORS; k=k+1)
        begin
            vec_i[k] = 0;
            vec_n[k] = 0;
            vec_iin[k] = 0;
            vec_options[k] = 0;
            vec_nrep[k] = 0;
            vec_deci[k] = 0;
            vec_next[k] = 0;
            vec_du[k] = 0;
            vec_dx[k] = 0;
            vec_dy[k] = 0;
            vec_dz[k] = 0;
            vec_dam[k] = 0;
            vec_dfm[k] = 0;
            vecX_opcd[k] = 0;
            vecX_cmpv[k] = 0;
            vecX_rchi[k] = 0;
            vecX_jmpn[k] = 0;
        end
    end
  
    // extra bits for vector components. 
    // ATTENTION: NO SATURATION WHILE VPC!
    // Saturation on outputs ONLY!
    
    localparam integer VEC_OV_BITS = 2;

    // data vector register
    reg signed [32+VEC_OV_BITS-1:0]  vec_x=0;
    reg signed [32+VEC_OV_BITS-1:0]  vec_y=0;
    reg signed [32+VEC_OV_BITS-1:0]  vec_z=0;
    reg signed [32+VEC_OV_BITS-1:0]  vec_u=0;
    reg signed [32+VEC_OV_BITS-1:0]  vec_a=0;
    reg signed [32+VEC_OV_BITS-1:0]  vec_b=0;
    reg signed [32+VEC_OV_BITS-1:0]  vec_am=0;
    reg signed [32+VEC_OV_BITS-1:0]  vec_fm=0;

    reg [32-1:0] set_options=0;
    reg [48-1:0] vec_gvp_time=0;
   
    // data store trigger
    reg [1:0] store = 0;       
    
    // for vector programming -- in reset mode to be safe -- but shoudl work also for live updates -- caution, check!
    reg [8:0] rd=511;

    assign x_gvp_ch = x_rchi;

    always @ (posedge a_clk) // 120MHz
    begin
    
        X_value <= S_AXIS_XV_tdata;
    
        if (reset_flg) // reset mode / hold
        begin
            vec_gvp_time <= 0;
        end
        else
        begin
            vec_gvp_time <= vec_gvp_time+1;
        end

        config_addr_reg <= config_addr;
        config_data_reg <= config_data;

        // module configuration
        case (config_addr_reg)
            control_reg_address:
            begin
                reset <= config_data_reg[0:0]; //reset;
                pause <= config_data_reg[1:1]; //pause;
            end

            reset_options_reg_address: 
            begin
                reset_options <= config_data_reg[15:0]; // GVP options assigned when in reset mode/finished
            end
            
            vector_reset_components_reg_address: // WARNING: THIS SETA ALL VEC COMPONETS TO ZERO INSTANTLY (particular used by MOVER to assure all 0 after abort/stop)
            begin                         // WARNING: DO NOT USE WHILE NORMAL SPM OPERATOPM
                if (config_data_reg[0:0])
                    vec_x <= 0;
                if (config_data_reg[1:1])
                    vec_y <= 0;
                if (config_data_reg[2:2])
                    vec_z <= 0;
                if (config_data_reg[3:3])
                    vec_u <= 0;
                if (config_data_reg[4:4])
                    vec_a <= 0;
                if (config_data_reg[5:5])
                    vec_b <= 0;
                if (config_data_reg[6:6])
                    vec_am <= 0;
                if (config_data_reg[7:7])
                    vec_fm <= 0;
            end

            vector_set_reg_address: // set GVP vector variable registers to presets
            begin
                // place holders, but do not... for xyz
                //vec_x <= config_data_reg[1*32-1 : 0*32]; // DO NEVER JUMP on XYZ!!
                //vec_y <= config_data_reg[2*32-1 : 1*32]; // DO NEVER JUMP on XYZ!!
                //vec_z <= config_data_reg[3*32-1 : 2*32]; // DO NEVER JUMP on XYZ!!
                vec_u <= config_data_reg[4*32-1 : 3*32]; // always reset U
                vec_a <= config_data_reg[5*32-1 : 4*32]; // always reset A
                vec_b <= config_data_reg[6*32-1 : 5*32]; // always reset B
                vec_am <= config_data_reg[7*32-1 : 6*32]; // always reset AM
                vec_fm <= config_data_reg[8*32-1 : 7*32]; // always reset FM
            end
            
            vector_programming_reg_address: // enter vector programming load mode
            begin
                case (setvec_mode)
                    0: // fetch and stage update
                    begin
                        vp_set <= config_data_reg; // [VAdr], [N, NII, Options, Nrep, Next, dx, dy, dz, du] ** full vector data set block **
                        setvec_mode <= 1; //setvec; // program vector data using vp_set data
                    end
                    1: // update vector[vp_set [NUM_VECTORS_N2:0]]
                    begin  
                        vec_n[vp_set [NUM_VECTORS_N2:0]]       <= vp_set [2*32-1:1*32];
                        vec_iin[vp_set [NUM_VECTORS_N2:0]]     <= vp_set [3*32-1:2*32];
                        vec_options[vp_set [NUM_VECTORS_N2:0]] <= vp_set [4*32-1:3*32];
                        
                        vec_nrep[vp_set [NUM_VECTORS_N2:0]]    <= vp_set [5*32-1:4*32];
                        vec_i[vp_set [NUM_VECTORS_N2:0]]       <= vp_set [5*32-1:4*32];
            
                        vec_deci[vp_set [NUM_VECTORS_N2:0]]    <= vp_set [16*32-1:15*32]; // all over process decimation adjust
                        
                        vec_next[vp_set [NUM_VECTORS_N2:0]]    <= vp_set [5*32+NUM_VECTORS_N2:5*32]; // limited to +/- NUM_VECTORS
                        vec_dx[vp_set [NUM_VECTORS_N2:0]]      <= vp_set [7*32-1:6*32];
                        vec_dy[vp_set [NUM_VECTORS_N2:0]]      <= vp_set [8*32-1:7*32];
                        vec_dz[vp_set [NUM_VECTORS_N2:0]]      <= vp_set [9*32-1:8*32];
                        vec_du[vp_set [NUM_VECTORS_N2:0]]      <= vp_set [10*32-1:9*32];
                        vec_da[vp_set [NUM_VECTORS_N2:0]]      <= vp_set [11*32-1:10*32];
                        vec_db[vp_set [NUM_VECTORS_N2:0]]      <= vp_set [12*32-1:11*32];
                        vec_dam[vp_set [NUM_VECTORS_N2:0]]     <= vp_set [13*32-1:12*32];
                        vec_dfm[vp_set [NUM_VECTORS_N2:0]]     <= vp_set [14*32-1:13*32];
                        vecX_opcd[vp_set [NUM_VECTORS_N2:0]]   <= 0; // VECX opcode *** RESET, MUST SET EXTENSION AFTER THIS!
                        vecX_cmpv[vp_set [NUM_VECTORS_N2:0]]   <= 0; // VECX value for X-operation
                        vecX_rchi[vp_set [NUM_VECTORS_N2:0]]   <= 0; // VECX reference SRCS index to work with
                        vecX_jmpn[vp_set [NUM_VECTORS_N2:0]]   <= 0; // VECX jump relative next
                        setvec_mode <= 2; // done, next load only possible after address=0 cycle (default below)
                    end
                endcase             
            end
            
            vectorX_programming_reg_address: // enter vector programming load mode: load vector extension: ONLY ACTIVE IF OPTIONS bit 15 is set
            begin
                case (setvec_mode)
                    0: // fetch and stage update
                    begin
                        vp_set <= config_data_reg; // [VAdr]
                        setvec_mode <= 1; //setvec; // program vector data using vp_set data
                    end
                    1: // update vector[vp_set [NUM_VECTORS_N2:0]]
                    begin  
                        vecX_opcd[vp_set [NUM_VECTORS_N2:0]]  <= vp_set [2*32-1:1*32]; // VECX opcode
                        vecX_cmpv[vp_set [NUM_VECTORS_N2:0]]  <= vp_set [3*32-1:2*32]; // VECX value for X-operation
                        vecX_rchi[vp_set [NUM_VECTORS_N2:0]]  <= vp_set [4*32-1:3*32]; // VECX reference SRCS index to work with
                        vecX_jmpn[vp_set [NUM_VECTORS_N2:0]]  <= vp_set [5*32-1:4*32]; // VECX jump relative next
                        setvec_mode <= 2; // done, next load only possible after address=0 cycle (default below)
                    end
                endcase             
            end


            default:
                setvec_mode <= 0;
        endcase

            
        rd[0] <= reset;  rd[1] <= rd[0]; rd[2] <= rd[1]; rd[3] <= rd[2]; rd[4] <= rd[3]; rd[5] <= rd[4];  rd[6] <= rd[5]; rd[7] <= rd[6]; rd[8] <= rd[7];
        reset_flg  <= rd[8];  // put into reset mode (set program and hold)
        pause_flg  <= pause || stall;  // put/release into/from pause mode -- always completes the "ii" nop cycles!
            
        if (rdecii == 0)
        begin
            rdecii <= decimation;

            // runs GVP code if out of reset mode until finished!
            if (reset_flg) // reset mode / hold
            begin
                pvc <= 0;
                sec <= 0;
                store <= 0;
                finished <= 0;
                load_next_vector <= 1;
                set_options <= reset_options;
            end
            else // run program step
            begin
                if (finished)
                begin
                    store <= 0; // hold finsihed state -- until reset
                    decimation <= 1; // reset to fast 
                    set_options <= reset_options;
                end            
                else
                begin
                    if (load_next_vector || finished) // load next vector
                    begin
                        load_next_vector <= 0;
                        i   <= vec_n[pvc];
                        ii  <= vec_iin[pvc];
                        ik  <= 2;
                        xflag <= 0;
                        x_rchi    <= vecX_rchi[pvc][3:0];
                        if (vec_n[pvc] == 0) // n == 0: END OF VECTOR PROGRAM REACHED
                        begin
                            finished <= 1;
                            store <= 3; // finshed -- store full header (push trigger) and GVP end mark
                            set_options <= 32'hffffffff;
                        end 
                        else
                        begin
                            store <= 2; // store full header (push trigger)
                            decimation <= vec_deci[pvc];
                            set_options <= vec_options[pvc];
                        end
                    end
                    else
                    if (!pause_flg)        
                    begin // go...
                        // add vector
                        if (!xflag)
                        begin
                            vec_x <= vec_x + vec_dx[pvc];
                            vec_y <= vec_y + vec_dy[pvc];
                            vec_z <= vec_z + vec_dz[pvc];
                            vec_u <= vec_u + vec_du[pvc];
                            vec_a <= vec_a + vec_da[pvc];
                            vec_b <= vec_b + vec_db[pvc];
                            vec_am <= vec_am + vec_dam[pvc];
                            vec_fm <= vec_fm + vec_dfm[pvc];
                        end                        
                        
                        if (ii) // do intermediate step(s) ?
                        begin
                            store <= 0;
                            
                            if (!xflag && !ik && vec_options[pvc][7]) // process Vector Extension: Optional Process Controls
                            begin
                                X_GE <= X_value > vecX_cmpv[pvc] ? 1:0;
                                X_LE <= X_value < vecX_cmpv[pvc] ? 1:0;
                                // X_value, vecX_opcd, vecX_cmpv
                                case (vecX_opcd[pvc][3:0]) 
                                    1: begin if (X_GE) begin xflag <= 1; i <= 1; ii <= 0; end else ii <= ii-1; end // next vector index, UNTIL X GE VAL, JUMP REL 
                                    2: begin if (X_LE) begin xflag <= 1; i <= 1; ii <= 0; end else ii <= ii-1; end // next vector index, UNTIL X LE VAL, JUMP REL
                                    5: begin                 xflag <= 1; i <= 1; ii <= 0; end // JUMP REL ALWAYS
                                    default: ii <= ii-1; 
                                endcase
                            end
                            else
                                ii <= ii-1;
                        end
                        else
                        begin // arrived at data point
                            if (i) // advance to next point...
                            begin
                                store <= 1; // store data sources (push trigger)
                                i  <= i-1;
                                ii <= vec_iin[pvc];
                                if (ik) ik <= ik-1;
                            end
                            else
                            begin // finished section, next vector -- if n != 0...
                                store <= 0; // full store data sources at next section start (push trigger)
                                sec <= sec + 1;
                                if (vec_i[pvc] > 0) // do next loop?
                                begin
                                    vec_i[pvc] <= vec_i[pvc] - 1;
                                    pvc <= pvc + vec_next[pvc]; // jump to loop head
                                    load_next_vector <= 1;
                                end
                                else // next vector in vector program list
                                begin
                                    vec_i[pvc] <= vec_nrep[pvc]; // reload loop counter for next time now!
                                    pvc <= pvc + 1 + vecX_jmpn[pvc]; // next vector index
                                    load_next_vector <= 1;
                                end
                            end            
                        end
                    end
                end
            end       
        end
        else
        begin
            rdecii <= rdecii-1;
        end
    end    
    
    assign M_AXIS_X_tdata = `SATURATE_32(vec_x);
    assign M_AXIS_X_tvalid = 1;
    assign M_AXIS_Y_tdata = `SATURATE_32(vec_y);
    assign M_AXIS_Y_tvalid = 1;
    assign M_AXIS_Z_tdata = `SATURATE_32(vec_z);
    assign M_AXIS_Z_tvalid = 1;
    assign M_AXIS_U_tdata = `SATURATE_32(vec_u);
    assign M_AXIS_U_tvalid = 1;
    assign M_AXIS_A_tdata = `SATURATE_32(vec_a);
    assign M_AXIS_A_tvalid = 1;
    assign M_AXIS_B_tdata = `SATURATE_32(vec_b);
    assign M_AXIS_B_tvalid = 1;
    assign M_AXIS_AM_tdata = `SATURATE_32(vec_am);
    assign M_AXIS_AM_tvalid = 1;
    assign M_AXIS_FM_tdata = `SATURATE_32(vec_fm);
    assign M_AXIS_FM_tvalid = 1;
    assign M_AXIS_SRCS_tdata = set_options;
    assign M_AXIS_SRCS_tvalid = 1;
    
    assign options = set_options;
       
    assign store_data = store;
    assign gvp_finished = finished;
    assign hold = pause_flg;
    assign M_AXIS_index_tdata = i;
    assign M_AXIS_index_tvalid = 1;
    assign M_AXIS_gvp_time_tdata = vec_gvp_time;
    assign M_AXIS_gvp_time_tvalid = 1;
    
    assign reset_state = reset;
    
    assign dbg_status = {sec[32-4:0], setvec_mode[0], reset_flg, pause, ~finished };
    
endmodule
