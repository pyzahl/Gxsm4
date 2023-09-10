`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 08/15/2023 06:52:29 PM
// Design Name: 
// Module Name: ad5791_tb
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



module ad5791_tb;
    reg tb_ACLK;
    reg tb_ARESETn;
   
    wire temp_clk;
    wire temp_rstn; 
   
    reg [31:0] read_data;
    wire [3:0] leds;
    reg resp;
    
    reg pclk=0;
    reg r=1;
    reg prg=0;
    reg [512-1:0] data; // [VAdr], [N, NII, Nrep, Options, Next, dx, dy, dz, du] ** full vector data set block **

    wire [31:0] wx; // vector components
    wire [31:0] wy; // ..
    wire [31:0] wz; // ..
    wire [31:0] wu; // ..
    wire [31:0] wopt;  // section options: FB, ...
    wire [31:0] wsec;  // section count
    wire [1:0] sto; // trigger to store data:: 2: full vector header, 1: data sources
    wire fin;  // finished 
    
    
    initial 
    begin       
        tb_ACLK = 1'b0;
    end
    
    //------------------------------------------------------------------------
    // Simple Clock Generator
    //------------------------------------------------------------------------
    
    always #10 tb_ACLK = !tb_ACLK;
       
    always begin
        pclk = 1; #5;
        pclk = 0; #5;
    end

    initial
    begin
    
        $display ("running the tb");

        r=1;
        #20

        prg=0;
        #20
        // move to start point
        //                  du        dz        dy        dx     Next       Nrep,   Options,     nii,      N,    [Vadr]
        data = {192'd0, 32'd0001, 32'd0000, -32'd0002, -32'd0002,  32'd0, 32'd0000,   32'h001, 32'd002, 32'd005, 32'd00 };
        #2
        prg=1;
        #20
        prg=0;
        #20

        data = {192'd0, -32'd0001, 32'd0000, 32'd0002, 32'd0002,  32'd0, 32'd0000,   32'h001, 32'd002, 32'd005, 32'd01 };
        #2
        prg=1;
        #20
        prg=0;
        #20

        data = {192'd0, 32'd0004, 32'd0003, 32'd0002, 32'd0001,  32'd0, 32'd0000,   32'h000, 32'd000, 32'd000, 32'd02 }; // END
        #2
        prg=1;
        #20
        prg=0;
        #20

        r=0; // release reset to run
        #20

        wait (fin);

        r=1; // put into reset/hold
        #20

        prg=0;
        #20

        // scan procedure
        //                  du        dz        dy        dx     Next       Nrep,   Options,     nii,      N,    [Vadr]
        data = {192'd0, 32'd0000, 32'd0000, 32'd0000, 32'd0002,  32'd0, 32'd0000,   32'h001, 32'd002, 32'd010, 32'd00 };
        #2
        prg=1;
        #20
        prg=0;
        #20
        
        data = {192'd0, 32'd0000, 32'd0000, 32'd0000, -32'sd0002,  32'd0, 32'd0000,   32'h001, 32'd002, 32'd010, 32'd01 };
        #2
        prg=1;
        #20
        prg=0;
        #20

        data = {192'd0, 32'd0000, 32'd0000, 32'd0002, 32'd0000,  -32'sd2, 32'd0010,   32'h001, 32'd002, 32'd001, 32'd02 };
        #2
        prg=1;
        #20
        prg=0;
        #20

        data = {192'd0, 32'd0004, 32'd0003, 32'd0002, 32'd0001,  32'd0, 32'd0000,   32'h000, 32'd000, 32'd000, 32'd03 }; // END
        #2
        prg=1;
        #20
        prg=0;
        #20

        r=0; // release reset to run
        wait (fin);

        #20
        r=1; // reset to hold

        $display ("Simulation 2 completed");
        $stop;
    end


    assign temp_clk = tb_ACLK;
    assign temp_rstn = tb_ARESETn;




gvp gvp1
    (
        .a_clk(pclk),    // clocking up to aclk
        .reset(r),  // put into reset mode (hold)
        .pause(0),
        .setvec(prg), // program vector data using vp_set data
        .vp_set(data), // [VAdr], [N, NII, Nrep, Options, Next, dx, dy, dz, du] ** full vector data set block **
        .reset_options(0),
        .x(wx), // vector components
        .y(wy), // ..
        .z(wz), // ..
        .u(wu), // ..
        .options(wopt),  // section options: FB, ...
        .section(wsec),  // section count
        .store_data(sto), // trigger to store data:: 2: full vector header, 1: data sources
        .gvp_finished(fin)      // finished 
);


endmodule
