/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 8 c-style: "K&R" -*- */

/* Gnome gxsm - Gnome X Scanning Microscopy
 * universal STM/AFM/SARLS/SPALEED/... controlling and
 * data analysis software
 *
 * Gxsm Plugin Name: spm_scancontrol.h
 * ========================================
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

#ifndef __RPSPMC_PACPLL_H
#define __RPSPMC_PACPLL_H

#include <config.h>
#include "plugin.h"

#include "unit.h"
#include "pcs.h"
#include "xsmtypes.h"
#include "glbvars.h"
#include "action_id.h"

#include "core-source/app_profile.h"
#include "rpspmc_hwi_structs.h"

#include "json_talk.h"

#include "rpspmc_stream.h"
//#include "rpspmc_hwi_dev.h"


#define MAX_PROGRAM_VECTORS 32
#define i_X 0
#define i_Y 1
#define i_Z 2

#define DMA_SIZE         0x40000            // 20bit count of 32bit words ==> 1MB DMA Block:  2 x 0x80000 bytes
#define EXPAND_MULTIPLES 32



#define CPN(N) ((double)(1LL<<(N))-1.)

// WARNING WARNING WARNING.. not working life if table is initialized with this
#define BiasFac    (main_get_gapp()->xsm->Inst->BiasGainV2V ())
#define CurrFac    (1./main_get_gapp()->xsm->Inst->nAmpere2V (1.))
#define ZAngFac    (main_get_gapp()->xsm->Inst->Volt2ZA (1.))
#define XAngFac    (main_get_gapp()->xsm->Inst->Volt2XA (1.))
#define YAngFac    (main_get_gapp()->xsm->Inst->Volt2YA (1.))

#define CPN(N) ((double)(1LL<<(N))-1.)

#define RP_FPGA_QEXEC 31 // Q EXEC READING Controller        -- 1V/(2^RP_FPGA_QEXEC-1)
#define RP_FPGA_QSQRT 23 // Q CORDIC SQRT Amplitude Reading  -- 1V/(2^RP_FPGA_QSQRT-1)
#define RP_FPGA_QATAN 21 // Q CORDIC ATAN Phase Reading      -- 180deg/(PI*(2^RP_FPGA_QATAN-1))
#define RP_FPGA_QFREQ 44 // Q DIFF FREQ READING              -- 125MHz/(2^RP_FPGA_QFREQ-1) well number should not exceed 32bit 

#define DSP32Qs15dot16TO_Volt (50/(32767.*(1<<16)))

#define SPMC_AD5791_REFV 5.0 // DAC AD5791 Reference Volatge is 5.000000V (+/-5V Range)
#define SPMC_AD5791_to_volts (SPMC_AD5791_REFV / QN(31))
#define SPMC_RPIN12_REFV 1.0 // RP RF DACs Reference Voltage is 1.0V (+/-1V Range)
#define SPMC_RPIN12_to_volts (SPMC_RPIN12_REFV / QN(31))
#define SPMC_RPIN34_REFV 5.0 // RP AD463-24 DACs Reference Voltage is 5.0V (Differential +/-5V Range)
#define SPMC_RPIN34_to_volts (SPMC_RPIN34_REFV / QN(31))




/* RP-PACPLL and RP-SPMC communication -> -dev */

// Scan Control Class based on AppBase
// -> AppBase provides a GtkWindow and some window handling basics used by Gxsm
class RPspmc_pacpll : public AppBase, public RP_JSON_talk{
public:

        RPspmc_pacpll(Gxsm4app *app); // create window and setup it contents, connect buttons, register cb's...
	virtual ~RPspmc_pacpll(); // unregister cb's
        int setup_scan (int ch, 
                        const gchar *titleprefix, 
                        const gchar *name,
                        const gchar *unit,
                        const gchar *label,
                        double d2u
                        );

        static void connect_cb (GtkWidget *widget, RPspmc_pacpll *self);

        static void enable_scope (GtkWidget *widget, RPspmc_pacpll *self);
        static void dbg_l1 (GtkWidget *widget, RPspmc_pacpll *self);
        static void dbg_l2 (GtkWidget *widget, RPspmc_pacpll *self);
        static void dbg_l4 (GtkWidget *widget, RPspmc_pacpll *self);

        void set_stream_mux(int *mux_source);
           
        static void scan_gvp_opt6 (GtkWidget *widget, RPspmc_pacpll *self);
        static void scan_gvp_opt7 (GtkWidget *widget, RPspmc_pacpll *self);
        
	static void scan_start_callback (gpointer user_data);
	static void scan_stop_callback (gpointer user_data);

	static void pac_tau_transport_changed (Param_Control* pcs, gpointer user_data);
	static void pac_tau_parameter_changed (Param_Control* pcs, gpointer user_data);
	static void pac_frequency_parameter_changed (Param_Control* pcs, gpointer user_data);
	static void pac_volume_parameter_changed (Param_Control* pcs, gpointer user_data);
        static void select_pac_lck_amplitude (GtkWidget *widget, RPspmc_pacpll *self);
        static void select_pac_lck_phase (GtkWidget *widget, RPspmc_pacpll *self);
        static void show_dF_control (GtkWidget *widget, RPspmc_pacpll *self);
        static void show_pulse_control (GtkWidget *widget, RPspmc_pacpll *self);
        static void qcontrol (GtkWidget *widget, RPspmc_pacpll *self);
	static void qc_parameter_changed (Param_Control* pcs, gpointer user_data);
	static void tune_parameter_changed (Param_Control* pcs, gpointer user_data);
	static void amp_ctrl_parameter_changed (Param_Control* pcs, gpointer user_data);
	static void phase_ctrl_parameter_changed (Param_Control* pcs, gpointer user_data);
	static void amplitude_gain_changed (Param_Control* pcs, gpointer user_data);
        static void amplitude_controller_invert (GtkWidget *widget, RPspmc_pacpll *self);
        static void amplitude_controller (GtkWidget *widget, RPspmc_pacpll *self);
	static void phase_gain_changed (Param_Control* pcs, gpointer user_data);
        static void phase_controller_invert (GtkWidget *widget, RPspmc_pacpll *self);
        static void phase_controller (GtkWidget *widget, RPspmc_pacpll *self);
        static void phase_unwrapping_always (GtkWidget *widget, RPspmc_pacpll *self);
        static void phase_rot_ab (GtkWidget *widget, RPspmc_pacpll *self);
        static void phase_unwrap_plot (GtkWidget *widget, RPspmc_pacpll *self);
        static void set_ss_auto_trigger (GtkWidget *widget, RPspmc_pacpll *self);
	static void dfreq_ctrl_parameter_changed (Param_Control* pcs, gpointer user_data);
	static void dfreq_gain_changed (Param_Control* pcs, gpointer user_data);
        static void dfreq_controller_invert (GtkWidget *widget, RPspmc_pacpll *self);
        static void dfreq_controller (GtkWidget *widget, RPspmc_pacpll *self);
        static void EnZdfreq_control (GtkWidget *widget, RPspmc_pacpll *self);
        static void EnUdfreq_control (GtkWidget *widget, RPspmc_pacpll *self);

	static void pulse_form_parameter_changed (Param_Control* pcs, gpointer user_data);
        static void pulse_form_enable (GtkWidget *widget, RPspmc_pacpll *self);
        static void pulse_form_fire (GtkWidget *widget, RPspmc_pacpll *self);
        static void pulse_form_pf_ts (GtkWidget *widget, RPspmc_pacpll *self);

        static void choice_operation_callback (GtkWidget *widget, RPspmc_pacpll *self);
        static void choice_transport_ch12_callback (GtkWidget *widget, RPspmc_pacpll *self);
        static void choice_transport_ch3_callback (GtkWidget *widget, RPspmc_pacpll *self);
        static void choice_transport_ch4_callback (GtkWidget *widget, RPspmc_pacpll *self);
        static void choice_transport_ch5_callback (GtkWidget *widget, RPspmc_pacpll *self);
        static void scope_ac_ch1_callback (GtkWidget *widget, RPspmc_pacpll *self);
        static void scope_ac_ch2_callback (GtkWidget *widget, RPspmc_pacpll *self);
        static void scope_ac_ch3_callback (GtkWidget *widget, RPspmc_pacpll *self);
        static void scope_xy_on_callback (GtkWidget *widget, RPspmc_pacpll *self);
        static void scope_fft_on_callback (GtkWidget *widget, RPspmc_pacpll *self);
        static void scope_fft_time_zoom_callback (GtkWidget *widget, RPspmc_pacpll *self);

        static void scope_z_ch1_callback (GtkWidget *widget, RPspmc_pacpll *self);
        static void scope_z_ch2_callback (GtkWidget *widget, RPspmc_pacpll *self);

        static void scope_buffer_position_callback (GtkWidget *widget, RPspmc_pacpll *self);
        static void scope_save_data_callback (GtkWidget *widget, RPspmc_pacpll *self);

        static void copy_f0_callback (GtkWidget *widget, RPspmc_pacpll *self);

        
        static void choice_update_ts_callback (GtkWidget *widget, RPspmc_pacpll *self);
        static void choice_trigger_mode_callback (GtkWidget *widget, RPspmc_pacpll *self);
        static void choice_auto_set_callback (GtkWidget *widget, RPspmc_pacpll *self);

	void send_all_parameters ();
	void update_SPMC_parameters ();
	void SPMC_cold_start_init ();

        void save_values (NcFile &ncf);
        
	void update (); // window update (inputs, etc. -- here currently not really necessary)
        void update_monitoring_parameters ();
        void save_scope_data ();

        static void graph_draw_function (GtkDrawingArea *area, cairo_t *cr,
                                         int             width,
                                         int             height,
                                         RPspmc_pacpll *self);
        void dynamic_graph_draw_function (GtkDrawingArea *area, cairo_t *cr,
                                          int             width,
                                          int             height);
        void stream_data ();


        virtual const gchar *get_rp_address (){
                return gtk_entry_buffer_get_text (GTK_ENTRY_BUFFER (gtk_entry_get_buffer (GTK_ENTRY (input_rpaddress))));
        };

        virtual void status_append (const gchar *msg);
        virtual void update_health (const gchar *msg=NULL);
        virtual void on_connect_actions(); // called after connection is made -- init, setup all, ...

        virtual int get_debug_level() { return debug_level; };
        
        virtual void on_new_data ();

        gboolean update_shm_monitors (int close=0);
        void memcpy_to_rt_monitors (double *dvec, gsize count, int start_pos) {
                if (rt_monitors_shm_ptr)
                        memcpy (rt_monitors_shm_ptr+start_pos*sizeof(double), dvec, count*sizeof(double));
        };
        int memcpy_from_rt_monitors (double *dvec, gsize count, int start_pos) {
                if (rt_monitors_shm_ptr){
                        memcpy (dvec, rt_monitors_shm_ptr+start_pos*sizeof(double), count*sizeof(double));
                        return -1;
                }
                return 0;
        };


        
        double unwrap (int k, double phi);
        
        // 1000mV = 0dB, 1mV = -60dB 
        inline double dB_from_mV (double mv){
                return 20.*(log10 (fabs(mv)+1e-10)-3.);
        };
        inline double db_to_y (double db, double dB_hi, double y_hi, double dB_mags){
                return -y_hi*2*((db-dB_hi)/20./dB_mags+0.5);
        };
        inline double deg_to_y (double deg, double y_hi){
#if 0 // use auto extent of range
                if (fabs (deg) > 180.*deg_extend)
                        if (deg_extend < 4)
                                deg_extend++;
#endif
                return -y_hi*deg/180./deg_extend;
        };
        inline double freq_to_y (double df, double y_hi){
                return -y_hi*df/10.0;
        };
        inline double binary_to_y (double x, int bit, int ch, double y_hi, int bits=8, int num_ch=2){
                int    bitv = (double) (1 << bit);
                double on = 0.4 * (((int)(x) & bitv) ? 1 : 0);
                double pos = ch*y_hi/(num_ch-1);
                double bith = 0.8*y_hi/(num_ch-1)/bits;
                return -(-y_hi+y_hi*ch+bith*(bit+on));
        };
         
private:
        int ch_freq;
        int ch_ampl;
        int data_decimation;
        int data_shr;
        int data_shr_max;
        int streampos;
        int x,y;
        int streaming;
        int operation_mode;
        int channel_selections[7];
        int deg_extend;

        double resonator_frequency_fitted;
        double resonator_phase_fitted;
        
        double bram_window_length; // scope window length in sec
        
        PACPLL_parameters parameters;
        PACPLL_signals signals;

        BuildParam *bp;
        
        gboolean run_scope;
        gboolean scope_xy_on;
        gboolean scope_fft_on;
        double scope_fft_time_zoom;
        gboolean scope_ac[5];
        double scope_z[2];
        double scope_dc_level[5];
        const gchar** Y1Y2_transport_mode;
        int transport;
        int bram_shift;
        int trigger_mode;
        double trigger_post_time;
        gchar bram_info[100];
        double bram_saved_buffer[5][4096];
        double gain_scale[5];
        double time_scale[5];
        gboolean unwrap_phase_plot;
        double scope_width_points;
        double scope_height_points;
        GtkWidget *signal_graph_area;

        GtkWidget *update_ts_widget;
        GtkWidget *update_op_widget;
        GtkWidget *update_tr_widget;

        GtkWidget *signal_graph;
        GtkWidget *dF_control_frame;
        GtkWidget *pulse_control_frame;
        Gtk_EntryControl *input_ddsfreq;
        UnitObj *Unity, *Hz, *Deg, *VoltDeg, *Volt, *mVolt, *VoltHz, *dB, *Time, *mTime, *uTime;

	GSList*   SPMC_RemoteEntryList;

	GMutex mutex;
        void *rt_monitors_shm_ptr;

       
public:
        const gchar* get_transport () { memset(bram_info,0,sizeof(bram_info)); strncpy(bram_info, Y1Y2_transport_mode[transport], sizeof(bram_info)); return Y1Y2_transport_mode[transport]; };
        // moved controls to main tab
        GtkWidget *input_rpaddress;
        GtkWidget *text_status;
	GtkWidget *red_pitaya_health;

        gint reconnect_mode;
        
        gint debug_level; 
        int scan_gvp_options;
        GSettings *inet_json_settings;
};

#endif
