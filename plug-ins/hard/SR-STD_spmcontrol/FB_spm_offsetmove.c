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

extern ANALOG_VALUES    analog;
extern MOVE_OFFSET      move;

/* calc of f_dx/y and num_steps by host! */
void init_offset_move (){
	move.start = 0;
	move.pflg  = 1;
}

void run_offset_move (){
	if (move.num_steps){
//		move.XPos += move.f_dx;
//		move.YPos += move.f_dy;
		move.XPos = _lsadd (move.XPos, move.f_dx);
		move.YPos = _lsadd (move.YPos, move.f_dy);
		move.num_steps--;
	}
	else{
		move.pflg = 0;
		move.XPos = (long)move.Xnew<<16;
		move.YPos = (long)move.Ynew<<16;
//		move.XPos = _lsshl ((long)move.Xnew, 16);
//		move.YPos = _lsshl ((long)move.Ynew, 16);
	}
//	analog.x_offset = move.XPos>>16;
//	analog.y_offset = move.YPos>>16;
	analog.x_offset = _lsshl (move.XPos, -16);
	analog.y_offset = _lsshl (move.YPos, -16);
}

