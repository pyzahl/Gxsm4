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



#define SERVO_SETPT 0
#define SERVO_CP    1
#define SERVO_CI    2


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


// GUI for hardware template specific controls
class SPM_Template_Control : public AppBase{
public:
        SPM_Template_Control(Gxsm4app *app):AppBase(app){
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

                
                sim_speed[0]=sim_speed[1]=2000.0; // per tab
                sim_bias[0]=sim_bias[1]=0.0;
                options = 0x03;
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

        static int config_options_callback (GtkWidget *widget, SPM_Template_Control *dspc);
        
	//static void ChangedWaveOut(Param_Control* pcs, gpointer data);
	//static int config_waveform (GtkWidget *widget, SPM_Template_Control *spmsc);
	static void configure_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data);
        static int choice_Ampl_callback(GtkWidget *widget, SPM_Template_Control *spmsc);

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

        void update_controller () {}; // dummy, implement control parameter updates

        
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

        
private:
	GSettings *hwi_settings;

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
                

        // sim params and internal data
        double sim_current;
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


private:
	GThread *data_read_thread;
        gboolean KillFlg; 

protected:
};







#endif

