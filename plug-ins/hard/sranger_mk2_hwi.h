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

#ifndef __SRANGER_MK2_HWI_H
#define __SRANGER_MK2_HWI_H

#include "MK2-A810_spmcontrol/FB_spm_dataexchange.h" // SRanger data exchange structs and consts
#include "sranger_mk23common_hwi.h"

typedef struct{
	DSP_UINT    r_position;	  /* read pos (Gxsm) (always in word size) (WO) by host =WO */
	DSP_UINT    w_position;   /* write pos (DSP) (always in word size) (RO) by host =RO */
	DSP_UINT    current_section_head_position; /* resync/verify and status info =RO */
	DSP_UINT    current_section_index; /* index of current section =RO */
	DSP_UINT    fill;	      /* filled indicator = max num to read =RO */
	DSP_UINT    stall;	      /* number of fifo stalls =RO */
	DSP_UINT    length;	      /* constant, buffer size =RO */
	DSP_UINT    p_buffer_base; /* pointer to external memory, buffer start =RO */
	DSP_UINT    p_buffer_w;    /* pointer to external memory =RO */
	DSP_UINT    p_buffer_l;
} DATA_FIFO_EXTERN_PCOPY;

typedef struct{
	DSP_LONG srcs;
	DSP_LONG n;
	DSP_LONG time;
	DSP_INT  x_offset;
	DSP_INT  y_offset;
	DSP_INT  z_offset;
	DSP_INT  x_scan;
	DSP_INT  y_scan;
	DSP_INT  z_scan;
	DSP_INT  u_scan;
	DSP_INT  section;
} PROBE_SECTION_HEADER;

// reference
extern DSPControl *DSPControlClass;

// MK2 translation interface 
class sranger_mk2_hwi_dev : public sranger_common_hwi_dev{

public: 
	friend class DSPControl;

	sranger_mk2_hwi_dev();
	virtual ~sranger_mk2_hwi_dev();

        virtual gboolean dsp_device_status() { return false; };
  
	virtual int update_gxsm_configurations (){
		// update resources from signals
		DSPControlClass->update_sourcesignals_from_DSP_callback ();
		return 0; 
	}; /* called after GUI build complete */

	/* Parameter  */
	virtual long GetMaxPointsPerLine(){ return  AIC_max_points; };
	virtual long GetMaxLines(){ return  AIC_max_points; };

	virtual const gchar* get_info();

	/* Hardware realtime monitoring -- all optional */
	/* default properties are
	 * "X" -> current realtime tip position in X, inclusive rotation and offset
	 * "Y" -> current realtime tip position in Y, inclusive rotation and offset
	 * "Z" -> current realtime tip position in Z
	 * "xy" -> X and Y
	 * "zxy" -> Z, X, Y
	 * "U" -> current bias
	 */
	virtual gint RTQuery (const gchar *property, double &val) { return FALSE; };
	virtual gint RTQuery (const gchar *property, double &val1, double &val2) { return FALSE; };
	virtual gint RTQuery (const gchar *property, double &val1, double &val2, double &val3) { return FALSE; };
	virtual gint RTQuery (const gchar *property, gchar **val) { return FALSE; };

	virtual gint RTQuery () { int yi=0; g_mutex_lock (&RTQmutex); yi=fifo_data_y_index + subscan_data_y_index_offset; g_mutex_unlock (&RTQmutex); return yi; }; // actual progress on scan -- y-index mirror from FIFO read

	virtual double GetUserParam (gint n, gchar *id=NULL);
	virtual gint   SetUserParam (gint n, gchar *id=NULL, double value=0.);
	

	virtual void ExecCmd(int Cmd) {};
	virtual void SetMode(int mode) {};
	virtual void ClrMode(int mode) {};

	virtual void set_ldc (double dxdt=0., double dydt=0., double dzdt=0.) {};


// SRanger format conversions and endianess adaptions
	void swap (guint16 *addr);
	void swap (gint16  *addr);
	void swap (gint32 *addr);
	void swap (guint32 *addr);
	void check_and_swap (gint16 &data) {
		if (swap_flg)
			swap (&data);
	};
	void check_and_swap (guint16 &data) {
		if (swap_flg)
			swap (&data);
	};
	void check_and_swap (gint32 &data) {
		if (swap_flg){
			swap (&data);
			return;
		}

		if (target == SR_HWI_TARGET_C55){
			gint32 x, temp0, temp1, temp2, temp3;
			gchar  x32[4];
		
			x = data;
			temp0 = (x)       & 0xFF;
			temp1 = (x >> 8)  & 0xFF;
			temp2 = (x >> 16) & 0xFF;
			temp3 = (x >> 24) & 0xFF;

			x32[0] = temp2;
			x32[1] = temp3;
			x32[2] = temp0;
			x32[3] = temp1;

			data = *((gint32*)x32);
		}
	};
	gint32 float_2_sranger_q15 (double x);
	gint32 float_2_sranger_q31 (double x);
	gint32 int_2_sranger_int (gint32 x);
	guint16 uint_2_sranger_uint (guint16 x);
	gint32 long_2_sranger_long (gint32 x);
	guint32 ulong_2_sranger_ulong (guint32 x);

	virtual int start_fifo_read (int y_start, 
			     int num_srcs0, int num_srcs1, int num_srcs2, int num_srcs3, 
			     Mem2d **Mob0, Mem2d **Mob1, Mem2d **Mob2, Mem2d **Mob3);

	virtual int ReadLineFromFifo (int y_index);

	virtual int ReadProbeFifo (int dspdev, int control=0);

	int probe_fifo_thread_active;
	int fifo_data_y_index;
	int subscan_data_y_index_offset;
	int fifo_data_num_srcs[4]; // 0: XP, 1: XM, 2: 2ND_XP, 3: 2ND_XM
	Mem2d **fifo_data_Mobp[4]; // 0: XP, 1: XM, 2: 2ND_XP, 3: 2ND_XM

	int probe_thread_dsp; // connection to SRanger used by probe thread

	inline void sr_read (int fh, void *d, size_t n){
		ssize_t ret;
		if ((ret = read (fh, d, n)) < 0){
			gchar *details = g_strdup_printf ("ret=%d", (int)ret);
			gapp->alert (N_("DSP read error"), N_("Error reading data from DSP."), details, 1);
			g_free (details);
		}
	};
	inline void sr_write (int fh, const void *d, size_t n){
		ssize_t ret;
		if ((ret = write (fh, d, n)) < 0){
			gchar *details = g_strdup_printf ("ret=%d",(int)ret);
			gapp->alert (N_("DSP read error"), N_("Error reading data from DSP."), details, 1);
			g_free (details);
		}
	};

	virtual void read_dsp_state (gint32 &mode); // FB on/off
	virtual void write_dsp_state (gint32 mode);
	
	virtual void write_dsp_reset ();
	
	void conv_dsp_feedback ();
	virtual double read_dsp_feedback (const gchar *property=NULL, int index=-1); // query feedback parameters
	virtual void write_dsp_feedback (
		 double set_point[4], double factor[4], double gain[4], double level[4], int transform_mode[4],
		 double IIR_I_crossover, double IIR_f0_max[4], double IIR_f0_min, double LOG_I_offset, int IIR_flag,
		 double setpoint_zpos, double z_servo[3], double m_servo[3], double pllref);
	
	void conv_dsp_analog ();
	virtual void read_dsp_analog (); // bias
	virtual void write_dsp_analog (double bias[4], double motor) ;

	void conv_dsp_scan ();
	virtual void read_dsp_scan (gint32 &pflg);
	virtual void write_dsp_scan () ;
	virtual void recalculate_dsp_scan_speed_parameters ();
	virtual void recalculate_dsp_scan_slope_parameters ();

	void conv_dsp_probe ();
	virtual void read_dsp_lockin (double *AC_amp, double &AC_frq, double &AC_phaseA, double &AC_phaseB, gint32 &AC_lockin_avg_cycels);
	virtual int dsp_lockin_state (int set=-1);

	virtual void write_dsp_lockin_probe_final (double AC_amp[4], double &AC_frq, double AC_phaseA, double AC_phaseB, gint32 AC_lockin_avg_cycels, double VP_lim_val[2], double noise_amp, int start=0);

	void conv_dsp_vector ();
	virtual void read_dsp_vector (int index, PROBE_VECTOR_GENERIC *__dsp_vector);
	virtual void write_dsp_vector (int index, PROBE_VECTOR_GENERIC *__dsp_vector);
	virtual void write_dsp_abort_probe ();

	virtual int read_pll_signal1 (gfloat *signal, int n, double scale=1., gint flag=FALSE){return 0;};
	virtual int read_pll_signal2 (gfloat *signal, int n, double scale=1., gint flag=FALSE){return 0;};
 
	void set_scope (int s1, int s2){};
	void set_blcklen (int len=1024){};
	void setup_step_response (double dPhase, double dAmplitude){};

protected:
	int dsp; // connection to SRanger
	int dsp_alternative; // 2nd connection to SRanger
	int thread_dsp; // connection to SRanger used by thread
	// add data shared w upper spm class here
	SPM_MAGIC_DATA_LOCATIONS magic_data;
	int target;
	int swap_flg;

private:
        GMutex RTQmutex;
	GThread *fifo_read_thread;
	GThread *probe_fifo_read_thread;
	int FifoRead (int start, int end, int &xi, int num_srcs, int len, float *buffer_f, SHT *fifo_w, LNG *fifo_l);
	LNG bz_unpack (int i, LNG *fifo_l, int &q, int &count);
	gchar *productid;
	int AIC_max_points;
	gint32 bz_last[BZ_MAX_CHANNELS];
	int bz_byte_pos;
	int bz_need_reswap;
	double bz_scale;
	gint32 bz_dsp_srcs;
	gint32 bz_dsp_ns;
	guint32 bz_push_mode;

protected:
	gint32 tmp_nx_pre;
	gint32 tmp_fs_dx;
	gint32 tmp_fs_dy;
	gint32 tmp_dnx;
	gint32 tmp_dny;
	gint32 tmp_fast_return;
	gint32 tmp_sRe;
	gint32 tmp_sIm;

	SPM_STATEMACHINE dsp_state;
	SPM_PI_FEEDBACK  dsp_feedback;
	FEEDBACK_MIXER   dsp_feedback_mixer;
	ANALOG_VALUES    dsp_analog;
	AREA_SCAN        dsp_scan;
	DATA_SYNC_IO     dsp_data_sync_io;
	PROBE            dsp_probe;
	DATA_FIFO        dsp_fifo;
	CR_OUT_PULSE     dsp_cr_out_pulse;  // IO puls engine
	CR_GENERIC_IO    dsp_cr_generic_io; // IO and Counter control
	PROBE_VECTOR     dsp_vector;
};


/*
 * SPM hardware abstraction class for use with Signal Ranger compatible hardware
 * ======================================================================
 */
class sranger_mk2_hwi_spm : public sranger_mk2_hwi_dev{
 public:
	sranger_mk2_hwi_spm();
	virtual ~sranger_mk2_hwi_spm();
        virtual gboolean dsp_device_status() { return dsp ? true:false; };

	/* Hardware realtime monitoring -- all optional */
	/* default properties are
	 * "X" -> current realtime tip position in X, inclusive rotation and offset
	 * "Y" -> current realtime tip position in Y, inclusive rotation and offset
	 * "Z" -> current realtime tip position in Z
	 * "xy" -> X and Y
	 * "zxy" -> Z, X, Y
	 * "U" -> current bias
	 */
	virtual gint RTQuery (const gchar *property, double &val) { return FALSE; };
	virtual gint RTQuery (const gchar *property, double &val1, double &val2) { return FALSE; };
	virtual gint RTQuery (const gchar *property, double &val1, double &val2, double &val3);
	virtual gint RTQuery (const gchar *property, gchar **val) { return FALSE; };

	virtual int RotateStepwise(int exec=0);
	virtual void UpdateScanGainMirror ();
	virtual gboolean SetOffset(double x, double y);
	virtual gboolean MovetoXY (double x, double y);
	virtual void StartScan2D();
	virtual gboolean ScanLineM(int yindex, int xdir, int muxmode,
				   Mem2d *Mob[MAX_SRCS_CHANNELS],
				   int ixy_sub[4]);
	virtual gboolean EndScan2D();
	virtual void PauseScan2D();
	virtual void ResumeScan2D();
	virtual void KillScan2D(){ KillFlg=TRUE; };

	virtual void ExecCmd(int Cmd);
	virtual void SetMode(int mode);
	virtual void ClrMode(int mode);

	virtual void set_ldc (double dxdt=0., double dydt=0., double dzdt=0.);

	void reset_scandata_fifo (int stall=0); //!< reset scan data FIFO buffer
	gboolean tip_to_origin (double x=0., double y=0.); //!< move tip to origin (default) or position in scan coordinate system

 protected:

 private:
	DSPControl *dc;
	double tip_pos[2]; // manual tip positioning memory;
};


#endif

