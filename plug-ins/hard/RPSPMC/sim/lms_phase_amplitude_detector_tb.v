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


module lms_phase_amplitude_detector_tb();
    parameter S_AXIS_SIGNAL_TDATA_WIDTH = 32;
    parameter S_AXIS_SIGNAL_DATA_WIDTH = 16;
    parameter S_AXIS_SIGNAL_SIGNIFICANT_DATA_WIDTH = 14;
    parameter S_AXIS_SC_TDATA_WIDTH = 64;
    parameter M_AXIS_SC_TDATA_WIDTH = 64;
    parameter SC_DATA_WIDTH = 26;
    parameter LMS_DATA_WIDTH = 26;
    parameter LMS_Q_WIDTH = 22; // do not change
    parameter M_AXIS_XY_TDATA_WIDTH = 64;
    parameter M_AXIS_AM_TDATA_WIDTH = 48;
    parameter MDC_TIME_CONST_N = 20; // x' = [x*(1<<(N-1))+X]>>N
    parameter MDC_DATA_WIDTH = S_AXIS_SIGNAL_SIGNIFICANT_DATA_WIDTH+MDC_TIME_CONST_N+2;
    parameter M_AXIS_MDC_TDATA_WIDTH = 32;
    
    reg clock;
    reg [S_AXIS_SIGNAL_TDATA_WIDTH-1:0]  td_signal;
    reg                                  valid;
    reg [S_AXIS_SIGNAL_DATA_WIDTH-1:0]   signal;
    reg [S_AXIS_SC_TDATA_WIDTH-1:0]  td_sc;
    reg [SC_DATA_WIDTH-1:0]  s;
    reg [SC_DATA_WIDTH-1:0]  c;
    reg signed [31:0] tau; // Q22 tau phase
    reg signed [31:0] Atau; // Q22 tau amplitude
    reg signed [31:0] dc_tau; // Q22 tai DC iir at cs zero x
    reg signed [31:0] dc; // Q22

    reg signed [31:0] t;
    reg signed [24:0] td_ph;

    // SinCosSDB64
    parameter AXIS_TDATA_WIDTH = 64;
    parameter signed CR_INIT = 32'h7FFFFFFF;  // ((1<<31)-1)
    parameter signed CI_INIT = 32'h00000000;
    parameter signed ERROR_E_INIT = 64'h3FFFFFFFFFFFFFFF; //4611686018427387903
    reg signed [31:0] deltasRe;
    reg signed [31:0] deltasIm;
    reg signed [AXIS_TDATA_WIDTH-1:0] deltasReIm;
    wire signed [AXIS_TDATA_WIDTH-1:0] sinecosine_out;
    wire scv;
 
    phase_unwrap phase_Unwrap(
            .aclk(clock),
            .S_AXIS_tdata(td_ph),
            .S_AXIS_tvalid(1),
            .enable(1)
    );

    SineSDB64 s_sdb64 (
              .aclk(clock),
              .S_AXIS_DELTAS_tdata(deltasReIm),
              .S_AXIS_DELTAS_tvalid(1),
              .M_AXIS_SC_tdata(sinecosine_out),
              .M_AXIS_SC_tvalid(scv)
              );


    lms_phase_amplitude_detector lms_phase_amplitude_Detector(
                  .aclk(clock),
                  .S_AXIS_SIGNAL_tdata(td_signal),
                  .S_AXIS_SIGNAL_tvalid(valid),
                  .S_AXIS_SC_tdata(td_sc),
                  .S_AXIS_SC_tvalid(valid),
                  .tau(tau),
                  .Atau(Atau),
                  .dc_tau(dc_tau),
                  .dc(dc)
                  );
                  
    initial begin
        clock = 0;

        deltasRe = 2145957801;
        deltasIm = 80939050;
        deltasReIm = { deltasRe, deltasIm };

        signal = 1000;
        td_signal = {signal, signal};
        valid=1;
        //        set_gpio_cfgreg_int32 (PACPLL_CFG_PACTAU, (int)(Q22/ADC_SAMPLING_RATE/tau)); // Q22 significant from top - tau for phase

        tau = (1<<22)/125000000/50e-6;
        Atau = (1<<22)/125000000/20e-6;
        dc_tau = (1<<32)/30000/10e-4;
           
        dc = -1800;
        t=0;
                               
        forever #1 begin 
            clock = ~clock;
            t=t+1;
            td_sc = {{(8){sinecosine_out[63]}}, sinecosine_out[63:32+8], {(8){sinecosine_out[31]}}, sinecosine_out[31:0+8]};
            signal = 1000+$signed(sinecosine_out[63:57]);
            td_signal = {signal, signal};

            td_ph = -(((t<<<12) % 13176794) - 6588397);

//           if (t > 1000)
//                dc_tau=-1;
        end
    end

endmodule
