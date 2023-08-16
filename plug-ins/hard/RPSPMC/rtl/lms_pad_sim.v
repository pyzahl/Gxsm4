module tb_lms_phase_amplitude_detector;

    parameter S_AXIS_SIGNAL_TDATA_WIDTH = 32; // actually used: LMS DATA WIDTH and Q
    parameter S_AXIS_SC_TDATA_WIDTH = 64;
    parameter M_AXIS_SC_TDATA_WIDTH = 64;
    parameter SC_DATA_WIDTH  = 25;  // SC 25Q24
    parameter SC_Q_WIDTH     = 24;  // SC 25Q24
    parameter LMS_DATA_WIDTH = 26;  // LMS 26Q22
    parameter LMS_Q_WIDTH    = 22;  // LMS 26Q22
    parameter M_AXIS_XY_TDATA_WIDTH = 64;
    parameter M_AXIS_AM_TDATA_WIDTH = 48;
    parameter PH_DELAY = 10;     // PHASE delay steps
    parameter DPHASE_WIDTH = 44; // 44 for delta phase width
    parameter LCK_BUFFER_LEN2 = 14;
    parameter USE_DUAL_PAC = 1;
    parameter COMPUTE_LOCKIN = 0;


    reg aclk;
    reg [S_AXIS_SIGNAL_TDATA_WIDTH-1:0]  S_AXIS_SIGNAL_tdata;
    reg [S_AXIS_SC_TDATA_WIDTH-1:0]  S_AXIS_SC_tdata;
    reg signed [31:0] tau  = 671; // Q22 tau phase      ****   set_gpio_cfgreg_int32 (PACPLL_CFG_PACTAU, (int)(Q22/ADC_SAMPLING_RATE/(1e-6*tau))); // Q22 significant from top - tau for phase ;   ===  Q22/125e6/(1e-6*50)   50us
    reg signed [31:0] Atau = 350; // Q22 tau amplitude
    
    reg [48-1:0]  S_AXIS_DDS_dphi_tdata; // Q44
    
    reg lck_ampl = 0;
    reg lck_phase = 0;



    wire [M_AXIS_SC_TDATA_WIDTH-1:0] M_AXIS_SC_tdata; // (Sine, Cosine) vector pass through
    wire [M_AXIS_XY_TDATA_WIDTH-1:0] M_AXIS_XY_tdata; // (Sine, Cosine) vector pass through
    wire                             M_AXIS_XY_tvalid;

    wire [M_AXIS_AM_TDATA_WIDTH-1:0] M_AXIS_AM2_tdata; // dbg
    wire                             M_AXIS_AM2_tvalid;
    
    wire [31:0] M_AXIS_Aout_tdata;
    wire        M_AXIS_Aout_tvalid;
    wire [31:0] M_AXIS_Bout_tdata;
    wire        M_AXIS_Bout_tvalid;
    wire [31:0] M_AXIS_LockInX_tdata;
    wire        M_AXIS_LockInX_tvalid;
    wire [31:0] M_AXIS_LockInY_tdata;
    wire        M_AXIS_LockInY_tvalid;

    wire signed [31:0] dbg_lckN;
    wire signed [31:0] dbg_lckX;
    wire signed [31:0] dbg_lckY;
    
    wire sc_zero_x;

    // SIM VARS

    reg signed [48:0]PHASE;

    /*
     * Generate a 100Mhz (10ns) clock 
     */
    always begin
        aclk = 1; #5;
        aclk = 0; #5;
    end
    
    real pi = 3.14159265359;

    real freq = 30e3;
    real offset = 0.0;
    real ampl = 0.3;
    real sine_out;
    real cosine_out;
    real testsignal;
    real Q44 = 17592186044416.0;
    real Q24 = 16777216.0;
    real Q22 = 4194304.0;
    reg  [31:0]S;
    reg  [31:0]C;
    reg  [31:0]TS;
    real phlast;
    real ph;
    real dph;


    always @(aclk) begin
        ph = 2 * pi * freq * $time * 1e-12;
        sine_out   = sin(ph);
        cosine_out = cos(ph);
        dph = ph - phlast;
        phlast = ph;
        $write("Sine value at time=%0g is =%0g\n", $time, sine_out);
        
        
        testsignal = 0.2*sin(0.5*pi + 2 * pi * freq * $time * 1e-12);
        
        S = int(Q24*sine_out);
        C = int(Q24*cosine_out);
        TS = int(Q22*testsignal);
        S_AXIS_DDS_dphi_tdata = int(Q44*dph);
        
        S_AXIS_SC_tdata = {S,C};
        S_AXIS_SIGNAL_tdata = TS;
        

    end
    
    lms_phase_amplitude_detector (
    // INPUTS
    .aclk(aclk),
    .S_AXIS_SIGNAL_tdata(S_AXIS_SIGNAL_tdata),
    .S_AXIS_SIGNAL_tvalid(1),

    .S_AXIS_SC_tdata(S_AXIS_SC_tdata),
    .S_AXIS_SC_tvalid(1),
    .tau(TAU), // Q22 tau phase
    .Atau(ATAU), // Q22 tau amplitude
    
    .S_AXIS_DDS_dphi_tdata(S_AXIS_DDS_dphi_tdata),
    .S_AXIS_DDS_dphi_tvalid(1),
    
    .lck_ampl(0),  // ampl selector lck
    .lck_phase(0), // ph selector lck
    
    // OUTPUTS
    .M_AXIS_SC_tdata(M_AXIS_SC_tdata), // (Sine, Cosine) vector pass through
    .M_AXIS_SC_tvalid(M_AXIS_SC_tvalid),

    .M_AXIS_XY_tdata(M_AXIS_XY_tdata), // (Sine, Cosine) vector pass through
    .M_AXIS_XY_tvalid(M_AXIS_XY_tvalid),

    .M_AXIS_AM2_tdata(M_AXIS_AM2_tdata), // dbg
    .M_AXIS_AM2_tvalid(M_AXIS_AM2_tvalid),
    
    .M_AXIS_Aout_tdata(M_AXIS_Aout_tdata),
    .M_AXIS_Aout_tvalid(M_AXIS_Aout_tvalid),
    .M_AXIS_Bout_tdata(M_AXIS_Bout_tdata),
    .M_AXIS_Bout_tvalid(M_AXIS_Bout_tvalid),
    .M_AXIS_LockInX_tdata(M_AXIS_LockInX_tdata),
    .M_AXIS_LockInX_tvalid(M_AXIS_LockInX_tvalid),
    .M_AXIS_LockInY_tdata(M_AXIS_LockInY_tdata),
    .M_AXIS_LockInY_tvalid(M_AXIS_LockInY_tvalid),

    .dbg_lckN(dbg_lckN),
    .dbg_lckX(dbg_lckX),
    .dbg_lckY(dbg_lckY),
    
    .sc_zero_x(sc_zero_x)
    );

    
    
    
    
endmodule