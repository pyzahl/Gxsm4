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
#include "dataprocess.h"

#include "PAC_pll.h"
#include "ReadWrite_GPIO.h"

#include "mcbsp_support.h"


/* compensate for X AIC pipe delay and add user "nx_pre" amount */
#define PIPE_LEN (5+scan.nx_pre)

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
extern DATA_SYNC_IO     data_sync_io;

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

//#define DATAPROCESS_PUSH_INSTANT
// or push via idle and FIFO
#define RT_FIFO_HALF 16 //  8
#define RT_FIFO_FULL 30 // 14
#define RT_FIFO_MASK 31 // 15
DSP_INT32 rt_fifo_i=0;
DSP_INT32 rt_fifo_ix=0;
DSP_INT32 rt_fifo_j=0;
DSP_INT32 rt_fifo_push_data[17][RT_FIFO_MASK+1];

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

DSP_INT32 *Z_data_buffer;

/* Scan Control States */
#define AS_READY    0
#define AS_MOVE_XY  1
#define AS_SCAN_XP  2
#define AS_SCAN_YP  3
#define AS_SCAN_XM  4
#define AS_SCAN_YM  5
#define AS_SCAN_2ND_XP  6
#define AS_SCAN_2ND_XM  7

extern void switch_rt_task_areascan_to_probe (void);

// to control (Z-)Servo manually for 2nd line scan modes

inline void run_servo_manual (SERVO *servo, DSP_INT32 set){
	long long tmp;
        servo->i_sum = set;
	tmp = set;
	// make both output polarities available
	servo->control = _SAT32 (tmp);
	servo->neg_control = _SAT32 (-tmp);
}


#if 0  // --- same function available via PAC_pll lib as SineSDB()
// NEW with 2f: void SineSDB(int *cr, int *ci, int *cr2, int *ci2, int deltasRe, int deltasIm)
void SineSDB64(int *cr, int *ci, int deltasRe, int deltasIm)
{
        long long RealP,ImaP,Error_e;
        long RealP_Hi,ImaP_Hi,Error_int;
        
        // *********** Sinus calculation

        //    Real part calculation : crdr - cidi
        //  Q31*Q31 - Q31*Q31=Q62
        //  For cr=2147483647 and deltasRe=2129111627
        //      ci=0 and  deltasIm=280302863
        //  crdr - cidi = 2129111626 Q31 (RealP_Hi)
        //  RealP=4572232401620063669

        RealP=((long long)cr[0] * (long long)deltasRe) - ((long long)ci[0] * (long long)deltasIm);
        RealP_Hi=(long)(RealP>>31); //Q62 to Q31

        //    Imaginary part calculation : drci + crdi
        //  Q31*Q31 - Q31*Q31=Q62
        //  drci - crdi= -280302863 Q31 (ImaP_Hi)
        //  ImaP=-601945814499781361

        ImaP=((long long)ci[0] * (long long)deltasRe) + ((long long)cr[0] * (long long)deltasIm);
        ImaP_Hi=(long)(ImaP>>31); //Q62 to Q31

        //    e=Error/2 = (1 - (cr^2+ci^2))/2
        //  e=(Q62 - Q62 - Q62))>>32 --> Q30 (so Q31/2)
        //  4611686018427387903 - (-280302863*-280302863) - (2129111626*2129111626)
        //  Error_e=4611686018427387903 - 78569695005996769 - 4533116315968363876
        //  Error_e = 7453027258
        //  Error_int = 1

        Error_e=0x3FFFFFFFFFFFFFFF; //4611686018427387903
        Error_e=Error_e - ((long long)ImaP_Hi*(long long)ImaP_Hi) - ((long long)RealP_Hi*(long long)RealP_Hi);
        Error_int=(long)(Error_e>>32); //Q62 to Q31 plus /2

        //    Update Cre and Cim
        //  Cr=Cr(1-e)=Cr-Cr*e // Q62 - Q31*Q31
        //  Ci=Ci(1-e)=Ci-Ci*e
        //  ImaP=-601945814499781361 - 1*-280302863 = -601945814219478498
        //  ci[0]=-280302862
        //  RealP=4572232401620063669 - 1*2129111626 = 4572232399490952043
        //  cr[0]=2129111625

        ImaP=ImaP + ((long long)ImaP_Hi*(long long)Error_int);
        ImaP_Hi=(long)(ImaP>>31); //Q62 to Q31
        if (ImaP_Hi>2147483647)ImaP_Hi=2147483647;
        if (ImaP_Hi<-2147483647)ImaP_Hi=-2147483647;
        ci[0]=(int)ImaP_Hi;
        RealP=RealP + ((long long)RealP_Hi*(long long)Error_int);
        RealP_Hi=(long)(RealP>>31); //Q62 to Q31
        if (RealP_Hi>2147483647)RealP_Hi=2147483647;
        if (RealP_Hi<-2147483647)RealP_Hi=-2147483647;
        cr[0]=(int)RealP_Hi;
}
#endif

#pragma CODE_SECTION(bz_init, ".text:slow")
void bz_init(){ 
	int i; 
	for (i=0; i<(DATAFIFO_LENGTH>>1); ++i) datafifo.buffer_ul[i] = 0x3f3f3f3f;
	for (i=16; i<32; ++i) datafifo.buffer_ul[i] = (DSP_UINT32)i;
	for (i=0; i<BZ_MAX_CHANNELS; ++i) bz_last[i] = 0; 
	bz_push_mode = BZ_PUSH_MODE_32_START; 
	bz_byte_pos=0;
	datafifo.w_position=datafifo.r_position=0;

        // reset RT_FIFO for push data
        rt_fifo_i=0;
        rt_fifo_ix=0;
        rt_fifo_j=0;

        // DEBUG
        analog.McBSP_FPGA[8] = 0;
        analog.McBSP_FPGA[9] = 0;

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
		case 0:  datafifo.buffer_ul[datafifo.w_position>>1]  =  0xC0000000UL | (delta.ul>>2)       ; bz_byte_pos=1; break;
		case 1:  datafifo.buffer_ul[datafifo.w_position>>1] |= (0xC0000000UL | (delta.ul>>2)) >>  8; bz_byte_pos=2; break;
		case 2:  datafifo.buffer_ul[datafifo.w_position>>1] |= (0xC0000000UL | (delta.ul>>2)) >> 16; bz_byte_pos=3; break;
		default: datafifo.buffer_ul[datafifo.w_position>>1] |= (0xC0000000UL | (delta.ul>>2)) >> 24; bz_byte_pos=0;
			datafifo.w_position += 2;
			datafifo.w_position &= DATAFIFO_MASK;
			break;
		}
	} else if (bits > 18){ // 16 bit package, 14 bit data
		delta.l <<= 18;
//		std::cout << ">18**16:14bit** BZdelta.l=" << std::hex << delta.l << std::dec << " bz_byte_pos=" << bz_byte_pos << std::endl;;
 		switch (bz_byte_pos){
		case 0:  datafifo.buffer_ul[datafifo.w_position>>1]  =  0x80000000UL | (delta.ul>>2)       ; bz_byte_pos=2; break;
		case 1:  datafifo.buffer_ul[datafifo.w_position>>1] |= (0x80000000UL | (delta.ul>>2)) >>  8; bz_byte_pos=3; break;
		case 2:  datafifo.buffer_ul[datafifo.w_position>>1] |= (0x80000000UL | (delta.ul>>2)) >> 16; bz_byte_pos=0;
			datafifo.w_position += 2;
			datafifo.w_position &= DATAFIFO_MASK;
			break;
		default: datafifo.buffer_ul[datafifo.w_position>>1] |= (0x80000000UL | (delta.ul>>2)) >> 24; 
			datafifo.w_position += 2;
			datafifo.w_position &= DATAFIFO_MASK;
			datafifo.buffer_ul[datafifo.w_position>>1] = ((delta.ul>>2)&0x00ff0000UL) << 8; bz_byte_pos=1; break;
		}
	} else if (bits > 10){ // 24 bit package, 22 bit data
		delta.l <<= 10;
//		std::cout << ">10**24:22bit** BZdelta.l=" << std::hex << delta.l << std::dec << " bz_byte_pos=" << bz_byte_pos << std::endl;
		switch (bz_byte_pos){
		case 0:  datafifo.buffer_ul[datafifo.w_position>>1]  =  0x40000000UL | (delta.ul>>2)       ; bz_byte_pos=3; break;
		case 1:  datafifo.buffer_ul[datafifo.w_position>>1] |= (0x40000000UL | (delta.ul>>2)) >>  8; bz_byte_pos=0;
			datafifo.w_position += 2;
			datafifo.w_position &= DATAFIFO_MASK;
			break;
		case 2:  datafifo.buffer_ul[datafifo.w_position>>1] |= (0x40000000UL | (delta.ul>>2)) >> 16;
			datafifo.w_position += 2;
			datafifo.w_position &= DATAFIFO_MASK;
			datafifo.buffer_ul[datafifo.w_position>>1] = ((delta.ul>>2)&0x0000ff00UL) << 16; bz_byte_pos=1; break;
		default: datafifo.buffer_ul[datafifo.w_position>>1] |= (0x40000000UL | (delta.ul>>2)) >> 24;
			datafifo.w_position += 2;
			datafifo.w_position &= DATAFIFO_MASK;
			datafifo.buffer_ul[datafifo.w_position>>1] = ((delta.ul>>2)&0x00ffff00UL) << 8; bz_byte_pos=2; break;
		}
	} else { // full 32 bit data -- special on byte alignment -- 6 spare unused bits left!
	bz_push_mode_32:
		delta.l = x;
//		std::cout << "ELSE**40:32bit** BZdelta.l=" << std::hex << delta.l << std::dec << " bz_byte_pos=" << bz_byte_pos << std::endl;
//		bits=datafifo.w_position>>1;

		switch (bz_byte_pos){
		case 0:  datafifo.buffer_ul[datafifo.w_position>>1]  =  bz_push_mode | (delta.ul >> 8); // 0xMMffffff..
			datafifo.w_position += 2;
			datafifo.w_position &= DATAFIFO_MASK;
			datafifo.buffer_ul[datafifo.w_position>>1] = delta.ul << 24; bz_byte_pos=1; break;
		case 1:  datafifo.buffer_ul[datafifo.w_position>>1] |= (bz_push_mode | (delta.ul >> 8)) >> 8; // 0x==MMffff....
			datafifo.w_position += 2;
			datafifo.w_position &= DATAFIFO_MASK;
			datafifo.buffer_ul[datafifo.w_position>>1] = delta.ul << 16; bz_byte_pos=2; break;
		case 2:  datafifo.buffer_ul[datafifo.w_position>>1] |= (bz_push_mode | (delta.ul >> 8)) >> 16; // 0x====MMff......
			datafifo.w_position += 2;
			datafifo.w_position &= DATAFIFO_MASK;
			datafifo.buffer_ul[datafifo.w_position>>1] = delta.ul <<  8; bz_byte_pos=3; break;
		default: datafifo.buffer_ul[datafifo.w_position>>1] |= bz_push_mode >> 24; // 0x====MM........
			datafifo.w_position += 2;
			datafifo.w_position &= DATAFIFO_MASK;
			datafifo.buffer_ul[datafifo.w_position>>1] = delta.ul;
			datafifo.w_position += 2;
			datafifo.w_position &= DATAFIFO_MASK; bz_byte_pos=0;
			break;
		}
	}
//		Xdumpbuffer ((unsigned char*)&datafifo.buffer_ul[bits-4], 32, 0);
}

/* calc of f_dx/y and num_steps by host! */
#pragma CODE_SECTION(init_area_scan, ".text:slow")
void init_area_scan (int smode){
	// wait until fifo read thread on host is ready
	if (!datafifo.stall) {
		// reset FIFO -- done by host

		init_probe_fifo (); // reset probe fifo once at start!

                if (scan.start &  AREA_SCAN_RUN_FAST){
                        AS_deltasRe = scan.Zoff_2nd_xp; // shared variables, no 2nd line mode in fast scan!
                        AS_deltasIm = scan.Zoff_2nd_xm;
                        // #define CPN(N) ((double)(1LL<<(N))-1.)
                        // deltasRe = round (CPN(PREC) * cos (2.*M_PI*freq[0]/150000.)); -- here 2147483647 * cos (2pi/N), N=n*dnx*2+dny
                        // deltasIm = round (CPN(PREC) * sin (2.*M_PI*freq[0]/150000.)); -- ... sin
                        AS_cr = -2147483647; // staring sine at 6 O'Clock or at -1.00 or -(1<<31) int equiv.
                        AS_ci = 0;
                        AS_amplitude = (long long)((scan.dnx * scan.nx * scan.cfs_dx)>>1);
                        AS_offset    = 0;
                }
		// calculate num steps to go to starting point -- done by host
		
		// init probe trigger count (-1 will disable it)
		AS_mod = scan.dnx_probe;
		if (AS_mod > 0){
			AS_ip = 1;
			AS_jp = 0;
			AS_kp = 0;
		} else {
			AS_jp = -1;
			AS_ip = -1;
		}
		AS_ch2nd_scan_switch = AS_SCAN_YP; // just go to next line by default
		AS_ch2nd_constheight_enabled = 0;  // disable first
		
		// 2nd stage line scan, memorized Z OK -- here we have a size limit due to the buffer used! ?
		if (scan.srcs_2nd_xp || scan.srcs_2nd_xm)
			if (scan.nx < Z_DATA_BUFFER_SIZE)
				AS_ch2nd_scan_switch = AS_SCAN_2ND_XP; // enable 2nd scan line mode
		
		if ((scan.srcs_xp & 0x08000) || (scan.srcs_xm & 0x08000)){ // if Counter channel requested, restart counter/timer now
			analog.counter[0] = 0;
			analog.counter[1] = 0;
		}

		bz_init();

		Z_data_buffer = (DSP_INT32*)prbdf; // using probe buffer -- shared!

		// enable first subtask state for vector scan state machine
		scan.sstate = AS_MOVE_XY;
                scan.section=0;
                scan.start = 0;
                scan.stop  = 0;
		scan.pflg  = smode;
	}
}

inline void area_scan_finished (){
        scan.pflg = 0;
        scan.stop = AREA_SCAN_STOP;
        scan.dnx_probe = 0;
}
 
#pragma CODE_SECTION(finish_area_scan, ".text:slow")
void finish_area_scan (){
	int i;

	bz_push_mode = BZ_PUSH_MODE_32_FINISH;
	for (i=0; i<BZ_MAX_CHANNELS; ++i)
		bz_push (i, 0);
	scan.sstate = AS_READY;
	scan.pflg = 0;
        scan.section=0;
}

void clear_summing_data (){
	AS_AIC_data_sum[0] = AS_AIC_data_sum[1] = AS_AIC_data_sum[2] =
	AS_AIC_data_sum[3] = AS_AIC_data_sum[4] = AS_AIC_data_sum[5] =
	AS_AIC_data_sum[6] = AS_AIC_data_sum[7] = AS_AIC_data_sum[8] = 0;
	analog.counter[0] = 0;
	AS_AIC_num_samples = 0;
}

void integrate_as_data_srcs (DSP_UINT32 srcs){

        // initiate external data SPI request and transfer
        if (AS_AIC_num_samples == 0 && !(state.dp_task_control[9].process_flag&0xffff))
                initiate_McBSP_transfer (((scan.iy&0xffff)<<16) | (scan.ix&0xffff));

        if (srcs & 0x01) // PIDSrcA1 (Dest) --> Zmonitor (Topo) feedback generated Z signal
		AS_AIC_data_sum[8] += z_servo.control >> 16;

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

#ifdef DATAPROCESS_PUSH_INSTANT
// do now
void push_area_scan_data (DSP_UINT32 srcs){
	DSP_UINT32 tmp;

	// read and buffer (for Rate Meter, gatetime not observed -- always last completed count)
	CR_generic_io.count_0 = analog.counter[0];

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
			bz_push (1, z_servo.control);
		if (srcs & 0x0002) // MIXER selection 1
			bz_push (2, feedback_mixer.FB_IN_processed[1]);
		if (srcs & 0x0004) // MIXER selection 2
			bz_push (3, feedback_mixer.FB_IN_processed[2]);
		if (srcs & 0x0008) // MIXER selection 3
			bz_push (4, feedback_mixer.FB_IN_processed[3]);
		if (srcs & 0x0010) // DataSrcA1 --> AIC0
			bz_push (5, feedback_mixer.FB_IN_processed[0]); // *** (int)AS_AIC_data_sum[0]);
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
			bz_push (13, *scan.src_input[0]);
		if (srcs & 0x02000) // DataSrcD1 --> LockIn2ndA [default maped signal]
			bz_push (14, *scan.src_input[1]);
		if (srcs & 0x04000) // DataSrcE1 --> LockIn0 [default maped signal]
			bz_push (15, *scan.src_input[2]); 
		if (srcs & 0x08000) // "DataSrcF1" last CR Counter count [default maped signal]
			bz_push (16, *scan.src_input[3]);
	}

	// auto clear now including counter0
	clear_summing_data ();
	analog.counter[0] = 0;
	analog.counter[1] = 0;
}

int bz_push_area_scan_data_out (void){ return 0; }

#else

// push to intermediate FIFO for ideal processing
void push_area_scan_data (DSP_UINT32 srcs){
	// read and buffer (for Rate Meter, gatetime not observed -- always last completed count)
	CR_generic_io.count_0 = analog.counter[0];

	if (srcs & 0xFFF1){
                rt_fifo_push_data[0][rt_fifo_i] = (srcs << 16) | AS_AIC_num_samples;
        
                if (srcs & 0x0001) // PIDSrc1/Dest <-- Z = -AIC_Z-value ** AS_AIC_data_sum[8]/AS_AIC_num_samples
                        rt_fifo_push_data[1][rt_fifo_i] = z_servo.control;
                if (srcs & 0x0002) // MIXER selection 1
                        rt_fifo_push_data[2][rt_fifo_i] = feedback_mixer.FB_IN_processed[1];
                if (srcs & 0x0004) // MIXER selection 2
                        rt_fifo_push_data[3][rt_fifo_i] = feedback_mixer.FB_IN_processed[2];
                if (srcs & 0x0008) // MIXER selection 3
                        rt_fifo_push_data[4][rt_fifo_i] = feedback_mixer.FB_IN_processed[3];
                if (srcs & 0x0010) // DataSrcA1 --> AIC0
                        rt_fifo_push_data[5][rt_fifo_i] = feedback_mixer.FB_IN_processed[0]; // *** (int)AS_AIC_data_sum[0];
                if (srcs & 0x0020) // DataSrcA2 --> AIC1
                        rt_fifo_push_data[6][rt_fifo_i] = (int)AS_AIC_data_sum[1];
                if (srcs & 0x0040) // DataSrcA3 --> AIC2
                        rt_fifo_push_data[7][rt_fifo_i] = (int)AS_AIC_data_sum[2];
                if (srcs & 0x0080) // DataSrcA4 --> AIC3
                        rt_fifo_push_data[8][rt_fifo_i] = (int)AS_AIC_data_sum[3];
                if (srcs & 0x0100) // DataSrcB1 --> AIC4
                        rt_fifo_push_data[9][rt_fifo_i] = (int)AS_AIC_data_sum[4];
                if (srcs & 0x0200) // DataSrcB2 --> AIC5
                        rt_fifo_push_data[10][rt_fifo_i] = (int)AS_AIC_data_sum[5];
                if (srcs & 0x0400) // DataSrcB3 --> AIC6
                        rt_fifo_push_data[11][rt_fifo_i] = (int)AS_AIC_data_sum[6];
                if (srcs & 0x0800) // DataSrcB4 --> AIC7
                        rt_fifo_push_data[12][rt_fifo_i] = (int)AS_AIC_data_sum[7]; // debugging test

                if (srcs & 0x01000) // DataSrcC1 --> LockIn1stA [default maped signal]
                        rt_fifo_push_data[13][rt_fifo_i] = *scan.src_input[0];
                if (srcs & 0x02000) // DataSrcD1 --> LockIn2ndA [default maped signal]
                        rt_fifo_push_data[14][rt_fifo_i] = *scan.src_input[1];
                if (srcs & 0x04000) // DataSrcE1 --> LockIn0 [default maped signal]
                        rt_fifo_push_data[15][rt_fifo_i] = *scan.src_input[2];
                if (srcs & 0x08000) // "DataSrcF1" last CR Counter count [default maped signal]
                        rt_fifo_push_data[16][rt_fifo_i] = *scan.src_input[3];

                rt_fifo_i++;
                rt_fifo_ix++;
                rt_fifo_i &= RT_FIFO_MASK;
        }
        
	// auto clear now including counter0
	clear_summing_data ();
	analog.counter[0] = 0;
	analog.counter[1] = 0;
}

// called from idle task control with highest priority
int bz_push_area_scan_data_out (void){
        DSP_UINT32 srcs;
        int i,m;
        int processed=0;
        // using bz_push for bit packing and delta signal usage
        // init/reinit data set?
        while (rt_fifo_ix > rt_fifo_j ){
                // looping?
                if (rt_fifo_j == 0){
                        rt_fifo_ix &= RT_FIFO_MASK;
                        if (rt_fifo_ix <= rt_fifo_j )
                                break;
                }
                srcs = rt_fifo_push_data[0][rt_fifo_j]>>16; 
                if (rt_fifo_push_data[0][rt_fifo_j] != bz_last[0]){ // re init using BZ_PUSH_MODE_32_START to indicate extra info set
                        bz_push_mode = BZ_PUSH_MODE_32_START; 
                        bz_push (0, rt_fifo_push_data[0][rt_fifo_j]);
                        bz_push_mode = BZ_PUSH_NORMAL;
                }

                for (i=m=1; m <= 0x08000; i++, m <<= 1){
                        if (srcs & m)
                                bz_push (i, rt_fifo_push_data[i][rt_fifo_j]);
                }
                
                rt_fifo_j++;
                rt_fifo_j &= RT_FIFO_MASK;
                processed++;
        }
        return processed;
}
#endif

// TESTING IF SLOW WORKS HERE *****!!!!!!!!!!
#pragma CODE_SECTION(run_area_scan, ".text:slow")
void run_area_scan (){
	switch (scan.sstate){
	case AS_SCAN_XP: // "X+" -> scan and dataaq.
		if (--scan.iiix > 0)
			break;
                if (AS_ch2nd_constheight_enabled)
                        scan.iiix = scan.slow_down_factor_2nd;
                else
                        scan.iiix = scan.slow_down_factor;

		if (scan.iix < scan.dnx)
			integrate_as_data_srcs (scan.srcs_xp);
//		scan.xyz_vec[i_X] += scan.fs_dx;
		scan.xyz_vec[i_X] = _SADD32 (scan.xyz_vec[i_X], scan.cfs_dx); // this is with SAT!!
		if (!scan.iix--){
			if (scan.ix--){
#ifdef ENABLE_SCAN_GP53_CLOCK
                                // generate external pixel data clock on GP53
                                if (scan.ix&1)
                                        SET_DSP_GP53;
                                else
                                        CLR_DSP_GP53;
#endif
#if 0
				if (AS_ip >= 0 && (AS_jp == 0 || scan.raster_a) && (scan.dnx_probe > 0))
#else
                                if (AS_ip >= 0)
#endif
                                {
                                        if (! --AS_ip){ // trigger probing process ?
						AS_ip = scan.dnx_probe;
                                                switch_rt_task_areascan_to_probe();
					}
				}
				scan.iix  = scan.dnx-1;
				
				if (AS_ch2nd_constheight_enabled){
                                        // use memorized ZA profile + offset
                                        run_servo_manual (&z_servo, _sadd (Z_data_buffer [scan.ix & Z_DATA_BUFFER_MASK], scan.Zoff_2nd_xp));
					push_area_scan_data (scan.srcs_2nd_xp); // get 2nd XP data here!
				}else{
					if (AS_ch2nd_scan_switch == AS_SCAN_2ND_XP) // only if enabled 2nd scan line mode
						Z_data_buffer [scan.ix & Z_DATA_BUFFER_MASK] = z_servo.i_sum; // memorize Z profile

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
                                ++scan.section; // next bias is list
			}
			scan.cfs_dx = scan.fs_dx;
		}
		break;

	case AS_SCAN_XM: // "X-" <- scan and dataaq.
		scan.ifr = scan.fast_return;
		do{ // fast rep this for return speedup option
			if (scan.iix < scan.dnx)
				integrate_as_data_srcs (scan.srcs_xm);
//			scan.xyz_vec[i_X] -= scan.fs_dx;
			scan.xyz_vec[i_X] = _SSUB32 (scan.xyz_vec[i_X], scan.cfs_dx);
			if (!scan.iix--){
				if (scan.ix--){
#ifdef ENABLE_SCAN_GP53_CLOCK
                                        // generate external pixel data clock on GP53
                                        if (scan.ix&1)
                                                SET_DSP_GP53;
                                        else
                                                CLR_DSP_GP53;
#endif
                                        scan.iix   = scan.dnx-1;
					
					if (AS_ch2nd_constheight_enabled){
                                                // use memorized ZA profile + offset
                                                run_servo_manual (&z_servo, _sadd (Z_data_buffer [(scan.nx-scan.ix-1) & Z_DATA_BUFFER_MASK], scan.Zoff_2nd_xm));
						push_area_scan_data (scan.srcs_2nd_xm); // get 2nd XP data here!
					}else{
						push_area_scan_data (scan.srcs_xm); // get XM data here!
					}
				}
				else{
					if (!scan.iy){ // area scan done?
						area_scan_finished ();
						goto finish_now;
					}
					scan.iiy = scan.dny;
					scan.cfs_dy = scan.fs_dy;
					if (AS_ch2nd_constheight_enabled) {
						scan.sstate = AS_SCAN_YP;
						AS_ch2nd_constheight_enabled = 0;  // disable now, start normal
					}else{
						scan.sstate = AS_ch2nd_scan_switch; // switch to next YP or 2nd stage scan line
                                        }
				}
				scan.cfs_dx = scan.fs_dx;
			}
		} while (--scan.ifr > 0 && scan.sstate == AS_SCAN_XM);
        finish_now:
 		break;

	case AS_SCAN_2ND_XP: // configure 2nd XP scan
		AS_ch2nd_constheight_enabled = 1;  // enable now
		// disable feedback!!
		// config normal XP again
		scan.ix  = scan.nx;
		scan.iix = scan.dnx + PIPE_LEN;
		scan.cfs_dx = scan.fs_dx;
		scan.sstate = AS_SCAN_XP;
                scan.section++; // next bias section
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
                scan.section=0; // reset section to default bias
		if (scan.iiy--){
//			scan.xyz_vec[i_Y] -= scan.fs_dy;
			scan.xyz_vec[i_Y] = _SSUB32 (scan.xyz_vec[i_Y], scan.cfs_dy);
		}
		else{
			if (scan.iy--){
#ifdef ENABLE_SCAN_GP54_CLOCK
                                // generate external line sync clock on GP54
                                if (scan.iy&1)
                                        SET_DSP_GP54;
                                else
                                        CLR_DSP_GP54;
#endif
				if (AS_jp >= 0){
#if 1
                                        if (++AS_jp == scan.raster_b){
                                                AS_jp = 0;
                                                AS_ip = 1; // scan.dnx_probe; // reset X grid counter
                                        } else {
                                                AS_ip = -1;
                                        }
#else
					if (scan.raster_a){
						AS_ip = AS_kp;
						if ((++AS_jp) & 1){ // set start if X grid counter
							if (++AS_kp >= AS_mod)
								AS_kp=0;
							AS_ip += scan.raster_a;
						}
						if (AS_ip++ >= AS_mod)
							AS_ip -= AS_mod;
					} else {
						if (++AS_jp == scan.dnx_probe){
							AS_jp = 0;
							AS_ip = 1; // scan.dnx_probe; // reset X grid counter
						}
                                        }
#endif
				}
				scan.ix  = scan.nx;
				scan.iix = scan.dnx + PIPE_LEN;
				scan.sstate = AS_SCAN_XP;
				clear_summing_data ();
			}
			else // area scan is done -- double check here.
				area_scan_finished ();
			scan.cfs_dx = scan.fs_dx;
		}
		break;

//	case AS_SCAN_YM: // "Y-" -- not valid yet -- exit AS
//		area_scan_finished ();
//		break;

	case AS_MOVE_XY: // move-XY:  and Z -- init/finalize scan, move to start/end of scan position and generic tip positioning withing scan coordinates, 
		// can be used with nx==0, then the job is finished after the xy move!!
		if (scan.num_steps_move_xy){
                        // MOVE XY
                        scan.xyz_vec[i_X] = _SADD32 (scan.xyz_vec[i_X], scan.fm_dx);
                        scan.xyz_vec[i_Y] = _SADD32 (scan.xyz_vec[i_Y], scan.fm_dy);
                        // Z-ADJUST?
			if (!(state.mode & MD_ZPOS_ADJUSTER)){ // is MOVE_XY Z allowed? Then may manipulate Zpos as well and store data
				// optional (if fm_dz non Null) Z-pos manipulation within safe configurable guards
				if (*probe.limiter_input < *probe.limiter_updn[0] && *probe.limiter_input > *probe.limiter_updn[1]){
					probe.Zpos = _SADD32 (probe.Zpos, scan.fm_dz); // manipulate Z
				}
				push_area_scan_data (scan.srcs_xp); // now also store channel data along trajectory!
				clear_summing_data ();
			}
			scan.num_steps_move_xy--;
		} else {
			if (scan.pflg & AREA_SCAN_MOVE_TIP){ // was MOVE_XY only?, then done
				area_scan_finished ();
			} else {
				scan.sstate = AS_SCAN_XP;
				scan.iix    = scan.dnx + PIPE_LEN;
                                scan.iiix   = scan.slow_down_factor;
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
		area_scan_finished ();
		break;
	}
}

// fast scan with X ramp mode as perfect harmonic Sine wave, no gizmos like LockIn, 2nd-line or probe.
// CHECK SineSDB for compatibility with new 1F2F PAC PLL lib
// with 2f: void SineSDB(int *cr, int *ci, int *cr2, int *ci2, int deltasRe, int deltasIm)
#pragma CODE_SECTION(run_area_scan_fast, ".text:slow")
void run_area_scan_fast (){
	if (scan.sstate != AS_MOVE_XY){
	  //	SineSDB (&AS_cr, &AS_ci, AS_deltasRe, AS_deltasIm); // must match scan "speed" settings to be in sync. ==> N=n*dnx*2+dny
	        SineSDB (&AS_cr, &AS_ci, &AS_cr2, &AS_ci2, AS_deltasRe, AS_deltasIm); // must match scan "speed" settings to be in sync. ==> N=n*dnx*2+dny
		scan.xyz_vec[i_X] = _SADD32 ((int)((AS_amplitude * (long long)AS_cr)>>31), AS_offset); // adjust "volume" to X scan range, add offset
	}
	switch (scan.sstate){
	case AS_SCAN_XP: // "X+" -> scan and dataaq.
		if (scan.iix < scan.dnx)
			integrate_as_data_srcs (scan.srcs_xp);
//		scan.xyz_vec[i_X] = _SADD32 (scan.xyz_vec[i_X], scan.cfs_dx); // this is with SAT!!
		if (!scan.iix--){
			if (scan.ix--){
#ifdef ENABLE_SCAN_GP53_CLOCK
                                // generate external pixel data clock on GP53
                                if (scan.ix&1)
                                        SET_DSP_GP53;
                                else
                                        CLR_DSP_GP53;
#endif                                
				scan.iix    = AS_sdnx-1;
				push_area_scan_data (scan.srcs_xp); // get XP data here!
			}
			else{
				scan.ix  = scan.nx;
				scan.iix = AS_sdnx;
				scan.sstate = AS_SCAN_XM;
				clear_summing_data ();
			}
//			scan.cfs_dx = scan.fs_dx;
		}
		break;

	case AS_SCAN_XM: // "X-" <- scan and dataaq.
		if (scan.iix < scan.dnx)
			integrate_as_data_srcs (scan.srcs_xm);
//		scan.xyz_vec[i_X] = _SSUB32 (scan.xyz_vec[i_X], scan.cfs_dx);
		if (!scan.iix--){
			if (scan.ix--){
#ifdef ENABLE_SCAN_GP53_CLOCK
                                // generate external pixel data clock on GP53
                                if (scan.ix&1)
                                        SET_DSP_GP53;
                                else
                                        CLR_DSP_GP53;
#endif                                
				scan.iix   = AS_sdnx-1;
				push_area_scan_data (scan.srcs_xm); // get XM data here!
			}
			else{
				if (!scan.iy){ // area scan done?
					area_scan_finished ();
					goto finish_now;
				}
				scan.iiy = scan.dny;
				scan.cfs_dy = scan.fs_dy;
				scan.sstate = AS_ch2nd_scan_switch; // switch to next YP or 2nd stage scan line
			}
//			scan.cfs_dx = scan.fs_dx;
		}
        finish_now:
 		break;

	case AS_SCAN_YP: // "Y+" next line (could be Y-up or Y-dn, dep. on sign. of fs_dy!)
		if (scan.iiy--){
//			scan.xyz_vec[i_Y] -= scan.fs_dy;
			scan.xyz_vec[i_Y] = _SSUB32 (scan.xyz_vec[i_Y], scan.cfs_dy);
		}
		else{
			if (scan.iy--){
#ifdef ENABLE_SCAN_GP54_CLOCK
                                // generate external line sync clock on GP54
                                if (scan.iy&1)
                                        SET_DSP_GP54;
                                else
                                        CLR_DSP_GP54;
#endif
				scan.ix     = scan.nx;
				scan.iix    = AS_sdnx = scan.dnx; // in fast sin mode only speed update in line basis possible due to non-linear steps
				scan.sstate = AS_SCAN_XP;

				// update Sine frequency now possible, Sine must (should) be now at 6 O'Clock (-1)
				AS_deltasRe = scan.Zoff_2nd_xp; // shared variables, no 2nd line mode in fast scan!
				AS_deltasIm = scan.Zoff_2nd_xm;
				// make sure to keep in sync in case errors build up
				AS_cr = -2147483647; // staring sine at 6 O'Clock or at -1.00 or -(1<<31) int equiv.
				AS_ci = 0;
				clear_summing_data ();
			}
			else // area scan is done -- double check here.
				area_scan_finished ();
//			scan.cfs_dx = scan.fs_dx;
		}
		break;

//	case AS_SCAN_YM: // "Y-" -- not valid yet -- exit AS
//		area_scan_finished ();
//		break;

	case AS_MOVE_XY: // move-XY: init/finalize scan, move to start/end of scan position, 
		// can be used with nx==0, then the job is finished after the xy move!!
		if (scan.num_steps_move_xy){
			scan.xyz_vec[i_X] = _SADD32 (scan.xyz_vec[i_X], scan.fm_dx);
			scan.xyz_vec[i_Y] = _SADD32 (scan.xyz_vec[i_Y], scan.fm_dy);
			scan.num_steps_move_xy--;
		} else {
			if (scan.pflg & AREA_SCAN_MOVE_TIP){ // was MOVE_XY only?, then done
				area_scan_finished ();
			} else {
				scan.sstate = AS_SCAN_XP;
				scan.iix    = AS_sdnx = scan.dnx;
				scan.ix     = scan.nx;
				scan.iy     = scan.ny;
				scan.fm_dx  = scan.fs_dx;
				scan.fm_dy  = -scan.fs_dy;
//				scan.cfs_dx = scan.fs_dx;
				scan.cfs_dy = scan.fs_dy;
				AS_amplitude = ((long long)scan.dnx * (long long)scan.nx * (long long)scan.cfs_dx) >> 1;
				AS_offset   = _SADD32 (scan.xyz_vec[i_X], (long)AS_amplitude);
				clear_summing_data ();
			}
		}
		break;

	default: // just cancel job in case sth. went wrong!
		area_scan_finished ();
		break;
	}
}

