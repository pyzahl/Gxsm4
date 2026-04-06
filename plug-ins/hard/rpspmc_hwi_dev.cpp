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
#include "rpspmc_hwi_dev.h"
#include "rpspmc_control.h"
#include "rpspmc_pacpll.h"
#include "rpspmc_stream.h"

#include "rpspmc_conversions.h"



#define TEST_DMA128 1        




// Define HwI PlugIn reference name here, this is what is listed later within "Preferenced Dialog"
// i.e. the string selected for "Hardware/Card"!
#define THIS_HWI_PLUGIN_NAME "SPM-TEMPL:SPM"
#define THIS_HWI_PREFIX      "SPM_TEMPL_HwI"

extern int debug_level;
extern SOURCE_SIGNAL_DEF rpspmc_source_signals[];
extern SOURCE_SIGNAL_DEF rpspmc_swappable_signals[];
extern SOURCE_SIGNAL_DEF z_servo_current_source[];

extern RPspmc_pacpll *rpspmc_pacpll;
extern GxsmPlugin rpspmc_pacpll_hwi_pi;
extern RPSPMC_Control *RPSPMC_ControlClass;
extern GxsmPlugin rpspmc_hwi_pi;

extern rpspmc_hwi_dev  *rpspmc_hwi;

// HwI Implementation
// ================================================================================
void ext_message_func(const gchar* msg, bool sched){
        rpspmc_hwi -> status_append (msg, sched);
}

rpspmc_hwi_dev::rpspmc_hwi_dev():RP_stream(){
        g_mutex_init (&RTQmutex);
        
        z_offset_internal = 0.;
        info_blob = NULL;
        
        delayed_tip_move_update_timer_id = 0;
        rpspmc_history=NULL;
        history_block_size = 0;


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
        
        // auto adjust and override preferences

        hwi_init_overrides(); // !!!! Note: must re-overrride after life Gxsm Preferences Adjustments

        subscan_data_y_index_offset = 0;
        ScanningFlg=0;
        KillFlg=FALSE;

        for (int i=0; i<4; ++i){
                srcs_dir[i] = nsrcs_dir[i] = 0;
                Mob_dir[i] = NULL;
        }

        g_free (AddStatusString);
        AddStatusString = g_strdup_printf ("RPSPMC HwI initialized. Not connected.");

        //set_message_func (&rpspmc_hwi_dev::status_append); // not effin possible w/o static?!?!
        set_message_func (ext_message_func);
        
        //gint32 *GVP_stream_buffer=new gint32[0x1000000]; // temporary
}

rpspmc_hwi_dev::~rpspmc_hwi_dev(){
}

void rpspmc_hwi_dev::hwi_init_overrides(){ // auto adjust and override preferences
        g_message ("Reconfiguring HwI ** rpspmc_hwi_dev : public XSM_Hardware :: hwi_init_overrides()");
        main_get_gapp()->xsm->Inst->override_dig_range (1<<20);    // gxsm does precision sanity checks and trys to round to best fit grid
        main_get_gapp()->xsm->Inst->override_volt_in_range (5.0);  // FOR AD4630-24 24bit
        main_get_gapp()->xsm->Inst->override_volt_out_range (5.0); // PMODs AD5791-20 20bit
        
        update_hardware_mapping_to_rpspmc_source_signals ();
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
        // update global hardware info
        g_free (AddStatusString);
        AddStatusString = g_strdup_printf ("RPSPMC on %s connected.", rpspmc_pacpll->get_rp_address ());
        strncpy(xsmres.DSPDev, AddStatusString, PATHSIZE);

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

void rpspmc_hwi_dev::skip_to_next_header (){
        //g_message ("rpspmc_hwi_dev::skip_to_next_header from 0x%08x   #CH: %d", GVP_stream_buffer_offset, GVP_vp_header_current.number_channels);
#if TEST_DMA128
        GVP_stream_buffer_offset += 4 + ((((GVP_vp_header_current.number_channels-1)>>2)+1)<<2); // skip forward by read number of entries
#else
        GVP_stream_buffer_offset += 1 + GVP_vp_header_current.number_channels; // skip forward on success by read number of entries
#endif
        //g_message ("rpspmc_hwi_dev::skip_to_next_header to   0x%08x", GVP_stream_buffer_offset);
}

// ScanDataReadThread:
// Image Data read thread -- actual scan/pixel data transfer

int rpspmc_hwi_dev::GVP_expect_header(double *pv, int &index_all){
        int ret = -100;
        do {
                if (GVP_stream_buffer_AB >= 0){ // make sure stream actually started!
                        ret = read_GVP_data_block_to_position_vector (GVP_stream_buffer_offset, true); // NEED FULL HEADER

                        if (ret == -1){ // GOT FULL HEADER
                                skip_to_next_header ();
                                
                                // next section, load header, manage pc
                                GVP_vp_header_current.section = RPSPMC_ControlClass->next_section (GVP_vp_header_current.section);
                                        
#if GVP_DEBUG_VERBOSE > 2
                                g_message (" *** OK, loading pv sec[%d] *** timestamp: %g ms", GVP_vp_header_current.section, GVP_vp_header_current.gvp_time_ms);
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
                                // header, data point FPGA time stamp
                                pv[PROBEDATA_ARRAY_TIME]  = GVP_vp_header_current.gvp_time_ms;     // == dataexpanded[PROBEDATA_ARRAY_MS_TIME];     // time in ms
                                pv[PROBEDATA_ARRAY_MS_TIME] = GVP_vp_header_current.gvp_time_ms;     // == dataexpanded[PROBEDATA_ARRAY_MS_TIME];     // time in ms
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
                                pv[PROBEDATA_ARRAY_S15]   = GVP_vp_header_current.dataexpanded[14]; // **
                                pv[PROBEDATA_ARRAY_S16]   = GVP_vp_header_current.dataexpanded[15]; // **

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
                        skip_to_next_header ();
                        
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
                                                pvlut[dir][ch] = PROBEDATA_ARRAY_MS_TIME - PROBEDATA_ARRAY_S1;  //17 //14; /// NOTE: GVP_vp_header_current.dataexpanded[14]; // is time in ms
                                        else
                                                pvlut[dir][ch] = rpspmc_source_signals[i].garr_index - PROBEDATA_ARRAY_S1; // as ..._S1 .. _S14 are exactly incremental => 0..13 ; see (***) above
                                        g_message ("GVP Data Expanded Lookup table signal %02d is selected: pvlut[%02d][%02d] = %02d for mask 0x%08x (%s), garri %d", i,dir,ch,pvlut[dir][ch], rpspmc_source_signals[i].mask, rpspmc_source_signals[i].label, rpspmc_source_signals[i].garr_index);
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
                                                int xi_mapped = dir == 0 ? xi : x0 + nx-1 - (xi-x0); // mirror xi to correctly put data into mem2d now matching the forward direction
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
                                                                hwi->Mob_dir[dir][ch]->PutDataPkt (hwi->GVP_vp_header_current.dataexpanded [k], xi_mapped, yi); // direct use, skipping pv[] remapping ** data is coming in RP base unit Volts
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
                RPSPMC_ControlClass->add_probedata (GVP_vp_header_current.dataexpanded, pv,
                                                    GVP_vp_header_current.fss_data,  GVP_vp_header_current.fss_len,
                                                    true, true);
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
                g_message ("VP: Waiting for Section Header [%d] StreamPos=0x%08x  Next expected @ 0x%08x", GVP_stream_buffer_AB, GVP_stream_buffer_position, GVP_stream_buffer_offset);
                if (GVP_stream_buffer_AB >= 0){
                        g_message ("VP: section header ** reading pos[%04x] off[%04x] #AB=%d", GVP_stream_buffer_position, GVP_stream_buffer_offset, GVP_stream_buffer_AB);
                        g_message ("Reading VP section header...");
                        ret = read_GVP_data_block_to_position_vector (GVP_stream_buffer_offset, true); // NEED FULL HEADER
                        if (ret == -1){ // GOT FULL HEADER
                                skip_to_next_header ();

                                // next section, load header, manage pc
                                GVP_vp_header_current.section = RPSPMC_ControlClass->next_section (GVP_vp_header_current.section);
                                        
                                // g_message (" *** OK, loading pv sec[%d] ***", GVP_vp_header_current.section);

                                // copy header to pv[] as assigned below
                                pv[PROBEDATA_ARRAY_SRCS]    = GVP_vp_header_current.srcs; // new
                                pv[PROBEDATA_ARRAY_INDEX]   = (double)index_all++;
                                pv[PROBEDATA_ARRAY_PHI]     = (double)GVP_vp_header_current.index; // testing, point index in section
                                pv[PROBEDATA_ARRAY_SEC]     = (double)GVP_vp_header_current.section;
                                pv[PROBEDATA_ARRAY_TIME]    = GVP_vp_header_current.gvp_time_ms;     // time in ms
                                pv[PROBEDATA_ARRAY_MS_TIME] = GVP_vp_header_current.gvp_time_ms;     // time in ms
                                pv[PROBEDATA_ARRAY_XS]      = GVP_vp_header_current.dataexpanded[0]; // Xs in Volts
                                pv[PROBEDATA_ARRAY_YS]      = GVP_vp_header_current.dataexpanded[1]; // Ys in Volts
                                pv[PROBEDATA_ARRAY_ZS]      = GVP_vp_header_current.dataexpanded[2]; // Zs in Volts
                                pv[PROBEDATA_ARRAY_U ]      = GVP_vp_header_current.dataexpanded[3]; // Bias in Volts

#if GVP_DEBUG_VERBOSE > 2
                                g_message ("PVh sec[%g] i=%g t=%g ms U=%g V",
                                           pv[PROBEDATA_ARRAY_SEC],pv[PROBEDATA_ARRAY_INDEX],
                                           pv[PROBEDATA_ARRAY_TIME],pv[PROBEDATA_ARRAY_U ]);
#endif

                        } else {
                                if (GVP_vp_header_current.endmark){ // checking -- finished?
                                        // add last point
                                        RPSPMC_ControlClass->add_probedata (GVP_vp_header_current.dataexpanded, pv,
                                                                            GVP_vp_header_current.fss_data,  GVP_vp_header_current.fss_len,
                                                                            false, true);
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

                // Add data
                RPSPMC_ControlClass->add_probedata (GVP_vp_header_current.dataexpanded, pv,
                                                    GVP_vp_header_current.fss_data,  GVP_vp_header_current.fss_len,
                                                    true, true);

                point_index = 0;

                need_hdr = FR_NO;
        }

        // KEED READING DATA...
        
        clock_t ct = clock();
        //g_message ("VP: section: %d gvpi[%04d]", GVP_vp_header_current.section, GVP_vp_header_current.i);
        for (; GVP_vp_header_current.i < GVP_vp_header_current.n; ){
                ret = read_GVP_data_block_to_position_vector (GVP_stream_buffer_offset);
                if (ret == GVP_vp_header_current.number_channels){
                        skip_to_next_header ();

                        //g_message ("VP [%04d] of %d\n", GVP_vp_header_current.i, GVP_vp_header_current.n);

                        // add vector and data to expanded data array representation
                        pv[PROBEDATA_ARRAY_INDEX] = (double)index_all++;
                        RPSPMC_ControlClass->add_probedata (GVP_vp_header_current.dataexpanded, pv,
                                                            GVP_vp_header_current.fss_data,  GVP_vp_header_current.fss_len,
                                                            false, true); // using diect mapping from expanded header //(GVP_vp_header_current.i < (GVP_vp_header_current.n-1)) ? true:false);

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
                                                                    slew, subscan, srcs_dir, 0, y_start);  // y_start: 0: top-down, else bottom-up
                

                g_message ("Scan GVP written and send. Preparing for start. y_start: %d", y_start);

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

                g_message ("Assigning SRCS 0x%08x", srcs_mask);
                for (int i=0; rpspmc_source_signals[i].mask; ++i){
                        //g_message ("Checking [%d] for 0x%08x (%s)", i, rpspmc_source_signals[i].mask, rpspmc_source_signals[i].label);
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
        double dvec[20];
        int rt=0;
        if (rpspmc_pacpll) // try fetch SHM monitor data
                rt = rpspmc_pacpll->memcpy_from_rt_monitors (dvec, 20, 100+400-20); // last

        if (*property == '!'){ // Scan Coordinates: ZScan, XScan, YScan  with offset!! -- in volts with gains!
                val1 = spmc_parameters.gxsm_z_polarity*spmc_parameters.z_monitor*main_get_gapp()->xsm->Inst->VZ(); // adjust for polarity as Z-Monitor is the actual DAC OUT Z
                val2 = spmc_parameters.x_monitor*main_get_gapp()->xsm->Inst->VX();
                val3 = spmc_parameters.y_monitor*main_get_gapp()->xsm->Inst->VY();
		return TRUE;
	}
        if (*property == 'z'){ // Scan Coordinates: ZScan, XScan, YScan  with offset!! -- in volts with gains!
                if (rt) {
                        val1 = dvec[7]*spmc_parameters.gxsm_z_polarity*main_get_gapp()->xsm->Inst->VZ(); // adjust for polarity as Z-Monitor is the actual DAC OUT Z
                        val2 = dvec[1]*main_get_gapp()->xsm->Inst->VX();
                        val3 = dvec[4]*main_get_gapp()->xsm->Inst->VY();
                } else {
                        val1 = spmc_parameters.gxsm_z_polarity*spmc_parameters.z_monitor*main_get_gapp()->xsm->Inst->VZ(); // adjust for polarity as Z-Monitor is the actual DAC OUT Z
                        val2 = spmc_parameters.x_monitor*main_get_gapp()->xsm->Inst->VX();
                        val3 = spmc_parameters.y_monitor*main_get_gapp()->xsm->Inst->VY();
                }
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
                if (rt) {
                        val1 = dvec[17]; // dFreq: **VEC_DFREQ 17
                        val2 = main_get_gapp()->xsm->Inst->V2nAmpere (dvec[19]); // Reading converted to nA
                        val3 = dvec[19]; // Current Input reading in Volts (+/-1 V for RF-IN2, +/-5V for AD24-IN3 **VEC_ZSSIG 19
                } else {
                        val1 = pacpll_parameters.dfreq_monitor; // Freq Shift
                        val2 = main_get_gapp()->xsm->Inst->V2nAmpere (spmc_parameters.signal_monitor); // Reading converted to nA
                        val3 = spmc_parameters.signal_monitor; // Current Input reading in Volts (+/-1 V for RF-IN2, +/-5V for AD24-IN3
                }
		return TRUE;
	}

        if (*property == 'Z'){ // probe Z -- well total Z here again
                val1 = rpspmc_pacpll_hwi_pi.app->xsm->Inst->Volt2ZA (spmc_parameters.gxsm_z_polarity*spmc_parameters.z_monitor); // Z pos in Ang,  adjust for polarity as Z-Monitor is the actual DAC OUT Z
		return TRUE;
        }

        if (*property == 'W'){
                val2=val3=val1 = ScanningFlg ? 1.:0.;               
                return ScanningFlg ? TRUE : FALSE;
        }

	// DSP Status Indicators
	if (*property == 's' || *property == 'S'){
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
        static int signal[4] = { 7, 19, 15, 17 }; // Z, Current per default
        static double scale[4] = { 1.0, 1.0, 1.0, 1.0 };
        
        // Request History Vector n: "Hnn"
        if (property[0] == 'H'){
                int pos = atoi(&property[1]);
                if (pos < 0 || pos >= 20) pos=0;
                get_history_vector_f (pos, data, n);
                return 0;
        }

        // Request signal[0] = Vector n: from Channel "C1xxxx"
        // Request signal[1] = Vector n: from Channel "C2xxxx"
        // ...
        // Request signal[3] = Vector n: from Channel "C4xxxx"
        //                           0  1          4          7          10  11  12  13  14  15  16   17    18    19
        // xxxx is vector component [T  X xma xmi  Y yma ymi  Z zma zmi  U   IN1 IN2 IN3 IN4 AMP EXEC DFREQ PHASE CURR ] 0 ... 19 are valid
        if (property[0] == 'C'){
                static gchar *VCmap[] = { "T", "X", "xma", "xmi",  "Y", "yma", "ymi",  "Z", "zma", "zmi",  "BIAS",
                                          "IN1", "IN2", "IN3", "IN4", "AMP", "EXEC", "DFREQ", "PHASE", "CURR", NULL };
                static gchar *Umap[]   = { "s\0\0\0", "Å\0\0\0", "Å\0\0\0", "Å\0\0\0", "Å\0\0\0", "Å\0\0\0", "Å\0\0\0", "Å\0\0\0", "Å\0\0\0", "Å\0\0\0",  "V\0\0\0", // NOTE UTF Å is 2 chars
                                           "V\0\0\0", "V\0\0\0", "V\0\0\0", "V\0\0\0",  "mV\0\0",  "mV\0\0",  "Hz\0\0", "°\0\0\0",  "nA\0\0", NULL };
                int pos=0;
                for (pos=0; VCmap[pos]; ++pos) if (!strcmp(&property[2], VCmap[pos])) break;
                if (pos < 0 || pos >= 20) pos=19; // CURR as fallback

                int ch=-1;
                switch (property[1]){
                case '1': ch=0; break;
                case '2': ch=1; break;
                case '3': ch=2; break;
                case '4': ch=3; break;
                }
                if (ch>=0){
                        signal[ch] = pos;
                        if (data)
                                memcpy ((void*)data, Umap[pos], 4);
                } else return -1;

                // update scale to World Units if non native
                scale[ch] = 1.; // default native, no scale
                switch (signal[ch]){
                case 1: case 2: case 3: scale[ch] = main_get_gapp()->xsm->Inst->Volt2XA(1.); break;
                case 4: case 5: case 6: scale[ch] = main_get_gapp()->xsm->Inst->Volt2YA(1.); break;
                case 7: case 8: case 9: scale[ch] = main_get_gapp()->xsm->Inst->Volt2ZA(1.); break;
                case 10: scale[ch] = main_get_gapp()->xsm->Inst->BiasGainV2V(); break;
                case 19: scale[ch] = main_get_gapp()->xsm->Inst->nAmpere2V(); break;
                }

                return pos;
        }
        
        // Request signal[0] = Vector n: "S1", ... "S4", "ST"
        if (property[0] == 'S'){
                int ch = -1;
                // signal[0]
                switch (property[1]){
                case '1': ch = 0; break;
                case '2': ch = 1; break;
                case '3': ch = 2; break;
                case '4': ch = 3; break;
                case 'T': get_history_vector_f (0, data, n); return 0; break; // FPGA Time
                }
                if (ch >= 0){
                        if (signal[ch] >= 0 && signal[ch] < 20){
                                get_history_vector_f (signal[ch], data, n);
                                if (scale[ch] != 1.0) // if required
                                        for (int k=0; k<n; ++k) data[k] *= scale[ch];
                        }
                        return 0;
                }
        }

        if (property[0] == 'L')
                return (gint)get_history_len ();
        
        return 0;
}

gint rpspmc_hwi_dev::RTQuery (const gchar *property, int n, double *data){
        static int signal[4] = { 7, 19, 15, 17 }; // Z, Current per default
        static double scale[4] = { 1.0, 1.0, 1.0, 1.0 };
        
        // Request History Vector n: "Hnn"
        if (property[0] == 'H'){
                int pos = atoi(&property[1]);
                if (pos < 0 || pos >= 20) pos=0;
                get_history_vector (pos, data, n);
                return 0;
        }

        // Request signal[0] = Vector n: from Channel "C1xxxx"
        // Request signal[1] = Vector n: from Channel "C2xxxx"
        // ...
        // Request signal[3] = Vector n: from Channel "C4xxxx"
        //                           0  1          4          7          10  11  12  13  14  15  16   17    18    19
        // xxxx is vector component [T  X xma xmi  Y yma ymi  Z zma zmi  U   IN1 IN2 IN3 IN4 AMP EXEC DFREQ PHASE CURR ] 0 ... 19 are valid
        if (property[0] == 'C'){
                static gchar *VCmap[] = { "T", "X", "xma", "xmi",  "Y", "yma", "ymi",  "Z", "zma", "zmi",  "BIAS",
                                          "IN1", "IN2", "IN3", "IN4", "AMP", "EXEC", "DFREQ", "PHASE", "CURR", NULL };
                static gchar *Umap[]   = { "s\0\0\0", "Å\0\0\0", "Å\0\0\0", "Å\0\0\0", "Å\0\0\0", "Å\0\0\0", "Å\0\0\0", "Å\0\0\0", "Å\0\0\0", "Å\0\0\0",  "V\0\0\0", // NOTE UTF Å is 2 chars
                                           "V\0\0\0", "V\0\0\0", "V\0\0\0", "V\0\0\0",  "mV\0\0",  "mV\0\0",  "Hz\0\0", "°\0\0\0",  "nA\0\0", NULL };
                int pos=0;
                for (pos=0; VCmap[pos]; ++pos) if (!strcmp(&property[2], VCmap[pos])) break;
                if (pos < 0 || pos >= 20) pos=19; // CURR as fallback

                int ch=-1;
                switch (property[1]){
                case '1': ch=0; break;
                case '2': ch=1; break;
                case '3': ch=2; break;
                case '4': ch=3; break;
                }
                if (ch>=0){
                        signal[ch] = pos;
                        if (data)
                                memcpy ((void*)data, Umap[pos], 4);
                } else return -1;

                // update scale to World Units if non native
                scale[ch] = 1.; // default native, no scale
                switch (signal[ch]){
                case 1: case 2: case 3: scale[ch] = main_get_gapp()->xsm->Inst->Volt2XA(1.); break;
                case 4: case 5: case 6: scale[ch] = main_get_gapp()->xsm->Inst->Volt2YA(1.); break;
                case 7: case 8: case 9: scale[ch] = main_get_gapp()->xsm->Inst->Volt2ZA(1.); break;
                case 10: scale[ch] = main_get_gapp()->xsm->Inst->BiasGainV2V(); break;
                case 19: scale[ch] = main_get_gapp()->xsm->Inst->nAmpere2V(); break;
                }

                return pos;
        }
        
        // Request signal[0] = Vector n: "S1", ... "S4", "ST"
        if (property[0] == 'S'){
                int ch = -1;
                // signal[0]
                switch (property[1]){
                case '1': ch = 0; break;
                case '2': ch = 1; break;
                case '3': ch = 2; break;
                case '4': ch = 3; break;
                case 'T': get_history_vector (0, data, n); return 0; break; // FPGA Time
                }
                if (ch >= 0){
                        if (signal[ch] >= 0 && signal[ch] < 20){
                                get_history_vector (signal[ch], data, n);
                                if (scale[ch] != 1.0) // if required
                                        for (int k=0; k<n; ++k) data[k] *= scale[ch];
                        }
                        return 0;
                }
        }

        if (property[0] == 'L')
                return (gint)get_history_len ();
        
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
        on_new_data (NULL, 0, true); // GVP stream

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
#define I_GVP_SRCSB     4
#define I_GVP_NREP      5
#define I_GVP_NEXT      6
#define I_GVPX_OPCD     7
#define I_GVPX_RCHI     8
#define I_GVPX_JMPR     9
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
                "SPMC_GVP_VECTORSRCB",
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

#if TEST_DMA128
        unsigned int srcs_indices_list_A = 0;
        unsigned int srcs_indices_list_B = 0;
        int channel=-1;
        int last=-1;
        //g_message ("*** TEST DMA 128 CHANNEL INDICES for SRCS in vec #%d code %08x: ", i ,v->srcs);
        for (int k=0; k<16; ++k){
                for (int q=channel+1; q < 16; q++) // 16 max so far, excluding time now
                        if (v->srcs & (1<<q)) { channel = q; break; }
                if (channel < 0)
                        break;
                g_print ("%d=>%d, ", k,channel);
                if (k<8)
                        srcs_indices_list_A |= channel << (4*k);
                else
                        srcs_indices_list_B |= channel << (4*(k-8));
        }
        //g_print ("=> BA: %08x %08x \n****\n", srcs_indices_list_B, srcs_indices_list_A);
        gvp_vector_i [I_GVP_SRCS ] = srcs_indices_list_A;
        gvp_vector_i [I_GVP_SRCSB] = srcs_indices_list_B; // =0 if not using more than 8 channels total
#else
        // currently 24 bits, actaully 16 mapped used in stream serialization, with 6 been multiplexd from 32 = 42 possible different signals. May be expandd to 32 if needed.
        gvp_vector_i [I_GVP_SRCS    ] = (src_time_msk | v->srcs) & 0xffffffff;
        // g_message ("GVP_write_program_vector[%d]: srcs = 0x%08x", i, gvp_vector_i [I_GVP_OPTIONS ] );
#endif
        
        //const int src_time_msk = 0xc000; // 48bit time mon mask
        // Vector Program Code Setup
        gvp_vector_i [I_GVP_PC_INDEX] = i;
        gvp_vector_i [I_GVP_N       ] = v->n;
        //gvp_vector_i [I_GVP_OPTIONS ] = (((v->srcs | src_time_msk) & 0xffffff) << 8) | (v->options & 0xff); // 0: FB, 7: XVEC  **   ((v->options & VP_FEEDBACK_HOLD) ? 0:1) | (1<<7) | (1<<6) | (1<<5);
        gvp_vector_i [I_GVP_OPTIONS ] = v->options; // currently only lowest 8 bits
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

#if TEST_DMA128
/*
NEW STRUCTURE/HDR/SRCS
          SRCS-ALL      N-1     TTTTTTTTTTTTT        0        1        2        3        4        5        6        7        8        9       10      11
00000000: 0000ffff 0000000f 00000005 00000000 00000000 00000000 c0000000 00000000 0009a758 b56e18d5 0009a753 00016348 00000000 33320000 00006c0f 0031205e
                12       13       14       15     SRCS      N-2     TTTTTTTTTTTTT        0        1        2        3     SRCS      N-3     TTTTTTTTTTTTT
00000010: 00000094 000163e7 b4b4b4b4 ed2efbfe 00000059 0000000e 0000049a 00000000 00000000 017f69c4 00377df6 0037ecff 00000059 0000000d 0000092f 00000000
                 0        1        2        3     SRCS      N-3     TTTTTTTTTTTTT ....
00000020: 00000000 0311aec4 01710728 0171f440 00000059 0000000c 00000dc4 00000000 00000000 04a3f3c4 03066604 03076f50 00000059 0000000b 00001259 00000000
00000030: 00000000 064c388a 049bbc2d 049d09cf 00000059 0000000a 000016ee 00000000 00000000 07de7d8a 062fe388 0630d0e3 00000059 00000009 00001b83 00000000
00000040: 00000000 0970c28a 07c6dab8 07c8087d 00000059 00000008 00002018 00000000 00000000 0b1a137e 095922ec 095a3014 00000059 00000007 000024ad 00000000
00000050: 00000000 0cac587e 0af1b10d 0af2ac47 00000059 00000006 00002942 00000000 00000000 0e3e9d7e 0c867ef4 0c877cec 00000059 00000005 00002dd7 00000000
00000060: 00000000 0fe6e244 0e1a260b 0e1b19e3 00000059 00000004 0000326c 00000000 00000000 11792744 0fb11db4 0fb2210b 00000059 00000003 00003701 00000000
00000070: 00000000 130b6c44 11442dc4 11454055 00000059 00000002 00003b96 00000000 00000000 14b3b10a 12db7d25 12dc7c26 00000059 00000001 0000402b 00000000
00000080: 00000000 1645f60a 1470ee14 1471e9b3 00000059 00000000 000044c0 00000000 00000000 17d83b0a 160458e4 16059a18 0000ffff 0000000f 00004955 00000000
00000090: 00000000 00000000 c0000000 19818bfe 179ce4f6 b2b7fefe 179df2ac 00016825 00000000 33320000 00006c0f 0031205e 17679169 00015a0a b2b2b2b2 ecb401f0
>>>>
** (gxsm4:197927): WARNING **: 00:19:39.678: read_GVP_data_block_to_position_vector: Stream ERROR at Reading offset 00000011, write position 000000c9.
SRCS/index mismatch detected: 0x0000 vs 0x63e7 or missing data/index jump: i 0 -> -1263225676


?????

** (gxsm4:78905): WARNING **: 21:45:21.790: read_GVP_data_block_to_position_vector: Reading offset 000001f8 is beyond stream write position 000001f9. Awaiting data.

000001b8: fbe10a31 fbe10a31 fbe10a31 fbe10a31 0000c058 0000000c 00008814 00000000 14e81219 166733e9 16662f1a efefefef fbe402a7 fbe402a7 fbe402a7 fbe402a7
000001c8: 0000c058 0000000b 00008ca9 00000000 1355cd19 14cfd8d6 14cec440 efefefef fbdff3d4 fbdff3d4 fbdff3d4 fbdff3d4 0000c058 0000000a 0000913e 00000000
000001d8: 11c38819 133af03d 1339ef9f efefefef fbe605da fbe605da fbe605da fbe605da 0000c058 00000009 000095d3 00000000 101a3725 11a62737 11a5304a efefefef
000001e8: fbe807d7 fbe807d7 fbe807d7 fbe807d7 0000c058 00000008 00009a68 00000000 0e87f225 1010dde4 100fe757 efefefef fbcdf3f8 fbcdf3f8 fbcdf3f8 fbcdf3f8
**>>      ********
000001f8: 0000c058 eeeeeeee eeeeeeee eeeeeeee eeeeeeee eeeeeeee eeeeeeee eeeeeeee eeeeeeee eeeeeeee eeeeeeee eeeeeeee eeeeeeee eeeeeeee eeeeeeee eeeeeeee
00000208: eeeeeeee eeeeeeee eeeeeeee eeeeeeee eeeeeeee eeeeeeee eeeeeeee eeeeeeee eeeeeeee eeeeeeee eeeeeeee eeeeeeee eeeeeeee eeeeeeee eeeeeeee eeeeeeee
00000218: eeeeeeee eeeeeeee eeeeeeee eeeeeeee eeeeeeee eeeeeeee eeeeeeee eeeeeeee eeeeeeee eeeeeeee eeeeeeee eeeeeeee eeeeeeee eeeeeeee eeeeeeee eeeeeeee
00000228: eeeeeeee eeeeeeee eeeeeeee eeeeeeee eeeeeeee eeeeeeee eeeeeeee eeeeeeee eeeeeeee eeeeeeee eeeeeeee eeeeeeee eeeeeeee eeeeeeee eeeeeeee eeeeeeee
00000238: eeeeeeee eeeeeeee eeeeeeee eeeeeeee eeeeeeee eeeeeeee eeeeeeee eeeeeeee eeeeeeee eeeeeeee eeeeeeee eeeeeeee eeeeeeee eeeeeeee eeeeeeee eeeeeeee
00000248: eeeeeeee eeeeeeee eeeeeeee eeeeeeee eeeeeeee eeeeeeee eeeeeeee eeeeeeee eeeeeeee eeeeeeee eeeeeeee eeeeeeee eeeeeeee eeeeeeee eeeeeeee eeeeeeee

RP:
...
B[00000600] W[00000180] D 000384: dddddddd dddddddd dddddddd dddddddd dddddddd dddddddd dddddddd dddddddd dddddddd dddddddd dddddddd dddddddd dddddddd dddddddd dddddddd dddddddd
B[00000640] W[00000190] D 000400: dddddddd dddddddd dddddddd dddddddd dddddddd dddddddd dddddddd dddddddd dddddddd dddddddd dddddddd dddddddd dddddddd dddddddd dddddddd dddddddd
B[00000680] W[000001a0] D 000416: dddddddd dddddddd dddddddd dddddddd 0000c058 0000000e 00007eea 00000000 18229bdf 18d5e8e4 18d5f652 efefefef fbcff5ed fbcff5ed fbcff5ed fbcff5ed
B[000006c0] W[000001b0] D 000432: 0000c058 0000000d 0000837f 00000000 169056df 17f2c704 17f1dd62 efefeff0 fbe10a31 fbe10a31 fbe10a31 fbe10a31 0000c058 0000000c 00008814 00000000
B[00000700] W[000001c0] D 000448: 14e81219 166733e9 16662f1a efefefef fbe402a7 fbe402a7 fbe402a7 fbe402a7 0000c058 0000000b 00008ca9 00000000 1355cd19 14cfd8d6 14cec440 efefefef
B[00000740] W[000001d0] D 000464: fbdff3d4 fbdff3d4 fbdff3d4 fbdff3d4 0000c058 0000000a 0000913e 00000000 11c38819 133af03d 1339ef9f efefefef fbe605da fbe605da fbe605da fbe605da
B[00000780] W[000001e0] D 000480: 0000c058 00000009 000095d3 00000000 101a3725 11a62737 11a5304a efefefef fbe807d7 fbe807d7 fbe807d7 fbe807d7 0000c058 00000008 00009a68 00000000
**>>                              0        1        2        3        4        5        6        7        8******* 9        a        b        c        d       e         f
B[000007c0] W[000001f0] D 000496: 0e87f225 1010dde4 100fe757 efefefef fbcdf3f8 fbcdf3f8 fbcdf3f8 fbcdf3f8 0000c058 00000007 00009efd 00000000 0cf5ad25 0e7bdd26 0e7ad1c3 efefefef
B[00000800] W[00000200] D 000512: fbea0014 fbea0014 fbea0014 fbea0014 0000c058 00000006 0000a392 00000000 0b4d685f 0ce454df 0ce30740 efefefef fbf00b24 fbf00b24 fbf00b24 fbf00b24
B[00000840] W[00000210] D 000528: 0000c058 00000005 0000a827 00000000 09bb235f 0b5077cb 0b4f88b5 efefefef fbd4f79e fbd4f79e fbd4f79e fbd4f79e 0000c058 00000004 0000acbc 00000000
B[00000880] W[00000220] D 000544: 0828de5f 09bace80 09b9b81b efefefef fbdafa89 fbdafa89 fbdafa89 fbdafa89 0000c058 00000003 0000b151 00000000 06809999 0825a43e 082492e2 efefefef
B[000008c0] W[00000230] D 000560: fbdb0c18 fbdb0c18 fbdb0c18 fbdb0c18 0000c058 00000002 0000b5e6 00000000 04ee5499 06908ab1 068f8bfc efefefef fbddfcc5 fbddfcc5 fbddfcc5 fbddfcc5
B[00000900] W[00000240] D 000576: 0000c058 00000001 0000ba7b 00000000 035c0f99 04f961ea 04f8607a efefefef fbeef5bc fbeef5bc fbeef5bc fbeef5bc 0000c058 00000000 0000bf10 00000000
B[00000940] W[00000250] D 000592: 01b2bea5 0365ed97 0364ec38 efefefef fbe509f4 fbe509f4 fbe509f4 fbe509f4 fefefefe fefefefe fefefefe fefefefe 0a02be64 0cc332bc c0000000 002079a5
B[00000980] W[00000260] D 000608: 01491500 ef7da700 01491500 ffff4400 00000000 00000000 00000000 00000000 00000000 00000000 efefefef fbce0275 ffffeeee ffffeeee ffffeeee ffffeeee
B[000009c0] W[00000270] D 000624: ffffeeee ffffeeee ffffeeee ffffeeee ffffeeee ffffeeee ffffeeee ffffeeee ffffeeee ffffeeee ffffeeee ffffeeee ffffeeee ffffeeee ffffeeee ffffeeee
B[00000a00] W[00000280] D 000640: ffffeeee ffffeeee ffffeeee ffffeeee ffffeeee ffffeeee ffffeeee ffffeeee ffffeeee ffffeeee ffffeeee ffffeeee ffffeeee ffffeeee ffffeeee ffffeeee
B[00000a40] W[00000290] D 000656: ffffeeee ffffeeee ffffeeee ffffeeee ffffeeee ffffeeee ffffeeee ffffeeee ffffeeee ffffeeee ffffeeee ffffeeee ffffeeee ffffeeee ffffeeee ffffeeee
B[00000a80] W[000002a0] D 000672: ffffeeee ffffeeee ffffeeee ffffeeee ffffeeee ffffeeee ffffeeee ffffeeee ffffeeee ffffeeee ffffeeee ffffeeee 00000010 00000000 00000000 00000000
B[00000ac0] W[000002b0] D 000688: 0000000f 00000000 00000000 00000000 0000000e 00000000 00000000 00000000 0000000d 00000000 00000000 00000000 0000000c 00000000 00000000 00000000
B[00000b00] W[000002c0] D 000704: 0000000b 00000000 00000000 00000000 0000000a 00000000 00000000 00000000 00000009 00000000 00000000 00000000 00000008 00000000 00000000 00000000
B[00000b40] W[000002d0] D 000720: 00000007 00000000 00000000 00000000 00000006 00000000 00000000 00000000 00000005 00000000 00000000 00000000 00000004 00000000 00000000 00000000
B[00000b80] W[000002e0] D 000736: 00000003 00000000 00000000 00000000 00000002 00000000 00000000 00000000 00000001 00000000 00000000 00000000 dddddddd dddddddd dddddddd dddddddd
B[00000bc0] W[000002f0] D 000752: dddddddd dddddddd dddddddd dddddddd dddddddd dddddddd dddddddd dddddddd dddddddd dddddddd dddddddd dddddddd dddddddd dddddddd dddddddd dddddddd
B[00000c00] W[00000300] D 000768: dddddddd dddddddd dddddddd dddddddd dddddddd dddddddd dddddddd dddddddd dddddddd dddddddd dddddddd dddddddd dddddddd dddddddd dddddddd dddddddd
B[00000c40] W[00000310] D 000784: dddddddd dddddddd dddddddd dddddddd dddddddd dddddddd dddddddd dddddddd dddddddd dddddddd dddddddd dddddddd dddddddd dddddddd dddddddd dddddddd


LOOP ISSUE:

0003fec0: 00000114 00000162 0e6a9ba5 00000000 c0000000 0009e309 00000000 00000000 00000114 00000161 0e6acc7d 00000000 c0000000 000a010c 00000000 00000000
0003fed0: 00000114 00000160 0e6afd55 00000000 c0000000 0009f514 00000000 00000000 00000114 0000015f 0e6b2e2d 00000000 c0000000 0009e9ee 00000000 00000000
0003fee0: 00000114 0000015e 0e6b5f05 00000000 c0000000 0009e4e4 00000000 00000000 00000114 0000015d 0e6b8fdd 00000000 c0000000 0009ee41 00000000 00000000
0003fef0: 00000114 0000015c 0e6bc0b5 00000000 c0000000 0009eab1 00000000 00000000 00000114 0000015b 0e6bf18d 00000000 c0000000 0009e1e4 00000000 00000000
0003ff00: 00000114 0000015a 0e6c2265 00000000 c0000000 0009e1e0 00000000 00000000 00000114 00000159 0e6c533d 00000000 c0000000 0009e91f 00000000 00000000
0003ff10: 00000114 00000158 0e6c8415 00000000 c0000000 0009ea01 00000000 00000000 00000114 00000157 0e6cb4ed 00000000 c0000000 0009c8f3 00000000 00000000
0003ff20: 00000114 00000156 0e6ce5c5 00000000 c0000000 0009e263 00000000 00000000 00000114 00000155 0e6d169d 00000000 c0000000 0009d0c3 00000000 00000000
0003ff30: 00000114 00000154 0e6d4775 00000000 c0000000 0009ec0f 00000000 00000000 00000114 00000153 0e6d784d 00000000 c0000000 0009d734 00000000 00000000
0003ff40: 00000114 00000152 0e6da925 00000000 c0000000 0009d7d6 00000000 00000000 00000114 00000151 0e6dd9fd 00000000 c0000000 0009e9f2 00000000 00000000
0003ff50: 00000114 00000150 0e6e0ad5 00000000 c0000000 0009e6bf 00000000 00000000 00000114 0000014f 0e6e3bad 00000000 c0000000 000a0be8 00000000 00000000
0003ff60: 00000114 0000014e 0e6e6c85 00000000 c0000000 0009e4ab 00000000 00000000 00000114 0000014d 0e6e9d5d 00000000 c0000000 0009ec43 00000000 00000000
0003ff70: 00000114 0000014c 0e6ece35 00000000 c0000000 0009d954 00000000 00000000 00000114 0000014b 0e6eff0d 00000000 c0000000 0009ee95 00000000 00000000
0003ff80: 00000114 0000014a 0e6f2fe5 00000000 c0000000 0009f3d0 00000000 00000000 00000114 00000149 0e6f60bd 00000000 c0000000 0009ed05 00000000 00000000
0003ff90: 00000114 00000148 0e6f9195 00000000 c0000000 000a0010 00000000 00000000 00000114 00000147 0e6fc26d 00000000 c0000000 0009f50e 00000000 00000000
0003ffa0: 00000114 00000146 0e6ff345 00000000 c0000000 0009e86c 00000000 00000000 00000114 00000145 0e70241d 00000000 c0000000 0009f3ef 00000000 00000000
0003ffb0: 00000114 00000144 0e7054f5 00000000 c0000000 0009f269 00000000 00000000 00000114 00000143 0e7085cd 00000000 c0000000 0009ecd3 00000000 00000000
0003ffc0: 00000114 00000142 0e70b6a5 00000000 c0000000 0009e0a6 00000000 00000000 00000114 00000141 0e70e77d 00000000 c0000000 0009e812 00000000 00000000
0003ffd0: 00000114 00000140 0e711855 00000000 c0000000 0009ef5a 00000000 00000000 00000114 0000013f 0e71492d 00000000 c0000000 0009efe9 00000000 00000000
0003ffe0: 00000114 0000013e 0e717a05 00000000 c0000000 0009ebfe 00000000 00000000 00000114 0000013d 0e71aadd 00000000 c0000000 0009f175 00000000 00000000
0003fff0: 00000114 0000013c 0e71dbb5 00000000 c0000000 0009e284 00000000 00000000 00000114 0000013b 0e720c8d 00000000 c0000000 0009ec6b 00000000 00000000
00040000: 0000013a 0e723d65 00000000 c0000000 0009e1d8 00000000 00000000 00000114 00000139 0e726e3d 00000000 c0000000 0009e021 00000000 00000000 00000114
00040010: 00000138 0e729f15 00000000 c0000000 0009edfb 00000000 00000000 00000114 00000137 0e72cfed 00000000 c0000000 0009f438 00000000 00000000 00000114
00040020: 00000136 0e7300c5 00000000 c0000000 0009d5b9 00000000 00000000 00000114 00000135 0e73319d 00000000 c0000000 0009fa85 00000000 00000000 00000114
00040030: 00000134 0e736275 00000000 c0000000 0009e73f 00000000 00000000 00000114 00000133 0e73934d 00000000 c0000000 0009e2c0 00000000 00000000 00000114
00040040: 00000132 0e73c425 00000000 c0000000 0009e17e 00000000 00000000 00000114 00000131 0e73f4fd 00000000 c0000000 0009f6af 00000000 00000000 00000114
00040050: 00000130 0e7425d5 00000000 c0000000 0009e616 00000000 00000000 00000114 0000012f 0e7456ad 00000000 c0000000 000a0434 00000000 00000000 00000114
00040060: 0000012e 0e748785 00000000 c0000000 0009e7b3 00000000 00000000 00000114 0000012d 0e74b85d 00000000 c0000000 0009f106 00000000 00000000 00000114
00040070: 0000012c 0e74e935 00000000 c0000000 0009f6e3 00000000 00000000 00000114 0000012b 0e751a0d 00000000 c0000000 0009f304 00000000 00000000 00000114
00040080: 0000012a 0e754ae5 00000000 c0000000 0009d36b 00000000 00000000 00000114 00000129 0e757bbd 00000000 c0000000 0009f18a 00000000 00000000 00000114
00040090: 00000128 0e75ac95 00000000 c0000000 0009e475 00000000 00000000 00000114 00000127 0e75dd6d 00000000 c0000000 0009da2d 00000000 00000000 00000114
000400a0: 00000126 0e760e45 00000000 c0000000 0009ea5a 00000000 00000000 00000114 00000125 0e763f1d 00000000 c0000000 0009da34 00000000 00000000 00000114
000400b0: 00000124 0e766ff5 00000000 c0000000 0009e6e7 00000000 00000000 00000114 00000123 0e76a0cd 00000000 c0000000 0009ed2f 00000000 00000000 00000114
000400c0: 00000122 0e76d1a5 00000000 c0000000 0009de17 00000000 00000000 00000114 00000121 0e77027d 00000000 c0000000 0009de0b 00000000 00000000 00000114
000400d0: 00000120 0e773355 00000000 c0000000 0009fda3 00000000 00000000 00000114 0000011f 0e77642d 00000000 c0000000 0009e1cc 00000000 00000000 00000114
000400e0: 0000011e 0e779505 00000000 c0000000 0009e705 00000000 00000000 00000114 0000011d 0e77c5dd 00000000 c0000000 0009e4a6 00000000 00000000 00000114
000400f0: 0000011c 0e77f6b5 00000000 c0000000 0009d7fe 00000000 00000000 00000114 0000011b 0e78278d 00000000 c0000000 0009e87c 00000000 00000000 00000114
00040100: 0000011a 0e785865 00000000 c0000000 0009e699 00000000 00000000 00000114 00000119 0e78893d 00000000 c0000000 0009d83d 00000000 00000000 00000114
00040110: 00000118 0e78ba15 00000000 c0000000 0009ebd4 00000000 00000000 00000114 00000117 0e78eaed 00000000 c0000000 0009d2d1 00000000 00000000 00000114
00040120: 00000116 0e791bc5 00000000 c0000000 0009df60 00000000 00000000 00000114 00000115 0e794c9d 00000000 c0000000 0009e643 00000000 00000000 00000114
00040130: 00000114 0e797d75 00000000 c0000000 0009dcda 00000000 00000000 00000114 00000113 0e79ae4d 00000000 c0000000 0009e30d 00000000 00000000 00000114



NO FIX DMA128 LOOPING -- missing 0x00040000 @ 0x40000 -- TEST DATA := ADDR (except HDRs)

0003fec0: 3ec000ff 000009b1 06b12fe9 19e80000 0003fec4 0003fec5 0003fec6 0003fec7 0003fec8 0003fec9 0003feca 0003fecb 3ecc00ff 000009b0 06b14403 19020000
0003fed0: 0003fed0 0003fed1 0003fed2 0003fed3 0003fed4 0003fed5 0003fed6 0003fed7 3ed800ff 000009af 06b1581d 191c0000 0003fedc 0003fedd 0003fede 0003fedf
0003fee0: 0003fee0 0003fee1 0003fee2 0003fee3 3ee400ff 000009ae 06b16c37 19360000 0003fee8 0003fee9 0003feea 0003feeb 0003feec 0003feed 0003feee 0003feef
0003fef0: 3ef000ff 000009ad 06b18051 19500000 0003fef4 0003fef5 0003fef6 0003fef7 0003fef8 0003fef9 0003fefa 0003fefb 3efc00ff 000009ac 06b1946b 196a0000
0003ff00: 0003ff00 0003ff01 0003ff02 0003ff03 0003ff04 0003ff05 0003ff06 0003ff07 3f0800ff 000009ab 06b1a885 19840000 0003ff0c 0003ff0d 0003ff0e 0003ff0f
0003ff10: 0003ff10 0003ff11 0003ff12 0003ff13 3f1400ff 000009aa 06b1bc9f 199e0000 0003ff18 0003ff19 0003ff1a 0003ff1b 0003ff1c 0003ff1d 0003ff1e 0003ff1f
0003ff20: 3f2000ff 000009a9 06b1d0b9 19b80000 0003ff24 0003ff25 0003ff26 0003ff27 0003ff28 0003ff29 0003ff2a 0003ff2b 3f2c00ff 000009a8 06b1e4d3 19d20000
0003ff30: 0003ff30 0003ff31 0003ff32 0003ff33 0003ff34 0003ff35 0003ff36 0003ff37 3f3800ff 000009a7 06b1f8ed 19ec0000 0003ff3c 0003ff3d 0003ff3e 0003ff3f
0003ff40: 0003ff40 0003ff41 0003ff42 0003ff43 3f4400ff 000009a6 06b20d07 19060000 0003ff48 0003ff49 0003ff4a 0003ff4b 0003ff4c 0003ff4d 0003ff4e 0003ff4f
0003ff50: 3f5000ff 000009a5 06b22121 19200000 0003ff54 0003ff55 0003ff56 0003ff57 0003ff58 0003ff59 0003ff5a 0003ff5b 3f5c00ff 000009a4 06b2353b 193a0000
0003ff60: 0003ff60 0003ff61 0003ff62 0003ff63 0003ff64 0003ff65 0003ff66 0003ff67 3f6800ff 000009a3 06b24955 19540000 0003ff6c 0003ff6d 0003ff6e 0003ff6f
0003ff70: 0003ff70 0003ff71 0003ff72 0003ff73 3f7400ff 000009a2 06b25d6f 196e0000 0003ff78 0003ff79 0003ff7a 0003ff7b 0003ff7c 0003ff7d 0003ff7e 0003ff7f
0003ff80: 3f8000ff 000009a1 06b27189 19880000 0003ff84 0003ff85 0003ff86 0003ff87 0003ff88 0003ff89 0003ff8a 0003ff8b 3f8c00ff 000009a0 06b285a3 19a20000
0003ff90: 0003ff90 0003ff91 0003ff92 0003ff93 0003ff94 0003ff95 0003ff96 0003ff97 3f9800ff 0000099f 06b299bd 19bc0000 0003ff9c 0003ff9d 0003ff9e 0003ff9f
0003ffa0: 0003ffa0 0003ffa1 0003ffa2 0003ffa3 3fa400ff 0000099e 06b2add7 19d60000 0003ffa8 0003ffa9 0003ffaa 0003ffab 0003ffac 0003ffad 0003ffae 0003ffaf
0003ffb0: 3fb000ff 0000099d 06b2c1f1 19f00000 0003ffb4 0003ffb5 0003ffb6 0003ffb7 0003ffb8 0003ffb9 0003ffba 0003ffbb 3fbc00ff 0000099c 06b2d60b 190a0000
0003ffc0: 0003ffc0 0003ffc1 0003ffc2 0003ffc3 0003ffc4 0003ffc5 0003ffc6 0003ffc7 3fc800ff 0000099b 06b2ea25 19240000 0003ffcc 0003ffcd 0003ffce 0003ffcf
0003ffd0: 0003ffd0 0003ffd1 0003ffd2 0003ffd3 3fd400ff 0000099a 06b2fe3f 193e0000 0003ffd8 0003ffd9 0003ffda 0003ffdb 0003ffdc 0003ffdd 0003ffde 0003ffdf
0003ffe0: 3fe000ff 00000999 06b31259 19580000 0003ffe4 0003ffe5 0003ffe6 0003ffe7 0003ffe8 0003ffe9 0003ffea 0003ffeb 3fec00ff 00000998 06b32673 19720000
0003fff0: 0003fff0 0003fff1 0003fff2 0003fff3 0003fff4 0003fff5 0003fff6 0003fff7 3ff800ff 00000997 06b33a8d 198c0000 0003fffc 0003fffd 0003fffe 0003ffff
00040000: 00040001 00040002 00040003 000400ff 00000996 06b34ea7 19a60000 00040008 00040009 0004000a 0004000b 0004000c 0004000d 0004000e 0004000f 001000ff
00040010: 00000995 06b362c1 19c00000 00040014 00040015 00040016 00040017 00040018 00040019 0004001a 0004001b 001c00ff 00000994 06b376db 19da0000 00040020
00040020: 00040021 00040022 00040023 00040024 00040025 00040026 00040027 002800ff 00000993 06b38af5 19f40000 0004002c 0004002d 0004002e 0004002f 00040030
00040030: 00040031 00040032 00040033 003400ff 00000992 06b39f0f 190e0000 00040038 00040039 0004003a 0004003b 0004003c 0004003d 0004003e 0004003f 004000ff
00040040: 00000991 06b3b329 19280000 00040044 00040045 00040046 00040047 00040048 00040049 0004004a 0004004b 004c00ff 00000990 06b3c743 19420000 00040050
00040050: 00040051 00040052 00040053 00040054 00040055 00040056 00040057 005800ff 0000098f 06b3db5d 195c0000 0004005c 0004005d 0004005e 0004005f 00040060
00040060: 00040061 00040062 00040063 006400ff 0000098e 06b3ef77 19760000 00040068 00040069 0004006a 0004006b 0004006c 0004006d 0004006e 0004006f 007000ff
00040070: 0000098d 06b40391 19900000 00040074 00040075 00040076 00040077 00040078 00040079 0004007a 0004007b 007c00ff 0000098c 06b417ab 19aa0000 00040080
00040080: 00040081 00040082 00040083 00040084 00040085 00040086 00040087 008800ff 0000098b 06b42bc5 19c40000 0004008c 0004008d 0004008e 0004008f 00040090
00040090: 00040091 00040092 00040093 009400ff 0000098a 06b43fdf 19de0000 00040098 00040099 0004009a 0004009b 0004009c 0004009d 0004009e 0004009f 00a000ff
000400a0: 00000989 06b453f9 19f80000 000400a4 000400a5 000400a6 000400a7 000400a8 000400a9 000400aa 000400ab 00ac00ff 00000988 06b46813 19120000 000400b0
000400b0: 000400b1 000400b2 000400b3 000400b4 000400b5 000400b6 000400b7 00b800ff 00000987 06b47c2d 192c0000 000400bc 000400bd 000400be 000400bf 000400c0
000400c0: 000400c1 000400c2 000400c3 00c400ff 00000986 06b49047 19460000 000400c8 000400c9 000400ca 000400cb 000400cc 000400cd 000400ce 000400cf 00d000ff
000400d0: 00000985 06b4a461 19600000 000400d4 000400d5 000400d6 000400d7 000400d8 000400d9 000400da 000400db 00dc00ff 00000984 06b4b87b 197a0000 000400e0
000400e0: 000400e1 000400e2 000400e3 000400e4 000400e5 000400e6 000400e7 00e800ff 00000983 06b4cc95 19940000 000400ec 000400ed 000400ee 000400ef 000400f0
000400f0: 000400f1 000400f2 000400f3 00f400ff 00000982 06b4e0af 19ae0000 000400f8 000400f9 000400fa 000400fb 000400fc 000400fd 000400fe 000400ff 010000ff
00040100: 00000981 06b4f4c9 19c80000 00040104 00040105 00040106 00040107 00040108 00040109 0004010a 0004010b 010c00ff 00000980 06b508e3 19e20000 00040110
00040110: 00040111 00040112 00040113 00040114 00040115 00040116 00040117 011800ff 0000097f 06b51cfd 19fc0000 0004011c 0004011d 0004011e 0004011f 00040120
00040120: 00040121 00040122 00040123 012400ff 0000097e 06b53117 19160000 00040128 00040129 0004012a 0004012b 0004012c 0004012d 0004012e 0004012f 013000ff
00040130: 0000097d 06b54531 19300000 00040134 00040135 00040136 00040137 00040138 00040139 0004013a 0004013b 013c00ff 0000097c 06b5594b 194a0000 00040140

TEST COUNTS WITH ADDR_NEXT from 3FFF0:

0003ff50: fffef498 9b9b9c9c e70307ba e70307ba 3f54c0f8 00004ae2 0881cf31 00000000 1890431f 1858ce65 9b87460f 1858ce29 fffeeb1f 9b9b9b9b e6def33f e6def33f
0003ff60: 3f60c0f8 00004ae1 0881e79d 00000000 188fef3d 18588255 9b23dd71 1858819d fffed4c1 9b9b9b9b e6c507f0 e6c507f0 3f6cc0f8 00004ae0 08820009 00000000
0003ff70: 188f9b5b 18581daa 9ab5cdc8 18581f76 fffed4f0 9a9a9a9a e6ac01a6 e6ac01a6 3f78c0f8 00004adf 08821875 00000000 188f4779 1857ecdb 9a3c775c 1857ee1a
0003ff80: fffedddd 9a9a999a e67af5aa e67af5aa 3f84c0f8 00004ade 088230e1 00000000 188ef397 185794fc 99e0fc38 185795f1 fffea3e9 99999999 e67f0b47 e67f0b47
0003ff90: 3f90c0f8 00004add 0882494d 00000000 188e9fb5 18571bb1 996e149d 18571a55 fffec6a7 99999999 e65afba6 e65afba6 3f9cc0f8 00004adc 088261b9 00000000
0003ffa0: 188e4bd3 1856d461 99003d24 1856d331 fffeab4a 98989899 e63df99a e63df99a 3fa8c0f8 00004adb 08827a25 00000000 188df7f1 18569c33 98ab3df6 18569afb
0003ffb0: fffe9c60 98989898 e6220c2c e6220c2c 3fb4c0f8 00004ada 08829291 00000000 188da40f 18565c33 9839bb13 18565c89 fffe8aa6 98989898 e603f655 e603f655
0003ffc0: 3fc0c0f8 00004ad9 0882aafd 00000000 188d502d 1855eddb 97dcd6fb 1855edf8 fffe8bd6 97979797 e5f6002b e5f6002b 3fccc0f8 00004ad8 0882c369 00000000
0003ffd0: 188cfc4b 1855a284 9787bb8d 1855a39b fffe8b0e 97979797 e5e808e5 e5e808e5 3fd8c0f8 00004ad7 0882dbd5 00000000 188ca869 18553ce7 9718d61d 18553a8e
0003ffe0: fffe796b 97969797 e5bff3d6 e5bff3d6 3fe4c0f8 00004ad6 0882f441 00000000 188c5487 1854e426 96c9f9a1 1854e5be fffe8b73 96969696 e5ab05f5 e5ab05f5
0003fff0: 0003fff4 0003fff5 0003fff6 0003fff7 0003fff8 0003fff9 0003fffa 0003fffb 0003fffc 0003fffd 0003fffe 0003ffff 00040000 00040001 00040002 00040003
00040000: 00040005 00040006 00040007 00040008 00040009 0004000a 0004000b 0004000c 0004000d 0004000e 0004000f 00040010 00040011 00040012 00040013 00040014
00040010: 00040015 00040016 00040017 00040018 00040019 0004001a 0004001b 0004001c 0004001d 0004001e 0004001f 00040020 00040021 00040022 00040023 00040024
00040020: 00040025 00040026 00040027 00040028 00040029 0004002a 0004002b 0004002c 0004002d 0004002e 0004002f 00040030 00040031 00040032 00040033 00040034
00040030: 00040035 00040036 00040037 00040038 00040039 0004003a 0004003b 0004003c 0004003d 0004003e 0004003f 00040040 00040041 00040042 00040043 00040044
00040040: 00040045 00040046 00040047 00040048 00040049 0004004a 0004004b 0004004c 0004004d 0004004e 0004004f 00040050 00040051 00040052 00040053 00040054
00040050: 00040055 00040056 00040057 00040058 00040059 0004005a 0004005b 0004005c 0004005d 0004005e 0004005f 00040060 00040061 00040062 00040063 00040064
00040060: 00040065 00040066 00040067 00040068 00040069 0004006a 0004006b 0004006c 0004006d 0004006e 0004006f 00040070 00040071 00040072 00040073 00040074
00040070: 00040075 00040076 00040077 00040078 00040079 0004007a 0004007b 0004007c 0004007d 0004007e 0004007f 00040080 00040081 00040082 00040083 00040084
00040080: 00040085 00040086 00040087 00040088 00040089 0004008a 0004008b 0004008c 0004008d 0004008e 0004008f 00040090 00040091 00040092 00040093 00040094
00040090: 00040095 00040096 00040097 00040098 00040099 0004009a 0004009b 0004009c 0004009d 0004009e 0004009f 000400a0 000400a1 000400a2 000400a3 000400a4
000400a0: 000400a5 000400a6 000400a7 000400a8 000400a9 000400aa 000400ab 000400ac 000400ad 000400ae 000400af 000400b0 000400b1 000400b2 000400b3 000400b4
000400b0: 000400b5 000400b6 000400b7 000400b8 000400b9 000400ba 000400bb 000400bc 000400bd 000400be 000400bf 000400c0 000400c1 000400c2 000400c3 000400c4
000400c0: 000400c5 000400c6 000400c7 000400c8 000400c9 000400ca 000400cb 000400cc 000400cd 000400ce 000400cf 000400d0 000400d1 000400d2 000400d3 000400d4
000400d0: 000400d5 000400d6 000400d7 000400d8 000400d9 000400da 000400db 000400dc 000400dd 000400de 000400df 000400e0 000400e1 000400e2 000400e3 000400e4
000400e0: 000400e5 000400e6 000400e7 000400e8 000400e9 000400ea 000400eb 000400ec 000400ed 000400ee 000400ef 000400f0 000400f1 000400f2 000400f3 000400f4
000400f0: 000400f5 000400f6 000400f7 000400f8 000400f9 000400fa 000400fb 000400fc 000400fd 000400fe 000400ff 00040100 00040101 00040102 00040103 00040104
00040100: 00040105 00040106 00040107 00040108 00040109 0004010a 0004010b 0004010c 0004010d 0004010e 0004010f 00040110 00040111 00040112 00040113 00040114
00040110: 00040115 00040116 00040117 00040118 00040119 0004011a 0004011b 0004011c 0004011d 0004011e 0004011f 00040120 00040121 00040122 00040123 00040124
00040120: 00040125 00040126 00040127 00040128 00040129 0004012a 0004012b 0004012c 0004012d 0004012e 0004012f 00040130 00040131 00040132 00040133 00040134
00040130: 00040135 00040136 00040137 00040138 00040139 0004013a 0004013b 0004013c 0004013d 0004013e 0004013f 00040140 00040141 00040142 00040143 00040144


*/
        
#define RECOVERY_RETRYS 1
int rpspmc_hwi_dev::read_GVP_data_block_to_position_vector (int offset, gboolean expect_full_header){
        static int hdr_i=0;
        static int hdr_hist[16];
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
                
        if (offset+3 >= GVP_stream_buffer_position){ // Buffer is huge now all pages concat

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
                
        // srcs_bit_mask = GVP_stream_buffer[0+offset]; // reconstructed bit mask for CH00..CH15 as of Channel Indices Selections of actual data been send (may be more as of 128bit DMA chuncs on FPGA)
        // sample_index  = GVP_stream_buffer[1+offset]; // current sample index, down counting from N-1 to 0
        // timecode31_0  = GVP_stream_buffer[2+offset]; // FPGA 48 bit timecode [31: 0]
        // timecode48_32 = GVP_stream_buffer[3+offset]; // FPGA 48 bit timecode [47:32]
        // ... channel data in 32bit words
#define GVP_STREAM_HDR_SRCS_MASK       0 // 0x0000MMMM
#define GVP_STREAM_HDR_SAMPLE_INDEX    1 // 0xNNNNNNNN
#define GVP_STREAM_HDR_TIME_CODE_31_0  2 // 0xTTTTTTTT
#define GVP_STREAM_HDR_TIME_CODE_47_32 3 // 0x0000TTTT
#define GVP_STREAM_HDR_FSS_BLOCK_16    3 // 0xFSSBXXXX
#define GVP_STREAM_DATA_CH0            4 // 0xVVVVVVVV

        GVP_vp_header_current.srcs      = GVP_stream_buffer[offset+GVP_STREAM_HDR_SRCS_MASK] & 0x0000ffff;
        GVP_vp_header_current.i         = GVP_stream_buffer[offset+GVP_STREAM_HDR_SAMPLE_INDEX];           // data point index within GVP section (N points)
        GVP_vp_header_current.gvp_time  = ((((guint64)GVP_stream_buffer[offset+GVP_STREAM_HDR_TIME_CODE_47_32]) & 0x0000ffff)<<32) | (guint64)GVP_stream_buffer[offset+GVP_STREAM_HDR_TIME_CODE_31_0];
        GVP_vp_header_current.dataexpanded[PROBEDATA_ARRAY_MS_TIME-PROBEDATA_ARRAY_S1] = GVP_vp_header_current.gvp_time_ms = (double)GVP_vp_header_current.gvp_time/125e3; // time in ms
        //GVP_vp_header_current.dataexpanded[14] = (double)GVP_vp_header_current.gvp_time/125e3; // time in ms

        unsigned int fss                = (guint32)(GVP_stream_buffer[offset + GVP_STREAM_HDR_FSS_BLOCK_16]) >> 16;

        
	//g_print ("GVP HDR[%08x]: S:%08x i=%d t=%12.6f ms {%08x %08x}\n", offset, GVP_vp_header_current.srcs, GVP_vp_header_current.i, (double)GVP_vp_header_current.gvp_time_ms, GVP_stream_buffer[offset+GVP_STREAM_HDR_TIME_CODE_47_32], GVP_stream_buffer[offset+GVP_STREAM_HDR_TIME_CODE_31_0]);
	
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
                                                int hl[6] = { GVP_vp_header_current.n-1, GVP_vp_header_current.srcs, -1, GVP_vp_header_current.srcs_mask_vector, -1 , -1 };
                                                if (offset > 64)
                                                        status_append_int32 (&GVP_stream_buffer[offset-64], 10*16, true, offset-64, true, hl, offset, hdr_hist);
                                                else
                                                        status_append_int32 (&GVP_stream_buffer[0], 10*16, true, 0, true, hl, offset, hdr_hist);
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

#if 1
                                                int hl[6] = { GVP_vp_header_current.n-1, GVP_vp_header_current.srcs, -1, GVP_vp_header_current.srcs_mask_vector, -1 , -1 };
                                                if (offset > 64)
                                                        status_append_int32 (&GVP_stream_buffer[offset-64], 10*16, true, offset-64, true, hl, offset, hdr_hist);
                                                else
                                                        status_append_int32 (&GVP_stream_buffer[0], 10*16, true, 0, true, hl, offset, hdr_hist);

                                                if (GVP_stream_buffer_position >= 40000){
                                                        status_append_int32 (&GVP_stream_buffer[0], 40*16, true, 0, true, hl, offset, hdr_hist);
                                                        status_append_int32 (&GVP_stream_buffer[0x40000-20*16], 40*16, true, 0x40000-20*16, true, hl, offset, hdr_hist);
                                                }
#endif

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

        // decode SCRS to lookup
        GVP_vp_header_current.number_channels=0;
        for (int i=0; i<16; i++){
                GVP_vp_header_current.ch_lut[i]=-1;
                if (GVP_vp_header_current.srcs & (1<<i))
                        GVP_vp_header_current.ch_lut[GVP_vp_header_current.number_channels++] = i;
        }
                
        // expand data stream and sort into channels
        for (ch_index=0; ch_index < GVP_vp_header_current.number_channels && GVP_STREAM_DATA_CH0 + ch_index + offset < GVP_stream_buffer_position; ++ch_index){
                int ich = GVP_vp_header_current.ch_lut[ch_index];
                GVP_vp_header_current.chNs[ich] = GVP_stream_buffer[GVP_STREAM_DATA_CH0 + ch_index + offset];
                if (ich < 16) // 0..15 are assigned DATA CHANNELS
                        GVP_vp_header_current.dataexpanded[ich] = rpspmc_source_signals[ich+SIGNAL_INDEX_ICH0].scale_factor*GVP_vp_header_current.chNs[ich]; // in units, base units Volts used for XYZ. etc.
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
                
        // check for all channels fetched
        if (GVP_STREAM_DATA_CH0 + ch_index + offset < GVP_stream_buffer_position){

                retry=RECOVERY_RETRYS;
                if (GVP_vp_header_current.srcs == 0xffff){
                        g_mutex_unlock (&GVP_stream_buffer_mutex);
                        return -1; // true for full position header update
                }
                
                GVP_vp_header_current.ilast = GVP_vp_header_current.i; // next point should be this - 1

#ifdef BBB_DBG
                status_append_int32 (&GVP_stream_buffer[hdr], ch_index+offset+GVP_STREAM_DATA_CH0-hdr, true, hdr, true);
                hdr = ch_index+offset+GVP_STREAM_DATA_CH0; // next hdr
#endif

                // decode/expand FSS data
                if (fss){
                        guint64 fss_data = (((guint64)GVP_vp_header_current.chNs[1]) << 32) | GVP_vp_header_current.chNs[0]; // Y,X
                        int     fss_n    = fss & 0xff;
                        int     fss_len  = (fss>>8) & 0xff;
                        GVP_vp_header_current.fss_len = fss_len;
                        guint32 IN12 = (guint32)GVP_vp_header_current.chNs[3];
                        for (int k=0; k<fss_len; ++k){
                                // reconstruct HS time matching FPGA register delay line storage
                                GVP_vp_header_current.fss_data[HS_FSS_TIME   ][k] = GVP_vp_header_current.gvp_time_ms + (-fss_len + k)*8e-6; //  8ns spaced data (125MHz) in ms = x 8e-9ns*1e3ms
                                GVP_vp_header_current.fss_data[HS_FSS_DIGITAL][k] = fss_data & (1LL<<((fss_n+k)&0x3f)) ? 1.:0.;
                                // GVP_vp_header_current.fss_data[1][k] = ((GVP_vp_header_current.chNs[14]>>(8*(k-1)))&0xff) << 6;  // RF
                                // stream_buffer[14] <= { RFdata[3][29:22], RFdata[2][29:22], RFdata[1][29:22], RFdata[0][29:22] }; // RF CH2 upper 8 bits -- ...,most recent
                                // stream_buffer[15] <= { RFdata[7][29:22], RFdata[6][29:22], RFdata[5][29:22], RFdata[4][29:22] }; // RF CH2 upper 8 bits
                                //if (k==0)
                                        //GVP_vp_header_current.fss_data[HS_FSS_RFIN2][k] = (gint16)((((guint32)GVP_vp_header_current.chNs[5]>>(8*(4-1)))&0xff) << 8);  // RF MS 8bits of 14 to 16
                                                //(gint16)((guint32)IN12>>16);  // RF14 dec to 16
                                //else if (k < 5)
                                if (k < 4)
                                        GVP_vp_header_current.fss_data[HS_FSS_RFIN2][k] = (gint16)((((guint32)GVP_vp_header_current.chNs[5]>>(8*(3-k)))&0xff) << 8);  // RF MS 8bits of 14 to 16
                                else if (k < 8)
                                        GVP_vp_header_current.fss_data[HS_FSS_RFIN2][k] = (gint16)((((guint32)GVP_vp_header_current.chNs[4]>>(8*(7-k)))&0xff) << 8);  // RF
                                else if (k < 12)
                                        GVP_vp_header_current.fss_data[HS_FSS_RFIN2][k] = (gint16)((((guint32)GVP_vp_header_current.chNs[15]>>(8*(11-k)))&0xff) << 8);  // RF
                                else if (k < 16)
                                        GVP_vp_header_current.fss_data[HS_FSS_RFIN2][k] = (gint16)((((guint32)GVP_vp_header_current.chNs[14]>>(8*(15-k)))&0xff) << 8);  // RF
                                else    GVP_vp_header_current.fss_data[HS_FSS_RFIN2][k] = GVP_vp_header_current.fss_data[HS_FSS_RFIN2][15]; // hold
                        }
                        GVP_vp_header_current.chNs[3] = (gint16)(IN12&0xffff);
                        GVP_vp_header_current.chNs[4] = (gint16)(IN12>>16);
                        //g_message ("[%08x] *** FSS DATA: %d %d %d %08x %08x", offset, fss_len, fss_n, fss_data, GVP_vp_header_current.chNs[14],GVP_vp_header_current.chNs[15]);
                } else
                        GVP_vp_header_current.fss_len = 0; // no FSS DATA

                g_mutex_unlock (&GVP_stream_buffer_mutex);

                hdr_hist[(hdr_i++)&0xf]=offset;
                
                return ch_index;

        } else {
                // g_message ("[%08x] *** end of new data at ch=%d ** Must wait for next page/update send and retry.", offset, ch_index);
                g_mutex_unlock (&GVP_stream_buffer_mutex);
                return -99;   // OK -- but have wait and reprocess when data package is completed
                // return ch_index; // number channels read until position
        }
};



#else // original DMA32 ---


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
                GVP_vp_header_current.dataexpanded[PROBEDATA_ARRAY_MS_TIME] = GVP_vp_header_current.gvp_time_ms = GVP_vp_header_current.dataexpanded[14] = (double)GVP_vp_header_current.gvp_time/125e3; // time in ms

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
                // stream_buffer[14] <= S_AXIS_gvp_time_tdata[32-1:0]; // TIME lower 32
                // stream_buffer[15] <= { fss_len, fss_n, S_AXIS_gvp_time_tdata[48-1:32] }; // TIME upper
                                                    
                if ((GVP_vp_header_current.srcs & 0xc000) == 0xc000){ // fetch 64 bit time
                        GVP_vp_header_current.gvp_time = (((guint64)((guint32)(GVP_vp_header_current.chNs[15]&0x0000ffff)))<<32) | (guint64)((guint32)GVP_vp_header_current.chNs[14]);
                        GVP_vp_header_current.dataexpanded[14] = (double)GVP_vp_header_current.gvp_time/125e3; // time in ms
                } else if ((GVP_vp_header_current.srcs & 0xc000) == 0x4000){ // fetch 32 bit time only
                        GVP_vp_header_current.gvp_time = (guint64)((guint32)GVP_vp_header_current.chNs[14]);
                        GVP_vp_header_current.dataexpanded[14] = (double)GVP_vp_header_current.gvp_time/125e3;
                } else if  ((GVP_vp_header_current.srcs & 0xc000) == 0x8000){ // fetch upper 32 bit time only -- unusual but possible
                        GVP_vp_header_current.gvp_time = ((guint64)((guint32)(GVP_vp_header_current.chNs[15]&0x0000ffff)))<<32;
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

                int     fss      = (guint32)(GVP_vp_header_current.chNs[15]) >> 16;
                if (fss){
                        guint64 fss_data = (((guint64)GVP_vp_header_current.chNs[1]) << 32) | GVP_vp_header_current.chNs[0]; // Y,X
                        int     fss_n    = fss & 0xff;
                        int     fss_len  = (fss>>8) & 0xff;
                        GVP_vp_header_current.fss_len = fss_len;
                        for (int k=0; k<fss_len; ++k){
                                GVP_vp_header_current.fss_data[0][k] = fss_data & (1LL<<((fss_n+k)&0x3f)) ? 1.:0.;
                                GVP_vp_header_current.fss_data[1][k] = fss_len; // dbg
                        }
                        //g_message ("[%08x] *** FSS DATA: %d %d %d", offset, fss_len, fss_n, fss_data);
                } else
                        GVP_vp_header_current.fss_len = 0; // no FSS DATA

                g_mutex_unlock (&GVP_stream_buffer_mutex);
                return ch_index;

        } else {
                // g_message ("[%08x] *** end of new data at ch=%d ** Must wait for next page/update send and retry.", offset, ch_index);
                g_mutex_unlock (&GVP_stream_buffer_mutex);
                return -99;   // OK -- but have wait and reprocess when data package is completed
                // return ch_index; // number channels read until position
        }
};

#endif // DMA32

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




