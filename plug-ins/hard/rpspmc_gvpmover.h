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

#ifndef __RPSPMC_GVPMOVER_H
#define __RPSPMC_GVPMOVER_H

#include "core-source/app_profile.h"

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


typedef enum {
	GVP_CMD_Z0_STOP,
	GVP_CMD_Z0_P,
	GVP_CMD_Z0_M,
	GVP_CMD_Z0_AUTO,
	GVP_CMD_Z0_CENTER,
	GVP_CMD_Z0_GOTO,
	GVP_CMD_AFM_MOV_XM,
	GVP_CMD_AFM_MOV_XP,
	GVP_CMD_AFM_MOV_YM,
	GVP_CMD_AFM_MOV_YP,
	GVP_CMD_AFM_MOV_ZM,
	GVP_CMD_AFM_MOV_ZP,
	GVP_CMD_APPROCH_MOV_XP,
	GVP_CMD_APPROCH,
	GVP_CMD_CLR_PA
} GVP_Mover_Command;
	
typedef struct{
	/* Mover Control */
	double MOV_Ampl, MOV_Speed, MOV_Steps;
	int MOV_GPIO_setting; /* Parameter Memory for different Mover/Slider Modes */
	
#define GVP_AFMMOV_MODES 6
	double AFM_Amp, AFM_Speed, AFM_Steps;  /* STM/AFM Besocke Kontrolle -- current */
	int    AFM_GPIO_setting; /* Parameter Memory for different Mover/Slider Modes */
	double AFM_usrAmp[GVP_AFMMOV_MODES];  
	double AFM_usrSpeed[GVP_AFMMOV_MODES];
	double AFM_usrSteps[GVP_AFMMOV_MODES]; /* Parameter Memory for different Mover/Slider Modes */
	int    AFM_GPIO_usr_setting[GVP_AFMMOV_MODES]; /* Parameter Memory for different Mover/Slider Modes */

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
        int axis_counts[GVP_AFMMOV_MODES][3];
        int axis_counts_ref[GVP_AFMMOV_MODES][3];
} Mover_Param;


class GVP_MOV_GUI_Builder;

class GVPMoverControl : public AppBase{
public:
	GVPMoverControl(Gxsm4app *app);
	virtual ~GVPMoverControl();

        virtual void AppWindowInit(const gchar *title=NULL, const gchar *sub_title=NULL);
        
	void update();
	void updateDSP(int sliderno=-1);
        void updateAxisCounts (GtkWidget* w, int idx, int cmd);
	static void ExecCmd(int cmd);
	static void ChangedNotify(Param_Control* pcs, gpointer data);
	static void ChangedWaveOut(Param_Control* pcs, gpointer data);
	static int config_waveform (GtkWidget *widget, GVPMoverControl *dspc);
	int configure_waveform (GtkWidget *widget);
        static void wave_preview_draw_function (GtkDrawingArea *area, cairo_t *cr,
                                                    int             width,
                                                    int             height,
                                                    GVPMoverControl *dspc);
	static int config_output (GtkWidget *widget, GVPMoverControl *dspc);
	static int CmdAction(GtkWidget *widget, GVPMoverControl *dspc);

#if 0
        static void direction_button_pressed_cb (GtkGesture *gesture, int n_press, double x, double y, GVPMoverControl *dspc){
                g_message ("PRESSED,  CMD=%d", GPOINTER_TO_INT (g_object_get_data(g_object_get_data( G_OBJECT (gesture), "Button"), "GVP_cmd")));
                dspc->CmdAction (g_object_get_data( G_OBJECT (gesture), "Button"), dspc);
        };
        static void direction_button_stopped_cb (GtkGesture *gesture, GVPMoverControl *dspc){
                g_message ("STOPPED,  CMD=%d", GPOINTER_TO_INT (g_object_get_data(g_object_get_data( G_OBJECT (gesture), "Button"), "GVP_cmd")));
                //dspc->CmdAction (g_object_get_data( G_OBJECT (gesture), "Button"), dspc);
        };
        static void direction_button_released_cb (GtkGesture *gesture, int n_press, double x, double y, GVPMoverControl *dspc){
                g_message ("RELEASED, CMD=%d", GPOINTER_TO_INT (g_object_get_data(g_object_get_data( G_OBJECT (gesture), "Button"), "GVP_cmd")));
                dspc->StopAction (g_object_get_data( G_OBJECT (gesture), "Button"), dspc);
        };
        static void direction_button_clicked_cb (GtkWidget *button, GVPMoverControl *dspc){
                g_message ("CLICKED, CMD=%d", GPOINTER_TO_INT (g_object_get_data(G_OBJECT (button), "GVP_cmd")));
                dspc->StopAction (button, dspc);
        };
#endif
	static int StopAction(GtkWidget *widget, GVPMoverControl *dspc);
        static int RampspeedUpdate(GtkWidget *widget, GVPMoverControl *dspc);
        
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

        GVP_MOV_GUI_Builder *mov_bp;

        GtkWidget *mc_popup_menu;
        GActionGroup *mc_action_group;
        GtkWidget *mc_rampspeed_label;        

private:
	void create_folder();
	GSettings *hwi_settings;

	UnitObj *Unity, *Hex, *Volt, *Time, *Phase, *Length, *Ang, *Speed;
};


#endif

