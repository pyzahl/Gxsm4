/* Gxsm - Gnome X Scanning Microscopy Project
 * universal STM/AFM/SARLS/SPALEED/... controlling and
 * data analysis software
 *
 * DSP tools for Linux
 *
 * Copyright (C) 1999,2000,2001 Percy Zahl
 *
 * Authors: Percy Zahl <zahl@users.sf.net>
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

/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 8 c-style: "K&R" -*- */

/*
 * Mover/Slider controller
 * ------------------------------------------------------------
 * - gen saw tooth like (parabola branches) signal for mover
 * Notes:
 * - signal frq is depend on speed AND amplitude, because of 
 *   the simple algorithm used here...
 * - runs now via Offset -- always full range gain!
 */

#ifdef BESOCKE_MOVER
void mover_isr ( void ){
	if(! (STMMode & MD__AFMADJ)) return;

	afm_u_piezo = AFM_MV_count*AFM_MV_count;
	if(afm_u_piezo >= afm_u_piezomax){
		AFM_MV_count -= afm_piezo_speed;
		afm_u_piezo = AFM_MV_count*AFM_MV_count;
		AFM_MV_dir=1;
	}
	if(AFM_MV_dir){
		AFM_MV_count -= afm_piezo_speed;
		afm_u_piezo   = afm_piezo_amp - afm_u_piezo;
	} else
		AFM_MV_count += afm_piezo_speed;

	if(!AFM_MV_count){
		AFM_MV_dir=0;
		++AFM_MV_Scount;
		if(AFM_MV_Scount >= afm_piezo_steps){
			afm_mover_mode = AFM_MOV_RESET; /* reset X&Y voltages */
			STMMode = LetzterSTMMode; /* alten Mode restaurieren */
		}
	}
	switch(afm_mover_mode){
	case AFM_MOV_RESET:  /* reset X&Y voltages */
		DACoutX(0);
		DACoutY(0);
# ifdef USE_U_FOR_MOVER_APP
		DACoutU(0);
# endif
		break;
	case AFM_MOV_XP: DACoutX( afm_u_piezo-afm_u_piezomax); break;
	case AFM_MOV_XM: DACoutX(-afm_u_piezo+afm_u_piezomax); break;
	case AFM_MOV_YP: DACoutY( afm_u_piezo-afm_u_piezomax); break;
	case AFM_MOV_YM: DACoutY(-afm_u_piezo+afm_u_piezomax); break;
# ifdef USE_U_FOR_MOVER_APP
	case AFM_MOV_QP: DACoutU( afm_u_piezo-afm_u_piezomax); break;
	case AFM_MOV_QM: DACoutU(-afm_u_piezo+afm_u_piezomax); break;
# endif
	default: break;
	} 
}
#endif // BESOCKE_MOVER

