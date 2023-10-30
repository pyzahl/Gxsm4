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
    parameter NUM_VECTORS_N2 = 4,
    parameter NUM_VECTORS    = 16
)
(
    (* X_INTERFACE_PARAMETER = "ASSOCIATED_CLKEN a_clk, ASSOCIATED_BUSIF M_AXIS_X:M_AXIS_Y:M_AXIS_Z:M_AXIS_U:M_AXIS_SRCS:M_AXIS_INDEX:M_AXIS_GVP_TIME" *)
    input		 a_clk, // clocking up to aclk
    input		 reset, // put into reset mode (set program and hold)
    input		 pause, // put/release into/from pause mode -- always completes the "ii" nop cycles!
    input		 setvec, // program vector data using vp_set data
    input [512-1:0]	 vp_set, // [VAdr], [N, NII, Options, Nrep, Next, dx, dy, dz, du] ** full vector data set block **
    input [16-1:0]	 reset_options, // option (fb hold/go, srcs... to pass when idle or in reset mode

    input        stall, // asserted by stream srcs when AXI DMA / FIFO is not ready to accept data -- should never happen, but prevents data loss/gap 
    
    output wire [32-1:0] M_AXIS_X_tdata, // vector components
    output wire		     M_AXIS_X_tvalid,
    output wire [32-1:0] M_AXIS_Y_tdata, // ..
    output wire		     M_AXIS_Y_tvalid,
    output wire [32-1:0] M_AXIS_Z_tdata, // ..
    output wire		     M_AXIS_Z_tvalid,
    output wire [32-1:0] M_AXIS_U_tdata, // ..
    output wire		     M_AXIS_U_tvalid,
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
    output wire [32-1:0] dbg_status
    );

    // buffers
    reg reset_flg=1;  // put into reset mode (set program and hold)
    reg pause_flg=0;  // put/release into/from pause mode -- always completes the "ii" nop cycles!
    reg setvec_flg=0; // program vector data using vp_set data
    reg [512-1:0] vp_set_data; // [VAdr], [N, NII, Options, Nrep, Next, dx, dy, dz, du] ** full vector data set block **
    
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
    reg signed [NUM_VECTORS_N2:0] vec_next[NUM_VECTORS-1:0];

    reg signed [32-1:0]  vec_dx[NUM_VECTORS-1:0];
    reg signed [32-1:0]  vec_dy[NUM_VECTORS-1:0];
    reg signed [32-1:0]  vec_dz[NUM_VECTORS-1:0];
    reg signed [32-1:0]  vec_du[NUM_VECTORS-1:0];
    
    reg signed [NUM_VECTORS_N2:0] pvc=0; // program counter. 0...NUM_VECTORS-1 "+ signuma bit"

    // data vector register
    reg signed [32-1:0]  vec_x=0;
    reg signed [32-1:0]  vec_y=0;
    reg signed [32-1:0]  vec_z=0;
    reg signed [32-1:0]  vec_u=0;

    reg [32-1:0] set_options=0;
    reg [48-1:0] vec_gvp_time=0;
   
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
        if (reset_flg) // reset mode / hold
            vec_gvp_time <= 0;
        else
            vec_gvp_time <= vec_gvp_time+1;

        reset_flg  <= reset;  // put into reset mode (set program and hold)
        pause_flg  <= pause || stall;  // put/release into/from pause mode -- always completes the "ii" nop cycles!
        setvec_flg <= setvec; // program vector data using vp_set data
        if (rdecii == 0)
        begin
            rdecii <= decimation;
            clk <= !clk;
            if (setvec_flg)
            begin
                vp_set_data <= vp_set; // buffer
            end
        end else begin
            rdecii <= rdecii-1;
        end
    end


    // runs GVP code if out of reset mode until finished!
    always @(posedge clk) // run on decimated clk as required
    begin
        if (setvec_flg)
        begin
            vec_n[vp_set [NUM_VECTORS_N2:0]]       <= vp_set_data [2*32-1:1*32];
            vec_iin[vp_set [NUM_VECTORS_N2:0]]     <= vp_set_data [3*32-1:2*32];
            vec_options[vp_set [NUM_VECTORS_N2:0]] <= vp_set_data [4*32-1:3*32];
            
            vec_nrep[vp_set [NUM_VECTORS_N2:0]]    <= vp_set_data [5*32-1:4*32];
            vec_i[vp_set [NUM_VECTORS_N2:0]]       <= vp_set_data [5*32-1:4*32];

            vec_deci[vp_set [NUM_VECTORS_N2:0]]    <= vp_set_data [16*32-1:15*32]; // all over process decimation adjust
            
            vec_next[vp_set [NUM_VECTORS_N2:0]]    <= vp_set_data [5*32+NUM_VECTORS_N2:5*32]; // limited to +/- NUM_VECTORS
            vec_dx[vp_set [NUM_VECTORS_N2:0]]      <= vp_set_data [7*32-1:6*32];
            vec_dy[vp_set [NUM_VECTORS_N2:0]]      <= vp_set_data [8*32-1:7*32];
            vec_dz[vp_set [NUM_VECTORS_N2:0]]      <= vp_set_data [9*32-1:8*32];
            vec_du[vp_set [NUM_VECTORS_N2:0]]      <= vp_set_data [10*32-1:9*32];
        end
        else
        begin
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
                        begin // arrived at data point
                            if (i) // advance to next point...
                            begin
                                store <= 1; // store data sources (push trigger)
                                ii <= vec_iin[pvc];
                                i <= i-1;
                            end
                            else
                            begin // finsihed section, next vector -- if n != 0...
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
                                    pvc <= pvc + 1; // next vector index
                                    load_next_vector <= 1;
                                end
                            end            
                        end
                    end
                end
            end       
        end     
    end
    
    assign M_AXIS_X_tdata = vec_x;
    assign M_AXIS_X_tvalid = 1;
    assign M_AXIS_Y_tdata = vec_y;
    assign M_AXIS_Y_tvalid = 1;
    assign M_AXIS_Z_tdata = vec_z;
    assign M_AXIS_Z_tvalid = 1;
    assign M_AXIS_U_tdata = vec_u;
    assign M_AXIS_U_tvalid = 1;
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
    
    assign dbg_status = {sec[32-3:0], reset, pause, ~finished };
    
endmodule
