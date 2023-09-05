`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 08/14/2023 06:42:21 PM
// Design Name: 
// Module Name: axis_AD5791
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


module axis_AD5791 #(
    parameter USE_RP_DIGITAL_IO = 0,
    parameter NUM_DAC = 4,
    parameter DAC_DATA_WIDTH = 20,
    parameter DAC_WORD_WIDTH = 24,
    parameter SAXIS_TDATA_WIDTH = 32
)
(
    (* X_INTERFACE_PARAMETER = "ASSOCIATED_CLKEN a_clk" *)
    (* X_INTERFACE_PARAMETER = "ASSOCIATED_BUSIF S_AXIS1:S_AXIS2:S_AXIS3:S_AXIS4:S_AXISCFG" *)
    input a_clk,
    input wire [SAXIS_TDATA_WIDTH-1:0]  S_AXIS1_tdata,
    input wire                          S_AXIS1_tvalid,
    input wire [SAXIS_TDATA_WIDTH-1:0]  S_AXIS2_tdata,
    input wire                          S_AXIS2_tvalid,
    input wire [SAXIS_TDATA_WIDTH-1:0]  S_AXIS3_tdata,
    input wire                          S_AXIS3_tvalid,
    input wire [SAXIS_TDATA_WIDTH-1:0]  S_AXIS4_tdata,
    input wire                          S_AXIS4_tvalid,

    input wire [SAXIS_TDATA_WIDTH-1:0]  S_AXISCFG_tdata,
    input wire                          S_AXISCFG_tvalid,
    
    input wire configuration_mode,
    input wire [2:0] configuration_axis,
    input wire configuration_send,
    
    // (* X_INTERFACE_PARAMETER = "FREQ_HZ 30000000" *)
    (* X_INTERFACE_PARAMETER = "ASSOCIATED_CLKEN wire_PMD_clk" *)
    (* X_INTERFACE_PARAMETER = "ASSOCIATED_BUSIF wire_PMD_sync:wire_PMD_dac" *)
    output wire wire_PMD_clk,
    output wire wire_PMD_sync,
    output wire [NUM_DAC-1:0] wire_PMD_dac,
    
    output wire [31:0] mon0,
    output wire [31:0] mon1,
    output wire [31:0] mon2,
    output wire [31:0] mon3,
    
    output wire ready
    );
    
    
    reg PMD_clk=0;   // PMOD SPI CLK
    reg PMD_sync=1;  // PMOD SPI SYNC
    reg PMD_dac [NUM_DAC-1:0]; // PMOD SPI DATA DAC[1..4]
    
    reg [DAC_WORD_WIDTH-1:0] reg_dac_data[NUM_DAC-1:0];
    reg [DAC_WORD_WIDTH-1:0] reg_dac_data_buf[NUM_DAC-1:0];
    
    reg cfg_mode_job=0;
    reg sync=1;
    reg start=0;
    reg [6-1:0] frame_bit_counter=0;
    reg [4-1:0]state_load_dacs=0;
    
    reg [2:0] rdecii = 0; //4 for tetsing slow [1:0]

    integer i;
    initial begin
      for (i=0;i<NUM_DAC;i=i+1)
      begin
            reg_dac_data[i] = 0;
            reg_dac_data_buf[i] = 0;
            PMD_dac[i]=0;
      end     
    end

    always @ (posedge a_clk) // 120MHz
    begin
        rdecii <= rdecii+1;
        PMD_clk <= rdecii[2]; // 1/4 => 30MHz  , 4: testing slow
    end

    // Latch data from AXIS when new
    always @(*) //posedge PMD_clk)
    begin
        if (configuration_mode)
        begin
            if (S_AXISCFG_tvalid)
            begin
                reg_dac_data[configuration_axis[1:0]] <= S_AXISCFG_tdata[DAC_WORD_WIDTH-1:0];
            end
        end
        else
        begin
            if (S_AXIS1_tvalid)
            begin            
                //                       send to data register 24 bits:         0 001 20-bit-data
                reg_dac_data[0] <= {{(DAC_WORD_WIDTH-DAC_DATA_WIDTH-1 ){1'b0}}, 1'b1, S_AXIS1_tdata[SAXIS_TDATA_WIDTH-1:SAXIS_TDATA_WIDTH-DAC_DATA_WIDTH]};
            end
            if (S_AXIS2_tvalid)
            begin            
                reg_dac_data[1] <= {{(DAC_WORD_WIDTH-DAC_DATA_WIDTH-1 ){1'b0}}, 1'b1, S_AXIS2_tdata[SAXIS_TDATA_WIDTH-1:SAXIS_TDATA_WIDTH-DAC_DATA_WIDTH]};
            end
            if (S_AXIS3_tvalid)
            begin            
                reg_dac_data[2] <= {{(DAC_WORD_WIDTH-DAC_DATA_WIDTH-1 ){1'b0}}, 1'b1, S_AXIS3_tdata[SAXIS_TDATA_WIDTH-1:SAXIS_TDATA_WIDTH-DAC_DATA_WIDTH]};
            end
            if (S_AXIS4_tvalid)
            begin            
                reg_dac_data[3] <= {{(DAC_WORD_WIDTH-DAC_DATA_WIDTH-1 ){1'b0}}, 1'b1, S_AXIS4_tdata[SAXIS_TDATA_WIDTH-1:SAXIS_TDATA_WIDTH-DAC_DATA_WIDTH]};
            end
        end
    end


    always @(negedge PMD_clk) // spi clock
    begin
        //clkr <= 0; // generate clkr
        // Detect Frame Sync
        case (state_load_dacs)
            0: // reset mode, wait for new data
            begin
                sync <= 1;
                if (reg_dac_data_buf[0] != reg_dac_data[0] || reg_dac_data_buf[1] != reg_dac_data[1] || reg_dac_data_buf[2] != reg_dac_data[2] || reg_dac_data_buf[3] != reg_dac_data[3])
                begin
                    if (configuration_mode)
                    begin // load only on configuration_send pos edge
                        if (configuration_send)
                        begin
                            if (!cfg_mode_job)
                            begin
                                cfg_mode_job <= 1;
                                state_load_dacs <= 1;
                            end
                        end
                        else
                        begin
                            cfg_mode_job <= 0; // do not repat sending! "send bit" must be reset while in config mode
                        end
                    end
                    else // auto load and start sending on new data
                    begin
                        state_load_dacs <= 1;
                    end
                end
            end
            1: // load dac start
            begin
                sync <= 1;
                frame_bit_counter <= 10'd23; // 24 bits to send
    
                // Latch Axis Data at Frame Sync Pulse and initial data serialization
                reg_dac_data_buf[0] <= reg_dac_data[0];
                reg_dac_data_buf[1] <= reg_dac_data[1];
                reg_dac_data_buf[2] <= reg_dac_data[2];
                reg_dac_data_buf[3] <= reg_dac_data[3];

                state_load_dacs <= 2;
            end
            2:
            begin
                sync <= 0;
                state_load_dacs <= 3; // one more sync high cycle
            end
            3:
            begin
                // completed?
                if (frame_bit_counter == 10'b0)
                begin
                    state_load_dacs <= 4; // finish
                end else
                begin
                    // next bit
                    frame_bit_counter <= frame_bit_counter - 1;
                end
            end   
            4:
            begin
                sync <= 1;
                state_load_dacs <= 0; // completed, standy next
            end   
        endcase
    end

    always @(posedge PMD_clk) // SPI transmit data setup edge
    begin
        PMD_sync   <= sync;
        PMD_dac[0] <= reg_dac_data_buf[0][frame_bit_counter];
        PMD_dac[1] <= reg_dac_data_buf[1][frame_bit_counter];
        PMD_dac[2] <= reg_dac_data_buf[2][frame_bit_counter];
        PMD_dac[3] <= reg_dac_data_buf[3][frame_bit_counter];
    end

assign ready = (state_load_dacs == 1'b0000);

assign wire_PMD_clk  = PMD_clk;
assign wire_PMD_sync = PMD_sync;
assign wire_PMD_dac  = { PMD_dac[3], PMD_dac[2], PMD_dac[1], PMD_dac[0] };
  
assign mon0 = { reg_dac_data_buf[0][19:0], {(32-24){1'b0}}, reg_dac_data_buf[0][23:20] };
assign mon1 = { reg_dac_data_buf[1][19:0], {(32-24){1'b0}}, reg_dac_data_buf[1][23:20] };
assign mon2 = { reg_dac_data_buf[2][19:0], {(32-24){1'b0}}, reg_dac_data_buf[2][23:20] };
assign mon3 = { reg_dac_data_buf[3][19:0], {(32-24){1'b0}}, reg_dac_data_buf[3][23:20] };
    
endmodule
