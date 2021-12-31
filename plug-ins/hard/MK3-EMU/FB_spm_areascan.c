/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 8 c-style: "K&R" -*- */

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

#include "g_intrinsics.h"
#include "FB_spm_dataexchange.h"
#include "FB_spm_areascan.h"
#include "FB_spm_probe.h"
#include "dataprocess.h"

#include "PAC_pll.h"

/* compensate for X AIC pipe delay and add user "nx_pre" amount */
#define PIPE_LEN (5+DSP_MEMORY(scan).nx_pre)

#define Z_DATA_BUFFER_SIZE    0x2000
#define Z_DATA_BUFFER_MASK    0x3fff

#define EN_PROBE_TRIGGER      0x0008

extern SPM_STATEMACHINE state;
extern FEEDBACK_MIXER   feedback_mixer;
extern SERVO            z_servo;
extern SERVO            m_servo;
extern ANALOG_VALUES    analog;
extern AREA_SCAN        scan;
extern PROBE            probe;
extern DATA_FIFO        datafifo;
extern DATA_FIFO        probe_datafifo;
extern CR_GENERIC_IO    CR_generic_io;
extern SCAN_EVENT_TRIGGER scan_event_trigger;

/* THIS IS DOUBLE USED !!! NO PROBE WHILE USED OF Z_DATA_BUFFER !!! */
extern DSP_INT16      prbdf[];

/* buffer for end of section probe data access for use with area scan */
extern int  VP_sec_end_buffer[10*8]; // 10x section + 8 values

#define PRB_SE_DELAY       8        /* Start/End Delay -- must match !!*/

int AS_ip, AS_jp, AS_kp, AS_mod;
int AS_ch2nd_scan_switch; /* const.height 2nd scan mode flag X+/-, holds next mode to switch to 2nd scanline */
int AS_ch2nd_constheight_enabled; /* const H mode flg */

int AS_deltasRe, AS_deltasIm, AS_cr, AS_ci, AS_sdnx;
int AS_cr2, AS_ci2; // 2F -- dummy
long long AS_amplitude;
int AS_offset;

long AS_AIC_data_sum[9];
int  AS_AIC_num_samples;

#define BZ_MAX_CHANNELS 18
DSP_INT32 bz_last[BZ_MAX_CHANNELS];
int  bz_byte_pos;

#define BZ_PUSH_NORMAL   0x00000000UL // normal atomatic mode using size indicator bits 31,30:
// -- Info: THIS CONST NAMES .._32,08,16,24 ARE NOT USED, JUST FOR DOCUMENMTATION PURPOSE
#define BZ_PUSH_MODE_32  0x00000000UL // 40:32 => 0b00MMMMMM(8bit) 0xDDDDDDDD(32bit), M:mode bits, D:Data
#define BZ_PUSH_MODE_08  0xC0000000UL //  8:6  => 0b11DDDDDD
#define BZ_PUSH_MODE_16  0x80000000UL // 16:14 => 0b10DDDDDD DDDDDDDD
#define BZ_PUSH_MODE_24  0x40000000UL // 24:22 => 0b01DDDDDD DDDDDDDD DDDDDDDD
#define BZ_PUSH_MODE_32_START   0x01000000UL // if 31,30=0 bits 29..0 available for special modes: 0x02..0x3f
// more modes free for later options and expansions
#define BZ_PUSH_MODE_32_FINISH  0x3f000000UL // finish up with Full Zero 32 bit record of max length as AS-END mark.
DSP_UINT32 bz_push_mode;

DSP_INT16 *Z_data_buffer;

/* Scan Control States */
#define AS_READY    0
#define AS_MOVE_XY  1
#define AS_SCAN_XP  2
#define AS_SCAN_YP  3
#define AS_SCAN_XM  4
#define AS_SCAN_YM  5
#define AS_SCAN_2ND_XP  6
#define AS_SCAN_2ND_XM  7

#ifndef DSPEMU
#pragma CODE_SECTION(bz_init, ".text:slow")
#endif

void bz_init(){ 
	int i; 
	for (i=0; i<(DATAFIFO_LENGTH>>1); ++i) DSP_MEMORY(datafifo).buffer_ul[i] = 0x3f3f3f3f;
	for (i=16; i<32; ++i) DSP_MEMORY(datafifo).buffer_ul[i] = (DSP_UINT32)i;
// testing only
//	for (i=(DATAFIFO_LENGTH>>2)-16; i<(DATAFIFO_LENGTH>>2); ++i) DSP_MEMORY(datafifo).buffer_ul[i] = (DSP_UINT32)i;
//	for (i=(DATAFIFO_LENGTH>>1)-16; i<(DATAFIFO_LENGTH>>1); ++i) DSP_MEMORY(datafifo).buffer_ul[i] = (DSP_UINT32)i;
// ------------
	for (i=0; i<BZ_MAX_CHANNELS; ++i) bz_last[i] = 0; 
	bz_push_mode = BZ_PUSH_MODE_32_START; 
	bz_byte_pos=0;
	DSP_MEMORY(datafifo).w_position=DSP_MEMORY(datafifo).r_position=0;
}

void bz_push(int i, DSP_INT32 x){
	union { DSP_INT32 l; DSP_UINT32 ul; } delta;
	int bits;
	delta.l = _SSUB32 (x, bz_last[i]); // if this saturates, original x is packed in 32bit mode either way.
	bz_last[i] = x;

	if (bz_push_mode) 
		goto bz_push_mode_32;

	bits = delta.l != 0 ? _NORM32 (delta.l) : 32;

//	std::cout << "BZ_PUSH(i=" << i << ", ***x=" << std::hex << x << std::dec << ") D.l=" << delta.l << " bits=" << bits;
	if (bits > 26){ // 8 bit package, 6 bit data
		delta.l <<= 26;
//		std::cout << ">26** 8:6bit** BZdelta.l=" << std::hex << delta.l << std::dec << " bz_byte_pos=" << bz_byte_pos << std::endl;;
		switch (bz_byte_pos){
		case 0:  DSP_MEMORY(datafifo).buffer_ul[DSP_MEMORY(datafifo).w_position>>1]  =  0xC0000000UL | (delta.ul>>2)       ; bz_byte_pos=1; break;
		case 1:  DSP_MEMORY(datafifo).buffer_ul[DSP_MEMORY(datafifo).w_position>>1] |= (0xC0000000UL | (delta.ul>>2)) >>  8; bz_byte_pos=2; break;
		case 2:  DSP_MEMORY(datafifo).buffer_ul[DSP_MEMORY(datafifo).w_position>>1] |= (0xC0000000UL | (delta.ul>>2)) >> 16; bz_byte_pos=3; break;
		default: DSP_MEMORY(datafifo).buffer_ul[DSP_MEMORY(datafifo).w_position>>1] |= (0xC0000000UL | (delta.ul>>2)) >> 24; bz_byte_pos=0;
			DSP_MEMORY(datafifo).w_position += 2;
			DSP_MEMORY(datafifo).w_position &= DATAFIFO_MASK;
			break;
		}
	} else if (bits > 18){ // 16 bit package, 14 bit data
		delta.l <<= 18;
//		std::cout << ">18**16:14bit** BZdelta.l=" << std::hex << delta.l << std::dec << " bz_byte_pos=" << bz_byte_pos << std::endl;;
 		switch (bz_byte_pos){
		case 0:  DSP_MEMORY(datafifo).buffer_ul[DSP_MEMORY(datafifo).w_position>>1]  =  0x80000000UL | (delta.ul>>2)       ; bz_byte_pos=2; break;
		case 1:  DSP_MEMORY(datafifo).buffer_ul[DSP_MEMORY(datafifo).w_position>>1] |= (0x80000000UL | (delta.ul>>2)) >>  8; bz_byte_pos=3; break;
		case 2:  DSP_MEMORY(datafifo).buffer_ul[DSP_MEMORY(datafifo).w_position>>1] |= (0x80000000UL | (delta.ul>>2)) >> 16; bz_byte_pos=0;
			DSP_MEMORY(datafifo).w_position += 2;
			DSP_MEMORY(datafifo).w_position &= DATAFIFO_MASK;
			break;
		default: DSP_MEMORY(datafifo).buffer_ul[DSP_MEMORY(datafifo).w_position>>1] |= (0x80000000UL | (delta.ul>>2)) >> 24; 
			DSP_MEMORY(datafifo).w_position += 2;
			DSP_MEMORY(datafifo).w_position &= DATAFIFO_MASK;
			DSP_MEMORY(datafifo).buffer_ul[DSP_MEMORY(datafifo).w_position>>1] = ((delta.ul>>2)&0x00ff0000UL) << 8; bz_byte_pos=1; break;
		}
	} else if (bits > 10){ // 24 bit package, 22 bit data
		delta.l <<= 10;
//		std::cout << ">10**24:22bit** BZdelta.l=" << std::hex << delta.l << std::dec << " bz_byte_pos=" << bz_byte_pos << std::endl;
		switch (bz_byte_pos){
		case 0:  DSP_MEMORY(datafifo).buffer_ul[DSP_MEMORY(datafifo).w_position>>1]  =  0x40000000UL | (delta.ul>>2)       ; bz_byte_pos=3; break;
		case 1:  DSP_MEMORY(datafifo).buffer_ul[DSP_MEMORY(datafifo).w_position>>1] |= (0x40000000UL | (delta.ul>>2)) >>  8; bz_byte_pos=0;
			DSP_MEMORY(datafifo).w_position += 2;
			DSP_MEMORY(datafifo).w_position &= DATAFIFO_MASK;
			break;
		case 2:  DSP_MEMORY(datafifo).buffer_ul[DSP_MEMORY(datafifo).w_position>>1] |= (0x40000000UL | (delta.ul>>2)) >> 16;
			DSP_MEMORY(datafifo).w_position += 2;
			DSP_MEMORY(datafifo).w_position &= DATAFIFO_MASK;
			DSP_MEMORY(datafifo).buffer_ul[DSP_MEMORY(datafifo).w_position>>1] = ((delta.ul>>2)&0x0000ff00UL) << 16; bz_byte_pos=1; break;
		default: DSP_MEMORY(datafifo).buffer_ul[DSP_MEMORY(datafifo).w_position>>1] |= (0x40000000UL | (delta.ul>>2)) >> 24;
			DSP_MEMORY(datafifo).w_position += 2;
			DSP_MEMORY(datafifo).w_position &= DATAFIFO_MASK;
			DSP_MEMORY(datafifo).buffer_ul[DSP_MEMORY(datafifo).w_position>>1] = ((delta.ul>>2)&0x00ffff00UL) << 8; bz_byte_pos=2; break;
		}
	} else { // full 32 bit data -- special on byte alignment -- 6 spare unused bits left!
	bz_push_mode_32:
		delta.l = x;
//		std::cout << "ELSE**40:32bit** BZdelta.l=" << std::hex << delta.l << std::dec << " bz_byte_pos=" << bz_byte_pos << std::endl;
//		bits=DSP_MEMORY(datafifo).w_position>>1;

		switch (bz_byte_pos){
		case 0:  DSP_MEMORY(datafifo).buffer_ul[DSP_MEMORY(datafifo).w_position>>1]  =  bz_push_mode | (delta.ul >> 8); // 0xMMffffff..
			DSP_MEMORY(datafifo).w_position += 2;
			DSP_MEMORY(datafifo).w_position &= DATAFIFO_MASK;
			DSP_MEMORY(datafifo).buffer_ul[DSP_MEMORY(datafifo).w_position>>1] = delta.ul << 24; bz_byte_pos=1; break;
		case 1:  DSP_MEMORY(datafifo).buffer_ul[DSP_MEMORY(datafifo).w_position>>1] |= (bz_push_mode | (delta.ul >> 8)) >> 8; // 0x==MMffff....
			DSP_MEMORY(datafifo).w_position += 2;
			DSP_MEMORY(datafifo).w_position &= DATAFIFO_MASK;
			DSP_MEMORY(datafifo).buffer_ul[DSP_MEMORY(datafifo).w_position>>1] = delta.ul << 16; bz_byte_pos=2; break;
		case 2:  DSP_MEMORY(datafifo).buffer_ul[DSP_MEMORY(datafifo).w_position>>1] |= (bz_push_mode | (delta.ul >> 8)) >> 16; // 0x====MMff......
			DSP_MEMORY(datafifo).w_position += 2;
			DSP_MEMORY(datafifo).w_position &= DATAFIFO_MASK;
			DSP_MEMORY(datafifo).buffer_ul[DSP_MEMORY(datafifo).w_position>>1] = delta.ul <<  8; bz_byte_pos=3; break;
		default: DSP_MEMORY(datafifo).buffer_ul[DSP_MEMORY(datafifo).w_position>>1] |= bz_push_mode >> 24; // 0x====MM........
			DSP_MEMORY(datafifo).w_position += 2;
			DSP_MEMORY(datafifo).w_position &= DATAFIFO_MASK;
			DSP_MEMORY(datafifo).buffer_ul[DSP_MEMORY(datafifo).w_position>>1] = delta.ul;
			DSP_MEMORY(datafifo).w_position += 2;
			DSP_MEMORY(datafifo).w_position &= DATAFIFO_MASK; bz_byte_pos=0;
			break;
		}
	}
//		Xdumpbuffer ((unsigned char*)&DSP_MEMORY(datafifo).buffer_ul[bits-4], 32, 0);
}

void clear_summing_data (){
	AS_AIC_data_sum[0] = AS_AIC_data_sum[1] = AS_AIC_data_sum[2] =
	AS_AIC_data_sum[3] = AS_AIC_data_sum[4] = AS_AIC_data_sum[5] =
	AS_AIC_data_sum[6] = AS_AIC_data_sum[7] = AS_AIC_data_sum[8] = 0;
	DSP_MEMORY(analog).counter[0] = 0;
	AS_AIC_num_samples = 0;
}

/* calc of f_dx/y and num_steps by host! */
#ifndef DSPEMU
#pragma CODE_SECTION(init_area_scan, ".text:slow")
#endif

void init_area_scan (){
	int m;
	// wait until fifo read thread on host is ready
	if (!DSP_MEMORY(datafifo).stall) {
		// reset FIFO -- done by host

		init_probe_fifo (); // reset probe fifo once at start!

		DSP_MEMORY(scan).stop  = 0;
		DSP_MEMORY(scan).slow_down_factor = DSP_MEMORY(scan).start > 0 ? DSP_MEMORY(scan).start : 1; // 1 normally, >1 for slow down
                m = AREA_SCAN_RUN | AREA_SCAN_START_NORMAL;
                if (DSP_MEMORY(scan).start < 0){
                        m = -DSP_MEMORY(scan).start; // ..START_NORMAL normal vector scan, linear X ramp, ..START_FAST_SCAN: fast mode, sine generator for X "ramp" 

                        if (m & AREA_SCAN_START_FASTSCAN){ // init Sine generator, delatsRe/Im calculated by host, can be changed on line by line schedule
                                AS_deltasRe = DSP_MEMORY(scan).Zoff_2nd_xp; // shared variables, no 2nd line mode in fast scan!
                                AS_deltasIm = DSP_MEMORY(scan).Zoff_2nd_xm;
                                // #define CPN(N) ((double)(1LL<<(N))-1.)
                                // deltasRe = round (CPN(PREC) * cos (2.*M_PI*freq[0]/150000.)); -- here 2147483647 * cos (2pi/N), N=n*dnx*2+dny
                                // deltasIm = round (CPN(PREC) * sin (2.*M_PI*freq[0]/150000.)); -- ... sin
                                AS_cr = -2147483647; // staring sine at 6 O'Clock or at -1.00 or -(1<<31) int equiv.
                                AS_ci = 0;
                                AS_amplitude = (long long)((DSP_MEMORY(scan).dnx * DSP_MEMORY(scan).nx * DSP_MEMORY(scan).cfs_dx)>>1);
                                AS_offset    = 0;
                        }
                }

		// now starting...
		DSP_MEMORY(scan).start = 0;
		
		// calculate num steps to go to starting point -- done by host
		
		// init probe trigger count (-1 will disable it)
		AS_mod = DSP_MEMORY(scan).dnx_probe;
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
		if (DSP_MEMORY(scan).srcs_2nd_xp || DSP_MEMORY(scan).srcs_2nd_xm)
			if (DSP_MEMORY(scan).nx < Z_DATA_BUFFER_SIZE)
				AS_ch2nd_scan_switch = AS_SCAN_2ND_XP; // enable 2nd scan line mode
		
		if ((DSP_MEMORY(scan).srcs_xp & 0x07000) || (DSP_MEMORY(scan).srcs_xm & 0x07000))  // setup LockIn job
			init_lockin (PROBE_RUN_LOCKIN_SCAN);
		
		
		if ((DSP_MEMORY(scan).srcs_xp & 0x08000) || (DSP_MEMORY(scan).srcs_xm & 0x08000)){ // if Counter channel requested, restart counter/timer now
			DSP_MEMORY(analog).counter[0] = 0;
			DSP_MEMORY(analog).counter[1] = 0;
		}

		bz_init();

		Z_data_buffer = &DSP_MEMORY(prbdf[0]); // using probe buffer -- shared!

		// enable subtask
		DSP_MEMORY(scan).sstate = AS_MOVE_XY;
		DSP_MEMORY(scan).pflg  = m;
	}
}

#ifndef DSPEMU
#pragma CODE_SECTION(finish_area_scan, ".text:slow")
#endif

void finish_area_scan (){
	int i;

	stop_lockin (PROBE_RUN_LOCKIN_SCAN);

	bz_push_mode = BZ_PUSH_MODE_32_FINISH;
	for (i=0; i<BZ_MAX_CHANNELS; ++i)
		bz_push (i, 0);
	DSP_MEMORY(scan).sstate = AS_READY;
	DSP_MEMORY(scan).pflg = 0;
}

void integrate_as_data_srcs (DSP_UINT32 srcs){
	if (srcs & 0x01) // PIDSrcA1 (Dest) --> Zmonitor (Topo) feedback generated Z signal
		AS_AIC_data_sum[8] += DSP_MEMORY(z_servo).control >> 16;

	if (srcs & 0x0010) // DataSrcA1 --> AIC0 <-- I (current), ...
		AS_AIC_data_sum[0] += AIC_IN(0);

	if (srcs & 0x0020) // DataSrcA2 --> AIC1
		AS_AIC_data_sum[1] += AIC_IN(1);

	if (srcs & 0x0040) // DataSrcA3 --> AIC2
		AS_AIC_data_sum[2] += AIC_IN(2);

	if (srcs & 0x0080) // DataSrcA4 --> AIC3
		AS_AIC_data_sum[3] += AIC_IN(3);

	if (srcs & 0x0100) // DataSrcB1 --> AIC4
		AS_AIC_data_sum[4] += AIC_IN(4);

	if (srcs & 0x0200) // DataSrcB2 --> AIC5
		AS_AIC_data_sum[5] += AIC_IN(5);

	if (srcs & 0x0400) // DataSrcB3 --> AIC6
		AS_AIC_data_sum[6] += AIC_IN(6);

	if (srcs & 0x0800) // DataSrcB4 --> AIC7
		AS_AIC_data_sum[7] += AIC_IN(7);

	++AS_AIC_num_samples;
}

void push_area_scan_data (DSP_UINT32 srcs){
	DSP_UINT32 tmp;

	// read and buffer (for Rate Meter, gatetime not observed -- always last completed count)
	DSP_MEMORY(CR_generic_io).count_0 = DSP_MEMORY(analog).counter[0];

	if (srcs & 0xFFF1){
		// using bz_push for bit packing and delta signal usage
		// init/reinit data set?

		tmp = (srcs << 16) | AS_AIC_num_samples;
		if (tmp != bz_last[0]){ // re init using BZ_PUSH_MODE_32_START to indicate extra info set
			bz_push_mode = BZ_PUSH_MODE_32_START; 
			bz_push (0, tmp);
			bz_push_mode = BZ_PUSH_NORMAL;
		}

		if (srcs & 0x0001) // PIDSrc1/Dest <-- Z = -AIC_Z-value ** AS_AIC_data_sum[8]/AS_AIC_num_samples
			bz_push (1, DSP_MEMORY(z_servo).control);
		if (srcs & 0x0002) // MIXER selection 1
			bz_push (2, DSP_MEMORY(feedback_mixer).FB_IN_processed[1]);
		if (srcs & 0x0004) // MIXER selection 2
			bz_push (3, DSP_MEMORY(feedback_mixer).FB_IN_processed[2]);
		if (srcs & 0x0008) // MIXER selection 3
			bz_push (4, DSP_MEMORY(feedback_mixer).FB_IN_processed[3]);
		if (srcs & 0x0010) // DataSrcA1 --> AIC0
			bz_push (5, DSP_MEMORY(feedback_mixer).FB_IN_processed[0]); // *** (int)AS_AIC_data_sum[0]);
		if (srcs & 0x0020) // DataSrcA2 --> AIC1
			bz_push (6, (int)AS_AIC_data_sum[1]);
		if (srcs & 0x0040) // DataSrcA3 --> AIC2
			bz_push (7, (int)AS_AIC_data_sum[2]);
		if (srcs & 0x0080) // DataSrcA4 --> AIC3
			bz_push (8, (int)AS_AIC_data_sum[3]);
		if (srcs & 0x0100) // DataSrcB1 --> AIC4
			bz_push (9, (int)AS_AIC_data_sum[4]);
		if (srcs & 0x0200) // DataSrcB2 --> AIC5
			bz_push (10, (int)AS_AIC_data_sum[5]);
		if (srcs & 0x0400) // DataSrcB3 --> AIC6
			bz_push (11, (int)AS_AIC_data_sum[6]);
		if (srcs & 0x0800) // DataSrcB4 --> AIC7
			bz_push (12, (int)AS_AIC_data_sum[7]);

		if (srcs & 0x01000) // DataSrcC1 --> LockIn1stA [default maped signal]
			bz_push (13, *DSP_MEMORY(scan).src_input[0]);
		if (srcs & 0x02000) // DataSrcD1 --> LockIn2ndA [default maped signal]
			bz_push (14, *DSP_MEMORY(scan).src_input[1]);
		if (srcs & 0x04000) // DataSrcE1 --> LockIn0 [default maped signal]
			bz_push (15, *DSP_MEMORY(scan).src_input[2]); 
		if (srcs & 0x08000) // "DataSrcF1" last CR Counter count [default maped signal]
			bz_push (16, *DSP_MEMORY(scan).src_input[3]);
	}

	// auto clear now including counter0
	clear_summing_data ();
	DSP_MEMORY(analog).counter[0] = 0;
	DSP_MEMORY(analog).counter[1] = 0;
}

#ifndef DSPEMU
#pragma CODE_SECTION(check_scan_event_trigger, ".text:slow")
#endif

void check_scan_event_trigger (int trig_i[8], int bias_setpoint[8]){
	int i;
	for (i=0; i<8; ++i)
		if (DSP_MEMORY(scan).ix == trig_i[i]){
			if (i<4)
				DSP_MEMORY(analog).bias = bias_setpoint [i];
			else
				DSP_MEMORY(feedback_mixer).setpoint[0] = bias_setpoint [i];
		}
}

// TESTING IF SLOW WORKS HERE *****!!!!!!!!!!
#ifndef DSPEMU
#pragma CODE_SECTION(run_area_scan, ".text:slow")
#endif

void run_area_scan (){
	switch (DSP_MEMORY(scan).sstate){
	case AS_SCAN_XP: // "X+" -> scan and dataaq.
		if (--DSP_MEMORY(scan).iiix > 0)
			break;
                DSP_MEMORY(scan).iiix = DSP_MEMORY(scan).slow_down_factor;

		if (DSP_MEMORY(scan).iix < DSP_MEMORY(scan).dnx)
			integrate_as_data_srcs (DSP_MEMORY(scan).srcs_xp);
//		DSP_MEMORY(scan).xyz_vec[i_X] += DSP_MEMORY(scan).fs_dx;
		DSP_MEMORY(scan).xyz_vec[i_X] = _SADD32 (DSP_MEMORY(scan).xyz_vec[i_X], DSP_MEMORY(scan).cfs_dx); // this is with SAT!!
		if (!DSP_MEMORY(scan).iix--){
			if (DSP_MEMORY(scan).ix--){
				if (AS_ip >= 0 && (AS_jp == 0 || DSP_MEMORY(scan).raster_a) && (DSP_MEMORY(scan).dnx_probe > 0)){
					if (! --AS_ip){ // trigger probing process ?
						if (!DSP_MEMORY(probe).pflg) // assure last prb job is done!!
							init_probe ();
						AS_ip = DSP_MEMORY(scan).dnx_probe;
					}
				}
				if (DSP_MEMORY(scan_event_trigger).pflg)
					check_scan_event_trigger (DSP_MEMORY(scan_event_trigger).trig_i_xp, DSP_MEMORY(scan_event_trigger).xp_bias_setpoint);

				DSP_MEMORY(scan).iix  = DSP_MEMORY(scan).dnx-1;
				
				if (AS_ch2nd_constheight_enabled){
//**** FIX!				feedback.z = _sadd (Z_data_buffer [DSP_MEMORY(scan).ix & Z_DATA_BUFFER_MASK], DSP_MEMORY(scan).Zoff_2nd_xp); // use memorized ZA profile + offset
					push_area_scan_data (DSP_MEMORY(scan).srcs_2nd_xp); // get 2nd XP data here!
				}else{
					if (AS_ch2nd_scan_switch == AS_SCAN_2ND_XP) // only if enabled 2nd scan line mode
						Z_data_buffer [DSP_MEMORY(scan).ix & Z_DATA_BUFFER_MASK] = DSP_MEMORY(z_servo).control; // memorize Z profile

					push_area_scan_data (DSP_MEMORY(scan).srcs_xp); // get XP data here!
				}
			}
			else{
				DSP_MEMORY(scan).ix  = DSP_MEMORY(scan).nx;
				DSP_MEMORY(scan).iix = DSP_MEMORY(scan).dnx + PIPE_LEN;
				if (AS_ch2nd_constheight_enabled)
					DSP_MEMORY(scan).sstate = AS_SCAN_2ND_XM;
				else
					DSP_MEMORY(scan).sstate = AS_SCAN_XM;
				clear_summing_data ();
			}
			DSP_MEMORY(scan).cfs_dx = DSP_MEMORY(scan).fs_dx;
		}
		break;

	case AS_SCAN_XM: // "X-" <- scan and dataaq.
		DSP_MEMORY(scan).ifr = DSP_MEMORY(scan).fast_return;
		do{ // fast rep this for return speedup option
			if (DSP_MEMORY(scan).iix < DSP_MEMORY(scan).dnx)
				integrate_as_data_srcs (DSP_MEMORY(scan).srcs_xm);
//			DSP_MEMORY(scan).xyz_vec[i_X] -= DSP_MEMORY(scan).fs_dx;
			DSP_MEMORY(scan).xyz_vec[i_X] = _SSUB32 (DSP_MEMORY(scan).xyz_vec[i_X], DSP_MEMORY(scan).cfs_dx);
			if (!DSP_MEMORY(scan).iix--){
				if (DSP_MEMORY(scan).ix--){
#if 0
					if (AS_ip >= 0 && AS_jp == 0 && (DSP_MEMORY(scan).dnx_probe > 0)){
						if (! --AS_ip){ // trigger probing process ?
							if (!DSP_MEMORY(probe).pflg) // assure last prb job is done!!
								init_probe ();
							AS_ip = DSP_MEMORY(scan).dnx_probe;
						}
					}
#endif
					if (DSP_MEMORY(scan_event_trigger).pflg)
						check_scan_event_trigger (DSP_MEMORY(scan_event_trigger).trig_i_xm, DSP_MEMORY(scan_event_trigger).xm_bias_setpoint);
					
					DSP_MEMORY(scan).iix   = DSP_MEMORY(scan).dnx-1;
					
					if (AS_ch2nd_constheight_enabled){
// FIX!!!!					feedback.z = _sadd (Z_data_buffer [(DSP_MEMORY(scan).nx-DSP_MEMORY(scan).ix-1) & Z_DATA_BUFFER_MASK], DSP_MEMORY(scan).Zoff_2nd_xm); // use memorized ZA profile + offset
						push_area_scan_data (DSP_MEMORY(scan).srcs_2nd_xm); // get 2nd XP data here!
					}else{
						push_area_scan_data (DSP_MEMORY(scan).srcs_xm); // get XM data here!
					}
				}
				else{
					if (!DSP_MEMORY(scan).iy){ // area scan done?
						finish_area_scan ();
						goto finish_now;
					}
					DSP_MEMORY(scan).iiy = DSP_MEMORY(scan).dny;
					DSP_MEMORY(scan).cfs_dy = DSP_MEMORY(scan).fs_dy;
					if (AS_ch2nd_constheight_enabled) {
						DSP_MEMORY(scan).sstate = AS_SCAN_YP;
						AS_ch2nd_constheight_enabled = 0;  // disable now
					}else
						DSP_MEMORY(scan).sstate = AS_ch2nd_scan_switch; // switch to next YP or 2nd stage scan line
				}
				DSP_MEMORY(scan).cfs_dx = DSP_MEMORY(scan).fs_dx;
			}
		} while (--DSP_MEMORY(scan).ifr > 0 && DSP_MEMORY(scan).sstate == AS_SCAN_XM);
        finish_now:
 		break;

	case AS_SCAN_2ND_XP: // configure 2nd XP scan
		AS_ch2nd_constheight_enabled = 1;  // enable now
		// disable feedback!!
		// config normal XP again
		DSP_MEMORY(scan).ix  = DSP_MEMORY(scan).nx;
		DSP_MEMORY(scan).iix = DSP_MEMORY(scan).dnx + PIPE_LEN;
		DSP_MEMORY(scan).cfs_dx = DSP_MEMORY(scan).fs_dx;
		DSP_MEMORY(scan).sstate = AS_SCAN_XP;
		clear_summing_data ();
		break;
	case AS_SCAN_2ND_XM: // configure 2nd XP scan
		AS_ch2nd_constheight_enabled = 1;  // enable now
		// config normal XM again
		DSP_MEMORY(scan).ix  = DSP_MEMORY(scan).nx;
		DSP_MEMORY(scan).iix = DSP_MEMORY(scan).dnx + PIPE_LEN;
		DSP_MEMORY(scan).cfs_dx = DSP_MEMORY(scan).fs_dx;
		DSP_MEMORY(scan).sstate = AS_SCAN_XM;
		clear_summing_data ();
		break;
	case AS_SCAN_YP: // "Y+" next line (could be Y-up or Y-dn, dep. on sign. of fs_dy!)
		if (DSP_MEMORY(scan).iiy--){
//			DSP_MEMORY(scan).xyz_vec[i_Y] -= DSP_MEMORY(scan).fs_dy;
			DSP_MEMORY(scan).xyz_vec[i_Y] = _SSUB32 (DSP_MEMORY(scan).xyz_vec[i_Y], DSP_MEMORY(scan).cfs_dy);
		}
		else{
			if (DSP_MEMORY(scan).iy--){
				if (AS_jp >= 0){
					if (DSP_MEMORY(scan).raster_a){
						AS_ip = AS_kp;
						if ((++AS_jp) & 1){ // set start if X grid counter
							if (++AS_kp >= AS_mod)
								AS_kp=0;
							AS_ip += DSP_MEMORY(scan).raster_a;
						}
						if (AS_ip++ >= AS_mod)
							AS_ip -= AS_mod;
					} else
						if (++AS_jp == DSP_MEMORY(scan).dnx_probe){
							AS_jp = 0;
							AS_ip = DSP_MEMORY(scan).dnx_probe; // reset X grid counter
						}
				}
				DSP_MEMORY(scan).ix  = DSP_MEMORY(scan).nx;
				DSP_MEMORY(scan).iix = DSP_MEMORY(scan).dnx + PIPE_LEN;
				DSP_MEMORY(scan).sstate = AS_SCAN_XP;
				clear_summing_data ();
			}
			else // area scan is done -- double check here.
				finish_area_scan ();
			DSP_MEMORY(scan).cfs_dx = DSP_MEMORY(scan).fs_dx;
		}
		break;

//	case AS_SCAN_YM: // "Y-" -- not valid yet -- exit AS
//		finish_area_scan ();
//		break;

	case AS_MOVE_XY: // move-XY:  and Z -- init/finalize scan, move to start/end of scan position and generic tip positioning withing scan coordinates, 
		// can be used with nx==0, then the job is finished after the xy move!!
		if (DSP_MEMORY(scan).num_steps_move_xy){
			if (!(DSP_MEMORY(state).mode & MD_ZPOS_ADJUSTER)){ // is MOVE_XY Z allowed? Then may manipulate Zpos as well and store data
				// optional (if fm_dz non Null) Z-pos manipulation within safe configurable guards
				if (*DSP_MEMORY(probe).limiter_input < *DSP_MEMORY(probe).limiter_updn[0] && *DSP_MEMORY(probe).limiter_input > *DSP_MEMORY(probe).limiter_updn[1]){
					DSP_MEMORY(scan).xyz_vec[i_X] = _SADD32 (DSP_MEMORY(scan).xyz_vec[i_X], DSP_MEMORY(scan).fm_dx);
					DSP_MEMORY(scan).xyz_vec[i_Y] = _SADD32 (DSP_MEMORY(scan).xyz_vec[i_Y], DSP_MEMORY(scan).fm_dy);
					DSP_MEMORY(probe).Zpos = _SADD32 (DSP_MEMORY(probe).Zpos, DSP_MEMORY(scan).fm_dz); // manipulate Z
				}
				push_area_scan_data (DSP_MEMORY(scan).srcs_xp); // now also store channel data along trajectory!
				clear_summing_data ();
			} else { // normal XY MOVE
				DSP_MEMORY(scan).xyz_vec[i_X] = _SADD32 (DSP_MEMORY(scan).xyz_vec[i_X], DSP_MEMORY(scan).fm_dx);
				DSP_MEMORY(scan).xyz_vec[i_Y] = _SADD32 (DSP_MEMORY(scan).xyz_vec[i_Y], DSP_MEMORY(scan).fm_dy);
			}
			DSP_MEMORY(scan).num_steps_move_xy--;
		} else {
			if (DSP_MEMORY(scan).nx == 0){ // was MOVE_XY only?, then done
				finish_area_scan ();
			} else {
				DSP_MEMORY(scan).sstate = AS_SCAN_XP;
				DSP_MEMORY(scan).iix    = DSP_MEMORY(scan).dnx + PIPE_LEN;
                                DSP_MEMORY(scan).iiix   = DSP_MEMORY(scan).slow_down_factor;
				DSP_MEMORY(scan).ix     = DSP_MEMORY(scan).nx;
				DSP_MEMORY(scan).iy     = DSP_MEMORY(scan).ny;
				DSP_MEMORY(scan).fm_dx  = DSP_MEMORY(scan).fs_dx;
				DSP_MEMORY(scan).fm_dy  = -DSP_MEMORY(scan).fs_dy;
				DSP_MEMORY(scan).cfs_dx = DSP_MEMORY(scan).fs_dx;
				DSP_MEMORY(scan).cfs_dy = DSP_MEMORY(scan).fs_dy;
				clear_summing_data ();
			}
		}
		break;

	default: // just cancel job in case sth. went wrong!
		finish_area_scan ();
		break;
	}
}

// fast scan with X ramp mode as perfect harmonic Sine wave, no gizmos like LockIn, 2nd-line or DSP_MEMORY(probe).
// CHECK SineSDB for compatibility with new 1F2F PAC PLL lib
// with 2f: void SineSDB(int *cr, int *ci, int *cr2, int *ci2, int deltasRe, int deltasIm)
void run_area_scan_fast (){
	if (DSP_MEMORY(scan).sstate != AS_MOVE_XY){
	  //	SineSDB (&AS_cr, &AS_ci, AS_deltasRe, AS_deltasIm); // must match scan "speed" settings to be in sync. ==> N=n*dnx*2+dny
	        SineSDB (&AS_cr, &AS_ci, &AS_cr2, &AS_ci2, AS_deltasRe, AS_deltasIm); // must match scan "speed" settings to be in sync. ==> N=n*dnx*2+dny
		DSP_MEMORY(scan).xyz_vec[i_X] = _SADD32 ((int)((AS_amplitude * (long long)AS_cr)>>31), AS_offset); // adjust "volume" to X scan range, add offset
	}
	switch (DSP_MEMORY(scan).sstate){
	case AS_SCAN_XP: // "X+" -> scan and dataaq.
		if (DSP_MEMORY(scan).iix < DSP_MEMORY(scan).dnx)
			integrate_as_data_srcs (DSP_MEMORY(scan).srcs_xp);
//		DSP_MEMORY(scan).xyz_vec[i_X] = _SADD32 (DSP_MEMORY(scan).xyz_vec[i_X], DSP_MEMORY(scan).cfs_dx); // this is with SAT!!
		if (!DSP_MEMORY(scan).iix--){
			if (DSP_MEMORY(scan).ix--){
				DSP_MEMORY(scan).iix    = AS_sdnx-1;
				push_area_scan_data (DSP_MEMORY(scan).srcs_xp); // get XP data here!
			}
			else{
				DSP_MEMORY(scan).ix  = DSP_MEMORY(scan).nx;
				DSP_MEMORY(scan).iix = AS_sdnx;
				DSP_MEMORY(scan).sstate = AS_SCAN_XM;
				clear_summing_data ();
			}
//			DSP_MEMORY(scan).cfs_dx = DSP_MEMORY(scan).fs_dx;
		}
		break;

	case AS_SCAN_XM: // "X-" <- scan and dataaq.
		if (DSP_MEMORY(scan).iix < DSP_MEMORY(scan).dnx)
			integrate_as_data_srcs (DSP_MEMORY(scan).srcs_xm);
//		DSP_MEMORY(scan).xyz_vec[i_X] = _SSUB32 (DSP_MEMORY(scan).xyz_vec[i_X], DSP_MEMORY(scan).cfs_dx);
		if (!DSP_MEMORY(scan).iix--){
			if (DSP_MEMORY(scan).ix--){
				DSP_MEMORY(scan).iix   = AS_sdnx-1;
				push_area_scan_data (DSP_MEMORY(scan).srcs_xm); // get XM data here!
			}
			else{
				if (!DSP_MEMORY(scan).iy){ // area scan done?
					finish_area_scan ();
					goto finish_now;
				}
				DSP_MEMORY(scan).iiy = DSP_MEMORY(scan).dny;
				DSP_MEMORY(scan).cfs_dy = DSP_MEMORY(scan).fs_dy;
				DSP_MEMORY(scan).sstate = AS_ch2nd_scan_switch; // switch to next YP or 2nd stage scan line
			}
//			DSP_MEMORY(scan).cfs_dx = DSP_MEMORY(scan).fs_dx;
		}
        finish_now:
 		break;

	case AS_SCAN_YP: // "Y+" next line (could be Y-up or Y-dn, dep. on sign. of fs_dy!)
		if (DSP_MEMORY(scan).iiy--){
//			DSP_MEMORY(scan).xyz_vec[i_Y] -= DSP_MEMORY(scan).fs_dy;
			DSP_MEMORY(scan).xyz_vec[i_Y] = _SSUB32 (DSP_MEMORY(scan).xyz_vec[i_Y], DSP_MEMORY(scan).cfs_dy);
		}
		else{
			if (DSP_MEMORY(scan).iy--){
				DSP_MEMORY(scan).ix     = DSP_MEMORY(scan).nx;
				DSP_MEMORY(scan).iix    = AS_sdnx = DSP_MEMORY(scan).dnx; // in fast sin mode only speed update in line basis possible due to non-linear steps
				DSP_MEMORY(scan).sstate = AS_SCAN_XP;

				// update Sine frequency now possible, Sine must (should) be now at 6 O'Clock (-1)
				AS_deltasRe = DSP_MEMORY(scan).Zoff_2nd_xp; // shared variables, no 2nd line mode in fast scan!
				AS_deltasIm = DSP_MEMORY(scan).Zoff_2nd_xm;
				// make sure to keep in sync in case errors build up
				AS_cr = -2147483647; // staring sine at 6 O'Clock or at -1.00 or -(1<<31) int equiv.
				AS_ci = 0;
				clear_summing_data ();
			}
			else // area scan is done -- double check here.
				finish_area_scan ();
//			DSP_MEMORY(scan).cfs_dx = DSP_MEMORY(scan).fs_dx;
		}
		break;

//	case AS_SCAN_YM: // "Y-" -- not valid yet -- exit AS
//		finish_area_scan ();
//		break;

	case AS_MOVE_XY: // move-XY: init/finalize scan, move to start/end of scan position, 
		// can be used with nx==0, then the job is finished after the xy move!!
		if (DSP_MEMORY(scan).num_steps_move_xy){
			DSP_MEMORY(scan).xyz_vec[i_X] = _SADD32 (DSP_MEMORY(scan).xyz_vec[i_X], DSP_MEMORY(scan).fm_dx);
			DSP_MEMORY(scan).xyz_vec[i_Y] = _SADD32 (DSP_MEMORY(scan).xyz_vec[i_Y], DSP_MEMORY(scan).fm_dy);
			DSP_MEMORY(scan).num_steps_move_xy--;
		} else {
			if (DSP_MEMORY(scan).nx == 0){ // was MOVE_XY only?, then done
				finish_area_scan ();
			} else {
				DSP_MEMORY(scan).sstate = AS_SCAN_XP;
				DSP_MEMORY(scan).iix    = AS_sdnx = DSP_MEMORY(scan).dnx;
				DSP_MEMORY(scan).ix     = DSP_MEMORY(scan).nx;
				DSP_MEMORY(scan).iy     = DSP_MEMORY(scan).ny;
				DSP_MEMORY(scan).fm_dx  = DSP_MEMORY(scan).fs_dx;
				DSP_MEMORY(scan).fm_dy  = -DSP_MEMORY(scan).fs_dy;
//				DSP_MEMORY(scan).cfs_dx = DSP_MEMORY(scan).fs_dx;
				DSP_MEMORY(scan).cfs_dy = DSP_MEMORY(scan).fs_dy;
				AS_amplitude = ((long long)DSP_MEMORY(scan).dnx * (long long)DSP_MEMORY(scan).nx * (long long)DSP_MEMORY(scan).cfs_dx) >> 1;
				AS_offset   = _SADD32 (DSP_MEMORY(scan).xyz_vec[i_X], (long)AS_amplitude);
				clear_summing_data ();
			}
		}
		break;

	default: // just cancel job in case sth. went wrong!
		finish_area_scan ();
		break;
	}
}

