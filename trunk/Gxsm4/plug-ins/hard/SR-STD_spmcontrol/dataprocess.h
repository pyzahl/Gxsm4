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

#ifndef __DATAPROCESS_H
#define __DATAPROCESS_H

#define AIC_OFFSET_COMPENSATION  0
#define AIC_OFFSET_CALCULATION   1
#define AIC_DATA_PROCESS_NORMAL  2
#define AIC_REQUEST_RECONFIG     3

#define EXTERNAL_FEEDBACK  0
#define EXTERNAL_OFFSET_X  1
#define EXTERNAL_LOG       2
#define EXTERNAL_DELTA     3



#define	TIM1	0x30

struct 	iobufdef {
 	int	min0;
	int	min1;
	int	min2;
	int	min3;
	int	min4;
	int	min5;
	int	min6;
	int	min7;
	int	mout0;
	int	mout1;
	int	mout2;
	int	mout3;
	int	mout4;
	int	mout5;
	int	mout6;
	int	mout7;
};

struct aicregdef {
	int     valprd;
	int	reg10;
	int	reg11;
	int	reg12;
	int	reg13;
	int	reg14;
	int	reg15;
	int	reg16;
	int	reg17;
	int	reg20;
	int	reg21;
	int	reg22;
	int	reg23;
	int	reg24;
	int	reg25;
	int	reg26;
	int	reg27;
	int	reg30;
	int	reg31;
	int	reg32;
	int	reg33;
	int	reg34;
	int	reg35;
	int	reg36;
	int	reg37;
	int	reg40;
	int	reg41;
	int	reg42;
	int	reg43;
	int	reg44;
	int	reg45;
	int	reg46;
	int	reg47;
};

/* external functions */

interrupt void dataprocess ();
extern void asm_feedback_lin ();
extern void asm_feedback_log ();
extern void run_offset_move ();


extern	struct	iobufdef    iobuf;

#endif						
