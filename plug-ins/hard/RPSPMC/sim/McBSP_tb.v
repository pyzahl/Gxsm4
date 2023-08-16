`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 06/13/2018 07:59:31 PM
// Design Name: 
// Module Name: lms_phase_amplitude_detector_tb
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

 
module McBSP_tb();
    parameter WORDS  = 8;
    parameter WWIDTH = 32;
    parameter PADDING = 10;

    parameter WORDS_PER_FRAME = 8;
    parameter BITS_PER_WORD = 32;
    parameter SAXIS_TDATA_WIDTH = 32;

    
    wire [8-1:0] exp_p_io; // inout
    wire [8-1:0] exp_n_io; // inout
    //reg  McBSP_clkr; // in  CLKR: clock return
    //reg  McBSP_tx;   // in  TX: data transmit
    //reg  McBSP_fsx;  // in  optional, debug: data frame start FSX
    //reg  McBSP_frm;  // in  optional, debug: data frame
    wire w_McBSP_clk;  // out  McBSP clock
    wire w_McBSP_fs;   // out  McBSP FS (frame start)
    wire w_McBSP_rx;   // out  McBSP RX (data receive)
    wire w_McBSP_nrx; // out  optional: Nth/other RedPitaya slaves data TX forwarded on scheme TDB
    wire [8-1:0] w_RP_exp_in; // out

    reg clock;
    reg frame;
    reg framew;
    reg data_bit;
    reg [WORDS*WWIDTH-1:0] data; 
    reg [31:0] cnt; 
    reg [15:0] bc;

    //reg r_McBSP_clk;
    //reg r_McBSP_fs;
    //reg r_McBSP_rx;
    //reg [8-1:0] r_RP_exp_in; // out

    wire w_McBSP_clkr;
    wire w_McBSP_tx;   // out  McBSP FS (frame start)
    wire w_McBSP_fsx;   // out  McBSP RX (data receive)
    wire w_McBSP_frm;   // out  McBSP RX (data receive)


    wire trigger;
    wire [(WORDS_PER_FRAME * BITS_PER_WORD) -1 : 0] dataset_read;

    McBSP_io_connect McBSP_io_Connect(
    .exp_p_io(exp_p_io),
    .exp_n_io(exp_n_io),
    .McBSP_clkr(w_McBSP_clkr), // CLKR: clock return (in)
    .McBSP_tx(w_McBSP_tx),   // TX: data transmit (in)
    .McBSP_fsx(w_McBSP_fsx),  // optional, debug: data frame start FSX
    .McBSP_frm(w_McBSP_frm),  // optional, debug: data frame
    .McBSP_clk(w_McBSP_clk),  // McBSP clock (out)
    .McBSP_fs(w_McBSP_fs),   // McBSP FS (frame start)
    .McBSP_rx(w_McBSP_rx),   // McBSP RX (data receive)
    .McBSP_nrx(w_McBSP_nrx), // optional: Nth/other RedPitaya slaves data TX forwarded on scheme TDB
    .RP_exp_in(w_RP_exp_in)
    );
        
    
    McBSP_controller McBSP_Controller(
    .a_clk(clock),
    .mcbsp_clk(w_McBSP_clk),
    .mcbsp_frame_start(w_McBSP_fs),
    .mcbsp_data_rx(w_McBSP_rx),
    .mcbsp_data_nrx(w_McBSP_rx),   // N-th RedPitaya Slave/Chained Link forwarding to McBSP Maser Slave -- optional
    .mcbsp_data_clkr(w_McBSP_clkr), // clock return
    .mcbsp_data_tx(w_McBSP_tx),
    .mcbsp_data_fsx(w_McBSP_fsx), // optional/debug FSX start
    .mcbsp_data_frm(w_McBSP_frm), // optional/debug Frame
    // input a_resetn,
    
    .trigger(w_trigger),
    .dataset_read(w_dataset_read),
    
    .S_AXIS1_tdata(13),
    .S_AXIS1_tvalid(1),
    .S_AXIS2_tdata(13+32),
    .S_AXIS2_tvalid(1),
    .S_AXIS3_tdata(13+64),
    .S_AXIS3_tvalid(1),
    .S_AXIS4_tdata(13+96),
    .S_AXIS4_tvalid(1),
    .S_AXIS5_tdata(13+128),
    .S_AXIS5_tvalid(1),
    .S_AXIS6_tdata(13+256),
    .S_AXIS6_tvalid(1),
    .S_AXIS7_tdata(13+512),
    .S_AXIS7_tvalid(1),
    .S_AXIS8_tdata(13+1024),
    .S_AXIS8_tvalid(1)
    );
            
               
    assign exp_p_io[0:0] = clock;
    assign exp_p_io[1:1] = frame;
    assign exp_p_io[2:2] = data_bit;
    // exp_p_io[1:1] // FS in
    // exp_p_io[2:2] // RX in
    // exp_p_io[3:3] // TX out
    // exp_p_io[4:4] // FSX
    // exp_p_io[5:5] // FRM
    // exp_p_io[6:6] // CLKR
    // exp_p_io[7:7] // NRX



           
    initial begin
        clock = 0;
        frame = 0;
        framew = 0;
        cnt = 12;
        bc = 0;
        data = 15-2;
                         
        forever #1 begin
            cnt = cnt-1;
            if (cnt == 0) begin
                clock = ~clock;
                cnt =  12;
                if (exp_p_io[0:0]) begin
                    if (bc == 0) begin
                        frame = 1;
                        framew = 1;
                    end else begin    
                        frame = 0;
                    end
                    if (bc < WORDS*WWIDTH)
                        data_bit = data[bc];
                    else
                        framew = 0;
                    bc = bc+1;
                    if (bc == WORDS*WWIDTH+PADDING)
                        bc = 0;
                end
            end
        end
    end

endmodule
