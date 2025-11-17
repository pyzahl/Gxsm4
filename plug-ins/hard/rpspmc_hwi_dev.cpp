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
extern SOURCE_SIGNAL_DEF rpspmc_source_signals[];
extern SOURCE_SIGNAL_DEF rpspmc_swappable_signals[];
extern SOURCE_SIGNAL_DEF z_servo_current_source[];

extern "C++" {
        extern RPspmc_pacpll *rpspmc_pacpll;
        extern GxsmPlugin rpspmc_pacpll_hwi_pi;
}

extern "C++" {
        extern RPSPMC_Control *RPSPMC_ControlClass;
        extern GxsmPlugin rpspmc_hwi_pi;
}


// HwI Implementation
// ================================================================================

rpspmc_hwi_dev::rpspmc_hwi_dev():RP_stream(this){
        g_mutex_init (&RTQmutex);
        
        z_offset_internal = 0.;
        info_blob = NULL;
        
        delayed_tip_move_update_timer_id = 0;
        rpspmc_history=NULL;
        history_block_size = 0;

        // auto adjust and override preferences
        main_get_gapp()->xsm->Inst->override_dig_range (1<<20, xsmres);    // gxsm does precision sanity checks and trys to round to best fit grid
        main_get_gapp()->xsm->Inst->override_volt_in_range (5.0, xsmres);  // FOR AD4630-24 24bit
        main_get_gapp()->xsm->Inst->override_volt_out_range (5.0, xsmres); // PMODs AD5791-20 20bit

        // SRCS Mapping for Scan Channels as set via Channelselector (independing from Graphs/GVP) in Gxsm, but here same masks as same GVP is used

	probe_fifo_thread_active=0;
        g_mutex_init (&GVP_stream_buffer_mutex);

        // Automatic overriding GXSM core resources for RPSPMC setup to map scan data sources via channel selector:

        // CHECK and init SWPS signals (GVP MUX MAPPING)
        if (RPSPMC_ControlClass)
                set_spmc_signal_mux (RPSPMC_ControlClass->scan_source);
        else {
                int defaults[6]={0,1,2,3,4,5};
                set_spmc_signal_mux (defaults);
        }
        
        update_hardware_mapping_to_rpspmc_source_signals ();

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


// use SOURCE_SIGNAL_DEF rpspmc_source_signals[] table to auto configure (Scan Sources Configurations mapping)
void rpspmc_hwi_dev::update_hardware_mapping_to_rpspmc_source_signals (){
        for (int i=0; rpspmc_source_signals[i].label; ++i){ // name
                g_message ("Reading SOURCE_SIGNALS[%d]",i);
                g_message ("Reading SOURCE_SIGNALS[%d].mask %x",i,rpspmc_source_signals[i].mask);
                g_message ("Reading SOURCE_SIGNALS[%d].label >%s<",i,rpspmc_source_signals[i].label);
                if (rpspmc_source_signals[i].scan_source_pos > 0){
                        double volt_to_unit = 1.0;
                        switch (rpspmc_source_signals[i].mask){
                        case 0x0001: volt_to_unit = XAngFac; break;
                        case 0x0002: volt_to_unit = YAngFac; break;
                        case 0x0004: volt_to_unit = ZAngFac; break;
                        case 0x0008: volt_to_unit = BiasFac; break;
                        case 0x0010: volt_to_unit = CurrFac; break;
                        }
                        main_get_gapp()->channelselector->ConfigureHardwareMapping (rpspmc_source_signals[i].scan_source_pos-1,
                                                                                    rpspmc_source_signals[i].label, rpspmc_source_signals[i].mask, // name
                                                                                    rpspmc_source_signals[i].label,
                                                                                    rpspmc_source_signals[i].unit, volt_to_unit); // conversion to Base Units in Volts is in stream expansion applied
                g_message ("SOURCE_SIGNAL_DEF %02d for %s mask: 0x%08x L: %s U: %s  x %g [to V] x %g [to %s]",
                           rpspmc_source_signals[i].scan_source_pos-1,
                           rpspmc_source_signals[i].label, rpspmc_source_signals[i].mask, // name
                           rpspmc_source_signals[i].label, rpspmc_source_signals[i].unit, rpspmc_source_signals[i].scale_factor, volt_to_unit, rpspmc_source_signals[i].unit);
                }
        }
}


void rpspmc_hwi_dev::spmc_stream_connect_cb (GtkWidget *widget, rpspmc_hwi_dev *self){
        self->stream_connect_cb (gtk_check_button_get_active (GTK_CHECK_BUTTON (widget))); // connect (checked) or dissconnect
}

const gchar *rpspmc_hwi_dev::get_rp_address (){
        return rpspmc_pacpll->get_rp_address ();
}

gboolean rpspmc_hwi_dev::update_status_idle(gpointer self){
        ((rpspmc_hwi_dev*)self) -> status_append(NULL, false);
        return G_SOURCE_REMOVE;
};
        
void rpspmc_hwi_dev::status_append (const gchar *msg, bool schedule_from_thread){
        static guint idle_func=0;
        static gchar *buffer=NULL;
        if (schedule_from_thread){
                if (!buffer)
                        buffer = g_strdup (msg);
                else {
                        gchar *tmp = buffer;
                        buffer = g_strconcat (buffer, msg, NULL); // append to buffer only, schedule idle if not yet exiting
                        g_free (tmp);
                        if (!idle_func)
                                idle_func = g_idle_add (update_status_idle, this);
                }
        } else {
                if (buffer){
                        if (idle_func)
                                idle_func = 0;
                        rpspmc_pacpll->status_append (buffer);
                        g_free (buffer); buffer=NULL;
                }
                if (msg)
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

gboolean rpspmc_hwi_dev::CenterZOffset(){
        z_offset_internal = 0.;
        ZOffsetMove (0.);
        return TRUE;
}

gboolean rpspmc_hwi_dev::ZOffsetMove(double dv){
        const gchar *SPMC_SET_ZOFFSET_COMPONENTS[] = {
                "SPMC_SET_OFFSET_Z", 
                "SPMC_SET_OFFSET_Z_SLEW",
                NULL };
        double jdata[2];

        z_offset_internal += dv;
        jdata[0] = main_get_gapp()->xsm->Inst->ZA2Volt (z_offset_internal);
        jdata[1] = main_get_gapp()->xsm->Inst->ZA2Volt (100.0); // 100 A/s

        g_message ("ZOffsetMove: %gV @%gV/s", jdata[0], jdata[1]);

        if (rpspmc_pacpll)
                rpspmc_pacpll->write_array (SPMC_SET_ZOFFSET_COMPONENTS, 0, NULL,  2, jdata);

        return FALSE;
}


gboolean rpspmc_hwi_dev::SetOffset(double x, double y){ // in "DIG"
        const gchar *SPMC_SET_OFFSET_COMPONENTS[] = {
                "SPMC_SET_OFFSET_X", 
                "SPMC_SET_OFFSET_Y", 
                "SPMC_SET_OFFSET_XY_SLEW",
                NULL };
        double jdata[3];
        jdata[0] = main_get_gapp()->xsm->Inst->XA2Volt (main_get_gapp()->xsm->Inst->Dig2XA (x));
        jdata[1] = main_get_gapp()->xsm->Inst->YA2Volt (main_get_gapp()->xsm->Inst->Dig2YA (y));
        jdata[2] = main_get_gapp()->xsm->Inst->XA2Volt (RPSPMC_ControlClass->move_speed_x);

        g_message ("Set OffsetXY: %g, %g D => %g, %g V @%gV/s", x,y, jdata[0], jdata[1], jdata[2]);

        if (rpspmc_pacpll)
                rpspmc_pacpll->write_array (SPMC_SET_OFFSET_COMPONENTS, 0, NULL,  3, jdata);

        return FALSE;
}

void rpspmc_hwi_dev::delayed_tip_move_update (){
        if (!ScanningFlg){
                const gchar *SPMC_SET_SCANPOS_COMPONENTS[] = {
                        "SPMC_SET_SCANPOS_OPTS", 
                        "SPMC_SET_SCANPOS_X", 
                        "SPMC_SET_SCANPOS_Y", 
                        "SPMC_SET_SCANPOS_SLEW", 
                        NULL };

                double jdata[3];
                int jdata_i[1];

                jdata_i[0] = 0x0001; // GVP OPTIONS: Feedback ON! *** WATCH THIS ***
                jdata[0] = main_get_gapp()->xsm->Inst->XA2Volt (main_get_gapp()->xsm->Inst->Dig2XA (requested_tip_move_xy[0]));
                jdata[1] = main_get_gapp()->xsm->Inst->YA2Volt (main_get_gapp()->xsm->Inst->Dig2YA (requested_tip_move_xy[1]));
                jdata[2] = RPSPMC_ControlClass->scan_speed_x_requested;
                
                g_message ("Set ScanPosXY (delayed): %g, %g D => %g, %g V @%gV/s", requested_tip_move_xy[0], requested_tip_move_xy[1], jdata[0], jdata[1], jdata[2]);

                if (rpspmc_pacpll)
                        rpspmc_pacpll->write_array (SPMC_SET_SCANPOS_COMPONENTS, 1, jdata_i,  3, jdata); // { OPTS, X, Y, SLEW }
        }
        delayed_tip_move_update_timer_id = 0; // done.
}

guint rpspmc_hwi_dev::delayed_tip_move_update_callback (rpspmc_hwi_dev *self){
        self->delayed_tip_move_update ();
	return G_SOURCE_REMOVE;
}

gboolean rpspmc_hwi_dev::MovetoXY (double x, double y){
        static gint64 t0=0;

        if (!ScanningFlg){
                requested_tip_move_xy[0]=x; // update
                requested_tip_move_xy[1]=y; // update
                g_message ("Update ScanPosXY: %g, %g Dt %g ms", requested_tip_move_xy[0], requested_tip_move_xy[1], (g_get_real_time ()-t0)*1e-3);
                // if not already scheduled, schedule delayed tip position moveto
                if (!delayed_tip_move_update_timer_id){ // schedule delayed, takes current/latest updated xy so!
                        t0 = g_get_real_time (); // us
                        g_message ("Scheduling Tip Move in 1s...");
                        delayed_tip_move_update_timer_id = g_timeout_add (1000, (GSourceFunc)rpspmc_hwi_dev::delayed_tip_move_update_callback, this);
                }
        }
        return FALSE;
}




// THREAD AND FIFO CONTROL STATES

#define GVP_DEBUG_VERBOSE 0

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
                                        
#if GVP_DEBUG_VERBOSE > 2
                                g_message (" *** OK, loading pv sec[%d] ***", GVP_vp_header_current.section);
#endif
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
                                pv[PROBEDATA_ARRAY_S9]    = GVP_vp_header_current.dataexpanded[8]; // DFREQ* (MUX0)
                                pv[PROBEDATA_ARRAY_S10]   = GVP_vp_header_current.dataexpanded[9]; // EXEC*  (MUX1)
                                pv[PROBEDATA_ARRAY_S11]   = GVP_vp_header_current.dataexpanded[10]; // PHASE* (MUX2)
                                pv[PROBEDATA_ARRAY_S12]   = GVP_vp_header_current.dataexpanded[11]; // AMPL*  (MUX3)
                                pv[PROBEDATA_ARRAY_S13]   = GVP_vp_header_current.dataexpanded[12]; // LCKInA* (MUX4)
                                pv[PROBEDATA_ARRAY_S14]   = GVP_vp_header_current.dataexpanded[13]; // dFreqCtrl* (MUX5)
                                pv[PROBEDATA_ARRAY_TIME]  = GVP_vp_header_current.dataexpanded[14]; // time in ms

#if GVP_DEBUG_VERBOSE > 2
                                g_message ("PVh sec[%g] i=%g t=%g ms U=%g V IN1: %g V IN2: %g V",
                                           pv[PROBEDATA_ARRAY_SEC],pv[PROBEDATA_ARRAY_INDEX],
                                           pv[PROBEDATA_ARRAY_TIME],pv[PROBEDATA_ARRAY_U ], pv[PROBEDATA_ARRAY_S5], pv[PROBEDATA_ARRAY_S6]);

                                g_message ("Got Header. ret=%d", ret);
#endif                           
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

#if GVP_DEBUG_VERBOSE > 2
                        g_message ("GVP point [%04d] ret (N#ch) = %d\n", GVP_vp_header_current.i, ret);
#endif
                        // copy vector data to expanded data array representation -- only active srcs will be updated between headers
                        pv[PROBEDATA_ARRAY_INDEX] = (double)index_all++;
                        if (GVP_vp_header_current.i == 0){
#if GVP_DEBUG_VERBOSE > 2
                                g_message ("*** GVP: section complete ***");
#endif
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
        rpspmc_hwi_dev *hwi = (rpspmc_hwi_dev*)ptr_hwi;
        int pvlut[4][NUM_PV_DATA_SIGNALS];
        double pv[MAX_NUM_CHANNELS]; // full pv vector
        int point_index = 0;
        int index_all = 0;
        int nx,ny, x0,y0;
        int ret=0;

        RPSPMC_ControlClass->probe_ready = FALSE;

        for (int i=0; i<MAX_NUM_CHANNELS; ++i) pv[i]=0.;

        for (int dir=0; dir<4; ++dir)
                for (int ch=0; ch<NUM_PV_DATA_SIGNALS; ++ch)
                        pvlut[dir][ch]=0;

        // setup direct lookup from src ch and dir  to  GVP_vp_header_current.dataexpanded
        for (int dir = 0; dir < 4; ++dir){
                int i=0;
                for (int ch=0; ch < hwi->nsrcs_dir[dir] && ch<NUM_PV_DATA_SIGNALS; ch++){
                        for (; rpspmc_source_signals[i].mask; ++i){
                                g_message ("GVP Data Expanded Lookup table signal dir %02d, ch %02d, ssi %02d: checking for mask 0x%08x (%s) match in 0x%08x", dir, ch, i, rpspmc_source_signals[i].mask, rpspmc_source_signals[i].label, hwi->srcs_dir[dir]);
                                if ((hwi->srcs_dir[dir] & rpspmc_source_signals[i].mask) == rpspmc_source_signals[i].mask){
                                        if (rpspmc_source_signals[i].garr_index == PROBEDATA_ARRAY_TIME)
                                                pvlut[dir][ch] = 14; /// NOTE: GVP_vp_header_current.dataexpanded[14]; // is time in ms
                                        else
                                                pvlut[dir][ch] = rpspmc_source_signals[i].garr_index - PROBEDATA_ARRAY_S1; // as ..._S1 .. _S14 are exactly incremental => 0..13 ; see (***) above
                                        g_message ("GVP Data Expanded Lookup table signal %02d: pvlut[%02d][%02d] = %02d for mask 0x%08x, garri %d", i,dir,ch,pvlut[dir][ch], rpspmc_source_signals[i].mask, rpspmc_source_signals[i].garr_index);
                                        ++i; break;
                                }
                        }
                }
        }

        // g_message("GVP Data Expanded Lookup table: PVLUT"); for (int dir = 0; dir < 4; ++dir){ for (int ch=0; ch<NUM_PV_DATA_SIGNALS; ch++){g_print ("%02d, ", pvlut[dir][ch]);}g_print ("\n");}
        
        //g_message("FifoReadThread Start");

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

        g_message ("last vector confirmed: %d, need %d", hwi->getVPCconfirmed (), hwi->last_vector_index);

        int timeout = 0;
        while (hwi->getVPCconfirmed () < hwi->last_vector_index){
#if GVP_DEBUG_VERBOSE > 4
                g_message ("Waiting for GVP-Scan been written and confirmed. [Vector %d/%d]", hwi->getVPCconfirmed (), hwi->last_vector_index);
#endif
                if (++timeout > 100){
                        g_message ("GVP-Scan program write and confirm failed. Aborting Scan.");
                        hwi->EndScan2D();
                        return NULL;
                }
                usleep(20000);
        }
        g_message ("GVP-Scan been written and confirmed for vector #%d. Init GVP. Executing GVP now.", hwi->getVPCconfirmed ());
        hwi->RPSPMC_data_y_index = hwi->RPSPMC_data_y_count == 0 ?  y0 : y0+ny-1;

        // start GVP
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
                ret = hwi->GVP_expect_point (pv, index_all); // returns section points count down to zero
                if (ret < 0) return NULL;
        }

        g_message ("Completed section: moved to 1st scan point");
        
	// hwi->RPSPMC_data_y_count == 0 for  top-down star tat line 0, else start line (last) bottom up
        g_message("FifoReadThread Scanning %s: %d", hwi->RPSPMC_data_y_count == 0 ? "Top Down" : "Bottom Up", hwi->ScanningFlg);

        int ydir = hwi->RPSPMC_data_y_count == 0 ? 1:-1;


        for (int yi=hwi->RPSPMC_data_y_count == 0 ?  y0 : y0+ny-1;     // ? top down : bottom up
             hwi->RPSPMC_data_y_count == 0 ? yi < y0+ny : yi-y0 >= 0;
             yi += ydir){ // for all lines and directional passes [->, <-, [-2>, <2-]]
                
#if GVP_DEBUG_VERBOSE > 0
                g_message ("*************************** scan line: #%d / %d   dyc: %d **********************", yi, ny+y0, hwi->RPSPMC_data_y_count);
#endif
                // for (int dir = 0; dir < 4 && hwi->ScanningFlg; ++dir){ // check for every pass -> <- 2> <2
                for (int dir = 0; dir < 2 && hwi->ScanningFlg; ++dir){ // check for every pass -> <- for first test only

                        // wait for and fetch header for first scan line
#if GVP_DEBUG_VERBOSE > 2
                        g_message ("scan line %d, dir=%d -- expect header.", yi, dir);
#endif
                        // this GVP expect header data includes the first data point and full position.
                        if (hwi->GVP_expect_header (pv, index_all) != -1) {
                                g_warning ("GVP SCAN ERROR: Invalid Header or Scan Aborted.");
                                return NULL;
                        }
                        
#if GVP_DEBUG_VERBOSE > 2
                        g_message ("Got header with 1st point of scan line %d, dir=%d -- expecting points now...", yi, dir);
                        g_message("FifoReadThread ny = %d, dir = %d, nsrcs = %d, srcs = 0x%04X", yi, dir, hwi->nsrcs_dir[dir], hwi->srcs_dir[dir]);
#endif
                        if (hwi->nsrcs_dir[dir] == 0){ // direction pass active?
                                // fetch points but discard
#if GVP_DEBUG_VERBOSE > 2
                                g_message ("discarding data of not selcted scan direction axis data...");
#endif
                                for (ret=-1; ret && hwi->ScanningFlg; ){
                                        ret = hwi->GVP_expect_point (pv, index_all);
                                        if (ret < 0) {
                                                g_message ("GVP SCAN ERROR: waiting for empty package error: %d. Aborting Scan.", ret);
                                                return NULL;
                                        }
                                }
#if GVP_DEBUG_VERBOSE > 2
                                g_message ("completed.");
#endif
                                continue; // not selected?
                        } else {
                                // sanity check
                                if (hwi->GVP_vp_header_current.n != nx)
                                        g_warning ("GVP SCAN ERROR: NX != GVP.current.n: %d, %d", nx, hwi->GVP_vp_header_current.n);
                                int ret2;
                                //for (int xi=x0, ret2=ret=1; xi < x0+nx && ret; ++xi){ // all points per line
                                for (int xi=x0, ret2=ret=1; ret; ++xi){ // all points per line
#if GVP_DEBUG_VERBOSE > 2
                                        g_message ("Got point data [N-%d] for: xi=%d [x0=%d nx=%d] nsrcs[%d]=",  hwi->GVP_vp_header_current.i, xi, x0, nx, dir, hwi->nsrcs_dir[dir]);
#endif
                                        hwi->RPSPMC_data_x_index = xi;
                                        for (int ch=0; ch<hwi->nsrcs_dir[dir] && ch<NUM_PV_DATA_SIGNALS; ++ch){
                                                int k = pvlut[dir][ch];
                                                if (k < 0 || k >= NUM_PV_DATA_SIGNALS){
                                                        g_message ("EEEE expand channel lookup table corruption => pvlut[%d][%d]=%d ** => fallback k=2", dir, ch, k);
                                                        k=2;
                                                }
#if GVP_DEBUG_VERBOSE > 4
                                                g_message("PUT DATA POINT dir[%d] ch[%d] xy[%d, %d] kch[%d] = %g V", dir,ch,xi,yi,k,hwi->GVP_vp_header_current.dataexpanded [k]);
#endif
                                                if (hwi->Mob_dir[dir][ch]){
                                                        if (xi >=0 && yi >=0 && xi < hwi->Mob_dir[dir][ch]->GetNx ()  && yi < hwi->Mob_dir[dir][ch]->GetNy ())
                                                                hwi->Mob_dir[dir][ch]->PutDataPkt (hwi->GVP_vp_header_current.dataexpanded [k], xi, yi); // direct use, skipping pv[] remapping ** data is coming in RP base unit Volts
                                                        else
                                                                g_message ("EEEE (xi, yi) <= (%d, %d) index out of range: [0..%d-1, 0..%d-1]", xi, yi, hwi->Mob_dir[dir][ch]->GetNx (), hwi->Mob_dir[dir][ch]->GetNy ());
                                                } else {
                                                        g_message("SCAN MOBJ[%d][%d] is NULL. SCAN SETUP ERROR?", dir,ch);
                                                        g_message("skipping MOBJ -> PutDataPkt (%g, %d, %d)", hwi->GVP_vp_header_current.dataexpanded [k], xi, yi);
                                                }
                                        }
                                        if ((ret=ret2)) // delay one, this fetches data for the next point, return 0 at last point.
                                                if (ret){
#if GVP_DEBUG_VERBOSE > 4
                                                        g_message ("Awaiting point data [N-%d] for: xi=%d [x0=%d nx=%d] nsrcs[%d]=",  hwi->GVP_vp_header_current.i, xi, x0, nx, dir, hwi->nsrcs_dir[dir]);
#endif
                                                        ret2 = hwi->GVP_expect_point (pv, index_all);
                                                        //g_message ("ret2 = %d", ret2);
                                                        if (ret2 < 0){
                                                                g_message ("Awaiting point data [N-%d] for: xi=%d [x0=%d nx=%d] nsrcs[%d]=",  hwi->GVP_vp_header_current.i, xi, x0, nx, dir, hwi->nsrcs_dir[dir]);
                                                                g_message("STREAM ERROR, FifoReadThread Aborting.");
                                                                hwi->GVP_abort_vector_program ();
                                                                RPSPMC_ControlClass->probe_ready = TRUE;
                                                                return NULL;
                                                        }
                                                }
                                }
#if GVP_DEBUG_VERBOSE > 2
                                g_print("\n");
#endif
                        }
                }
#if GVP_DEBUG_VERBOSE > 2
                g_message ("Completed scan line, move to next y...");
#endif

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
                hwi->RPSPMC_data_y_index = yi; // completed scan line
        }

        RPSPMC_ControlClass->probe_ready = TRUE;
        hwi->EndScan2D();
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
                        if (hwi->abort_GVP_flag)
                                g_message("ProbeFifoReadThread ** GVP Abort");
                
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

        hwi->GVP_abort_vector_program ();
        
	return NULL;
}

int rpspmc_hwi_dev::on_new_data (gconstpointer contents, gsize len, bool init){
        static int position=0; // internal write position

        //g_message ("on_new_data(len=%u) pos=0x%08x", len, position);
        
        if (init){
                g_message ("on_new_data(INIT)");
                position=0;
                memset (GVP_stream_buffer, 0xee, EXPAND_MULTIPLES*DMA_SIZE*sizeof(gint32));
                return 0;
        }

        g_mutex_lock (&GVP_stream_buffer_mutex);

        int half_full = EXPAND_MULTIPLES*DMA_SIZE/2;
        if (GVP_stream_buffer_offset > half_full){ // shift upper 1/2 data back down to 0 if completed processing lower half data
                g_message ("on_new_data() GXSM stream buffer processing pos > 1/2 full mark. [proc:%08x, wpos:%08x], copy back down...", GVP_stream_buffer_offset, position);
                GVP_stream_buffer_offset -= half_full;
                position -= half_full;
                memcpy (&GVP_stream_buffer[0], &GVP_stream_buffer[half_full], half_full*sizeof(gint32));
                memset (&GVP_stream_buffer[half_full], 0xee, half_full*sizeof(gint32));
        }
        
        memcpy (&GVP_stream_buffer[position], contents, len);

        position += len>>2; // update internal last write position
        GVP_stream_buffer_position = position; // update valid data position in stream

#if GVP_DEBUG_VERBOSE > 5
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

        GVP_stream_buffer_AB = 1; // update now after memcopy, valid data in buffer

        g_mutex_unlock (&GVP_stream_buffer_mutex);

        return 0;
}

// ReadProbeData:
// read from probe data FIFO, this engine needs to be called several times from master thread/function
int rpspmc_hwi_dev::ReadProbeData (int dspdev, int control){
        static int recursion_level=0;
	static double pv[MAX_NUM_CHANNELS]; //NUM_PV_HEADER_SIGNALS];
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
                
                for (int jj=0; jj<100 && getVPCconfirmed () < last_vector_index; jj++){
                        usleep(20000);
#if GVP_DEBUG_VERBOSE > 4
                        g_message ("Waiting for GVP vectors confirmed. #[%d] ** last vector confirmed: %d, need %d", jj, getVPCconfirmed (), last_vector_index);
#endif
                        if (jj > 99){
                                g_message ( "GVP VECTOR WRITE TIMEOUT, ABROTING -- FR::FINISH-OK");
                                return RET_FR_FCT_END; // finish OK.
                        }
                }
                g_message ("GVP VECTORS been written and confirmed for vector #%d. Executing GVP now.", getVPCconfirmed ());
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
                                        
                                // g_message (" *** OK, loading pv sec[%d] ***", GVP_vp_header_current.section);

                                // copy header to pv[] as assigned below
                                pv[PROBEDATA_ARRAY_SRCS]  = GVP_vp_header_current.srcs; // new
                                pv[PROBEDATA_ARRAY_INDEX] = (double)index_all++;
                                pv[PROBEDATA_ARRAY_PHI]   = (double)GVP_vp_header_current.index; // testing, point index in section
                                pv[PROBEDATA_ARRAY_SEC]   = (double)GVP_vp_header_current.section;
                                pv[PROBEDATA_ARRAY_TIME]  = GVP_vp_header_current.dataexpanded[14]; // time in ms
                                pv[PROBEDATA_ARRAY_XS]    = GVP_vp_header_current.dataexpanded[0]; // Xs in Volts
                                pv[PROBEDATA_ARRAY_YS]    = GVP_vp_header_current.dataexpanded[1]; // Ys in Volts
                                pv[PROBEDATA_ARRAY_ZS]    = GVP_vp_header_current.dataexpanded[2]; // Zs in Volts
                                pv[PROBEDATA_ARRAY_U ]    = GVP_vp_header_current.dataexpanded[3]; // Bias in Volts

#if GVP_DEBUG_VERBOSE > 2
                                g_message ("PVh sec[%g] i=%g t=%g ms U=%g V",
                                           pv[PROBEDATA_ARRAY_SEC],pv[PROBEDATA_ARRAY_INDEX],
                                           pv[PROBEDATA_ARRAY_TIME],pv[PROBEDATA_ARRAY_U ]);
#endif

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
                        RPSPMC_ControlClass->add_probedata (GVP_vp_header_current.dataexpanded, pv, false, true); // using diect mapping from expanded header //(GVP_vp_header_current.i < (GVP_vp_header_current.n-1)) ? true:false);

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
gboolean rpspmc_hwi_dev::ScanLineM(int yindex, int xdir, int muxmode, //srcs_mask,
                                   Mem2d *Mob[MAX_SRCS_CHANNELS],
                                   int ixy_sub[4]){
	static int ydir=0;
	static int running = FALSE;

        int srcs_mask = muxmode;
        
        if (yindex == -2){ // SETUP STAGE 1
                int num_srcs = 0;
                running      = FALSE;

                for (int i=0; rpspmc_source_signals[i].mask; ++i){
                        if ((srcs_mask & rpspmc_source_signals[i].mask) == rpspmc_source_signals[i].mask){
                                num_srcs++;
                                g_message ("Match [%d] for 0x%08x <==> %s  #%d", i, rpspmc_source_signals[i].mask, rpspmc_source_signals[i].label, num_srcs);
                        }
                }

                g_message ("rpspmc_hwi_dev::ScanLineM SETUP STAGE ** DIR%+d => SRCS=%08x #%d", xdir, srcs_mask, num_srcs);
                
                // SETUP MODE, CONFIGURE SRCS,...
                switch (xdir){
                case 1: // first init step of XP (->)
                        // reset all
                        for (int i=0; i<4; ++i){
                                srcs_dir[i] = nsrcs_dir[i] = 0;
                                Mob_dir[i] = NULL;
                        }
                        // setup XP ->
                        srcs_dir[0]  = srcs_mask;
                        nsrcs_dir[0] = num_srcs;
                        Mob_dir[0]   = Mob;
                        return TRUE;
                case -1: // second init step of XM (<-)
                        srcs_dir[1]  = srcs_mask;
                        nsrcs_dir[1] = num_srcs;
                        Mob_dir[1]   = Mob;
                        return TRUE;
                case  2: // ... init step of 2ND_XP (2>)
                        srcs_dir[2]  = srcs_mask;
                        nsrcs_dir[2] = num_srcs;
                        Mob_dir[2]   = Mob;
                        return TRUE;
                case -2: // ... init step of 2ND_XM (<2)
                        srcs_dir[3]  = srcs_mask;
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
                subscan_data_y_index_offset = ixy_sub[2];

                // may compute and set if available
		// main_get_gapp()->xsm->data.s.pixeltime = (double)dsp_scan.dnx/SamplingFreq;

                // assure clean GVP state
                GVP_abort_vector_program ();
                usleep(200000);

                // setup hardware for scan here and
                // start g-thread for data transfer now
                // this is generating a GVP vector scan program, loading it and starting the data transfer thread in time of excuting it.
		start_data_read (yindex, nsrcs_dir[0], nsrcs_dir[1], nsrcs_dir[2], nsrcs_dir[3], Mob_dir[0], Mob_dir[1], Mob_dir[2], Mob_dir[3]);
                
		running = TRUE; // and off we go....
                return TRUE;
	}

        // ACTUAL SCAN PROGRESS CHECK on line basis
        if (ScanningFlg){ // make sure we did not got aborted and completed already!

                y_current = RPSPMC_data_y_index;
                //g_print ("current RPSPMC_data_y_index: %04d ** rpspmc_hwi_spm::ScanLineM (yindex=%04d, xdir=%d, ydir=%d, lssrcs=%x) checking...\n", RPSPMC_data_y_index, yindex, xdir, ydir, srcs_mask);
                //g_print (".");
                
                if (ydir > 0 && yindex <= RPSPMC_data_y_index){
                        //g_print ("\r * top-down line %04d completed.\n", yindex);
                        return FALSE; // line completed top-down
                }
                if (ydir < 0 && yindex >= RPSPMC_data_y_index){
                        //g_print ("\r * bottom-up line %04d completed.\n", yindex);
                        return FALSE; // line completed bot-up
                }

                // optional
                //if ((ydir > 0 && yindex > 1) || (ydir < 0 && yindex < Ny-1)){
                //        double x,y,z;
                //        RTQuery ("W", x,y,z); // run watch dog -- may stop scan in any faulty condition
                //}
                
                return TRUE;
        }
        return FALSE; // scan was stopped by user
}




/*
 Real-Time Query of DSP signals/values, auto buffered
 Propertiy hash:      return val1, val2, val3:
 "z" :                ZS, XS, YS  with offset!! -- in volts after piezo amplifier
 "o" :                Z0, X0, Y0  offset -- in volts after piezo amplifier
 "R" :                Scan Pos ZS, XS, YS -- in Angstroem/base unit
 "f" :                dFreq, I reading in nA, "I" input reading in Volts
 "s","S" :            DSP Statemachine Status Bits, DSP load, DSP load peak
 "Z" :                probe Z Position
 "i" :                GPIO (high level speudo monitor)
 "A" :                Mover/Wave axis counts 0,1,2 (X/Y/Z)
 "p" :                X,Y Scan/Probe Coords in Pixel, 0,0 is center, DSP Scan Coords
 "P" :                X,Y Scan/Probe Coords in Pixel, 0,0 is top left [indices]
 "B" :                Bias
 "G" :                GVP
 "F" :                GVP-AMC-FMC
 "V" :                ADC IN1, IN2 Voltages
 "X {JSON-STRING}" :  X Extended RPSPMC control, send JSON string
*/

gint rpspmc_hwi_dev::RTQuery (const gchar *property, double &val1, double &val2, double &val3){
        if (*property == 'z'){ // Scan Coordinates: ZScan, XScan, YScan  with offset!! -- in volts with gains!
		val1 = spmc_parameters.gxsm_z_polarity*spmc_parameters.z_monitor*main_get_gapp()->xsm->Inst->VZ(); // adjust for polarity as Z-Monitor is the actual DAC OUT Z
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
                val3 =  main_get_gapp()->xsm->Inst->Volt2YA (spmc_parameters.ys_monitor);
		return TRUE;
        }

        if (*property == 'f'){
                val1 = pacpll_parameters.dfreq_monitor; // Freq Shift
                val2 = spmc_parameters.signal_monitor / main_get_gapp()->xsm->Inst->nAmpere2V (1.0); // Reading converted to nA
		val3 = spmc_parameters.signal_monitor; // Current Input reading in Volts (+/-1 V for RF-IN2, +/-5V for AD24-IN3
		return TRUE;
	}

        if (*property == 'Z'){ // probe Z -- well total Z here again
                val1 = rpspmc_pacpll_hwi_pi.app->xsm->Inst->Volt2ZA (spmc_parameters.gxsm_z_polarity*spmc_parameters.z_monitor); // Z pos in Ang,  adjust for polarity as Z-Monitor is the actual DAC OUT Z
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
                int Sgvp = (statusbits>>8) & 0xf;  // assign GVP_status = { sec[32-4:0], setvec, reset, pause, ~finished };
                int Sspm = statusbits & 0xff;      // assign SPM status = { ...SPM Z Servo Hold, GVP-FB-Hold, GVP-Finished, Z Servo EN }
		val3 = (double)Sgvp;  
		val2 = (double)Sspm; // assign
		val1 = (double)(   (((Sspm & 0x01) && !(Sspm & 0x04) && !(Sspm & 0x04) && !(Sspm & 0x08)) ? 1:0)   // Z: Z Servo: active
                                  + ((Sgvp & 0x01 ? 1:0) << 1)  // G GVP !FINISHED **Scan/GVP Running
                                  + ((Sgvp & 0x02 ? 1:0) << 2)  // P GVP: GVP Pause         (Scan)
                                  + ((Sgvp & 0x04 ? 1:0) << 3)  // R GVP: Reset
                                  + ((Sspm & 0x02 ? 1:0) << 4)  // F GVP: GVP Finished
                                  + ((Sspm & 0x04 ? 1:0) << 5)  // H GVP: GVP-FB-Hold
                                  + ((Sspm & 0x08 ? 1:0) << 6)  // h GVP: SPM-FB-Hold
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
#if 0
                val1 = (double)( RPSPMC_data_x_index - (main_get_gapp()->xsm->data.s.nx/2 - 1) + 1);
                val2 = (double)(-RPSPMC_data_y_index + (main_get_gapp()->xsm->data.s.ny/2 - 1) + 1);
                val3 = (double)RPSPMC_data_z_value;
#else
		val1 = main_get_gapp()->xsm->Inst->Volt2XA (spmc_parameters.xs_monitor)/main_get_gapp()->xsm->data.s.rx * main_get_gapp()->xsm->data.s.nx;
                val2 = -main_get_gapp()->xsm->Inst->Volt2YA (spmc_parameters.ys_monitor)/main_get_gapp()->xsm->data.s.ry * main_get_gapp()->xsm->data.s.ny;
                val3 = main_get_gapp()->xsm->Inst->Volt2ZA (spmc_parameters.zs_monitor);
#endif
		return TRUE;
        }
        if (*property == 'P'){ // SCAN DATA INDEX, range: 0..NX, 0..NY
#if 0
                val1 = (double)(RPSPMC_data_x_index);
                val2 = (double)(RPSPMC_data_y_index);
                val3 = (double)RPSPMC_data_z_value;
#else
		val1 = (main_get_gapp()->xsm->Inst->Volt2XA (spmc_parameters.xs_monitor)/main_get_gapp()->xsm->data.s.rx+0.5) * main_get_gapp()->xsm->data.s.nx;
                val2 = (-main_get_gapp()->xsm->Inst->Volt2YA (spmc_parameters.ys_monitor)/main_get_gapp()->xsm->data.s.ry+0.5) * main_get_gapp()->xsm->data.s.ny;
                val3 = main_get_gapp()->xsm->Inst->Volt2ZA (spmc_parameters.zs_monitor);
#endif

                return TRUE;
        }
        if (*property == 'B'){ // Monitors: Bias
                val1 = spmc_parameters.bias_monitor;
                val2 = spmc_parameters.bias_reg_monitor;
                val3 = spmc_parameters.bias_set_monitor;
		return TRUE;
        }
        if (*property == 'G'){ // Monitors: GVP
                val1 = spmc_parameters.gvpu_monitor;
                val2 = spmc_parameters.gvpa_monitor;
                val3 = spmc_parameters.gvpb_monitor;
		return TRUE;
        }
        if (*property == 'F'){ // Monitors: GVP-AMC-FMC
                val1 = spmc_parameters.gvpamc_monitor;
                val2 = spmc_parameters.gvpfmc_monitor;
                val3 = 0.;
		return TRUE;
        }
        if (*property == 'V'){ // Monitors: Volatges
                val1 = spmc_parameters.ad463x_monitor[0];
                val2 = spmc_parameters.ad463x_monitor[1];
                val3 = 0.;
		return TRUE;
        }

        if (*property == 'X'){ // Send JSON
                val1 = -2.;
                val2 = 0.;
                val3 = 0.;
                if (property[1]){
                        if (property[2]=='{' && property[3] && rpspmc_pacpll) { // just checking for initial { of JSON string in property "X {..."
                                g_message ("RTQuery: sending JSON: %s", &property[2]);
                                val1 = (double) rpspmc_pacpll->write_json_string (&property[2]); // send JSON
                                /* JSON FORMAT:
                                  json_string = g_strdup_printf ("{ \"parameters\":{\"%s\":{\"value\":%.10g}}}", parameter_id, value);
                                  json_string = g_strdup_printf ("{ \"parameters\":{ ...  }}", parameter_id, value);
                                  ... is list of pairs:
                                    g_string_append_printf (list, "\"%s\":{\"value\":%.10g}",  parameter_id[i], d_vec[j]);
                                    g_string_append_printf (list, "\"%s\":{\"value\":%d}",  parameter_id[i], i_vec[i]);
                                 */
                                
                                return TRUE;
                        }
                        if (property[2]=='*' && property[3]){
                                g_message ("RTQuery: query JSON parameter: %s", &property[3]);
                                JSON_parameter *p = rpspmc_pacpll->check_parameter (&property[3], strlen(&property[3])); // query parameter
                                if (p){
                                        g_message ("RTQuery: query JSON parameter: %s  = %g", &property[3], *p->value);
                                        val1 = *p->value;
                                }
                        }
                        if (property[2]=='$' && property[3]){
                                g_message ("RTQuery: query JSON signal: %s", &property[3]);
                                JSON_signal *s = rpspmc_pacpll->check_signal (&property[3], strlen(&property[3])); // query signal
                                if (s){
                                        g_message ("RTQuery: query JSON parameter: %s [%d] = [%g, %g ... %g]", &property[3],
                                                   s->size, s->value[0],s->value[1], s->value[s->size-1]);
                                        val1 = s->value[0];
                                        val2 = (int) s->size;
                                        // sketchy trick to send pointer
                                        g_message ("Size of val2: %d", sizeof(val3));
                                        memcpy (&val3, &s->value, sizeof(val3));
                                }
                        }
                }
                return TRUE;
        }

        //	printf ("ZXY: %g %g %g\n", val1, val2, val3);

//	val1 =  (double)dsp_analog_out.z_scan;
//	val2 =  (double)dsp_analog_out.x_scan;
//	val3 =  (double)dsp_analog_out.y_scan;

	return TRUE;
}



gint rpspmc_hwi_dev::RTQuery (const gchar *property, int n, gfloat *data){
        const gint64 max_age = 20000; // 20ms
        static gint64 time_of_last_reading1 = 0; // abs time in us
        static gint64 time_of_last_reading2 = 0; // abs time in us
        static gint64 time_of_last_reading3 = 0; // abs time in us
        static gint64 time_of_last_reading4 = 0; // abs time in us
        static gint64 time_of_last_trg = 0; // abs time in us
        static gint s1ok=0, s2ok=0, s3ok=0, s4ok=0;

        // Trigger
        if ( property[0] == 'T' && (time_of_last_trg+max_age) < g_get_real_time () ){
                time_of_last_trg = g_get_real_time ();
                //set_blcklen (n);
                s1ok=-1; s2ok=-1;
        }

        // Request History Vector n: "Hnn"
        if ( property[0] == 'H'){
                get_history_vector_f (atoi(&property[1]), data, n);
                return 0;
        }

        
        // Signal1
        if ( property[1] == '1' && ((time_of_last_reading1+max_age) < g_get_real_time () || s1ok)){
                time_of_last_reading1 = g_get_real_time ();
                get_history_vector_f (6, data, n); // Z

                //double scale =  DSP32Qs23dot8TO_Volt; // assuming MIX_IN_0..3 withe 23Q8 scale for10V
                //s1ok=read_pll_signal1 (data, n, scale, 0);
        }
        // Signal2
        if ( property[1] == '2' && ((time_of_last_reading2+max_age) < g_get_real_time () || s2ok)){
                time_of_last_reading2 = g_get_real_time ();
                get_history_vector_f (19, data, n); // Current

                //double scale =  DSP32Qs15dot16TO_Volt;
                //s2ok=read_pll_signal2 (data, n, scale, 0);
        }
        // Signal1 deci 256
        if ( property[1] == '3' && ((time_of_last_reading3+max_age) < g_get_real_time () || s3ok)){
                time_of_last_reading3 = g_get_real_time ();
                get_history_vector_f (6, data, n); // Z

                //double scale =  DSP32Qs23dot8TO_Volt;
                //s3ok=read_pll_signal1dec (data, n, scale, property[0] == 'R');
        }
        // Signal2 subsampled 256
        if ( property[1] == '4' && ((time_of_last_reading4+max_age) < g_get_real_time () || s4ok)){
                time_of_last_reading4 = g_get_real_time ();
                get_history_vector_f (19, data, n); // Current

                //double scale =  DSP32Qs15dot16TO_Volt;
                //s4ok=read_pll_signal2dec (data, n, scale, property[0] == 'R');
        }
        return 0;
}




// template/dummy signal management

void rpspmc_hwi_dev::set_spmc_signal_mux (int source[6]){
        int k;
        for (int i=0; rpspmc_source_signals[i].label; ++i){ // name
                k=-1;
                switch (rpspmc_source_signals[i].mask){
                case 0x0100: k=0; break; // SWP*00=MUX00
                case 0x0200: k=1; break; //..
                case 0x0400: k=2; break;
                case 0x0800: k=3; break;
                case 0x1000: k=4; break;
                case 0x2000: k=5; break; // SWP*05=MUX05
                default: continue;
                }
                k = source[k];
                if (!rpspmc_swappable_signals[k].label) { g_warning ("GVP SOURCE MUX/SWPS INIT ** i=%d k=%d SWPS invalid/NULL", i,k); break; }
                //rpspmc_source_signals[i].name         = rpspmc_swappable_signals[k].name;
                rpspmc_source_signals[i].label        = rpspmc_swappable_signals[k].label;
                rpspmc_source_signals[i].unit         = rpspmc_swappable_signals[k].unit;
                rpspmc_source_signals[i].unit_sym     = rpspmc_swappable_signals[k].unit_sym;
                rpspmc_source_signals[i].scale_factor = rpspmc_swappable_signals[k].scale_factor;
                g_message ("GVP SOURCE MUX/SWPS INIT ** i=%d k=%d {%s} sfac=%g", i, k, rpspmc_source_signals[i].label, rpspmc_source_signals[i].scale_factor);
        }
}


// GVP CONTROL MODES:
#define SPMC_GVP_CONTROL_RESET     0
#define SPMC_GVP_CONTROL_EXECUTE   1   
#define SPMC_GVP_CONTROL_PAUSE     2
#define SPMC_GVP_CONTROL_RESUME    3
#define SPMC_GVP_CONTROL_RESET_UAB 4
#define SPMC_GVP_CONTROL_GVP_EXECUTE_NO_DMA 5   
#define SPMC_GVP_CONTROL_GVP_RESET_ONLY     6   
#define SPMC_GVP_CONTROL_RESET_COMPONENTS   9


void rpspmc_hwi_dev::GVP_execute_vector_program(){ // init DMA and start GVP
        GVP_vp_init (); // prepare for streaming
        g_message ("rpspmc_hwi_dev::GVP_execute_vector_program ()");
        rpspmc_pacpll->write_parameter ("SPMC_GVP_CONTROL", SPMC_GVP_CONTROL_EXECUTE);
}

void rpspmc_hwi_dev::GVP_execute_only_vector_program(){ // only start GVP, no read back DMA initiated
        g_message ("rpspmc_hwi_dev::GVP_execute_vector_program ()");
        rpspmc_pacpll->write_parameter ("SPMC_GVP_CONTROL", SPMC_GVP_CONTROL_EXECUTE);
}

void rpspmc_hwi_dev::GVP_abort_vector_program (){
        g_message ("rpspmc_hwi_dev::GVP_abort_vector_program ()");
        rpspmc_pacpll->write_parameter ("SPMC_GVP_CONTROL", SPMC_GVP_CONTROL_RESET);
        abort_GVP_flag = true;
}

void rpspmc_hwi_dev::GVP_reset_vector_program (){ // ONLY GVP CORE RESET, NO DMA, etc. managed or reset
        g_message ("rpspmc_hwi_dev::GVP_reset_vector_program ()");
        rpspmc_pacpll->write_parameter ("SPMC_GVP_CONTROL", SPMC_GVP_CONTROL_GVP_RESET_ONLY);
        abort_GVP_flag = true;
}

void rpspmc_hwi_dev::GVP_reset_vector_components (int mask){
        g_message ("rpspmc_hwi_dev::GVP_reset_vector_components (0x%02x)", mask);
        rpspmc_pacpll->write_parameter ("SPMC_GVP_CONTROL", (mask << 8) | SPMC_GVP_CONTROL_RESET_COMPONENTS);
}


void rpspmc_hwi_dev::GVP_reset_UAB (){
        g_message ("rpspmc_hwi_dev::GVP_reset_UAB ()");
        rpspmc_pacpll->write_parameter ("SPMC_GVP_CONTROL", SPMC_GVP_CONTROL_RESET_UAB);
}

void rpspmc_hwi_dev::GVP_vp_init (){
        // reset GVP stream buffer read count
        g_message ("*** rpspmc_hwi_dev::GVP_vp_init () ***");
        GVP_stream_buffer_AB = -2;
        GVP_stream_buffer_position = 0;
        GVP_stream_buffer_offset = 0; // 0x100   //  =0x00 **** TESTING BRAM ISSUE -- FIX ME !!! *****
        GVP_stream_status=0;
        on_new_data (NULL, 0, true);

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

int rpspmc_hwi_dev::GVP_write_program_vector(int i, PROBE_VECTOR_GENERIC *v, PROBE_VECTOR_EXTENSION *vx){
        if (i >= MAX_PROGRAM_VECTORS || i < 0)
                return 0;

#define I_GVP_PC_INDEX  0
#define I_GVP_N         1
#define I_GVP_OPTIONS   2
#define I_GVP_SRCS      3
#define I_GVP_NREP      4
#define I_GVP_NEXT      5
#define I_GVPX_OPCD     6
#define I_GVPX_RCHI     7
#define I_GVPX_JMPR     8
#define I_GVP_SIZE (I_GVPX_JMPR+1)
        
#define D_GVP_DX        0
#define D_GVP_DY        1
#define D_GVP_DZ        2
#define D_GVP_DU        3
#define D_GVP_AA        4
#define D_GVP_BB        5
#define D_GVP_AM        6
#define D_GVP_FM        7
#define D_GVP_SLW       8
#define D_GVPX_CMPV     9
#define D_GVP_SIZE (D_GVPX_CMPV+1)
        
        const gchar *SPMC_GVP_VECTOR_COMPONENTS[] = {
                "SPMC_GVP_VECTOR_PC", 
                "SPMC_GVP_VECTOR__N", 
                "SPMC_GVP_VECTOR__O", 
                "SPMC_GVP_VECTORSRC", 
                "SPMC_GVP_VECTORNRP", 
                "SPMC_GVP_VECTORNXT", 
                "SPMC_GVP_XVECTOR_OPCD",
                "SPMC_GVP_XVECTOR_RCHI",
                "SPMC_GVP_XVECTOR_JMPR",
                "SPMC_GVP_VECTOR_DX", 
                "SPMC_GVP_VECTOR_DY", 
                "SPMC_GVP_VECTOR_DZ", 
                "SPMC_GVP_VECTOR_DU", 
                "SPMC_GVP_VECTOR_AA", 
                "SPMC_GVP_VECTOR_BB", 
                "SPMC_GVP_VECTOR_AM", 
                "SPMC_GVP_VECTOR_FM", 
                "SPMC_GVP_VECTORSLW", 
                "SPMC_GVP_XVECTOR_CMPV",
                NULL };

        int gvp_vector_i[I_GVP_SIZE];
        double gvp_vector_d[D_GVP_SIZE];

#define SPMC_GVP_VECTOR_EXTENSTION_BITMASK (1<<7)
        
        int vector_extension = vx && (v->options & SPMC_GVP_VECTOR_EXTENSTION_BITMASK) ? 1 : 0;

        const int src_time_msk = 0xc000; // 48bit time mon mask
        // Vector Program Code Setup
        gvp_vector_i [I_GVP_PC_INDEX] = i;
        gvp_vector_i [I_GVP_N       ] = v->n;
        //gvp_vector_i [I_GVP_OPTIONS ] = (((v->srcs | src_time_msk) & 0xffffff) << 8) | (v->options & 0xff); // 0: FB, 7: XVEC  **   ((v->options & VP_FEEDBACK_HOLD) ? 0:1) | (1<<7) | (1<<6) | (1<<5);
        gvp_vector_i [I_GVP_OPTIONS ] = v->options; // currently only lowest 8 bits
        // currently 24 bits, actaully 16 mapped used in stream serialization, with 6 been multiplexd from 32 = 42 possible different signals. May be expandd to 32 if needed.
        gvp_vector_i [I_GVP_SRCS    ] = (src_time_msk | v->srcs) & 0xffffffff;
        // g_message ("GVP_write_program_vector[%d]: srcs = 0x%08x", i, gvp_vector_i [I_GVP_OPTIONS ] );
        gvp_vector_i [I_GVP_NREP    ] = v->repetitions > 1 ? v->repetitions-1 : 0;
        gvp_vector_i [I_GVP_NEXT    ] = v->ptr_next;
        gvp_vector_i [I_GVPX_OPCD   ] = vector_extension ? vx->opcd : 0;
        gvp_vector_i [I_GVPX_RCHI   ] = vector_extension ? vx->rchi : 0;
        gvp_vector_i [I_GVPX_JMPR   ] = vector_extension ? vx->jmpr : 0;


        if (i == 0 && (v->options & VP_INITIAL_SET_VEC)){ // 1st vector is set postion vector? Get pos and calc differentials.
                gvp_vector_i [I_GVP_PC_INDEX] = 0x1000 | ((v->options>>16)&0xff); // all componets are absolute set coordnates!
                // componets can be masked to set=0 via dX=0 0x1001, dY=0 0x1002, dZ=0 0x1004, du=0 0x1008,da=0  0x1010, db=0 0x1020
        }

        g_print ("Vec[%2d] XYZU: %g %g %g %g V  [#%d, R%d J%d OPT=%08x SRCS=%08x] PC*initial Msk=%04x\n",
                 i,
                 v->f_dx, v->f_dy, v->f_dz, v->f_du,
                 gvp_vector_i [I_GVP_N       ], gvp_vector_i [I_GVP_NREP    ], gvp_vector_i [I_GVP_NEXT    ], gvp_vector_i [I_GVP_OPTIONS ], gvp_vector_i [I_GVP_SRCS ],
                 gvp_vector_i [I_GVP_PC_INDEX]);
        // Vector Components are all in Volts
        gvp_vector_d [D_GVP_DX      ] = v->f_dx;
        gvp_vector_d [D_GVP_DY      ] = v->f_dy;
        gvp_vector_d [D_GVP_DZ      ] = v->f_dz;
        gvp_vector_d [D_GVP_DU      ] = v->f_du;
        gvp_vector_d [D_GVP_AA      ] = v->f_da;
        gvp_vector_d [D_GVP_BB      ] = v->f_db;
        gvp_vector_d [D_GVP_AM      ] = v->f_dam;
        gvp_vector_d [D_GVP_FM      ] = v->f_dfm;
        gvp_vector_d [D_GVP_SLW     ] = v->slew;
        gvp_vector_d [D_GVPX_CMPV   ] = vector_extension ? vx->cmpv : 0.;
        
        // send it down
        if (rpspmc_pacpll)
                rpspmc_pacpll->write_array (SPMC_GVP_VECTOR_COMPONENTS,  I_GVP_SIZE, gvp_vector_i,  D_GVP_SIZE, gvp_vector_d);
                
        return -1;
}

//#define BBB_DBG

#define RECOVERY_RETRYS 1
int rpspmc_hwi_dev::read_GVP_data_block_to_position_vector (int offset, gboolean expect_full_header){
        static int retry = RECOVERY_RETRYS;
        size_t ch_index;
#ifdef BBB_DBG
        static int sec_hdr=0;
        static int hdr=0;
#endif
        g_mutex_lock (&GVP_stream_buffer_mutex);

#ifdef BBB_DBG
        if (offset == 0) sec_hdr=hdr=0;
#endif

        
#if 0
        if (expect_full_header || offset==0)
                status_append_int32 (&GVP_stream_buffer[offset], 10*16, true, offset, true);
#endif
                
#if 0
        if (offset < 0 || offset > (EXPAND_MULTIPLES*DMA_SIZE-20)){
                gchar *tmp = g_strdup_printf ("read_GVP_data_block_to_position_vector: Reading offset %08x out of range ERROR.",
                                              offset);
                status_append (tmp, true);
                g_warning (tmp);
                g_free (tmp);
                g_mutex_unlock (&GVP_stream_buffer_mutex);
                return -999;
        }
#endif
                
        if (offset >= GVP_stream_buffer_position){ // Buffer is huge now all pages concat

#if GVP_DEBUG_VERBOSE > 3
                gchar *tmp = g_strdup_printf ("read_GVP_data_block_to_position_vector: Reading offset %08x is beyond stream write position %08x. Awaiting data.\n",
                                              offset, GVP_stream_buffer_position);
                status_append (tmp, true);
                if (offset > 64)
                        status_append_int32 (&GVP_stream_buffer[offset-64], 10*16, true, offset-64, true);
                else
                        status_append_int32 (&GVP_stream_buffer[0], 10*16, true, 0, true);
                g_warning (tmp);
                g_free (tmp);
#endif
                g_mutex_unlock (&GVP_stream_buffer_mutex);
                return -99; // OK -- but have wait and reprocess when data package is completed
        }
                
        GVP_vp_header_current.srcs = GVP_stream_buffer[offset]&0xffff;

        GVP_vp_header_current.i = (int)(((guint32)GVP_stream_buffer[offset])>>16); // data point index within GVP section (N points)
        if (GVP_vp_header_current.srcs == 0xffff){
                GVP_vp_header_current.n     = GVP_vp_header_current.i + 1; // this full vector of first point, index=N-1 ... counts down to 0
                GVP_vp_header_current.endmark = 0;
                retry=RECOVERY_RETRYS;
        } else {
                if (GVP_stream_buffer[offset] == 0xfefefefe){
                        GVP_vp_header_current.endmark = 1;
                        GVP_vp_header_current.n = 0;
                        GVP_vp_header_current.i = 0;
                        GVP_vp_header_current.srcs = 0xffff;
                        status_append ("GVP END MARK DETECTED", true);
                        g_message ("** GVP END MARK DETECTED **");
                        g_mutex_unlock (&GVP_stream_buffer_mutex);
#ifdef BBB_DBG
                        status_append_int32 (&GVP_stream_buffer[offset], 64, true, offset, true);
#endif
                        return 0; // END OF GVP -- anywas a full position vector still follows, discarding now
                } else {
                        if (GVP_vp_header_current.n == GVP_vp_header_current.i+2) // 2nd point, store srcs mask for verify
                                GVP_vp_header_current.srcs_mask_vector = GVP_vp_header_current.srcs; // store ref mask
                        else { // normal GVP data stream, verify continuity
                                if ((GVP_vp_header_current.i != (GVP_vp_header_current.ilast-1) && GVP_vp_header_current.i != 0) // ** == 0 OK: as GVP VEC-X Until Abort Section OpCode induced
                                    ||
                                    GVP_vp_header_current.srcs_mask_vector != GVP_vp_header_current.srcs){
                                        // stream ERROR detected
                                        {
                                                gchar *tmp = g_strdup_printf ("read_GVP_data_block_to_position_vector: Stream ERROR at Reading offset %08x, write position %08x.\n"
                                                                              "SRCS/index mismatch detected: 0x%04x vs 0x%04x or missing data/index jump: i %d -> %d\n",
                                                                              offset, GVP_stream_buffer_position,
                                                                              GVP_vp_header_current.srcs_mask_vector, GVP_vp_header_current.srcs,
                                                                              GVP_vp_header_current.ilast, GVP_vp_header_current.i);
#ifdef BBB_DBG
                        status_append_int32 (&GVP_stream_buffer[offset], 256, true, offset, true);
#endif
#if GVP_DEBUG_VERBOSE > 3
                                                status_append (tmp, true); // this is effn out gtk shit even in idle callback
                                                if (offset > 64)
                                                        status_append_int32 (&GVP_stream_buffer[offset-64], 10*16, true, offset-64, true);
                                                else
                                                        status_append_int32 (&GVP_stream_buffer[0], 10*16, true, 0, true);
#endif
                                                g_warning (tmp);
                                                g_free (tmp);
                                        }
                                        if (--retry > 0){ //  try to recover
                                                gchar *tmp = g_strdup_printf ("Trying to recover stream from missing/bogus data. retry=%d\n", retry);
                                                //status_append (tmp, true);
                                                g_warning (tmp);
                                                g_free (tmp);
                                                if (GVP_vp_header_current.i >= 0 && GVP_vp_header_current.i < GVP_vp_header_current.n)
                                                        GVP_vp_header_current.ilast = GVP_vp_header_current.i+1; // adjust for skip ***
                                                g_mutex_unlock (&GVP_stream_buffer_mutex);
                                                return -99;  // OK -- but retry -- **have wait and reprocess when data package is completed
                                        }else {
                                                g_mutex_unlock (&GVP_stream_buffer_mutex);
                                                return -98;
                                        }
                                }
                        }
                }
        }

#ifdef BBB_DBG
        if ((GVP_vp_header_current.ilast-1) > 0 && GVP_vp_header_current.i == 0)
                g_message ("Vec-X Until/Jmp Section Abort Condition detected. i=0: END of SECTION.");
#endif
        
        GVP_vp_header_current.index = GVP_vp_header_current.n - GVP_vp_header_current.i;
        if (GVP_vp_header_current.index < 0){
                gchar *tmp = g_strdup_printf ("read_GVP_data_block_to_position_vector: Stream ERROR at Reading offset %08x, write position %08x.\n"
                                              "SRCS/index mismatch detected. %04x vs %04x, i %d -> %d  => n=%d (<0 is illegal)\n",
                                              offset, GVP_stream_buffer_position,
                                              GVP_vp_header_current.srcs_mask_vector, GVP_vp_header_current.srcs,
                                              GVP_vp_header_current.ilast, GVP_vp_header_current.i, GVP_vp_header_current.index);
#if GVP_DEBUG_VERBOSE > 3
                status_append (tmp, true); // this is effn out gtk shit even in idle callback
                if (offset > 64)
                        status_append_int32 (&GVP_stream_buffer[offset-64], 10*16, true, offset-64, true);
                else
                        status_append_int32 (&GVP_stream_buffer[0], 10*16, true, 0, true);
#endif
                g_warning (tmp);
                g_free (tmp);
                GVP_vp_header_current.index = 0; // to prevent issues
                g_mutex_unlock (&GVP_stream_buffer_mutex);
                return (-95);
        }
                
        GVP_vp_header_current.number_channels=0;
        for (int i=0; i<16; i++){
                GVP_vp_header_current.ch_lut[i]=-1;
                if (GVP_vp_header_current.srcs & (1<<i))
                        GVP_vp_header_current.ch_lut[GVP_vp_header_current.number_channels++] = i;
        }
                
        // expand data stream and sort into channels
        for (ch_index=0; ch_index < GVP_vp_header_current.number_channels && ch_index+offset < GVP_stream_buffer_position; ++ch_index){
                int ich = GVP_vp_header_current.ch_lut[ch_index];
                GVP_vp_header_current.chNs[ich] = GVP_stream_buffer[1+ch_index+offset];
                if (ich < 14){ // ICH 14,15 -> 64bit time
                        GVP_vp_header_current.dataexpanded[ich] = rpspmc_source_signals[ich+SIGNAL_INDEX_ICH0].scale_factor*GVP_vp_header_current.chNs[ich]; // in units, base units Volts used for XYZ. etc.
#if 1 // THIS DATA ASSIGNMENT AND SCALING IS CORRECT
                        if (ich >= 8){
                                GVP_vp_header_current.dataexpanded[ich] = rpspmc_source_signals[ich+SIGNAL_INDEX_ICH0].scale_factor*GVP_vp_header_current.chNs[ich];
                                //g_message ("GVP DATA [%d] ICH%d %s s:%g x:%d sx:%g %s", ch_index, ich,
                                //           rpspmc_source_signals[ich+SIGNAL_INDEX_ICH0].label, rpspmc_source_signals[ich+SIGNAL_INDEX_ICH0].scale_factor, GVP_vp_header_current.chNs[ich],
                                //           GVP_vp_header_current.dataexpanded[ich], rpspmc_source_signals[ich+SIGNAL_INDEX_ICH0].unit);
                        }
#endif
                }
        }

        if (expect_full_header && GVP_vp_header_current.srcs != 0xffff){
                gchar *tmp = g_strdup_printf ("ERROR: read_GVP_data_block_to_position_vector: Reading offset %08x, write position %08x. Expecting full header but found srcs=%04x, i=%d rty=%d\n",
                                              offset, GVP_stream_buffer_position,  GVP_vp_header_current.srcs, GVP_vp_header_current.i, retry);
                status_append (tmp, true);
                if (offset>64)
                        status_append_int32 (&GVP_stream_buffer[offset-64], 10*16, true, offset-64, true);
                else
                        status_append_int32 (&GVP_stream_buffer[0], 10*16, true, 0, true);
                g_warning (tmp);
                g_free (tmp);
                if (--retry > 0){
                        gchar *tmp = g_strdup_printf ("Trying to recover stream from missing/bogus data. retry=%d\n", retry);
                        //status_append (tmp, true);  // this is effn out gtk shit even in idle callback
                        g_warning (tmp);
                        g_free (tmp);
                        g_mutex_unlock (&GVP_stream_buffer_mutex);
                        return -99;  // OK -- but have wait and reprocess when data package is completed
                } else {
                        g_mutex_unlock (&GVP_stream_buffer_mutex);
                        return (-97);
                }
        }
                

#if 1
        if (GVP_vp_header_current.srcs == 0xffff){
                GVP_vp_header_current.gvp_time = (((guint64)((guint32)GVP_vp_header_current.chNs[15]))<<32) | (guint64)((guint32)GVP_vp_header_current.chNs[14]);
                GVP_vp_header_current.dataexpanded[14] = (double)GVP_vp_header_current.gvp_time/125e3;

#if GVP_DEBUG_VERBOSE > 2
                if (GVP_vp_header_current.endmark)
                        g_message ("N[ENDMARK] GVP_vp_header_current.srcs=%04x   Bias=%8g V    t=%8g ms",
                                   GVP_vp_header_current.srcs,
                                   rpspmc_to_volts (GVP_vp_header_current.chNs[3]),
                                   GVP_vp_header_current.dataexpanded[14]
                                   );
                else
                        g_message ("N[%4d / %4d] GVP_vp_header_current.srcs=%04x   Bias=%8g V    t=%8g ms",
                                   GVP_vp_header_current.index, GVP_vp_header_current.n, GVP_vp_header_current.srcs,
                                   rpspmc_to_volts (GVP_vp_header_current.chNs[3]),
                                   GVP_vp_header_current.dataexpanded[14]
                                   );
#endif
        }
#endif                        

        // check and process time code 
        if (ch_index+offset < GVP_stream_buffer_position){
                if ((GVP_vp_header_current.srcs & 0xc000) == 0xc000){ // fetch 64 bit time
                        GVP_vp_header_current.gvp_time = (((guint64)((guint32)GVP_vp_header_current.chNs[15]))<<32) | (guint64)((guint32)GVP_vp_header_current.chNs[14]);
                        GVP_vp_header_current.dataexpanded[14] = (double)GVP_vp_header_current.gvp_time/125e3;
                } else if ((GVP_vp_header_current.srcs & 0xc000) == 0x4000){ // fetch 32 bit time only
                        GVP_vp_header_current.gvp_time = (guint64)((guint32)GVP_vp_header_current.chNs[14]);
                        GVP_vp_header_current.dataexpanded[14] = (double)GVP_vp_header_current.gvp_time/125e3;
                } else if  ((GVP_vp_header_current.srcs & 0xc000) == 0x8000){ // fetch upper 32 bit time only -- unusual but possible
                        GVP_vp_header_current.gvp_time = ((guint64)((guint32)GVP_vp_header_current.chNs[15]))<<32;
                        GVP_vp_header_current.dataexpanded[14] = (double)GVP_vp_header_current.gvp_time/125e3;
                } // time stalls if not selected after header ref time
                
                retry=RECOVERY_RETRYS;
                if (GVP_vp_header_current.srcs == 0xffff){
                        g_mutex_unlock (&GVP_stream_buffer_mutex);
                        return -1; // true for full position header update
                }
                
                GVP_vp_header_current.ilast = GVP_vp_header_current.i; // next point should be this - 1

#ifdef BBB_DBG
                status_append_int32 (&GVP_stream_buffer[hdr], ch_index+offset+1-hdr, true, hdr, true);
                hdr = ch_index+offset+1; // next hdr
#endif

                
                g_mutex_unlock (&GVP_stream_buffer_mutex);
                return ch_index;

        } else {
                // g_message ("[%08x] *** end of new data at ch=%d ** Must wait for next page/update send and retry.", offset, ch_index);
                g_mutex_unlock (&GVP_stream_buffer_mutex);
                return -99;   // OK -- but have wait and reprocess when data package is completed
                // return ch_index; // number channels read until position
        }
};



/*

** Srcs: 0000c018 vSrcs: 0000c018
** make_dUZXYAB_vector PV[0] [N=0010] [      1000 pts/s, S:0000c018, O:00000000, #000, J00, dU      0 V, dX      0 V, dY      0 V, dZ      0 V, dA      0 V, dB      0 V, dAM      0 Veq, dFM      0 Veq]
Vec[ 0] XYZU: 0 0 0 0 V  [#10, R0 J0 OPT=00000000 SRCS=0000c018] initial Msk=0000
** make_dUZXYAB_vector PV[1] [N=0010] [      1000 pts/s, S:0000c018, O:00000001, #000, J00, dU      3 V, dX      0 V, dY    0.2 V, dZ    0.2 V, dA      0 V, dB      0 V, dAM      0 Veq, dFM      0 Veq]
Vec[ 1] XYZU: 0 0.2 0.2 3 V  [#10, R0 J0 OPT=00000001 SRCS=0000c018] initial Msk=0001
** make_dUZXYAB_vector PV[2] [N=0010] [      1000 pts/s, S:0000c018, O:00000001, #000, J00, dU     -3 V, dX      0 V, dY   -0.2 V, dZ   -0.2 V, dA      0 V, dB      0 V, dAM      0 Veq, dFM      0 Veq]
Vec[ 2] XYZU: 0 -0.2 -0.2 -3 V  [#10, R0 J0 OPT=00000001 SRCS=0000c018] initial Msk=0002
** make_dUZXYAB_vector PV[3] [N=0010] [      1000 pts/s, S:0000c018, O:00000000, #000, J00, dU      0 V, dX      0 V, dY      0 V, dZ      0 V, dA      0 V, dB      0 V, dAM      0 Veq, dFM      0 Veq]
Vec[ 3] XYZU: 0 0 0 0 V  [#10, R0 J0 OPT=00000000 SRCS=0000c018] initial Msk=0003
** append_null_vector PV[4] [N=0000] [         0 pts/s, S:00000000, O:00000000, #000, J00, dU      0 V, dX      0 V, dY      0 V, dZ      0 V, dA      0 V, dB      0 V, dAM      0 Veq, dFM      0 Veq]
Vec[ 4] XYZU: 0 0 0 0 V  [#0, R0 J0 OPT=00000000 SRCS=00000000] initial Msk=0004
** Executing Vector Probe Now! Mode: VP: Vector Program

  
  #ifdef BBB_DBG:

** WEBSOCKET STREAM TAG: RESET (GVP Start) Received.
          10 data points per section (10-1=0009 is first index countign down to 0
ADR-----: nnnnSRCS full header (ffff) and position vector (all sources) 16x32bit words are following ...
SECTION1:                 X        Y        Z        U        I      IN2      IN3      IN4   MUX DF   MUX EX   MUX PH   MUX AM   MUX LA   MUX LB [TIME64bit =====]        
00000000: 0009ffff 00000000 00000000 028f5c29 000d1b71 fffe4023 fe3e5e00 000fc800 fffefd00 000334c9 0ccc0000 000073f7 000c3743 000003b6 fffeef26 00000019 00000000
          ####SRCS        U        I [TIME64bit======]  (as of c018)

00000011: 0008c018 000d1b71 fffe3090 0001e964 00000000 
00000016: 0007c018 000d1b71 fffe1b1c 0003d2c4 00000000 
0000001b: 0006c018 000d1b71 fffe1db4 0005bc24 00000000 
00000020: 0005c018 000d1b71 fffe2287 0007a584 00000000 
00000025: 0004c018 000d1b71 fffe2c7a 00098ee4 00000000 
0000002a: 0003c018 000d1b71 fffe3a9e 000b7844 00000000 
0000002f: 0002c018 000d1b71 fffe2cd1 000d61a4 00000000 
00000034: 0001c018 000d1b71 fffe2692 000f4b04 00000000 
00000039: 0000c018 000d1b71 fffe20e1 00113464 00000000 

SECTION2:
0000003e: 0009ffff 00000000 00000000 028f5c29 000d1b71 fffe1b48 fe1c4c00 000f9f00 fffecf00 000334c9 0ccc0000 000073f1 000c374f 000003b6 fffee520 00131ee6
0000004e: 00000000
          0008c018 070873df fffe2054 0014dbfa 00000000 
00000054: 0007c018 0eb68858 fffe350c 0016c451 00000000 
00000059: 0006c018 16649cd1 fffe2d07 0018aca8 00000000 
0000005e: 0005c018 1e12b14a fffe2052 001a94ff 00000000 
00000063: 0004c018 25c0c5c3 fffe21b7 001c7d56 00000000 
00000068: 0003c018 2d6eda3c fffe3662 001e65ad 00000000 
0000006d: 0002c018 351ceeb5 fffe3ad5 00204e04 00000000 
00000072: 0001c018 3ccb032e fffe2feb 0022365b 00000000 
00000077: 0000c018 447917a7 fffe17ca 00241eb2 00000000 

0000007c: 0009ffff 00000000 051eb842 07ae146b 4cd9e82b fffe2494 fe291200 4c0f5500 fffed600 000334c9 0ccc0000 000073f6 000c374b 000003b6 fffec6dc 0026336e
0000008c: 00000000
          0008c018 45de8fbd fffe3ed8 00281bc5 00000000 
00000092: 0007c018 3e307b44 fffe34db 002a041c 00000000 
00000097: 0006c018 368266cb fffe21ae 002bec73 00000000 
0000009c: 0005c018 2ed45252 fffe2324 002dd4ca 00000000 
000000a1: 0004c018 27263dd9 fffe3488 002fbd21 00000000 
000000a6: 0003c018 1f782960 fffe2fd9 0031a578 00000000 
000000ab: 0002c018 17ca14e7 fffe1ea7 00338dcf 00000000 
000000b0: 0001c018 101c006e fffe200f 00357626 00000000 
000000b5: 0000c018 086debf5 fffe3026 00375e7d 00000000 

000000ba: 0009ffff 00000000 00000000 028f5c29 000d1b71 fffe3f06 fe43e600 000f7700 fffecf00 000334c9 0ccc0000 000073e2 000c374c 000003b6 fffecf16 00397339
000000ca: 00000000
          0008c018 000d1b71 fffe23c7 003b87dc 00000000 
000000d0: 0007c018 000d1b71 fffe1f92 003d713c 00000000 
000000d5: 0006c018 000d1b71 fffe23bd 003f5a9c 00000000 
000000da: 0005c018 000d1b71 fffe330c 004143fc 00000000 
000000df: 0004c018 000d1b71 fffe3e84 00432d5c 00000000 
000000e4: 0003c018 000d1b71 fffe31f9 004516bc 00000000 
000000e9: 0002c018 000d1b71 fffe2a99 0047001c 00000000 
000000ee: 0001c018 000d1b71 fffe20d8 0048e97c 00000000 
000000f3: 0000c018 000d1b71 fffe2668 004ad2dc 00000000

GVP END MARK DETECTED
ADR-----:  ENDMARK followed by full header and position vector (all sources) and time. Followed by additional GVP STREAM end marking pattern.
000000f8: fefefefe 00000000 00000009 06a7ef89 00000001 000002d5 fe238a00 0002e200 ffff3f00 000334c9 0ccc0000 0000744e 000c353c fffff850 ffff3462 004cbd5e
00000108: 00000000 ffffeeee ffffeeee ffffeeee ffffeeee ffffeeee ffffeeee ffffeeee ffffeeee ffffeeee ffffeeee ffffeeee ffffeeee ffffeeee ffffeeee ffffeeee
00000118: ffffeeee 00000010 0000000f 0000000e 0000000d 0000000c 0000000b 0000000a 00000009 00000008 00000007 00000006 00000005 00000004 00000003 00000002
00000128: 00000001 eeeeeeee eeeeeeee eeeeeeee eeeeeeee eeeeeeee eeeeeeee eeeeeeee eeeeeeee eeeeeeee eeeeeeee eeeeeeee eeeeeeee eeeeeeee eeeeeeee eeeeeeee


****
===>>>> SET-VP  i[0] sec=0 t=0.0002 ms   #valid sec{1}
00000000: 0009ffff 00000000 019521b7 083d1137 17bcf977 0017802f fe307400 17802f00 fffeea00 000334c9 0ccc0000 00007486 000c352a fffff83b fffed35c 00000019
00000010: 00000000
          0008c018 17bcf977 00178037 0001e964 00000000 
00000016: 0007c018 17bcf977 00178069 0003d2c4 00000000 
0000001b: 0006c018 17bcf977 00178047 0005bc24 00000000 
00000020: 0005c018 17bcf977 00178049 0007a584 00000000 
00000025: 0004c018 17bcf977 00178053 00098ee4 00000000 
0000002a: 0003c018 17bcf977 001780c0 000b7844 00000000 
0000002f: 0002c018 17bcf977 0017801a 000d61a4 00000000 
00000034: 0001c018 17bcf977 0017807d 000f4b04 00000000 
00000039: 0000c018 17bcf977 00178047 00113464 00000000 
** Message: 12:56:28.909: VP: Waiting for Section Header [1] StreamPos=0x000000fc
** Message: 12:56:28.909: VP: section header ** reading pos[00fc] off[003e] #AB=1
** Message: 12:56:28.909: Reading VP section header...
===>>>> SET-VP  i[10] sec=1 t=10.0248 ms   #valid sec{11}
0000003e: 0009ffff 00000000 019521b7 083d1137 17bcf977 0017807d fe2a4e00 17807a00 ffff5a00 000334c9 0ccc0000 00007480 000c352a fffff83b ffff2b5c 00131ee6
0000004e: 00000000
          0008c018 1eb851e5 001e6901 0014dbfa 00000000 
00000054: 0007c018 2666665e 002602ba 0016c451 00000000 
00000059: 0006c018 2e147ad7 002d9c4c 0018aca8 00000000 
0000005e: 0005c018 35c28f50 003535da 001a94ff 00000000 
00000063: 0004c018 3d70a3c9 003ccf8e 001c7d56 00000000 
00000068: 0003c018 451eb842 004468bc 001e65ad 00000000 
0000006d: 0002c018 4cccccbb 004c0248 00204e04 00000000 
00000072: 0001c018 547ae134 00539c00 0022365b 00000000 
00000077: 0000c018 5c28f5ad 005b355b 00241eb2 00000000 
** Message: 12:56:28.909: VP: Waiting for Section Header [1] StreamPos=0x000000fc
** Message: 12:56:28.909: VP: section header ** reading pos[00fc] off[007c] #AB=1
** Message: 12:56:28.909: Reading VP section header...

0000007c: 0000007d 0000007e 0000007f 00000080 00000081 00000082 00000083 00000084 00000085 00000086 00000087 00000088 00000089 0000008a 0000008b 0000008c
0000008c: 0000008d 0009ffff 00000000 06a7ef92 0d4fdf12 63d70a26 0062cf18 fe365200 62cf1800 ffff5600 000334c9 0ccc0000 00007475 000c3525 fffff83b ffff5b04
0000009c: 00268c38 00000000 0008c018 63d70a26 0062cefc 0028a0db 00000000 0007c018 63d70a26 0062cf29 002a8a3b 00000000 0006c018 63d70a26 0062cf2c 002c739b
000000ac: 00000000 0005c018 63d70a26 0062cf26 002e5cfb 00000000 0004c018 63d70a26 0062cf45 0030465b 00000000 0003c018 63d70a26 0062cf2d 00322fbb 00000000
000000bc: 0002c018 63d70a26 0062cf14 0034191b 00000000 0001c018 63d70a26 0062cef5 0036027b 00000000 0000c018 63d70a26 0062cec3 0037ebdb 00000000 fefefefe
000000cc: 00000000 06a7ef92 0d4fdf12 63d70a26 0062cef4 fe1ed400 62cef400 ffff2b00 000334c9 0ccc0000 00007469 000c3528 fffff83b ffff34de 0039d65d 00000000
000000dc: ffffeeee ffffeeee ffffeeee ffffeeee ffffeeee ffffeeee ffffeeee ffffeeee ffffeeee ffffeeee ffffeeee ffffeeee ffffeeee ffffeeee ffffeeee ffffeeee
000000ec: 00000010 0000000f 0000000e 0000000d 0000000c 0000000b 0000000a 00000009 00000008 00000007 00000006 00000005 00000004 00000003 00000002 00000001
000000fc: eeeeeeee eeeeeeee eeeeeeee eeeeeeee eeeeeeee eeeeeeee eeeeeeee eeeeeeee eeeeeeee eeeeeeee eeeeeeee eeeeeeee eeeeeeee eeeeeeee eeeeeeee eeeeeeee

*******************

** II: Auto calc init vector to absolute position [-0.2 0.2 0 0] V
** II: XYZU-GVP readings are => [-0.0563332 0.197995 0 -0.943321] V, Bias (actual)=0 V
** II: [PC=0,setMSK:1003] AUTO INIT **{dXYZU => [-0.143667 0.00200461 0 0]  dA 0 dB 0 dAM 0 dFM 0] V}
  ** II: BEST DELTA PRECISION CALC: ddmin=2.02486e-05 => 0.00698492  nii=3
  ** II: FINAL: decii=1178511 nii=3 t=0.0282843 s slew=35.3553 pts/s, dmin=0.00200461 V, dt=0.0282843 s, NII##=3.53553e+06, Nsteps=300 {ddmin=2.02486e-05 uV #8696.72 ddmax=0.00145118 uV #623277}

Write Vector[PC=000] = [[0100], {0003}, Op04030000, Sr00000013, #0000, 0000, dX -478.8894432 uV, dY 6.682044519 uV, dZ        0 uV, dU        0 uV, dA        0 uV, dB        0 uV, dAMc        0 uV, dFMc        0 uV, 0,0,0, decii=1178511]
** New Params **
** adjusting rotation to 0
  ** II: BEST DELTA PRECISION CALC: ddmin=0.00100251 => 0.00698492  nii=3
  ** II: FINAL: decii=416667 nii=3 t=0.01 s slew=100 pts/s, dmin=0.4 V, dt=0.01 s, NII##=1.25e+06, Nsteps=1200 {ddmin=0.00100251 uV #430573 ddmax=0.00100251 uV #430573}

Write Vector[PC=001] = [[0400], {0003}, Op00000000, Sr00000013, #0000, 0000, dX 333.3333333 uV, dY        0 uV, dZ        0 uV, dU        0 uV, dA        0 uV, dB        0 uV, dAMc        0 uV, dFMc        0 uV, 0,0,0, decii=416667]
** New Params **
** adjusting rotation to 0
  ** II: BEST DELTA PRECISION CALC: ddmin=0.00100251 => 0.00698492  nii=3
  ** II: FINAL: decii=416667 nii=3 t=0.01 s slew=100 pts/s, dmin=0.4 V, dt=0.01 s, NII##=1.25e+06, Nsteps=1200 {ddmin=0.00100251 uV #430573 ddmax=0.00100251 uV #430573}

Write Vector[PC=002] = [[0400], {0003}, Op00000000, Sr00000000, #0000, 0000, dX -333.3333333 uV, dY        0 uV, dZ        0 uV, dU        0 uV, dA        0 uV, dB        0 uV, dAMc        0 uV, dFMc        0 uV, 0,0,0, decii=416667]
** New Params **
** adjusting rotation to 0
  ** II: BEST DELTA PRECISION CALC: ddmin=0.00011139 => 0.00698492  nii=3
  ** II: FINAL: decii=41667 nii=3 t=0.00100001 s slew=1000 pts/s, dmin=0.00100251 V, dt=0.001 s, NII##=125000, Nsteps=30 {ddmin=0.00011139 uV #47841.5 ddmax=0.00011139 uV #47841.5}

Write Vector[PC=003] = [[0010], {0003}, Op00000000, Sr00000013, #0399, -002, dX        0 uV, dY -33.41687553 uV, dZ        0 uV, dU        0 uV, dA        0 uV, dB        0 uV, dAMc        0 uV, dFMc        0 uV, 0,0,0, decii=41667]
** New Params **
** adjusting rotation to 0
** II: ALLOWING HIGH SPEED GVP recording as no DAC changes detected.

Write Vector[PC=004] = [[0000], {0000}, Op00000000, Sr00000000, #0000, 0000, dX        0 uV, dY        0 uV, dZ        0 uV, dU        0 uV, dA        0 uV, dB        0 uV, dAMc        0 uV, dFMc        0 uV, 0,0,0, decii=2]
** New Params **
** adjusting rotation to 0
##Configure GVP: Reset RO[-1]
*** GVP Control Mode EXEC: set GVP to RESET
*** GVP Control Mode EXEC: reset stream server, clear buffer)
*** GVP Control Mode EXEC: START DMA+stream server

SCAN 400x400

** VECTOR:
VectorDef{   Decii,        0,     dFM,    dAM,    dB,     dA,     dU,     dZ,     dY,         dX,          nxt,    reps,      opts,     srcs      nii,     n ,       pc}
vector = {32'd1178511, 32'd0,  32'd0,  32'd0,  32'd0,  32'd0,  32'd0,  32'd0,  32'd418,   -32'd151798,  32'd0,  32'd000,   32'h1300, 32'h1300,  32'd2,  32'd99,   32'd0}; // Vector #0
vector = {32'd416667,  32'd0,  32'd0,  32'd0,  32'd0,  32'd0,  32'd0,  32'd0,  32'd0,      32'd143166,  32'd0,  32'd000,   32'h1300, 32'h1300,  32'd2,  32'd399,  32'd1}; // Vector #1
vector = {32'd416667,  32'd0,  32'd0,  32'd0,  32'd0,  32'd0,  32'd0,  32'd0,  32'd0,     -32'd143166,  32'd0,  32'd000,   32'h000,  32'h0,     32'd2,  32'd399,  32'd2}; // Vector #2
vector = {32'd41667,   32'd0,  32'd0,  32'd0,  32'd0,  32'd0,  32'd0,  32'd0, -32'd14352,  32'd0,      -32'd2,  32'd39900, 32'h1300, 32'h1300,  32'd2,  32'd9,    32'd3}; // Vector #3
vector = {32'd2,       32'd0,  32'd0,  32'd0,  32'd0,  32'd0,  32'd0,  32'd0,  32'd0,      32'd0,       32'd0,  32'd000,   32'h000,  32'h0,     32'd0,  32'd0,    32'd4}; // Vector #4
**

 */
