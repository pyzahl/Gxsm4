`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 11/26/2017 12:54:20 AM
// Design Name: 
// Module Name: PulseForm
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


module PulseForm #(
    parameter pulse_form_config_address = 20010,
    parameter pulse_form_wh_arr_address = 20011,
    parameter M_AXIS_DATA_WIDTH = 16,
    parameter ENABLE_ADC_OUT    = 1
)
(
    input [32-1:0]  config_addr,
    input [512-1:0] config_data,
   (* X_INTERFACE_PARAMETER = "ASSOCIATED_CLKEN a_clk" *)
   (* X_INTERFACE_PARAMETER = "ASSOCIATED_BUSIF M_AXIS" *)
   input a_clk,
   input single_shot,
   input run,
   input wire [2:0] zero_spcp,

   //input wire [7:0]                     pre_shr,
   output [M_AXIS_DATA_WIDTH-1:0]      M_AXIS_tdata,
   output                              M_AXIS_tvalid
);


    reg [31:0] p12_delay;
    reg [15:0] p12_wh_arr[0:27];

    reg [1:0] start=0;
    reg [1:0] last=1;
    reg signed [M_AXIS_DATA_WIDTH-1:0] pval_init0=0;
    reg signed [M_AXIS_DATA_WIDTH-1:0] pval_init1=0;
    reg signed [M_AXIS_DATA_WIDTH-1:0] pval=0;
    reg [15:0] nd0=0;
    reg [15:0] nd1=0;
    reg [15:0] nd0i=0;
    reg [15:0] nd1i=0;
    reg [15:0] nw0=0;
    reg [15:0] nw1=0;
    reg [5:0] arri0 = 10;
    reg [5:0] arri1 = 10;
    reg finished0 = 1;
    reg finished1 = 1;

    reg [1:0] rdecii = 0;

    reg init_shot     = 0; 
    reg [4:0] count   = 0;
    reg i_single_shot = 0;
    reg i_zero_spcp2  = 0; 

/*
    always @ (posedge a_clk)
    begin
        rdecii <= rdecii+1;
    end
*/


    always @ (posedge a_clk)
    begin
        // module configuration
        case (config_addr)
            pulse_form_wh_arr_address:
            begin
                {  p12_wh_arr[0],  p12_wh_arr[1] } <= config_data[ 1*32-1 : 0];    // 0W 1,2 =pWIF
                {  p12_wh_arr[2],  p12_wh_arr[3] } <= config_data[ 2*32-1 : 1*32]; // 0H 1,2 =pIHF
                {  p12_wh_arr[4],  p12_wh_arr[5] } <= config_data[ 3*32-1 : 2*32]; // 1W =pWIF
                {  p12_wh_arr[6],  p12_wh_arr[7] } <= config_data[ 4*32-1 : 3*32]; // 1H =pWIF
                {  p12_wh_arr[8],  p12_wh_arr[9] } <= config_data[ 5*32-1 : 4*32]; // 2W =WIF
                { p12_wh_arr[10], p12_wh_arr[11] } <= config_data[ 6*32-1 : 5*32]; // 2H =HIF
                { p12_wh_arr[12], p12_wh_arr[13] } <= config_data[ 7*32-1 : 6*32]; // 3W =pW
                { p12_wh_arr[14], p12_wh_arr[15] } <= config_data[ 8*32-1 : 7*32]; // 3H =pH
                { p12_wh_arr[16], p12_wh_arr[17] } <= config_data[ 9*32-1 : 8*32]; // 4W =pWIF
                { p12_wh_arr[18], p12_wh_arr[19] } <= config_data[10*32-1 : 9*32]; // 4H =pHIF
                { p12_wh_arr[20], p12_wh_arr[21] } <= config_data[11*32-1 :10*32]; // 5W =WIF
                { p12_wh_arr[22], p12_wh_arr[23] } <= config_data[12*32-1 :11*32]; // 5H =HIF
                { p12_wh_arr[24], p12_wh_arr[25] } <= config_data[13*32-1 :12*32]; // Bias pre
                { p12_wh_arr[26], p12_wh_arr[27] } <= config_data[14*32-1 :13*32]; // Bias post
            end   
            pulse_form_config_address:
            begin
                p12_delay <= config_data[1*32-1 : 0*32];
            end   
        endcase
    end

/*
    always @ (*) // Latch
    begin
        p12_delay <= pulse_12_delay;
        {  p12_wh_arr[0],  p12_wh_arr[1] } <= pulse_12_width_height_array[ 1*32-1 : 0];    // 0W 1,2 =pWIF
        {  p12_wh_arr[2],  p12_wh_arr[3] } <= pulse_12_width_height_array[ 2*32-1 : 1*32]; // 0H 1,2 =pIHF
        {  p12_wh_arr[4],  p12_wh_arr[5] } <= pulse_12_width_height_array[ 3*32-1 : 2*32]; // 1W =pWIF
        {  p12_wh_arr[6],  p12_wh_arr[7] } <= pulse_12_width_height_array[ 4*32-1 : 3*32]; // 1H =pWIF
        {  p12_wh_arr[8],  p12_wh_arr[9] } <= pulse_12_width_height_array[ 5*32-1 : 4*32]; // 2W =WIF
        { p12_wh_arr[10], p12_wh_arr[11] } <= pulse_12_width_height_array[ 6*32-1 : 5*32]; // 2H =HIF
        { p12_wh_arr[12], p12_wh_arr[13] } <= pulse_12_width_height_array[ 7*32-1 : 6*32]; // 3W =pW
        { p12_wh_arr[14], p12_wh_arr[15] } <= pulse_12_width_height_array[ 8*32-1 : 7*32]; // 3H =pH
        { p12_wh_arr[16], p12_wh_arr[17] } <= pulse_12_width_height_array[ 9*32-1 : 8*32]; // 4W =pWIF
        { p12_wh_arr[18], p12_wh_arr[19] } <= pulse_12_width_height_array[10*32-1 : 9*32]; // 4H =pHIF
        { p12_wh_arr[20], p12_wh_arr[21] } <= pulse_12_width_height_array[11*32-1 :10*32]; // 5W =WIF
        { p12_wh_arr[22], p12_wh_arr[23] } <= pulse_12_width_height_array[12*32-1 :11*32]; // 5H =HIF
        { p12_wh_arr[24], p12_wh_arr[25] } <= pulse_12_width_height_array[13*32-1 :12*32]; // Bias pre
        { p12_wh_arr[26], p12_wh_arr[27] } <= pulse_12_width_height_array[14*32-1 :13*32]; // Bias post
    end
*/
    //always @ (posedge rdecii[1])
    always @ (posedge a_clk)
    begin
        rdecii <= rdecii+1; // rdecii 00 01 *10 11 00 ...

        if (run || count > 0)
        begin
            if (zero_spcp[2] != i_zero_spcp2)
            begin
                i_zero_spcp2 <= zero_spcp[2];
                case (zero_spcp[1])
                    1: // c pos start
                    begin // P1
                        start <= 1;
                    end
                    0: // c neg start
                    begin
                        start <= 2;
                    end
                endcase  
            end              
        end

        if (single_shot != i_single_shot)
        begin
            i_single_shot <= single_shot;
            if (single_shot)
            begin
                count <= 2; // initiale single shot
                last  <= 1;
            end
        end
        

        if (rdecii == 1)
        begin
           if (ENABLE_ADC_OUT)
           begin
                if (start == 1 && last==2) // Pulse 0 Sin X Neg Start
                begin
                    nd0        <= p12_delay[31:16]; // Delay from zero x trigger
                    nw0        <= p12_wh_arr[0]; // width
                    pval_init0 <= p12_wh_arr[2]; // height
                    arri0 <= 4;
                    last  <= 1;
                    if (count > 0)
                        count <= count - 1;
                end
    
                if (start == 2 && last==1) // Pulse 1 Sin X Pos Start
                begin
                    nd1        <= p12_delay[15:0]; // Delay from zero x trigger
                    nw1        <= p12_wh_arr[1]; // width
                    pval_init1 <= p12_wh_arr[3]; // height
                    arri1 <= 5;
                    last  <= 2;
                    if (count > 0)
                        count <= count - 1;
                end
                
                // Pulse 0
                if (nd0 > 0)
                begin // run delay
                    if (finished1)
                    begin
                        pval <= p12_wh_arr[24];
                    end
                    nd0  <= nd0-1;
                end
                else begin
                    if (nw0 > 0) // set pulse signal to value
                    begin
                        finished0 <= 0;
                        pval <= pval_init0;
                        nw0  <= nw0-1;
                    end
                    else begin  // finished, reset all, pulse signal=0
                        if (arri0 < 22)
                        begin
                            nw0        <= p12_wh_arr[arri0];
                            pval_init0 <= p12_wh_arr[arri0+2];
                            arri0 <= arri0 + 4;
                        end
                        else begin  // finished, reset all, pulse signal=0
                            finished0 <= 1;
                            if (finished1)
                            begin
                                pval  <= p12_wh_arr[26];
                            end
                        end
                    end
                end
                // Pulse 1
                if (nd1 > 0)
                begin // run delay
                    if (finished0)
                    begin
                        pval <= p12_wh_arr[25];
                    end
                    nd1  <= nd1-1;
                end
                else begin
                    if (nw1 > 0) // set pulse signal to value
                    begin
                        finished1 <= 0;
                        pval <= pval_init1;
                        nw1  <= nw1-1;
                    end
                    else begin  // finished, reset all, pulse signal=0
                        if (arri1 < 22)
                        begin
                            nw1        <= p12_wh_arr[arri1];
                            pval_init1 <= p12_wh_arr[arri1+2];
                            arri1 <= arri1 + 4;
                        end
                        else begin  // finished, reset all, pulse signal=0
                            finished1 <= 1;
                            if (finished0)
                            begin
                                pval  <= p12_wh_arr[27];
                            end
                        end
                    end
                end
            end
        end
    end
        
    assign M_AXIS_tdata  = pval;
    assign M_AXIS_tvalid = 1;
   
endmodule
