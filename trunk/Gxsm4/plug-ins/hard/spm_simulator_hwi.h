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


#ifndef __SPM_SIMLATOR_HWI_H
#define __SPM_SIMLATOR_HWI_H


// GUI builder helper
class GUI_Builder : public BuildParam{
public:
        GUI_Builder (GtkWidget *build_grid=NULL, GSList *ec_list_start=NULL, GSList *ec_remote_list=NULL) :
                BuildParam (build_grid, ec_list_start, ec_remote_list) {
                wid = NULL;
                config_checkbutton_list = NULL;
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
                
		MoverCtrl = gtk_label_new (name);
		gtk_widget_show (MoverCtrl);
		gtk_notebook_append_page (GTK_NOTEBOOK (notebook), grid, MoverCtrl);

                g_settings_bind (settings, settings_name,
                                 G_OBJECT (button), "active",
                                 G_SETTINGS_BIND_DEFAULT);
        };

        void notebook_tab_show_all () {
                gtk_widget_show (tg);
        };

        GSList *get_config_checkbutton_list_head () { return config_checkbutton_list; };
        
        GtkWidget *wid;
        GtkWidget *tg;
        GtkWidget *MoverCtrl;
        GSList *config_checkbutton_list;
};


// GUI for hardware/simulator specific controls
class SPM_SIM_Control : public AppBase{
public:
        SPM_SIM_Control(Gxsm4app *app):AppBase(app){
                // need to create according xml recource files for this to make work....
                hwi_settings = g_settings_new (GXSM_RES_BASE_PATH_DOT".hwi.spm-sim-control");
                Unity    = new UnitObj(" "," ");
                Volt     = new UnitObj("V","V");
                Time     = new UnitObj("ms","ms");
                Velocity    = new UnitObj("px/s","px/s");

                speed[0]=speed[1]=2000.0; // per tab
                bias[0]=bias[1]=0.0;
                options = 0x03;
                create_folder ();
        };
	virtual ~SPM_SIM_Control() {
                delete Unity;
                delete Volt;
                delete Time;
                delete Velocity;
        };

        virtual void AppWindowInit(const gchar *title);
	void create_folder();

        static void ChangedNotify(Param_Control* pcs, gpointer data) {;};
        static int config_options_callback (GtkWidget *widget, SPM_SIM_Control *dspc);
        
	//static void ChangedWaveOut(Param_Control* pcs, gpointer data);
	//static int config_waveform (GtkWidget *widget, SPM_SIM_Control *spmsc);
	static void configure_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data);
        static int choice_Ampl_callback(GtkWidget *widget, SPM_SIM_Control *spmsc);

        static void show_tab_to_configure (GtkWidget* w, gpointer data){
                gtk_widget_show (GTK_WIDGET (g_object_get_data (G_OBJECT (w), "TabGrid")));
        };
        static void show_tab_as_configured (GtkWidget* w, gpointer data){
                if (gtk_check_button_get_active (GTK_CHECK_BUTTON (w)))
                        gtk_widget_show (GTK_WIDGET (g_object_get_data (G_OBJECT (w), "TabGrid")));
                else
                        gtk_widget_hide (GTK_WIDGET (g_object_get_data (G_OBJECT (w), "TabGrid")));
        };

        GUI_Builder *bp;

        gint options;
        
        // SPM SIM parameters
        double speed[2]; // per tab
        double bias[2];
        
private:
	GSettings *hwi_settings;

	UnitObj *Unity, *Volt, *Time, *Velocity;
};




/*
 * SPM simulator class -- derived from GXSM XSM_hardware abstraction class
 * =======================================================================
 */
class spm_simulator_hwi_dev : public XSM_Hardware{

public: 
	friend class SPM_SIM_Control;

	spm_simulator_hwi_dev();
	virtual ~spm_simulator_hwi_dev();

	/* Parameter  */
	virtual long GetMaxLines(){ return 32000; };

	virtual const gchar* get_info() { return "SPM Simulator V0.0"; };

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

