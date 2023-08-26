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
#include "spmc.h"

#define PACPLL_CFG0_OFFSET 0
#define PACPLL_CFG1_OFFSET 32
#define PACPLL_CFG2_OFFSET 64
#define PACPLL_CFG3_OFFSET 96


// CONFIGURATION (CFG) DATA REGISTER 0 [1023:0] x 4 = 4k
// PAC-PLL Control Core

// general control paging (future options)
//#define PACPLL_CFG_PAGE_CONTROL  30   // 32bit wide
//#define PACPLL_CFG_PAGE          31   // 32bit wide


// Controller Core (Servos) Relative Block Offsets:
#define PACPLL_CFG_SET   0
#define PACPLL_CFG_CP    1
#define PACPLL_CFG_CI    2
#define PACPLL_CFG_UPPER 3 // 3,4 64bit
#define PACPLL_CFG_LOWER 5 // 5,6 64bit

// [CFG0]+10 AMPL Controller
// [CFG0]+20 PHASE Controller
// [CFG1]+00 dFREQ Controller   
// +0 Set, +2 CP, +4 CI, +6 UPPER, +8 LOWER

// SPM Z-CONTROL SERVO
#define SPMC_CFG_Z_SERVO_CONTROLLER         (PACPLL_CFG1_OFFSET + 10) // 10:16
#define SPMC_CFG_Z_SERVO_ZSETPOINT          (PACPLL_CFG1_OFFSET + 17) // 17
#define SPMC_CFG_Z_SERVO_LEVEL              (PACPLL_CFG1_OFFSET + 18) // 18
#define SPMC_CFG_Z_SERVO_MODE               (PACPLL_CFG1_OFFSET + 19) // 19: SERVO CONTROL (enable) Bit0, ...

// CFG DATA REGISTER 3 [1023:0]
// SPMControl Core

#define SPMC_BASE                    (PACPLL_CFG3_OFFSET + 0)
#define SPMC_GVP_CONTROL              0 // 0: reset 1: setvec
#define SPMC_GVP_VECTOR_DATA          1..16 // 512 bits (16x32)

#define SPMC_CFG_AD5791_AXIS_DAC     17
#define SPMC_CFG_AD5791_DAC_CONTROL  18 // 0: config mode 1,2,3: axis 4: send

// SPMC Transformations Core
#define SPMC_ROTM_XX             20  // cos(Alpha)
#define SPMC_ROTM_XY             21  // sin(Alpha)
//#define SPMC_ROTM_YX = -XY = -sin(Alpha)
//#define SPMC_ROTM_YY =  XX =  cos(Alpha)
#define SPMC_SLOPE_X             22
#define SPMC_SLOPE_Y             23
#define SPMC_OFFSET_X            24
#define SPMC_OFFSET_Y            25
#define SPMC_OFFSET_Z            26

extern int verbose;


/*
 * RedPitaya A9 FPGA Control and Data Transfer
 * ------------------------------------------------------------
 */

/* RP SPMC FPGA Engine Configuration and Control */

void rp_spmc_set_bias (double bias){
  //if (verbose >= 1) fprintf(stderr, "##Configure: volume= %g V\n", volume); 
        //set_gpio_cfgreg_int32 (PACPLL_CFG_VOLUME_SINE, (int)(Q31 * volume));
}


// Phase Controller
// CONTROL[75] OUT[44] : [75-1-1:75-44]=43+1   m[24]  x  c[32]  = 56 M: 24{Q32},  P: 44{Q14}
void rp_spmc_set_zservo_controller (double setpoint, double cp, double ci, double upper, double lower){
        if (verbose > 2) fprintf(stderr, "##Configure RP SPMC Z-Servo Controller: set= %g  cp=%g ci=%g upper=%g lower=%g\n", setpoint, cp, ci, upper, lower); 

        /*
        //double cp = 20. * log10 (1.6575e-5  * pll.auto_set_BW_Phase);
        //double ci = 20. * log10 (1.7357e-10 * pll.auto_set_BW_Phase*pll.auto_set_BW_Phase);

        //write_pll_variable32 (dsp_pll.icoef_Phase, pll.signum_ci_Phase * CPN(29)*pow (10.,pll.ci_gain_Phase/20.));
        // = ISign * CPN(29)*pow(10.,Igain/20.);
	
        //write_pll_variable32 (dsp_pll.pcoef_Phase, pll.signum_cp_Phase * CPN(29)*pow (10.,pll.cp_gain_Phase/20.));
        // = PSign * CPN(29)*pow(10.,Pgain/20.);
        */

        //set_gpio_cfgreg_int32 (PACPLL_CFG_DFREQ_CONTROLLER + PACPLL_CFG_SET,   (long long)(round(dds_phaseinc (setpoint)))); // 32bit dFreq (Full range is Q44)
        //set_gpio_cfgreg_int32 (PACPLL_CFG_DFREQ_CONTROLLER + PACPLL_CFG_CP,    (long long)(QDFCOEF * cp)); // {32}
        //set_gpio_cfgreg_int32 (PACPLL_CFG_DFREQ_CONTROLLER + PACPLL_CFG_CI,    (long long)(QDFCOEF * ci));
        //set_gpio_cfgreg_int32 (PACPLL_CFG_DFREQ_CONTROLLER + PACPLL_CFG_UPPER, (long long)round (Q31*upper/10.0)); // => 15.16 32Q16 Control (Bias, Z, ...) @ +/-10V range in Q31
        //set_gpio_cfgreg_int32 (PACPLL_CFG_DFREQ_CONTROLLER + PACPLL_CFG_LOWER, (long long)round (Q31*lower/10.0));
}


