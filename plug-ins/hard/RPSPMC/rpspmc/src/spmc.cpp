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

#include <time.h>
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
#include <fstream>
#include <iostream>
#include <set>
#include <streambuf>
#include <string>

#include <DataManager.h>
#include <CustomParameters.h>

#include "fpga_cfg.h"
#include "spmc.h"

#include "spmc_stream_server.h"

#include "AD5791_lib.cpp"

#include "spmc_module_config_utils.h"


//extern int verbose;
//extern int stream_debug_flags;

extern CIntParameter     SPMC_GVP_STATUS;
extern CDoubleParameter  SPMC_BIAS_MONITOR;
extern CDoubleParameter  SPMC_BIAS_REG_MONITOR;
extern CDoubleParameter  SPMC_BIAS_SET_MONITOR;
extern CDoubleParameter  SPMC_GVPU_MONITOR;
extern CDoubleParameter  SPMC_GVPA_MONITOR;
extern CDoubleParameter  SPMC_GVPB_MONITOR;
extern CDoubleParameter  SPMC_GVPAMC_MONITOR;
extern CDoubleParameter  SPMC_GVPFMC_MONITOR;
extern CIntParameter     SPMC_MUX_MONITOR;

extern CDoubleParameter  SPMC_X_MONITOR;
extern CDoubleParameter  SPMC_Y_MONITOR;
extern CDoubleParameter  SPMC_Z_MONITOR;

extern CDoubleParameter  SPMC_XS_MONITOR;
extern CDoubleParameter  SPMC_YS_MONITOR;
extern CDoubleParameter  SPMC_ZS_MONITOR;

extern CDoubleParameter  SPMC_X0_MONITOR;
extern CDoubleParameter  SPMC_Y0_MONITOR;
extern CDoubleParameter  SPMC_Z0_MONITOR;

extern CDoubleParameter  SPMC_SIGNAL_MONITOR;
extern CDoubleParameter  SPMC_AD463X_CH1_MONITOR;
extern CDoubleParameter  SPMC_AD463X_CH2_MONITOR;

extern CDoubleParameter  SPMC_SC_LCK_BQ_COEF_B0;
extern CDoubleParameter  SPMC_SC_LCK_BQ_COEF_B1;
extern CDoubleParameter  SPMC_SC_LCK_BQ_COEF_B2;
extern CDoubleParameter  SPMC_SC_LCK_BQ_COEF_A0;
extern CDoubleParameter  SPMC_SC_LCK_BQ_COEF_A1;
extern CDoubleParameter  SPMC_SC_LCK_BQ_COEF_A2;

extern CDoubleParameter  SPMC_SC_LCK_RF_FREQUENCY;

extern CDoubleParameter  SPMC_UPTIME_SECONDS;

extern CFloatSignal SIGNAL_XYZ_METER;


extern int stream_server_control;
extern spmc_stream_server spmc_stream_server_instance;

int xyz_meter_reading_control=1;
pthread_attr_t xyz_meter_reading_attr;
pthread_t xyz_meter_reading_thread;


int x0_buf = 0;
int y0_buf = 0;
int z0_buf = 0;
int bias_buf = 0;


extern std::stringstream rpspmc_init_info;


/*
 * RedPitaya A9 FPGA Control and Data Transfer
 * ------------------------------------------------------------
 */

/* RP SPMC FPGA Engine Configuration and Control */

inline double rpspmc_to_volts (int value){ return SPMC_AD5791_REFV*(double)value / Q31; }
inline int volts_to_rpspmc (double volts){ return (int)round(Q31*volts/SPMC_AD5791_REFV); }

inline double rpspmc_CONTROL_SELECT_ZS_to_volts (int source_id, int value){ return (source_id == 1 ? SPMC_AD463_REFV : SPMC_IN01_REFV)*(double)value*256. / Q31; } // SQ24.8 internally in Z_SERVO rtl
inline double volts_to_rpspmc_CONTROL_SELECT_ZS (int source_id, double volts){ return round(Q31*volts/(source_id == 1 ? SPMC_AD463_REFV : SPMC_IN01_REFV)); }

// f(x)=ax, x = f(x)/a -> x = f(x) / f(1)

// initialize/reset all AD5791 channels and set SPMC to streaming
void rp_spmc_AD5791_init (){
        fprintf(stderr, "##rp_spmc_AD5791_init\n");

        rp_spmc_set_rotation (0.0, -1.0);

        // power up one by one
        ad5791_setup(0,0);
        ad5791_setup(1,0);
        ad5791_setup(2,0);
        ad5791_setup(3,0);
        ad5791_setup(4,0);
        ad5791_setup(5,0);

        usleep(10000);

        // send 0V
        ad5791_prepare_dac_value (0, 0.0);
        ad5791_prepare_dac_value (1, 0.0);
        ad5791_prepare_dac_value (2, 0.0);
        ad5791_prepare_dac_value (3, 0.0);
        ad5791_prepare_dac_value (4, 0.0);
        ad5791_prepare_dac_value (5, 0.0);

        ad5791_set_dac_value (0, 0.0);
        ad5791_set_dac_value (1, 0.0);
        ad5791_set_dac_value (2, 0.0);
        ad5791_set_dac_value (3, 0.0);
        ad5791_set_dac_value (4, 0.0);
        ad5791_set_dac_value (5, 0.0);

        usleep(10000);

        rp_spmc_AD5791_set_stream_mode (); // enable streaming for AD5791 DAC's from FPGA now!

        rpspmc_init_info << "\n** RPSPMC AD5791 PMODs INIT REPORT (blind) **\n";
        rpspmc_init_info << "RPSPMC: AD5791 PMODs 1..6 (XYZUAB) init completed. FPGA AXI streaming enabled.\n";

}

ad463x_dev* rp_spmc_AD463x_init (){
        if (verbose > 1) fprintf(stderr, "##rp_spmc_AD463x_init\n");

        ad463x_dev *dev=NULL;
        ad463x_init_param init_param;
        memset (&init_param, 0, sizeof(init_param));
        
        init_param.device_id   = ID_AD4630_24;
        init_param.clock_mode  = AD463X_SPI_COMPATIBLE_MODE;
        init_param.lane_mode   = AD463X_ONE_LANE_PER_CH;
        init_param.output_mode = AD463X_24_DIFF;
        init_param.data_rate   = AD463X_SDR_MODE; // AD463X_DDR_MODE
        init_param.spi_clock_divider = 3; // 3 should do, 10 works for testing

        rpspmc_init_info << "\n** RPSPMC AD4630 INIT REPORT **\n";
        rpspmc_init_info << "RPSPMC: AD4630-24 init: SPI, ONE LANE, 24_DIFF, SDR\n";

        int ret=ad463x_init(&dev, &init_param);

        rp_spmc_AD463x_set_stream_mode (dev);

        rpspmc_init_info << "RPSPMC: AD4630-24 init completed: " << (ret != 0 ? "INIT ERROR":"INIT OK") << "\n"
                         << "     ...FPGA internal streaming enabled.\n";
 
        return dev;
}

void rp_spmc_AD463x_test (struct ad463x_dev *dev){
	uint8_t data = 0;
	uint8_t pat[4];
	uint32_t ch0, ch1;

        if (verbose > 1) fprintf(stderr, "##rp_spmc_AD463x_test *** SPI / AD463x TESTING ***\n");

	ad463x_enter_config_mode (dev); // added

        rpspmc_init_info << "RPSPMC: AD4630-24 testing:\n";

	fprintf(stderr,"AD463x WRITE TEST TO SCRATCH PAD REG:\n");
        rpspmc_init_info << "     SPI SCRATCH PAD READ WRITE TEST:";
	ad463x_spi_reg_write(dev, AD463X_REG_SCRATCH_PAD, AD463x_TEST_DATA);
	ad463x_spi_reg_read(dev, AD463X_REG_SCRATCH_PAD, &data);

	if (data != AD463x_TEST_DATA) {
                // pr_err("Test Data read failed.\n");
                fprintf(stderr,"AD463x Test Data Read failed: %08x expected: %08x\n", data, AD463x_TEST_DATA);
                rpspmc_init_info << "AD463x Test Data Read failed: " << std::hex << std::setw(8) << data << " expected: " << AD463x_TEST_DATA << "\n";
	} else {
                fprintf(stderr,"AD463x Test Data Read: passed\n");
                rpspmc_init_info << "AD463x Test Data Read passed\n";
                fprintf(stderr,"AD463x Test Data Read/Write extensive run...");
                int errcount=0;
                for (uint8_t x=0; ; x++){
                        ad463x_spi_reg_write(dev, AD463X_REG_SCRATCH_PAD, x);
                        ad463x_spi_reg_read(dev, AD463X_REG_SCRATCH_PAD, &data);
                        if (data != x) {
                                fprintf(stderr,"Scratch R/W fail for %02x expected: %02x\n", data, x);
                                errcount++;
                        }
                        if (x==0xff) break;
                }
                if (errcount > 0) {
                        fprintf(stderr,"Scratch R/W extensive test fail # %d\n", errcount);
                        rpspmc_init_info << "AD463x R/W extensive test failed. Err count: " << errcount << "\n";
                }
                else {
                        fprintf(stderr,"Scratch R/W extensive test passed.\n");
                        rpspmc_init_info << "Scratch R/W extensive test passed.\n";
                }
        }
	ad463x_spi_reg_read(dev, AD463X_REG_PAT0, &pat[0]);
	ad463x_spi_reg_read(dev, AD463X_REG_PAT1, &pat[1]);
	ad463x_spi_reg_read(dev, AD463X_REG_PAT2, &pat[2]);
	ad463x_spi_reg_read(dev, AD463X_REG_PAT3, &pat[3]);
	fprintf(stderr,"AD463x AD463X_REG_TEST_PAT: %02x %02x %02x %02x\n", pat[3], pat[2], pat[1], pat[0]);

	fprintf(stderr,"AD463x AD463X_REG_TEST_PAT ENABLED NOW\n");
	ad463x_spi_reg_write_masked(dev, AD463X_REG_MODES, AD463X_OUT_DATA_MODE_MSK, AD463X_32_PATTERN);

        ad463x_exit_reg_cfg_mode(dev);  // exit CFG mode to do test conversion reads
        
	fprintf(stderr,"AD463x 4 TEST PAT READINGS, 24BIT mode:\n");
	{
	  uint32_t ch0, ch1;
	  ad463x_read_single_sample (dev, &ch0, &ch1, 0, 1);
	  ad463x_read_single_sample (dev, &ch0, &ch1, 0, 1);
	  ad463x_read_single_sample (dev, &ch0, &ch1, 0, 1);
	  ad463x_read_single_sample (dev, &ch0, &ch1, 0, 1);
	}

	fprintf(stderr,"AD463x 4 TEST PAT READINGS, 32BIT mode:\n");
	{
	  uint32_t ch0, ch1;
	  ad463x_read_single_sample (dev, &ch0, &ch1, 0, 1, 32);
	  ad463x_read_single_sample (dev, &ch0, &ch1, 0, 1, 32);
	  ad463x_read_single_sample (dev, &ch0, &ch1, 0, 1, 32);
	  ad463x_read_single_sample (dev, &ch0, &ch1, 0, 1, 32);
	}
	
        // ***
	
	fprintf(stderr,"AD463x RE-CONFIGURING MODES FOR NORMAL OPERATION\n");
	ad463x_enter_config_mode (dev); // added
	ad463x_spi_reg_write_masked(dev, AD463X_REG_MODES, AD463X_OUT_DATA_MODE_MSK, dev->output_mode);
	ad463x_spi_reg_write_masked(dev, AD463X_REG_MODES, AD463X_LANE_MODE_MSK, dev->lane_mode);
	ad463x_spi_reg_write_masked(dev, AD463X_REG_MODES, AD463X_CLK_MODE_MSK, dev->clock_mode);
	ad463x_spi_reg_write_masked(dev, AD463X_REG_MODES, AD463X_DDR_MODE_MSK, dev->data_rate);

        ad463x_exit_reg_cfg_mode(dev);

	fprintf(stderr,"AD463x EXIT REGISTER R/W MODE\n");

        if (verbose > 1) fprintf(stderr, "AD463x ** (24_DIFF, ONE_LANE, SDR_MODE) ** TEST READINGS:\n");
        
        ad463x_read_single_sample (dev, &ch0, &ch1, 0, 1);
        ad463x_read_single_sample (dev, &ch0, &ch1, 0, 1);

        rpspmc_init_info << "AD4630-24 test readings:\n";
        for (int i=0; i<10; ++i){
                ad463x_read_single_sample (dev, &ch0, &ch1, 1, 0);
                fprintf (stderr, "%03d: %10d  %10d  \t %10g mV \t %10g mV\n", i, ch0, ch1, 2*5000.*((int32_t)ch0)/(1<<23), 2*5000.0*((int32_t)ch1)/(1<<23));
                rpspmc_init_info << std::dec << std::setw(10) << i << ": "
                                 << ch0 << ", " << ch1 << " => "
                                 << (2*5000.*((int32_t)ch0)/(1<<23)) << "mV, "
                                 << (2*5000.*((int32_t)ch0)/(1<<23)) << "mV\n";
        }

       	fprintf(stderr,"AD463x ENTERING AXI STREAMING OPERATION\n");
        rpspmc_init_info << "AD4630-24: FPGA AXI streaming enabled.\n";

        rp_spmc_AD463x_set_stream_mode (dev);

}




void rp_spmc_AD463x_set_stream_mode (struct ad463x_dev *dev){
	rp_spmc_module_config_uint32 (MODULE_SETUP, SPMC_AD463X_CONFIG_MODE_AXI | SPMC_AD463X_CONFIG_MODE_STREAM, MODULE_START_VECTOR); // DISABLE CONFIG MODE, ENABLE AXI for streaming
	rp_spmc_module_config_uint32 (MODULE_SETUP, 0,                       MODULE_SETUP_VECTOR(SPMC_AD463X_CONFIG_WR_DATA));
	rp_spmc_module_config_uint32 (MODULE_SETUP, dev->spi_clock_divider,  MODULE_SETUP_VECTOR(SPMC_AD463X_CONFIG_N_DECII));
	rp_spmc_module_config_uint32 (SPMC_AD463X_CONTROL_REG, 24,           MODULE_SETUP_VECTOR(SPMC_AD463X_CONFIG_N_BITS));
        
}


// WARNING: FOR TEST/CALIBARTION ONLY, direct DAC write via config mode!!!
void rp_spmc_set_xyzu_DIRECT (double ux, double uy, double uz, double bias){
        if (verbose > 1) fprintf(stderr, "##Configure mode set AD5971 AXIS0,1,2,3 (XYZU) to %g V, %g V, %g V, %g V\n", ux, uy, uz, bias);

        ad5791_prepare_dac_value (0, ux);
        ad5791_prepare_dac_value (1, uy);
        ad5791_prepare_dac_value (2, uz);
        ad5791_set_dac_value     (3, bias);

        if (verbose > 1){
                fprintf(stderr, "Set AD5971 AXIS3 (Bias) to %g V\n", bias);
                fprintf(stderr, "MON-UXYZ: %8g  %8g  %8g  %8g V  STATUS: %08X PASS: %8g V\n",
                        rpspmc_to_volts (read_gpio_reg_int32 (8,0)),
                        rpspmc_to_volts (read_gpio_reg_int32 (8,1)),
                        rpspmc_to_volts (read_gpio_reg_int32 (9,0)),
                        rpspmc_to_volts (read_gpio_reg_int32 (9,1)),
                        read_gpio_reg_int32 (3,1),
                        1.0*(double)read_gpio_reg_int32 (7,1) / Q15
                        );            // (3,1) X6 STATUS, (7,1) X14 SIGNAL PASS
        }

}

//#define DBG_SETUPGVP

void rp_spmc_get_zservo_controller (double &setpoint, double &cp, double &ci, double &upper, double &lower, unsigned int &modes){
        int regA, regB;

        // readback Z-Servo configuration
        int mux_in;
        rp_spmc_module_read_config_data (SPMC_READBACK_IN_MUX_REG, &mux_in, &regB); // z_servo_src_mux, dum -- get input selection, need to range scaling 1V/5V!
        //volts_to_rpspmc_CONTROL_SELECT_ZS (regA (setpoint))

        rp_spmc_module_config_int32 (SPMC_Z_SERVO_SELECT_RB_SETPOINT_MODES_REG, 0, MODULE_START_VECTOR); // select data set
        rp_spmc_module_read_config_data (SPMC_READBACK_Z_SERVO_REG, &regA, &regB); // read data set
        setpoint = (double)regA / volts_to_rpspmc_CONTROL_SELECT_ZS (mux_in, 1.0);
        modes    = regB;
        
        rp_spmc_module_config_int32 (SPMC_Z_SERVO_SELECT_RB_CPI_REG, 0, MODULE_START_VECTOR);
        rp_spmc_module_read_config_data (SPMC_READBACK_Z_SERVO_REG, &regA, &regB);
        cp = (double)regA / (double) QZSCOEF;
        ci = (double)regB / (double) QZSCOEF;

        rp_spmc_module_config_int32 (SPMC_Z_SERVO_SELECT_RB_LIMITS_REG, 0, MODULE_START_VECTOR);
        rp_spmc_module_read_config_data (SPMC_READBACK_Z_SERVO_REG, &regA, &regB);
        upper = (double)regA / volts_to_rpspmc (1.);
        lower = (double)regB / volts_to_rpspmc (1.);
}

// Main SPM Z Feedback Servo Control
// CONTROL[32] OUT[32]   m[24]  x  c[32]  = 56 M: 24{Q32},  P: 44{Q14}
void rp_spmc_set_zservo_controller (double setpoint, double cp, double ci, double upper, double lower){
        if (verbose > 1) fprintf(stderr, "##Configure RP SPMC Z-Servo Controller: set= %g  cp=%g ci=%g upper=%g lower=%g\n", setpoint, cp, ci, upper, lower); 

        int mux_in, regB;
        rp_spmc_module_read_config_data (SPMC_READBACK_IN_MUX_REG, &mux_in, &regB); // z_servo_src_mux, dum -- get input selection, need to range scaling 1V/5V!
        rp_spmc_module_config_int32 (MODULE_SETUP, volts_to_rpspmc_CONTROL_SELECT_ZS (mux_in, setpoint), MODULE_START_VECTOR);
        
        rp_spmc_module_config_Qn (MODULE_SETUP, cp, MODULE_SETUP_VECTOR(1), QZSCOEF);
        rp_spmc_module_config_Qn (MODULE_SETUP, ci, MODULE_SETUP_VECTOR(2), QZSCOEF);
        rp_spmc_module_config_int32 (MODULE_SETUP,             volts_to_rpspmc (upper), MODULE_SETUP_VECTOR(3));
        rp_spmc_module_config_int32 (SPMC_Z_SERVO_CONTROL_REG, volts_to_rpspmc (lower), MODULE_SETUP_VECTOR(4));
}

void rp_spmc_set_zservo_gxsm_speciality_setting (int mode, double z_setpoint, double in_offset_comp){
        if (verbose > 1) fprintf(stderr, "##Configure RP SPMC Z-Servo Controller: mode= %d  Zset=%g offset_comp=%g\n", mode, z_setpoint, in_offset_comp); 

        int mux_in, regB;
        rp_spmc_module_read_config_data (SPMC_READBACK_IN_MUX_REG, &mux_in, &regB); // z_servo_src_mux, dum -- get input selection, need to range scaling 1V/5V!
        rp_spmc_module_config_int32 (MODULE_SETUP, volts_to_rpspmc_CONTROL_SELECT_ZS (mux_in, in_offset_comp), MODULE_START_VECTOR); // control input offset
        rp_spmc_module_config_int32 (MODULE_SETUP, 0, MODULE_SETUP_VECTOR(1)); // control setpoint offset = 0 always.
        rp_spmc_module_config_int32 (MODULE_SETUP, volts_to_rpspmc (z_setpoint), MODULE_SETUP_VECTOR(2));
        rp_spmc_module_config_uint32 (MODULE_SETUP, mode&0xff, MODULE_SETUP_VECTOR(3));
        rp_spmc_module_config_uint32 (SPMC_Z_SERVO_MODE_CONTROL_REG, (mode>>8)&0xff, MODULE_SETUP_VECTOR(4));
}




// RPSPMC GVP Engine Management
void rp_spmc_gvp_config (bool reset=true, bool pause=false, int reset_options=-1){
        
        if (!reset){
                rp_spmc_AD5791_set_stream_mode (); // enable streaming for AD5791 DAC's from FPGA now!
        }

        rp_spmc_module_config_uint32 (SPMC_GVP_CONTROL_REG, (reset ? 1:0) | (pause ? 2:0));

        if (reset_options >= 0)
                rp_spmc_module_config_uint32 (SPMC_GVP_RESET_OPTIONS_REG, reset_options);

        if (verbose > 0)
                fprintf(stderr, "##Configure GVP: %s RO[%d]\n", reset ? "Reset" : pause ? "Pause":"Run", reset_options);
}

// resets components by mask. (x=0 y=0, .. no change for mask = 0x03
void rp_spmc_gvp_reset_components (int mask){
        rp_spmc_module_config_uint32 (SPMC_GVP_RESET_COMPONENTS_REG, mask);
}


void reset_gvp_positions_uab(){
        double data[16] = { 0.,0.,0.,0., 0.,0.,0.,0., 0.,0.,0.,0., 0.,0.,0.,0. };
        rp_spmc_module_config_vector_Qn (SPMC_GVP_RESET_VECTOR_REG, data, 6);
}

void rp_spmc_gvp_init (){
        if (verbose > 0)
                fprintf(stderr, "##**** GVP INIT\n");

        reset_gvp_positions_uab();

        rp_spmc_gvp_config (true, false, 0x0000); // assure reset mode, FB not in hold

        // Program GVP with Null vectors for sanity:
        rp_spmc_module_start_config ();
        // write vector data using set_gpio_cfgreg_uint32 (SPMC_MODULE_CONFIG_DATA+N, data); // set config data 0..15 x 32bit
        for (int pc=0; pc<MAX_NUM_PROGRAN_VECTORS; ++pc){
                if (verbose > 2) fprintf(stderr, "Init Vector[PC=%03d] to zero, decii=%d\n", pc, SPMC_GVP_MIN_DECII);
                // write GVP-Vector [vector[0]] components
                //                   decii      du        dz        dy        dx     Next       Nrep,   Options,     nii,      N,    [Vadr]
                //data = {160'd0, 32'd0064, 32'd0000, 32'd0000, -32'd0002, -32'd0002,  32'd0, 32'd0000,   32'h001, 32'd0128, 32'd005, 32'd00 };
                set_gpio_cfgreg_int32 (SPMC_MODULE_CONFIG_DATA + GVP_VEC_VADR, pc);
                for (int i=1; i<16; ++i)
                        set_gpio_cfgreg_int32 (SPMC_MODULE_CONFIG_DATA + i, 0);
                set_gpio_cfgreg_int32 (SPMC_MODULE_CONFIG_DATA + GVP_VEC_DECII, SPMC_GVP_MIN_DECII);  // decimation
        }
        rp_spmc_module_write_config_data (SPMC_GVP_VECTOR_DATA_REG);

        rpspmc_init_info << "\n** RPSPMC GVP Engine INIT **\n";

        
}

// pc: ProgramCounter (Vector #)
// n: num points / vector
// opts: option flags: FB, ...
// nrp: num repetitions
// nxt: next vector, rel jump to +/-# vectors on PC
// deltas: for dx,dy,dz,du, da,db
// slew: data points/second / vector
// update_file: allow update while in progress (scan speed adjust, etc.), applied on FPGA level at next vector start!
void rp_spmc_set_gvp_vector (int pc, int n, unsigned int opts, unsigned int srcs, int nrp, int nxt,
                             int x_opcd, double x_cmpv, int x_rchi, int x_jmpr,
                             double dx, double dy, double dz, double du,
                             double da, double db, double dam, double dfm,
                             double slew, bool update_life=false){

        if ((pc&0xff) == 0){
                stream_debug_flags = opts;
                if (verbose > 1) fprintf(stderr, "\n** WRITE GVP: PC=0: stream_debug_flags = opts = 0x%04x\n", stream_debug_flags);
        }
                
        if (pc&0x1000){ // special auto vector delta computation mode
                if (verbose > 1) fprintf(stderr, "** Auto calc init vector to absolute position [%g %g %g %g] V\n", dx, dy, dz, du);
                double x = rpspmc_to_volts (read_gpio_reg_int32 (1,0));
                double y = rpspmc_to_volts (read_gpio_reg_int32 (1,1));
                //double z = rpspmc_to_volts (read_gpio_reg_int32 (10,0));
                //double u = rpspmc_to_volts (read_gpio_reg_int32 (8,0)); // Bias sum from SET and GVP
                double bias = rpspmc_to_volts (bias_buf);

                int regA, regB;
                rp_spmc_module_read_config_data (SPMC_READBACK_Z_REG, &regA, &regB);
                double z_gvp = rpspmc_to_volts (regA); // Z-GVP

                rp_spmc_module_read_config_data (SPMC_READBACK_GVPBIAS_REG, &regA, &regB); // GVP Bias Comp, GVP-A
                double u_gvp = rpspmc_to_volts (regA);

                if (verbose > 1) fprintf(stderr, "** XYZU-GVP readings are => [%g %g %g %g] V, Bias (actual)=%g V\n", x, y, z_gvp, u_gvp, bias);

                dx -= x;
                dy -= y;
                dz -= z_gvp;
                du -= u_gvp;

                // selection flags to clear auto delta to only adjust selected component
                if ((pc&0x1001) == 0x1001)
                        dx = 0.;
                if ((pc&0x1002) == 0x1002)
                        dy = 0.;
                if ((pc&0x1004) == 0x1004)
                        dz = 0.;
                if ((pc&0x1008) == 0x1008)
                        du = 0.;
                if ((pc&0x1010) == 0x1010)
                        da = 0.;
                if ((pc&0x1020) == 0x1020)
                        db = 0.;
                pc=0; // set PC = 0. Remove special mask
                if (verbose > 1)
                        fprintf(stderr, ">>>>>>>>>>>>[PC=%03d] AUTO INIT **{dXYZU => [%g %g %g %g] V}\n", pc, dx, dy, dz, du);
                // da, db not managed yet
        }

        unsigned int nii   = 0;
        unsigned int min_decii = SPMC_GVP_REGULAR_MIN_DECII; // minimum regular decii required for ADC: 32*4  => 4x DECII FOR ADC CLK x 32bits
        unsigned int decii = min_decii;
        double       Nsteps = 1.0;

        if (pc&0x2000){ // allow this vector for max (recording) speed?
                if (verbose > 1) fprintf(stderr, "ALLOWING MAX SLEW as of PC 0x2000 set\n");
                min_decii = SPMC_GVP_MIN_DECII; // experimental fast option -- no good for 1MSPS OUTs, shall stay fixed for this vector
                pc ^= 0x2000;  // clear bit
        }
        if (slew > 1e6){ // allow this vector for max (recording) speed?
                if (verbose > 1) fprintf(stderr, "ALLOWING MAX SLEW as requested.\n");
                // dx=dy=dz=du=0.;
                min_decii = SPMC_GVP_MIN_DECII; // experimental fast option -- no good for 1MSPS OUTs, shall stay fixed for this vector
        }

        if (pc >= MAX_NUM_PROGRAN_VECTORS || pc < 0){
                fprintf(stderr, "ERROR: Vector[PC=%03d] out of valif vector program space. Ignoring. GVP is put into RESET mode now.", pc);
                return;
        }

        if (n > 0 && slew > 0.0){
                // find smallest non null delta component
                double dt = 0.0;
                double deltas[8] = { dx,dy,dz,du,da,db,dam,dfm };
                double dmin = 10.0; // Volts here, +/-5V is max range
                double dmax =  0.0; // Volts here, +/-5V is max range
                for (int i=0; i<8; ++i){
                        double x = fabs(deltas[i]);
                        if (x > 1e-8 && dmin > x)
                                dmin = x;
                        if (dmax < x)
                                dmax = x;
                }
                double ddmin = dmin/n; // smallest point distance in volts at full step
                double ddmax = dmax/n; // smallest point distance in volts at full step
                double Vstep_prec_Q31 = 5.0/Q31;  // GVP precison:  5/(1<<31)*1e6 = ~0.00232830643653869629 uV
                                                  // DAC precision: 5/(1<<19)*1e6 = 9.5367431640625  uV
                
                //int ddminQ31  = (int)round(Q31*ddmin/SPMC_AD5791_REFV); // smallest point distance in Q31 for DAC in S19Q12  (32 bit total precision fixed point)
                // Error:
                //double ddminE = (double)ddminQ31 - Q31*ddmin/SPMC_AD5791_REFV; // smallest point distance in Q31 for DAC in S19Q12  (32 bit total precision fixed point)

                // SPMC_GVP_CLK = 125/2 MHz
                // slew in V/s
                // => time / point: 1/slew

                dt = 1.0/slew;
                // => NII total = dt*SPMC_GVP_CLK
                double NII_total = dt*SPMC_GVP_CLK;

                // problem: find best factors
                // NII_total = decii * nii

                // 100 A/V  x 20 -> 2000 A/V or 0.5mV/A or 5uV/pm assumptions for max decci step 1pm

                /*
                nii = 1;
                if (ddmin > 2e-6) // 1uV steps min
                        nii = (int)round(ddmin * 1e6);
                */

                if (round(ddmin/Vstep_prec_Q31)/1e6 > 4294967294.){  // maxed out?? (1<<32)-1 is max
                        nii = 4294967294;
                        if (verbose > 1) fprintf(stderr, "    ** WARNING NII MAX LIMITER ** Smalled full step ddmin=%g => %g\n", ddmin, nii*Vstep_prec_Q31*1e6);
                } else
                        nii = 3+(unsigned int)round(ddmin/Vstep_prec_Q31)/1e6; // Error < relative 1e6

                
                if ((NII_total / nii) > 4294967294.){ // maxed out?? (1<<32)-1 is max
                        decii = 4294967294; // limit and warn
                        if (verbose > 1) fprintf(stderr, "    ** WARNING DECII MAX LIMITER ** nii=%g NII/nii=%g NII_total=%g nii=%d\n", round (ddmin/Vstep_prec_Q31)/1e6, NII_total/nii, NII_total, nii);
                } else
                        decii = (unsigned int)round(NII_total / nii);

                // check for limits, auto adjust
                if (decii < min_decii){

                        decii = SPMC_GVP_MIN_DECII;
                        nii = (unsigned int)round(NII_total / decii);
                        if (nii < 2)
                                nii=3;
                        if (verbose > 1) fprintf(stderr, "    ** WARNING DECII MIN LIMITER ** nii=%d\n", nii);
                }
                
                //double deciiE = (double)decii - NII_total / nii;

                // total vector steps:
                Nsteps = nii * n;

                if (verbose > 1) fprintf(stderr, "    ** Auto calc decii: slew=%g pts/s, dmin=%g V, dt=%g s, ##=%g, Nsteps=%g {ddmin=%g uV #%g ddmax=%g uV #%g}\n",
                                         slew, dmin, dt, NII_total, Nsteps, ddmin, ddmin/Vstep_prec_Q31, ddmax, ddmax/Vstep_prec_Q31);

                
        }

        //if (!update_life)
        //        rp_spmc_gvp_config (); // assure reset+hold mode // NOT REQUIRED

        if (verbose > 1) fprintf(stderr, "\nWrite Vector[PC=%03d] = [", pc);

        // *** IMPORTANT GVP BEHAVIOR PER DESIGN ***
        // N=5 ii=2 ==> 3 inter vec add steps (2 extra waits), 6 (points) x3 (inter) delta vectors added for section
        
        
        // write GVP-Vector [vector[0]] components
        //           decii  ******     du        dz        dy        dx       Next      Nrep,   Options,     nii,      N,  [Vadr]
        // data = {32'd016, 160'd0, 32'd0004, 32'd0003, 32'd0064, 32'd0001,  32'd0, 32'd0000,   32'h0100, 32'd02, 32'd005, 32'd00 }; // 000
        
        rp_spmc_module_start_config ();

        int idv[32];
        set_gpio_cfgreg_int32 (SPMC_MODULE_CONFIG_DATA + GVP_VEC_VADR, idv[0]=pc);

        if (verbose > 1) fprintf(stderr, "[%04d], ", n);
        set_gpio_cfgreg_int32 (SPMC_MODULE_CONFIG_DATA + GVP_VEC_N, idv[1]=n > 0 ? n-1 : 0); // *** see above note

        if (verbose > 1) fprintf(stderr, "{%04d}, ", nii);
        set_gpio_cfgreg_uint32 (SPMC_MODULE_CONFIG_DATA + GVP_VEC_NII, idv[2]=nii > 0 ? nii-1 : 0); // *** see above note nii > 1 for normal vector, need that time for logic

        if (verbose > 1) fprintf(stderr, "Op%08x, ", opts);
        //set_gpio_cfgreg_uint32 (SPMC_MODULE_CONFIG_DATA + GVP_VEC_OPT, idv[3]=opts);

        //gvp_vector_i [I_GVP_OPTIONS ] = ((v->srcs & 0xffffff) << 8) | (v->options & 0xff);
        set_gpio_cfgreg_uint32 (SPMC_MODULE_CONFIG_DATA + GVP_VEC_OPT, idv[3]=((srcs & 0xffffff) << 8) | (opts & 0xff));
        
        // preparing to add/disentangle SRCS from opts
        if (verbose > 1) fprintf(stderr, "Sr%08x, ", srcs);
        //set_gpio_cfgreg_uint32 (SPMC_MODULE_CONFIG_DATA + GVP_VEC_SRC, idv[4]=srcs);
        idv[4]=srcs;
        
        if (verbose > 1) fprintf(stderr, "#%04d, ", nrp);
        set_gpio_cfgreg_int32 (SPMC_MODULE_CONFIG_DATA + GVP_VEC_NREP, idv[5]=nrp);

        if (verbose > 1) fprintf(stderr, "%04d, ", nxt);
        set_gpio_cfgreg_int32 (SPMC_MODULE_CONFIG_DATA + GVP_VEC_NEXT, idv[6]=nxt);

        if (verbose > 1) fprintf(stderr, "dX %8.10g uV, ", 1e6*dx/Nsteps);
        set_gpio_cfgreg_int32 (SPMC_MODULE_CONFIG_DATA + GVP_VEC_DX, idv[7]=(int)round(Q31*dx/Nsteps/SPMC_AD5791_REFV));  // => 23.1 S23Q8 @ +/-5V range in Q31

        if (verbose > 1) fprintf(stderr, "dY %8.10g uV, ", 1e6*dy/Nsteps);
        set_gpio_cfgreg_int32 (SPMC_MODULE_CONFIG_DATA + GVP_VEC_DY, idv[8]=(int)round(Q31*dy/Nsteps/SPMC_AD5791_REFV));  // => 23.1 S23Q8 @ +/-5V range in Q31

        if (verbose > 1) fprintf(stderr, "dZ %8.10g uV, ", 1e6*dz/Nsteps);
        set_gpio_cfgreg_int32 (SPMC_MODULE_CONFIG_DATA + GVP_VEC_DZ, idv[9]=(int)round(Q31*dz/Nsteps/SPMC_AD5791_REFV));  // => 23.1 S23Q8 @ +/-5V range in Q31

        if (verbose > 1) fprintf(stderr, "dU %8.10g uV, ", 1e6*du/Nsteps);
        set_gpio_cfgreg_int32 (SPMC_MODULE_CONFIG_DATA + GVP_VEC_DU, idv[10]=(int)round(Q31*du/Nsteps/SPMC_AD5791_REFV));  // => 23.1 S23Q8 @ +/-5V range in Q31

        if (verbose > 1) fprintf(stderr, "dA %8.10g uV, ", 1e6*da/Nsteps);
        set_gpio_cfgreg_int32 (SPMC_MODULE_CONFIG_DATA + GVP_VEC_DA, idv[11]=(int)round(Q31*da/Nsteps/SPMC_AD5791_REFV));  // => 23.1 S23Q8 @ +/-5V range in Q31
        
        if (verbose > 1) fprintf(stderr, "dB %8.10g uV, ", 1e6*db/Nsteps);
        set_gpio_cfgreg_int32 (SPMC_MODULE_CONFIG_DATA + GVP_VEC_DB, idv[12]=(int)round(Q31*db/Nsteps/SPMC_AD5791_REFV));  // => 23.1 S23Q8 @ +/-5V range in Q31
        
        if (verbose > 1) fprintf(stderr, "dAMc %8.10g uV, ", 1e6*dam/Nsteps);
        set_gpio_cfgreg_int32 (SPMC_MODULE_CONFIG_DATA + GVP_VEC_DAM, idv[13]=(int)round(Q31*dam/Nsteps/SPMC_AD5791_REFV));  // => 23.1 S23Q8 @ +/-5V range in Q31
        if (verbose > 1) fprintf(stderr, "dFMc %8.10g uV, ", 1e6*dfm/Nsteps);
        set_gpio_cfgreg_int32 (SPMC_MODULE_CONFIG_DATA + GVP_VEC_DFM, idv[14]=(int)round(Q31*dfm/Nsteps/SPMC_AD5791_REFV));  // => 23.1 S23Q8 @ +/-5V range in Q31
        set_gpio_cfgreg_int32 (SPMC_MODULE_CONFIG_DATA + GVP_VEC_014, idv[15]=0);  // clear bits

        if (verbose > 1) fprintf(stderr, "0,0,0, decii=%d]\n", decii); // last vector component is decii
        set_gpio_cfgreg_uint32 (SPMC_MODULE_CONFIG_DATA + GVP_VEC_DECII, idv[16]=decii);  // decimation

        // and load vector data to module
        rp_spmc_module_write_config_data (SPMC_GVP_VECTOR_DATA_REG);

        // add Vector Extension Code?
        if (opts & SPMC_GVP_VECTOR_EXT_BITMASK){
                rp_spmc_module_start_config ();
                set_gpio_cfgreg_int32 (SPMC_MODULE_CONFIG_DATA + GVP_VECX_VADR, pc);
                set_gpio_cfgreg_int32 (SPMC_MODULE_CONFIG_DATA + GVP_VECX_OPCD, idv[17]=x_opcd);
                set_gpio_cfgreg_int32 (SPMC_MODULE_CONFIG_DATA + GVP_VECX_CMPV, idv[18]=(int)round(Q31*x_cmpv/SPMC_AD5791_REFV));
                set_gpio_cfgreg_int32 (SPMC_MODULE_CONFIG_DATA + GVP_VECX_RCHI, idv[19]=x_rchi);
                set_gpio_cfgreg_int32 (SPMC_MODULE_CONFIG_DATA + GVP_VECX_JMPR, idv[20]=x_jmpr);
                //set_gpio_cfgreg_uint32 (SPMC_MODULE_CONFIG_DATA + GVP_VECX_OPT, idv[30]=opts);
                //set_gpio_cfgreg_uint32 (SPMC_MODULE_CONFIG_DATA + GVP_VECX_SRC, idv[31]=srcs);
                // and load vector-extension data to module
                rp_spmc_module_write_config_data (SPMC_GVP_VECTORX_DATA_REG);
        }
        
        //<< std::setfill('0') << std::hex << std::setw(8) << data
        
        std::stringstream vector_def;
        if (pc == 0)
                //             vector =   {32'd0, -32'd0, -32'd0, -32'd0, -32'd0, -32'd0, -32'd0, -32'd0, -32'd0, -32'd0, -32'd0, -32'd000, 32'h1001, -32'd0, -32'd0,  32'd1}; // Vector #1
                vector_def << "VectorDef: { Decii, 0,0,0, dB,dA,dU,dZ,dY,dX, nxt,reps,opts,nii,n, pc}\n";
        vector_def << "vector = {32'd" << ((unsigned int)idv[16]); // decii
        for (int i=15; i>=0; i--){
                switch (i){
                case 3:
                case 4:
                        vector_def << std::setfill('0') << std::setw(8) << std::hex << ", 32'h" << ((unsigned int)idv[i]); // SRCS, OPTIONS
                        break;
                default:
                        vector_def << ((idv[i] < 0) ?", -":",  ") << std::setfill('0') << std::dec << "32'd" << (abs(idv[i]));
                        break;
                }
        }
        if (opts & SPMC_GVP_VECTOR_EXT_BITMASK){
                vector_def << ", vecx = {";
                for (int i=20; i>=17; i--){
                        vector_def << ((idv[i] < 0) ?", -":",  ") << std::setfill('0') << std::dec << "32'd" << (abs(idv[i]));
                }
                vector_def << "}";
        }
        vector_def << "}; // Vector #" << pc;
        spmc_stream_server_instance.add_vector (vector_def.str());
}

// SPMC GVP STREAM SOURCE MUX 16 to 6
void rp_set_gvp_stream_mux_selector (unsigned long selector, unsigned long test_mode=0, int testval=0){
        if (verbose > 1) fprintf(stderr, "** set gvp stream mux 0x%06x\n", (unsigned int)selector);
        rp_spmc_module_config_uint32 (MODULE_SETUP, selector, MODULE_START_VECTOR);
        rp_spmc_module_config_uint32 (MODULE_SETUP, test_mode, MODULE_SETUP_VECTOR(1));
        rp_spmc_module_config_int32 (SPMC_MUX16_6_SRCS_CONTROL_REG, testval, MODULE_SETUP_VECTOR(2));

        int a,b;
        rp_spmc_module_read_config_data (SPMC_READBACK_XX_REG, &a, &b);
        if (verbose > 1) fprintf(stderr, "** FPGA MUX selector is %08x\n", (unsigned int)a);

}

// SPMC GVP STREAM SOURCE MUX 16 to 6
void rp_set_z_servo_stream_mux_selector (unsigned long selector, unsigned long test_mode=0, int testval=0){
        if (verbose > 1) fprintf(stderr, "** set gvp stream mux 0x%06x\n", (unsigned int)selector);
        rp_spmc_module_config_uint32 (MODULE_SETUP, selector, MODULE_START_VECTOR);
        rp_spmc_module_config_uint32 (MODULE_SETUP, test_mode, MODULE_SETUP_VECTOR(1));
        rp_spmc_module_config_int32 (SPMC_MUX2_Z_SERVO_CONTROL_REG, testval, MODULE_SETUP_VECTOR(2));

        if (verbose > 1) fprintf(stderr, "** Z_SERVO MUX selector is %08x\n", (unsigned int)selector);

}

// SPMC GVP STREAM SOURCE MUX 16 to 6
void rp_set_rf_out_mux_selector (unsigned long selector, unsigned long test_mode=0, int testval=0){
        if (verbose > 1) fprintf(stderr, "** set gvp stream mux 0x%06x\n", (unsigned int)selector);
        rp_spmc_module_config_uint32 (MODULE_SETUP, selector, MODULE_START_VECTOR);
        rp_spmc_module_config_uint32 (MODULE_SETUP, test_mode, MODULE_SETUP_VECTOR(1));
        rp_spmc_module_config_int32 (SPMC_MUX2_RF_OUT_CONTROL_REG, testval, MODULE_SETUP_VECTOR(2));

        if (verbose > 1) fprintf(stderr, "** Z_SERVO MUX selector is %08x\n", (unsigned int)selector);

}

// RPSPMC Location and Geometry
int rp_spmc_set_rotation (double alpha, double slew){
        static double current_alpha=0.0;
        int ret = -1;
        
        if (verbose > 1) fprintf(stderr, "** adjusting rotation to %g\n", alpha);
#ifdef ROTATE_SMOOTH
        if (slew < 0.){ // FORCE SET/INIT
                current_alpha = alpha;
                double data[16] = { cos(current_alpha), sin(current_alpha),0.,0., 0.,0.,0.,0., 0.,0.,0.,0., 0.,0.,0.,0. };
                rp_spmc_module_config_vector_Qn (SPMC_MAIN_CONTROL_ROTM_REG, data, 2, Q_XY_PRECISION);
                return 0;
        }
        
        double delta = alpha - current_alpha;

        if (fabs (delta) < 1e-3){
                current_alpha = alpha;
                ret=0; // done
        } else {
                double xs = rpspmc_to_volts (read_gpio_reg_int32 (1,0)); // (0,0 in FPGA-DIA)
                double ys = rpspmc_to_volts (read_gpio_reg_int32 (1,1));
                double r  = sqrt(xs*xs + ys*ys);

                if (r < 1e-4){
                        current_alpha = alpha;
                        ret=0; // done, at rot invariant position
                } else {
                        // radiant from volts to rotate at current radius at xy_move slew speed / update interval
                        double dphi  = slew*(PARAMETER_UPDATE_INTERVAL/1000.) / (delta*r); // PARAMETER_UPDATE_INTERVAL is 200ms
                        
                        if (fabs(delta) <= dphi){
                                current_alpha = alpha;
                                ret=0; // done next
                        } else {
                                if (delta > 0)
                                        current_alpha += dphi;
                                else
                                        current_alpha -= dphi;
                        }
                }
        }
#else
        current_alpha = alpha;
#endif        
        double data[16] = { cos(current_alpha), sin(current_alpha),0.,0., 0.,0.,0.,0., 0.,0.,0.,0., 0.,0.,0.,0. };
        rp_spmc_module_config_vector_Qn (SPMC_MAIN_CONTROL_ROTM_REG, data, 2, Q_XY_PRECISION);

        return ret;
}

void rp_spmc_set_slope (double dzx, double dzy, double dzxy_slew){
        if (verbose > 1){
                int dzxy_step = 1+(int)round (Q_Z_SLOPE_PRECISION*dzxy_slew/SPMC_CLK);
                fprintf(stderr, "** set dZxy slopes %g, %g ** dZ-step: %g => %d\n", dzx, dzy, dzxy_slew, dzxy_step);
        }
        
        double data[16] = { dzx, dzy, dzxy_slew/SPMC_CLK,0., 0.,0.,0.,0., 0.,0.,0.,0., 0.,0.,0.,0. };
        rp_spmc_module_config_vector_Qn (SPMC_MAIN_CONTROL_SLOPE_REG, data, 2, Q_Z_SLOPE_PRECISION);
}

// 1+ / -1 (=negate)
void rp_spmc_set_z_polarity (int z_polarity){
        if (verbose > 1){
                fprintf(stderr, "** WARNING: NON LINEAR. Setting Z-Polarity to %d.\n", z_polarity);
        }
        
        rp_spmc_module_config_int32 (SPMC_MAIN_CONTROL_Z_POLARITY_REG, z_polarity < 0 ? 1:0); // negate final Z? (Bit 0 = 1 for YES)
}


double rp_spmc_set_bias_test (double bias){
        double v=0.;
        rp_spmc_module_config_int32 (SPMC_MAIN_CONTROL_BIAS_REG, bias_buf = volts_to_rpspmc (bias)); // Set U0 = Gxsm BiasRef
        
        usleep(MODULE_ADDR_SETTLE_TIME);
        int regA, regB;
        rp_spmc_module_read_config_data (SPMC_READBACK_BIAS_REG, &regA, &regB); // Bias Sum Reading, Bias U0 Set Val
        v = rpspmc_to_volts (regA);

        return v-bias;
}

void rp_spmc_set_bias (double bias){
        int n=2;
        double v=0.;
        do {
                usleep(MODULE_ADDR_SETTLE_TIME);
                if (verbose > 1) fprintf(stderr, "##Configure SPMC Bias=%g V...", bias);
                rp_spmc_module_config_int32 (SPMC_MAIN_CONTROL_BIAS_REG, bias_buf = volts_to_rpspmc (bias)); // Set U0 = Gxsm BiasRef
        
                if (verbose > 1){
                        usleep(MODULE_ADDR_SETTLE_TIME);
                        int regA, regB;
                        rp_spmc_module_read_config_data (SPMC_READBACK_BIAS_REG, &regA, &regB); // Bias Sum Reading, Bias U0 Set Val
                        v = rpspmc_to_volts (regA);
                        fprintf(stderr, "[%d] Verify: Bias=%g V Bbuf: %d regA: %d ERROR=%d regB: %d\n",
                                2-n, v,      bias_buf, regA,  regA-bias_buf, regB);
                }
        }while (--n && fabs(v-bias)>1e-4);

        if (bias == -5.0){
                fprintf(stderr, "\n ** BIAS SET TEST CYCLE ** \n");
                double dv;
                int e=0;
                for (v=-5.; v<=5; v+=0.01){
                        dv = rp_spmc_set_bias_test (v);
                        if (fabs (dv > 1e-4)){
                                e++;
                                fprintf(stderr, "%d %g @%g V\n", e, dv, v);
                        }
                }

        }
}

void rp_spmc_set_offsets (double x0, double y0, double z0, double bias, double xy_move_slew, double z_move_slew){
        static int xy_step = 1+(int)(volts_to_rpspmc (0.5/SPMC_CLK));
        static int z_step = 1+(int)(volts_to_rpspmc (0.2/SPMC_CLK));

        // adjust rate is SPMC_CLK in Hz
        if (xy_move_slew >= 0.0)
                xy_step = 1+(int)(volts_to_rpspmc (xy_move_slew/SPMC_CLK));
        if (z_move_slew >= 0.0)
                z_step = 1+(int)(volts_to_rpspmc (z_move_slew/SPMC_CLK));

        if (verbose > 1) fprintf(stderr, "##Configure XYZU [%g %g %g Bias=%g]V {%d %d}/cycle\n", x0, y0, z0, bias, xy_step, z_step);

        rp_spmc_module_config_int32 (MODULE_SETUP, x0_buf = volts_to_rpspmc (x0), MODULE_START_VECTOR); // first, init ** X0
        rp_spmc_module_config_int32 (MODULE_SETUP, y0_buf = volts_to_rpspmc (y0), MODULE_SETUP_VECTOR(1)); // setup...    ** Y0
        rp_spmc_module_config_int32 (MODULE_SETUP, z0_buf = volts_to_rpspmc (z0), MODULE_SETUP_VECTOR(2)); // setup...    ** Z0
        rp_spmc_module_config_int32 (MODULE_SETUP, bias_buf = volts_to_rpspmc (bias), MODULE_SETUP_VECTOR(3)); // setup...    ** U0=BiasRef
        rp_spmc_module_config_int32 (MODULE_SETUP, xy_step, MODULE_SETUP_VECTOR(4)); // setup...        ** xy_offset_step/moveto adjuster speed
        rp_spmc_module_config_int32 (SPMC_MAIN_CONTROL_XYZU_OFFSET_REG, z_step,  MODULE_SETUP_VECTOR(5));  // last, write all ** xy_offset_step/moveto adjuster speed
}

/* TEST XY MOVE
PC=0: stream_debug_flags = opts = 0xc03b01, Write Vector[PC=000] = [Auto calc decii: slew=50 pts/s, dmin=0.5285 V, dt=0.02 s, ##=2.5e+06, Nsteps=350 {ddmin=0.01057 uV #4.53978e+06 ddmax=0.0415 uV #1.78241e+07}
Life Vector Update is ON
0050, 0007, 00c03b01, 0000, 0000, dX     1510 uV, dY -5928.571429 uV, dZ        0 uV, dU        0 uV, dA        0 uV, dB        0 uV, 0,0,0, decii=357143]
Write Vector[PC=001] = [Life Vector Update is ON
0000, 0000, 00000000, 0000, 0000, dX        0 uV, dY        0 uV, dZ        0 uV, dU        0 uV, dA        0 uV, dB        0 uV, 0,0,0, decii=128]
Write Vector[PC=001] = [Life Vector Update is ON
0000, 0000, 00000000, 0000, 0000, dX        0 uV, dY        0 uV, dZ        0 uV, dU        0 uV, dA        0 uV, dB        0 uV, 0,0,0, decii=128]
##Configure GVP: Reset
*/

void rp_spmc_set_scanpos (double xs, double ys, double slew, int opts){
        // setup GVP to do so...
        // read current xy scan pos
        double dx = xs - rpspmc_to_volts (read_gpio_reg_int32 (1,0));
        double dy = ys - rpspmc_to_volts (read_gpio_reg_int32 (1,1));

        double dist = sqrt(dx*dx + dy*dy);
        double eps = ad5791_dac_to_volts (16); // arbitrary limit 4bit to ignore delta **** WATCH ME ****
        int n = 50;
        
        if (dist < eps)
                return;

        // convert slew comes in A/s => px/s
        slew /= dist; // convert A/s to slew rate in N/dt or pts/s per vector
        
        if (verbose > 1) fprintf(stderr, "** set scanpos [%g V, %g V @%g s/Vec]V ** dxy: |%g, %g| = %g  ** %s\n", xs, ys, slew, dx,dy, dist, dist < eps ? "At Position.":"Loading GVP...");
        
        rp_spmc_gvp_config (true, false); // reset any GVP now!
        //if (verbose > 1) fprintf(stderr, "** set scanpos GVP reset **\n");
        usleep(1000);

        // make vector to new scan pos requested:
        rp_spmc_set_gvp_vector (0, n, opts, 0, 0, 0,
                                0, 0.0, 0, 0,
                                dx, dy, 0.0, 0.0,
                                0.0, 0.0, 0.0, 0.0,
                                slew, false);
        rp_spmc_set_gvp_vector (1, 0, 0, 0, 0, 0,
                                0, 0.0, 0, 0,
                                0.0, 0.0, 0.0, 0.0,
                                0.0, 0.0, 0.0, 0.0,
                                0.0, false);
        rp_spmc_set_gvp_vector (2, 0, 0, 0, 0, 0,
                                0, 0.0, 0, 0,
                                0.0, 0.0, 0.0, 0.0,
                                0.0, 0.0, 0.0, 0.0,
                                0.0, false);

        if (verbose > 1) fprintf(stderr, "** set scanpos GVP vectors programmed, exec... **\n");
        usleep(1000);

#if 0 // ** START RECORDING ** possible...
        fprintf(stderr, "*** GVP Control Mode EXEC: reset stream server, clear buffer)\n");
        stream_server_control = (stream_server_control & 0x01) | 4; // set stop bit
        usleep(100000);
        spm_dma_instance->clear_buffer (); // clean buffer now!

        fprintf(stderr, "*** GVP Control Mode EXEC: START DMA+stream server\n");

        spm_dma_instance->start_dma (); // get DMA ready to take data
        stream_server_control = (stream_server_control & 0x01) | 2; // set start bit
        usleep(10000);

        rp_spmc_gvp_config (false, false); // take GVP out of reset -> release GVP to start executing VPC now
        fprintf(stderr, "*** GVP Control Mode EXEC: START 3 (taking out of RESET)\n");
#else
        // and execute
        rp_spmc_gvp_config (false, false);
        //if (verbose > 1) fprintf(stderr, "** set scanpos GVP started **\n");
#endif
}

void rp_spmc_set_modulation (double volume, int target, int dfc_target){
        if (verbose > 1) fprintf(stderr, "##Configure: LCK VOLUME volume= %g V, TARGET: #%d\n", volume, target);

        rp_spmc_module_config_int32  (MODULE_SETUP, volts_to_rpspmc(volume), MODULE_START_VECTOR);
        rp_spmc_module_config_uint32 (MODULE_SETUP, target&0x0f, MODULE_SETUP_VECTOR(1));
        rp_spmc_module_config_uint32 (SPMC_MAIN_CONTROL_MODULATION_REG, dfc_target&0x0f, MODULE_SETUP_VECTOR(2));
}



// Lock In / Modulation

double lck_decimation_factor = 1.0;

double rp_spmc_configure_lockin (double freq, double gain, double FM_scale, unsigned int mode, double RF_ref_freq, int LCKID){
        // 44 Bit Phase, using 48bit tdata
        unsigned int n2 = round (log2(dds_phaseinc (freq)));
        unsigned long long phase_inc = 1LL << n2;
        double fact = dds_phaseinc_to_freq (phase_inc);
        //double hzv = dds_phaseinc (1.) / volts_to_rpspmc(1.); // 1V Ramp => 1Hz
        double hzv = 171.798691839990234375; //((1<<32)-1)/125000000*5   Hz / V in phase inc

        //parameter LCK_BUFFER_LEN2 = 10, // **** need to decimate 100Hz min: log2(1<<44 / (1<<24)) => 20 *** LCK DDS IDEAL Freq= 100 Hz [16777216, N2=24, M=1048576]  Actual f=119.209 Hz  Delta=19.2093 Hz  (using power of 2 phase_inc)
        //decii2 <= 44 - LCK_BUFFER_LEN2 - dds_n2;
        lck_decimation_factor = (1 << (44 - 10 - n2)) - 1.;
 
        if (verbose > 1){
                double ferr = fact - freq;
                fprintf(stderr, "##Configure: LCK DDS IDEAL Freq= %g Hz [%lld, N2=%d, decii=%d, M=%lld]  Actual f=%g Hz  Delta=%g Hz  (using power of 2 phase_inc)\n", freq, phase_inc, n2, n2>10? 1<<(n2-10):0, QQ44/phase_inc, fact, ferr);
                fprintf(stderr, "##Configure: RF Gen Freq= %g Hz PhInc=%g\n", RF_ref_freq, round(4294967295. * RF_ref_freq/125000000.));
                if (mode)
                        fprintf(stderr, "##Configure: LCK DDS MODE 0x%x FMS: %g V/Hz hzv*FMS: %g, gain: %g x  LckDecFac: %g\n", mode, FM_scale, hzv*FM_scale, gain, lck_decimation_factor);
                // Configure: LCK DDS IDEAL Freq= 100 Hz [16777216, N2=24, M=1048576]  Actual f=119.209 Hz  Delta=19.2093 Hz  (using power of 2 phase_inc)
        }
        
        rp_spmc_module_config_uint32 (MODULE_SETUP, mode, MODULE_START_VECTOR); // first, init
        rp_spmc_module_config_Qn     (MODULE_SETUP, gain, MODULE_SETUP_VECTOR(1), Q24); // GAIN
        rp_spmc_module_config_int32  (MODULE_SETUP, n2, MODULE_SETUP_VECTOR(2)); // last, write
        rp_spmc_module_config_int64  (MODULE_SETUP, phase_inc, MODULE_SETUP_VECTOR(3)); // last, write

        // dds_FM_Scale: [Hz/V] * ((1<<44)-1)/125000000*5/(1<<(44-32))
        //    fmc          <= dds_FM_Scale * $signed(S_AXIS_FMC_tdata); // Q** *Q31 => Q48    == xxx Hz <=> 1V (Q24)  **** 1000Hz*((1<<44)-1)/125000000Hz
        //    dds_FM       <= fmc >>> 20; // FM-Scale: Q44

        rp_spmc_module_config_uint32 (MODULE_SETUP, (unsigned int)(round(hzv*FM_scale)), MODULE_SETUP_VECTOR(5)); // write FM-Scale Hz/V
        rp_spmc_module_config_uint32 (LCKID, (unsigned int)round(4294967295. * RF_ref_freq/125000000.), MODULE_SETUP_VECTOR(6)); // last, write RF-DDS PHASE INC

        
        return fact;
}


// set BiQuad to pass
void rp_spmc_set_biqad_Lck_F0_pass (int BIQID, int test_mode){
        double data[16] = { 1.0, 0.0, 0.0,  1.0, 0.0, 0.0,  0.,0., 0.,0.,0.,0., 0.,0.,0.,0. };
        if (verbose > 1){
                fprintf(stderr, "##Configure: BiQuad to pass mode: (test=%d)\n", test_mode);
                fprintf(stderr, "## b0=1, b1=b2=0 a0=1, a1=a2=0\n");
        }
        rp_spmc_module_config_vector_Qn (MODULE_SETUP, data, 6, Q28);
        rp_spmc_module_config_uint32 (MODULE_SETUP, test_mode, MODULE_SETUP_VECTOR(6));
        int test_deci = (int)(lck_decimation_factor * 16);   // 1+(int)(125e6 / SPMC_SC_LCK_RF_FREQUENCY.Value () / 1024.);
        rp_spmc_module_config_uint32 (BIQID, test_deci, MODULE_SETUP_VECTOR(7));
}

// set BiQuad to IIR 1st order LP
void rp_spmc_set_biqad_Lck_F0_IIR (double f_cut, int BIQID, int test_mode){
        double b0, b1, b2, a0, a1, a2;
        b0=1.0;
        b1=b2=0.0;
        a0=1.0;
        a1=a2=0.0;
        
        // 2nd order BiQuad Low Pass parameters
        double wc = 2.*M_PI*f_cut/(125e6 / lck_decimation_factor / 64); // 125MHz  LockIn Dec + FIR DECI (64x)
        //double c  = cos (wc); // near 1 for f_c/Fs << 1 *** or wc/(1+wc)
        //double xc = 1.0 - c;
        
        b0 = wc/(1+wc);  // new    //0.5*xc;
        a1 = -(1.0-b0);  // last   //xc;
        
        if (verbose > 1){
                fprintf(stderr, "##Configure: BiQuad to IIR 1st order mode. Fc=%g Hz, Fs=%g Hz, test=%d\n", f_cut,  125e6/lck_decimation_factor, test_mode);
                fprintf(stderr, "## b0=%g b1=%g b2=%g  a0=%g a1=%g a2=%g\n", b0, b1, b2, a0, a1, a2);
                fprintf(stderr, "##Q28: b0=%08x a1=%08x\n", (int)round(b0*Q28),  (int)round(a1*Q28));
        }

        double data[16] = { b0/a0, b1/a0, b2/a0, 1.0, a1/a0,a2/a0,  0.,0., 0.,0.,0.,0., 0.,0.,0.,0. };
        rp_spmc_module_config_vector_Qn (MODULE_SETUP, data, 6, Q28);
        rp_spmc_module_config_uint32 (MODULE_SETUP, test_mode, MODULE_SETUP_VECTOR(6));
        int test_deci = (int)(lck_decimation_factor * 16);   // 1+(int)(125e6 / SPMC_SC_LCK_RF_FREQUENCY.Value () / 1024.);
        rp_spmc_module_config_uint32 (BIQID, test_deci, MODULE_SETUP_VECTOR(7));
}

/*
BiQuad Direct form 1
y[n] = b0 x[n] + b1 x[n-1] + b2 x[n-2] - a1 y[n-1] - a2 y[n-2]

Normalized so a0=1

x[n] ------ *b0 ---> + -------------> y[n]
        |            |            |
input   z  pipe1     |            z   output pipe1
        |            |            |
x[n-1]  --- *b1 ---> + <-- -a1* ---   y[n-1]
        |            |            |
input   z  pipe2     |            z   output pipe2
        |            |            |
x[n-2]  --- *b2 ---> + <-- -a2* ---   y[n-2]
*/

// set BiQuad to low pass at f_cut, with Quality Factor Q and at sampling rate Fs Hz
void rp_spmc_set_biqad_Lck_F0 (double f_cut, double Q, int BIQID, int test_mode){
        double b0, b1, b2, a0, a1, a2;

        // 2nd order BiQuad Low Pass parameters
        double w0 = 2.*M_PI*f_cut/(125e6 / lck_decimation_factor / 64); // 125MHz  LockIn Dec + FIR DECI (64x)

#if 1
        double c  = cos (w0);
        double xc = 1.0 - c;
        double a  = sin (w0) / (2.0 * Q);
        
        b0 = 0.5*xc;
        b1 = xc;
        b2 = b0;
        a0 = 1.0+a;
        a1 = -2.0*c;
        a2 = 1.0-a;

#else

        //double Q = f_cut/BW;
        double w024 = w0*w0/4.;
        double Qdn = (1. + 1./Q + w024);
        double Qmn = (1. - 1./Q + w024);
        b0 = 1.;
        b1 = 2.*(1.-w024);
        b2 = b0;
        a0 = Qdn;
        a1 = 2*(w024-1.);
        a2 = Qmn;
#endif   
        if (verbose > 1){
                fprintf(stderr, "##Configure: BiQuad Fc=%g Hz  Q=%g, Fs=%g Hz test=%d:\n", f_cut, Q, 125e6/lck_decimation_factor, test_mode);
                fprintf(stderr, "## b0=%g b1=%g b2=%g  a0=%g a1=%g a2=%g\n", b0, b1, b2, a0, a1, a2);
                fprintf(stderr, "##Q28: b0=%08x b1=%08x a1=%08x a2=%08x\n", (int)round(b0*Q28),(int)round(b1*Q28), (int)round(a1*Q28),(int)round(a2*Q28));
        }

        double data[16] = { b0/a0, b1/a0, b2/a0, 1.0, a1/a0,a2/a0, (double)test_mode,0., 0.,0.,0.,0., 0.,0.,0.,0. };
        rp_spmc_module_config_vector_Qn (MODULE_SETUP, data, 6, Q28);
        rp_spmc_module_config_uint32 (MODULE_SETUP, test_mode, MODULE_SETUP_VECTOR(6));
        int test_deci = (int)(lck_decimation_factor * 16);   // 1+(125e6 / SPMC_SC_LCK_RF_FREQUENCY.Value () / 1024.);
        rp_spmc_module_config_uint32 (BIQID, test_deci, MODULE_SETUP_VECTOR(7));
}

void rp_spmc_set_biqad_Lck_AB (int BIQID, int test_mode){
        double b0, b1, b2, a0, a1, a2;

        b0 = SPMC_SC_LCK_BQ_COEF_B0.Value ();
        b1 = SPMC_SC_LCK_BQ_COEF_B1.Value ();
        b2 = SPMC_SC_LCK_BQ_COEF_B2.Value ();
        a0 = SPMC_SC_LCK_BQ_COEF_A0.Value ();
        a1 = SPMC_SC_LCK_BQ_COEF_A1.Value ();
        a2 = SPMC_SC_LCK_BQ_COEF_A2.Value ();

        if (verbose > 1){
                fprintf(stderr, "##Configure: BiQuad AB Fs=%g Hz test=%d:\n", 125e6/lck_decimation_factor, test_mode);
                fprintf(stderr, "## b0=%g b1=%g b2=%g  a0=%g a1=%g a2=%g\n", b0/a0, b1/a0, b2/a0, a0/a0, a1/a0, a2/a0);
                fprintf(stderr, "##Q28: b0=%08x b1=%08x a1=%08x a2=%08x\n", (int)round(b0/a0*Q28),(int)round(b1/a0*Q28), (int)round(a1/a0*Q28),(int)round(a2/a0*Q28));
        }

        double data[16] = { b0/a0, b1/a0, b2/a0, 1.0, a1/a0,a2/a0, (double)test_mode,0., 0.,0.,0.,0., 0.,0.,0.,0. };
        rp_spmc_module_config_vector_Qn (MODULE_SETUP, data, 6, Q28);
        rp_spmc_module_config_uint32 (MODULE_SETUP, test_mode, MODULE_SETUP_VECTOR(6));
        int test_deci = (int)(lck_decimation_factor * 16);   // 1+(125e6 / SPMC_SC_LCK_RF_FREQUENCY.Value () / 1024.);
        rp_spmc_module_config_uint32 (BIQID, test_deci, MODULE_SETUP_VECTOR(7));
}



/*
  Signal Monitoring via GPIO:
  U-Mon: GPIO 7 X15
  X-Mon: GPIO 7 X16
  Y-Mon: GPIO 8 X17
  Z-Mon: GPIO 8 X18
 */


void rp_spmc_update_readings (){

        //spmc_stream_server_instance.add_xyz_info ( rpspmc_to_volts (read_gpio_reg_int32 (8,1)),
        //                                           rpspmc_to_volts (read_gpio_reg_int32 (9,0)),
        //                                           rpspmc_to_volts (read_gpio_reg_int32 (9,1)));
      
        SPMC_GVP_STATUS.Value () = read_gpio_reg_int32 (3,1);
        //assign dbg_status = { GVP-STATUS, 0,0,0,0,  0, GVP-hold, GVP-finished, z-servo };
        //                      |= { sec[32-3], reset, pause, ~finished }[23:0]
        
        SPMC_BIAS_MONITOR.Value () = rpspmc_to_volts (read_gpio_reg_int32 (8,0));

        SPMC_X_MONITOR.Value () = rpspmc_to_volts (read_gpio_reg_int32 (8,1));
        SPMC_Y_MONITOR.Value () = rpspmc_to_volts (read_gpio_reg_int32 (9,0));
        SPMC_Z_MONITOR.Value () = rpspmc_to_volts (read_gpio_reg_int32 (9,1));

        SPMC_XS_MONITOR.Value () = rpspmc_to_volts (read_gpio_reg_int32 (1,0)); // (0,0 in FPGA-DIA)
        SPMC_YS_MONITOR.Value () = rpspmc_to_volts (read_gpio_reg_int32 (1,1));
        //SPMC_ZS_MONITOR.Value () = rpspmc_to_volts (read_gpio_reg_int32 (10,0));

        int regA, regB;
        rp_spmc_module_read_config_data (SPMC_READBACK_Z_REG, &regA, &regB);
        SPMC_Z0_MONITOR.Value () = rpspmc_to_volts (regA); // Z-GVP
        SPMC_ZS_MONITOR.Value () = rpspmc_to_volts (regB); // Z-slope component

        rp_spmc_module_read_config_data (SPMC_READBACK_BIAS_REG, &regA, &regB); // Bias Sum Reading, Bias U0 Set Val
        SPMC_BIAS_REG_MONITOR.Value () = rpspmc_to_volts (regA);
        SPMC_BIAS_SET_MONITOR.Value () = rpspmc_to_volts (regB);
        
        rp_spmc_module_read_config_data (SPMC_READBACK_GVPBIAS_REG, &regA, &regB); // GVP Bias Comp, GVP-A
        SPMC_GVPU_MONITOR.Value () = rpspmc_to_volts (regA);
        //SPMC_ MOD _MONITOR.Value () = rpspmc_to_volts (regB); // modulation monitor

        rp_spmc_module_read_config_data (SPMC_READBACK_SRCS_MUX_REG, &regA, &regB); // Srcs-MUX, GVP-B
        SPMC_MUX_MONITOR.Value ()  = rpspmc_to_volts (regA);
        // 0
        
        //rp_spmc_module_read_config_data (SPMC_READBACK_IN_MUX_REG, &regA, &regB); // Srcs-MUX, GVP-B
        //SPMC_MUX_IN_MONITOR.Value ()  = rpspmc_to_volts (regA);
        // 0
        
        regA=regB=0;
        rp_spmc_module_read_config_data (SPMC_READBACK_PMD_DA56_REG, &regA, &regB); // GVP-A, GVP-B
        SPMC_GVPA_MONITOR.Value () = rpspmc_to_volts (regA);
        SPMC_GVPB_MONITOR.Value () = rpspmc_to_volts (regB);

        regA=regB=0;
        rp_spmc_module_read_config_data (SPMC_READBACK_GVP_AMC_FMC_REG, &regA, &regB); // GVP-AMC, GVP-FMC
        SPMC_GVPAMC_MONITOR.Value () = rpspmc_to_volts (regA);
        SPMC_GVPFMC_MONITOR.Value () = rpspmc_to_volts (regB);

        regA=regB=0;
        rp_spmc_module_read_config_data (SPMC_READBACK_AD463X_REG, &regA, &regB); // AD463x CH1, CH2 readings
        SPMC_AD463X_CH1_MONITOR.Value () = rpspmc_to_volts (regA);
        SPMC_AD463X_CH2_MONITOR.Value () = rpspmc_to_volts (regB);

        SPMC_X0_MONITOR.Value () = rpspmc_to_volts (x0_buf); // ** mirror
        SPMC_Y0_MONITOR.Value () = rpspmc_to_volts (y0_buf); // ** mirror
        //SPMC_Z0_MONITOR.Value () = 0.0; // rpspmc_to_volts (read_gpio_reg_int32 (10,1));

        rp_spmc_module_read_config_data (SPMC_READBACK_IN_MUX_REG, &regA, &regB); // z_servo_src_mux, dum
        SPMC_SIGNAL_MONITOR.Value () = rpspmc_CONTROL_SELECT_ZS_to_volts (regA, read_gpio_reg_int32 (7,1)); // SQ8.24 (Z-Servo Input Signal, processed)

        rp_spmc_module_read_config_data (SPMC_READBACK_UPTIME_CLOCK_REG, &regA, &regB); // uptime, clock 8ns/sec
        SPMC_UPTIME_SECONDS.Value () = (double)regA + (double)regB/125e6;
}


void *thread_xyz_meter_reading(void *arg) {
        static double xyz[3][3];

        for (; xyz_meter_reading_control; ){
                xyz[0][0] = rpspmc_to_volts (read_gpio_reg_int32 (8,1)); // X
                xyz[1][0] = rpspmc_to_volts (read_gpio_reg_int32 (9,0)); // Y
                xyz[2][0] = rpspmc_to_volts (read_gpio_reg_int32 (9,1)); // Z

                for (int i=0; i<3; ++i){
                        if (xyz[i][0] > xyz[i][1]) xyz[i][1] = xyz[i][0]; else xyz[i][1] = 0.999*xyz[i][1] + 0.001*xyz[i][0]; // inst MAX, slow converge back
                        if (xyz[i][0] < xyz[i][2]) xyz[i][2] = xyz[i][0]; else xyz[i][2] = 0.999*xyz[i][2] + 0.001*xyz[i][0]; // inst MIN, slow converge back
                        SIGNAL_XYZ_METER[3*i+0] = (float)xyz[i][0]; 
                        SIGNAL_XYZ_METER[3*i+1] = (float)xyz[i][1]; 
                        SIGNAL_XYZ_METER[3*i+2] = (float)xyz[i][2]; 
                }
                usleep (1000); // updated every 1ms
        }
        return (NULL);
}
