/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 8 c-style: "K&R" -*- */

/* Gxsm - Gnome X Scanning Microscopy
 * universal STM/AFM/SARLS/SPALEED/... controlling and
 * data analysis software
 * 
 * Copyright (C) 1999,2000,2001,2002,2003 Percy Zahl
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

#ifndef __SRANGER_MK2_HWI_CONTROL_H
#define __SRANGER_MK2_HWI_CONTROL_H

#include "core-source/app_profile.h"

typedef enum { DestDSP, DestHwI } Destination;

typedef enum {
	NOTEBOOK_SR_CRTL_MAIN, NOTEBOOK_SR_CRTL_USER, NOTEBOOK_WINDOW_NUMBER
} Notebook_Window_Name;

typedef enum {
	NOTEBOOK_TAB_FBSCAN,
	NOTEBOOK_TAB_TRIGGER,
	NOTEBOOK_TAB_ADVANCED,
	NOTEBOOK_TAB_STS,
	NOTEBOOK_TAB_Z,
	NOTEBOOK_TAB_PL,
	NOTEBOOK_TAB_LPC,
	NOTEBOOK_TAB_SP,
	NOTEBOOK_TAB_TS,
	NOTEBOOK_TAB_GVP,
	NOTEBOOK_TAB_TK,
	NOTEBOOK_TAB_LOCKIN,
	NOTEBOOK_TAB_AX,
	NOTEBOOK_TAB_ABORT,
	NOTEBOOK_TAB_GRAPHS,
	NOTEBOOK_TAB_UT,
	NOTEBOOK_TAB_NUMBER
} Notebook_Tab_Name;


typedef union {
        struct { unsigned char ch, x, y, z; } s;
        unsigned long   l;
} AmpIndex;

/**
 * Vector Probe Vector
 */
typedef struct{
	gint32    n;             /**< 0: number of steps to do =WR */
	gint32    dnx;           /**< 2: distance of samples in steps =WR */
	guint32   srcs;          /**< 4: SRCS source channel coding =WR */
	guint32   options;       /**< 6: Options, Dig IO, ... not yet all defined =WR */
	guint32   ptr_fb;        /**< 8: optional pointer to new feedback data struct first 3 values of SPM_PI_FEEDBACK =WR */
	guint32   repetitions;   /**< 9: numer of repetitions =WR */
	guint32   i,j;           /**<10,11: loop counter(s) =RO/Zero */
	gint32    ptr_next;      /**<12: next vector (relative to VPC) until --rep_index > 0 and ptr_next != 0 =WR */
        gint32    ptr_final;     /**<13: next vector (relative to VPC), =1 for next Vector in VP, if 0, probe is done =WR */
	gint32    f_du;          /**<14: U (bias) stepwidth (32bit) =WR */
	gint32    f_dx;          /**<16: X stepwidth (32bit) =WR */
	gint32    f_dy;          /**<18: Y stepwidth (32bit) =WR */
	gint32    f_dz;          /**<20: Z stepwidth (32bit) =WR */
	gint32    f_dx0;         /**<22: X0 (offset) stepwidth (32bit) =WR */
	gint32    f_dy0;         /**<24: Y0 (offset) stepwidth (32bit) =WR */
	gint32    f_dphi;        /**<26: Phase stepwidth (32bit) +/-15.16Degree =WR */
} PROBE_VECTOR_GENERIC;


#define MM_OFF     0x00  // ------
#define MM_ON      0x01  // ON/OFF
#define MM_LOG     0x02  // LOG/LIN
#define MM_LV_FUZZY   0x04  // FUZZY-LV/NORMAL
#define MM_CZ_FUZZY   0x08  // FUZZY-CZ/NORMAL
#define MM_NEG     0x10  // NEGATE SOURCE (INPUT)


/**
 ** COMMON DSP ACTION/COMMAND/RESOURCE-IDs 
 ** ====== MUST MATCH WITH DSP VERSION !!!!!!!!!!
 ** may be some time a dynamic lookup...
 **/

#ifndef MD_IDLE
/** definitions used for statemaschine controls **/
#define MD_IDLE         0x0000  /**< all zero means ready, no idle task (scan,move,probe,approch,...) is running */
#define MD_HR           0x0001  /**< High Res mode enable (Sigma-Delta resolution enhancement via bit0 for Z) */
#define MD_BLK          0x0002  /**< Blinking indicator - DSPs heartbeat */
#define MD_PID          0x0004  /**< PID-Regler activ */
#define MD_XXX          0x0008  /**< Obsolete -- Log enable flag */
#define MD_OFFSETCOMP   0x0010  /**< Offset Compensation on */
#define MD_ZTRAN        0x0020  /**< Transform Z(AIC[7]) * (-1) -> AIC[6] */
#define MD_NOISE        0x0040  /**< add noise to Bias */
#define MD_OFFSETADDING 0x0080  /**< DSP-level Offset Adding */
#define MD_EXTFB        0x0100  /**< Switching external feedback control ON/OFF */
#define MD_ZPOS_ADJUSTER 0x0200  /**< must be normally enabled: auto readjust ZPos to center after probe (or XYZ scan manupilations) */
#define MD_WATCH        0x0400  /**< Watching variables on OUT7 */
#define MD_AIC5M7       0x0800  /**< Software differential inputs: AIC5&7 -> mix to virtual AIC5 */
#define MD_2_NOTUSED    0x1000  /**< reserved for future use */
#define MD_AIC_STOP     0x2000  /**< AICs can be placed in stop (no conversion will take place, dataprocess runs in dummy mode */
#define MD_AIC_RECONFIG 0x4000  /**< Request AIC Reconfig */
#define MD_3_NOTUSED    0x8000  /**< reserved for future use */

/** VP option masks ** MUST MATCH DSP DEFINITIONS ** **/
#define VP_FEEDBACK_HOLD 0x01
#define VP_AIC_INTEGRATE 0x02
#define VP_TRACK_REF     0x04
#define VP_TRACK_UP      0x08
#define VP_TRACK_DN      0x10
#define VP_TRACK_FIN     0x20
#define VP_TRACK_SRC     0xC0
#define VP_LIMITER       0x300 // Limiter ON/OFF flag mask
#define VP_LIMITER_UP    0x100 // Limit if > value
#define VP_LIMITER_DN    0x200 // Limit if < value
#define VP_LIMIT_SRC     0xC0  // Limiter "Value" source code bit mask 0x40+0x80  00: Z (IN0), 01: I (IN1), 10: (IN2), 11: (IN3) // MK2!, Signal Mk3
#define VP_GPIO_MSK        0x00ff0000 // GPIO 8bit mask (lower only)
#define VP_TRIGGER_P       0x01000000 // GPIO/signal trigger flag on pos edge -- release VP on "data & mask" or time end/out of section 
#define VP_TRIGGER_N       0x02000000 // GPIO/signal trigger flag on neg edge -- release VP on "data & mask" or time end/out of section 
#define VP_GPIO_SET        0x04000000 // GPIO set/update data -- once per section via statemachine, using idle cycle time for slow IO!!
#define VP_GPIO_READ       0x08000000 // GPIO set/update data -- once per section via statemachine, using idle cycle time for slow IO!!
#define VP_RESET_COUNTER_0 0x10000000
#define VP_RESET_COUNTER_1 0x20000000
#define VP_NODATA_RESERVED 0x80000000
#endif

typedef union {
        struct { unsigned char ch, x, y, z; } s;
        unsigned long   l;
} MixMode;

typedef struct{
	/* Mover Control */
	double MOV_Ampl, MOV_Speed, MOV_Steps;
	int MOV_GPIO_setting; /* Parameter Memory for different Mover/Slider Modes */
	
#define DSP_AFMMOV_MODES 6
	double AFM_Amp, AFM_Speed, AFM_Steps;  /* STM/AFM Besocke Kontrolle -- current */
	int    AFM_GPIO_setting; /* Parameter Memory for different Mover/Slider Modes */
	double AFM_usrAmp[DSP_AFMMOV_MODES];  
	double AFM_usrSpeed[DSP_AFMMOV_MODES];
	double AFM_usrSteps[DSP_AFMMOV_MODES]; /* Parameter Memory for different Mover/Slider Modes */
	int    AFM_GPIO_usr_setting[DSP_AFMMOV_MODES]; /* Parameter Memory for different Mover/Slider Modes */

#define MOV_MAXWAVELEN     4096
#define MOV_WAVE_SAWTOOTH  0
#define MOV_WAVE_SINE      1
#define MOV_WAVE_CYCLO     2
#define MOV_WAVE_CYCLO_PL  3
#define MOV_WAVE_CYCLO_MI  4
#define MOV_WAVE_CYCLO_IPL 5
#define MOV_WAVE_CYCLO_IMI 6
#define MOV_WAVE_STEP      7
#define MOV_WAVE_PULSE     8
#define MOV_WAVE_USER      9
#define MOV_WAVE_USER_TTL 10
#define MOV_WAVE_KOALA    11
#define MOV_WAVE_BESOCKE  12
#define MOV_WAVE_PULSEX   13
#define MOV_WAVE_GPIO     14
#define MOV_WAVE_STEPPERMOTOR 15
#define MOV_WAVE_LAST     16

#define MOV_AUTO_APP_MODE 0x0100
        
	int MOV_output, MOV_waveform_id;
        int wave_out_channels_used;
	int wave_out_channel_xyz[6][3];
	int MOV_wave_len;
	int MOV_wave_speed_fac;
	short MOV_waveform[MOV_MAXWAVELEN];
        double MOV_angle;          //0° -> X-direction      90° -> Y-direction                   [(-180)°; 180°] -> xy-plane         <(-180): (-Z)-direction        >180°:(+Z)-direction
	double final_delay;
	double max_settling_time;
        double retract_ci;
	double inch_worm_phase;
	double Wave_space, Wave_offset;
        double z_Rate;
        double time_delay_1;
        double time_delay_2;
        int GPIO_on, GPIO_off, GPIO_reset, GPIO_scan, GPIO_tmp1, GPIO_tmp2, GPIO_direction;
	double GPIO_delay;
        int axis_counts[DSP_AFMMOV_MODES][3];
        int axis_counts_ref[DSP_AFMMOV_MODES][3];
} Mover_Param;

typedef enum { PV_MODE_NONE, PV_MODE_IV, PV_MODE_FZ, PV_MODE_PL, PV_MODE_LP, PV_MODE_SP, PV_MODE_TS, PV_MODE_GVP, PV_MODE_AC, PV_MODE_AX, PV_MODE_TK, PV_MODE_ABORT } pv_mode;
typedef enum { MAKE_VEC_FLAG_NORMAL=0, MAKE_VEC_FLAG_VHOLD=1, MAKE_VEC_FLAG_RAMP=2, MAKE_VEC_FLAG_END=4 } make_vector_flags;


typedef struct{
	double  FSineHz;

	double 	Tau_PAC;
	double  deltaReT;
	double  deltaImT;

	double  PI_Phase_Out;
	double  volumeSine;
	gint32  ctrlmode_Phase;
	double  setpoint_Phase;
	double  corrmax_Phase;
	double  corrmin_Phase;
	double  memI_Phase;
	double  icoef_Phase;
	double  pcoef_Phase;
	double  errorPI_Phase;
	double  signum_ci_Phase, ci_gain_Phase;
	double  signum_cp_Phase, cp_gain_Phase;
	double  auto_set_BW_Phase;

	gint32  ctrlmode_Amp;
	double  setpoint_Amp;
	double  corrmax_Amp;
	double  corrmin_Amp;
	double  memI_Amp;
	double  icoef_Amp;
	double  pcoef_Amp;
	double  errorPI_Amp;
	double  signum_ci_Amp, ci_gain_Amp;
	double  signum_cp_Amp, cp_gain_Amp;
	double  auto_set_BW_Amp;

	double  Qfactor;
	double  GainRes;

	gint32  LPSelect_Phase;
	gint32  LPSelect_Amp;

	double  Reference[4];
	double  Range[4];
	double  ClipMin[4];
	double  ClipMax[4];

// life watches
	double  InputFiltered;
	double  SineOut0;
	double  phase;
	double  amp_estimation;
	double  res_gain_computed;
	#define F64_Excitation 0
	#define F64_ResPhaseLP 1
	#define F64_ResAmpLP   2
	#define F64_ExecAmpLP  3
	double  Filter64Out[4]; // [0] : excitation, [1] : ResPhaseLP, [2] : ResAmpLP, [3] ExecAmpLP

// #define MAX_PAC_BLCKLEN 999999
#define PAC_BLCKLEN       1000000
	gint32  blcklen;
//	double  Signal1[PAC_BLCKLEN]; // -- from PSignal1 pointed block on DSP
//	double  Signal2[PAC_BLCKLEN]; // -- from PSignal1 pointed block on DSP
	double  *Signal1; // -- from PSignal1 pointed block on DSP
	double  *Signal1x; // -- from PSignal1 pointed block on DSP
	double  *Signal2; // -- from PSignal1 pointed block on DSP
	double  *Signal2x; // -- from PSignal1 pointed block on DSP
	gint32  *tmp_array;
	double  *x_array;
	double  *x_array_x;

	double  tune_freq[3]; // start, stop, step
	double  tune_integration_time;

        gint32  ctrlmode_Amp_adaptive;
        double  ctrlmode_Amp_adaptive_delta;
        double  ctrlmode_Amp_adaptive_ratio;
        double  ctrlmode_Amp_adaptive_max;

} PAC_control;

class DSP_GUI_Builder;

class DSPControl : public AppBase{
	friend class sranger_mk2_hwi_dev;

#define DSP_FB_ON  1
#define DSP_FB_OFF 0

#define FLAG_FB_ON       0x01 // FB on
#define FLAG_DUAL        0x02 // Dual Data
#define FLAG_SHOW_RAMP   0x04 // show ramp data
#define FLAG_INTEGRATE   0x08 // integrate and normalize date of all AIC data point inbetween

#define FLAG_AUTO_SAVE   0x01 // auto save
#define FLAG_AUTO_PLOT   0x02 // auto plot
#define FLAG_AUTO_GLOCK  0x04 // auto graph lock
#define FLAG_AUTO_RUN_INITSCRIPT  0x08 // auto run init script

 public:
        DSPControl (Gxsm4app *app);
        virtual ~DSPControl();

        virtual void AppWindowInit(const gchar *title=NULL, const gchar *sub_title=NULL);

	static void configure_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data);

        static void show_tab_to_configure (GtkWidget* w, gpointer data){
                gtk_widget_show (GTK_WIDGET (g_object_get_data (G_OBJECT (w), "TabGrid")));
        };
        static void show_tab_as_configured (GtkWidget* w, gpointer data){
                if (gtk_check_button_get_active (GTK_CHECK_BUTTON (w)))
                        gtk_widget_show (GTK_WIDGET (g_object_get_data (G_OBJECT (w), "TabGrid")));
                else
                        gtk_widget_hide (GTK_WIDGET (g_object_get_data (G_OBJECT (w), "TabGrid")));
        };


        GtkWidget* build_tab_start (const gchar *name, const gchar *settings_name, Notebook_Tab_Name id, GtkWidget **sc=NULL);
	void AddProbeModes (GtkWidget *notebook);

	void get_tab_settings (const gchar *tab_key, guint64 &option_flags, guint64 &AC_auto_flags, guint64 glock_data[6]);
        void set_tab_settings (const gchar *tab_key, guint64 option_flags, guint64 AC_auto_flags, guint64 glock_data[6]);

	void save_values (NcFile *ncf);
        void load_values (NcFile *ncf);

	void recalculate_dsp_scan_speed_parameters (gint32 &dsp_scan_dnx, gint32 &dsp_scan_dny, 
						    gint32 &dsp_scan_fs_dx, gint32 &dsp_scan_fs_dy, 
						    gint32 &dsp_scan_nx_pre, gint32 &dsp_scan_fast_return);
	void recalculate_dsp_scan_slope_parameters (gint32 &dsp_scan_fs_dx, gint32 &dsp_scan_fs_dy,
						    gint32 &dsp_scan_fm_dz0x, gint32 &dsp_scan_fm_dz0y,
						    double swx=1., double swy=1.);

	void update();
        void updateDSP(int FbFlg=-1);
	//emvoid update_trigger (gboolean flg);

	double GetUserParam (gint n, gchar *id);
	gint SetUserParam (gint n, gchar *id, double value);

	void update_zpos_readings ();
	
        static void ChangedNotify(Param_Control* pcs, gpointer data);
        static int ChangedAction(GtkWidget *widget, DSPControl *dspc);
        static int feedback_callback(GtkWidget *widget, DSPControl *dspc);
        static int spd_link_callback(GtkWidget *widget, DSPControl *dspc);
	static guint refresh_zpos_readings(DSPControl *dspc);
        static int zpos_monitor_callback(GtkWidget *widget, DSPControl *dspc);
        static int IIR_callback(GtkWidget *widget, DSPControl *dspc);
        static int set_clr_mode_callback(GtkWidget *widget, gpointer mask);
        static int se_auto_trigger_callback(GtkWidget *widget, DSPControl *dspc);
        static int ldc_callback(GtkWidget *widget, DSPControl *dspc);
        static void lockin_adjust_callback(Param_Control* pcs, gpointer data);
        static int lockin_runfree_callback(GtkWidget *widget, DSPControl *dspc);
        static int fast_scan_callback(GtkWidget *widget, DSPControl *dspc);
        static int choice_mixmode_callback(GtkWidget *widget, DSPControl *dspc);
        static int choice_ivc_gain_callback(GtkWidget *widget, DSPControl *dspc);
        static int choice_mixsource_callback(GtkWidget *widget, DSPControl *dspc);  
	static int choice_vector_index_j_callback(GtkWidget *widget, DSPControl *dspc);
	static int choice_scansource_callback(GtkWidget *widget, DSPControl *dspc);
        static int choice_prbsource_callback(GtkWidget *widget, DSPControl *dspc);
        static int choice_lcksource_callback(GtkWidget *widget, DSPControl *dspc);
        static int choice_Ampl_callback(GtkWidget *widget, DSPControl *dspc);
        static int auto_probe_callback(GtkWidget *widget, DSPControl *dspc);
	static int LockIn_exec_callback(GtkWidget *widget, DSPControl *dspc);
	static int LockIn_read_callback(GtkWidget *widget, DSPControl *dspc);
	static int LockIn_write_callback(GtkWidget *widget, DSPControl *dspc);
	static int Probing_exec_IV_callback(GtkWidget *widget, DSPControl *dspc);
	static int Probing_write_IV_callback(GtkWidget *widget, DSPControl *dspc);
	static int Probing_exec_FZ_callback(GtkWidget *widget, DSPControl *dspc);
	static int Probing_write_FZ_callback(GtkWidget *widget, DSPControl *dspc);
	static int Probing_exec_PL_callback(GtkWidget *widget, DSPControl *dspc);
	static int Probing_write_PL_callback(GtkWidget *widget, DSPControl *dspc);
	static int Probing_exec_LP_callback(GtkWidget *widget, DSPControl *dspc);
	static int Probing_write_LP_callback(GtkWidget *widget, DSPControl *dspc);
	static int Probing_exec_SP_callback(GtkWidget *widget, DSPControl *dspc);
	static int Probing_write_SP_callback(GtkWidget *widget, DSPControl *dspc);
	static int Probing_exec_TS_callback(GtkWidget *widget, DSPControl *dspc);
	static int Probing_write_TS_callback(GtkWidget *widget, DSPControl *dspc);
	static int Probing_exec_GVP_callback(GtkWidget *widget, DSPControl *dspc);
	static int Probing_write_GVP_callback(GtkWidget *widget, DSPControl *dspc);
	static int Probing_exec_TK_callback(GtkWidget *widget, DSPControl *dspc);
	static int Probing_write_TK_callback(GtkWidget *widget, DSPControl *dspc);
	static int Probing_exec_AX_callback(GtkWidget *widget, DSPControl *dspc);
	static int Probing_write_AX_callback(GtkWidget *widget, DSPControl *dspc);
	static int Probing_exec_ABORT_callback(GtkWidget *widget, DSPControl *dspc);
	static int Probing_write_ABORT_callback(GtkWidget *widget, DSPControl *dspc);
	static int Probing_exec_RF_callback(GtkWidget *widget, DSPControl *dspc);
        static int Probing_event_setup_scan (int ch, const gchar *titleprefix, const gchar *name, const gchar *unit, const gchar *label, double d2u, int nvalues);
	static int Probing_eventcheck_callback(GtkWidget *widget, DSPControl *dspc);
	static int Probing_graph_callback(GtkWidget *widget, DSPControl *dspc, int finish_flag=0);
	static int Probing_save_callback(GtkWidget *widget, DSPControl *dspc);
	static int Probing_abort_callback(GtkWidget *widget, DSPControl *dspc);
        static int change_source_callback (GtkWidget *widget, DSPControl *dspc);
	static int callback_change_AC_option_flags (GtkWidget *widget, DSPControl *dspc);
	static int callback_change_AC_auto_flags (GtkWidget *widget, DSPControl *dspc);
	static int callback_change_IV_option_flags (GtkWidget *widget, DSPControl *dspc);
	static int callback_change_IV_auto_flags (GtkWidget *widget, DSPControl *dspc);
	static int callback_change_FZ_option_flags (GtkWidget *widget, DSPControl *dspc);
	static int callback_change_FZ_auto_flags (GtkWidget *widget, DSPControl *dspc);
	static int callback_change_PL_option_flags (GtkWidget *widget, DSPControl *dspc);
	static int callback_change_PL_auto_flags (GtkWidget *widget, DSPControl *dspc);
	static int callback_change_LP_option_flags (GtkWidget *widget, DSPControl *dspc);
	static int callback_change_LP_auto_flags (GtkWidget *widget, DSPControl *dspc);
	static int callback_change_SP_option_flags (GtkWidget *widget, DSPControl *dspc);
	static int callback_change_SP_auto_flags (GtkWidget *widget, DSPControl *dspc);
	static int callback_change_TS_option_flags (GtkWidget *widget, DSPControl *dspc);
	static int callback_change_TS_auto_flags (GtkWidget *widget, DSPControl *dspc);
	static int callback_change_GVP_vpc_option_flags (GtkWidget *widget, DSPControl *dspc);
	static int callback_update_GVP_vpc_option_checkbox (GtkWidget *widget, DSPControl *dspc);
	static int callback_change_GVP_option_flags (GtkWidget *widget, DSPControl *dspc);
	static int callback_change_GVP_auto_flags (GtkWidget *widget, DSPControl *dspc);
	static int callback_edit_GVP (GtkWidget *widget, DSPControl *dspc);
	static int callback_GVP_store_vp (GtkWidget *widget, DSPControl *dspc);
	static int callback_GVP_restore_vp (GtkWidget *widget, DSPControl *dspc);
	static int callback_change_TK_ref(GtkWidget *widget, DSPControl *dspc);
	static int callback_change_FZ_ref(GtkWidget *widget, DSPControl *dspc);
	static int callback_change_TK_option_flags (GtkWidget *widget, DSPControl *dspc);
	static int callback_change_TK_auto_flags (GtkWidget *widget, DSPControl *dspc);
	static int callback_change_AX_option_flags (GtkWidget *widget, DSPControl *dspc);
	static int callback_change_AX_auto_flags (GtkWidget *widget, DSPControl *dspc);
	static int callback_change_ABORT_option_flags (GtkWidget *widget, DSPControl *dspc);
	static int callback_change_ABORT_auto_flags (GtkWidget *widget, DSPControl *dspc);
	static int callback_XJoin (GtkWidget *widget, DSPControl *dspc);
	static int callback_GrMatWindow (GtkWidget *widget, DSPControl *dspc);
	static int DSP_multiIV_callback (GtkWidget *widget, DSPControl *dspc);
	static int DSP_multiBias_callback (GtkWidget *widget, DSPControl *dspc);

	static int DSP_cret_callback (GtkWidget *widget, DSPControl *dspc);
	static int DSP_slope_callback (GtkWidget *widget, DSPControl *dspc);

	const char* vp_label_lookup(int i);
	const char* vp_unit_lookup(int i);
	double      vp_scale_lookup(int i);

	void update_sourcesignals_from_DSP_callback ();

	// !!!!!!!!!??????? THREAD PROBLEM ?????????????!!!!!!!!!!!!!!!!! try new g_idle_add 

	static gboolean idle_callback (gpointer data){
		DSPControl *dspc = (DSPControl*) data;

                // make probe vector data is not locked for vector manipulations, wait until available (fast)
                while (dspc->pv_lock)
                        usleep (1000);

                dspc->gr_lock = TRUE;
		dspc->Probing_graph_callback (NULL, dspc, dspc->idle_callback_data_ff);
                dspc->gr_lock = FALSE;
                
		dspc->idle_id = 0; // done.

		return FALSE;
	};
		
	void Probing_graph_update_thread_safe (int finish_flag=0) {
                static int timeout_count=0;
                static int last_index=0;
                static int count=0;
                // g_message ("Probing_graph_update_thread_safe: current index=%d, last=%d", current_probe_data_index, last_index);

                if (current_probe_data_index <= last_index && !finish_flag){
                        // g_message ("Probing_graph_update_thread_safe: exit no new data");
                        return;
                }
                last_index = current_probe_data_index;
                ++count;
                // call: Probing_graph_callback (NULL, this, finish_flag);
                // check for --  idle_id ??
                while (idle_id && finish_flag){
                        g_message ("Probing_graph_update_thread_safe: Finish_flag set -- waiting for last update to complete. current index=%d, last=%d, #toc=%d",
                                   current_probe_data_index, last_index, timeout_count);
                        usleep(250000);
                        ++timeout_count;
                        
                        if (last_index == current_probe_data_index && timeout_count > 10){
                                g_warning ("Probing_graph_update_thread_safe: Trying auto recovery from stalled update (timeout reached). current index=%d, last=%d",
                                         current_probe_data_index, last_index);
                                idle_id = 0;
                                timeout_count = 0;
                        }
                        
                }
                if (idle_id == 0){
                        // g_message ("Probing_graph_update_thread_safe: plot update. delayed by %d attempts. Current data index=%d",
                        //           count, current_probe_data_index);
                        count=0;
                        idle_callback_data_ff = finish_flag;
                        idle_id = g_idle_add (DSPControl::idle_callback, this);
                        if (finish_flag){
                                g_message ("Probing_graph_update_thread_safe: plot update. Finished. Current data index=%d",
                                           current_probe_data_index);
                                last_index = 0;
                        }
                } else {
                        // g_warning ("Probing_graph_update_thread_safe: is busy, skipping. [%d]", count);
                }
	};


	static gboolean idle_callback_update_gui (gpointer data){
		DSPControl *dspc = (DSPControl*) data;
                gapp->spm_update_all();
                gapp->SetStatus(N_("Saved VP data: "), dspc->idle_callback_data_fn);
                gchar *bbt = g_strdup_printf ("Save now - last: %s", dspc->idle_callback_data_fn);
                gtk_button_set_label (GTK_BUTTON (dspc->save_button), bbt);
                g_free (bbt);
                g_free (dspc->idle_callback_data_fn); // clean up tmp data now
                dspc->idle_callback_data_fn = NULL;
		dspc->idle_id_update_gui = 0; // done.
		return FALSE;
	};
		
	void update_gui_thread_safe (const gchar *fntmp) {
                // execute GUI updated thread safe
                if (idle_id_update_gui == 0){
                        idle_callback_data_fn = g_strdup (fntmp);
                        idle_id_update_gui = g_idle_add (DSPControl::idle_callback_update_gui, this);
                } else {
                        g_warning ("update_gui_thread_safe: is busy, skipping [%s].", fntmp);
                }
	};



        
	int probedata_length () { return current_probe_data_index; };
	void push_probedata_arrays ();
	GArray** pop_probedata_arrays ();
	GArray** pop_probehdr_arrays ();
	void init_probedata_arrays ();
	static void free_probedata_array_set (GArray** garr, DSPControl *dc);
	static void free_probehdr_array_set (GArray** garr, DSPControl *dc);
	void free_probedata_arrays ();

	int check_vp_in_progress (const gchar *extra_info=NULL);

	void add_probedata(double data[13]);
	void add_probevector();
	void set_probevector(double pv[9]);

	void add_probe_hdr(double pv[9]);
	void dump_probe_hdr();

	void probedata_visualize (GArray *probedata_x, GArray *probedata_y,  GArray *probedata_sec, 
				  ProfileControl* &pc, ProfileControl* &pc_av, int plot_msk,
				  const gchar *xlab, const gchar *xua, double xmult,
				  const gchar *ylab, const gchar *yua, double ymult,
				  int current_i, int si, int nas, gboolean join_same_x=FALSE,
                                  gint xmap=0, gint src=0, gint num_active_xmaps=1, gint num_active_sources=1);

	void StartScanPreCheck ();
	void EndScanCheck ();

        GSettings *get_hwi_settings(){ return hwi_settings; };

        pv_mode write_vector_mode;

	void store_values ();

	double zpos_ref;
	gint   zpos_refresh_timer_id;
	  
	double frq_ref;           //!< Frequency Reference: i.e. feedback, scan, ... dataaq. sampling rate
	int    feedback_flag;
	// -- section ANALOG_VALUES --
	double bias;           //!< Bias (usually applied to the sample)
	double bias_sec[4];    //!< Bias (usually applied to the sample)
	double motor;          //!< Additional "Motor"-Output -- aux parameter for gate, source, etc.

	// -- section SPM_PI_FEEDBACK MIXER --
	int    mix_fbsource[4];   // only for documentation
	double mix_unit2volt_factor[4]; // on default: [0] Current Setpoint (STM; log mode) [1..3]: off
	double mix_set_point[4]; // on default: [0] Current Setpoint (STM; log mode) [1..3]: off
	double mix_gain[4];      // Mixing Gains: 0=OFF, 1=100%, note: can be > 1 or even negative for scaling purposes
	double mix_level[4];     // fuzzy mixing control via level, applied only if fuzzy flag set
	// -- may not yet be applied to all signal --> check with DSP code
	int    mix_transform_mode[4]; //!< transformation mode on/off log/lin iir/full fuzzy/normal

	int    scan_source[4];   // only for documentation

	int    probe_source[7];    //!< probe source mapping for 32bit data channels [0..3] and trigger [4], limiter [5], tracker [6] input signals
	int    probe_output[4];    //!< probe output mapping for 32bit data channels (in preparation)
	int    lockin_input[4];    //!< LockIn input A, B selection only, -- alt. A,B ref (TDB)

	int    vp_input_id_cache[4];  // cache VP input config;
	int    DSP_vpdata_ij[2];
	GtkWidget *VPSig_menu, *VPSig_mi[8], *VPScanSrcVPitem[4];

	// SERVO COMPONENTS:
#define SERVO_SETPT 0
#define SERVO_CP    1
#define SERVO_CI    2
	double z_servo[3];    // Z-Servo (Feedback) [0] (not used here), [1] Const Proportional, [2] Const Integral [user visible values]
	int    IIR_flag;          // IIR on/off
	double IIR_f0_min, IIR_f0_max[4], IIR_f_now; // IIR autoadjust frq. limits
	double IIR_I_crossover; // IIR self adapt crossover current setting
	double LOG_I_offset;    // tune up cheat value before log

	// M-Servo-Module
	double m_servo[3];

	// -- section AREA_SCAN --
	int    dsp_scan_pflg;
	double dynamic_zoom;      //!< on-the-fly zooming, 1 = normal
	double fast_return;       //!< on-the-fly fast return option (scan retrace speed override factor, 1=normal)
	double x2nd_Zoff;         //!< Z lift off for 2nd scan line (MFM etc...)
	double area_slope_x;      //!< slope compensation in X, in scan coordinate system (possibly rotated) -- applied before, but by feedback
	double area_slope_y;      //!< slope compensation in Y, in scan coordinate system (possibly rotated) -- applied before, but by feedback
	Gtk_EntryControl *slope_x_ec;
	Gtk_EntryControl *slope_y_ec;
	int    area_slope_compensation_flag; //!< enable/disable slope compensation
	int    center_return_flag; //!< enable/disable return to center after scan
	double move_speed_x;      //!< in DAC (AIC) units per second, GXSM core computes from A/s using X-gain and Ang/DAC...
	double scan_speed_x_requested;      //!< in DAC (AIC) units per second - requested
	double scan_speed_x;      //!< in DAC (AIC) units per second - best match
	Gtk_EntryControl *scan_speed_ec;
	double gain_ratio;        //!< may be used later for compensation of direction dependence of speed in case of different X/Y gains. 
	                          //!< i.e. it is gain-X/gain-Y
	int pre_points;           //!< pre point before scan line data is taken (experimental)
	int scan_forward_slow_down; //!<  additional scan forward slow down =1 normally
	int scan_forward_slow_down_2nd; //!<  additional scan forward slow down =1 normally
	
        GtkWidget *VXYZS0Gain[6];
        double AdjustGain (int channel, int index) {
                gtk_combo_box_set_active (GTK_COMBO_BOX (VXYZS0Gain[channel]), index);
                return 0.;
        };
        
        // -- linear drift correction --
	double dxdt, dydt, dzdt;  //!< linear drift correction speeds in Ang/s
	int    ldc_flag;          //! LDC status at last update
	GtkWidget *LDC_status;    //!< linear drift correction flag (on/off)
	int    fast_scan_flag;    //!< Fast Scan flag
	GtkWidget *FastScan_status;    //!< Fast Scan mode flag (on/off) -- Mk3 so far only
        // -- DSP Scan DNX,DNY mirror
        gint32 mirror_dsp_scan_dx32, mirror_dsp_scan_dy32;

	Gtk_EntryControl *ZPos_ec;
        
	// UserEvent sensitive:
	double ue_bias;
	double ue_set_point[4];
	double ue_z_servo[3];
	double ue_scan_speed_x_r;
	double ue_scan_speed_x;
	double ue_slope_x;
	double ue_slope_y;
	double ue_slope_flg;

	double volt_points[10];
	int    num_points[10];

	// LockIn
	double    noise_amp; // auxillary use
	double    AC_amp[4], AC_frq, AC_phaseA, AC_phaseB;
	Gtk_EntryControl *AC_frq_ec;
	double    AC_phase_span, AC_phase_slope, AC_final_delay;
	int       AC_points;
	int       AC_repetitions;
	int       AC_lockin_avg_cycels;
	guint64   AC_option_flags;
	guint64   AC_auto_flags;
	GtkWidget *LockIn_mode;
	GtkWidget *AC_status;
	guint64   AC_glock_data[6];

	// Probing
	int probe_trigger_raster_points_user;
	int probe_trigger_raster_points;
	int probe_trigger_raster_points_b;
	int probe_trigger_single_shot;

	// Graphs Folder -- user settings origin
	int Source, XSource, PSource;
        gboolean XJoin, GrMatWin;
	int PlotAvg, PlotSec;

	// Graphs used life -- dep. on GLOCK if copy of user settings or memorized last setting
	int vis_Source, vis_XSource, vis_XJoin, vis_PSource;
	int vis_PlotAvg, vis_PlotSec;

	int probe_ready;
	gchar *probe_fname;
	int probe_findex;

	// vector generation helpers
	void make_auto_n_vector_elments (double fnum);
	double make_Vdz_vector (double Ui, double Uf, double dZ, int n, double slope, int source, int options, double long &duration, make_vector_flags flags);
	double make_Vdx0_vector (double Ui, double Uf, double dZ, int n, double slope, int source, int options, double long &duration, make_vector_flags flags);
	double make_dx0_vector (double X0i, double X0f, int n, double slope, int source, int options, double long &duration, make_vector_flags flags);
	double make_ZXYramp_vector (double dZ, double dX, double dY, int n, double slope, int source, int options, double long &duration, make_vector_flags flags);
	double make_UZXYramp_vector (double dU, double dZ, double dX, double dY, double dSig1, double dSig2, int n, int nrep, int ptr_next, double ts, int source, int options, double long &duration, make_vector_flags flags);
	double make_phase_vector (double dPhi, int n, double slope, int source, int options, double long &duration, make_vector_flags flags);
	double make_delay_vector (double delay, int source, int options, double long &duration, make_vector_flags flags, int points=0);
	void append_null_vector (int options, int index);

	PROBE_VECTOR_GENERIC dsp_vector;

	// STS (I-V)
	int    IV_sections; // 1 .. 8max
	double IV_start[8], IV_end[8], IV_slope, IV_slope_ramp, IV_final_delay, IV_recover_delay;
	double IV_dz, IV_dM;
	int    IV_points[8];
	int    IV_repetitions;
	int    IVdz_repetitions;
	double IV_dx, IV_dy, IV_dxy_slope, IV_dxy_delay;
	int    IV_dxy_points;
	guint64 IV_option_flags;
	guint64 IV_auto_flags;
	GtkWidget *IV_status;
	guint64 IV_glock_data[6];


	// FZ (Force-Z(Distance))
	double FZ_start, FZ_end, FZ_slope, FZ_slope_ramp, FZ_final_delay, FZ_limiter_val[2];
	int    FZ_points, FZ_limiter_ch;
	int    FZ_repetitions;
	guint64    FZ_option_flags;
	guint64    FZ_auto_flags;
	GtkWidget *FZ_status;
	guint64    FZ_glock_data[6];

	// PL (Puls)
	GtkWidget *PL_remote_set_target;
	double PL_remote_set_value;
	double PL_duration, PL_time_resolution, PL_slope, PL_volt, PL_dZ, PL_dZ_ext, PL_step, PL_stepZ, PL_initial_delay, PL_final_delay;
	int    PL_repetitions;
	guint64    PL_option_flags;
	guint64    PL_auto_flags;
	GtkWidget *PL_status;
	guint64    PL_glock_data[6];

	// LP (Laserpuls)
	double LP_duration, LP_slope, LP_volt, LP_final_delay, LP_FZ_end, LP_triggertime;
	int    LP_repetitions;
	guint64    LP_option_flags;
	guint64    LP_auto_flags;
	GtkWidget *LP_status;
	guint64    LP_glock_data[6];

	// SP (Special/Slow Puls)
	double SP_duration, SP_ramptime, SP_volt, SP_final_delay, SP_flag_volt;
	int    SP_repetitions;
	guint64    SP_option_flags;
	guint64    SP_auto_flags;
	GtkWidget *SP_status;
	guint64    SP_glock_data[6];

	// TS (Time Spectroscopy)
	double TS_duration, TS_points;
	int    TS_repetitions;
	guint64    TS_option_flags;
	guint64    TS_auto_flags;
	GtkWidget *TS_status;
	guint64    TS_glock_data[6];

	// GVP (General Vector Probe)
#define GVP_GPIO_KEYCODE    57
#define GVP_GPIO_KEYCODE_S "57"
#define N_GVP_VECTORS 45 // 50 vectors max total, need a few extra for controls and finish.
	double GVP_du[N_GVP_VECTORS], GVP_dx[N_GVP_VECTORS], GVP_dy[N_GVP_VECTORS], GVP_dz[N_GVP_VECTORS], GVP_dsig[N_GVP_VECTORS], GVP_ts[N_GVP_VECTORS], GVP_final_delay;
	gint32 GVP_points[N_GVP_VECTORS];
	gint32 GVP_opt[N_GVP_VECTORS];   // options
	gint32 GVP_data[N_GVP_VECTORS];  // GPIO data
	gint32 GVP_vnrep[N_GVP_VECTORS]; // Vector N repetitions
	gint32 GVP_vpcjr[N_GVP_VECTORS]; // VPC jump relative length
	int    GVP_repetitions;
	int    GVP_GPIO_lock;
	guint64    GVP_option_flags;
	guint64    GVP_auto_flags;
	GtkWidget *VPprogram[10];
	GtkWidget *GVP_status;
	guint64    GVP_glock_data[6];
	gchar  GVP_key[8];
	void GVP_store_vp (const gchar *key);
	void GVP_restore_vp (const gchar *key);

	// TK (Track Mode)
	double TK_r, TK_r2, TK_speed, TK_delay;
	int    TK_points, TK_nodes;
	int    TK_mode;
	int    TK_ref;
	int    TK_repetitions;
	guint64    TK_option_flags;
	guint64    TK_auto_flags;
	GtkWidget *TK_status;
	guint64    TK_glock_data[6];

	// AX (Auxillary Probe -- QMA, CMA, etc.)
	double AX_to_volt, AX_resolution, AX_gain, AX_gatetime;
	double AX_start, AX_end, AX_slope, AX_slope_ramp, AX_final_delay;
	int    AX_points;
	int    AX_repetitions;
	guint64    AX_option_flags;
	guint64    AX_auto_flags;
	GtkWidget *AX_status;
	guint64    AX_glock_data[6];

	// ABORT
	double ABORT_final_delay;
	guint64    ABORT_option_flags;
	guint64    ABORT_auto_flags;
	GtkWidget *ABORT_status;
	guint64    ABORT_glock_data[6];

	// -- Profile Displays
	int last_probe_data_index;

	// dynamic temporary probe data storage
	GSList *probedata_list;
	GSList *probehdr_list;
	int num_probe_events;
	// -- Array of full expanded probe data set
#define NUM_PROBEDATA_ARRAYS 27
	GArray *garray_probe_hdrlist[NUM_PROBEDATA_ARRAYS];
	GArray *garray_probedata[NUM_PROBEDATA_ARRAYS];
	int current_probe_data_index;
	int nun_valid_data_sections;
	int nun_valid_hdr, last_nun_hdr_dumped;
#define PROBEDATA_ARRAY_INDEX 0 // Array [0] holds the probe index over all sections
#define PROBEDATA_ARRAY_TIME  1 // Array [1] holds the time
#define PROBEDATA_ARRAY_X0    2 // Array [2] holds X-Offset
#define PROBEDATA_ARRAY_Y0    3 // Array [3] holds Y-Offset
#define PROBEDATA_ARRAY_PHI   4 // Array [4] holds Z-Offset
#define PROBEDATA_ARRAY_XS    5 // Array [5] holds X-Scan
#define PROBEDATA_ARRAY_YS    6 // Array [6] holds Y-Scan
#define PROBEDATA_ARRAY_ZS    7 // Array [7] holds Z-Scan
#define PROBEDATA_ARRAY_U     8 // Array [8] holds U (Bias)
#define PROBEDATA_ARRAY_SEC   9 // Array [9] holds Section Index
#define PROBEDATA_ARRAY_AIC5OUT_ZMON 10 // Array [10] holds ZMON (AIC5 out)
#define PROBEDATA_ARRAY_AIC6OUT_UMON 11 // Array [11] holds UMON (AIC6 out)
#define PROBEDATA_ARRAY_AIC0         12 // Array [12] holds FBS (Feedback Source, i.e. I, df, force, ...)
#define PROBEDATA_ARRAY_AIC1         13 // Array [13] holds AIC0 in
#define PROBEDATA_ARRAY_AIC2         14 // Array [14] holds AIC1 in
#define PROBEDATA_ARRAY_AIC3         15 // Array [15] holds AIC2 in
#define PROBEDATA_ARRAY_AIC4         16 // Array [16] holds AIC3 in
#define PROBEDATA_ARRAY_AIC5         17 // Array [17] holds AIC4 in
#define PROBEDATA_ARRAY_AIC6         18 // Array [18] holds AIC6 in (not used yet)
#define PROBEDATA_ARRAY_AIC7         19 // Array [19] holds AIC7 in (not used yet)
#define PROBEDATA_ARRAY_LCK0         20 // Array [20] holds LockIn0st
#define PROBEDATA_ARRAY_LCK1A        21 // Array [21] holds LockIn1st
#define PROBEDATA_ARRAY_LCK1B        22 // Array [22] holds LockIn22st
#define PROBEDATA_ARRAY_LCK2A        23 // Array [23] holds LockIn1st
#define PROBEDATA_ARRAY_LCK2B        24 // Array [24] holds LockIn22st
#define PROBEDATA_ARRAY_COUNT        25 // Array [25] holds Count
#define PROBEDATA_ARRAY_BLOCK        26 // Array [26] holds Block start index (hold start index for every section) 

#define PROBEDATA_ARRAY_END          PROBEDATA_ARRAY_COUNT // last element number

#define MAX_NUM_CHANNELS 26

	// The factor of 2 is to have it big enough to hold also the averaged data
	ProfileControl *probe_pc_matrix[2*MAX_NUM_CHANNELS][2*MAX_NUM_CHANNELS];

	gchar *vp_exec_mode_name;

	guint64    current_auto_flags;
	guint64    raster_auto_flags;
	GtkWidget *save_button;

	gboolean        pv_lock;
	gboolean        gr_lock;


protected:
	void read_dsp_probe ();
	void write_dsp_probe (int start, pv_mode pvm);

	void read_dsp_vector (int index);
	void write_dsp_vector (int index);

	void write_dsp_abort_probe ();

private:
	#define MAX_PV 50
	PROBE_VECTOR_GENERIC     dsp_vector_list[MAX_PV]; // copy for GXSM internal use only

        DSP_GUI_Builder *dsp_bp;
        UnitObj *Unity, *Volt, *Angstroem, *Current, *Current_pA, *SetPtUnit, *Speed, *PhiSpeed, *Frq, *Deg, *Vslope, *Time, *msTime, *TimeUms, *minTime, *Hex;

        GtkWidget *dspc_popup_menu;
        GActionGroup *dspc_action_group;

	int multiIV_mode;
	int multiBias_mode;
	#define MAX_DSPC_TABS 20
	int show_tab_mode[MAX_DSPC_TABS];

	GSettings *hwi_settings;
        
        // ==== VP graphs organizer
        Gxsm4appWindow *vpg_app_window;
	GtkWindow* vpg_window;
        GtkWidget* vpg_grid;
        
protected:
        int idle_callback_data_ff;
        guint idle_id;

        gchar *idle_callback_data_fn;
        guint idle_id_update_gui;

	gboolean GUI_ready;
};

class DSPControlUserTabs : public AppBase{
public:
	DSPControlUserTabs(Gxsm4app *app);
	virtual ~DSPControlUserTabs();
};


class MOV_GUI_Builder;

class DSPMoverControl : public AppBase{
public:
	DSPMoverControl(Gxsm4app *app);
	virtual ~DSPMoverControl();

        virtual void AppWindowInit(const gchar *title=NULL, const gchar *sub_title=NULL);
        
	void update();
	void updateDSP(int sliderno=-1);
        void updateAxisCounts (GtkWidget* w, int idx, int cmd);
	static void ExecCmd(int cmd);
	static void ChangedNotify(Param_Control* pcs, gpointer data);
	static void ChangedWaveOut(Param_Control* pcs, gpointer data);
	static int config_waveform (GtkWidget *widget, DSPMoverControl *dspc);
	int configure_waveform (GtkWidget *widget);
        static void wave_preview_draw_function (GtkDrawingArea *area, cairo_t *cr,
                                                    int             width,
                                                    int             height,
                                                    DSPMoverControl *dspc);
	static int config_output (GtkWidget *widget, DSPMoverControl *dspc);
	static int CmdAction(GtkWidget *widget, DSPMoverControl *dspc);

#if 0
        static void direction_button_pressed_cb (GtkGesture *gesture, int n_press, double x, double y, DSPMoverControl *dspc){
                g_message ("PRESSED,  CMD=%d", GPOINTER_TO_INT (g_object_get_data(g_object_get_data( G_OBJECT (gesture), "Button"), "DSP_cmd")));
                dspc->CmdAction (g_object_get_data( G_OBJECT (gesture), "Button"), dspc);
        };
        static void direction_button_stopped_cb (GtkGesture *gesture, DSPMoverControl *dspc){
                g_message ("STOPPED,  CMD=%d", GPOINTER_TO_INT (g_object_get_data(g_object_get_data( G_OBJECT (gesture), "Button"), "DSP_cmd")));
                //dspc->CmdAction (g_object_get_data( G_OBJECT (gesture), "Button"), dspc);
        };
        static void direction_button_released_cb (GtkGesture *gesture, int n_press, double x, double y, DSPMoverControl *dspc){
                g_message ("RELEASED, CMD=%d", GPOINTER_TO_INT (g_object_get_data(g_object_get_data( G_OBJECT (gesture), "Button"), "DSP_cmd")));
                dspc->StopAction (g_object_get_data( G_OBJECT (gesture), "Button"), dspc);
        };
        static void direction_button_clicked_cb (GtkWidget *button, DSPMoverControl *dspc){
                g_message ("CLICKED, CMD=%d", GPOINTER_TO_INT (g_object_get_data(G_OBJECT (button), "DSP_cmd")));
                dspc->StopAction (button, dspc);
        };
#endif
	static int StopAction(GtkWidget *widget, DSPMoverControl *dspc);
        static int RampspeedUpdate(GtkWidget *widget, DSPMoverControl *dspc);
        
	static void configure_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data);

        static void show_tab_to_configure (GtkWidget* w, gpointer data){
                gtk_widget_show (GTK_WIDGET (g_object_get_data (G_OBJECT (w), "TabGrid")));
        };
        static void show_tab_as_configured (GtkWidget* w, gpointer data){
                if (gtk_check_button_get_active (GTK_CHECK_BUTTON (w)))
                        gtk_widget_show (GTK_WIDGET (g_object_get_data (G_OBJECT (w), "TabGrid")));
                else
                        gtk_widget_hide (GTK_WIDGET (g_object_get_data (G_OBJECT (w), "TabGrid")));
        };

	int create_waveform (double amp, double duration, int space=-1);
	Mover_Param mover_param;
	double Z0_speed, Z0_adjust, Z0_goto;

        MOV_GUI_Builder *mov_bp;

        GtkWidget *mc_popup_menu;
        GActionGroup *mc_action_group;
        GtkWidget *mc_rampspeed_label;        

private:
	void create_folder();
	GSettings *hwi_settings;

	UnitObj *Unity, *Hex, *Volt, *Time, *Phase, *Length, *Ang, *Speed;
};


class PAC_GUI_Builder;

class DSPPACControl : public AppBase{
public:
	DSPPACControl(Gxsm4app *app);
	virtual ~DSPPACControl();

	void update();
	void update_readings();
	static guint refresh_readings(DSPPACControl *dspc);


	static void ChangedNotify (Param_Control* pcs, gpointer data);

	static void Changed_Sine (Param_Control* pcs, gpointer data);
	static void Changed_Operation (Param_Control* pcs, gpointer data);
	static void Changed_ControllerAmp (Param_Control* pcs, gpointer data);
	static void Changed_ControllerAmpAutoSet (Param_Control* pcs, gpointer data);
	static void Changed_ControllerPhase (Param_Control* pcs, gpointer data);
	static void Changed_ControllerPhaseAutoSet (Param_Control* pcs, gpointer data);
	static void Changed_TauPAC (Param_Control* pcs, gpointer data);
        static void copy_ref_freq_to_ref_callback (Param_Control* pcs, DSPPACControl *dspc);
	static void SetClipping_callback(GtkWidget* widget, DSPPACControl *dspc);
	static int controller_callback (GtkWidget *widget, DSPPACControl *dspc);

	static int tune_callback (GtkWidget *widget, DSPPACControl *dspc);

	static int config_mode (GtkWidget *widget, DSPPACControl *dspc);
	static int config_output (GtkWidget *widget, DSPPACControl *dspc);
	static int CmdAction(GtkWidget *widget, DSPPACControl *dspc);

	static int choice_PAC_LP_callback (GtkWidget *widget, DSPPACControl *dspc);

	// PLL (generic user level vars, used by MK3Pro/PLL so far)
	PAC_control pll;

private:
	void create_folder();
	guint refresh_timer_id;

        PAC_GUI_Builder *pac_bp;
        
        GtkWidget *pac_popup_menu;
        GActionGroup *pac_action_group;

	UnitObj *Unity, *Hz, *Deg, *VoltDeg, *Volt, *VoltHz, *dB, *mVolt, *Time, *uTime;
};




// --------------- global notebook&tabs management

class DSPControlContainer {
public:
	DSPControlContainer (int default_tabs=0, int debug=0);
	~DSPControlContainer ();

	static int callback_tab_reorder (GtkNotebook *notebook, GtkWidget *child, guint page_num, DSPControlContainer *dspc);
	static int callback_tab_added (GtkNotebook *notebook, GtkWidget *child, guint page_num, DSPControlContainer *dspc);
	static int callback_tab_removed (GtkNotebook *notebook, GtkWidget *child, guint page_num, DSPControlContainer *dspc);


	void realize ();

	// add notebook to list and configure defaults
	void add_notebook (GtkWidget *notebook, Notebook_Window_Name id);

	// add tab to list and configure defaults
	void add_tab (GtkWidget *tab, Notebook_Tab_Name id);

	void populate_tabs ();
	
        void populate_tabs_missing ();

	GtkWidget *DSP_notebook[NOTEBOOK_WINDOW_NUMBER];
	GtkWidget *DSP_notebook_tab[NOTEBOOK_TAB_NUMBER];
	int DSP_notebook_tab_placed[NOTEBOOK_TAB_NUMBER];

	private:
	int freeze_config;
	int dbg;

	GSettings *hwi_settings;
};

#endif

