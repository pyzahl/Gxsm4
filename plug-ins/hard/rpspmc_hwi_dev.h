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

#ifndef __RPSPMC_HWI_DEV_H
#define __RPSPMC_HWI_DEV_H

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


#define MAX_PROGRAM_VECTORS 32
#define i_X 0
#define i_Y 1
#define i_Z 2

// forward defs
extern PACPLL_parameters pacpll_parameters;
extern PACPLL_signals pacpll_signals;
extern SPMC_parameters spmc_parameters;
extern SPMC_signals spmc_signals;
extern JSON_parameter PACPLL_JSON_parameters[];
extern JSON_signal PACPLL_JSON_signals[];

class RPSPMC_Control;
class RPspmc_pacpll;

extern RPSPMC_Control *RPSPMC_ControlClass;

extern "C++" {
        extern RPspmc_pacpll *rpspmc_pacpll;
        extern GxsmPlugin rpspmc_pacpll_hwi_pi;
}

#define DMA_SIZE         0x40000            // 20bit count of 32bit words ==> 1MB DMA Block:  2 x 0x80000 bytes
#define EXPAND_MULTIPLES 32

/*
 * RPSPMC hardware interface class -- derived from GXSM XSM_hardware abstraction class
 * =======================================================================
 */

class rpspmc_hwi_dev : public XSM_Hardware, public RP_stream{

public: 
	friend class RPSPMC_Control;

	rpspmc_hwi_dev();
	virtual ~rpspmc_hwi_dev();

        void update_hardware_mapping_to_rpspmc_source_signals ();
        
        static void spmc_stream_connect_cb (GtkWidget *widget, rpspmc_hwi_dev *self);
        virtual const gchar *get_rp_address ();

        static gboolean update_status_idle(gpointer self);
        virtual void status_append (const gchar *msg, bool schedule_from_thread=false);
        virtual void on_connect_actions();
        virtual int on_new_data (gconstpointer contents, gsize len, bool init=false);
        
	/* Parameter  */
	virtual long GetMaxLines(){ return 32000; };
        
	virtual const gchar* get_info() { return info_blob ? info_blob : "RPSPMC 4 GXSM ** not connected: Check Restart or Re-Connect in SPM Control Window, Tab: RedPitaya Web Socket to establish connection."; };
        void info_append (const gchar *s) { if (s) { gchar *ns = g_strdup_printf ("%s%s", info_blob?info_blob:" ", s); g_free (info_blob); info_blob = ns; } else  { g_free (info_blob); info_blob = NULL; }};
        gchar *info_blob;

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

	virtual gint RTQuery () { return RPSPMC_data_y_index + subscan_data_y_index_offset; }; // actual progress on scan -- y-index mirror from FIFO read

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

	void delayed_tip_move_update ();
	static guint delayed_tip_move_update_callback (rpspmc_hwi_dev *dspc);
        gint delayed_tip_move_update_timer_id;
        double requested_tip_move_xy[2];
        
	virtual void StartScan2D() { PauseFlg=0; ScanningFlg=1; KillFlg=FALSE; };
        // EndScan2D() is been called until it returns TRUE from scan control idle task until it returns FALSE (indicating it's completed)
	virtual gboolean EndScan2D() { ScanningFlg=0; GVP_abort_vector_program (); return FALSE; };
	virtual void PauseScan2D()   { PauseFlg=1; };
	virtual void ResumeScan2D()  { PauseFlg=0; };
	virtual void KillScan2D()    { PauseFlg=0; KillFlg=TRUE; };

        // ScanLineM():
        // Scan setup: (yindex=-2),
        // Scan init: (first call with yindex >= 0)
        // while scanning following calls are progress checks (return FALSE when yindex line data transfer is completed to go to next line for checking, else return TRUE to continue with this index!
	virtual gboolean ScanLineM(int yindex, int xdir, int muxmode, //srcs_mask, // muxmode
				   Mem2d *Mob[MAX_SRCS_CHANNELS],
				   int ixy_sub[4]);

        int start_data_read (int y_start, 
                             int num_srcs0, int num_srcs1, int num_srcs2, int num_srcs3, 
                             Mem2d **Mob0, Mem2d **Mob1, Mem2d **Mob2, Mem2d **Mob3);

	virtual int ReadProbeData (int dspdev=0, int control=0);

        int GVP_expect_header(double *pv, int &index_all);
        int GVP_expect_point(double *pv, int &index_all);


        
	int probe_fifo_thread_active;
	int fifo_data_y_index;

	int fifo_data_num_srcs[4]; // 0: XP, 1: XM, 2: 2ND_XP, 3: 2ND_XM
	Mem2d **fifo_data_Mobp[4]; // 0: XP, 1: XM, 2: 2ND_XP, 3: 2ND_XM


        
        // dummy template signal management
	// SIGNAL MANAGEMENT

        void set_spmc_signal_mux (int source[6]);

        int read_GVP_data_block_to_position_vector (int offset, gboolean expect_full_header=false);
       
        int subscan_data_y_index_offset;

   	Mem2d **Mob_dir[4]; // reference to scan memory object (mem2d)
	long srcs_dir[4]; // souce channel coding
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


        gint32 GVP_stream_buffer[EXPAND_MULTIPLES*DMA_SIZE];
        GMutex GVP_stream_buffer_mutex;
        int GVP_stream_buffer_offset;
        int GVP_stream_buffer_AB;
        int GVP_stream_buffer_position;
        int GVP_stream_status;
        
public:
        gint last_vector_index;
 
        //SPM_emulator *spm_emu; // DSP emulator for dummy data generation and minimal SPM behavior
        void GVP_execute_vector_program(); // non blocking, initializes and starts DMA <-> FPGA,stream server and then initiated GVP execution
	void GVP_abort_vector_program (); // stops GVP and DMA streaming

        void GVP_execute_only_vector_program(); // GVP execute only, no read back (data is been ignored)
        void GVP_reset_vector_program (); // reset GVP only, no DMA abort, etc
        void GVP_reset_vector_components (int mask); // reset GVP components my mask

        void GVP_vp_init ();
        void GVP_start_data_read(); // non blocking

        void GVP_reset_UAB ();

	
        PROBE_HEADER_POSITIONVECTOR GVP_vp_header_current;

	int GVP_read_program_vector(int i, PROBE_VECTOR_GENERIC *v){
		if (i >= MAX_PROGRAM_VECTORS || i < 0)
			return 0;
                // NOTE: READINGN BACK NOT YET USED NOR SUPPORTED BY RPSPMC !!!
		// memcpy (v, &vector_program[i], sizeof (PROBE_VECTOR_GENERIC));
		return -1;
	};
	int GVP_write_program_vector(int i, PROBE_VECTOR_GENERIC *v);

        gint RPSPMC_GVP_section_count;
        gint RPSPMC_GVP_n;
        
        gint RPSPMC_GVP_secn;
        gint RPSPMC_data_y_count;
        gint RPSPMC_data_z_value;
        gint RPSPMC_data_y_index;
        gint RPSPMC_data_x_index;

        gboolean abort_GVP_flag;
};

#endif
