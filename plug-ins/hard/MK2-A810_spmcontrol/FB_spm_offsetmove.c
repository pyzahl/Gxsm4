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

extern SPM_PI_FEEDBACK  feedback;
extern ANALOG_VALUES    analog;
extern MOVE_OFFSET      move;

long ldc_wait = 0;

/* calc of f_dx/y and num_steps by host! */
void init_offset_move (){
	int m = move.start;
	move.start = 0;
	move.pflg  = m;
}

void run_offset_move (){
	int i;
	if (move.pflg & MODE_OFFSET_MOVE){
		if (move.num_steps){
			for (i=0; i<=i_Y; ++i)
				move.xyz_vec[i] = _lsadd (move.xyz_vec[i], move.f_d_xyz_vec[i]);
			move.num_steps--;
		}
		else{
			move.pflg = 0;
			for (i=0; i<=i_Y; ++i)
				move.xyz_vec[i] = move.xy_new_vec[i];
		}
	} else if (move.pflg & MODE_LDC_MOVE){
		if (ldc_wait > 0){
			--ldc_wait;
		}
		else {
			ldc_wait = move.num_steps;
			for (i=0; i<=i_Z; ++i)
				move.xyz_vec[i] = _lsadd (move.xyz_vec[i], move.f_d_xyz_vec[i]);
		}
	}

	if (move.pflg & MODE_ZOFFSET_MOVE){
		if (move.num_steps){
			if (move.f_d_xyz_vec[i_Z] > 0 && feedback.z < -15000){ // abort - danger!
				move.f_d_xyz_vec[i_Z] = 0;	
				move.pflg &= ~MODE_ZOFFSET_MOVE;
			} else
				move.xyz_vec[i_Z] = _lsadd (move.xyz_vec[i_Z], move.f_d_xyz_vec[i_Z]);
			move.num_steps--;
		} else {
			move.f_d_xyz_vec[i_Z] = 0;	
			move.pflg &= ~MODE_ZOFFSET_MOVE;
		}
	} else if (move.pflg & MODE_ZOFFSET_AUTO_ADJUST){
		if (move.num_steps){ // limit to num_steps
			if (feedback.z > 2500)
				move.f_d_xyz_vec[i_Z] = move.f_d_xyz_vec[i_Z] > 0 ?  move.f_d_xyz_vec[i_Z] :-move.f_d_xyz_vec[i_Z];
			else
				move.f_d_xyz_vec[i_Z] = move.f_d_xyz_vec[i_Z] > 0 ? -move.f_d_xyz_vec[i_Z] : move.f_d_xyz_vec[i_Z];
					
			if (feedback.z < 5000 && feedback.z > 0){ // OK, done.
				move.f_d_xyz_vec[i_Z] = 0;	
				move.pflg &= ~MODE_ZOFFSET_AUTO_ADJUST;
			} else
				move.xyz_vec[i_Z] = _lsadd (move.xyz_vec[i_Z], move.f_d_xyz_vec[i_Z]);
			move.num_steps--;
		} else {
			move.f_d_xyz_vec[i_Z] = 0;	
			move.pflg &= ~MODE_ZOFFSET_AUTO_ADJUST;
		}
	}
}

/* END */
