/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 8 c-style: "K&R" -*- */

/* Gnome gxsm - Gnome X Scanning Microscopy
 * universal STM/AFM/SARLS/SPALEED/... controlling and
 * data analysis software
 *
 * Gxsm Hardware Interface Plugin Name: rpspmc_pacpll_hwi.C
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
#include "rpspmc_hwi_structs.h"
#include "rpspmc_pacpll.h"
#include "rpspmc_stream.h"

// Define HwI PlugIn reference name here, this is what is listed later within "Preferenced Dialog"
// i.e. the string selected for "Hardware/Card"!
#define THIS_HWI_PLUGIN_NAME "SPM-TEMPL:SPM"
#define THIS_HWI_PREFIX      "SPM_TEMPL_HwI"

extern int debug_level;
extern SOURCE_SIGNAL_DEF source_signals[];

extern "C++" {
        extern RPspmc_pacpll *rpspmc_pacpll;
        extern GxsmPlugin rpspmc_pacpll_hwi_pi;
}

extern "C++" {
        extern RPSPMC_Control *RPSPMC_ControlClass;
        extern GxsmPlugin rpspmc_hwi_pi;
}

extern const gchar *SPMC_GVP_VECTOR_COMPONENTS[];
extern const gchar *SPMC_SET_OFFSET_COMPONENTS[];
extern const gchar *SPMC_SET_SCANPOS_COMPONENTS[];

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


rpspmc_hwi_dev::rpspmc_hwi_dev(){

        // auto adjust and override preferences
        main_get_gapp()->xsm->Inst->override_dig_range (1<<19, xsmres); // gxsm does precision sanity checks and trys to round to best fit grid
        main_get_gapp()->xsm->Inst->override_volt_in_range (1.0, xsmres);
        main_get_gapp()->xsm->Inst->override_volt_out_range (5.0, xsmres);

        // SRCS Mapping for Scan Channels as set via Channelselector (independing from Graphs/GVP) in Gxsm, but here same masks as same GVP is used
        
        /*
        // do overrides/reconfigs of srcs mappings:
#define MSK_PID(X)  (1<<((X)&3)) // 0..3
#define MSK_DAQ(X)  (1<<((X)+4)) // 4..16
        // may be overridden/updated by HwI! Historic to Gxsm, but universal enough for future!
        
        void gxsm_init_dynamic_res(){
                for (int i=0; i<MAXPALANZ; ++i)
                        xsmres.PalPathList[i] = NULL;
                for (int i=0; i<PIDCHMAX; ++i){
                        xsmres.pidsrcZd2u[i] = 0.; 
                        xsmres.pidsrc_msk[i] = MSK_PID(i); // generate defaults
                }
                for (int i=0; i<DAQCHMAX; ++i){
                        xsmres.daqZd2u[i] = 0.;
                        xsmres.daq_msk[i] = MSK_DAQ(i); // generate defaults
                }
        }
        */

        xsmres.pidsrc_msk[2] = 0x000001;  // XS (Scan) non rot scan coord sys
        xsmres.pidsrc_msk[3] = 0x000002;  // YS (Scan) non rot scan coord sys
        xsmres.pidsrc_msk[0] = 0x000004;  // ZS-total
        xsmres.pidsrc_msk[1] = 0x000008;  // UMon Bias

        xsmres.daq_msk[0] = 0x000020;  // In2 = Current
        xsmres.daq_msk[1] = 0x000010;  // In1 = QP Signal
        xsmres.daq_msk[2] = 0x000040;  // In3 **
        xsmres.daq_msk[3] = 0x000080;  // In4 **
        xsmres.daq_msk[4] = 0x001000;  // LockInX
        xsmres.daq_msk[5] = 0x002000;  // dFreqCrtl value
        xsmres.daq_msk[6] = 0x004000;  // Time lower 32 (cycles at ~17sec)
        xsmres.daq_msk[7] = 0x00C000;  // time 64bit time tics (1/125 MHz resolution)

        xsmres.daq_msk[8]  = 0x000100;  // SWP signal PAC-PLL: defualt Phase
        xsmres.daq_msk[9]  = 0x000200;  // SWP signal PAC-PLL: defualt dFreq
        xsmres.daq_msk[10] = 0x000400;  // SWP signal PAC-PLL: defualt Ampl
        xsmres.daq_msk[12] = 0x000800;  // SWP signal PAC-PLL: defualt Exec
        
	probe_fifo_thread_active=0;

	subscan_data_y_index_offset = 0;
        ScanningFlg=0;
        KillFlg=FALSE;

        for (int i=0; i<4; ++i){
                srcs_dir[i] = nsrcs_dir[i] = 0;
                Mob_dir[i] = NULL;
        }

        //gint32 *GVP_stream_buffer=new gint32[0x1000000]; // temporary
}

rpspmc_hwi_dev::~rpspmc_hwi_dev(){
}


void rpspmc_hwi_dev::spmc_stream_connect_cb (GtkWidget *widget, rpspmc_hwi_dev *self){
        self->stream_connect_cb (gtk_check_button_get_active (GTK_CHECK_BUTTON (widget))); // connect (checked) or dissconnect
}

const gchar *rpspmc_hwi_dev::get_rp_address (){
        return rpspmc_pacpll->get_rp_address ();
}
void rpspmc_hwi_dev::status_append (const gchar *msg, bool schedule_from_thread){
        static gchar *buffer=NULL;
        if (schedule_from_thread){
                if (!buffer)
                        buffer = g_strdup (msg);
                else {
                        gchar *tmp = buffer;
                        buffer = g_strconcat (buffer, msg, NULL);
                        g_free (tmp);
                }
        } else {
                if (buffer){
                        rpspmc_pacpll->status_append (buffer);
                        g_free (buffer); buffer=NULL;
                }
                rpspmc_pacpll->status_append (msg);
        }
}
void rpspmc_hwi_dev::on_connect_actions(){
        status_append ("RedPitaya SPM Control Stream -- initializing.\n ");
}


int rpspmc_hwi_dev::RotateStepwise(int exec) {
        rpspmc_pacpll->write_parameter ("SPMC_ALPHA", Alpha);
        return 0;
}

gboolean rpspmc_hwi_dev::SetOffset(double x, double y){ // in "DIG"
        double jdata[3];
        jdata[0] = main_get_gapp()->xsm->Inst->XA2Volt (main_get_gapp()->xsm->Inst->Dig2XA (x));
        jdata[1] = main_get_gapp()->xsm->Inst->YA2Volt (main_get_gapp()->xsm->Inst->Dig2YA (y));
        jdata[2] = main_get_gapp()->xsm->Inst->XA2Volt (RPSPMC_ControlClass->move_speed_x);

        g_message ("Set OffsetXY: %g, %g D => %g, %g V @%gV/s", x,y, jdata[0], jdata[1], jdata[2]);

        if (rpspmc_pacpll)
                rpspmc_pacpll->write_array (SPMC_SET_OFFSET_COMPONENTS, 0, NULL,  3, jdata);

        return FALSE;
}

gboolean rpspmc_hwi_dev::MovetoXY (double x, double y){
        if (!ScanningFlg){
                double jdata[3];
                jdata[0] = main_get_gapp()->xsm->Inst->XA2Volt (main_get_gapp()->xsm->Inst->Dig2XA (x));
                jdata[1] = main_get_gapp()->xsm->Inst->YA2Volt (main_get_gapp()->xsm->Inst->Dig2YA (y));
                jdata[2] = main_get_gapp()->xsm->Inst->XA2Volt (RPSPMC_ControlClass->scan_speed_x_requested);

                g_message ("Set ScanPosXY: %g, %g D => %g, %g V @%gV/s", x,y, jdata[0], jdata[1], jdata[2]);

                if (rpspmc_pacpll)
                        rpspmc_pacpll->write_array (SPMC_SET_SCANPOS_COMPONENTS, 0, NULL,  3, jdata);
        }
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

int rpspmc_hwi_dev::GVP_expect_header(double *pv, int &index_all){
        int ret = -100;
        do {
                if (GVP_stream_buffer_AB >= 0){ // make sure stream actually started!
                        ret = read_GVP_data_block_to_position_vector (GVP_stream_buffer_offset, true); // NEED FULL HEADER

                        if (ret == -1){ // GOT FULL HEADER

                                GVP_stream_buffer_offset += 1 + GVP_vp_header_current.number_channels; // skip forward by read number of entries
                                
                                // next section, load header, manage pc
                                GVP_vp_header_current.section = RPSPMC_ControlClass->next_section (GVP_vp_header_current.section);
                                        
                                g_message (" *** OK, loading pv sec[%d] ***", GVP_vp_header_current.section);
                        
                                // copy header to pv[] as assigned below
                                pv[PROBEDATA_ARRAY_INDEX] = (double)index_all++;
                                pv[PROBEDATA_ARRAY_PHI]   = (double)GVP_vp_header_current.index; // testing, point index in section
                                pv[PROBEDATA_ARRAY_SEC]   = (double)GVP_vp_header_current.section;
                                // header ref
                                pv[PROBEDATA_ARRAY_XS]    = GVP_vp_header_current.dataexpanded[0]; // Xs in Volts
                                pv[PROBEDATA_ARRAY_YS]    = GVP_vp_header_current.dataexpanded[1]; // Ys in Volts
                                pv[PROBEDATA_ARRAY_ZS]    = GVP_vp_header_current.dataexpanded[2]; // Zs in Volts
                                pv[PROBEDATA_ARRAY_U]     = GVP_vp_header_current.dataexpanded[3]; // Bias in Volts
                                // data
                                pv[PROBEDATA_ARRAY_S1]    = GVP_vp_header_current.dataexpanded[0]; // Xs in Volts
                                pv[PROBEDATA_ARRAY_S2]    = GVP_vp_header_current.dataexpanded[1]; // Ys in Volts
                                pv[PROBEDATA_ARRAY_S3]    = GVP_vp_header_current.dataexpanded[2]; // Zs in Volts
                                pv[PROBEDATA_ARRAY_S4]    = GVP_vp_header_current.dataexpanded[3]; // Bias in Volts
                                pv[PROBEDATA_ARRAY_S5]    = GVP_vp_header_current.dataexpanded[4]; // IN1
                                pv[PROBEDATA_ARRAY_S6]    = GVP_vp_header_current.dataexpanded[5]; // IN2
                                pv[PROBEDATA_ARRAY_S7]    = GVP_vp_header_current.dataexpanded[6]; // IN3 **
                                pv[PROBEDATA_ARRAY_S8]    = GVP_vp_header_current.dataexpanded[7]; // IN4 **
                                pv[PROBEDATA_ARRAY_S9]    = GVP_vp_header_current.dataexpanded[8]; // DFREQ
                                pv[PROBEDATA_ARRAY_S10]   = GVP_vp_header_current.dataexpanded[9]; // EXEC
                                pv[PROBEDATA_ARRAY_S11]   = GVP_vp_header_current.dataexpanded[10]; // PHASE
                                pv[PROBEDATA_ARRAY_S12]   = GVP_vp_header_current.dataexpanded[11]; // AMPL
                                pv[PROBEDATA_ARRAY_S13]   = GVP_vp_header_current.dataexpanded[12]; // LCKInA
                                pv[PROBEDATA_ARRAY_S14]   = GVP_vp_header_current.dataexpanded[13]; // dFreqCtrl
                                pv[PROBEDATA_ARRAY_TIME]  = GVP_vp_header_current.dataexpanded[14]; // time in ms
                        
                                g_message ("PVh sec[%g] i=%g t=%g ms U=%g V IN1: %g V IN2: %g V",
                                           pv[PROBEDATA_ARRAY_SEC],pv[PROBEDATA_ARRAY_INDEX],
                                           pv[PROBEDATA_ARRAY_TIME],pv[PROBEDATA_ARRAY_U ], pv[PROBEDATA_ARRAY_S5], pv[PROBEDATA_ARRAY_S6]);

                                g_message ("Got Header. ret=%d", ret);
                                
                                return ret;
                        
                        } else {
                                if (GVP_vp_header_current.endmark){ // checking -- finished?
                                        g_message ("*** GVP: finished ***");
                                        return 0;
                                }
                        
                                if (ret > -99 && ret < -90){
                                        g_message ("*** GVP: DATA STREAM ERROR, NO VALID HEDAER AS EXPECTED -- aborting. EE ret = %d ***", ret);
                                        GVP_abort_vector_program ();
                                        RPSPMC_ControlClass->probe_ready = TRUE;
                                        return ret;
                                }
                        
                                if (GVP_stream_status < 0){
                                        g_message ("*** GVP: stream ended -- finished ***");
                                        return 0;
                                }

                                // -99 is only OK error, "wait, repeat, need more data"
                                g_message ("Scan VP: waiting for section header -- incomplete **");
                                usleep(100000);
                        }
                } else {
                        g_message ("Scan VP: waiting for header data");
                        usleep(100000);
                }
                
                if (GVP_vp_header_current.endmark){ // finished?
                        return 0;
                }
        } while (ret < -1 && ScanningFlg);

        if (!ScanningFlg){
                g_message("User Abort.");
                GVP_abort_vector_program ();
                RPSPMC_ControlClass->probe_ready = TRUE;
                return -9999;
        }

        return 0;
}

int rpspmc_hwi_dev::GVP_expect_point(double *pv, int &index_all){
        int ret = -100;

        do {
                ret = read_GVP_data_block_to_position_vector (GVP_stream_buffer_offset);
                if (ret == GVP_vp_header_current.number_channels){
                        GVP_stream_buffer_offset += 1 + GVP_vp_header_current.number_channels; // skip forward on success by read number of entries

                        g_message ("GVP point [%04d] ret (N#ch) = %d\n", GVP_vp_header_current.i, ret);

                        // copy vector data to expanded data array representation -- only active srcs will be updated between headers
                        pv[PROBEDATA_ARRAY_INDEX] = (double)index_all++;
                        /* (***) obsolete/skipping as direct lookup
                        pv[PROBEDATA_ARRAY_S1]    = GVP_vp_header_current.dataexpanded[0]; // Xs in Volts
                        pv[PROBEDATA_ARRAY_S2]    = GVP_vp_header_current.dataexpanded[1]; // Ys in Volts
                        pv[PROBEDATA_ARRAY_S3]    = GVP_vp_header_current.dataexpanded[2]; // Zs in Volts
                        pv[PROBEDATA_ARRAY_S4]    = GVP_vp_header_current.dataexpanded[3]; // Bias in Volts
                        pv[PROBEDATA_ARRAY_S5]    = GVP_vp_header_current.dataexpanded[4]; // IN1
                        pv[PROBEDATA_ARRAY_S6]    = GVP_vp_header_current.dataexpanded[5]; // IN2
                        pv[PROBEDATA_ARRAY_S7]    = GVP_vp_header_current.dataexpanded[6]; // IN3 **
                        pv[PROBEDATA_ARRAY_S8]    = GVP_vp_header_current.dataexpanded[7]; // IN4 **
                        pv[PROBEDATA_ARRAY_S9]    = GVP_vp_header_current.dataexpanded[8]; // DFREQ
                        pv[PROBEDATA_ARRAY_S10]   = GVP_vp_header_current.dataexpanded[9]; // EXEC
                        pv[PROBEDATA_ARRAY_S11]   = GVP_vp_header_current.dataexpanded[10]; // PHASE
                        pv[PROBEDATA_ARRAY_S12]   = GVP_vp_header_current.dataexpanded[11]; // AMPL
                        pv[PROBEDATA_ARRAY_S13]   = GVP_vp_header_current.dataexpanded[12]; // LCKInA
                        pv[PROBEDATA_ARRAY_S14]   = GVP_vp_header_current.dataexpanded[13]; // dFreqCtrl
                        pv[PROBEDATA_ARRAY_TIME]  = GVP_vp_header_current.dataexpanded[14]; // time in ms
                        */
                        if (GVP_vp_header_current.i == 0){
                                g_message ("*** GVP: section complete ***");
                                return 0; // OK done.
                        }
                        return  GVP_vp_header_current.i;
                } else {
                        if (GVP_vp_header_current.endmark){ // finished?
                                g_message ("*** GVP: finished -- end mark found ***");
                                return 0;
                        }


                        if (GVP_stream_status < 0){
                                g_message ("*** GVP: data stream ended -- finished ***");
                                return 0;
                        }

                        if (ret > -99 && ret < -90){
                                g_message ("*** GVP: DATA STREAM ERROR -- aborting. EE ret = %d ***", ret);
                                GVP_abort_vector_program ();
                                RPSPMC_ControlClass->probe_ready = TRUE;
                                return ret;
                        }

                        // -99 is only OK error, "wait, repeat, need more data"

                        //g_message ("Scan VP [EEOKwait, ret=%d]: waiting for data ** section: %d gvpi[%04d] Pos:{0x%04x} Offset:0x%04x",
                        //           ret, GVP_vp_header_current.section, GVP_vp_header_current.i, (int)spmc_parameters.gvp_data_position, GVP_stream_buffer_offset);
                        usleep(100000);
                }
        } while (ret < 0 && ScanningFlg);
        
        if (!ScanningFlg){
                g_message("User Abort.");
                GVP_abort_vector_program ();
                RPSPMC_ControlClass->probe_ready = TRUE;
                return -9999;
        }

        return GVP_vp_header_current.i;
}


gpointer ScanDataReadThread (void *ptr_hwi){
        double pv[NUM_PV_HEADER_SIGNALS];
        int point_index = 0;
        int index_all = 0;
        rpspmc_hwi_dev *hwi = (rpspmc_hwi_dev*)ptr_hwi;
        int nx,ny, x0,y0;
        int ret=0;
        int lut[4][NUM_PV_DATA_SIGNALS];

        RPSPMC_ControlClass->probe_ready = FALSE;

        for (int i=0; i<NUM_PV_HEADER_SIGNALS; ++i) pv[i]=0.;
        
        for (int dir=0; dir<4; ++dir)
                for (int ch=0; ch<NUM_PV_DATA_SIGNALS; ++ch)
                        lut[dir][ch]=0;

        // setup direct lookup from src ch and dir  to  GVP_vp_header_current.dataexpanded
        for (int dir = 0; dir < 4; ++dir)
                for (int ch=0; ch < hwi->nsrcs_dir[dir] && ch<NUM_PV_DATA_SIGNALS; ch++)
                        for (int i=0; source_signals[i].mask; ++i)
                                if (hwi->srcs_dir[dir] & source_signals[i].mask){
                                        if (source_signals[i].garr_index == PROBEDATA_ARRAY_TIME)
                                                lut[dir][ch] = 14;
                                        else
                                                lut[dir][ch] = source_signals[i].garr_index - PROBEDATA_ARRAY_S1; // as ..._S1 .. _S14 are exactly incremental => 0..13 ; see (***) above
                                        g_message ("GVP Data Expanded Lookup table signal %02d: lut[%d][%02d] = %d", i,dir,ch,lut[dir][ch]);
                                }
        
        g_message("FifoReadThread Start");

        if (hwi->Mob_dir[hwi->srcs_dir[0] ? 0:1]){
                ny = hwi->Mob_dir[hwi->srcs_dir[0] ? 0:1][0]->GetNySub(); // number lines to transfer
                nx = hwi->Mob_dir[hwi->srcs_dir[0] ? 0:1][0]->GetNxSub(); // number points per lines
                x0 = hwi->Mob_dir[hwi->srcs_dir[0] ? 0:1][0]->data->GetX0Sub(); // number points per lines
                y0 = hwi->Mob_dir[hwi->srcs_dir[0] ? 0:1][0]->data->GetY0Sub(); // number points per lines

                g_message("FifoReadThread nx,ny = (%d, %d) @ (%d, %d)", nx, ny, x0, y0);

        } else {
                RPSPMC_ControlClass->probe_ready = TRUE;
                return NULL; // error, no reference
        }
        
        // center_return_flag,
        // update:
        // scanpixelrate

        g_message ("last vector confirmed: %d, need %d", hwi->getVPCconfirmed (), hwi->last_vector_index);

        while (hwi->getVPCconfirmed () < hwi->last_vector_index){
                //g_message ("Waiting for GVP been written and confirmed. [Vector %d]", hwi->getVPCconfirmed ());
                usleep(20000);
        }
        g_message ("GVP been written and confirmed for vector #%d. Executing GVP now.", hwi->getVPCconfirmed ());


        // start GVP
        hwi->GVP_vp_init ();
        hwi->GVP_execute_vector_program(); // non blocking
        
        g_message ("Expect header for move to initial scan point...");
        // wait for and fetch header for move to scan start point
        if (hwi->GVP_expect_header (pv, index_all) != -1) return NULL;

        g_message ("Initial move hdr OK.");
        // fetch points but discard
        g_message ("point i=%d",  hwi->GVP_vp_header_current.i);
        g_message ("num points=%d", hwi->GVP_vp_header_current.n);
        g_message ("Fetch dummy points of move...");

        for (ret=-1; ret && hwi->ScanningFlg; ){
                ret = hwi->GVP_expect_point (pv, index_all);
                if (ret < 0) return NULL;
        }

        g_message ("Completed section: moved to 1st scan point");
        
	// hwi->RPSPMC_data_y_count == 0 for  top-down star tat line 0, else start line (last) bottom up
        g_message("FifoReadThread Scanning %s: %d", hwi->RPSPMC_data_y_count == 0 ? "Top Down" : "Bottom Up", hwi->ScanningFlg);

        int ydir = hwi->RPSPMC_data_y_count == 0 ? 1:-1;
        
        for (int yi=hwi->RPSPMC_data_y_count == 0 ?  y0 : y0+ny-1;     // ? top down : bottom up
             hwi->RPSPMC_data_y_count == 0 ? yi < y0+ny : yi-y0 >= 0;
             yi += ydir){ // for all lines and directional passes [->, <-, [-2>, <2-]]
                
                g_message ("scan line: #%d", yi);
                // for (int dir = 0; dir < 4 && hwi->ScanningFlg; ++dir){ // check for every pass -> <- 2> <2
                for (int dir = 0; dir < 2 && hwi->ScanningFlg; ++dir){ // check for every pass -> <- for first test only
                        // wait for and fetch header for first scan line
                        g_message ("scan line %d, dir=%d -- expect header.", yi, dir);
                        if (hwi->GVP_expect_header (pv, index_all) != -1) return NULL;  // this get includes the first data point and full position
                        
                        g_message ("scan line %d, dir=%d -- expecting points now...", yi, dir);
                        //g_message("FifoReadThread ny = %d, dir = %d, nsrcs = %d, srcs = 0x%04X", yi, dir, hwi->nsrcs_dir[dir], hwi->srcs_dir[dir]);
                        if (hwi->nsrcs_dir[dir] == 0){ // direction pass active?
                                // fetch points but discard
                                g_message ("discarding not selcted scan axis data...");
                                for (ret=-1; ret && hwi->ScanningFlg; ){
                                        ret = hwi->GVP_expect_point (pv, index_all);
                                        if (ret < 0) return NULL;
                                }
                                continue; // not selected?
                        } else {
                                // sanity check
                                if (hwi->GVP_vp_header_current.n != nx)
                                        g_warning ("GVP SCAN ERROR: NX != GVP.current.n: %d, %d", nx, hwi->GVP_vp_header_current.n);
                                int ret2;
                                //for (int xi=x0, ret2=ret=1; xi < x0+nx && ret; ++xi){ // all points per line
                                for (int xi=x0, ret2=ret=1; ret; ++xi){ // all points per line
                                        g_message ("Got point data [N-%d] for: xi=%d [x0=%d nx=%d]",  hwi->GVP_vp_header_current.i, xi, x0, nx);
                                        if (ret < 0){
                                                g_message("STREAM ERROR, FifoReadThread Aborting.");
                                                hwi->GVP_abort_vector_program ();
                                                RPSPMC_ControlClass->probe_ready = TRUE;
                                                return NULL;
                                        }
                                        hwi->RPSPMC_data_x_index = xi;
                                        for (int ch=0; ch<hwi->nsrcs_dir[dir]; ++ch){
                                                int k = lut [dir][ch];
                                                if (k < 0 || k >= NUM_PV_DATA_SIGNALS){
                                                        g_message ("EEEE lookup corruption => Lut[][] is fucked: invalid k=%d",k);
                                                        k=4;
                                                }
                                                g_message("PUT DATA POINT dir[%d] ch[%d] xy[%d, %d] kch[%d] = %g V", dir,ch,xi,yi,k,hwi->GVP_vp_header_current.dataexpanded [k]);
                                                if (hwi->Mob_dir[dir][ch]){
                                                        if (xi >=0 && yi >=0 && xi < hwi->Mob_dir[dir][ch]->GetNx ()  && yi < hwi->Mob_dir[dir][ch]->GetNy ())
                                                                hwi->Mob_dir[dir][ch]->PutDataPkt (hwi->GVP_vp_header_current.dataexpanded [k], xi, yi);
                                                        else
                                                                g_message ("EEEE xi, yi index out of range: [0..%d-1, 0..%d-1]", hwi->Mob_dir[dir][ch]->GetNx (), hwi->Mob_dir[dir][ch]->GetNy ());
                                                } else 
                                                        g_message("SCAN MOBJ[%d][%d] ERROR", dir,ch);
                                        }
                                        if ((ret=ret2)) // delay one
                                                if (ret)
                                                        ret2 = hwi->GVP_expect_point (pv, index_all);
                                }
                                //g_print("\n");
                        }
                }
                g_message ("Completed scan line, move to next y...");

                if (hwi->GVP_vp_header_current.endmark){ // checking -- finished?
                        g_message("GVP STREAM END DETECTED, FifoReadThread Aborting.");
                        RPSPMC_ControlClass->probe_ready = TRUE;
                        return NULL;
                }
                
                // wait for and fetch header for move to next scan line start point
                if (hwi->GVP_expect_header (pv, index_all) != -1) return NULL;
                // fetch points but discard
                for (ret=-1; ret && hwi->ScanningFlg; ){
                        ret = hwi->GVP_expect_point (pv, index_all);
                        if (ret < 0) return NULL;
                }

                if (hwi->GVP_vp_header_current.endmark){ // checking -- finished?
                        g_message("GVP STREAM END DETECTED, FifoReadThread Aborting.");
                        RPSPMC_ControlClass->probe_ready = TRUE;
                        return NULL;
                }
                
                hwi->RPSPMC_data_y_count = yi-y0; // completed
                hwi->RPSPMC_data_y_index = yi; // completed
        }

        RPSPMC_ControlClass->probe_ready = TRUE;
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
gpointer ProbeDataReadThread (void *ptr_hwi){
	int finish_flag=FALSE;
	rpspmc_hwi_dev *hwi = (rpspmc_hwi_dev*)ptr_hwi;
        g_message("ProbeFifoReadThread ENTER");

	if (hwi->probe_fifo_thread_active > 0){
                g_message("ProbeFifoReadThread ERROR: Attempt to start again while in progress! [%d]", hwi->probe_fifo_thread_active );
		//LOGMSGS ( "ProbeFifoReadThread ERROR: Attempt to start again while in progress! [#" << hwi->probe_fifo_thread_active << "]" << std::endl);
		return NULL;
	}
	hwi->probe_fifo_thread_active++;
	RPSPMC_ControlClass->probe_ready = FALSE;

        hwi->ReadProbeData (0, FR_INIT); // init
        
        // normal mode, wait for finish (single shot probe exec by user)
	if (RPSPMC_ControlClass->probe_trigger_single_shot)
		 finish_flag=TRUE;

        g_message("ProbeFifoReadThread ** starting processing loop ** FF=%s", finish_flag?"True":"False");
	while (hwi->is_scanning () || finish_flag){ // while scanning (raster mode) or until single shot probe is finished

                if (hwi->ReadProbeData () || hwi->abort_GVP_flag){ // True when finished or cancelled
                
                        g_message("ProbeFifoReadThread ** Finished ** FF=%s", finish_flag?"True":"False");
                        if (finish_flag || hwi->abort_GVP_flag){
                                if (RPSPMC_ControlClass->current_auto_flags & FLAG_AUTO_PLOT)
                                        RPSPMC_ControlClass->Probing_graph_update_thread_safe (1);
				
                                if (RPSPMC_ControlClass->current_auto_flags & FLAG_AUTO_SAVE)
                                        RPSPMC_ControlClass->Probing_save_callback (NULL, RPSPMC_ControlClass);	

                                break;
                        }
                        RPSPMC_ControlClass->push_probedata_arrays ();
                        RPSPMC_ControlClass->init_probedata_arrays ();
                }
                if (RPSPMC_ControlClass->current_auto_flags & FLAG_AUTO_PLOT)
                        RPSPMC_ControlClass->Probing_graph_update_thread_safe ();
	}

	--hwi->probe_fifo_thread_active;
	RPSPMC_ControlClass->probe_ready = TRUE;

        g_message("ProbeFifoReadThread EXIT");

	return NULL;
}

int rpspmc_hwi_dev::on_new_data (gconstpointer contents, gsize len, int position, int new_count, bool last){

        //gint32 *GVP_stream_buffer=NULL;

        if (position==0)
                memset (GVP_stream_buffer, 0xee, EXPAND_MULTIPLES*DMA_SIZE*sizeof(gint32));

        memcpy (&GVP_stream_buffer[position], contents, len);

        GVP_stream_buffer_position = position + (len>>2);
#if 0
        //gchar *tmp = g_strdup_printf ("WS-BUFFER-DATA_AB%03d_Off%0x08d_Pos0x%04x_GVPPos%0x08d.bin", AB, offset, position,  GVP_stream_buffer_position);
        gchar *tmp = g_strdup_printf ("GXSM-BUFFER-DATA_Off_%08d_GVPPos0x%08x.bin", position, GVP_stream_buffer_position);
        FILE *pFile = fopen(tmp, "wb");
        g_free (tmp);
        fwrite(&GVP_stream_buffer[0], 1, 4*GVP_stream_buffer_position, pFile);
        fclose(pFile);
        // hexdump -v -e '"%08_ax: "' -e ' 16/4 "%08x_L[red:0x018ec108,green:0x018fffff] " " \n"' WS-BRAM-DATA-BLOCK_000_Pos0x1f7e_AB_00.bin
#endif

        //g_message ("on_new_data ** AB=%d pos=%d  buffer_pos=0x%08x  new_count=%d  %s",
        //           AB, position, GVP_stream_buffer_position, new_count, last? "finished":"...");

        GVP_stream_buffer_AB = 1; // update now after memcopy
        return 0;
}

// ReadProbeData:
// read from probe data FIFO, this engine needs to be called several times from master thread/function
int rpspmc_hwi_dev::ReadProbeData (int dspdev, int control){
        static int recursion_level=0;
	static double pv[NUM_PV_HEADER_SIGNALS];
	static int point_index = 0;
        static int index_all = 0;
	static int end_read = 0;
	static int need_fct = FR_YES;  // need fifo control
	static int need_hdr = FR_YES;  // need header
	static int need_data = FR_YES; // need data
        int ret=0;
        
        //std::ostringstream stream;
#ifdef LOGMSGS0
	static double dbg0=0., dbg1=0.;
	static int dbgi0=0;
#endif

	switch (control){
	case FR_FIFO_FORCE_RESET: // not used normally -- FIFO is reset by DSP at probe_init (single probe)
                return ReadProbeData (0, FR_FINISH); // finish

	case FR_INIT:
                g_message ("VP INIT: ReadProbeData rl=%d", recursion_level);
                recursion_level=1;
                
                g_message ("VP: init");
                for (int i=0; i<NUM_PV_HEADER_SIGNALS; ++i) pv[i]=0.;

                g_message ("last vector confirmed: %d, need %d", getVPCconfirmed (), last_vector_index);
                // wait for vectors been confirmed
                while (getVPCconfirmed () < last_vector_index){
                        usleep(20000);
                }
                g_message ("GVP been written and confirmed for vector #%d. Executing GVP now.", getVPCconfirmed ());
                GVP_vp_init (); // vectors should be written by now!
                GVP_execute_vector_program(); // non blocking

                index_all = 0;
		point_index = 0;
                //                index_all = 0;

		need_fct  = FR_YES; // Fifo Control
		need_hdr  = FR_YES; // Header
		need_data = FR_YES; // Data

		RPSPMC_ControlClass->init_probedata_arrays ();
		for (int i=0; i<16; GVP_vp_header_current.dataexpanded[i++]=0.);

		LOGMSGS ( std::endl << "************** PROBE FIFO-READ INIT **************" << std::endl);
		LOGMSGS ( "FR::INIT-OK." << std::endl);
		return RET_FR_OK; // init OK.

	case FR_FINISH:
                RPSPMC_ControlClass->add_probedata (GVP_vp_header_current.dataexpanded, pv, true, true);
                --recursion_level;
                LOGMSGS ( "FR::FINISH-OK." << std::endl);
                g_message ( "FR::FINISH-OK");
		return RET_FR_FCT_END; // finish OK.

	default: break;
	}

        if (recursion_level > 2){
                g_warning ("ReadProbeData rl=%d", recursion_level);
                usleep(200000);
                return ReadProbeData (0, FR_FINISH); // finish
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
                // READ FULL SECTION HEADER
                g_message ("VP: Waiting for Section Header [%d] StreamPos=0x%08x", GVP_stream_buffer_AB, GVP_stream_buffer_position);
                if (GVP_stream_buffer_AB >= 0){
                        g_message ("VP: section header ** reading pos[%04x] off[%04x] #AB=%d", GVP_stream_buffer_position, GVP_stream_buffer_offset, GVP_stream_buffer_AB);
                        g_message ("Reading VP section header...");
                        ret = read_GVP_data_block_to_position_vector (GVP_stream_buffer_offset, true); // NEED FULL HEADER
                        if (ret == -1){ // GOT FULL HEADER

                                GVP_stream_buffer_offset += 1 + GVP_vp_header_current.number_channels; // skip forward by read number of entries
                                
                                // next section, load header, manage pc
                                GVP_vp_header_current.section = RPSPMC_ControlClass->next_section (GVP_vp_header_current.section);
                                        
                                g_message (" *** OK, loading pv sec[%d] ***", GVP_vp_header_current.section);

                                // copy header to pv[] as assigned below
                                pv[PROBEDATA_ARRAY_INDEX] = (double)index_all++;
                                pv[PROBEDATA_ARRAY_PHI]   = (double)GVP_vp_header_current.index; // testing, point index in section
                                pv[PROBEDATA_ARRAY_SEC]   = (double)GVP_vp_header_current.section;
                                pv[PROBEDATA_ARRAY_TIME]  = GVP_vp_header_current.dataexpanded[14]; // time in ms
                                pv[PROBEDATA_ARRAY_XS]    = GVP_vp_header_current.dataexpanded[0]; // Xs in Volts
                                pv[PROBEDATA_ARRAY_YS]    = GVP_vp_header_current.dataexpanded[1]; // Ys in Volts
                                pv[PROBEDATA_ARRAY_ZS]    = GVP_vp_header_current.dataexpanded[2]; // Zs in Volts
                                pv[PROBEDATA_ARRAY_U ]    = GVP_vp_header_current.dataexpanded[3]; // Bias in Volts

                                g_message ("PVh sec[%g] i=%g t=%g ms U=%g V",
                                           pv[PROBEDATA_ARRAY_SEC],pv[PROBEDATA_ARRAY_INDEX],
                                           pv[PROBEDATA_ARRAY_TIME],pv[PROBEDATA_ARRAY_U ]);

                        } else {
                                if (GVP_vp_header_current.endmark){ // checking -- finished?
                                        // add last point
                                        RPSPMC_ControlClass->add_probedata (GVP_vp_header_current.dataexpanded, pv, false, true);
                                        g_message ("*** GVP: finished ***");
                                        return ReadProbeData (0, FR_FINISH); // finish
                                }

                                if (ret > -99 && ret < -90){
                                        g_message ("*** GVP: DATA STREAM ERROR, NO VALID HEDAER AS EXPECTED -- aborting. EE ret = %d ***", ret);
                                        return ReadProbeData (0, FR_FINISH); // finish
                                }

                                if (GVP_stream_status < 0){
                                        g_message ("*** GVP: stream ended -- finished ***");
                                        return ReadProbeData (0, FR_FINISH); // finish
                                }
                                
                                g_message ("VP: waiting for section header -- incomplete **");
                                usleep(100000);
                                return RET_FR_OK;
                        }
                } else {
                        g_message ("VP: waiting for data");
                        usleep(100000);
                        return RET_FR_OK;
                }
                
                if (GVP_vp_header_current.endmark){ // finished?
                        return ReadProbeData (0, FR_FINISH);
                }
                
                RPSPMC_ControlClass->add_probedata (GVP_vp_header_current.dataexpanded, pv, true, true);

                point_index = 0;

                need_hdr = FR_NO;
        }

        // KEED READING DATA...
        
        clock_t ct = clock();
        //g_message ("VP: section: %d gvpi[%04d]", GVP_vp_header_current.section, GVP_vp_header_current.i);
        for (; GVP_vp_header_current.i < GVP_vp_header_current.n; ){
                ret = read_GVP_data_block_to_position_vector (GVP_stream_buffer_offset);
                if (ret == GVP_vp_header_current.number_channels){
                        GVP_stream_buffer_offset += 1 + GVP_vp_header_current.number_channels; // skip forward on success by read number of entries

                        //g_message ("VP [%04d] of %d\n", GVP_vp_header_current.i, GVP_vp_header_current.n);

                        // add vector and data to expanded data array representation
                        pv[PROBEDATA_ARRAY_INDEX] = (double)index_all++;
                        RPSPMC_ControlClass->add_probedata (GVP_vp_header_current.dataexpanded, pv, false, true); //(GVP_vp_header_current.i < (GVP_vp_header_current.n-1)) ? true:false);

                } else {
                        if (GVP_vp_header_current.endmark){ // finished?
                                g_message ("*** GVP: finished -- end mark found ***");
                                return ReadProbeData (0, FR_FINISH); // finish
                        }

                        if (ret > -99 && ret < -90){
                                g_message ("*** GVP: DATA STREAM ERROR -- aborting. EE ret = %d ***", ret);
                                return ReadProbeData (0, FR_FINISH); // finish
                        }

                        if (GVP_stream_status < 0){
                                g_message ("*** GVP: data stream ended -- finished ***");
                                return ReadProbeData (0, FR_FINISH); // finish
                        }

                        // g_message ("VP: waiting for data ** section: %d gvpi[%04d] Pos:{0x%04x} Offset:0x%04x", GVP_vp_header_current.section, GVP_vp_header_current.i, (int)spmc_parameters.gvp_data_position, GVP_stream_buffer_offset);
                        //usleep(100000);
                        return RET_FR_OK; // wait for data
                }
                        
                if (!GVP_vp_header_current.i || (clock () - ct)*25/CLOCKS_PER_SEC > 1) break; // for graph updates
	}

        if (GVP_vp_header_current.i == 0){
                LOGMSGS ( "FR:FIFO NEED FCT" << std::endl);
                need_fct = FR_YES;
                need_hdr = FR_YES;
        }
        
	return RET_FR_OK;
}



// prepare, create and start g-thread for data transfer
int rpspmc_hwi_dev::start_data_read (int y_start, 
                                            int num_srcs0, int num_srcs1, int num_srcs2, int num_srcs3, 
                                            Mem2d **Mob0, Mem2d **Mob1, Mem2d **Mob2, Mem2d **Mob3){

	PI_DEBUG_GP (DBG_L1, "HWI-SPM-TEMPL-DBGI mk2::start fifo read\n");

        g_message("Setting up FifoReadThread");

	if (num_srcs0 || num_srcs1 || num_srcs2 || num_srcs3){ // setup streaming of scan data
                g_message("... for scan.");

                double slew[2];
                slew[0] = RPSPMC_ControlClass->scan_speed_x = RPSPMC_ControlClass->scan_speed_x_requested;
                slew[1] = RPSPMC_ControlClass->fast_return * RPSPMC_ControlClass->scan_speed_x_requested;
                RPSPMC_ControlClass->scanpixelrate = slew[0]/main_get_gapp()->xsm->data.s.rx*main_get_gapp()->xsm->data.s.nx;
                // x2nd_Zoff

                int subscan[4];
                if (Mob_dir[srcs_dir[0] ? 0:1]){
                        subscan[3] = Mob_dir[srcs_dir[0] ? 0:1][0]->GetNySub(); // number lines to transfer
                        subscan[1] = Mob_dir[srcs_dir[0] ? 0:1][0]->GetNxSub(); // number points per lines
                        subscan[0] = Mob_dir[srcs_dir[0] ? 0:1][0]->data->GetX0Sub(); // number points per lines
                        subscan[2] = Mob_dir[srcs_dir[0] ? 0:1][0]->data->GetY0Sub(); // number points per lines
                } else return 0; // error, no reference
                
                // write scan program

                resetVPCconfirmed ();
                RPSPMC_ControlClass->write_spm_scan_vector_program (main_get_gapp()->xsm->data.s.rx, main_get_gapp()->xsm->data.s.ry,
                                                                    main_get_gapp()->xsm->data.s.nx, main_get_gapp()->xsm->data.s.ny,
                                                                    slew, subscan, srcs_dir);
                

                g_message ("Scan GVP written and send. Preparing for start.");

                ScanningFlg=1; // can be used to abort data read thread. (Scan Stop forced by user)
                RPSPMC_data_y_count = y_start; // if > 0, scan dir is "bottom-up"
		main_get_gapp()->monitorcontrol->LogEvent ("Starting Data read thread.","*");
                
                data_read_thread = g_thread_new ("FifoReadThread", ScanDataReadThread, this);
                
	}
	else{ // expect and stream probe data
                g_message("... for probe.");
		//if (RPSPMC_ControlClass->vis_Source & 0xffff)
                RPSPMC_ControlClass->probe_trigger_single_shot = 1;
                probe_data_read_thread = g_thread_new ("ProbeFifoReadThread", ProbeDataReadThread, this);
	}
      
	return 0;
}

// ScanLineM():
// Scan setup: (yindex=-2),
// Scan init: (first call with yindex >= 0)
// while scanning following calls are progress checks (return FALSE when yindex line data transfer is completed to go to next line for checking, else return TRUE to continue with this index!
gboolean rpspmc_hwi_dev::ScanLineM(int yindex, int xdir, int muxmode,
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
                y_current = RPSPMC_data_y_index;

                if (ydir > 0 && yindex <= RPSPMC_data_y_index){
                        g_print ("sranger_mk3_hwi_spm::ScanLineM(yindex=%d [fifo-y=%d], xdir=%d, ydir=%d, lssrcs=%x) y done.\n", yindex, RPSPMC_data_y_index, xdir, ydir, muxmode);
                        return FALSE; // line completed top-down
                }
                if (ydir < 0 && yindex >= RPSPMC_data_y_index){
                        g_print ("sranger_mk3_hwi_spm::ScanLineM(yindex=%d [fifo-y=%d], xdir=%d, ydir=%d, lssrcs=%x) y done.\n", yindex, RPSPMC_data_y_index, xdir, ydir, muxmode);
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
 "R" :                Scan Pos ZS, XS, YS -- in Angstroem/base unit
 "f" :                dFreq, I-avg, I-RMS
 "s","S" :            DSP Statemachine Status Bits, DSP load, DSP load peak
 "Z" :                probe Z Position
 "i" :                GPIO (high level speudo monitor)
 "A" :                Mover/Wave axis counts 0,1,2 (X/Y/Z)
 "p" :                X,Y Scan/Probe Coords in Pixel, 0,0 is center, DSP Scan Coords
 "P" :                X,Y Scan/Probe Coords in Pixel, 0,0 is top left [indices]
 "B" :                Bias
*/

gint rpspmc_hwi_dev::RTQuery (const gchar *property, double &val1, double &val2, double &val3){
        if (*property == 'z'){ // Scan Coordinates: ZScan, XScan, YScan  with offset!! -- in volts with gains!
		val1 = spmc_parameters.z_monitor*main_get_gapp()->xsm->Inst->VZ();
		val2 = spmc_parameters.x_monitor*main_get_gapp()->xsm->Inst->VX();
                val3 = spmc_parameters.y_monitor*main_get_gapp()->xsm->Inst->VY();
		return TRUE;
	}

	if (*property == 'o' || *property == 'O'){ // Offsets: Z0, X0, Y0  offset -- in volts after piezo amplifier
                val1 =  spmc_parameters.z0_monitor * main_get_gapp()->xsm->Inst->VZ();
                val2 =  spmc_parameters.x0_monitor * main_get_gapp()->xsm->Inst->VX();
                val3 =  spmc_parameters.y0_monitor * main_get_gapp()->xsm->Inst->VY();
		
		return TRUE;
	}

        // ZXY in Angstroem
        if (*property == 'R'){ // Scan Pos ZS, XS, YS -- without offset in Angstroem
		val1 =  main_get_gapp()->xsm->Inst->Volt2ZA (spmc_parameters.zs_monitor);
		val2 =  main_get_gapp()->xsm->Inst->Volt2XA (spmc_parameters.xs_monitor);
                val3 =  main_get_gapp()->xsm->Inst->Volt2YA (spmc_parameters.zs_monitor);
		return TRUE;
        }

        if (*property == 'f'){
                val1 = pacpll_parameters.dfreq_monitor; // qf - DSPPACClass->pll.Reference[0]; // Freq Shift
		val2 = spmc_parameters.signal_monitor / main_get_gapp()->xsm->Inst->nAmpere2V (1.); // actual nA reading
		val3 =  0; //
		return TRUE;
	}

        if (*property == 'Z'){ // probe Z -- well total Z here again
                val1 = rpspmc_pacpll_hwi_pi.app->xsm->Inst->Volt2ZA (spmc_parameters.z_monitor); // Z pos in Ang
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
                int statusbits = round(spmc_parameters.gvp_status);
		val3 = (double)(statusbits>>8);   //  assign dbg_status = {sec[32-3:0], reset, pause, ~finished };
		val2 = (double)(statusbits&0xff);
		val1 = (double)(    (statusbits&0x01       ? 1:0)      // (Z Servo Feedback Active: Bit0)
				+ ((((statusbits>>8)&0x04) ? 1:0) << 1)  // Scan/GVP Stop/Reset    (Scan)
                                + ((((statusbits>>8)&0x02) ? 1:0) << 2)  // Scan/GVP Pause         (Scan)
				+ ((((statusbits>>8)&0x01) ? 1:0) << 3)  // Scan/GVP not finished  (Probing)
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
                val1 = (double)( RPSPMC_data_x_index - (main_get_gapp()->xsm->data.s.nx/2 - 1) + 1);
                val2 = (double)(-RPSPMC_data_y_index + (main_get_gapp()->xsm->data.s.ny/2 - 1) + 1);
                val3 = (double)RPSPMC_data_z_value;
		return TRUE;
        }
        if (*property == 'P'){ // SCAN DATA INDEX, range: 0..NX, 0..NY
                val1 = (double)(RPSPMC_data_x_index);
                val2 = (double)(RPSPMC_data_y_index);
                val3 = (double)RPSPMC_data_z_value;
		return TRUE;
        }
        if (*property == 'B'){ // Monitors: Bias, ...
                val1 = spmc_parameters.bias_monitor;
                val2 = 0.0;
                val3 = 0.0;
		return TRUE;
        }
//	printf ("ZXY: %g %g %g\n", val1, val2, val3);

//	val1 =  (double)dsp_analog_out.z_scan;
//	val2 =  (double)dsp_analog_out.x_scan;
//	val3 =  (double)dsp_analog_out.y_scan;

	return TRUE;
}

// template/dummy signal management



int rpspmc_hwi_dev::lookup_signal_by_ptr(gint64 sigptr){
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

int rpspmc_hwi_dev::lookup_signal_by_name(const gchar *sig_name){
	for (int i=0; i<NUM_SIGNALS; ++i)
		if (!strcmp (sig_name, dsp_signal_lookup_managed[i].label))
			return i;
	return -1;
}

const gchar *rpspmc_hwi_dev::lookup_signal_name_by_index(int i){
	if (i<NUM_SIGNALS && i >= 0){
		// dsp_signal_lookup_managed[i].index
		return (const gchar*)dsp_signal_lookup_managed[i].label;
	} else
		return "INVALID INDEX";
}

const gchar *rpspmc_hwi_dev::lookup_signal_unit_by_index(int i){
	if (i<NUM_SIGNALS && i >= 0)
		return (const gchar*)dsp_signal_lookup_managed[i].unit;
	else
		return "INVALID INDEX";
}

double rpspmc_hwi_dev::lookup_signal_scale_by_index(int i){
	if (i<NUM_SIGNALS && i >= 0)
		return dsp_signal_lookup_managed[i].scale;
	else
		return 0.;
}

int rpspmc_hwi_dev::change_signal_input(int signal_index, gint32 input_id, gint32 voffset){
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

int rpspmc_hwi_dev::query_module_signal_input(gint32 input_id){
        int mode=0;
        int ret;

        // template dummy -- query controller here:
        // read signal address at input_id, then lookup signal by address from map

        // use dummy list
	int signal_index = mod_input_list[input_id].id; // lookup_signal_by_ptr (sm.act_address_signal);

	return signal_index;
}

int rpspmc_hwi_dev::read_signal_lookup (){
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

int rpspmc_hwi_dev::read_actual_module_configuration (){
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


// GVP CONTROL MODES:
#define SPMC_GVP_CONTROL_RESET     0
#define SPMC_GVP_CONTROL_EXECUTE   1   
#define SPMC_GVP_CONTROL_PAUSE     2
#define SPMC_GVP_CONTROL_RESUME    3
#define SPMC_GVP_CONTROL_PROGRAM   4



void rpspmc_hwi_dev::GVP_execute_vector_program(){
        g_message ("rpspmc_hwi_dev::GVP_execute_vector_program ()");
        rpspmc_pacpll->write_parameter ("SPMC_GVP_CONTROL", SPMC_GVP_CONTROL_EXECUTE);
}

void rpspmc_hwi_dev::GVP_abort_vector_program (){
        g_message ("rpspmc_hwi_dev::GVP_abort_vector_program ()");
        rpspmc_pacpll->write_parameter ("SPMC_GVP_CONTROL", SPMC_GVP_CONTROL_RESET);
        abort_GVP_flag = true;
}


void rpspmc_hwi_dev::GVP_vp_init (){
        // reset GVP stream buffer read count
        GVP_stream_buffer_AB = -2;
        GVP_stream_buffer_position = 0;
        GVP_stream_buffer_offset = 0; // 0x100   //  =0x00 **** TESTING BRAM ISSUE -- FIX ME !!! *****
        GVP_stream_status=0;

        GVP_vp_header_current.endmark = 0;
        GVP_vp_header_current.n    = 0;
        GVP_vp_header_current.srcs = 0;
        GVP_vp_header_current.time = 0;
        GVP_vp_header_current.scan_xyz[i_X] = 0;
        GVP_vp_header_current.scan_xyz[i_Y] = 0;
        GVP_vp_header_current.scan_xyz[i_Z] = 0;
        GVP_vp_header_current.bias    = 0;
        GVP_vp_header_current.section = -1; // still invalid

        RPSPMC_GVP_section_count = 0;
        RPSPMC_GVP_n = 0;


        abort_GVP_flag = false;
}


void rpspmc_hwi_dev::rpspmc_hwi_dev::GVP_start_data_read(){
}

int rpspmc_hwi_dev::GVP_write_program_vector(int i, PROBE_VECTOR_GENERIC *v){
        if (i >= MAX_PROGRAM_VECTORS || i < 0)
                return 0;

#define I_GVP_PC_INDEX  0
#define I_GVP_N         1
#define I_GVP_OPTIONS   2
#define I_GVP_NREP      3
#define I_GVP_NEXT      4
#define I_GVP_SIZE (I_GVP_NEXT+1)
        
#define D_GVP_DX        0
#define D_GVP_DY        1
#define D_GVP_DZ        2
#define D_GVP_DU        3
#define D_GVP_AA        4
#define D_GVP_BB        5
#define D_GVP_SLW       6
#define D_GVP_SIZE (D_GVP_SLW+1)
        
        int gvp_vector_i[I_GVP_SIZE];
        double gvp_vector_d[D_GVP_SIZE];

        // Vector Program Code Setup
        gvp_vector_i [I_GVP_PC_INDEX] = i;
        gvp_vector_i [I_GVP_N       ] = v->n;
        gvp_vector_i [I_GVP_OPTIONS ] = ((v->srcs & 0xffffff) << 8) | (v->options & 0xff); //   ((v->options & VP_FEEDBACK_HOLD) ? 0:1) | (1<<7) | (1<<6) | (1<<5);
        g_message ("GVP_write_program_vector[%d]: srcs = 0x%08x", i, gvp_vector_i [I_GVP_OPTIONS ] );
        gvp_vector_i [I_GVP_NREP    ] = v->repetitions > 1 ? v->repetitions-1 : 0;
        gvp_vector_i [I_GVP_NEXT    ] = v->ptr_next;


        if (i == 0 && (v->options & VP_INITIAL_SET_VEC)){ // 1st vector is set postion vector? Get pos and calc differentials.
                gvp_vector_i [I_GVP_PC_INDEX] = 0x1000; // all componets are absolute set coordnates!
                // componets can be masked to set=0 via dX=0 0x1001, dY=0 0x1002, dZ=0 0x1004, du=0 0x1008,da=0  0x1010, db=0 0x1020
        }

        g_print ("Vec[%2d] XYZU: %g %g %g %g V  [#%d, R%d J%d SRCS=%08x] initial Msk=%04x\n",
                 i,
                 v->f_dx, v->f_dy, v->f_dz, v->f_du,
                 gvp_vector_i [I_GVP_N       ], gvp_vector_i [I_GVP_NREP    ], gvp_vector_i [I_GVP_NEXT    ], gvp_vector_i [I_GVP_OPTIONS ],
                 gvp_vector_i [I_GVP_PC_INDEX]);
        // Vector Components are all in Volts
        gvp_vector_d [D_GVP_DX      ] = v->f_dx;
        gvp_vector_d [D_GVP_DY      ] = v->f_dy;
        gvp_vector_d [D_GVP_DZ      ] = v->f_dz;
        gvp_vector_d [D_GVP_DU      ] = v->f_du;
        gvp_vector_d [D_GVP_AA      ] = v->f_da;
        gvp_vector_d [D_GVP_BB      ] = v->f_db;
        gvp_vector_d [D_GVP_SLW     ] = v->slew;
        
        // send it down
        if (rpspmc_pacpll)
                rpspmc_pacpll->write_array (SPMC_GVP_VECTOR_COMPONENTS,  I_GVP_SIZE, gvp_vector_i,  D_GVP_SIZE, gvp_vector_d);
                
        return -1;
}


