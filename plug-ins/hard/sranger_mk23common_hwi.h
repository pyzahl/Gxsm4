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

#ifndef __SRANGER_MK23COMMON_HWI_H
#define __SRANGER_MK23COMMON_HWI_H

#include "sranger_mk2_hwi_control.h"

#define SR_HWI_TARGET_NONE 0
#define SR_HWI_TARGET_C54  1
#define SR_HWI_TARGET_C55  2

#define BZ_MAX_CHANNELS 16

enum PLL_GROUP { PLL_SYMBOLS=1,
		 PLL_SINE=2,
		 PLL_TAUPAC=4,
		 PLL_OPERATION=0x20,
		 PLL_READINGS=0x40,
		 PLL_CONTROLLER_PHASE=0x100,
		 PLL_CONTROLLER_PHASE_AUTO=0x200,
		 PLL_CONTROLLER_PHASE_AUTO_SET=0x300,
		 PLL_CONTROLLER_AMPLITUDE=0x1000,
		 PLL_CONTROLLER_AMPLITUDE_AUTO=0x2000,
		 PLL_CONTROLLER_AMPLITUDE_AUTO_SET=0x3000,
		 PLL_ALL=0xffffff,
		 PLL_NONE=0
};

#define NUM_SIGNALS_UNIVERSAL 200

typedef struct{
	guint32 p;  // pointer in usigned int (32 bit)
	guint32 dim;
	const gchar *label;
	const gchar *unit;
	double scale; // factor to multipy with to get base unit value representation
	const gchar *module;
	int index; // actual vector index
} DSP_SIG_UNIVERSAL;

/*
 * general hardware abstraction class for Signal Ranger compatible hardware
 * - instrument independent device support part
 * =================================================================
 */


// SRanger type DSP common HwI device abstraction level
class sranger_common_hwi_dev : public XSM_Hardware{

public: 
	friend class DSPControl;

	sranger_common_hwi_dev() { sranger_mark_id = -2; }; // pure virtual
	virtual ~sranger_common_hwi_dev() {};

	virtual int update_gxsm_configurations () { return 0; };

	/* Parameter  */
	virtual long GetMaxPointsPerLine(){ return 0; };
	virtual long GetMaxLines(){ return 0; };

	virtual int ReadScanData(int y_index, int num_srcs, Mem2d *m[MAX_SRCS_CHANNELS]) { return 0; }; // done via thread.
	virtual int ReadProbeData(int nsrcs, int nprobe, int kx, int ky, Mem2d *m, double scale=1.) { return 0; };	// obsolete since VP

	virtual const gchar* get_info() { return "*Virtual*"; };

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
	virtual gint RTQuery (const gchar *property, int n, gfloat *data) { return FALSE; };

	/* high level calls for instrtument condition checks */
	virtual gint RTQuery_clear_to_start_scan (){ return 1; };
	virtual gint RTQuery_clear_to_start_probe (){ return 1; };
	virtual gint RTQuery_clear_to_move_tip (){ return 1; };

	virtual double GetUserParam (gint n, gchar *id=NULL) { return 0.; };
	virtual gint   SetUserParam (gint n, gchar *id=NULL, double value=0.) { return 0; };
	
	virtual double GetScanrate () { return scanpixelrate; }; // query current set scan rate in s/pixel

	virtual int RotateStepwise(int exec=1) { return 0; };

	virtual void ExecCmd(int Cmd) {};
	virtual void SetMode(int mode) {};
	virtual void ClrMode(int mode) {};

	virtual void set_ldc (double dxdt=0., double dydt=0., double dzdt=0.) {};

	int is_scanning() { return ScanningFlg; };
	virtual	int start_fifo_read (int y_start, 
                                     int num_srcs0, int num_srcs1, int num_srcs2, int num_srcs3, 
                                     Mem2d **Mob0, Mem2d **Mob1, Mem2d **Mob2, Mem2d **Mob3) { return 0; };

	virtual	int ReadLineFromFifo (int y_index) { return 0; };

	virtual int ReadProbeFifo (int dspdev, int control=0) { return 0; };

	// ========================================
	virtual void read_dsp_state (gint32 &mode) {}; // FB on/off
	virtual void write_dsp_state (gint32 mode) {};

	virtual void write_dsp_reset () {};
	
	virtual double read_dsp_feedback (const gchar *property=NULL, int index=-1) { return 0.; }; // query feedback parameters
	virtual void write_dsp_feedback ( 
		      double set_point[4], double factor[4], double gain[4], double level[4], int transform_mode[4],
		      double IIR_I_crossover, double IIR_f0_max[4], double IIR_f0_min, double LOG_I_offset, int IIR_flag,
		      double setpoint_zpos, double z_servo[3], double m_servo[3], double pllref=0.) {};
	
	virtual void read_dsp_analog () {}; // bias
	virtual void write_dsp_analog (double bias[4], double motor) {};

	virtual void read_dsp_scan (gint32 &pflg) {};
	virtual void write_dsp_scan () {};
	virtual void recalculate_dsp_scan_speed_parameters () {};
	virtual void recalculate_dsp_scan_slope_parameters () {};
 	
	virtual void read_dsp_lockin (double AC_amp[4], double &AC_frq, double &AC_phaseA, double &AC_phaseB, gint32 &AC_lockin_avg_cycels) {};
	virtual void write_dsp_lockin_probe_final (double AC_amp[4], double &AC_frq, double AC_phaseA, double AC_phaseB,
						   gint32 AC_lockin_avg_cycels, double VP_lim_val[2], double noise_amp, int start=0) {};
	virtual int dsp_lockin_state(int set=-1){return 0;};

	virtual void read_dsp_vector (int index, PROBE_VECTOR_GENERIC *__dsp_vector) {};
	virtual void write_dsp_vector (int index, PROBE_VECTOR_GENERIC *__dsp_vector) {};
	virtual void write_dsp_abort_probe () {};
	
	virtual int check_pac () { return -1; }; // returns 0 if PAC/PLL capability available, else -1
	virtual int read_pll (PAC_control &pll, PLL_GROUP group=PLL_ALL) { return -1; };
	virtual int write_pll (PAC_control &pll, PLL_GROUP group, int enable=0) { return -1; };

	virtual int read_pll_signal1 (gfloat *signal, int n, double scale=1., gint flag=FALSE){ return 0; };
	virtual int read_pll_signal2 (gfloat *signal, int n, double scale=1., gint flag=FALSE){ return 0; };
	virtual int read_pll_signal1dec (gfloat *signal, int n, double scale=1.){ return 0; };
	virtual int read_pll_signal2dec (gfloat *signal, int n, double scale=1.){ return 0; };
	virtual void set_blcklen (int len){};
	virtual void set_scope (int s1, int s2){};
	virtual void setup_step_response (double dPhase, double dAmplitude){};

	virtual void read_dsp_signals () { dsp_signal_lookup_managed[0].p = 0; };
	virtual int lookup_signal_by_ptr(gint64 sigptr) { return -1; };
	virtual int lookup_signal_by_name(const gchar *sig_name) { return -1; };
	virtual const gchar *lookup_signal_name_by_index(int i) { return "NOT SUPPORTED"; };
	virtual const gchar *lookup_signal_unit_by_index(int i) { return "NOT SUPPORTED"; };
	virtual double lookup_signal_scale_by_index(int i) { return 0.; };
	virtual int change_signal_input(int signal_index, gint32 input_id, gint32 voffset=0) { return -10; };
	virtual int query_module_signal_input(gint32 input_id) { return -10; };

	// ========================================

	int get_mark_id () { return sranger_mark_id; };

	DSP_SIG_UNIVERSAL *lookup_dsp_signal_managed(gint i){
                if (i<NUM_SIGNALS_UNIVERSAL)
                        return &dsp_signal_lookup_managed[i];
                else
                        return NULL;
	};
	
protected:
	int ScanningFlg;
	int KillFlg;
	DSP_SIG_UNIVERSAL dsp_signal_lookup_managed[NUM_SIGNALS_UNIVERSAL]; // signals, generic version

protected:
	int sranger_mark_id;
public:
	int probe_time_estimate;
	int bz_statistics[5];
	int fifo_block;
        double scanpixelrate;
};

#endif
