/* SRanger and Gxsm - Gnome X Scanning Microscopy Project
 * universal STM/AFM/SARLS/SPALEED/... controlling and
 * data analysis software
 *
 * DSP tools for Linux
 *
 * Copyright (C) 1999,2000,2001,2002 Percy Zahl
 *
 * Authors: Percy Zahl <zahl@users.sf.net>
 * WWW Home:
 * DSP part:  http://sranger.sf.net
 * Gxsm part: http://gxsm.sf.net
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

/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 8 c-style: "K&R" -*- */

#include "FB_spm_dataexchange.h"
#include "dataprocess.h"

/* compensate for X AIC pipe delay and add user "nx_pre" amount */
#define PIPE_LEN (25+scan.nx_pre)

#define Z_DATA_BUFFER_ADDRESS 0x5000
#define Z_DATA_BUFFER_SIZE    0x4000
#define Z_DATA_BUFFER_MASK    0x3fff

#define EN_PROBE_TRIGGER      0x0008

extern SPM_PI_FEEDBACK  feedback;
extern ANALOG_VALUES    analog;
extern AREA_SCAN        scan;
extern PROBE            probe;
extern DATA_FIFO        datafifo;
extern DATA_FIFO        probe_datafifo;
extern CR_GENERIC_IO    CR_generic_io;
extern SCAN_EVENT_TRIGGER scan_event_trigger;

#define PRB_SE_DELAY       8        /* Start/End Delay -- must match !!*/

int AS_ip, AS_jp, AS_kp, AS_mod;
int AS_ch2nd_scan_switch; /* const.height 2nd scan mode flag X+/-, holds next mode to switch to 2nd scanline */
int AS_ch2nd_constheight_enabled; /* const H mode flg */

long AS_AIC_data_sum[9];
int  AS_AIC_num_samples;

int *Z_data_buffer = (int*) Z_DATA_BUFFER_ADDRESS;

/* Scan Control States */
#define AS_READY    0
#define AS_MOVE_XY  1
#define AS_SCAN_XP  2
#define AS_SCAN_YP  3
#define AS_SCAN_XM  4
#define AS_SCAN_YM  5
#define AS_SCAN_2ND_XP  6
#define AS_SCAN_2ND_XM  7

// disable 2nd scan line stuff
// #define ENABLE_ZDATABUFFER_2ND_SCAN

void clear_summing_data (){
	AS_AIC_data_sum[0] = AS_AIC_data_sum[1] = AS_AIC_data_sum[2] =
	AS_AIC_data_sum[3] = AS_AIC_data_sum[4] = AS_AIC_data_sum[5] =
	AS_AIC_data_sum[6] = AS_AIC_data_sum[7] = AS_AIC_data_sum[8] = 0;
	AS_AIC_num_samples = 0;
}

/* calc of f_dx/y and num_steps by host! */
void init_area_scan (){
	// wait until fifo read thread on host is ready
	if (!datafifo.stall) {
		// reset FIFO -- done by host

		init_probe_fifo (); // reset probe fifo once at start!

		// now starting...
		scan.start = 0;
		scan.stop  = 0;
				
		// calculate num steps to go to starting point -- done by host

		// init probe trigger count (-1 will disable it)
		AS_mod = scan.dnx_probe;
		if (AS_mod > 0){
			AS_ip = 1;
			AS_jp = 0;
			AS_kp = 0;
		} else {
			AS_jp = -1;
		}
		AS_ch2nd_scan_switch = AS_SCAN_YP; // just go to next line by default
		AS_ch2nd_constheight_enabled = 0;  // disable first
					
		// 2nd stage line scan, memorized Z OK -- here we have a size limit due to the buffer used! ?
		if (scan.srcs_2nd_xp || scan.srcs_2nd_xm)
			if (scan.nx < Z_DATA_BUFFER_SIZE)
				AS_ch2nd_scan_switch = AS_SCAN_2ND_XP; // enable 2nd scan line mode

		if ((scan.srcs_xp & 0x07000) || (scan.srcs_xm & 0x07000))  // setup LockIn job
			init_lockin_for_bgjob ();


		if ((scan.srcs_xp & 0x08000) || (scan.srcs_xm & 0x08000)){ // if Counter channel requested, restart counter/timer now
			setup_counter();
			restart_counter();
		}

		// enable subtask
		scan.sstate = AS_MOVE_XY;
		scan.pflg  = 1;
	}
}


void integrate_as_data_srcs (int srcs){
#ifdef DSP_AS_AIC_AVG
	if (srcs & 0x01) // PIDSrcA1 (Dest) --> Zmonitor (Topo) feedback generated Z signal
		AS_AIC_data_sum[8] += -feedback.z;

	if (srcs & 0x0010) // DataSrcA1 --> AIC5 <-- I (current), ...
		AS_AIC_data_sum[5] += iobuf.min5;

	if (srcs & 0x0020) // DataSrcA2 --> AIC0
		AS_AIC_data_sum[0] += iobuf.min0;

	if (srcs & 0x0040) // DataSrcA3 --> AIC1
		AS_AIC_data_sum[1] += iobuf.min1;

	if (srcs & 0x0080) // DataSrcA4 -->  AIC2
		AS_AIC_data_sum[2] += iobuf.min2;

	if (srcs & 0x0100) // DataSrcB1 --> AIC3
		AS_AIC_data_sum[3] += iobuf.min3;

	if (srcs & 0x0200) // DataSrcB2 -->  AIC4
		AS_AIC_data_sum[4] += iobuf.min4;

	if (srcs & 0x0400) // DataSrcB3 -->  AIC6
		AS_AIC_data_sum[6] += iobuf.min6;

	if (srcs & 0x0800) // DataSrcB4 -->  AIC7
		AS_AIC_data_sum[7] += iobuf.min7;

	++AS_AIC_num_samples;
#endif
}

void push_area_scan_data (int srcs){
	int lp;
#ifdef DSP_AS_AIC_AVG
	if (srcs & 0x0001){ // PIDSrc1/Dest <-- Z = -AIC_Z-value 
		datafifo.buffer.w[datafifo.w_position++] = (int)(AS_AIC_data_sum[8]/AS_AIC_num_samples);
		datafifo.w_position &= DATAFIFO_MASK;
	}
	if (srcs & 0x0010){ // DataSrcA1 --> AIC5 <-- I current [STM/log-fb] / normal Force [AFM/lin-fb]
		datafifo.buffer.w[datafifo.w_position++] = (int)(AS_AIC_data_sum[5]/AS_AIC_num_samples);
		datafifo.w_position &= DATAFIFO_MASK;
	}
	if (srcs & 0x0020){ // DataSrcA2 --> AIC0
		datafifo.buffer.w[datafifo.w_position++] = (int)(AS_AIC_data_sum[0]/AS_AIC_num_samples);
		datafifo.w_position &= DATAFIFO_MASK;
	}
	if (srcs & 0x0040){ // DataSrcA3 --> AIC1
		datafifo.buffer.w[datafifo.w_position++] = (int)(AS_AIC_data_sum[1]/AS_AIC_num_samples);
		datafifo.w_position &= DATAFIFO_MASK;
	}
	if (srcs & 0x0080){ // DataSrcA4 --> AIC2
		datafifo.buffer.w[datafifo.w_position++] = (int)(AS_AIC_data_sum[2]/AS_AIC_num_samples);
		datafifo.w_position &= DATAFIFO_MASK;
	}
	if (srcs & 0x0100){ // DataSrcB1 --> AIC3
		datafifo.buffer.w[datafifo.w_position++] = (int)(AS_AIC_data_sum[3]/AS_AIC_num_samples);
		datafifo.w_position &= DATAFIFO_MASK;
	}
	if (srcs & 0x0200){ // DataSrcB2 --> AIC4
		datafifo.buffer.w[datafifo.w_position++] = (int)(AS_AIC_data_sum[4]/AS_AIC_num_samples);
		datafifo.w_position &= DATAFIFO_MASK;
	}
	if (srcs & 0x0400){ // DataSrcB3 --> AIC6
		datafifo.buffer.w[datafifo.w_position++] = (int)(AS_AIC_data_sum[6]/AS_AIC_num_samples);
		datafifo.w_position &= DATAFIFO_MASK;
	}
	if (srcs & 0x0800){ // DataSrcB4 --> AIC7
		datafifo.buffer.w[datafifo.w_position++] = (int)(AS_AIC_data_sum[7]/AS_AIC_num_samples);
		datafifo.w_position &= DATAFIFO_MASK;
	}
	// auto clear now
	clear_summing_data ();

#else

	if (srcs & 0x0001){ // PIDSrc/Dest <-- Z = -AIC_Z-value
		datafifo.buffer.w[datafifo.w_position++] = -feedback.z;
		datafifo.w_position &= DATAFIFO_MASK;
	}
	if (srcs & 0x0010){ // DataSrcA1 --> AIC5 <-- I current [STM/log-fb] / normal Force [AFM/lin-fb]
		datafifo.buffer.w[datafifo.w_position++] = iobuf.min5;
		datafifo.w_position &= DATAFIFO_MASK;
	}
	if (srcs & 0x0020){ // DataSrcA2 -->  AIC0
		datafifo.buffer.w[datafifo.w_position++] = iobuf.min0;
		datafifo.w_position &= DATAFIFO_MASK;
	}
	if (srcs & 0x0040){ // DataSrcA3 --> AIC1
		datafifo.buffer.w[datafifo.w_position++] = iobuf.min1;
		datafifo.w_position &= DATAFIFO_MASK;
	}
	if (srcs & 0x0080){ // DataSrcA4 --> AIC2
		datafifo.buffer.w[datafifo.w_position++] = iobuf.min2;
		datafifo.w_position &= DATAFIFO_MASK;
	}
	if (srcs & 0x0100){ // DataSrcB1 --> AIC3
		datafifo.buffer.w[datafifo.w_position++] = iobuf.min3;
		datafifo.w_position &= DATAFIFO_MASK;
	}
	if (srcs & 0x0200){ // DataSrcB2 -->  AIC4
		datafifo.buffer.w[datafifo.w_position++] = iobuf.min4;
		datafifo.w_position &= DATAFIFO_MASK;
	}
	if (srcs & 0x0400){ // DataSrcB3 -->  AIC6
		datafifo.buffer.w[datafifo.w_position++] = iobuf.min6;
		datafifo.w_position &= DATAFIFO_MASK;
	}
	if (srcs & 0x0800){ //  DataSrcB4 --> AIC7
		datafifo.buffer.w[datafifo.w_position++] = iobuf.min7;
		datafifo.w_position &= DATAFIFO_MASK;
	}
#endif

	// adjust long ptr to even word address for 32bit data sources?
	if (srcs & 0x0F000){
		if (datafifo.w_position&1){
			datafifo.w_position++;
			datafifo.w_position &= DATAFIFO_MASK;
		}
		lp = datafifo.w_position >> 1;

		// 32bit data chunks from here
		if (srcs & 0x01000){ // DataSrcC1 --> LockIn1stA
			datafifo.buffer.l[lp] = probe.LockIn_1stA;
			datafifo.w_position += 2;
			datafifo.w_position &= DATAFIFO_MASK;
			lp = datafifo.w_position >> 1;
		}
		if (srcs & 0x02000){ // DataSrcD1 --> LockIn2ndA
			datafifo.buffer.l[lp] = probe.LockIn_2ndA;
			datafifo.w_position += 2;
			datafifo.w_position &= DATAFIFO_MASK;
			lp = datafifo.w_position >> 1;
		}
		if (srcs & 0x04000){ // DataSrcE1 --> LockIn0
			datafifo.buffer.l[lp] = _lssub (feedback.i_pl_sum, feedback.i_sum); // probe.LockIn_0; 
			datafifo.w_position += 2;
			datafifo.w_position &= DATAFIFO_MASK;
			lp = datafifo.w_position >> 1;
		}
		if (srcs & 0x08000){ // "DataSrcF1" last CR Counter count
			update_count (); // read, reset and restart counter
			datafifo.buffer.l[lp] = CR_generic_io.count;
			datafifo.w_position += 2;
			datafifo.w_position &= DATAFIFO_MASK;
			lp = datafifo.w_position >> 1;
		}
	}
}

void check_scan_event_trigger (short trig_i[8], short bias_setpoint[8]){
	int i;
	for (i=0; i<8; ++i)
		if (scan.ix == trig_i[i]){
			if (i<4)
				analog.bias = bias_setpoint [i];
			else
				feedback.setpoint = bias_setpoint [i];
		}
}

void run_area_scan (){
	if ((scan.srcs_xp & 0x07000) || (scan.srcs_xm & 0x07000)){
		// run LockIn
		probe.ix = -PRB_SE_DELAY - 2;
		run_lockin ();
	}

	switch (scan.sstate){
	case AS_SCAN_XP: // "X+" -> scan and dataaq.
		integrate_as_data_srcs (scan.srcs_xp);
//		scan.Xpos += scan.fs_dx;
		scan.Xpos = _lsadd (scan.Xpos, scan.cfs_dx); // this is with SAT!!
		feedback.i_pl_sum = _lsadd (feedback.i_pl_sum, scan.fm_dzx); // apply slope compensation
		feedback.i_sum    = _lsadd (feedback.i_sum, scan.fm_dzx); // keep mirror of integrator without slope compensation (plain I)
		if (!scan.iix--){
			if (scan.ix--){
				if (AS_ip >= 0 && (AS_jp == 0 || scan.raster_a) && (scan.srcs_xp &  EN_PROBE_TRIGGER)){
					if (! --AS_ip){ // trigger probing process ?
						if (!probe.pflg) // assure last prb job is done!!
							init_probe ();
						AS_ip = scan.dnx_probe;
					}
				}
				if (scan_event_trigger.pflg)
					check_scan_event_trigger (scan_event_trigger.trig_i_xp, scan_event_trigger.xp_bias_setpoint);

				scan.iix    = scan.dnx-1;
				
				if (AS_ch2nd_constheight_enabled){
					iobuf.mout5 = _sadd (Z_data_buffer [scan.ix], scan.Zoff_2nd_xp) & 0xfffe; // use memorized ZA profile + offset
					push_area_scan_data (scan.srcs_2nd_xp); // get 2nd XP data here!
				}else{
#ifdef ENABLE_ZDATABUFFER_2ND_SCAN
					Z_data_buffer [scan.ix & Z_DATA_BUFFER_MASK] = feedback.z; // memorize Z profile
#endif
					push_area_scan_data (scan.srcs_xp); // get XP data here!
				}
			}
			else{
				scan.ix  = scan.nx;
				scan.iix = scan.dnx + PIPE_LEN;
				if (AS_ch2nd_constheight_enabled)
					scan.sstate = AS_SCAN_2ND_XM;
				else
					scan.sstate = AS_SCAN_XM;
				clear_summing_data ();
			}
			scan.cfs_dx = scan.fs_dx;
		}
		break;

	case AS_SCAN_XM: // "X-" <- scan and dataaq.
		integrate_as_data_srcs (scan.srcs_xm);
//		scan.Xpos -= scan.fs_dx;
		scan.Xpos = _lssub (scan.Xpos, scan.cfs_dx);
		feedback.i_pl_sum = _lssub (feedback.i_pl_sum, scan.fm_dzx); // apply slope compensation
		feedback.i_sum    = _lssub (feedback.i_sum, scan.fm_dzx); // keep mirror of integrator without slope compensation (plain I)
		if (!scan.iix--){
			if (scan.ix--){
				if (AS_ip >= 0 && AS_jp == 0 && (scan.srcs_xm & EN_PROBE_TRIGGER)){
					if (! --AS_ip){ // trigger probing process ?
						if (!probe.pflg) // assure last prb job is done!!
							init_probe ();
						AS_ip = scan.dnx_probe;
					}
				}
				if (scan_event_trigger.pflg)
					check_scan_event_trigger (scan_event_trigger.trig_i_xm, scan_event_trigger.xm_bias_setpoint);

				scan.iix   = scan.dnx-1;
				
				if (AS_ch2nd_constheight_enabled){
					iobuf.mout5 = _sadd (Z_data_buffer [scan.nx-scan.ix-1], scan.Zoff_2nd_xm) & 0xfffe; // use memorized ZA profile + offset
					push_area_scan_data (scan.srcs_2nd_xm); // get 2nd XP data here!
				}else{
					push_area_scan_data (scan.srcs_xm); // get XM data here!
				}
			}
			else{
				if (!scan.iy){ // area scan done?
					scan.sstate = AS_READY;
					scan.pflg = 0;
					break;
				}
				scan.iiy = scan.dny;
				scan.cfs_dy = scan.fs_dy;
				if (AS_ch2nd_constheight_enabled) {
					scan.sstate = AS_SCAN_YP;
					AS_ch2nd_constheight_enabled = 0;  // disable now
				}else
					scan.sstate = AS_ch2nd_scan_switch; // switch to next YP or 2nd stage scan line
			}
			scan.cfs_dx = scan.fs_dx;
		}
		break;

	case AS_SCAN_2ND_XP: // configure 2nd XP scan
		AS_ch2nd_constheight_enabled = 1;  // enable now
		// disable feedback!!
		// config normal XP again
		scan.ix  = scan.nx;
		scan.iix = scan.dnx + PIPE_LEN;
		scan.cfs_dx = scan.fs_dx;
		scan.sstate = AS_SCAN_XP;
		clear_summing_data ();
		break;
	case AS_SCAN_2ND_XM: // configure 2nd XP scan
		AS_ch2nd_constheight_enabled = 1;  // enable now
		// config normal XM again
		scan.ix  = scan.nx;
		scan.iix = scan.dnx + PIPE_LEN;
		scan.cfs_dx = scan.fs_dx;
		scan.sstate = AS_SCAN_XM;
		clear_summing_data ();
		break;
	case AS_SCAN_YP: // "Y+" next line (could be Y-up or Y-dn, dep. on sign. of fs_dy!)
		if (scan.iiy--){
//						scan.Ypos -= scan.fs_dy;
			scan.Ypos = _lssub (scan.Ypos, scan.cfs_dy);
			feedback.i_pl_sum = _lssub (feedback.i_pl_sum, scan.fm_dzy); // apply slope compensation
			feedback.i_sum    = _lssub (feedback.i_sum, scan.fm_dzy); // keep mirror without slope compensation
		}
		else{
			if (scan.iy--){
				if (AS_jp >= 0){
					if (scan.raster_a){
						AS_ip = AS_kp;
						if ((++AS_jp) & 1){ // set start if X grid counter
							if (++AS_kp >= AS_mod)
								AS_kp=0;
							AS_ip += scan.raster_a;
						}
						if (AS_ip++ >= AS_mod)
							AS_ip -= AS_mod;
					} else
						if (++AS_jp == scan.dnx_probe){
							AS_jp = 0;
							AS_ip = scan.dnx_probe; // reset X grid counter
						}
				}
				scan.ix  = scan.nx;
				scan.iix = scan.dnx + PIPE_LEN;
				scan.sstate = AS_SCAN_XP;
				clear_summing_data ();
			}
			else{ // area scan is done -- double check here.
				scan.sstate = AS_READY;
				scan.pflg = 0;
			}
			scan.cfs_dx = scan.fs_dx;
		}
		break;

	case AS_SCAN_YM: // "Y-" -- not valid yet --
		scan.pflg = 0; // cancel for now
		break;

	case AS_MOVE_XY: // move-XY: init/finalize scan, move to start/end of scan position, 
		// can be used with nx==0, then the job is finished after the xy move!!
		if (scan.num_steps_move_xy){
//			scan.Xpos += scan.fm_dx;
//			scan.Ypos += scan.fm_dy;
			scan.Xpos = _lsadd (scan.Xpos, scan.fm_dx);
			scan.Ypos = _lsadd (scan.Ypos, scan.fm_dy);
			feedback.i_pl_sum = _lsadd (feedback.i_pl_sum, scan.fm_dzxy); // apply slope compensation
			feedback.i_sum    = _lsadd (feedback.i_sum, scan.fm_dzxy); // keep mirror without slope compensation
			scan.num_steps_move_xy--;
		} else {
			if (scan.nx == 0){ // was MOVE_XY only?, then done
				scan.sstate = AS_READY;
				scan.pflg   = 0;
			} else {
				scan.sstate = AS_SCAN_XP;
				scan.iix    = scan.dnx + PIPE_LEN;
				scan.ix     = scan.nx;
				scan.iy     = scan.ny;
				scan.fm_dx  = scan.fs_dx;
				scan.fm_dy  = -scan.fs_dy;
				scan.cfs_dx = scan.fs_dx;
				scan.cfs_dy = scan.fs_dy;
				clear_summing_data ();
			}
		}
		break;

	default: // just cancel job in case sth. went wrong!
		scan.pflg = 0;
		break;
	}

	// calculate rotated coordinates, using rotation matrix "rot-[[xx,xy],[yx,yy]]"
	// using 32bit integer calculus

// no rotation
//		analog.x_scan = scan.Xpos>>16;
//		analog.y_scan = scan.Ypos>>16;

//		analog.x_scan = _lsshl (scan.Xpos, -16);
//		analog.y_scan = _lsshl (scan.Ypos, -16);

// with rotation, C Version is slow, needs long C-lib fkt and did not saturate, overflow risk!
//		analog.x_scan = ((long)(scan.Xpos>>15)*(long)scan.rotxx + (long)(scan.Ypos>>15)*(long)scan.rotxy)>>16;
//		analog.y_scan = ((long)(scan.Xpos>>15)*(long)scan.rotyx + (long)(scan.Ypos>>15)*(long)scan.rotyy)>>16;

// DSP-C-asm calls are fast and are saturating, no overflow risk, just limited.
	analog.x_scan = _lsshl (_lsadd ( _lsmpy (_lsshl (scan.Xpos, -16), scan.rotxx),
					 _lsmpy (_lsshl (scan.Ypos, -16), scan.rotxy)
					), -16);
	analog.y_scan = _lsshl (_lsadd ( _lsmpy (_lsshl (scan.Xpos, -16), scan.rotyx),
					 _lsmpy (_lsshl (scan.Ypos, -16), scan.rotyy)
					), -16);

}

