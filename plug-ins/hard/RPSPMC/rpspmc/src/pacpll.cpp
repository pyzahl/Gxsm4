/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 8 c-style: "K&R" -*- */

/* Gxsm - Gnome X Scanning Microscopy
 * universal STM/AFM/SARLS/SPALEED/... controlling and
 * data analysis software
 * 
 * Copyright (C) 1999,2000,..2023 Percy Zahl
 *
 * Authors: Percy Zahl <zahl@users.sf.net>
 * additional features: Andreas Klust <klust@users.sf.net>
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

#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/sysinfo.h>
#include <vector>
#include <sys/mman.h>
#include <fcntl.h>
#include <pthread.h>

#include <DataManager.h>
#include <CustomParameters.h>

#include "fpga_cfg.h"
#include "pacpll.h"

#include "spmc_module_config_utils.h"



pthread_attr_t gpio_reading_attr;
pthread_mutex_t gpio_reading_mutexsum;
int gpio_reading_control = -1;

double gpio_reading_FIRV_vector[MAX_FIR_VALUES];
double gpio_reading_FIRV_vector_CH3_mapping = 0.0;
double gpio_reading_FIRV_vector_CH4_mapping = 0.0;
double gpio_reading_FIRV_vector_CH5_mapping = 0.0;
int gpio_reading_FIRV_buffer[MAX_FIR_VALUES][GPIO_FIR_LEN];
int gpio_reading_FIRV_buffer_CH3_mapping[GPIO_FIR_LEN];
int gpio_reading_FIRV_buffer_CH4_mapping[GPIO_FIR_LEN];
int gpio_reading_FIRV_buffer_CH5_mapping[GPIO_FIR_LEN];
unsigned long gpio_reading_FIRV_buffer_DDS[2][GPIO_FIR_LEN];

static pthread_t gpio_reading_thread;



/*
 * RedPitaya A9 FPGA Control and Data Transfer
 * ------------------------------------------------------------
 */




// extern int verbose;
extern double signal_dc_measured;
extern int phase_setpoint_qcordicatan;


extern CIntSignal SIGNAL_GPIOX;
extern CDoubleParameter FREQUENCY_MANUAL;
extern CDoubleParameter DDS_FREQ_MONITOR;
extern CIntParameter SHR_DEC_DATA;
extern CIntParameter TRANSPORT_DECIMATION;
extern CDoubleParameter AUX_SCALE;
extern CDoubleParameter FREQUENCY_CENTER;
extern CDoubleParameter PAC_DCTAU;

extern CIntParameter TRANSPORT_CH3;
extern CIntParameter TRANSPORT_CH4;
extern CIntParameter TRANSPORT_CH5;


// Q44: 32766.0000 Hz -> phase_inc=4611404543  0000000112dc72ff
double dds_phaseinc (double freq){
        double fclk = ADC_SAMPLING_RATE;
        return QQ44*freq/fclk;
}

double dds_phaseinc_to_freq (unsigned long long ddsphaseincQ44){
        double fclk = ADC_SAMPLING_RATE;
#ifdef DEVELOPMENT_PACPLL_OP_V
        double df = fclk*(double)ddsphaseincQ44/(double)(QQ44);
        fprintf(stderr, "##DDS Phase Inc abs to freq: df= %12.4f Hz <- QQ44 phase_inc=%lld  %016llx\n", df, ddsphaseincQ44, ddsphaseincQ44phase_inc);
        return df;
#else
        return fclk*(double)ddsphaseincQ44/(double)(QQ44);
#endif
}

double dds_phaseinc_rel_to_freq (long long ddsphaseincQ44){
        double fclk = ADC_SAMPLING_RATE;
#ifdef DEVELOPMENT_PACPLL_OP_V
        double df = fclk*(double)ddsphaseincQ44/(double)(QQ44);
        fprintf(stderr, "##DDS Phase Inc rel to freq: df= %12.4f Hz <- Q44 phase_inc=%lld  %016llx\n", df, ddsphaseincQ44, ddsphaseincQ44phase_inc);
        return df;
#else
        return fclk*(double)ddsphaseincQ44/(double)(QQ44);
#endif
}


void reset_lms (){
        rp_spmc_module_config_int32 (RESET_LMS_PHASE_AMPLITUDE_DETECTOR_ADDRESS, 1); // WRITE: SET RESETN
        rp_spmc_module_config_int32 (RESET_LMS_PHASE_AMPLITUDE_DETECTOR_ADDRESS, 0); // WRITE: RELEASE RESETN
}
        
void config_lms (int pactau_ph, int pactau_am, int modes){
        rp_spmc_module_config_int32 (MODULE_SETUP, pactau_ph, LMS_PACPLL_CFG_PACTAU);
        rp_spmc_module_config_int32 (MODULE_SETUP, pactau_am, LMS_PACPLL_CFG_PACATAU);
        rp_spmc_module_config_int32 (LMS_PHASE_AMPLITUDE_DETECTOR_ADDRESS, modes, LMS_PACPLL_CFG_LCK_AM_PH); // WRITE
}

void config_transport (int channelsel, int ndecimate, int nsamples, int opmode, int shift, unsigned long long fcenter){
        rp_spmc_module_config_int32 (MODULE_SETUP, channelsel, TRANSPORT_CHANNEL_SELECTOR); // MODULE_START_VECTOR=0
        rp_spmc_module_config_int32 (MODULE_SETUP, ndecimate,  TRANSPORT_NDECIMATE);
        rp_spmc_module_config_int32 (MODULE_SETUP, nsamples,   TRANSPORT_NSAMPLES);
        rp_spmc_module_config_int32 (MODULE_SETUP, opmode,     TRANSPORT_OPERATION);
        rp_spmc_module_config_int32 (MODULE_SETUP, shift,      TRANSPORT_SHIFT);
        rp_spmc_module_config_int64 (TRANSPORT_4S_COMBINE_ADDRESS, fcenter, TRANSPORT_FREQ_CENTER); // WRITE
}

void config_controller (int addr, int setpoint, int cp, int ci, long long upper, long long lower){
        rp_spmc_module_config_int32 (MODULE_SETUP, setpoint,    CONTROLLER_SETPOINT); // MODULE_START_VECTOR=0
        rp_spmc_module_config_int32 (MODULE_SETUP, cp,          CONTROLLER_CP);
        rp_spmc_module_config_int32 (MODULE_SETUP, ci,          CONTROLLER_CI);
        rp_spmc_module_config_int64 (MODULE_SETUP, upper, CONTROLLER_UPPER);
        rp_spmc_module_config_int64 (addr,         lower, CONTROLLER_LOWER); // WRITE
}

void config_controller_m (int addr, long long resetval, int mode, unsigned int threshold=0){
        rp_spmc_module_config_int64 (MODULE_SETUP, resetval,  CONTROLLER_M_RESET_VAL); // MODULE_START_VECTOR=0
        rp_spmc_module_config_int32 (MODULE_SETUP, mode,      CONTROLLER_M_MODE);
        rp_spmc_module_config_int32 (addr,         threshold, CONTROLLER_M_THREASHOLD); // WRITE
}


/*
void rp_PAC_adjust_dds (double freq){
        // 44 Bit Phase, using 48bit tdata
        unsigned long long phase_inc = (unsigned long long)round (dds_phaseinc (freq));
        //        unsigned int lo32, hi32;
#ifdef DEVELOPMENT_PACPLL_OP
        if (verbose == 1) fprintf(stderr, "##Adjust DDS: f= %12.4f Hz -> Q44 phase_inc=%lld  %016llx\n", freq, phase_inc, phase_inc);
#else
        if (verbose > 2) fprintf(stderr, "##Adjust: f= %12.4f Hz -> Q44 phase_inc=%lld  %016llx\n", freq, phase_inc, phase_inc);
#endif
        
        set_gpio_cfgreg_int48 (PACPLL_CFG_DDS_PHASEINC, phase_inc); // DFREQ_RESET

#ifdef DEVELOPMENT_PACPLL_OP
        if (verbose >= 2){
                // Verify
                unsigned long xx8 = read_gpio_reg_uint32 (4,1); // GPIO X8 : DDS Phase Inc (Freq.) upper 32 bits of 44 (signed)
                unsigned long xx9 = read_gpio_reg_uint32 (5,0); // GPIO X9 : DDS Phase Inc (Freq.) lower 32 bits of 44 (signed)
                unsigned long long ddsphi = ((unsigned long long)xx8<<(44-32)) + ((unsigned long long)xx9>>(64-44)); // DDS Phase Inc (Freq.) upper 32 bits of 44 (signed)
                double vfreq = dds_phaseinc_to_freq(ddsphi);
                fprintf(stderr, "##Verify DDS: f= %12.4f Hz -> Q44 phase_inc=%lld  %016llx  ERROR: %12.4f Hz\n", vfreq, ddsphi, ddsphi, (vfreq-freq));
        }
#endif

}
*/

/*
void rp_PAC_set_volume (double volume){
        if (verbose > 2) fprintf(stderr, "##Configure: volume= %g V\n", volume); 
        set_gpio_cfgreg_int32 (PACPLL_CFG_VOLUME_SINE, (int)(Q31 * volume));
}
*/
/*
// Configure Control Switched: Loops Ampl and Phase On/Off, Unwrapping, QControl
void rp_PAC_configure_switches (int phase_ctrl, int am_ctrl, int phase_unwrap_always, int qcontrol, int lck_amp, int lck_phase, int dfreq_ctrl){
        if (verbose > 2) fprintf(stderr, "##Configure loop controls: 0x%08x\n",  phase_ctrl ? 1:0 | am_ctrl ? 2:0); 
        set_gpio_cfgreg_int32 (PACPLL_CFG_CONTROL_LOOPS,
                               (phase_ctrl ? 1:0) | (am_ctrl ? 2:0) | (phase_unwrap_always ? 4:0) |   // Bits 1,2,3
                               (qcontrol ? 8:0) |                                                     // Bit  4
                               (lck_amp ? 0x10:0)  | (lck_phase ? 0x20:0) |                           // Bits 5,6
                               //(b7 ? 0x40:0) | (b8 ? 0x80:0)
                               (dfreq_ctrl ? 0x100:0)                                                   // Bit 9
                               );

        config_lms (int dctau, int dc_offset, int pactau_ph, int pactau_am, int modes);       
}
*/

// Configure Q-Control Logic build into Volume Adjuster
void rp_PAC_set_qcontrol (double gain, double phase, int enable){
        double samples_per_period = ADC_SAMPLING_RATE / FREQUENCY_MANUAL.Value ();
        int ndelay = int (samples_per_period * phase/360. + 0.5);

        if (ndelay < 0 || ndelay > 8192 || phase < 0.)
                ndelay = 0; // Q-Control disabled when delay == 0

        if (verbose > 2) fprintf(stderr, "##Configure: qcontrol= %d, %d\n", (int)(Q15*gain), ndelay); 

        ndelay = 8192-ndelay; // pre compute offset in ring buffer
        //set_gpio_cfgreg_int32 (QCONTROL_CFG_GAIN_DELAY, ((int)(Q10 * gain)<<16) | ndelay );

        rp_spmc_module_config_int32 (MODULE_SETUP,          enable ? 1:0, QCONTROL_ENABLE); // 0: MODULE_START_VECTOR);
        rp_spmc_module_config_int32 (MODULE_SETUP,     (int)(Q10 * gain), QCONTROL_GAIN);
        rp_spmc_module_config_int32 (QCONTROL_ADDRESS,           ndelay , QCONTROL_DELAY);

}

#if 0
// tau from mu
double time_const(double fref, double mu){
        return -(1.0/fref) / log(1.0-mu);
}
// mu from tau
double mu_const(double fref, double tau){
        return 1.0-exp(-1.0/(fref*tau));
}
// -3dB cut off freq
double cut_off_freq_3db(double fref, double mu){
        return -(1.0/(fref*2.*M_PI)) * log(1.0-mu);
}

double mu_opt (double periods){
        double mu = 11.9464 / (6.46178 + periods);
        return mu > 1.0 ? 1.0 : mu;
}
#endif

// tau in s for dual PAC and auto DC offset
// Set "manual" DC offset used if dc_tau (see above) signum bit is set (neg).

void rp_PAC_set_pactau (double tau, double atau, int modes){
        if (verbose > 1) fprintf(stderr, "##Configure PAC TAU: %g us, %g us, M=%x\n", tau, atau, modes);

        config_lms ((int)(Q22/ADC_SAMPLING_RATE/(1e-6*tau)), // PH TAU in tau s (us) -> mu
                    (int)(Q22/ADC_SAMPLING_RATE/(1e-6*atau)), // AM TAU  in tau s (us) -> mu
                    modes);       

}

// Set "manual" DC offset used if dc_tau (see above) signum bit is set (neg).
void rp_PAC_set_dc_filter (double dc, double dc_tau){
        if (verbose > 3) fprintf(stderr, "##Configure DC-FILTER: dc=%g, dc-tau=%g s\n", dc, dc_tau); 
        //set_gpio_cfgreg_int32 (PACPLL_CFG_DC_OFFSET, (int)(Q22 * dc));

        // Q22 significant from top -- dc_tau is tau for DC FIR-IIR Filter on phase aligned decimated data:
        // at 4x Freq Ref sampling rate. Moving averaging FIR sampling at past 4 zero crossing of Sin Cos ref passed on to IIR with tau
        // dc offset (manual)
        //        dc_tau > 0. ? (int)(Q31/(4.*FREQUENCY_MANUAL.Value ())/dc_tau) : dc_tau < 0. ? 0xffffffff : 0,  // set dc-tau, disable, freeze
        //        (int)(Q22 * dc), 

        rp_spmc_module_config_int32 (MODULE_SETUP, dc_tau > 0. ? (int)(Q31/(4.*FREQUENCY_MANUAL.Value ())/dc_tau) : dc_tau < 0. ? 0xffffffff : 0, DC_FILTER_TAU); // MODULE_START_VECTOR=0  ** set dc-tau, disable, freeze
        rp_spmc_module_config_Qn (DC_FILTER_ADDRESS, dc, DC_FILTER_OFFSET, Q22); // Q22 DC Offset

}

// measure DC, update manual offset
void rp_PAC_auto_dc_offset_adjust (){
        double dc = 0.0;
        int x,i,k;
        double x1;

        rp_PAC_set_dc_filter (0.0, -1.);
        usleep(10000);
        
        for (k=i=0; i<300000; ++i){
                if (i%777 == 0) usleep(1000);
                x = read_gpio_reg_int32 (3,0); // 2,0
                x1=(double)x / QLMS;
                if (fabs(x1) < 0.5){
                        dc += x1;
                        ++k;
                }
        }
        dc /= k;
        if (verbose >= 1) fprintf(stderr, "RP PACPLL DC-Offset = %10.6f  {%d}\n", dc, k);
        rp_PAC_set_dc_filter (dc, PAC_DCTAU.Value() * 1e-3);

        signal_dc_measured = dc;
}

// update/follow (slow IIR) DC offset
void rp_PAC_auto_dc_offset_correct (){
        int i,x;
        double x1;
        double q=0.0000001;
        double q1=1.0-q;
        for (i=0; i<30000; ++i){
                if (i%777 == 0) usleep(1000);
                x = read_gpio_reg_int32 (3,0);
                x1=(double)x / QLMS;
                if (fabs(x1) < 0.1)
                        signal_dc_measured = q1*signal_dc_measured + q*x1;
        }
        rp_PAC_set_dc_filter (signal_dc_measured, PAC_DCTAU.Value() * 1e-3);
}

// Controller Topology:
/*
  // IP Configuration
  //                                  DEFAULTS   PHASE   AMPL         
    parameter AXIS_TDATA_WIDTH =            32,  24      24    // INPUT AXIS DATA WIDTH
    parameter M_AXIS_CONTROL_TDATA_WIDTH =  32,  48      16    // SERVO CONTROL DATA WIDTH OF AXIS
    parameter CONTROL_WIDTH =               32,  44      16    // SERVO CONTROL DATA WIDTH
    parameter M_AXIS_CONTROL2_TDATA_WIDTH = 32,  48      32    // INTERNAL CONTROl DATA WIDTH MAPPED TO AXIS FOR READOUT not including extend
    parameter CONTROL2_WIDTH =              50,  75      56    // INTERNAL CONTROl DATA WIDTH not including extend **** COEFQ+AXIS_TDATA_WIDTH == CONTROL2_WIDTH
    parameter CONTROL2_OUT_WIDTH =          32,  44      32    // max passed outside control width, must be <= CONTROL2_WIDTH
    parameter COEF_WIDTH =                  32,  32      32    // CP, CI WIDTH
    parameter QIN =                         22,  22      22    // Q In Signal
    parameter QCOEF =                       31,  31      31    // Q CP, CI's
    parameter QCONTROL =                    31,  31      31    // Q Controlvalue
    parameter CEXTEND =                      4,   1       4    // room for saturation check
    parameter DEXTEND =                      1,   1       1    // data, erorr extend
    parameter AMCONTROL_ALLOW_NEG_SPECIAL =  1    0       1    // Special Ampl. Control Mode
  // ********************

  if (AMCONTROL_ALLOW_NEG_SPECIAL)
     if (error_next > $signed(0) && control_next < $signed(0)) // auto reset condition for amplitude control to preven negative phase, but allow active "damping"
          control      <= 0;
          controlint   <= 0;

  ... check limits and limit to upper and lower, set limiter status indicators;

  m = AXI-axi_input;   // [AXIS_TDATA_WIDTH-1:0]
  error = setpoint - m;

  if (enable)
    controlint_next <= controlint + ci*error; // saturation via extended range and limiter // Q64.. += Q31 x Q22 ==== AXIS_TDATA_WIDTH + COEF_WIDTH
    control_next    <= controlint + cp*error; // 
  else
    controlint_next <= reset;
    control_next    <= reset;

  *************************
  assign M_AXIS_CONTROL_tdata   = {control[CONTROL2_WIDTH+CEXTEND-1], control[CONTROL2_WIDTH-2:CONTROL2_WIDTH-CONTROL_WIDTH]}; // strip extension
  assign M_AXIS_CONTROL2_tdata  = {control[CONTROL2_WIDTH+CEXTEND-1], control[CONTROL2_WIDTH-2:CONTROL2_WIDTH-CONTROL2_OUT_WIDTH]};

  assign mon_signal  = {m[AXIS_TDATA_WIDTH+DEXTEND-1], m[AXIS_TDATA_WIDTH-2:0]};
  assign mon_error   = {error[AXIS_TDATA_WIDTH+DEXTEND-1], error[AXIS_TDATA_WIDTH-2:0]};
  assign mon_control = {control[CONTROL2_WIDTH+CEXTEND-1], control[CONTROL2_WIDTH-2:CONTROL2_WIDTH-32]};
  assign mon_control_lower32 = {{control[CONTROL2_WIDTH-32-1 : (CONTROL2_WIDTH>=64? CONTROL2_WIDTH-32-1-31:0)]}, {(CONTROL2_WIDTH>=64?0:(64-CONTROL2_WIDTH)){1'b0}}}; // signed, lower 31
  *************************
 */


// Configure Controllers


// Amplitude Controller
// AMPL from CORDIC: 24bit Q23 -- QCORDICSQRT
void rp_PAC_set_amplitude_controller (double setpoint, double cp, double ci, double upper, double lower, double manual_volume, int enable){
        if (verbose > 1) fprintf(stderr, "##Configure AM-Controller: setpt=%g, cp=%g ci=%g, upper=%g lower=%g volume=%g V, en=%d\n",
                                 setpoint, cp, ci, upper, lower, manual_volume, enable); 
        /*
        double Q = pll.Qfactor;     // Q-factor
        double F0 = pll.FSineHz; // Res. Freq
        double Fc = pll.auto_set_BW_Amp; // 1.5 .. 10Hz
        double gainres = pll.GainRes;
        double cp = 20. * log10 (0.08045   * Q*Fc / (gainres*F0));
        double ci = 20. * log10 (8.4243e-7 * Q*Fc*Fc / (gainres*F0));

        write_pll_variable32 (dsp_pll.icoef_Amp, pll.signum_ci_Amp * CPN(29)*pow (10.,pll.ci_gain_Amp/20.));
        // = ISign * CPN(29)*pow(10.,Igain/20.);
		
        write_pll_variable32 (dsp_pll.pcoef_Amp, pll.signum_cp_Amp * CPN(29)*pow (10.,pll.cp_gain_Amp/20.));
        // = PSign * CPN(29)*pow(10.,Pgain/20.);
        */
        //set_gpio_cfgreg_int32 (PACPLL_CFG_AMPLITUDE_CONTROLLER + PACPLL_CFG_SET,   ((int)(QCORDICSQRT * setpoint))<<(32-BITS_CORDICSQRT));
        //set_gpio_cfgreg_int32 (PACPLL_CFG_AMPLITUDE_CONTROLLER + PACPLL_CFG_CP,    ((int)(QAMCOEF * cp)));
        //set_gpio_cfgreg_int32 (PACPLL_CFG_AMPLITUDE_CONTROLLER + PACPLL_CFG_CI,    ((int)(QAMCOEF * ci)));
        //set_gpio_cfgreg_int32 (PACPLL_CFG_AMPLITUDE_CONTROLLER + PACPLL_CFG_UPPER, ((int)(QEXEC * upper)));
        //set_gpio_cfgreg_int32 (PACPLL_CFG_AMPLITUDE_CONTROLLER + PACPLL_CFG_LOWER, ((int)(QEXEC * lower)));

        config_controller (AMPLITUDE_CONTROLLER_ADDRESS,
                           (int)(QCORDICSQRT * setpoint),
                           (int)(QAMCOEF * cp),
                           (int)(QAMCOEF * ci),
                           (long long)(QEXEC_L * upper),
                           (long long)(QEXEC_L * lower));
        
        config_controller_m (AMPLITUDE_CONTROLLER_M_ADDRESS,
                             (long long)(QEXEC_L * manual_volume), // Volume Sine Manual -- when in reset mode // Reset is 16bit
                             enable ? 1:0,
                             0);
        }

// Phase Controller
// CONTROL[75] OUT[44] : [75-1-1:75-44]=43+1   m[24]  x  c[32]  = 56 M: 24{Q32},  P: 44{Q14}
void rp_PAC_set_phase_controller (double setpoint, double cp, double ci, double upper, double lower, double am_threashold, double freq_manual, int enable){
        if (verbose > 1) fprintf(stderr, "##Configure PH-Controller: setpt=%g, cp=%g ci=%g, upper=%g lower=%g, amth=%g, frq=%g, en=%d\n",
                                 setpoint, cp, ci, upper, lower, am_threashold, freq_manual, enable);

        /*
        double cp = 20. * log10 (1.6575e-5  * pll.auto_set_BW_Phase);
        double ci = 20. * log10 (1.7357e-10 * pll.auto_set_BW_Phase*pll.auto_set_BW_Phase);

        write_pll_variable32 (dsp_pll.icoef_Phase, pll.signum_ci_Phase * CPN(29)*pow (10.,pll.ci_gain_Phase/20.));
        // = ISign * CPN(29)*pow(10.,Igain/20.);
	
        write_pll_variable32 (dsp_pll.pcoef_Phase, pll.signum_cp_Phase * CPN(29)*pow (10.,pll.cp_gain_Phase/20.));
        // = PSign * CPN(29)*pow(10.,Pgain/20.);
        */

        //set_gpio_cfgreg_int32 (PACPLL_CFG_PHASE_CONTROLLER + PACPLL_CFG_SET,   (phase_setpoint_qcordicatan = (int)(QCORDICATAN * setpoint))); // <<(32-BITS_CORDICATAN));
        //set_gpio_cfgreg_int32 (PACPLL_CFG_PHASE_CONTROLLER + PACPLL_CFG_CP,    (long long)(QPHCOEF * cp)); // {32}   22+1 bit error, 32bit CP,CI << 31 --  m[24]  x  cp|ci[32]  = 56 M: 24{Q32},  P: 44{Q14}, top S43
        //set_gpio_cfgreg_int32 (PACPLL_CFG_PHASE_CONTROLLER + PACPLL_CFG_CI,    (long long)(QPHCOEF * ci));
        //set_gpio_cfgreg_int48 (PACPLL_CFG_PHASE_CONTROLLER + PACPLL_CFG_UPPER, (unsigned long long)round (dds_phaseinc (upper))); // => 44bit phase
        //set_gpio_cfgreg_int48 (PACPLL_CFG_PHASE_CONTROLLER + PACPLL_CFG_LOWER, (unsigned long long)round (dds_phaseinc (lower)));

        config_controller (PHASE_CONTROLLER_ADDRESS,
                           (phase_setpoint_qcordicatan = (int)(QCORDICATAN * setpoint)),
                           (int)(QPHCOEF * cp),
                           (int)(QPHCOEF * ci),
                           (long long)round (dds_phaseinc (upper)),
                           (long long)round (dds_phaseinc (lower)));

        
// PHASE_CONTROLLER_THREASHOLD for hold control if amplituide drops into noise regime to avoid run away
// AMPL from CORDIC: 24bit Q23 -- QCORDICSQRT
        //set_gpio_cfgreg_int32 (PACPLL_CFG_PHASE_CONTROL_THREASHOLD,   ((int)(QCORDICSQRT * am_threashold))<<(32-BITS_CORDICSQRT));

        config_controller_m (PHASE_CONTROLLER_M_ADDRESS,
                             (long long)round (dds_phaseinc (freq_manual)),
                             enable ? 1:0,
                             (int)(QCORDICSQRT * am_threashold)); // AM Threashold
        
}


void rp_PAC_set_pulse_form (double bias0, double bias1,
                            double phase0, double phase1,
                            double width0, double width0if,
                            double width1, double width1if,
                            double height0, double height0if,
                            double height1, double height1if,
                            double shapexw, double shapexwif,
                            double shapex, double shapexif){ // deg, us, mV, [Hz-ref]
        // T = 1/freq_ref, N = T/ADC_SAMPLING_RATE, N = freq_ref / ADC_SAMPLING_RATE
        // m = phase/360 * N
        
        const int RDECI = 2;
        double freq_ref = DDS_FREQ_MONITOR.Value ();
        double mfactor  = ADC_SAMPLING_RATE / RDECI / freq_ref / 360.; // phase in deg to # samples
        double usfactor = 1e-6 * ADC_SAMPLING_RATE;     // us to samples
        double mVfactor = 1e-3*32768.; // mV to DAC[16bit]

        double p0phws2 =  0.5*(width0 + 2*width0if)*usfactor; // complete pulse 1/2 width in steps
        double p1phws2 =  0.5*(width1 + 2*width1if)*usfactor;

        phase0 -= p0phws2/mfactor; if (phase0 < 0.) phase0 = 0.;
        phase1 -= p1phws2/mfactor; if (phase1 < 0.) phase1 = 0.;

        // phase delay
        rp_spmc_module_config_uint16_uint16 (PULSE_FORMER_DL_ADDRESS, (guint16)(phase0*mfactor+0.5),    (guint16)(phase1*mfactor+0.5)); // do conversion phase angle in deg to phase steps ** WRITE
        
        double rstif = shapexwif; // relative shaping pulse length time
        double pstif = 1.0 - rstif; // remaining pulse length time
        double rst = shapexw; // relative shaping pulse length time
        double pst = 1.0 - rst; // remaining pulse length time
        
        // pre pulse shaping pre pulse, then remaining pulse
        rp_spmc_module_config_uint16_uint16 (MODULE_SETUP, (guint16)(width0if*rstif*usfactor+0.5),     (guint16)(width1if*rstif*usfactor+0.5), 0);  // 0,1: width in us to steps
        rp_spmc_module_config_int16_int16   (MODULE_SETUP, ( gint16)(height0if*shapexif*mVfactor),     ( gint16)(height1if*shapexif*mVfactor), 1);  // 2,3: height in mV to 16bit (later 14bit on FPGA)
        rp_spmc_module_config_uint16_uint16 (MODULE_SETUP, (guint16)(width0if*pstif*usfactor+0.5),     (guint16)(width1if*pstif*usfactor+0.5), 2);  // 0,1: width in us to steps
        rp_spmc_module_config_int16_int16   (MODULE_SETUP, ( gint16)(height0if*mVfactor),              ( gint16)(height1if*mVfactor),          3);  // 2,3: height in mV to 16bit (later 14bit on FPGA)

        // pulse shaping pre pulse, then remaining pulse
        rp_spmc_module_config_uint16_uint16 (MODULE_SETUP, (guint16)(width0*rst*usfactor+0.5),         (guint16)(width1*rst*usfactor+0.5),     4);  // 4,5: width in us to steps
        rp_spmc_module_config_int16_int16   (MODULE_SETUP, ( gint16)(height0*shapex*mVfactor),         ( gint16)(height1*shapex*mVfactor),     5);  // 6,7: height in mV to 16bit (later 14bit on FPGA)
        rp_spmc_module_config_uint16_uint16 (MODULE_SETUP, (guint16)(width0*pst*usfactor+0.5),         (guint16)(width1*pst*usfactor+0.5),     6);  // 4,5: width in us to steps
        rp_spmc_module_config_int16_int16   (MODULE_SETUP, ( gint16)(height0*mVfactor),                ( gint16)(height1*mVfactor),            7);  // 6,7: height in mV to 16bit (later 14bit on FPGA)

        // post pulse shaping pre pulse, then remaining pulse
        rp_spmc_module_config_uint16_uint16 (MODULE_SETUP, (guint16)(width0if*rstif*usfactor+0.5),     (guint16)(width1if*rstif*usfactor+0.5), 8);  // 8,9: width in us to steps
        rp_spmc_module_config_int16_int16   (MODULE_SETUP, ( gint16)(height0if*shapexif*mVfactor),     ( gint16)(height1if*shapexif*mVfactor), 9);  // 10,11: height in mV to 16bit (later 14bit on FPGA)
        rp_spmc_module_config_uint16_uint16 (MODULE_SETUP, (guint16)(width0if*pstif*usfactor+0.5),     (guint16)(width1if*pstif*usfactor+0.5),10);  // 8,9: width in us to steps
        rp_spmc_module_config_int16_int16   (MODULE_SETUP, ( gint16)(height0if*mVfactor),              ( gint16)(height1if*mVfactor),         11);  // 10,11: height in mV to 16bit (later 14bit on FPGA)
       
        // offset/bias base lines pre/post
        rp_spmc_module_config_int16_int16   (MODULE_SETUP,            ( gint16)(bias0*mVfactor),        ( gint16)(bias1*mVfactor),            12);   // 12,13: B height in mV to 16bit (later 14bit on FPGA)
        rp_spmc_module_config_int16_int16   (PULSE_FORMER_WH_ADDRESS, ( gint16)(bias0*mVfactor),        ( gint16)(bias1*mVfactor),            13);   // 14,15: B height in mV to 16bit (later 14bit on FPGA) ** WRITE
}


/*
        OPERATION::
        start   <= operation[0:0]; // Trigger start for single shot
        init    <= operation[7:4]; // reset/reinit
        shr_dec_data <= operation[31:8]; // DEC data shr for ch1..4

 */

// buffers
int                rp_pactransport_opmode    =0;
int                rp_pactransport_chansel   =0;
int                rp_pactransport_decimation=0;
int                rp_pactransport_shr_dec   =0;
int                rp_pactransport_nsamples  =0;
unsigned long long rp_pactransport_fcenter   =0;

// Configure BRAM Data Transport Mode
void rp_PAC_transport_set_control (int control){
        if (verbose > 2) fprintf(stderr, "##Configure TRANSPORT: SET CONTROL %d", control);
        
        if (control >= 0){
                rp_pactransport_opmode = control;
                if (verbose == 1) fprintf(stderr, "TR: RESET: %s, START: %s, MODE: %s\n",
                                          control & PACPLL_CFG_TRANSPORT_RESET ? "SET  " : "READY",
                                          control & PACPLL_CFG_TRANSPORT_START ? "GO" : "ARMED",
                                          control & PACPLL_CFG_TRANSPORT_SINGLE ? "SINGLE":"LOOP");

                // udpate control bits in opmode 
                config_transport (rp_pactransport_chansel, rp_pactransport_decimation, rp_pactransport_nsamples, rp_pactransport_opmode,  rp_pactransport_shr_dec,  rp_pactransport_fcenter);
        }
}

void rp_PAC_configure_transport (int control, int shr_dec_data, int nsamples, int decimation, int channel_select, double center){
        rp_pactransport_shr_dec = channel_select > 7 ? 0 : ((shr_dec_data >= 0 && shr_dec_data <= 24) ? shr_dec_data : 0); // keep memory, do not shift DBG data
        if (verbose == 1) fprintf(stderr, "##Configure TRANSPORT: dec=%d, shr=%d, CHM=%d N=%d\n", decimation, shr_dec_data, channel_select, nsamples);
        
        if (control >= 0) // update ?
                rp_pactransport_opmode     = control;

        if (nsamples > 0) // update?
                rp_pactransport_nsamples = nsamples;

        rp_pactransport_chansel    = channel_select;
        rp_pactransport_decimation = decimation;
        rp_pactransport_fcenter    = (unsigned long long)round (dds_phaseinc (center)); // => 44bit phase f0 center ref for delta_freq calculation on FPGA
        
        config_transport (rp_pactransport_chansel, rp_pactransport_decimation, rp_pactransport_nsamples, rp_pactransport_opmode,  rp_pactransport_shr_dec,  rp_pactransport_fcenter);
}

void rp_PAC_start_transport (int control_mode, int nsamples, int tr_mode){
        
        // RESET TRANSPORT
        rp_PAC_configure_transport (PACPLL_CFG_TRANSPORT_RESET,
                                    SHR_DEC_DATA.Value (),
                                    nsamples,
                                    TRANSPORT_DECIMATION.Value (),
                                    tr_mode, // CHANNEL COMBINATION MODE
                                    FREQUENCY_CENTER.Value()
                                    );
        
        if (control_mode == PACPLL_CFG_TRANSPORT_RESET)
                return;
        
        // START TRANSPORT IN SINGLE or LOOP MODE
        rp_PAC_transport_set_control (PACPLL_CFG_TRANSPORT_START | control_mode);
}

// BRAM write completed (single shot mode)?
int bram_ready(){
        return read_gpio_reg_int32 (6,1) & 0x8000 ? 1:0; // BRAM write completed (finshed flag set)
}

int bram_status(int bram_status[3]){
        int x12 = read_gpio_reg_int32 (6,1); // GPIO X12 : assign writeposition = { decimate_count[15:0], finished, BR_wpos[14:0] };
        bram_status[0] = x12 & 0x7fff;       // GPIO X12 : Transport WPos [15bit]
        bram_status[1] = (x12>>16) & 0xffff; // GPIO X12 : Transport Sample Count
        bram_status[2] = x12 & 0x8000 ? 1:0;      // GPIO X12 : Transport Finished
        return bram_status[2]; // finished (finished and run set)
}


/*
// ---------- NOT IMPLEMENTED/NO ROOM ---------------------
// Configure transport tau: time const or high speed IIR filter stages

void rp_PAC_set_tau_transport (double tau_dfreq, double tau_phase, double tau_exec, double tau_ampl){

#ifdef REMAP_TO_OLD_FPGA_VERSION
        return;
#endif
        //if (verbose == 1) fprintf(stderr, "PAC TAU TRANSPORT DBG: dFreq tau = %g ms ->  %d\n", tau_dfreq, (int)(Q32/ADC_SAMPLING_RATE/(1e-3*tau_dfreq)));
        if (tau_dfreq > 0.)
                set_gpio_cfgreg_uint32 (PACPLL_CFG_TRANSPORT_TAU_DFREQ, (int)(Q32/ADC_SAMPLING_RATE/(1e-3*tau_dfreq)));
        else
                set_gpio_cfgreg_uint32 (PACPLL_CFG_TRANSPORT_TAU_DFREQ, 0xffffffff);
        if (tau_phase > 0.)
                set_gpio_cfgreg_uint32 (PACPLL_CFG_TRANSPORT_TAU_PHASE, (int)(Q32/ADC_SAMPLING_RATE/(1e-3*tau_phase)));
        else
                set_gpio_cfgreg_uint32 (PACPLL_CFG_TRANSPORT_TAU_PHASE, 0xffffffff);
        if (tau_exec > 0.)
                set_gpio_cfgreg_uint32 (PACPLL_CFG_TRANSPORT_TAU_EXEC,  (int)(Q32/ADC_SAMPLING_RATE/(1e-3*tau_exec)));
        else
                set_gpio_cfgreg_uint32 (PACPLL_CFG_TRANSPORT_TAU_EXEC, 0xffffffff);
        if (tau_ampl > 0.)
                set_gpio_cfgreg_uint32 (PACPLL_CFG_TRANSPORT_TAU_AMPL,  (int)(Q32/ADC_SAMPLING_RATE/(1e-3*tau_ampl)));
        else
                set_gpio_cfgreg_uint32 (PACPLL_CFG_TRANSPORT_TAU_AMPL, 0xffffffff);
}
*/

// Phase Controller
// CONTROL[75] OUT[44] : [75-1-1:75-44]=43+1   m[24]  x  c[32]  = 56 M: 24{Q32},  P: 44{Q14}
 void rp_PAC_set_dfreq_controller (double setpoint, double cp, double ci, double upper, double lower, double reset_value, int enable){
        if (verbose > 2) fprintf(stderr, "##Configure Controller: set= %g  Q22: %d    cp=%g ci=%g upper=%g lower=%g\n", setpoint, (int)(Q22 * setpoint), cp, ci, upper, lower); 

        /*
        double cp = 20. * log10 (1.6575e-5  * pll.auto_set_BW_Phase);
        double ci = 20. * log10 (1.7357e-10 * pll.auto_set_BW_Phase*pll.auto_set_BW_Phase);

        write_pll_variable32 (dsp_pll.icoef_Phase, pll.signum_ci_Phase * CPN(29)*pow (10.,pll.ci_gain_Phase/20.));
        // = ISign * CPN(29)*pow(10.,Igain/20.);
	
        write_pll_variable32 (dsp_pll.pcoef_Phase, pll.signum_cp_Phase * CPN(29)*pow (10.,pll.cp_gain_Phase/20.));
        // = PSign * CPN(29)*pow(10.,Pgain/20.);
        */

        config_controller (DFREQ_CONTROLLER_ADDRESS,
                           (int) round (dds_phaseinc (setpoint)),
                           (int) round (QDFCOEF * cp),
                           (int) round (QDFCOEF * ci),
                           (long long) round (Q31*upper/10.0),   // *** FIX ME 10V -> 5V ??
                           (long long) round (Q31*lower/10.0));
        config_controller_m (DFREQ_CONTROLLER_M_ADDRESS, (long long) round(Q31*reset_value/10.0), enable ? 1:0);

        //set_gpio_cfgreg_int32 (PACPLL_CFG_DFREQ_CONTROLLER + PACPLL_CFG_SET,   (long long)(round(dds_phaseinc (setpoint)))); // 32bit dFreq (Full range is Q44)
        //set_gpio_cfgreg_int32 (PACPLL_CFG_DFREQ_CONTROLLER + PACPLL_CFG_CP,    (long long)(QDFCOEF * cp)); // {32}
        //set_gpio_cfgreg_int32 (PACPLL_CFG_DFREQ_CONTROLLER + PACPLL_CFG_CI,    (long long)(QDFCOEF * ci));
        //set_gpio_cfgreg_int32 (PACPLL_CFG_DFREQ_CONTROLLER + PACPLL_CFG_UPPER, (long long)round (Q31*upper/10.0)); // => 15.16 32Q16 Control (Bias, Z, ...) @ +/-10V range in Q31
        //set_gpio_cfgreg_int32 (PACPLL_CFG_DFREQ_CONTROLLER + PACPLL_CFG_LOWER, (long long)round (Q31*lower/10.0));
}



// new GPIO READING THREAD with FIR
void *thread_gpio_reading_FIR(void *arg) {
        unsigned long x8,x9;
        int i,j,x;
	int ch;

	//fprintf(stderr, "thread_gpio_reading_FIR started\n");
	
        for (j=0; j<GPIO_FIR_LEN; j++){
                gpio_reading_FIRV_buffer[GPIO_READING_DDS_X8][j] = 0;
                gpio_reading_FIRV_buffer[GPIO_READING_DDS_X9][j] = 0;
        }
        for (i=0; i<MAX_FIR_VALUES; i++){
                gpio_reading_FIRV_vector[i] = 0.0;
                for (j=0; j<GPIO_FIR_LEN; j++)
                        gpio_reading_FIRV_buffer[i][j] = 0;
        }
	gpio_reading_FIRV_vector_CH3_mapping = gpio_reading_FIRV_vector_CH4_mapping = gpio_reading_FIRV_vector_CH5_mapping = 0.0; 
	for (j=0; j<GPIO_FIR_LEN; j++)
	  gpio_reading_FIRV_buffer_CH3_mapping[j] = gpio_reading_FIRV_buffer_CH4_mapping[j] = gpio_reading_FIRV_buffer_CH5_mapping[j] = 0.0;

        for(j=0; gpio_reading_control; ){

                pthread_mutex_lock (&gpio_reading_mutexsum);

                x = read_gpio_reg_int32 (1,0); // GPIO X1 : LMS A (cfg + 0x1000)
                gpio_reading_FIRV_vector[GPIO_READING_LMS_A] -= (double)gpio_reading_FIRV_buffer[GPIO_READING_LMS_A][j];
                gpio_reading_FIRV_vector[GPIO_READING_LMS_A] += (double)x;
                gpio_reading_FIRV_buffer[GPIO_READING_LMS_A][j] = x;

                x = read_gpio_reg_int32 (1,1); // GPIO X2 : LMS B (cfg + 0x1008)
                gpio_reading_FIRV_vector[GPIO_READING_LMS_B] -= (double)gpio_reading_FIRV_buffer[GPIO_READING_LMS_B][j];
                gpio_reading_FIRV_vector[GPIO_READING_LMS_B] += (double)x;
                gpio_reading_FIRV_buffer[GPIO_READING_LMS_B][j] = x;

                //x = read_gpio_reg_int32 (3,0); // GPIO X5 : MDC
                //gpio_reading_FIRV_vector[GPIO_READING_OFSET] -= (double)gpio_reading_FIRV_buffer[GPIO_READING_OFSET][j];
                //gpio_reading_FIRV_vector[GPIO_READING_OFSET] += (double)x;
                //gpio_reading_FIRV_buffer[GPIO_READING_OFSET][j] = x;
                
                x = read_gpio_reg_int32 (2,1); // GPIO X4 : CORDIC SQRT (AM2=A^2+B^2) = Amplitude Monitor
                gpio_reading_FIRV_vector[GPIO_READING_AMPL] -= (double)gpio_reading_FIRV_buffer[GPIO_READING_AMPL][j];
                gpio_reading_FIRV_vector[GPIO_READING_AMPL] += (double)x;
                gpio_reading_FIRV_buffer[GPIO_READING_AMPL][j] = x;

                x = read_gpio_reg_int32 (5,1); // GPIO X10: CORDIC ATAN(X/Y) = Phase Monitor
                gpio_reading_FIRV_vector[GPIO_READING_PHASE] -= (double)gpio_reading_FIRV_buffer[GPIO_READING_PHASE][j];
                gpio_reading_FIRV_vector[GPIO_READING_PHASE] += (double)x;
                gpio_reading_FIRV_buffer[GPIO_READING_PHASE][j] = x;

                x = read_gpio_reg_int32 (4,0); // GPIO X7 : Exec Ampl Control Signal (signed)
                gpio_reading_FIRV_vector[GPIO_READING_EXEC] -= (double)gpio_reading_FIRV_buffer[GPIO_READING_EXEC][j];
                gpio_reading_FIRV_vector[GPIO_READING_EXEC] += (double)x;
                gpio_reading_FIRV_buffer[GPIO_READING_EXEC][j] = x;
                
                x8 = read_gpio_reg_uint32 (4,1); // GPIO X8 : DDS Phase Inc (Freq.) upper 32 bits of 44 (unsigned)
                x9 = read_gpio_reg_uint32 (5,0); // GPIO X9 : DDS Phase Inc (Freq.) lower 32 bits of 44 (unsigned)
                gpio_reading_FIRV_vector[GPIO_READING_DDS_FREQ] -= dds_phaseinc_to_freq(  ((unsigned long long)gpio_reading_FIRV_buffer_DDS[GPIO_READING_DDS_X8][j]<<(44-32))
                                                                                        + ((unsigned long long)gpio_reading_FIRV_buffer_DDS[GPIO_READING_DDS_X9][j]>>(64-44)));
                gpio_reading_FIRV_vector[GPIO_READING_DDS_FREQ] += dds_phaseinc_to_freq(  ((unsigned long long)x8<<(44-32))
                                                                                        + ((unsigned long long)x9>>(64-44)));
                gpio_reading_FIRV_buffer_DDS[GPIO_READING_DDS_X8][j] = x8;
                gpio_reading_FIRV_buffer_DDS[GPIO_READING_DDS_X9][j] = x9;

                x = read_gpio_reg_int32 (6,0); // GPIO X11 : dFreq
                gpio_reading_FIRV_vector[GPIO_READING_DFREQ] -= dds_phaseinc_rel_to_freq((long long)(gpio_reading_FIRV_buffer[GPIO_READING_DFREQ][j]));
                gpio_reading_FIRV_vector[GPIO_READING_DFREQ] += dds_phaseinc_rel_to_freq((long long)(x));
                gpio_reading_FIRV_buffer[GPIO_READING_DFREQ][j] = x;

                x = read_gpio_reg_int32 (7,0); // GPIO X13: control dFreq value
                gpio_reading_FIRV_vector[GPIO_READING_CONTROL_DFREQ] -= (double)gpio_reading_FIRV_buffer[GPIO_READING_CONTROL_DFREQ][j];
                gpio_reading_FIRV_vector[GPIO_READING_CONTROL_DFREQ] += (double)x;
                gpio_reading_FIRV_buffer[GPIO_READING_CONTROL_DFREQ][j] = x;


                ch = TRANSPORT_CH3.Value ();
                x = read_gpio_reg_int32 ((ch>>1)+1,ch&1); // GPIO (0,0) = (x1, x2), (0,1), (1,0), (1,1), ... (9,0), (9,1) = (x19,x20)
                gpio_reading_FIRV_vector_CH3_mapping -= (double)gpio_reading_FIRV_buffer_CH3_mapping[j];
                gpio_reading_FIRV_vector_CH3_mapping += (double)x;
                gpio_reading_FIRV_buffer_CH3_mapping[j] = x;

		ch = TRANSPORT_CH4.Value ();
                x = read_gpio_reg_int32 ((ch>>1)+1,ch&1); // GPIO (0,0) = (x1, x2), (0,1), (1,0), (1,1), ... (9,0), (9,1) = (x19,x20)
                gpio_reading_FIRV_vector_CH4_mapping -= (double)gpio_reading_FIRV_buffer_CH4_mapping[j];
                gpio_reading_FIRV_vector_CH4_mapping += (double)x;
                gpio_reading_FIRV_buffer_CH4_mapping[j] = x;

		ch = TRANSPORT_CH5.Value ();
                x = read_gpio_reg_int32 ((ch>>1)+1,ch&1); // GPIO (0,0) = (x1, x2), (0,1), (1,0), (1,1), ... (9,0), (9,1) = (x19,x20)
                gpio_reading_FIRV_vector_CH5_mapping -= (double)gpio_reading_FIRV_buffer_CH5_mapping[j];
                gpio_reading_FIRV_vector_CH5_mapping += (double)x;
                gpio_reading_FIRV_buffer_CH5_mapping[j] = x;


		
                pthread_mutex_unlock (&gpio_reading_mutexsum);

                ++j;
                j &= 0x3ff; // 1024
                usleep (1000); // updated every 1ms
        }
        return NULL;
}

/*
 * Get Single Direct FPGA Reading via GPIO
 * ========================================
 * reading_vector[0] := LMS Amplitude estimation (from A,B)
 * reading_vector[1] := LMS Phase estimation (from A,B)
 * reading_vector[2] := LMS A
 * reading_vector[3] := LMS B
 * reading_vector[4] := FPGA CORDIC Amplitude Monitor
 * reading_vector[5] := FPGA CORDIC Phase Monitor
 * reading_vector[6] := x5 M-DC_LMS
 * reading_vector[7] := x6
 * reading_vector[8] := x7 Exec Amp Mon
 * reading_vector[9] := DDS Freq
 * reading_vector[10]:= x3 Monitor (In0 Signal, LMS inpu)
 * reading_vector[11]:= x3
 * reading_vector[12]:= x11
 * reading_vector[13]:= x12
 */

// compat func
void rp_PAC_get_single_reading_FIR (double reading_vector[READING_MAX_VALUES]){
        double a,b,v,p;
        // LMS Detector Readings and double precision conversions
        a = gpio_reading_FIRV_vector[GPIO_READING_LMS_A]/GPIO_FIR_LEN / QLMS;
        b = gpio_reading_FIRV_vector[GPIO_READING_LMS_B]/GPIO_FIR_LEN / QLMS;
        // hi level atan2 and sqrt
        v=sqrt (a*a+b*b);
        p=atan2 ((a-b),(a+b));
        reading_vector[0] = v*1000.; // LMS Amplitude (Volume) in mVolts (from A,B)
        reading_vector[1] = p/M_PI*180.; // LMS Phase (from A,B)
        reading_vector[2] = a; // LMS A (Real)
        reading_vector[3] = b; // LMS B (Imag)

        // FPGA CORDIC based convertions
        reading_vector[4] = gpio_reading_FIRV_vector[GPIO_READING_AMPL]/GPIO_FIR_LEN * 1000./QCORDICSQRT; //reading_vector[4]; // LMS Amplitude (Volume) in mVolts (from A,B)
        reading_vector[5] = gpio_reading_FIRV_vector[GPIO_READING_PHASE]/GPIO_FIR_LEN * 180./QCORDICATAN/M_PI;   //reading_vector[5]; // LMS Phase (from A,B) in deg

        // N/A
        //reading_vector[6] = x5*1000.;  // X5
        //reading_vector[7] = x6;  // X4
        //reading_vector[10] = x3; // M (LMS input Signal)
        //reading_vector[11] = x3; // M1 (LMS input Signal-DC), tests...

        // Controllers
        reading_vector[8] = gpio_reading_FIRV_vector[GPIO_READING_EXEC]/GPIO_FIR_LEN  * 1000/QEXEC;
        reading_vector[9] = gpio_reading_FIRV_vector[GPIO_READING_DDS_FREQ]/GPIO_FIR_LEN;
        reading_vector[12] = gpio_reading_FIRV_vector[GPIO_READING_DFREQ]/GPIO_FIR_LEN;
        reading_vector[13] = gpio_reading_FIRV_vector[GPIO_READING_CONTROL_DFREQ]/GPIO_FIR_LEN * 10000/Q31;


        SIGNAL_GPIOX[0] = read_gpio_reg_int32 (1,0); // GPIO X1 : LMS A (cfg + 0x1000)
        SIGNAL_GPIOX[1] = read_gpio_reg_int32 (1,1); // GPIO X2 : LMS B (cfg + 0x1008)
        SIGNAL_GPIOX[2] = read_gpio_reg_int32 (2,0); // GPIO X3 : LMS DBG1 :: M (LMS input Signal) (cfg + 0x2000) ===> used for DC OFFSET calculation
        SIGNAL_GPIOX[3] = read_gpio_reg_int32 (2,1); // GPIO X4 : CORDIC SQRT (AM2=A^2+B^2) = Amplitude Monitor
        SIGNAL_GPIOX[4] = read_gpio_reg_int32 (3,0); // GPIO X5 : MDC
        SIGNAL_GPIOX[5] = read_gpio_reg_int32 (3,1); // GPIO X6 : XXX
        SIGNAL_GPIOX[6] = read_gpio_reg_int32 (4,0); // GPIO X7 : Exec Ampl Control Signal (signed)
        SIGNAL_GPIOX[7] = read_gpio_reg_uint32 (4,1); // GPIO X8 : DDS Phase Inc (Freq.) upper 32 bits of 44 (signed)
        SIGNAL_GPIOX[8] = read_gpio_reg_uint32 (5,0); // GPIO X9 : DDS Phase Inc (Freq.) lower 32 bits of 44 (signed)
        SIGNAL_GPIOX[9] = read_gpio_reg_int32 (5,1); // GPIO X10: CORDIC ATAN(X/Y) = Phase Monitor
        SIGNAL_GPIOX[10]= read_gpio_reg_int32 (6,0); // GPIO X11 : dFreq
        SIGNAL_GPIOX[11]= read_gpio_reg_int32 (6,1); // GPIO X12 BRAM write position
        SIGNAL_GPIOX[12]= read_gpio_reg_int32 (7,0); // GPIO X13: control dFreq
        SIGNAL_GPIOX[13]= read_gpio_reg_int32 (7,1); // GPIO X14
        //SIGNAL_GPIOX[14] = read_gpio_reg_int32 (8,0); // GPIO X15
        //SIGNAL_GPIOX[15] = read_gpio_reg_int32 (8,1); // GPIO X16

}

#if 0
// Get all GPIO mapped data / system state snapshot
void rp_PAC_get_single_reading (double reading_vector[READING_MAX_VALUES]){
        int x,y,xx7;
        unsigned long xx8, xx9, uix;
        double a,b,v,p,x3,x4,x5,x6,x7,x8,x9,x10,qca,pfpga,x11,x12;
        
        SIGNAL_GPIOX[0] = x = read_gpio_reg_int32 (1,0); // GPIO X1 : LMS A (cfg + 0x1000)
        a=(double)x / QLMS;
        SIGNAL_GPIOX[1] = x = read_gpio_reg_int32 (1,1); // GPIO X2 : LMS B (cfg + 0x1008)
        b=(double)x / QLMS;
        v=sqrt (a*a+b*b);
        p=atan2 ((a-b),(a+b));
        
        SIGNAL_GPIOX[2] = x = read_gpio_reg_int32 (2,0); // GPIO X3 : LMS DBG1 :: M (LMS input Signal) (cfg + 0x2000) ===> used for DC OFFSET calculation
        x3=(double)x / QLMS;
        SIGNAL_GPIOX[3] = x = read_gpio_reg_int32 (2,1); // GPIO X4 : CORDIC SQRT (AM2=A^2+B^2) = Amplitude Monitor
        x4=(double)x / QCORDICSQRT;
        //x4=(double)x / QEXEC;
        SIGNAL_GPIOX[4] = x = read_gpio_reg_int32 (3,0); // GPIO X5 : MDC
        x5=(double)x / QLMS; // MDC    /QCORDICSQRTFIR;
        SIGNAL_GPIOX[5] = x = read_gpio_reg_int32 (3,1); // GPIO X6 : XXX
        x6=(double)x / QCORDICATANFIR;
        SIGNAL_GPIOX[6] = xx7 = x = read_gpio_reg_int32 (4,0); // GPIO X7 : Exec Ampl Control Signal (signed)
        x7=(double)x / QEXEC;

        SIGNAL_GPIOX[7] = xx8 = x = read_gpio_reg_uint32 (4,1); // GPIO X8 : DDS Phase Inc (Freq.) upper 32 bits of 44 (signed)
        SIGNAL_GPIOX[8] = xx9 = x = read_gpio_reg_uint32 (5,0); // GPIO X9 : DDS Phase Inc (Freq.) lower 32 bits of 44 (signed)
        x9=(double)x / QLMS;

        SIGNAL_GPIOX[9] = x = read_gpio_reg_int32 (5,1); // GPIO X10: CORDIC ATAN(X/Y) = Phase Monitor
        x10 = (double)x / QCORDICATAN; // ATAN 24bit 3Q21 

        SIGNAL_GPIOX[10] = x = read_gpio_reg_int32 (6,0); // GPIO X11 : dFreq
        x11=(double)(x);

        SIGNAL_GPIOX[11] = x = read_gpio_reg_int32 (6,1); // GPIO X12 BRAM write position
        x12=(double)(x);

        SIGNAL_GPIOX[12] = read_gpio_reg_int32 (7,0); // GPIO X13: control dFreq
        SIGNAL_GPIOX[13] = read_gpio_reg_int32 (7,1); // GPIO X14
        //SIGNAL_GPIOX[14] = read_gpio_reg_int32 (8,0); // GPIO X15
        //SIGNAL_GPIOX[15] = read_gpio_reg_int32 (8,1); // GPIO X16

        
        // LMS Detector Readings and double precision conversions
        reading_vector[0] = v*1000.; // LMS Amplitude (Volume) in mVolts (from A,B)
        reading_vector[1] = p/M_PI*180.; // LMS Phase (from A,B)
        reading_vector[2] = a; // LMS A (Real)
        reading_vector[3] = b; // LMS B (Imag)

        // FPGA CORDIC based convertions
        reading_vector[4] = x4*1000.;  // FPGA CORDIC (SQRT) Amplitude Monitor in mVolt
        reading_vector[5] = x10/M_PI*180.; // FPGA CORDIC (ATAN) Phase Monitor

        reading_vector[6] = x5*1000.;  // X5
        reading_vector[7] = x6;  // X4

        reading_vector[8] = x7*1000.0;  // Exec Ampl Control Signal (signed) in mV
        reading_vector[9] = dds_phaseinc_to_freq(((unsigned long long)xx8<<(44-32)) + ((unsigned long long)xx9>>(64-44)));  // DDS Phase Inc (Freq.) upper 32 bits of 44 (signed)

        reading_vector[10] = x3; // M (LMS input Signal)
        reading_vector[11] = x3; // M1 (LMS input Signal-DC), tests...

        reading_vector[12] = dds_phaseinc_rel_to_freq((long long)(x11)); // delta Freq
        reading_vector[13] = 10000.*(double)SIGNAL_GPIOX[12]/Q31; // control of dFreq controller

        if (verbose > 2){
                if (verbose == 1) fprintf(stderr, "PAC TRANSPORT DBG: BRAM WritePos=%04X Sample#=%04x Finished=%d\n", x&0x7fff, x>>16, x&0x8000 ? 1:0);
                else if (verbose > 4) fprintf(stderr, "PAC DBG: A:%12f  B:%12f X1:%12f X2:%12f X3:%12f X4:%12f X5:%12f X6:%12f X7:%12f X8:%12f X9:%12f X10:%12f X11:%12f X12:%12f\n", a,b,x3,x4,x5,x6,x7,x8,x9,x10,x11,x12);
                else if (verbose > 3) fprintf(stderr, "PAC READING: Ampl=%10.4f V Phase=%10.4f deg a=%10.4f b=%10.4f  FPGA: %10.4f %10.4f FIR: %10.4f %10.4f  M %10.4f V  pfpga=%10.4f\n", v,p, a,b, x4, x10, x5, x6, x3, pfpga);
        }
}

#endif


