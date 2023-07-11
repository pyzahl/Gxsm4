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

/* ignore this module for docuscan
   % PlugInModuleIgnore
*/



#include <locale.h>
#include <libintl.h>
#include <time.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#include "glbvars.h"
#include "action_id.h"
#include "modules/dsp.h"

#include "surface.h"

#include "spm_template_hwi.h"
#include "spm_template_hwi_emulator.h"

extern spm_template_hwi_dev *spm_template_hwi;

extern "C++" {
        extern SPM_Template_Control *Template_ControlClass;
        extern GxsmPlugin spm_template_hwi_pi;
}

void SPM_Template_Control::read_spm_vector_program (){
	// if (!spm_template_hwi) return; 
	// spm_template_hwi->read_dsp_lockin (AC_amp, AC_frq, AC_phaseA, AC_phaseB, AC_lockin_avg_cycels);
	// update ();
}


// some needfull macros to get some readable code
#define CONST_DSP_F16 65536.
#define VOLT2AIC(U)   (int)(main_get_gapp()->xsm->Inst->VoltOut2Dig (main_get_gapp()->xsm->Inst->BiasV2V (U)))
#define DVOLT2AIC(U)  (int)(main_get_gapp()->xsm->Inst->VoltOut2Dig ((U)/main_get_gapp()->xsm->Inst->BiasGainV2V ()))



// make automatic n and dnx from float number of steps, keep n below 1000.
void SPM_Template_Control::make_auto_n_vector_elments (double fnum){
	program_vector.n = 1;
	program_vector.dnx = 0;
	if (fnum >= 1.){
		if (fnum <= 1000.){ // automatic n ramp limiter
			program_vector.n = (gint32)round (fnum);
			program_vector.dnx = 0;
		} else if (fnum <= 10000.){
			program_vector.n = (gint32)round (fnum/10.);
			program_vector.dnx = 10;
		} else if (fnum <= 100000.){
			program_vector.n = (gint32)round (fnum/100.);
			program_vector.dnx = 100;
		} else if (fnum <= 1000000.){
			program_vector.n = (gint32)round (fnum/1000.);
			program_vector.dnx = 1000;
		} else{
			program_vector.n = (gint32)round (fnum/10000.);
			program_vector.dnx = 10000;
		}
	}
	++program_vector.n;
}

// make IV and dz (optional) vector from U_initial, U_final, dZ, n points and V-slope
// Options:
// Ramp-Mode: MAKE_VEC_FLAG_RAMP, auto n computation
// FixV-Mode: MAKE_VEC_FLAG_VHOLD, fix bias, only compute "speed" by Ui,Uf,slope
double SPM_Template_Control::make_Vdz_vector (double Ui, double Uf, double dZ, int n, double slope, int source, int options, double long &duration, make_vector_flags flags){
	double dv = fabs (Uf - Ui);

	if (dv < 1e-6)
		return make_delay_vector ((double)n*256./spm_template_hwi->spm_emu->frq_ref, source, options, duration, flags, n);

	if (flags & MAKE_VEC_FLAG_RAMP || n < 2)
		make_auto_n_vector_elments (dv/slope*spm_template_hwi->spm_emu->frq_ref);
	else {
		program_vector.n = n; // number of data points
                //		++program_vector.n;
		program_vector.dnx = abs((gint32)round ((Uf - Ui)*spm_template_hwi->spm_emu->frq_ref/(slope*program_vector.n))); // number of steps between data points
	}
	double steps = (double)(program_vector.n) * (double)(program_vector.dnx+1);	//total number of steps

	duration += (double long) steps;
	program_vector.srcs = source & 0xffff; // source channel coding
	program_vector.options = options;
	program_vector.repetitions = 0;
	program_vector.ptr_next = 0x0;
	program_vector.ptr_final = flags & MAKE_VEC_FLAG_END ? 0:1; // VPC relative branch to next vector
	program_vector.f_du = flags & MAKE_VEC_FLAG_VHOLD ? 0 : (gint32)round (CONST_DSP_F16*main_get_gapp()->xsm->Inst->VoltOut2Dig ((Uf-Ui)/main_get_gapp()->xsm->Inst->BiasGainV2V ())/(steps));
	program_vector.f_dz = (gint32)round (CONST_DSP_F16*main_get_gapp()->xsm->Inst->ZA2Dig (dZ/(steps)));
	program_vector.f_dx = 0;
	program_vector.f_dy = 0;
	program_vector.f_dx0 = 0;
	program_vector.f_dy0 = 0;
	program_vector.f_dz0 = 0;
	return main_get_gapp()->xsm->Inst->V2BiasV (main_get_gapp()->xsm->Inst->Dig2VoltOut (VOLT2AIC(Ui) + (double)program_vector.f_du*steps/CONST_DSP_F16));
}	

// Copy of Vdz above, but the du steps were used for dx0
double SPM_Template_Control::make_Vdx0_vector (double Ui, double Uf, double dZ, int n, double slope, int source, int options, double long &duration, make_vector_flags flags
                                     ){
        double dv = fabs (Uf - Ui);
        if (flags & MAKE_VEC_FLAG_RAMP || n < 2)
                make_auto_n_vector_elments (dv/slope*spm_template_hwi->spm_emu->frq_ref);
        else {
                program_vector.n = n; // number of data points
                //              ++program_vector.n;
                program_vector.dnx = abs((gint32)round ((Uf - Ui)*spm_template_hwi->spm_emu->frq_ref/(slope*program_vector.n))); // number of steps between data points
        }
        double steps = (double)(program_vector.n) * (double)(program_vector.dnx+1);     //total number of steps

        duration += (double long) steps;
        program_vector.srcs = source & 0xffff; // source channel coding
        program_vector.options = options;
	program_vector.repetitions = 0;
	program_vector.ptr_next = 0x0;
        program_vector.ptr_final = flags & MAKE_VEC_FLAG_END ? 0:1; // VPC relative branch to next vector
        program_vector.f_dx0 = flags & MAKE_VEC_FLAG_VHOLD ? 0 : (gint32)round (CONST_DSP_F16*main_get_gapp()->xsm->Inst->VoltOut2Dig ((Uf-Ui)/main_get_gapp()->xsm->Inst->BiasGainV2V ())/(steps)); // !!!!!x ????
        program_vector.f_dz = (gint32)round (CONST_DSP_F16*main_get_gapp()->xsm->Inst->ZA2Dig (dZ/(steps)));
        program_vector.f_dx = 0;
        program_vector.f_dy = 0;
        program_vector.f_du = 0;
        program_vector.f_dy0 = 0;
        program_vector.f_dz0 = 0;
        return main_get_gapp()->xsm->Inst->V2BiasV (main_get_gapp()->xsm->Inst->Dig2VoltOut (VOLT2AIC(Ui) + (double)program_vector.f_du*steps/CONST_DSP_F16));
}       

// Copy of Vdz above, but the du steps were used for dx0
double SPM_Template_Control::make_dx0_vector (double X0i, double X0f, int n, double slope, int source, int options, double long &duration, make_vector_flags flags
                                    ){
        double dv = fabs (X0f - X0i);
        if (flags & MAKE_VEC_FLAG_RAMP || n < 2)
                make_auto_n_vector_elments (dv/slope*spm_template_hwi->spm_emu->frq_ref);
        else {
                program_vector.n = n; // number of data points
                //              ++program_vector.n;
                program_vector.dnx = abs((gint32)round ((X0f - X0i)*spm_template_hwi->spm_emu->frq_ref/(slope*program_vector.n))); // number of steps between data points
        }
        double steps = (double)(program_vector.n) * (double)(program_vector.dnx+1);     //total number of steps

        duration += (double long) steps;
        program_vector.srcs = source & 0xffff; // source channel coding
        program_vector.options = options;
	program_vector.repetitions = 0;
	program_vector.ptr_next = 0x0;
        program_vector.ptr_final = flags & MAKE_VEC_FLAG_END ? 0:1; // VPC relative branch to next vector
        program_vector.f_dx0 = flags & MAKE_VEC_FLAG_VHOLD ? 0 : (gint32)round (CONST_DSP_F16*main_get_gapp()->xsm->Inst->VoltOut2Dig (X0f-X0i)/(steps));
        program_vector.f_dz = 0;
        program_vector.f_dx = 0;
        program_vector.f_dy = 0;
        program_vector.f_du = 0;
        program_vector.f_dy0 = 0;
        program_vector.f_dz0 = 0;
        return main_get_gapp()->xsm->Inst->V2BiasV (main_get_gapp()->xsm->Inst->Dig2VoltOut (VOLT2AIC(X0i) + (double)program_vector.f_dx0*steps/CONST_DSP_F16));
}       

// make dZ/dX/dY vector from n point (if > 2, else automatic n) and (dX,dY,dZ) slope
double SPM_Template_Control::make_ZXYramp_vector (double dZ, double dX, double dY, int n, double slope, int source, int options, double long &duration, make_vector_flags flags){
	double dr = sqrt(dZ*dZ + dX*dX + dY*dY);

	if (flags & MAKE_VEC_FLAG_RAMP || n<2)
		make_auto_n_vector_elments (dr/slope*spm_template_hwi->spm_emu->frq_ref);
	else {
		program_vector.n = n;
		program_vector.dnx = (gint32)round ( fabs (dr*spm_template_hwi->spm_emu->frq_ref/(slope*program_vector.n)));
		++program_vector.n;
	}
	double steps = (double)(program_vector.n) * (double)(program_vector.dnx+1);

	duration += (double long) steps;
	program_vector.srcs = source & 0xffff;
	program_vector.options = options;
	program_vector.ptr_fb = 0;
	program_vector.repetitions = 0;
	program_vector.ptr_next = 0x0;
	program_vector.ptr_final = flags & MAKE_VEC_FLAG_END ? 0:1; // VPC relative branch to next vector
	program_vector.f_du = 0;
	program_vector.f_dx = (gint32)round (program_vector.n > 1 ? (CONST_DSP_F16*main_get_gapp()->xsm->Inst->XA2Dig (dX) / steps) : 0);
	program_vector.f_dy = (gint32)round (program_vector.n > 1 ? (CONST_DSP_F16*main_get_gapp()->xsm->Inst->YA2Dig (dY) / steps) : 0);
	program_vector.f_dz = (gint32)round (program_vector.n > 1 ? (CONST_DSP_F16*main_get_gapp()->xsm->Inst->ZA2Dig (dZ) / steps) : 0);
	program_vector.f_dx0 = 0;
	program_vector.f_dy0 = 0;
	program_vector.f_dz0 = 0;

	return main_get_gapp()->xsm->Inst->Dig2ZA ((long)round ((double)program_vector.f_dz*steps/CONST_DSP_F16));
}

// make dU/dZ/dX/dY vector for n points and ts time per segment
double SPM_Template_Control::make_UZXYramp_vector (double dU, double dZ, double dX, double dY, double dSig1, double dSig2, int n, int nrep, int ptr_next, double ts, int source, int options, double long &duration, make_vector_flags flags){
	program_vector.n = n;
	if (ts <= (0.01333e-3 * (double)(n))) 
		program_vector.dnx = 0; // do N vectors at full speed, no inbetween steps.
	else
		program_vector.dnx = (gint32)round ( fabs (ts*spm_template_hwi->spm_emu->frq_ref/(double)(program_vector.n)));
	//	++program_vector.n;

	double steps = (double)(program_vector.n) * (double)(program_vector.dnx+1);

	duration += (double long) steps;
	program_vector.srcs = source & 0xffff;
	program_vector.options = options;
	program_vector.ptr_fb = 0;
	program_vector.repetitions = nrep;
	program_vector.ptr_next = ptr_next;
	program_vector.ptr_final = flags & MAKE_VEC_FLAG_END ? 0:1; // VPC relative branch to next vector

	program_vector.f_du = (gint32)round (program_vector.n > 1 ? (CONST_DSP_F16*main_get_gapp()->xsm->Inst->VoltOut2Dig (dU/main_get_gapp()->xsm->Inst->BiasGainV2V ())/(steps)) : 0);
	program_vector.f_dx = (gint32)round (program_vector.n > 1 ? (CONST_DSP_F16*main_get_gapp()->xsm->Inst->XA2Dig (dX) / steps) : 0);
	program_vector.f_dy = (gint32)round (program_vector.n > 1 ? (CONST_DSP_F16*main_get_gapp()->xsm->Inst->YA2Dig (dY) / steps) : 0);
	program_vector.f_dz = (gint32)round (program_vector.n > 1 ? (CONST_DSP_F16*main_get_gapp()->xsm->Inst->ZA2Dig (dZ) / steps) : 0);
	program_vector.f_dx0 = (gint32)round (program_vector.n > 1 ? (CONST_DSP_F16*main_get_gapp()->xsm->Inst->ZA2Dig (dSig1) / steps) : 0);
	program_vector.f_dy0 = (gint32)round (program_vector.n > 1 ? (CONST_DSP_F16*main_get_gapp()->xsm->Inst->ZA2Dig (dSig2) / steps) : 0);
	program_vector.f_dz0 = 0;

	return main_get_gapp()->xsm->Inst->Dig2ZA ((long)round ((double)program_vector.f_dz*steps/CONST_DSP_F16));
}




// make dPhi vector from n point (if > 2, else automatic n) and dPhi slope
double SPM_Template_Control::make_phase_vector (double dPhi, int n, double slope, int source, int options, double long &duration, make_vector_flags flags){
	double dr = dPhi*16.;
	slope *= 16.;

	if (flags & MAKE_VEC_FLAG_RAMP || n<2)
		make_auto_n_vector_elments (dr/slope*spm_template_hwi->spm_emu->frq_ref);
	else {
		program_vector.n = n;
		program_vector.dnx = (gint32)round ( fabs (dr*spm_template_hwi->spm_emu->frq_ref/(slope*program_vector.n)));
		++program_vector.n;
	}
	double steps = (double)(program_vector.n) * (double)(program_vector.dnx+1);

	duration += (double long) steps;
	program_vector.srcs = source & 0xffff;
	program_vector.options = options;
	program_vector.ptr_fb = 0;
	program_vector.repetitions = 0;
	program_vector.ptr_next = 0x0;
	program_vector.ptr_final = flags & MAKE_VEC_FLAG_END ? 0:1; // VPC relative branch to next vector
	program_vector.f_du = 0;
	program_vector.f_dx = 0;
	program_vector.f_dy = 0;
	program_vector.f_dz = 0;
	program_vector.f_dx0 = 0;
	program_vector.f_dy0 = 0;
	program_vector.f_dz0 = (gint32)round (program_vector.n > 1 ? (CONST_DSP_F16 * dr / steps) : 0);

	return round ((double)program_vector.f_dz0*steps/CONST_DSP_F16/16.);
}

// Make a delay Vector
double SPM_Template_Control::make_delay_vector (double delay, int source, int options, double long &duration, make_vector_flags flags, int points){
	if (points > 2){
		double rnum = delay*spm_template_hwi->spm_emu->frq_ref;
		program_vector.n = points;
		program_vector.dnx = (gint32)round (rnum / points - 1.);
		if (program_vector.dnx < 0)
			program_vector.dnx = 0;
	} else
		make_auto_n_vector_elments (delay*spm_template_hwi->spm_emu->frq_ref);

	duration += (double long) (program_vector.n)*(program_vector.dnx+1);
	program_vector.srcs = source & 0xffff;
	program_vector.options = options;
	program_vector.repetitions = 0; // number of repetitions, not used yet
	program_vector.ptr_next = 0x0;  // pointer to next vector -- not used, only for loops
	program_vector.ptr_final = flags & MAKE_VEC_FLAG_END ? 0:1;   // VPC relative branch to next vector
	program_vector.f_du = 0;
	program_vector.f_dz = 0;
	program_vector.f_dx = 0; // x stepwidth, not used for probing
	program_vector.f_dy = 0; // y stepwidth, not used for probing
	program_vector.f_dx0 = 0; // x0 stepwidth, not used for probing
	program_vector.f_dy0 = 0; // y0 stepwidth, not used for probing
	program_vector.f_dz0 = 0; // z0 stepwidth, not used for probing
	return (double)((long)(program_vector.n)*(long)(program_vector.dnx+1))/spm_template_hwi->spm_emu->frq_ref;
}

// Make Vector Table End
void SPM_Template_Control::append_null_vector (int options, int index){
	// NULL vector -- just to clean vector table
	program_vector.n = 0;
	program_vector.dnx = 0;
	program_vector.srcs = 0x0000;
	program_vector.options = options;
	program_vector.repetitions = 0; // number of repetitions
	program_vector.ptr_next = 0;  // END
	program_vector.ptr_final= 0;  // END
	program_vector.f_dx = 0;
	program_vector.f_dy = 0;
	program_vector.f_dz = 0;
	// append 4 NULL-Vectors, just to clean up the end.
	write_program_vector (index);
	write_program_vector (index+1);
	write_program_vector (index+2);
	write_program_vector (index+3);
}

static void via_remote_list_Check_ec(Gtk_EntryControl* ec, remote_args* ra){
	ec->CheckRemoteCmd (ra);
};

// Create Vector Table form Mode (pvm=PV_MODE_XXXXX) and Execute if requested (start=TRUE) or write only
void SPM_Template_Control::write_spm_vector_program (int start, pv_mode pvm){
	int options=0;
	int options_FBon=0;
	int ramp_sources=0x000;
	int ramp_points;
	int recover_options=0;
	int vector_index = 0;
	double long vp_duration_0 = 0;
	double long vp_duration_1 = 0;
	double long vp_duration_2 = 0;
	double long vp_duration = 0;
	double dU_IV=0.;
	double dU_step=0.;
	int vpci;
	double vp_dt_sec=0.;
	gboolean remote_set_post_start = FALSE;
        int warn_flag=FALSE;
	gchar *info=NULL;
	
	if (!spm_template_hwi) return; 

	if (!start) // reset 
		probe_trigger_single_shot = 0;

	switch (pvm){
#if 0
	case PV_MODE_NONE: // write dummy delay and NULL Vector
                g_free (vp_exec_mode_name); vp_exec_mode_name = g_strdup ("VP: NONE");
		options      = (PL_option_flags & FLAG_FB_ON     ? 0      : VP_FEEDBACK_HOLD)
                        | (PL_option_flags & FLAG_INTEGRATE ? VP_AIC_INTEGRATE : 0);
		ramp_sources = PL_option_flags & FLAG_SHOW_RAMP ? vis_Source : 0x000;
		recover_options = 0;

		make_delay_vector (0.1, ramp_sources, options, vp_duration, MAKE_VEC_FLAG_NORMAL);
		write_program_vector (vector_index++);
		make_delay_vector (0.1, ramp_sources, options, vp_duration, MAKE_VEC_FLAG_END);
		write_program_vector (vector_index++);
		append_null_vector (options, vector_index);
		spm_template_hwi->probe_time_estimate = (int)vp_duration; // used for timeout check
		break;
	case PV_MODE_IV: // ------------ Special Vector Setup for IV Probes "Probe ala STM"
                g_free (vp_exec_mode_name); vp_exec_mode_name = g_strdup ("VP: STS");
		options      = (IV_option_flags & FLAG_FB_ON     ? 0      : VP_FEEDBACK_HOLD)
                        | (IV_option_flags & FLAG_INTEGRATE ? VP_AIC_INTEGRATE : 0);
		ramp_sources = IV_option_flags & FLAG_SHOW_RAMP ? vis_Source : 0x000;
		recover_options = 0; // FeedBack On for recovery!

		vpci = vector_index;
		vp_duration_0 =	vp_duration;

		// do we need to split up if crossing zero?
		if (fabs (IV_dz) > 0.001 && ((IV_end[0] < 0. && IV_start[0] > 0.) || (IV_end[0] > 0. && IV_start[0] < 0.)) && (bias != 0.)){
			int vpc=0;
			// compute sections   
			// z = z0 + dz * ( 1 - | U / Bias | )
			// dz_bi := dz*(1-|Ui/Bias|) - 0
			// dz_i0 := dz - dz_bi
			// dz_0f := dz*(1-|Uf/Bias|) - (dz_bi + dz_i0)
			double ui,u0,uf;
			double dz_bi, dz_i0, dz_0f;
			int n12, n23;
			ui = IV_start[0]; 
			u0 = 0.;
			uf = IV_end[0];
			n12 = (int)(round ((double)IV_points[0]*fabs (ui/(uf-ui))));
			n23 = (int)(round ((double)IV_points[0]*fabs (uf/(uf-ui))));

			dz_bi = IV_dz*(1.-fabs (ui/bias));
			dz_i0 = IV_dz - dz_bi;
			dz_0f = IV_dz*(1.-fabs (uf/bias)) - (dz_bi + dz_i0);

			vp_duration_1 =	vp_duration;
			vp_duration_2 =	vp_duration;
				
			make_Vdz_vector (bias, ui, dz_bi, -1, IV_slope_ramp, ramp_sources, options, vp_duration, MAKE_VEC_FLAG_RAMP);
			write_program_vector (vector_index++);

			make_Vdz_vector (ui, u0, dz_i0, n12, IV_slope, vis_Source, options, vp_duration, MAKE_VEC_FLAG_NORMAL);
			write_program_vector (vector_index++);

			make_Vdz_vector (u0, uf, dz_0f, n23, IV_slope, vis_Source, options, vp_duration, MAKE_VEC_FLAG_NORMAL);
			write_program_vector (vector_index++);

			dU_IV = uf-ui; dU_step = dU_IV/IV_points[0];

			if (IV_option_flags & FLAG_DUAL) {
				// run also reverse probe ramp in dual mode
				make_Vdz_vector (uf, u0, -dz_0f, n23, IV_slope, vis_Source, options, vp_duration, MAKE_VEC_FLAG_NORMAL);
				write_program_vector (vector_index++);

				make_Vdz_vector (u0, ui, -dz_i0, n12, IV_slope, vis_Source, options, vp_duration, MAKE_VEC_FLAG_NORMAL);
				write_program_vector (vector_index++);

				// Ramp back to given bias voltage   
				make_Vdz_vector (ui, bias, -dz_bi, -1, IV_slope_ramp, ramp_sources, options, vp_duration, MAKE_VEC_FLAG_RAMP);
				write_program_vector (vector_index++);

			} else {
				make_Vdz_vector (uf, bias, -(dz_bi+dz_i0+dz_0f), -1, IV_slope_ramp, ramp_sources, options, vp_duration, MAKE_VEC_FLAG_RAMP);
				write_program_vector (vector_index++);
			}

			if (IV_repetitions > 1){
				// Final vector, gives the IVC some time to recover   
				make_delay_vector (IV_final_delay, ramp_sources, options, vp_duration, MAKE_VEC_FLAG_NORMAL);
				write_program_vector (vector_index++);
				make_delay_vector (IV_recover_delay, ramp_sources, recover_options, vp_duration, MAKE_VEC_FLAG_NORMAL);
				program_vector.repetitions = IV_repetitions-1;
				program_vector.ptr_next = -vector_index; // go to start
				vp_duration +=	(IV_repetitions-1)*(vp_duration - vp_duration_1);
				write_program_vector (vector_index++);
			} else {
				make_delay_vector (IV_final_delay, ramp_sources, options, vp_duration, MAKE_VEC_FLAG_NORMAL);
				write_program_vector (vector_index++);
				make_delay_vector (IV_recover_delay, ramp_sources, recover_options, vp_duration, MAKE_VEC_FLAG_NORMAL);
				write_program_vector (vector_index++);
			}
			// add automatic conductivity measurement rho(Z) -- HOLD Bias fixed now!
			if (IVdz_repetitions > 0){
				vp_duration_2 =	vp_duration;
				// don't know the reason, but the following delay vector is needed to separate dI/dU and dI/dz
				make_delay_vector (0., ramp_sources, options, vp_duration, MAKE_VEC_FLAG_NORMAL);
				write_program_vector (vector_index++);

				// in case of rep > 1 the DSP will jump back to this point
				vpc = vector_index;
	
				make_Vdz_vector (bias, ui, dz_bi, -1, IV_slope_ramp, ramp_sources, options, vp_duration, (make_vector_flags)(MAKE_VEC_FLAG_RAMP | MAKE_VEC_FLAG_VHOLD));
				write_program_vector (vector_index++);
	
				make_Vdz_vector (ui, u0, dz_i0, n12, IV_slope, vis_Source, options, vp_duration, MAKE_VEC_FLAG_VHOLD);
				write_program_vector (vector_index++);
	
				make_Vdz_vector (u0, uf, dz_0f, n23, IV_slope, vis_Source, options, vp_duration, MAKE_VEC_FLAG_VHOLD);
				write_program_vector (vector_index++);
	
				if (IV_option_flags & FLAG_DUAL) {
					make_Vdz_vector (uf, u0, -dz_0f, n23, IV_slope, vis_Source, options, vp_duration, MAKE_VEC_FLAG_VHOLD);
					write_program_vector (vector_index++);
	
					make_Vdz_vector (u0, ui, -dz_i0, n12, IV_slope, vis_Source, options, vp_duration, MAKE_VEC_FLAG_VHOLD);
					write_program_vector (vector_index++);
	
					make_Vdz_vector (ui, bias, -dz_bi, -1, IV_slope_ramp, ramp_sources, options, vp_duration, (make_vector_flags)(MAKE_VEC_FLAG_RAMP | MAKE_VEC_FLAG_VHOLD));
					write_program_vector (vector_index++);
				} else {
					make_Vdz_vector (uf, bias, -(dz_bi+dz_i0+dz_0f), -1, IV_slope_ramp, ramp_sources, options, vp_duration, (make_vector_flags)(MAKE_VEC_FLAG_RAMP | MAKE_VEC_FLAG_VHOLD));
					write_program_vector (vector_index++);
				}
	
				if (IVdz_repetitions > 1){
					// Final vector, gives the IVC some time to recover   
					make_delay_vector (IV_final_delay, ramp_sources, options, vp_duration, MAKE_VEC_FLAG_NORMAL);
					write_program_vector (vector_index++);
	
					make_delay_vector (IV_recover_delay, ramp_sources, recover_options, vp_duration, MAKE_VEC_FLAG_NORMAL);
					program_vector.repetitions = IVdz_repetitions-1;
					program_vector.ptr_next = -(vector_index-vpc); // go to rho start
					vp_duration +=	(IVdz_repetitions-1)*(vp_duration - vp_duration_2);
					write_program_vector (vector_index++);
				} else {
					make_delay_vector (IV_final_delay, ramp_sources, options, vp_duration, MAKE_VEC_FLAG_NORMAL);
					write_program_vector (vector_index++);
					make_delay_vector (IV_recover_delay, ramp_sources, recover_options, vp_duration, MAKE_VEC_FLAG_NORMAL);
					write_program_vector (vector_index++);
				}
			}

		} else {
			// compute sections   
			// z = z0 + dz * ( 1 - | U / Bias | )
			// dz_bi := dz*(1-|Ui/Bias|) - 0
			double ui,uf;
			double dz_bi, dz_if;
			int vpc=0;

			vp_duration_1 =	vp_duration;
			vp_duration_2 =	vp_duration;

			for (int IVs=0; IVs<IV_sections; ++IVs){
				if (!multiIV_mode && IVs > 0) break;
				ui = IV_start[IVs]; 
				uf = IV_end[IVs];
				double bias_prev = IVs > 0 ? IV_option_flags & FLAG_DUAL ? IV_start[IVs-1]:IV_end[IVs-1]:bias;

				if (bias_prev != 0.){
					dz_bi = IV_dz*(1.-fabs (ui/bias_prev));
					dz_if = IV_dz*(1.-fabs (uf/bias_prev)) - dz_bi;
				} else {
					dz_bi = dz_if = 0.;
				}

				make_Vdz_vector (bias_prev, ui, dz_bi, -1, IV_slope_ramp, ramp_sources, options, vp_duration, MAKE_VEC_FLAG_RAMP);
				write_program_vector (vector_index++);

				make_Vdz_vector (ui, uf, dz_if, IV_points[IVs], IV_slope, vis_Source, options, vp_duration, MAKE_VEC_FLAG_NORMAL);
				write_program_vector (vector_index++);
			
				// info of real values set
				dU_IV   = main_get_gapp()->xsm->Inst->V2BiasV (main_get_gapp()->xsm->Inst->Dig2VoltOut ((long double)program_vector.f_du*(long double)(program_vector.n-1)*(long double)(program_vector.dnx ? program_vector.dnx+1 : 1)/CONST_DSP_F16));
				dU_step = main_get_gapp()->xsm->Inst->V2BiasV (main_get_gapp()->xsm->Inst->Dig2VoltOut ((long double)program_vector.f_du*(long double)(program_vector.dnx ? program_vector.dnx+1 : 1)/CONST_DSP_F16));
			
				// add vector for reverse return ramp? -- Force return path if dz != 0
				if (IV_option_flags & FLAG_DUAL) {
					// run also reverse probe ramp in dual mode
					make_Vdz_vector (uf, ui, -dz_if, IV_points[IVs], IV_slope, vis_Source, options, vp_duration, MAKE_VEC_FLAG_NORMAL);
					write_program_vector (vector_index++);
					
					if (IVs == (IV_sections-1)){
						// Ramp back to given bias voltage   
						make_Vdz_vector (ui, bias, -dz_bi, -1, IV_slope_ramp, ramp_sources, options, vp_duration, MAKE_VEC_FLAG_RAMP);
						write_program_vector (vector_index++);
					}
				} else {
					if (IVs == (IV_sections-1)){
						// Ramp back to given bias voltage   
						make_Vdz_vector (uf, bias, -(dz_if+dz_bi), -1, IV_slope_ramp, ramp_sources, options, vp_duration, MAKE_VEC_FLAG_RAMP);
						write_program_vector (vector_index++);
					}
				}
				if (IV_repetitions > 1){
					// Final vector, gives the IVC some time to recover   
					make_delay_vector (IV_final_delay, ramp_sources, options, vp_duration, MAKE_VEC_FLAG_NORMAL);
					write_program_vector (vector_index++);
					
					if (IVs == (IV_sections-1)){
						make_delay_vector (IV_recover_delay, ramp_sources, recover_options, vp_duration, MAKE_VEC_FLAG_NORMAL);
						program_vector.repetitions = IV_repetitions-1;
						program_vector.ptr_next = -vector_index; // go to start
						vp_duration +=	(IV_repetitions-1)*(vp_duration - vp_duration_1);
						write_program_vector (vector_index++);
					}
				} else {
					make_delay_vector (IV_final_delay, ramp_sources, options, vp_duration, MAKE_VEC_FLAG_NORMAL);
					write_program_vector (vector_index++);
					if (IVs == (IV_sections-1)){
						make_delay_vector (IV_recover_delay, ramp_sources, recover_options, vp_duration, MAKE_VEC_FLAG_NORMAL);
						write_program_vector (vector_index++);
					}
				}
			}

			// add automatic conductivity measurement rho(Z) -- HOLD Bias fixed now!
			if ((IVdz_repetitions > 0) && (fabs (IV_dz) > 0.001) && (bias != 0.)){
				vp_duration_2 =	vp_duration;
				// don't know the reason, but the following delay vector is needed to separate dI/dU and dI/dz
				make_delay_vector (0., ramp_sources, options, vp_duration, MAKE_VEC_FLAG_NORMAL);
				write_program_vector (vector_index++);

				// in case of rep > 1 the DSP will jump back to this point
				vpc = vector_index;
	
				make_Vdz_vector (bias, ui, dz_bi, -1, IV_slope_ramp, ramp_sources, options, vp_duration, (make_vector_flags)(MAKE_VEC_FLAG_RAMP | MAKE_VEC_FLAG_VHOLD));
				write_program_vector (vector_index++);

				make_Vdz_vector (ui, uf, dz_if, IV_points[0], IV_slope, vis_Source, options, vp_duration, MAKE_VEC_FLAG_VHOLD);
				write_program_vector (vector_index++);
			
				if (IV_option_flags & FLAG_DUAL) {
					make_Vdz_vector (uf, ui, -dz_if, IV_points[0], IV_slope, vis_Source, options, vp_duration, MAKE_VEC_FLAG_VHOLD);
					write_program_vector (vector_index++);

					// Ramp back to given bias voltage   
					make_Vdz_vector (ui, bias, -dz_bi, -1, IV_slope_ramp, ramp_sources, options, vp_duration, (make_vector_flags)(MAKE_VEC_FLAG_RAMP | MAKE_VEC_FLAG_VHOLD));
					write_program_vector (vector_index++);
				} else {
					make_Vdz_vector (uf, bias, -(dz_if+dz_bi), -1, IV_slope_ramp, ramp_sources, options, vp_duration, (make_vector_flags)(MAKE_VEC_FLAG_RAMP | MAKE_VEC_FLAG_VHOLD));
					write_program_vector (vector_index++);
				}
	
				if (IVdz_repetitions > 1 ){
					// Final vector, gives the IVC some time to recover   
					make_delay_vector (IV_final_delay, ramp_sources, options, vp_duration, MAKE_VEC_FLAG_NORMAL);
					write_program_vector (vector_index++);
	
					make_delay_vector (IV_recover_delay, ramp_sources, recover_options, vp_duration, MAKE_VEC_FLAG_NORMAL);
					program_vector.repetitions = IVdz_repetitions-1;
					program_vector.ptr_next = -(vector_index-vpc); // go to rho start
					vp_duration +=	(IVdz_repetitions-1)*(vp_duration - vp_duration_2);
					write_program_vector (vector_index++);
				} else {
					make_delay_vector (IV_final_delay, ramp_sources, options, vp_duration, MAKE_VEC_FLAG_NORMAL);
					write_program_vector (vector_index++);
					make_delay_vector (IV_recover_delay, ramp_sources, recover_options, vp_duration, MAKE_VEC_FLAG_NORMAL);
					write_program_vector (vector_index++);
				}
			}
		}

		// Step and Repeat along Line defined by dx dy -- if dxy points > 1, auto return?
		if (IV_dxy_points > 1){

			// Move probe to next position and setup auto repeat!
			make_ZXYramp_vector (0., IV_dx/(IV_dxy_points-1), IV_dy/(IV_dxy_points-1), 100, IV_dxy_slope, 
					     ramp_sources, options, vp_duration, MAKE_VEC_FLAG_RAMP);

			program_vector.f_dz0 = (gint32)round (CONST_DSP_F16*main_get_gapp()->xsm->Inst->VoltOut2Dig (IV_dM/main_get_gapp()->xsm->Inst->BiasGainV2V ()));

			write_program_vector (vector_index++);
			make_delay_vector (IV_dxy_delay, ramp_sources, options, vp_duration, MAKE_VEC_FLAG_RAMP);
			program_vector.repetitions = IV_dxy_points-1;
			program_vector.ptr_next = -(vector_index-vpci); // go to initial IV start for full repeat
			write_program_vector (vector_index++);
			vp_duration +=	(IV_dxy_points-1)*(vp_duration - vp_duration_0);

			// add vector for full reverse return path -- YES!, always auto return!
			make_ZXYramp_vector (0., 
					     -IV_dx*(IV_dxy_points)/(IV_dxy_points-1.), 
					     -IV_dy*(IV_dxy_points)/(IV_dxy_points-1.), 100, IV_dxy_slope, 
					     ramp_sources, options, vp_duration, MAKE_VEC_FLAG_RAMP);
			write_program_vector (vector_index++);
		}

		// Final vector
		make_delay_vector (0., ramp_sources, options, vp_duration, MAKE_VEC_FLAG_END);
		write_program_vector (vector_index++);

		append_null_vector (options, vector_index);

		spm_template_hwi->probe_time_estimate = (int)vp_duration; // used for timeout check


                if (probe_trigger_raster_points_user > 0 && write_vector_mode != PV_MODE_NONE){
                        double T_probe_cycle   = 1e3 * (double)vp_duration/spm_template_hwi->spm_emu->frq_ref; // Time of full probe cycle in ms
                        double T_raster2raster = 1e3 * main_get_gapp()->xsm->data.s.rx / (main_get_gapp()->xsm->data.s.nx/probe_trigger_raster_points_user) / scan_speed_x; // Time inbetween raster points in ms
                        info = g_strdup_printf ("Tp=%.2f ms, Tr=%.2f ms, Td=%.2f ms", T_probe_cycle, T_raster2raster, T_raster2raster - T_probe_cycle);
                } else
                        info = g_strdup_printf ("Tp=%.2f ms, dU=%.3f V, dUs=%.2f mV, O*0x%02x S*0x%06x", 
                                                1e3*(double)vp_duration/spm_template_hwi->spm_emu->frq_ref, dU_IV, dU_step*1e3, options, vis_Source
                                                );
                if (IV_status){
                        gtk_entry_buffer_set_text (GTK_ENTRY_BUFFER (gtk_entry_get_buffer (GTK_ENTRY(IV_status))), info, -1);

#if 0
                        GtkStyle *style;
                        GdkColor ct, cbg;
                        style = gtk_style_copy (gtk_widget_get_style(IV_status));
                        if (warn_flag){
                                ct.red = 0xffff;
                                ct.green = 0x0;
                                ct.blue = 0x0;
                                cbg.red = 0xffff;
                                cbg.green = 0x9999;
                                cbg.blue = 0xffff;
                        }else{
                                ct.red = 0x0;
                                ct.green = 0x0;
                                ct.blue = 0x0;
                                cbg.red = 0xeeee;
                                cbg.green = 0xdddd;
                                cbg.blue = 0xdddd;
                        }
                        // GTK3QQQ AgRArajhagfzjs
                        // gdk_color_alloc (gtk_widget_get_colormap(IV_status), &ct);
                        // gdk_color_alloc (gtk_widget_get_colormap(IV_status), &cbg);
                        style->text[GTK_STATE_NORMAL] = ct;
                        style->bg[GTK_STATE_NORMAL] = cbg;
                        gtk_widget_set_style(IV_status, style);
#endif
                }
		break;




#endif
	case PV_MODE_GVP: // ------------ Special Vector Setup for GVP (general vector probe)
                g_free (vp_exec_mode_name); vp_exec_mode_name = g_strdup ("VP: Vector Program");
		options      = (GVP_option_flags & FLAG_FB_ON     ? 0      : VP_FEEDBACK_HOLD)
                        | (GVP_option_flags & FLAG_INTEGRATE ? VP_AIC_INTEGRATE : 0);
		ramp_sources = GVP_option_flags & FLAG_SHOW_RAMP ? vis_Source : 0x000;

		// setup vector program
		{
			long double vpd[N_GVP_VECTORS];
			int k=0;
			for (k=0; k<N_GVP_VECTORS; ++k){
				vpd[k] = vp_duration;
				int lmopt = GVP_opt[k];
				if (GVP_ts[k] <= 0. && GVP_points[k] == 0) 
					break;
				if (GVP_GPIO_lock != GVP_GPIO_KEYCODE) // arbitrary positive 4 digit key code
					lmopt &= ~VP_GPIO_SET;  // remove VP_GPIO_SET flag!

				// invert FB flag GVP_opt bit VP_FEEDBACK_HOLD is set is FB=ON, VP-options require it to be SET for FB-HOLD!! Invert this bit now HERE:
				lmopt = (lmopt & ~VP_FEEDBACK_HOLD) | (GVP_opt[k] & VP_FEEDBACK_HOLD ? 0 : VP_FEEDBACK_HOLD);

				// override FB on/off option flag here (1 = FB on, 0 = FB off):
				options = (GVP_option_flags & FLAG_INTEGRATE ? VP_AIC_INTEGRATE : 0) | (lmopt) | ((GVP_data[k]&0xff) << 16);
				if (GVP_vnrep[k] < 0)
					GVP_vnrep[k] = 0;
				if (GVP_vpcjr[k] < -vector_index || GVP_vpcjr[k] > 0) // origin of VP, no forward jump
					GVP_vpcjr[k] = -vector_index; // defaults to start
				make_UZXYramp_vector (GVP_du[k], GVP_dz[k], GVP_dx[k], GVP_dy[k], GVP_dsig[k], 0., GVP_points[k], GVP_vnrep[k]-1, GVP_vpcjr[k], GVP_ts[k], vis_Source, options, vp_duration, MAKE_VEC_FLAG_NORMAL);
				write_program_vector (vector_index++);
				if (GVP_vnrep[k]-1 > 0)	
					vp_duration +=	(GVP_vnrep[k]-1)*(vp_duration - vpd[k+GVP_vpcjr[k]]);
			}
		}

		options      = (GVP_option_flags & FLAG_FB_ON     ? 0      : VP_FEEDBACK_HOLD)
                        | (GVP_option_flags & FLAG_INTEGRATE ? VP_AIC_INTEGRATE : 0);
		make_delay_vector (GVP_final_delay, ramp_sources, options, vp_duration, MAKE_VEC_FLAG_END);
		write_program_vector (vector_index++);

		append_null_vector (options, vector_index);

		//spm_template_hwi->probe_time_estimate = (int)vp_duration; // used for timeout check

		info = g_strdup_printf ("T=%.2f ms", 1e3*(double)vp_duration/spm_template_hwi->spm_emu->frq_ref);
		if (GVP_status)
                        gtk_entry_buffer_set_text (GTK_ENTRY_BUFFER (gtk_entry_get_buffer (GTK_ENTRY(GVP_status))), info, -1);
		break;

#if 0
	case PV_MODE_ABORT: // ------------ Special Vector Setup for aborting/killing VP program, protected DSP reset capability
                g_free (vp_exec_mode_name); vp_exec_mode_name = g_strdup ("VP: Abort");
		options      = (ABORT_option_flags & FLAG_FB_ON     ? 0      : VP_FEEDBACK_HOLD)
                        | (ABORT_option_flags & FLAG_INTEGRATE ? VP_AIC_INTEGRATE : 0);
		ramp_sources = ABORT_option_flags & FLAG_SHOW_RAMP ? vis_Source : 0x000;

		if (ABORT_final_delay < 0.){
			ABORT_final_delay = 1.;
			spm_template_hwi->write_dsp_reset ();
		}

		make_delay_vector (ABORT_final_delay, ramp_sources, options, vp_duration, MAKE_VEC_FLAG_END);
		write_program_vector (vector_index++);

		// clear program
		for (; vector_index<36; vector_index+=4)
			append_null_vector (options, vector_index);
		append_null_vector (options, vector_index);

		spm_template_hwi->probe_time_estimate = (int)vp_duration; // used for timeout check

		break;
#endif
	}

	if (start){
                g_message ("Executing Vector Probe Now! Mode: %s", vp_exec_mode_name);
		main_get_gapp()->monitorcontrol->LogEvent ("VectorProbe Execute", vp_exec_mode_name);
		main_get_gapp()->monitorcontrol->LogEvent ("VectorProbe", info, 2);
                // may want to make sure LockIn settings are up to date...
                spm_template_hwi->spm_emu->execute_vector_program(); // non blocking
        }

        g_free (info);

	// --------------------------------------------------

#if 0
	if (remote_set_post_start){
                gchar *line3[] = { g_strdup ("set"), 
                                   g_strdup (gtk_entry_buffer_get_text (GTK_ENTRY_BUFFER (gtk_entry_get_buffer (GTK_ENTRY(PL_remote_set_target))))), 
                                   g_strdup_printf ("%.4f", PL_remote_set_value), 
                                   NULL
                };
                g_slist_foreach(main_get_gapp()->RemoteEntryList, (GFunc) via_remote_list_Check_ec, (gpointer)line3);
                for (int k=0; line3[k]; ++k) g_free (line3[k]);
	}

	// now write probe structure, this may starts probe if "start" is true
	spm_template_hwi->write_dsp_lockin_probe_final (AC_amp, AC_frq, AC_phaseA, AC_phaseB, AC_lockin_avg_cycels, FZ_limiter_val, 
							  noise_amp,
							  start);
#endif

	// Update EC's
	update_GUI();

	// Update from DSP
	read_spm_vector_program ();
}


void SPM_Template_Control::write_program_vector (int index){
        if (!spm_template_hwi) return; 

        spm_template_hwi->spm_emu->write_program_vector (index, &program_vector);


	// update GXSM's internal copy of vector list
	program_vector_list[index].n = program_vector.n;
	program_vector_list[index].dnx = program_vector.dnx;
	program_vector_list[index].srcs = program_vector.srcs;
	program_vector_list[index].options = program_vector.options;
	program_vector_list[index].ptr_fb = program_vector.ptr_fb;
	program_vector_list[index].repetitions = program_vector.repetitions;
	program_vector_list[index].i = 0; //program_vector.i;
	program_vector_list[index].j = 0; //program_vector.j;
	program_vector_list[index].ptr_next = program_vector.ptr_next;
	program_vector_list[index].ptr_final = program_vector.ptr_final;
	program_vector_list[index].f_du = program_vector.f_du;
	program_vector_list[index].f_dx = program_vector.f_dx;
	program_vector_list[index].f_dy = program_vector.f_dy;
	program_vector_list[index].f_dz = program_vector.f_dz;
	program_vector_list[index].f_dx0 = program_vector.f_dx0;
	program_vector_list[index].f_dy0 = program_vector.f_dy0;
	program_vector_list[index].f_dz0 = program_vector.f_dz0;

	{ 
		double mVf = 10000. / (65536. * 32768.);
		gchar *pvi = g_strdup_printf ("ProbeVector[pc%02d]", index);
		gchar *pvd = g_strdup_printf ("(n:%05d, dnx:%05d, 0x%04x, 0x%04x, r:%4d, pc:%d, f:%d),"
					      "(dU:%6.4f mV, dxzy:%6.4f, %6.4f, %6.4f mV, dxy0:%6.4f, %6.4f mV, dP:%.4f)", 
					      program_vector.n, program_vector.dnx,
					      program_vector.srcs, program_vector.options,
					      program_vector.repetitions,
					      program_vector.ptr_next, program_vector.ptr_final,
					      mVf * program_vector.f_du, mVf * program_vector.f_dx, mVf * program_vector.f_dy, mVf * program_vector.f_dz,
					      mVf * program_vector.f_dx0, mVf * program_vector.f_dy0,
					      mVf * program_vector.f_dz0
                                              );

		main_get_gapp()->monitorcontrol->LogEvent (pvi, pvd, 2);
		g_free (pvi);
		g_free (pvd);
	}
}

void SPM_Template_Control::read_program_vector (int index){
        if (!spm_template_hwi) return; 
	spm_template_hwi->spm_emu->read_program_vector (index, &program_vector);
}


void SPM_Template_Control::write_dsp_abort_probe (){
        if (!spm_template_hwi) return; 
	spm_template_hwi->spm_emu->abort_vector_program ();
}

