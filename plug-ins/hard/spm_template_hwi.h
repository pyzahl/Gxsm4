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


#ifndef __SPM_TEMPLATE_HWI_H
#define __SPM_TEMPLATE_HWI_H

#include "core-source/app_profile.h"

#define REMOTE_PREFIX "dsp-"

#define SERVO_SETPT 0
#define SERVO_CP    1
#define SERVO_CI    2

#define NUM_SIGNALS 99

#define NUM_SIGNALS_UNIVERSAL 200

typedef struct { gint32 id; const gchar* name; guint32 sigptr; } MOD_INPUT;

/**
# * Signal Input ID's for safe DSP based signal pointer adjustment methods
#**/

#define DSP_SIGNAL_MONITOR_INPUT_BASE_ID   0x0000

#define DSP_SIGNAL_BASE_BLOCK_A_ID         0x1000
#define DSP_SIGNAL_Z_SERVO_INPUT_ID       (DSP_SIGNAL_BASE_BLOCK_A_ID+1)
#define DSP_SIGNAL_M_SERVO_INPUT_ID       (DSP_SIGNAL_Z_SERVO_INPUT_ID+1)
#define DSP_SIGNAL_MIXER0_INPUT_ID        (DSP_SIGNAL_M_SERVO_INPUT_ID+1)
#define DSP_SIGNAL_MIXER1_INPUT_ID        (DSP_SIGNAL_MIXER0_INPUT_ID+1)
#define DSP_SIGNAL_MIXER2_INPUT_ID        (DSP_SIGNAL_MIXER1_INPUT_ID+1)
#define DSP_SIGNAL_MIXER3_INPUT_ID        (DSP_SIGNAL_MIXER2_INPUT_ID+1)
#define DSP_SIGNAL_DIFF_IN0_ID            (DSP_SIGNAL_MIXER3_INPUT_ID+1)
#define DSP_SIGNAL_DIFF_IN1_ID            (DSP_SIGNAL_DIFF_IN0_ID+1)
#define DSP_SIGNAL_DIFF_IN2_ID            (DSP_SIGNAL_DIFF_IN1_ID+1)
#define DSP_SIGNAL_DIFF_IN3_ID            (DSP_SIGNAL_DIFF_IN2_ID+1)
#define DSP_SIGNAL_SCAN_CHANNEL_MAP0_ID   (DSP_SIGNAL_DIFF_IN3_ID+1)
#define DSP_SIGNAL_SCAN_CHANNEL_MAP1_ID   (DSP_SIGNAL_SCAN_CHANNEL_MAP0_ID+1)
#define DSP_SIGNAL_SCAN_CHANNEL_MAP2_ID   (DSP_SIGNAL_SCAN_CHANNEL_MAP1_ID+1)
#define DSP_SIGNAL_SCAN_CHANNEL_MAP3_ID   (DSP_SIGNAL_SCAN_CHANNEL_MAP2_ID+1)

#define DSP_SIGNAL_BASE_BLOCK_B_ID         0x2000
#define DSP_SIGNAL_LOCKIN_A_INPUT_ID      (DSP_SIGNAL_BASE_BLOCK_B_ID+1)
#define DSP_SIGNAL_LOCKIN_B_INPUT_ID      (DSP_SIGNAL_LOCKIN_A_INPUT_ID+1)
#define DSP_SIGNAL_VECPROBE0_INPUT_ID     (DSP_SIGNAL_LOCKIN_B_INPUT_ID+1)
#define DSP_SIGNAL_VECPROBE1_INPUT_ID     (DSP_SIGNAL_VECPROBE0_INPUT_ID+1)
#define DSP_SIGNAL_VECPROBE2_INPUT_ID     (DSP_SIGNAL_VECPROBE1_INPUT_ID+1)
#define DSP_SIGNAL_VECPROBE3_INPUT_ID     (DSP_SIGNAL_VECPROBE2_INPUT_ID+1)
#define DSP_SIGNAL_VECPROBE0_CONTROL_ID   (DSP_SIGNAL_VECPROBE3_INPUT_ID+1)
#define DSP_SIGNAL_VECPROBE1_CONTROL_ID   (DSP_SIGNAL_VECPROBE0_CONTROL_ID+1)
#define DSP_SIGNAL_VECPROBE2_CONTROL_ID   (DSP_SIGNAL_VECPROBE1_CONTROL_ID+1)
#define DSP_SIGNAL_VECPROBE3_CONTROL_ID   (DSP_SIGNAL_VECPROBE2_CONTROL_ID+1)
#define DSP_SIGNAL_VECPROBE_TRIGGER_INPUT_ID (DSP_SIGNAL_VECPROBE3_CONTROL_ID+1)
#define DSP_SIGNAL_VECPROBE_LIMITER_INPUT_ID (DSP_SIGNAL_VECPROBE_TRIGGER_INPUT_ID+1)
#define DSP_SIGNAL_VECPROBE_LIMITER_UP_INPUT_ID (DSP_SIGNAL_VECPROBE_LIMITER_INPUT_ID+1)
#define DSP_SIGNAL_VECPROBE_LIMITER_DN_INPUT_ID (DSP_SIGNAL_VECPROBE_LIMITER_UP_INPUT_ID+1)
#define DSP_SIGNAL_VECPROBE_TRACKER_INPUT_ID (DSP_SIGNAL_VECPROBE_LIMITER_DN_INPUT_ID+1)

#define DSP_SIGNAL_BASE_BLOCK_C_ID             0x3000
#define DSP_SIGNAL_OUTMIX_CH0_INPUT_ID        (DSP_SIGNAL_BASE_BLOCK_C_ID+1)
#define DSP_SIGNAL_OUTMIX_CH0_ADD_A_INPUT_ID  (DSP_SIGNAL_OUTMIX_CH0_INPUT_ID+1)
#define DSP_SIGNAL_OUTMIX_CH0_SUB_B_INPUT_ID  (DSP_SIGNAL_OUTMIX_CH0_ADD_A_INPUT_ID+1)
#define DSP_SIGNAL_OUTMIX_CH0_SMAC_A_INPUT_ID (DSP_SIGNAL_OUTMIX_CH0_SUB_B_INPUT_ID+1)
#define DSP_SIGNAL_OUTMIX_CH0_SMAC_B_INPUT_ID (DSP_SIGNAL_OUTMIX_CH0_SMAC_A_INPUT_ID+1)

#define DSP_SIGNAL_OUTMIX_CH1_INPUT_ID        (DSP_SIGNAL_OUTMIX_CH0_SMAC_B_INPUT_ID+1)
#define DSP_SIGNAL_OUTMIX_CH1_ADD_A_INPUT_ID  (DSP_SIGNAL_OUTMIX_CH1_INPUT_ID+1)
#define DSP_SIGNAL_OUTMIX_CH1_SUB_B_INPUT_ID  (DSP_SIGNAL_OUTMIX_CH1_ADD_A_INPUT_ID+1)
#define DSP_SIGNAL_OUTMIX_CH1_SMAC_A_INPUT_ID (DSP_SIGNAL_OUTMIX_CH1_SUB_B_INPUT_ID+1)
#define DSP_SIGNAL_OUTMIX_CH1_SMAC_B_INPUT_ID (DSP_SIGNAL_OUTMIX_CH1_SMAC_A_INPUT_ID+1)

#define DSP_SIGNAL_OUTMIX_CH2_INPUT_ID        (DSP_SIGNAL_OUTMIX_CH1_SMAC_B_INPUT_ID+1)
#define DSP_SIGNAL_OUTMIX_CH2_ADD_A_INPUT_ID  (DSP_SIGNAL_OUTMIX_CH2_INPUT_ID+1)
#define DSP_SIGNAL_OUTMIX_CH2_SUB_B_INPUT_ID  (DSP_SIGNAL_OUTMIX_CH2_ADD_A_INPUT_ID+1)
#define DSP_SIGNAL_OUTMIX_CH2_SMAC_A_INPUT_ID (DSP_SIGNAL_OUTMIX_CH2_SUB_B_INPUT_ID+1)
#define DSP_SIGNAL_OUTMIX_CH2_SMAC_B_INPUT_ID (DSP_SIGNAL_OUTMIX_CH2_SMAC_A_INPUT_ID+1)

#define DSP_SIGNAL_OUTMIX_CH3_INPUT_ID        (DSP_SIGNAL_OUTMIX_CH2_SMAC_B_INPUT_ID+1)
#define DSP_SIGNAL_OUTMIX_CH3_ADD_A_INPUT_ID  (DSP_SIGNAL_OUTMIX_CH3_INPUT_ID+1)
#define DSP_SIGNAL_OUTMIX_CH3_SUB_B_INPUT_ID  (DSP_SIGNAL_OUTMIX_CH3_ADD_A_INPUT_ID+1)
#define DSP_SIGNAL_OUTMIX_CH3_SMAC_A_INPUT_ID (DSP_SIGNAL_OUTMIX_CH3_SUB_B_INPUT_ID+1)
#define DSP_SIGNAL_OUTMIX_CH3_SMAC_B_INPUT_ID (DSP_SIGNAL_OUTMIX_CH3_SMAC_A_INPUT_ID+1)

#define DSP_SIGNAL_OUTMIX_CH4_INPUT_ID        (DSP_SIGNAL_OUTMIX_CH3_SMAC_B_INPUT_ID+1)
#define DSP_SIGNAL_OUTMIX_CH4_ADD_A_INPUT_ID  (DSP_SIGNAL_OUTMIX_CH4_INPUT_ID+1)
#define DSP_SIGNAL_OUTMIX_CH4_SUB_B_INPUT_ID  (DSP_SIGNAL_OUTMIX_CH4_ADD_A_INPUT_ID+1)
#define DSP_SIGNAL_OUTMIX_CH4_SMAC_A_INPUT_ID (DSP_SIGNAL_OUTMIX_CH4_SUB_B_INPUT_ID+1)
#define DSP_SIGNAL_OUTMIX_CH4_SMAC_B_INPUT_ID (DSP_SIGNAL_OUTMIX_CH4_SMAC_A_INPUT_ID+1)

#define DSP_SIGNAL_OUTMIX_CH5_INPUT_ID        (DSP_SIGNAL_OUTMIX_CH4_SMAC_B_INPUT_ID+1)
#define DSP_SIGNAL_OUTMIX_CH5_ADD_A_INPUT_ID  (DSP_SIGNAL_OUTMIX_CH5_INPUT_ID+1)
#define DSP_SIGNAL_OUTMIX_CH5_SUB_B_INPUT_ID  (DSP_SIGNAL_OUTMIX_CH5_ADD_A_INPUT_ID+1)
#define DSP_SIGNAL_OUTMIX_CH5_SMAC_A_INPUT_ID (DSP_SIGNAL_OUTMIX_CH5_SUB_B_INPUT_ID+1)
#define DSP_SIGNAL_OUTMIX_CH5_SMAC_B_INPUT_ID (DSP_SIGNAL_OUTMIX_CH5_SMAC_A_INPUT_ID+1)

#define DSP_SIGNAL_OUTMIX_CH6_INPUT_ID        (DSP_SIGNAL_OUTMIX_CH5_SMAC_B_INPUT_ID+1)
#define DSP_SIGNAL_OUTMIX_CH6_ADD_A_INPUT_ID  (DSP_SIGNAL_OUTMIX_CH6_INPUT_ID+1)
#define DSP_SIGNAL_OUTMIX_CH6_SUB_B_INPUT_ID  (DSP_SIGNAL_OUTMIX_CH6_ADD_A_INPUT_ID+1)
#define DSP_SIGNAL_OUTMIX_CH6_SMAC_A_INPUT_ID (DSP_SIGNAL_OUTMIX_CH6_SUB_B_INPUT_ID+1)
#define DSP_SIGNAL_OUTMIX_CH6_SMAC_B_INPUT_ID (DSP_SIGNAL_OUTMIX_CH6_SMAC_A_INPUT_ID+1)

#define DSP_SIGNAL_OUTMIX_CH7_INPUT_ID        (DSP_SIGNAL_OUTMIX_CH6_SMAC_B_INPUT_ID+1)
#define DSP_SIGNAL_OUTMIX_CH7_ADD_A_INPUT_ID  (DSP_SIGNAL_OUTMIX_CH7_INPUT_ID+1)
#define DSP_SIGNAL_OUTMIX_CH7_SUB_B_INPUT_ID  (DSP_SIGNAL_OUTMIX_CH7_ADD_A_INPUT_ID+1)
#define DSP_SIGNAL_OUTMIX_CH7_SMAC_A_INPUT_ID (DSP_SIGNAL_OUTMIX_CH7_SUB_B_INPUT_ID+1)
#define DSP_SIGNAL_OUTMIX_CH7_SMAC_B_INPUT_ID (DSP_SIGNAL_OUTMIX_CH7_SMAC_A_INPUT_ID+1)

#define DSP_SIGNAL_OUTMIX_CH8_INPUT_ID        (DSP_SIGNAL_OUTMIX_CH7_SMAC_B_INPUT_ID+1)
#define DSP_SIGNAL_OUTMIX_CH8_ADD_A_INPUT_ID  (DSP_SIGNAL_OUTMIX_CH8_INPUT_ID+1)
#define DSP_SIGNAL_OUTMIX_CH9_INPUT_ID        (DSP_SIGNAL_OUTMIX_CH8_ADD_A_INPUT_ID+1)
#define DSP_SIGNAL_OUTMIX_CH9_ADD_A_INPUT_ID  (DSP_SIGNAL_OUTMIX_CH9_INPUT_ID+1)
#define DSP_SIGNAL_OUTMIX_CH10_INPUT_ID       (DSP_SIGNAL_OUTMIX_CH9_ADD_A_INPUT_ID+1)
#define DSP_SIGNAL_OUTMIX_CH10_ADD_A_INPUT_ID (DSP_SIGNAL_OUTMIX_CH10_INPUT_ID+1)
#define DSP_SIGNAL_OUTMIX_CH11_INPUT_ID       (DSP_SIGNAL_OUTMIX_CH10_ADD_A_INPUT_ID+1)
#define DSP_SIGNAL_OUTMIX_CH11_ADD_A_INPUT_ID (DSP_SIGNAL_OUTMIX_CH11_INPUT_ID+1)
#define DSP_SIGNAL_OUTMIX_CH12_INPUT_ID       (DSP_SIGNAL_OUTMIX_CH11_ADD_A_INPUT_ID+1)
#define DSP_SIGNAL_OUTMIX_CH12_ADD_A_INPUT_ID (DSP_SIGNAL_OUTMIX_CH12_INPUT_ID+1)
#define DSP_SIGNAL_OUTMIX_CH13_INPUT_ID       (DSP_SIGNAL_OUTMIX_CH12_ADD_A_INPUT_ID+1)
#define DSP_SIGNAL_OUTMIX_CH13_ADD_A_INPUT_ID (DSP_SIGNAL_OUTMIX_CH13_INPUT_ID+1)

#define DSP_SIGNAL_BASE_BLOCK_D_ID             0x4000
#define DSP_SIGNAL_ANALOG_AVG_INPUT_ID        (DSP_SIGNAL_BASE_BLOCK_D_ID+1)
#define DSP_SIGNAL_SCOPE_SIGNAL1_INPUT_ID     (DSP_SIGNAL_ANALOG_AVG_INPUT_ID+1)
#define DSP_SIGNAL_SCOPE_SIGNAL2_INPUT_ID     (DSP_SIGNAL_SCOPE_SIGNAL1_INPUT_ID+1)
#define DSP_SIGNAL_SCO1_INPUT_ID               (DSP_SIGNAL_SCOPE_SIGNAL2_INPUT_ID+1)
#define DSP_SIGNAL_SCO1_AMPLITUDE_INPUT_ID     (DSP_SIGNAL_SCO1_INPUT_ID+1)
#define DSP_SIGNAL_SCO2_INPUT_ID               (DSP_SIGNAL_SCO1_AMPLITUDE_INPUT_ID+1)
#define DSP_SIGNAL_SCO2_AMPLITUDE_INPUT_ID     (DSP_SIGNAL_SCO2_INPUT_ID+1)

#define LAST_INPUT_ID                          DSP_SIGNAL_SCO2_AMPLITUDE_INPUT_ID

#define SIGNAL_INPUT_DISABLED -10



typedef struct{
	guint32 p;  // pointer in usigned int (32 bit)
	guint32 dim;
	const gchar *label;
	const gchar *unit;
	double scale; // factor to multipy with to get base unit value representation
	const gchar *module;
	int index; // actual vector index
} DSP_SIG_UNIVERSAL;

typedef struct{
	gint32    mindex;           /**< monitor signal index to be set to adress by signald_id -- 0...NUM_MONITOR_SIGNALS-1, or any valid MODULE_SIGNAL_ID.  -1: ready/no action =WO */
	gint32    signal_id;        /**< valid signal identification -- 0...#siganls configured in signal list OR set this to -1 to query signal at mindex  =WO */
	guint32   act_address_input_set;  /**RO (W=0 OK)  verification of actual action taken -- actual input addresss of pointer manipulated =WO */
	guint32   act_address_signal;     /**RO (W=0 OK)  verification of actual signal address configure for input =WO */
} SIGNAL_MANAGE;


typedef enum { PV_MODE_NONE, PV_MODE_IV, PV_MODE_FZ, PV_MODE_PL, PV_MODE_LP, PV_MODE_SP, PV_MODE_TS, PV_MODE_GVP, PV_MODE_AC, PV_MODE_AX, PV_MODE_TK, PV_MODE_ABORT } pv_mode;
typedef enum { MAKE_VEC_FLAG_NORMAL=0, MAKE_VEC_FLAG_VHOLD=1, MAKE_VEC_FLAG_RAMP=2, MAKE_VEC_FLAG_END=4 } make_vector_flags;

#define FLAG_FB_ON       0x01 // FB on
#define FLAG_DUAL        0x02 // Dual Data
#define FLAG_SHOW_RAMP   0x04 // show ramp data
#define FLAG_INTEGRATE   0x08 // integrate and normalize date of all AIC data point inbetween

#define FLAG_AUTO_SAVE   0x01 // auto save
#define FLAG_AUTO_PLOT   0x02 // auto plot
#define FLAG_AUTO_GLOCK  0x04 // auto graph lock
#define FLAG_AUTO_RUN_INITSCRIPT  0x08 // auto run init script


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



class SPM_Template_Control;

// GUI builder helper
class GUI_Builder : public BuildParam{
public:
        GUI_Builder (GtkWidget *build_grid=NULL, GSList *ec_list_start=NULL, GSList *ec_remote_list=NULL) :
                BuildParam (build_grid, ec_list_start, ec_remote_list) {
                wid = NULL;
                config_checkbutton_list = NULL;
                scan_freeze_widget_list = NULL;
        };

        void start_notebook_tab (GtkWidget *notebook, const gchar *name, const gchar *settings_name,
                                 GSettings *settings) {
                new_grid (); tg=grid;
                gtk_widget_set_hexpand (grid, TRUE);
                gtk_widget_set_vexpand (grid, TRUE);
                grid_add_check_button("Configuration: Enable This");
                g_object_set_data (G_OBJECT (button), "TabGrid", grid);
                config_checkbutton_list = g_slist_append (config_checkbutton_list, button);
                configure_list = g_slist_prepend (configure_list, button);

                new_line ();
                
		page = gtk_label_new (name);
		gtk_widget_show (page);
		gtk_notebook_append_page (GTK_NOTEBOOK (notebook), grid, page);

                g_settings_bind (settings, settings_name,
                                 G_OBJECT (button), "active",
                                 G_SETTINGS_BIND_DEFAULT);
        };

        void notebook_tab_show_all () {
                gtk_widget_show (tg);
        };

        GtkWidget* grid_add_mixer_options (gint channel, gint preset, gpointer ref);
        GtkWidget* grid_add_scan_input_signal_options (gint channel, gint preset, gpointer ref);
        GtkWidget* grid_add_probe_source_signal_options (gint channel, gint preset, gpointer ref);

        GtkWidget* grid_add_probe_status (const gchar *status_label);
        void grid_add_probe_controls (gboolean have_dual,
                                      guint64 option_flags, GCallback option_cb,
                                      guint64 auto_flags, GCallback auto_cb,
                                      GCallback exec_cb, GCallback write_cb, GCallback graph_cb, GCallback x_cb,
                                      gpointer cb_data,
                                      const gchar *control_id);
        // tmp use
        void add_to_remote_list (Gtk_EntryControl *ecx, const gchar *rid) {
                remote_list_ec = ecx->AddEntry2RemoteList(rid, remote_list_ec);
        };

        void add_to_scan_freeze_widget_list (GtkWidget *w){
                scan_freeze_widget_list = g_slist_prepend (scan_freeze_widget_list, w);
        };
        static void update_widget (GtkWidget *w, gpointer data){
                gtk_widget_set_sensitive (w, GPOINTER_TO_INT(data));
        };
        void scan_start_gui_actions (){
                g_message ("DSP GUI UDPATE SCAN START");
                g_slist_foreach (scan_freeze_widget_list, (GFunc) GUI_Builder::update_widget, GINT_TO_POINTER(FALSE));
        };
        void scan_end_gui_actions (){
                g_message ("DSP GUI UDPATE SCAN END");
                g_slist_foreach (scan_freeze_widget_list, (GFunc) GUI_Builder::update_widget, GINT_TO_POINTER(TRUE));
        };

        GSList *get_config_checkbutton_list_head () { return config_checkbutton_list; };

        GSList *config_checkbutton_list;
        GtkWidget *scrolled_contents;
	GSList *scan_freeze_widget_list;
        
        GtkWidget *wid;
        GtkWidget *tg;
        GtkWidget *page;
};

#define N_GVP_VECTORS 25 //  vectors max total, need a few extra for controls and finish.

// GUI for hardware template specific controls
class SPM_Template_Control : public AppBase{
public:
        SPM_Template_Control(Gxsm4app *app):AppBase(app){
                vpg_window = NULL;
                vpg_app_window = NULL;
                vpg_grid = NULL;

                // need to create according xml recource files for this to make work....
                hwi_settings = g_settings_new (GXSM_RES_BASE_PATH_DOT".hwi.spm-template-control");
                Unity    = new UnitObj(" "," ");
                Volt     = new UnitObj("V","V");
                Velocity  = new UnitObj("px/s","px/s");

                Angstroem= new UnitObj(UTF8_ANGSTROEM,"A");
                Frq      = new UnitObj("Hz","Hz");
                Time     = new UnitObj("s","s");
                TimeUms  = new LinUnit("ms","ms",1e-3);
                msTime   = new UnitObj("ms","ms");
                minTime  = new UnitObj("min","min");
                Deg      = new UnitObj(UTF8_DEGREE,"Deg");
                // Current  = new UnitAutoMag("A","A"); ((UnitAutoMag*) Current)->set_mag_get_base (1e-9, 1e-9); // nA default and internal "base"
                Current  = new UnitObj("nA","nA");
                Current_pA  = new UnitObj("pA","pA");
                Speed    = new UnitObj(UTF8_ANGSTROEM"/s","A/s");
                PhiSpeed = new UnitObj(UTF8_DEGREE"/s","Deg/s");
                Vslope   = new UnitObj("V/s","V/s");
                Hex      = new UnitObj("h","h");

                bias = 0;
                zpos_ref = 0;

                for (int i=0; i<4; ++i){
                        scan_source[i] = i;
                        probe_source[i] = i;
                        
                        mix_fbsource[i] = 0;
                        mix_unit2volt_factor[i] = 1;
                        mix_set_point[i] = 0;
                        mix_gain[i] = 1;
                        mix_level[i] = 0;
                        mix_transform_mode[i] = 0;
                }

                z_servo[0] = 0;
                z_servo[1] = 0;
                z_servo[2] = 0;

                fast_return = 0;
                x2nd_Zoff = 0;

                ldc_flag = 0;
                mirror_dsp_scan_dx32 = 1;
                mirror_dsp_scan_dy32 = 1;
        
                area_slope_x = 0;
                area_slope_y = 0;
                area_slope_compensation_flag = 0;
                center_return_flag = 0;

                move_speed_x = 500;
                scan_speed_x_requested = 500;
                scan_speed_x = 500;

                ue_bias = 0;
                ue_set_point[0] = 0;
                ue_set_point[1] = 0;
                ue_set_point[2] = 0;
                ue_set_point[3] = 0;
                ue_z_servo[0] = 0;
                ue_z_servo[1] = 0;
                ue_z_servo[2] = 0;
                ue_scan_speed_x_r = 0;
                ue_scan_speed_x = 0;
                ue_slope_x = 0;
                ue_slope_y = 0;
                ue_slope_flg = 0;

                dxdt = 0;
                dydt = 0;
                dzdt = 0;

                
                // LockIn
                AC_amp[0] = 0;
                AC_amp[1] = 0;
                AC_amp[2] = 0;
                AC_amp[3] = 0;
                AC_frq = 330;

	// Graphs Folder -- user settings origin
                Source = XSource = PSource = 0;
                XJoin = GrMatWin = 0;
                PlotAvg = PlotSec = 0;

	// Graphs used life -- dep. on GLOCK if copy of user settings or memorized last setting
                vis_Source = vis_XSource = vis_XJoin = vis_PSource = 0;
                vis_PlotAvg = vis_PlotSec = 0;


	// STS (I-V)
                IV_start = -1, IV_end = 1, IV_slope = 0.1, IV_slope_ramp = 0.5, IV_final_delay=0.1, IV_recover_delay=0.1;
                IV_points = 2000;
                IV_repetitions = 1;

                for (int i=0; i<N_GVP_VECTORS; ++i){
                        GVP_du[i] = 0;
                        GVP_dx[i] = 0;
                        GVP_dy[i] = 0;
                        GVP_dz[i] = 0;
                        GVP_dsig[i] = 0;
                        GVP_ts[i] = 0;
                        GVP_points[i] = 0;
                        GVP_opt[i] = 0;
                        GVP_data[i] = 0;
                        GVP_vnrep[i] = 0;
                        GVP_vpcjr[i] = 0;
                        GVP_final_delay = 0.0;
                }
                GVP_repetitions = 0;
                
                // init all vars with last used values is done via dconf / schemata
                // -- BUT not at very first generation via auto write schemata, will get random memory eventually to edit manually later....
                
                sim_speed[0]=sim_speed[1]=2000.0; // per tab
                sim_bias[0]=sim_bias[1]=0.0;
                options = 0x03;

                get_tab_settings ("IV", IV_option_flags, IV_auto_flags, IV_glock_data);
                get_tab_settings ("VP", GVP_option_flags, GVP_auto_flags, GVP_glock_data);
                GVP_restore_vp ("VP_set_last"); // last in view

                
                create_folder ();
        };
	virtual ~SPM_Template_Control() {
                delete Unity;
                delete Volt;
                delete Velocity;

                delete Angstroem;
                delete Frq;
                delete Time;
                delete TimeUms;
                delete msTime;
                delete minTime;
                delete Deg;
                delete Current;
                delete Current_pA;
                delete Speed;
                delete PhiSpeed;
                delete Vslope;
                delete Hex;
        };

        virtual void AppWindowInit(const gchar *title);
	void create_folder();

	void get_tab_settings (const gchar *tab_key, guint64 &option_flags, guint64 &AC_auto_flags, guint64 glock_data[6]);
        void set_tab_settings (const gchar *tab_key, guint64 option_flags, guint64 AC_auto_flags, guint64 glock_data[6]);

	void save_values (NcFile *ncf);
        void load_values (NcFile *ncf);

        void store_values ();

        static int config_options_callback (GtkWidget *widget, SPM_Template_Control *dspc);
        
	//static void ChangedWaveOut(Param_Control* pcs, gpointer data);
	//static int config_waveform (GtkWidget *widget, SPM_Template_Control *spmsc);
	static void configure_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data);
        static int choice_Ampl_callback(GtkWidget *widget, SPM_Template_Control *spmsc);
        static int choice_scansource_callback (GtkWidget *widget, SPM_Template_Control *dspc);

	static int DSP_cret_callback (GtkWidget *widget, SPM_Template_Control *dspc);
	static int DSP_slope_callback (GtkWidget *widget, SPM_Template_Control *dspc);

        static int ldc_callback(GtkWidget *widget, SPM_Template_Control *dspc);

        static void lockin_adjust_callback(Param_Control* pcs, gpointer data);
        static int lockin_runfree_callback(GtkWidget *widget, SPM_Template_Control *dspc);

        static void show_tab_to_configure (GtkWidget* w, gpointer data){
                gtk_widget_show (GTK_WIDGET (g_object_get_data (G_OBJECT (w), "TabGrid")));
        };
        static void show_tab_as_configured (GtkWidget* w, gpointer data){
                if (gtk_check_button_get_active (GTK_CHECK_BUTTON (w)))
                        gtk_widget_show (GTK_WIDGET (g_object_get_data (G_OBJECT (w), "TabGrid")));
                else
                        gtk_widget_hide (GTK_WIDGET (g_object_get_data (G_OBJECT (w), "TabGrid")));
        };



        static void ChangedNotify(Param_Control* pcs, gpointer data);
        static int ChangedAction(GtkWidget *widget, SPM_Template_Control *dspc);
	void update_zpos_readings ();
	static guint refresh_zpos_readings(SPM_Template_Control *dspc);
        static int zpos_monitor_callback(GtkWidget *widget, SPM_Template_Control *dspc);
        static int choice_mixmode_callback (GtkWidget *widget, SPM_Template_Control *dspc);
        static int choice_prbsource_callback(GtkWidget *widget, SPM_Template_Control *dspc);

        static int auto_probe_callback(GtkWidget *widget, SPM_Template_Control *dspc);
	static int Probing_graph_callback(GtkWidget *widget, SPM_Template_Control *dspc, int finish_flag=0);
        static int Probing_event_setup_scan (int ch, const gchar *titleprefix, const gchar *name, const gchar *unit, const gchar *label, double d2u, int nvalues);
	static int Probing_eventcheck_callback(GtkWidget *widget, SPM_Template_Control *dspc);
	static int Probing_exec_ABORT_callback(GtkWidget *widget, SPM_Template_Control *dspc);
	static int Probing_save_callback(GtkWidget *widget, SPM_Template_Control *dspc);
	static int Probing_abort_callback(GtkWidget *widget, SPM_Template_Control *dspc);

	static int Probing_exec_IV_callback(GtkWidget *widget, SPM_Template_Control *dspc);
	static int Probing_write_IV_callback(GtkWidget *widget, SPM_Template_Control *dspc);
	static int callback_change_IV_option_flags (GtkWidget *widget, SPM_Template_Control *dspc);
	static int callback_change_IV_auto_flags (GtkWidget *widget, SPM_Template_Control *dspc);

	static int Probing_exec_GVP_callback(GtkWidget *widget, SPM_Template_Control *dspc);
	static int Probing_write_GVP_callback(GtkWidget *widget, SPM_Template_Control *dspc);
        
        static int callback_change_GVP_vpc_option_flags (GtkWidget *widget, SPM_Template_Control *dspc);
	static int callback_update_GVP_vpc_option_checkbox (GtkWidget *widget, SPM_Template_Control *dspc);
	static int callback_change_GVP_option_flags (GtkWidget *widget, SPM_Template_Control *dspc);
	static int callback_change_GVP_auto_flags (GtkWidget *widget, SPM_Template_Control *dspc);
	static int callback_edit_GVP (GtkWidget *widget, SPM_Template_Control *dspc);
	static int callback_GVP_store_vp (GtkWidget *widget, SPM_Template_Control *dspc);
	static int callback_GVP_restore_vp (GtkWidget *widget, SPM_Template_Control *dspc);

        static int change_source_callback (GtkWidget *widget, SPM_Template_Control *dspc);
	static int callback_XJoin (GtkWidget *widget, SPM_Template_Control *dspc);
	static int callback_GrMatWindow (GtkWidget *widget, SPM_Template_Control *dspc);

#if 0
	const char* vp_label_lookup(int i);
	const char* vp_unit_lookup(int i);
	double      vp_scale_lookup(int i);

	void update_sourcesignals_from_DSP_callback ();
#endif

	static gboolean idle_callback (gpointer data){
		SPM_Template_Control *dspc = (SPM_Template_Control*) data;

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
                        idle_id = g_idle_add (SPM_Template_Control::idle_callback, this);
                        if (finish_flag){
                                g_message ("Probing_graph_update_thread_safe: plot update. Finished. Current data index=%d",
                                           current_probe_data_index);
                                last_index = 0;
                        }
                } else {
                        // g_warning ("Probing_graph_update_thread_safe: is busy, skipping. [%d]", count);
                }
	};



	//int check_vp_in_progress (const gchar *extra_info=NULL);



        
	int probedata_length () { return current_probe_data_index; };
	void push_probedata_arrays ();
	GArray** pop_probedata_arrays ();
	GArray** pop_probehdr_arrays ();
	void init_probedata_arrays ();
	static void free_probedata_array_set (GArray** garr, SPM_Template_Control *dc);
	static void free_probehdr_array_set (GArray** garr, SPM_Template_Control *dc);
	void free_probedata_arrays ();

	int check_vp_in_progress (const gchar *extra_info=NULL) { return 0; }; // DUMMY -- check controller for !!

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


        
   	const char* vp_label_lookup(int i);
	const char* vp_unit_lookup(int i);
	double      vp_scale_lookup(int i);

        void update_GUI(); // update GUI
           
	static gboolean idle_callback_update_gui (gpointer data){
		SPM_Template_Control *dspc = (SPM_Template_Control*) data;
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
                        idle_id_update_gui = g_idle_add (SPM_Template_Control::idle_callback_update_gui, this);
                } else {
                        g_warning ("update_gui_thread_safe: is busy, skipping [%s].", fntmp);
                }
	};

        void update_controller () {

                mirror_dsp_scan_dx32 = 1.0; // dsp_scan_fs_dx*dsp_scan_dnx; // actual DSP dx in S15.16 between pixels in X
                mirror_dsp_scan_dy32 = 1.0; // dsp_scan_fs_dy*dsp_scan_dny; // actual DSP dy in S15.16 between pixels in Y

        }; // dummy, implement control parameter updates

        
        GUI_Builder *bp;

        // Simultaor only
        double sim_bias[2];  // per tab -- simulation
        double sim_speed[2]; // per tab -- simulation
        gint options;
        
        // SPM parameters

  	double bias;           //!< STM Bias (usually applied to the sample)

	Gtk_EntryControl *ZPos_ec;
   	double zpos_ref;
	gint   zpos_refresh_timer_id;
	  
	// -- FEEDBACK MIXER --
	int    mix_fbsource[4];   // only for documentation
	double mix_unit2volt_factor[4]; // on default: [0] Current Setpoint (STM; log mode) [1..3]: off
	double mix_set_point[4]; // on default: [0] Current Setpoint (STM; log mode) [1..3]: off
	double mix_gain[4];      // Mixing Gains: 0=OFF, 1=100%, note: can be > 1 or even negative for scaling purposes
	double mix_level[4];     // fuzzy mixing control via level, applied only if fuzzy flag set
	// -- may not yet be applied to all signal --> check with DSP code
	int    mix_transform_mode[4]; //!< transformation mode on/off log/lin iir/full fuzzy/normal

        // Feedback (Z-Servo)
	double z_servo[3];    // Z-Servo (Feedback) [0] (not used here), [1] Const Proportional, [2] Const Integral [user visible values]

	int    scan_source[4];    // scan source mapping signal index for imaging
	int    probe_source[4];   // probe source mapping signal index for 32bit data channels [0..3]

	int    vp_input_id_cache[4];  // cache VP input config;
	int    DSP_vpdata_ij[2];
	GtkWidget *VPSig_menu, *VPSig_mi[8], *VPScanSrcVPitem[4];
        
	double fast_return;       //!< on-the-fly fast return option (scan retrace speed override factor, 1=normal)
	double x2nd_Zoff;         //!< Z lift off for 2nd scan line (MFM etc...)

	int    ldc_flag;          //! LDC status at last update
	GtkWidget *LDC_status;    //!< linear drift correction flag (on/off)

        // -- DSP Scan DNX,DNY mirror
        gint32 mirror_dsp_scan_dx32, mirror_dsp_scan_dy32;

        
        // Scan Slope Compensation Parameters
        double area_slope_x;      //!< slope compensation in X, in scan coordinate system (possibly rotated) -- applied before, but by feedback
	double area_slope_y;      //!< slope compensation in Y, in scan coordinate system (possibly rotated) -- applied before, but by feedback
	Gtk_EntryControl *slope_x_ec;
	Gtk_EntryControl *slope_y_ec;
	int    area_slope_compensation_flag; //!< enable/disable slope compensation
	int    center_return_flag; //!< enable/disable return to center after scan

        // Move and Scan Speed
        double move_speed_x;      //!< in DAC (AIC) units per second, GXSM core computes from A/s using X-gain and Ang/DAC...
	double scan_speed_x_requested;      //!< in DAC (AIC) units per second - requested
	double scan_speed_x;      //!< in DAC (AIC) units per second - best match
	Gtk_EntryControl *scan_speed_ec;

        double dxdt, dydt, dzdt;

        
	// UserEvent sensitive:
	double ue_bias;
	double ue_set_point[4];
	double ue_z_servo[3];
	double ue_scan_speed_x_r;
	double ue_scan_speed_x;
	double ue_slope_x;
	double ue_slope_y;
	double ue_slope_flg;

	// LockIn
	double    AC_amp[4], AC_frq, AC_phaseA, AC_phaseB;
	Gtk_EntryControl *AC_frq_ec;
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
	double IV_start, IV_end, IV_slope, IV_slope_ramp, IV_final_delay, IV_recover_delay;
	int    IV_points;
	int    IV_repetitions;
	guint64 IV_option_flags;
	guint64 IV_auto_flags;
	GtkWidget *IV_status;
	guint64 IV_glock_data[6];


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

        int idle_callback_data_ff;
        guint idle_id;

        gchar *idle_callback_data_fn;
        guint idle_id_update_gui;

	gboolean GUI_ready;

private:
	#define MAX_PV 50
	PROBE_VECTOR_GENERIC     dsp_vector_list[MAX_PV]; // copy for GXSM internal use only

	GSettings *hwi_settings;

        pv_mode write_vector_mode;
        
        // ==== VP graphs organizer
        Gxsm4appWindow *vpg_app_window;
	GtkWindow* vpg_window;
        GtkWidget* vpg_grid;

	UnitObj *Unity, *Volt, *Velocity;
        UnitObj *Angstroem, *Frq, *Time, *TimeUms, *msTime, *minTime, *Deg, *Current, *Current_pA, *Speed, *PhiSpeed, *Vslope, *Hex;
};




/*
 * SPM simulator class -- derived from GXSM XSM_hardware abstraction class
 * =======================================================================
 */
class spm_template_hwi_dev : public XSM_Hardware{

public: 
	friend class SPM_Template_Control;

	spm_template_hwi_dev();
	virtual ~spm_template_hwi_dev();

	/* Parameter  */
	virtual long GetMaxLines(){ return 32000; };

	virtual const gchar* get_info() { return "SPM Template V0.0"; };

	/* Hardware realtime monitoring -- all optional */
	/* default properties are
	 * "X" -> current realtime tip position in X, inclusive rotation and offset
	 * "Y" -> current realtime tip position in Y, inclusive rotation and offset
	 * "Z" -> current realtime tip position in Z
	 * "xy" -> X and Y
	 * "zxy" -> Z, X, Y  in Volts!
         * "O" -> Offset -> z0, x0, y0 (Volts)
         * "f0I" -> 
	 * "U" -> current bias
         * "W" -> WatchDog
	 */
	virtual gint RTQuery (const gchar *property, double &val1, double &val2, double &val3);

	virtual gint RTQuery () { return data_y_index + subscan_data_y_index_offset; }; // actual progress on scan -- y-index mirror from FIFO read

	/* high level calls for instrtument condition checks */
	virtual gint RTQuery_clear_to_start_scan (){ return 1; };
	virtual gint RTQuery_clear_to_start_probe (){ return 1; };
	virtual gint RTQuery_clear_to_move_tip (){ return 1; };

	int is_scanning() { return ScanningFlg; };

	virtual double GetUserParam (gint n, gchar *id=NULL) { return 0.; };
	virtual gint   SetUserParam (gint n, gchar *id=NULL, double value=0.) { return 0; };
	
	virtual double GetScanrate () { return 1.; }; // query current set scan rate in s/pixel

	virtual int RotateStepwise(int exec=1); // rotation not implemented in simulation, if required set/update scan angle here

	virtual gboolean SetOffset(double x, double y); // set offset to coordinated (non rotated)
	virtual gboolean MovetoXY (double x, double y); // set tip position in scan coordinate system (potentially rotated)

	virtual void StartScan2D() { PauseFlg=0; ScanningFlg=1; KillFlg=FALSE; };
        // EndScan2D() is been called until it returns TRUE from scan control idle task until it returns FALSE (indicating it's completed)
	virtual gboolean EndScan2D() { ScanningFlg=0; return FALSE; };
	virtual void PauseScan2D()   { PauseFlg=1; };
	virtual void ResumeScan2D()  { PauseFlg=0; };
	virtual void KillScan2D()    { PauseFlg=0; KillFlg=TRUE; };

        // ScanLineM():
        // Scan setup: (yindex=-2),
        // Scan init: (first call with yindex >= 0)
        // while scanning following calls are progress checks (return FALSE when yindex line data transfer is completed to go to next line for checking, else return TRUE to continue with this index!
	virtual gboolean ScanLineM(int yindex, int xdir, int muxmode,
				   Mem2d *Mob[MAX_SRCS_CHANNELS],
				   int ixy_sub[4]);

        double simulate_value (int xi, int yi, int ch); // simulate data at x,y,ch function

        int start_data_read (int y_start, 
                             int num_srcs0, int num_srcs1, int num_srcs2, int num_srcs3, 
                             Mem2d **Mob0, Mem2d **Mob1, Mem2d **Mob2, Mem2d **Mob3);

	virtual int ReadProbeData (int dspdev=0, int control=0);

	int probe_fifo_thread_active;
	int fifo_data_y_index;

	int fifo_data_num_srcs[4]; // 0: XP, 1: XM, 2: 2ND_XP, 3: 2ND_XM
	Mem2d **fifo_data_Mobp[4]; // 0: XP, 1: XM, 2: 2ND_XP, 3: 2ND_XM


        
        // dummy template signal management
	// SIGNAL MANAGEMENT

	virtual void read_dsp_signals () { read_signal_lookup (); read_actual_module_configuration (); };

	virtual int lookup_signal_by_ptr(gint64 sigptr);
	virtual int lookup_signal_by_name(const gchar *sig_name);
	virtual const gchar *lookup_signal_name_by_index(int i);
	virtual const gchar *lookup_signal_unit_by_index(int i);
	virtual double lookup_signal_scale_by_index(int i);
	virtual int change_signal_input(int signal_index, gint32 input_id, gint32 voffset=0);
	virtual int query_module_signal_input(gint32 input_id);
	int read_signal_lookup ();
	int read_actual_module_configuration ();

	DSP_SIG_UNIVERSAL *lookup_dsp_signal_managed(gint i){
                if (i<NUM_SIGNALS_UNIVERSAL)
                        return &dsp_signal_lookup_managed[i];
                else
                        return NULL;
	};

       

        // sim params and internal data
        double sim_bias;
        double sim_current;
        double sim_z;
        double data_z_value;
        double x0,y0; // offset

        // scan engine
        int data_y_count;
        int data_y_index;
        int data_x_index;
        int subscan_data_y_index_offset;
   	Mem2d **Mob_dir[4]; // reference to scan memory object (mem2d)
	long srcs_dir[4]; // souce cahnnel coding
	int nsrcs_dir[4]; // number of source channes active
        gint ScanningFlg;
        gint PauseFlg;

protected:
	int thread_sim; // connection to SRanger used by thread
        DSP_SIG_UNIVERSAL dsp_signal_lookup_managed[NUM_SIGNALS_UNIVERSAL]; // signals, generic version

private:
	GThread *data_read_thread;
	GThread *probe_data_read_thread;
        gboolean KillFlg; 

protected:
};







#endif

