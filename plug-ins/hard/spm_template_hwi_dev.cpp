/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 8 c-style: "K&R" -*- */

/* Gnome gxsm - Gnome X Scanning Microscopy
 * universal STM/AFM/SARLS/SPALEED/... controlling and
 * data analysis software
 *
 * Gxsm Hardware Interface Plugin Name: spm_template_hwi.C
 * ===============================================
 * 
 * Copyright (C) 1999 The Free Software Foundation
 *
 * Authors: Percy Zahl <zahl@fkp.uni-hannover.de>
 * additional features: Andreas Klust <klust@fkp.uni-hannover.de>
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

#include <sys/ioctl.h>

#include "config.h"
#include "plugin.h"
#include "xsmhard.h"
#include "glbvars.h"

#include "gxsm_app.h"
#include "gxsm_window.h"
#include "surface.h"

#include "../common/pyremote.h"
#include "spm_template_hwi.h"
#include "spm_template_hwi_emulator.h"

// Define HwI PlugIn reference name here, this is what is listed later within "Preferenced Dialog"
// i.e. the string selected for "Hardware/Card"!
#define THIS_HWI_PLUGIN_NAME "SPM-TEMPL:SPM"
#define THIS_HWI_PREFIX      "SPM_TEMPL_HwI"

extern int debug_level;

extern "C++" {
        extern SPM_Template_Control *Template_ControlClass;
        extern GxsmPlugin spm_template_hwi_pi;
}


MOD_INPUT mod_input_list[] = {
        //## [ MODULE_SIGNAL_INPUT_ID, name, actual hooked signal address ]
        { DSP_SIGNAL_Z_SERVO_INPUT_ID, "Z_SERVO", 0 },
        { DSP_SIGNAL_M_SERVO_INPUT_ID, "M_SERVO", 0 },
        { DSP_SIGNAL_MIXER0_INPUT_ID, "MIXER0", 0 },
        { DSP_SIGNAL_MIXER1_INPUT_ID, "MIXER1", 0 },
        { DSP_SIGNAL_MIXER2_INPUT_ID, "MIXER2", 0 },
        { DSP_SIGNAL_MIXER3_INPUT_ID, "MIXER3", 0 },

        { DSP_SIGNAL_DIFF_IN0_ID, "DIFF_IN0", 0 },
        { DSP_SIGNAL_DIFF_IN1_ID, "DIFF_IN1", 0 },
        { DSP_SIGNAL_DIFF_IN2_ID, "DIFF_IN2", 0 },
        { DSP_SIGNAL_DIFF_IN3_ID, "DIFF_IN3", 0 },

        { DSP_SIGNAL_SCAN_CHANNEL_MAP0_ID, "SCAN_CHMAP0", 0 },
        { DSP_SIGNAL_SCAN_CHANNEL_MAP1_ID, "SCAN_CHMAP1", 0 },
        { DSP_SIGNAL_SCAN_CHANNEL_MAP2_ID, "SCAN_CHMAP2", 0 },
        { DSP_SIGNAL_SCAN_CHANNEL_MAP3_ID, "SCAN_CHMAP3", 0 },

        { DSP_SIGNAL_LOCKIN_A_INPUT_ID, "LOCKIN_A", 0 },
        { DSP_SIGNAL_LOCKIN_B_INPUT_ID, "LOCKIN_B", 0 },
        { DSP_SIGNAL_VECPROBE0_INPUT_ID, "VECPROBE0", 0 },
        { DSP_SIGNAL_VECPROBE1_INPUT_ID, "VECPROBE1", 0 },
        { DSP_SIGNAL_VECPROBE2_INPUT_ID, "VECPROBE2", 0 },
        { DSP_SIGNAL_VECPROBE3_INPUT_ID, "VECPROBE3", 0 },
        { DSP_SIGNAL_VECPROBE0_CONTROL_ID, "VECPROBE0_C", 0 },
        { DSP_SIGNAL_VECPROBE1_CONTROL_ID, "VECPROBE1_C", 0 },
        { DSP_SIGNAL_VECPROBE2_CONTROL_ID, "VECPROBE2_C", 0 },
        { DSP_SIGNAL_VECPROBE3_CONTROL_ID, "VECPROBE3_C", 0 },

        { DSP_SIGNAL_OUTMIX_CH5_INPUT_ID, "OUTMIX_CH5", 0 },
        { DSP_SIGNAL_OUTMIX_CH5_ADD_A_INPUT_ID, "OUTMIX_CH5_ADD_A", 0 },
        { DSP_SIGNAL_OUTMIX_CH5_SUB_B_INPUT_ID, "OUTMIX_CH5_SUB_B", 0 },
        { DSP_SIGNAL_OUTMIX_CH5_SMAC_A_INPUT_ID, "OUTMIX_CH5_SMAC_A", 0 },
        { DSP_SIGNAL_OUTMIX_CH5_SMAC_B_INPUT_ID, "OUTMIX_CH5_SMAC_B", 0 },

        { DSP_SIGNAL_OUTMIX_CH6_INPUT_ID, "OUTMIX_CH6", 0 },
        { DSP_SIGNAL_OUTMIX_CH6_ADD_A_INPUT_ID, "OUTMIX_CH6_ADD_A", 0 },
        { DSP_SIGNAL_OUTMIX_CH6_SUB_B_INPUT_ID, "OUTMIX_CH6_SUB_B", 0 },
        { DSP_SIGNAL_OUTMIX_CH6_SMAC_A_INPUT_ID, "OUTMIX_CH6_SMAC_A", 0 },
        { DSP_SIGNAL_OUTMIX_CH6_SMAC_B_INPUT_ID, "OUTMIX_CH6_SMAC_B", 0 },

        { DSP_SIGNAL_OUTMIX_CH7_INPUT_ID, "OUTMIX_CH7", 0 },
        { DSP_SIGNAL_OUTMIX_CH7_ADD_A_INPUT_ID, "OUTMIX_CH7_ADD_A", 0 },
        { DSP_SIGNAL_OUTMIX_CH7_SUB_B_INPUT_ID, "OUTMIX_CH7_SUB_B", 0 },
        { DSP_SIGNAL_OUTMIX_CH7_SMAC_A_INPUT_ID, "OUTMIX_CH7_SMAC_A", 0 },
        { DSP_SIGNAL_OUTMIX_CH7_SMAC_B_INPUT_ID, "OUTMIX_CH7_SMAC_B", 0 },

        { DSP_SIGNAL_OUTMIX_CH8_INPUT_ID, "OUTMIX_CH8_INPUT", 0 },
        { DSP_SIGNAL_OUTMIX_CH8_ADD_A_INPUT_ID, "OUTMIX_CH8_ADD_A_INPUT", 0 },
        { DSP_SIGNAL_OUTMIX_CH9_INPUT_ID, "OUTMIX_CH9_INPUT", 0 },
        { DSP_SIGNAL_OUTMIX_CH9_ADD_A_INPUT_ID, "OUTMIX_CH9_ADD_A_INPUT", 0 },

        { DSP_SIGNAL_ANALOG_AVG_INPUT_ID, "ANALOG_AVG_INPUT", 0 },
        { DSP_SIGNAL_SCOPE_SIGNAL1_INPUT_ID, "SCOPE_SIGNAL1_INPUT", 0 },
        { DSP_SIGNAL_SCOPE_SIGNAL2_INPUT_ID, "SCOPE_SIGNAL2_INPUT", 0 },

        { 0, "END", 0 }
};




// HwI Implementation
// ================================================================================


spm_template_hwi_dev::spm_template_hwi_dev(){

        // auto adjust and override preferences
        main_get_gapp()->xsm->Inst->override_dig_range ((1<<31)-1, xsmres);
        main_get_gapp()->xsm->Inst->override_volt_in_range (10.0, xsmres);
        main_get_gapp()->xsm->Inst->override_volt_out_range (10.0, xsmres);
        
        spm_emu = new SPM_emulator ();
                 
	probe_fifo_thread_active=0;

	subscan_data_y_index_offset = 0;
        ScanningFlg=0;
        KillFlg=FALSE;
        
        for (int i=0; i<4; ++i){
                srcs_dir[i] = nsrcs_dir[i] = 0;
                Mob_dir[i] = NULL;
        }
}

spm_template_hwi_dev::~spm_template_hwi_dev(){
        delete spm_emu;
}

int spm_template_hwi_dev::RotateStepwise(int exec) {
        return 0;
}

gboolean spm_template_hwi_dev::SetOffset(double x, double y){
        spm_emu->x0=x; spm_emu->y0=y; // "DAC" units
        return FALSE;
}

gboolean spm_template_hwi_dev::MovetoXY (double x, double y){
        if (!ScanningFlg){
                spm_emu->data_x_index = (int)round(Nx/2 +   x/Dx);
                spm_emu->data_y_index = (int)round(Ny/2 + (-y/Dy));
        }
        // if slow, return TRUE until completed, execuite non blocking!
        // May/Should return FALSE right away if hardware is independently executing and completing the move.
        // else blocking GUI
        return FALSE;
}




// THREAD AND FIFO CONTROL STATES


#define RET_FR_OK      0
#define RET_FR_ERROR   -1
#define RET_FR_WAIT    1
#define RET_FR_NOWAIT  2
#define RET_FR_FCT_END 3

#define FR_NO   0
#define FR_YES  1

#define FR_INIT   1
#define FR_FINISH 2
#define FR_FIFO_FORCE_RESET 3


gpointer ScanDataReadThread (void *ptr_sr);
gpointer ProbeDataReadThread (void *ptr_sr);
gpointer ProbeDataReadFunction (void *ptr_sr, int dspdev);

// ScanDataReadThread:
// Image Data read thread -- actual scan/pixel data transfer

gpointer ScanDataReadThread (void *ptr_sr){
        spm_template_hwi_dev *sr = (spm_template_hwi_dev*)ptr_sr;
        int nx,ny, x0,y0;
        int Nx, Ny;
        g_message("FifoReadThread Start");

        if (sr->Mob_dir[sr->srcs_dir[0] ? 0:1]){
                ny = sr->Mob_dir[sr->srcs_dir[0] ? 0:1][0]->GetNySub(); // number lines to transfer
                nx = sr->Mob_dir[sr->srcs_dir[0] ? 0:1][0]->GetNxSub(); // number points per lines
                x0 = sr->Mob_dir[sr->srcs_dir[0] ? 0:1][0]->data->GetX0Sub(); // number points per lines
                y0 = sr->Mob_dir[sr->srcs_dir[0] ? 0:1][0]->data->GetY0Sub(); // number points per lines

                g_message("FifoReadThread nx,ny = (%d, %d) @ (%d, %d)", nx, ny, x0, y0);

        } else return NULL; // error, no reference

	if (sr->spm_emu->data_y_count == 0){ // top-down
                g_message("FifoReadThread Scanning Top-Down... %d", sr->ScanningFlg);
		for (int yi=y0; yi < y0+ny; ++yi){ // for all lines
                        for (int dir = 0; dir < 4 && sr->ScanningFlg; ++dir){ // check for every pass -> <- 2> <2
                                //g_message("FifoReadThread ny = %d, dir = %d, nsrcs = %d, srcs = 0x%04X", yi, dir, sr->nsrcs_dir[dir], sr->srcs_dir[dir]);
                                if (sr->nsrcs_dir[dir] == 0) // direction pass active?
                                        continue; // not selected?
                                else
                                        for (int xi=x0; xi < x0+nx; ++xi) // all points per line
                                                for (int ch=0; ch<sr->nsrcs_dir[dir]; ++ch){
                                                        sr->spm_emu->data_x_index = xi;
                                                        double z = sr->spm_emu->simulate_value (sr, xi, yi, ch);
                                                        //g_print("%d",ch);
                                                        usleep (1000000 / Template_ControlClass->sim_speed[0]);
                                                        sr->Mob_dir[dir][ch]->PutDataPkt (z, xi, yi);
                                                        while (sr->PauseFlg)
                                                                usleep (1000000 / Template_ControlClass->sim_speed[0]);
                                                }
                                g_print("\n");
                        }
                        sr->spm_emu->data_y_count = yi-y0; // completed
                        sr->spm_emu->data_y_index = yi; // completed
                }
	}else{ // bottom-up
		for (int yi=y0+ny-1; yi-y0 >= 0; --yi){
                        for (int dir = 0; dir < 4 && sr->ScanningFlg; ++dir) // check for every pass -> <- 2> <2
                                if (!sr->nsrcs_dir[dir])
                                        continue; // not selected?
                                else
                                        for (int xi=x0; xi < x0+nx; ++xi)
                                                for (int ch=0; ch<sr->nsrcs_dir[dir]; ++ch){
                                                        sr->spm_emu->data_x_index = xi;
                                                        double z = sr->spm_emu->simulate_value (sr, xi, yi, ch);
                                                        usleep (1000000 / Template_ControlClass->sim_speed[0]);
                                                        sr->Mob_dir[dir][ch]->PutDataPkt (z, xi, yi);
                                                        while (sr->PauseFlg)
                                                                usleep (1000000 / Template_ControlClass->sim_speed[0]);
                                                }
                        sr->spm_emu->data_y_count = yi-y0; // completed
                        sr->spm_emu->data_y_index = yi;
                }
        }

        g_message("FifoReadThread Completed");
        return NULL;
}

// FIFO watch verbosity...
//# define LOGMSGS0(X) PI_DEBUG (DBG_L1, X)
# define LOGMSGS0(X)

# define LOGMSGS(X) PI_DEBUG (DBG_L2, X)
//# define LOGMSGS(X)

//# define LOGMSGS1(X) PI_DEBUG (DBG_L4, X)
# define LOGMSGS1(X)

//# define LOGMSGS2(X) PI_DEBUG (DBG_L5, X)
# define LOGMSGS2(X)

// ProbeDataReadThread:
// Independent ProbeDataRead Thread (manual probe)
gpointer ProbeDataReadThread (void *ptr_sr){
	int finish_flag=FALSE;
	spm_template_hwi_dev *sr = (spm_template_hwi_dev*)ptr_sr;
        g_message("ProbeFifoReadThread ENTER");

	if (sr->probe_fifo_thread_active > 0){
                g_message("ProbeFifoReadThread ERROR: Attempt to start again while in progress! [%d]", sr->probe_fifo_thread_active );
		//LOGMSGS ( "ProbeFifoReadThread ERROR: Attempt to start again while in progress! [#" << sr->probe_fifo_thread_active << "]" << std::endl);
		return NULL;
	}
	sr->probe_fifo_thread_active++;
	Template_ControlClass->probe_ready = FALSE;

        // normal mode, wait for finish (single shot probe exec by user)
	if (Template_ControlClass->probe_trigger_single_shot)
		 finish_flag=TRUE;

        g_message("ProbeFifoReadThread ** starting processing loop ** FF=%s", finish_flag?"True":"False");
	while (sr->is_scanning () || finish_flag){ // while scanning (raster mode) or until single shot probe is finished
                if (Template_ControlClass->current_auto_flags & FLAG_AUTO_PLOT)
                        Template_ControlClass->Probing_graph_update_thread_safe ();

                if (sr->ReadProbeData ()){ // True when finished
                
                        g_message("ProbeFifoReadThread ** Finished ** FF=%s", finish_flag?"True":"False");
                        if (finish_flag){
                                if (Template_ControlClass->current_auto_flags & FLAG_AUTO_PLOT)
                                        Template_ControlClass->Probing_graph_update_thread_safe (1);
				
                                if (Template_ControlClass->current_auto_flags & FLAG_AUTO_SAVE)
                                        Template_ControlClass->Probing_save_callback (NULL, Template_ControlClass);	

                                break;
                        }
                        Template_ControlClass->push_probedata_arrays ();
                        Template_ControlClass->init_probedata_arrays ();
                }
	}

	--sr->probe_fifo_thread_active;
	Template_ControlClass->probe_ready = TRUE;

        g_message("ProbeFifoReadThread EXIT");

	return NULL;
}


// ReadProbeData:
// read from probe data FIFO, this engine needs to be called several times from master thread/function
int spm_template_hwi_dev::ReadProbeData (int dspdev, int control){
	int pvd_blk_size=0;
	static double pv[9];
	static int last = 0;
	static int last_read_end = 0;
	static int next_header = 0;
	static int number_channels = 0;
        static int number_points = 0;
	static int point_index = 0;
	static int end_read = 0;
	static int data_block_size=0;
	static int need_fct = FR_YES;  // need fifo control
	static int need_hdr = FR_YES;  // need header
	static int need_data = FR_YES; // need data
	static int ch_lut[32];
	static int ch_msk[]  = { 0x0000001, 0x0000002,   0x0000010, 0x0000020, 0x0000040, 0x0000080,   0x0000100, 0x0000200, 0x0000400, 0x0000800,
				 0x0000008, 0x0001000, 0x0002000, 0x0004000, 0x0008000,   0x0000004,   0x0000000 };
	static int ch_size[] = {    2, 2,    2, 2, 2, 2,   2, 2, 2, 2,    4,   4,  4,  4,   4,   4,  0 };
	static const char *ch_header[] = {"Zmon-AIC5Out", "Umon-AIC6Out", "AIC0-I", "AIC1", "AIC2", "AIC3", "AIC4", "AIC5", "AIC6", "AIC7",
					  "LockIn0", "*LockIn1stA", "*LockIn1stB", "*LockIn2ndA", "*LockIn2ndB", "Count", NULL };
	static double dataexpanded[16];
#ifdef LOGMSGS0
	static double dbg0=0., dbg1=0.;
	static int dbgi0=0;
#endif

	switch (control){
	case FR_FIFO_FORCE_RESET: // not used normally -- FIFO is reset by DSP at probe_init (single probe)
		return RET_FR_OK;

	case FR_INIT:

                g_message ("VP: init");
                spm_emu->vp_init (); // vectors should be written by now!
                
                last = 0;
		next_header = 0;
		number_channels = 0;
		point_index = 0;
		last_read_end = 0;

		need_fct  = FR_YES; // Fifo Control
		need_hdr  = FR_YES; // Header
		need_data = FR_YES; // Data

		Template_ControlClass->init_probedata_arrays ();
		for (int i=0; i<16; dataexpanded[i++]=0.);

		LOGMSGS ( std::endl << "************** PROBE FIFO-READ INIT **************" << std::endl);
		LOGMSGS ( "FR::INIT-OK." << std::endl);
		return RET_FR_OK; // init OK.

	case FR_FINISH:
		LOGMSGS ( "FR::FINISH-OK." << std::endl);
		return RET_FR_OK; // finish OK.

	default: break;
	}

	if (need_fct){ // read and check fifo control?
		LOGMSGS2 ( "FR::NEED_FCT" << std::endl);
                // template -- dummy, no actual FIFO here
                need_fct  = FR_NO;
                need_data = FR_YES;
	}

	if (need_data){ // read full FIFO block
		LOGMSGS ( "FR::NEED_DATA" << std::endl);
                // template dummy
		need_data = FR_NO;
	}

	if (need_hdr){ // we have enough data if control gets here!
		LOGMSGS ( "FR::NEED_HDR" << std::endl);

                g_message ("VP: section header reading");
                if (spm_emu->vp_header_current.srcs){ // should be valid now!
                        // copy header to pv[] as assigned below

                        // set vector of expanded data array representation, section start
                        number_points = spm_emu->vp_header_current.n;

                        if (!number_points){ // finished?
                                g_message ("VP: finished");
                                return RET_FR_FCT_END;
                        }
                        
                        pv[0] = spm_emu->vp_header_current.time;
                        pv[1] = spm_emu->vp_header_current.move_xyz[i_X];
                        pv[2] = spm_emu->vp_header_current.move_xyz[i_Y];
                        pv[3] = spm_emu->vp_header_current.move_xyz[i_Z];
                        pv[4] = spm_emu->vp_header_current.scan_xyz[i_X];
                        pv[5] = spm_emu->vp_header_current.scan_xyz[i_Y];
                        pv[6] = spm_emu->vp_header_current.scan_xyz[i_Z];
                        pv[7] = spm_emu->vp_header_current.bias;
                        pv[8] = spm_emu->vp_header_current.section;

                        Template_ControlClass->add_probe_hdr (pv); // add probe section with full position header

                        // analyze header and setup channel lookup table
                        number_channels=0;
                        LOGMSGS ( "FR::NEED_HDR: decoding DATA_SRCS to read..." << std::endl);
                        for (int i = 0; ch_msk[i] && i < 18; ++i){
                                if (spm_emu->vp_header_current.srcs & ch_msk[i]){
                                        ch_lut[number_channels] = i;
                                        ++number_channels;
                                }
                        }
                point_index = 0;
		need_hdr = FR_NO;
                Template_ControlClass->set_probevector (pv);
                } else {
                        g_warning ("VP READ ERROR: no header.");
                        return RET_FR_WAIT;
                }
                
        }

        int ix=0;
        g_message ("VP: section: %d", spm_emu->vp_header_current.section);
        for (; point_index < number_points;){
                // template code --------
                ix = spm_emu->vp_exec_callback (); // run one VP step... this generates the next data set, read to transfer from spm_emu->vp_data_set[]
                g_message ("VP: %d %d", ix, point_index);
                ++point_index;
                // ----------------------
                
                // expand data from stream
                for (int element=0; element < number_channels; ++element){
                        int channel = ch_lut[element];
                        dataexpanded[channel] = spm_emu->vp_data_set[element];
                }

		// add vector and data to expanded data array representation
                Template_ControlClass->add_probevector ();
                Template_ControlClass->add_probedata (dataexpanded);

                if (point_index % 10 == 0) break; // for graph updates
	}

        if (point_index == number_points || ix == 0){
                LOGMSGS ( "FR:FIFO NEED FCT" << std::endl);
                need_fct = FR_YES;
                need_hdr = FR_YES;
        }
        
	return RET_FR_OK;
}



// prepare, create and start g-thread for data transfer
int spm_template_hwi_dev::start_data_read (int y_start, 
                                            int num_srcs0, int num_srcs1, int num_srcs2, int num_srcs3, 
                                            Mem2d **Mob0, Mem2d **Mob1, Mem2d **Mob2, Mem2d **Mob3){

	PI_DEBUG_GP (DBG_L1, "HWI-SPM-TEMPL-DBGI mk2::start fifo read\n");

        g_message("Setting up FifoReadThread");


	if (num_srcs0 || num_srcs1 || num_srcs2 || num_srcs3){ // setup streaming of scan data
                g_message("... for scan.");
#if 0 // full version used by MK2/3
		fifo_data_num_srcs[0] = num_srcs0; // number sources -> (forward)
		fifo_data_num_srcs[1] = num_srcs1; // number sources <- (return)
		fifo_data_num_srcs[2] = num_srcs2; // numbre sources 2nd->  (2nd pass forward if used)
		fifo_data_num_srcs[3] = num_srcs3; // number sources <-2nd  (2nd pass return if used)
		fifo_data_Mobp[0] = Mob0; // 2d memory object list ->
		fifo_data_Mobp[1] = Mob1; // ...
		fifo_data_Mobp[2] = Mob2;
		fifo_data_Mobp[3] = Mob3;
		fifo_data_y_index = y_start; // if > 0, scan dir is "bottom-up"
		fifo_read_thread = g_thread_new ("FifoReadThread", FifoReadThread, this);

                // do also raster probing setup?
		if ((Template_ControlClass->Source & 0xffff) && Template_ControlClass->probe_trigger_raster_points > 0){
			Template_ControlClass->probe_trigger_single_shot = 0;
			ReadProbeFifo (thread_dsp, FR_FIFO_FORCE_RESET); // reset FIFO
			ReadProbeFifo (thread_dsp, FR_INIT); // init
		}
#endif
                // scan simulator
                ScanningFlg=1; // can be used to abort data read thread. (Scan Stop forced by user)
                spm_emu->data_y_count = y_start; // if > 0, scan dir is "bottom-up"
                data_read_thread = g_thread_new ("FifoReadThread", ScanDataReadThread, this);
                
	}
	else{ // expect and stream probe data
                g_message("... for probe.");
		//if (Template_ControlClass->vis_Source & 0xffff)
                Template_ControlClass->probe_trigger_single_shot = 1;
                ReadProbeData (0, FR_INIT); // init
                probe_data_read_thread = g_thread_new ("ProbeFifoReadThread", ProbeDataReadThread, this);
	}
      
	return 0;
}

// ScanLineM():
// Scan setup: (yindex=-2),
// Scan init: (first call with yindex >= 0)
// while scanning following calls are progress checks (return FALSE when yindex line data transfer is completed to go to next line for checking, else return TRUE to continue with this index!
gboolean spm_template_hwi_dev::ScanLineM(int yindex, int xdir, int muxmode,
                                          Mem2d *Mob[MAX_SRCS_CHANNELS],
                                          int ixy_sub[4]){
	static int ydir=0;
	static int running = FALSE;
	static double us_per_line;

        if (yindex == -2){ // SETUP STAGE 1
                int num_srcs_w = 0; // #words
                int num_srcs_l = 0; // #long words
                int num_srcs = 0;
                int bi = 0;

                running      = FALSE;

                do{
                        if(muxmode & (1<<bi++)) // aka "lssrcs" -- count active channels (bits=1)
                                ++num_srcs_w;
                }while(bi<12);
                
                // find # of srcs_l (32 bit data -> float ) 0x1000..0x8000 (Bits 12,13,14,15)
                do{
                        if(muxmode & (1<<bi++))
                                ++num_srcs_l;
                }while(bi<16);
                
                num_srcs = (num_srcs_l<<4) | num_srcs_w;

                switch (xdir){
                case 1: // first init step of XP (->)
                        // reset all
                        for (int i=0; i<4; ++i){
                                srcs_dir[i] = nsrcs_dir[i] = 0;
                                Mob_dir[i] = NULL;
                        }
                        // setup XP ->
                        srcs_dir[0]  = muxmode;
                        nsrcs_dir[0] = num_srcs;
                        Mob_dir[0]   = Mob;
                        return TRUE;
                case -1: // second init step of XM (<-)
                        srcs_dir[1]  = muxmode;
                        nsrcs_dir[1] = num_srcs;
                        Mob_dir[1]   = Mob;
                        return TRUE;
                case  2: // ... init step of 2ND_XP (2>)
                        srcs_dir[2]  = muxmode;
                        nsrcs_dir[2] = num_srcs;
                        Mob_dir[2]   = Mob;
                        return TRUE;
                case -2: // ... init step of 2ND_XM (<2)
                        srcs_dir[3]  = muxmode;
                        nsrcs_dir[3] = num_srcs;
                        Mob_dir[3]   = Mob;
                        return TRUE;
                }
                return FALSE; // error
        }
        
        // SETUP STAGE 2
	if (! running && yindex >= 0){ // now do final scan setup and send scan setup, start reading data fifo
		
		// Nx, Dx are number datapoints in X to take at Dx DAC increments
		// Ny, Dy are number datapoints in Y to take at Dy DAC increments

		ydir = yindex == 0 ? 1 : -1; // scan top-down or bottom-up ?

                // may compute and set if available
		// main_get_gapp()->xsm->data.s.pixeltime = (double)dsp_scan.dnx/SamplingFreq;

                // setup hardware for scan here and
                // start g-thread for data transfer now
		start_data_read (yindex, nsrcs_dir[0], nsrcs_dir[1], nsrcs_dir[2], nsrcs_dir[3], Mob_dir[0], Mob_dir[1], Mob_dir[2], Mob_dir[3]);
                
		running = TRUE; // and off we go....
                return TRUE;
	}

        // ACTUAL SCAN PROGRESS CHECK on line basis
        if (ScanningFlg){ // make sure we did not got aborted and comkpleted already!

                //g_print ("sranger_mk3_hwi_spm::ScanLineM(yindex=%d [fifo-y=%d], xdir=%d, ydir=%d, lssrcs=%x) checking...\n", yindex, data_y_index, xdir, ydir, muxmode);
                y_current = spm_emu->data_y_index;

                if (ydir > 0 && yindex <= spm_emu->data_y_count){
                        g_print ("sranger_mk3_hwi_spm::ScanLineM(yindex=%d [fifo-y=%d], xdir=%d, ydir=%d, lssrcs=%x) y done.\n", yindex, spm_emu->data_y_index, xdir, ydir, muxmode);
                        return FALSE; // line completed top-down
                }
                if (ydir < 0 && yindex >= spm_emu->data_y_count){
                        g_print ("sranger_mk3_hwi_spm::ScanLineM(yindex=%d [fifo-y=%d], xdir=%d, ydir=%d, lssrcs=%x) y done.\n", yindex, spm_emu->data_y_index, xdir, ydir, muxmode);
                        return FALSE; // line completed bot-up
                }

                // optional
                if ((ydir > 0 && yindex > 1) || (ydir < 0 && yindex < Ny-1)){
                        double x,y,z;
                        RTQuery ("W", x,y,z); // run watch dog -- may stop scan in any faulty condition
                }
                
                return TRUE;
        }
        return FALSE; // scan was stopped by users
}




/*
 Real-Time Query of DSP signals/values, auto buffered
 Propertiy hash:      return val1, val2, val3:
 "z" :                ZS, XS, YS  with offset!! -- in volts after piezo amplifier
 "o" :                Z0, X0, Y0  offset -- in volts after piezo amplifier
 "R" :                expected Z, X, Y -- in Angstroem/base unit
 "f" :                dFreq, I-avg, I-RMS
 "s","S" :                DSP Statemachine Status Bits, DSP load, DSP load peak
 "Z" :                probe Z Position
 "i" :                GPIO (high level speudo monitor)
 "A" :                Mover/Wave axis counts 0,1,2 (X/Y/Z)
 "p" :                X,Y Scan/Probe Coords in Pixel, 0,0 is center, DSP Scan Coords
 "P" :                X,Y Scan/Probe Coords in Pixel, 0,0 is top left [indices]
 */

gint spm_template_hwi_dev::RTQuery (const gchar *property, double &val1, double &val2, double &val3){
        if (*property == 'z'){
                double x = (double)(spm_emu->data_x_index-Nx/2)*Dx;
                double y = (double)(Ny/2-spm_emu->data_y_index)*Dy;
                Transform (&x, &y); // apply rotation!
		val1 =  main_get_gapp()->xsm->Inst->VZ() * spm_emu->data_z_value;
		val2 =  main_get_gapp()->xsm->Inst->VX() * (x + spm_emu->x0 ) * 10/32768; // assume "internal" non analog offset adding here (same gain)
                val3 =  main_get_gapp()->xsm->Inst->VY() * (y + spm_emu->y0 ) * 10/32768;
		return TRUE;
	}

	if (*property == 'o' || *property == 'O'){
		// read/convert and return offset
		// NEED to request 'z' property first, then this is valid and up-to-date!!!!
                // no offset simulated
		if (main_get_gapp()->xsm->Inst->OffsetMode () == OFM_ANALOG_OFFSET_ADDING){
			val1 =  main_get_gapp()->xsm->Inst->VZ0() * 0.;
			val2 =  main_get_gapp()->xsm->Inst->VX0() * spm_emu->x0*10/32768;
			val3 =  main_get_gapp()->xsm->Inst->VY0() * spm_emu->y0*10/32768;
		} else {
			val1 =  main_get_gapp()->xsm->Inst->VZ() * 0.;
			val2 =  main_get_gapp()->xsm->Inst->VX() * spm_emu->x0*10/32768;
			val3 =  main_get_gapp()->xsm->Inst->VY() * spm_emu->y0*10/32768;
		}
		
		return TRUE;
	}

        // ZXY in Angstroem
        if (*property == 'R'){
                // ZXY Volts after Piezoamp -- without analog offset -> Dig -> ZXY in Angstroem
		val1 = main_get_gapp()->xsm->Inst->V2ZAng (main_get_gapp()->xsm->Inst->VZ() * spm_emu->data_z_value);
		val2 = main_get_gapp()->xsm->Inst->V2XAng (main_get_gapp()->xsm->Inst->VX() * (double)(spm_emu->data_x_index-Nx/2)*Dx)*10/32768;
                val3 = main_get_gapp()->xsm->Inst->V2YAng (main_get_gapp()->xsm->Inst->VY() * (double)(Ny/2-spm_emu->data_y_index/2)*Dy)*10/32768;
		return TRUE;
        }

        if (*property == 'f'){
                val1 = 0.; // qf - DSPPACClass->pll.Reference[0]; // Freq Shift
		val2 = spm_emu->tip_current / main_get_gapp()->xsm->Inst->nAmpere2V(1.); // actual nA reading    xxxx V  * 0.1nA/V
		val3 = spm_emu->tip_current / main_get_gapp()->xsm->Inst->nAmpere2V(1.); // actual nA RMS reading    xxxx V  * 0.1nA/V -- N/A for simulation
		return TRUE;
	}

        if (*property == 'Z'){
                val1 = 0.; // spm_template_hwi_pi.app->xsm->Inst->Dig2ZA ((double)spm_vector_program.Zpos / (double)(1<<16));
		return TRUE;
        }

	// DSP Status Indicators
	if (*property == 's' || *property == 'S' || *property == 'W'){
		if (*property == 'W'){
                        if (ScanningFlg){
                                if (0) EndScan2D(); // if any error detected
                                return TRUE;
                        }
                }
                // build status flags
		val1 = (double)(0
                                +1 // (FeedBackActive    ? 1:0)
				+ ((ScanningFlg & 3) << 1)  // Stop = 1, Pause = 2
				//+ (( ProbingSTSActive   ? 1:0) << 3)
				//+ (( MoveInProgress     ? 1:0) << 4)
				//+ (( PLLActive          ? 1:0) << 5)
				//+ (( ZPOS-Adjuster      ? 1:0) << 6)
				//+ (( AutoApproachActive ? 1:0) << 7)
			);
        }
        
	// quasi GPIO monitor/mirror -- HIGH LEVEL!!!
	if (*property == 'i'){
		//val1 = (double)gpio3_monitor_out;
		//val2 = (double)gpio3_monitor_in;
		//val3 = (double)gpio3_monitor_dir;
		return TRUE;
	}

        if (*property == 'p'){ // SCAN DATA INDEX, CENTERED range: +/- NX/2, +/-NY/2
                val1 = (double)( spm_emu->data_x_index - (main_get_gapp()->xsm->data.s.nx/2 - 1) + 1);
                val2 = (double)(-spm_emu->data_y_index + (main_get_gapp()->xsm->data.s.ny/2 - 1) + 1);
                val3 = (double)spm_emu->data_z_value;
		return TRUE;
        }
        if (*property == 'P'){ // SCAN DATA INDEX, range: 0..NX, 0..NY
                val1 = (double)(spm_emu->data_x_index);
                val2 = (double)(spm_emu->data_y_index);
                val3 = (double)spm_emu->data_z_value;
		return TRUE;
        }
//	printf ("ZXY: %g %g %g\n", val1, val2, val3);

//	val1 =  (double)dsp_analog_out.z_scan;
//	val2 =  (double)dsp_analog_out.x_scan;
//	val3 =  (double)dsp_analog_out.y_scan;

	return TRUE;
}

// template/dummy signal management



int spm_template_hwi_dev::lookup_signal_by_ptr(gint64 sigptr){
	for (int i=0; i<NUM_SIGNALS; ++i){
		if (dsp_signal_lookup_managed[i].dim == 1 && sigptr == dsp_signal_lookup_managed[i].p)
			return i;
		gint64 offset = sigptr - (gint64)dsp_signal_lookup_managed[i].p;
		if (sigptr >= dsp_signal_lookup_managed[i].p && offset < 4*dsp_signal_lookup_managed[i].dim){
			dsp_signal_lookup_managed[i].index = offset/4;
			return i;
		}
	}
	return -1;
}

int spm_template_hwi_dev::lookup_signal_by_name(const gchar *sig_name){
	for (int i=0; i<NUM_SIGNALS; ++i)
		if (!strcmp (sig_name, dsp_signal_lookup_managed[i].label))
			return i;
	return -1;
}

const gchar *spm_template_hwi_dev::lookup_signal_name_by_index(int i){
	if (i<NUM_SIGNALS && i >= 0){
		// dsp_signal_lookup_managed[i].index
		return (const gchar*)dsp_signal_lookup_managed[i].label;
	} else
		return "INVALID INDEX";
}

const gchar *spm_template_hwi_dev::lookup_signal_unit_by_index(int i){
	if (i<NUM_SIGNALS && i >= 0)
		return (const gchar*)dsp_signal_lookup_managed[i].unit;
	else
		return "INVALID INDEX";
}

double spm_template_hwi_dev::lookup_signal_scale_by_index(int i){
	if (i<NUM_SIGNALS && i >= 0)
		return dsp_signal_lookup_managed[i].scale;
	else
		return 0.;
}

int spm_template_hwi_dev::change_signal_input(int signal_index, gint32 input_id, gint32 voffset){
	gint32 si = signal_index | (voffset >= 0 && voffset < dsp_signal_lookup_managed[signal_index].dim ? voffset<<16 : 0); 
	SIGNAL_MANAGE sm = { input_id, si, 0, 0 }; // for read/write control part of signal_monitor only
	PI_DEBUG_GM (DBG_L3, "XX::change_module_signal_input");

        //
        // dummy
        // must adjust signal configurayion on controller here
        //
        
	PI_DEBUG_GM (DBG_L3, "XX::change_module_signal_input done: [%d,%d,0x%x,0x%x]",
                     sm.mindex,sm.signal_id,sm.act_address_input_set,sm.act_address_signal);
	return 0;
}

int spm_template_hwi_dev::query_module_signal_input(gint32 input_id){
        int mode=0;
        int ret;

        // template dummy -- query controller here:
        // read signal address at input_id, then lookup signal by address from map

        // use dummy list
	int signal_index = mod_input_list[input_id].id; // lookup_signal_by_ptr (sm.act_address_signal);

	return signal_index;
}

int spm_template_hwi_dev::read_signal_lookup (){
        // read actual signal list from controller and manage a copy
#if 0
	for (int i=0; i<NUM_SIGNALS; ++i){
		CONV_32 (dsp_signal_list[i].p);
		dsp_signal_lookup_managed[i].p = dsp_signal_list[i].p;
		dsp_signal_lookup_managed[i].dim   = dsp_signal_lookup[i].dim;
		dsp_signal_lookup_managed[i].label = g_strdup(dsp_signal_lookup[i].label);
                if (i==0){ // IN0 dedicated to tunnel current via IVC
                        // g_print ("1nA to Volt=%g  1pA to Volt=%g",main_get_gapp()->xsm->Inst->nAmpere2V (1.),main_get_gapp()->xsm->Inst->nAmpere2V (1e-3));
                        if (main_get_gapp()->xsm->Inst->nAmpere2V (1.) > 1.){
                                dsp_signal_lookup_managed[i].unit  = g_strdup("pA"); // use pA scale
                                dsp_signal_lookup_managed[i].scale = dsp_signal_lookup[i].scale; // -> Volts
                                dsp_signal_lookup_managed[i].scale /= main_get_gapp()->xsm->Inst->nAmpere2V (1); // values are always in nA
                        } else {
                                dsp_signal_lookup_managed[i].unit  = g_strdup("nA");
                                dsp_signal_lookup_managed[i].scale = dsp_signal_lookup[i].scale; // -> Volts
                                dsp_signal_lookup_managed[i].scale /= main_get_gapp()->xsm->Inst->nAmpere2V (1.); // nA
                        }
                } else {
                        dsp_signal_lookup_managed[i].unit  = g_strdup(dsp_signal_lookup[i].unit);
                        dsp_signal_lookup_managed[i].scale = dsp_signal_lookup[i].scale;
                }
		dsp_signal_lookup_managed[i].module  = g_strdup(dsp_signal_lookup[i].module);
		dsp_signal_lookup_managed[i].index  = -1;
                PI_DEBUG_PLAIN (DBG_L2,
                                "Sig[" << i << "]: ptr=" << dsp_signal_lookup_managed[i].p 
                                << ", " << dsp_signal_lookup_managed[i].dim 
                                << ", " << dsp_signal_lookup_managed[i].label 
                                << " [v" << dsp_signal_lookup_managed[i].index << "] "
                                << ", " << dsp_signal_lookup_managed[i].unit
                                << ", " << dsp_signal_lookup_managed[i].scale
                                << ", " << dsp_signal_lookup_managed[i].module
                                << "\n"
                                );
		for (int k=0; k<i; ++k)
			if (dsp_signal_lookup_managed[k].p == dsp_signal_lookup_managed[i].p){
                                PI_DEBUG_PLAIN (DBG_L2,
                                                "Sig[" << i << "]: ptr=" << dsp_signal_lookup_managed[i].p 
                                                << " identical with Sig[" << k << "]: ptr=" << dsp_signal_lookup_managed[k].p 
                                                << " ==> POSSIBLE ERROR IN SIGNAL TABLE <== GXSM is aborting here, suspicious DSP data.\n"
                                                );
                                g_warning ("DSP SIGNAL TABLE finding: Sig[%d] '%s': ptr=%x is identical with Sig[%d] '%s': ptr=%x",
                                           i, dsp_signal_lookup_managed[i].label, dsp_signal_lookup_managed[i].p,
                                           k, dsp_signal_lookup_managed[k].label, dsp_signal_lookup_managed[k].p);
				// exit (-1);
			}
	}
#endif
	return 0;
}

int spm_template_hwi_dev::read_actual_module_configuration (){
	for (int i=0; mod_input_list[i].id; ++i){
		int si = query_module_signal_input (mod_input_list[i].id);
		if (si >= 0 && si < NUM_SIGNALS){
                        PI_DEBUG_GM (DBG_L2, "INPUT %s (%04X) is set to %s",
                                     mod_input_list[i].name, mod_input_list[i].id,
                                     dsp_signal_lookup_managed[si].label );
		} else {
                        if (si == SIGNAL_INPUT_DISABLED)
                                PI_DEBUG_GM (DBG_L2, "INPUT %s (%04X) is DISABLED", mod_input_list[i].name, mod_input_list[i].id);
                        else
                                PI_DEBUG_GM (DBG_L2, "INPUT %s (%04X) -- ERROR DETECTED", mod_input_list[i].name, mod_input_list[i].id);
		}
	}

	return 0;
}
