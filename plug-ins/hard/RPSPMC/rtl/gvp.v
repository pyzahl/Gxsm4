`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: GXSM
// Engineer: Percy Zahl
// 
// Create Date: 12/31/2022 08:50:50 PM
// Design Name: General Vector Program Execution Core
// Module Name: gvp
// Project Name: Open FPGA SPM Control 
// Target Devices: Zynq-7020
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


module gvp #(
    parameter NUM_VECTORS_N2 = 3,
    parameter NUM_VECTORS    = 8
)
(
    input a_clk,    // clocking up to aclk
    input reset,  // put into reset mode (set program and hold)
    input pause,  // put/release into/from pause mode -- always completes the "ii" nop cycles!
    input setvec, // program vector data using vp_set data
    input [512-1:0] vp_set, // [VAdr], [N, NII, Options, Nrep, Next, dx, dy, dz, du] ** full vector data set block **
    output [32-1:0] x, // vector components
    output [32-1:0] y, // ..
    output [32-1:0] z, // ..
    output [32-1:0] u, // ..
    output [32-1:0] options,  // section options: FB, ...
    output [32-1:0] section,  // section count
    output [1:0 ] store_data, // trigger to store data:: 2: full vector header, 1: data sources
    output gvp_finished       // finished flag
    );
    
    //localparam integer NUM_VECTORS = 1 << NUM_VECTORS_N2;
    reg [32-1:0] decimation=0;
    reg [32-1:0] rdecii=0;
    reg clk=0;
    
    // program controls
    reg [32-1:0] i=0;
    reg [32-1:0] ii=0;
    reg [32-1:0] sec=0;
    reg load_next_vector=0;
    reg finished=0;
    
    // vector program memory/list
    reg [32-1:0]  vec_i[NUM_VECTORS-1:0];
    reg [32-1:0]  vec_n[NUM_VECTORS-1:0];
    reg [32-1:0]  vec_iin[NUM_VECTORS-1:0];
    reg [32-1:0]  vec_options[NUM_VECTORS-1:0];
    reg [32-1:0]  vec_nrep[NUM_VECTORS-1:0];
    reg [32-1:0]  vec_deci[NUM_VECTORS-1:0];
    reg signed [8-1:0]   vec_next[NUM_VECTORS-1:0];

    reg signed [32-1:0]  vec_dx[NUM_VECTORS-1:0];
    reg signed [32-1:0]  vec_dy[NUM_VECTORS-1:0];
    reg signed [32-1:0]  vec_dz[NUM_VECTORS-1:0];
    reg signed [32-1:0]  vec_du[NUM_VECTORS-1:0];
    
    reg [NUM_VECTORS_N2-1:0] pvc=0; // program counter. 0...NUM_VECTORS-1

    // data vector register
    reg signed [32-1:0]  vec_x=0;
    reg signed [32-1:0]  vec_y=0;
    reg signed [32-1:0]  vec_z=0;
    reg signed [32-1:0]  vec_u=0;

    // data store trigger
    reg [1:0] store = 0;
    
    // for vector programming -- in reset mode to be safe -- but shoudl work also for live updates -- caution, check!
/*
    always @(posedge setvec)
    begin
        vec_n[vp_set [NUM_VECTORS_N2:0]]       <= vp_set [2*32-1:1*32];
        vec_iin[vp_set [NUM_VECTORS_N2:0]]     <= vp_set [3*32-1:2*32];
        vec_options[vp_set [NUM_VECTORS_N2:0]] <= vp_set [4*32-1:3*32];
        
        vec_nrep[vp_set [NUM_VECTORS_N2:0]]    <= vp_set [5*32-1:4*32];
        vec_i[vp_set [NUM_VECTORS_N2:0]]       <= vp_set [5*32-1:4*32];
        
        vec_next[vp_set [NUM_VECTORS_N2:0]]    <= vp_set [6*32-1:5*32];
        vec_dx[vp_set [NUM_VECTORS_N2:0]]      <= vp_set [7*32-1:6*32];
        vec_dy[vp_set [NUM_VECTORS_N2:0]]      <= vp_set [8*32-1:7*32];
        vec_dz[vp_set [NUM_VECTORS_N2:0]]      <= vp_set [9*32-1:8*32];
        vec_du[vp_set [NUM_VECTORS_N2:0]]      <= vp_set [10*32-1:9*32];
    end
*/

    always @ (posedge a_clk) // 120MHz
    begin
        if (rdecii == 0)
        begin
            rdecii <= decimation;
            clk <= !clk;
        end else begin
            rdecii <= rdecii-1;
        end
    end


    // runs GVP code if out of reset mode until finished!
    always @(posedge clk) // run on decimated clk as required
    begin
        if (setvec)
        begin
            vec_n[vp_set [NUM_VECTORS_N2:0]]       <= vp_set [2*32-1:1*32];
            vec_iin[vp_set [NUM_VECTORS_N2:0]]     <= vp_set [3*32-1:2*32];
            vec_options[vp_set [NUM_VECTORS_N2:0]] <= vp_set [4*32-1:3*32];
            
            vec_nrep[vp_set [NUM_VECTORS_N2:0]]    <= vp_set [5*32-1:4*32];
            vec_i[vp_set [NUM_VECTORS_N2:0]]       <= vp_set [5*32-1:4*32];

            vec_deci[vp_set [NUM_VECTORS_N2:0]]    <= vp_set [16*32-1:15*32]; // all over process decimation adjust
            
            vec_next[vp_set [NUM_VECTORS_N2:0]]    <= vp_set [6*32-1:5*32];
            vec_dx[vp_set [NUM_VECTORS_N2:0]]      <= vp_set [7*32-1:6*32];
            vec_dy[vp_set [NUM_VECTORS_N2:0]]      <= vp_set [8*32-1:7*32];
            vec_dz[vp_set [NUM_VECTORS_N2:0]]      <= vp_set [9*32-1:8*32];
            vec_du[vp_set [NUM_VECTORS_N2:0]]      <= vp_set [10*32-1:9*32];
        end
        else
        begin
            if (reset) // reset mode / hold
            begin
                pvc <= 0;
                sec <= 0;
                store <= 0;
                finished <= 0;
                load_next_vector <= 1;
            end
            else // run program step
            begin
                if (load_next_vector || finished) // load next vector / hold if finished
                begin
                    store <= 2; // store full header (push trigger)
                    load_next_vector <= 0;
                    i   <= vec_n[pvc];
                    ii  <= vec_iin[pvc];
                    decimation <= vec_deci[pvc];
                    if (vec_n[pvc] == 0) // n == 0: END OF VECTOR PROGRAM REACHED
                    begin
                        decimation <= 1; // reset to fast 
                        finished <= 1;
                    end
                end
                else
                begin // go...
                    // add vector
                    vec_x <= vec_x + vec_dx[pvc];
                    vec_y <= vec_y + vec_dy[pvc];
                    vec_z <= vec_z + vec_dz[pvc];
                    vec_u <= vec_u + vec_du[pvc];
                    
                    if (ii) // do intermediate step(s) ?
                    begin
                        store <= 0;
                        ii <= ii-1;
                    end
                    else
                    if (!pause)        
                    begin // arrived at data point
                        store <= 1; // store data sources (push trigger)
                        if (i) // advance to next point...
                        begin
                            ii <= vec_iin[pvc];
                            i <= i-1;
                        end
                        else
                        begin // finsihed section, next vector -- if n != 0...
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
                                pvc <= pvc + 1; // next vector index
                                load_next_vector <= 1;
                            end
                        end            
                    end
                end
            end
        end     
    end
    
    assign x = vec_x;
    assign y = vec_y;
    assign z = vec_z;
    assign u = vec_u;
    
    assign section = sec;
    assign store_data = store;
    assign gvp_finished = finished;
    
endmodule
