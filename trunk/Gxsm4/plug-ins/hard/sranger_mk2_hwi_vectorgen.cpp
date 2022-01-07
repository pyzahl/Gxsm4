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

#include "sranger_mk2_hwi_control.h"
#include "sranger_mk23common_hwi.h"
#include "modules/sranger_mk2_ioctl.h"


extern GxsmPlugin sranger_mk2_hwi_pi;
extern sranger_common_hwi_dev *sranger_common_hwi; // instance of the HwI derived XSM_Hardware class


void DSPControl::read_dsp_probe (){
	if (!sranger_common_hwi) return; 
	sranger_common_hwi->read_dsp_lockin (AC_amp, AC_frq, AC_phaseA, AC_phaseB, AC_lockin_avg_cycels);
	update ();
}


// some needfull macros to get some readable code
#define CONST_DSP_F16 65536.
#define VOLT2AIC(U)   (int)(main_get_gapp()->xsm->Inst->VoltOut2Dig (main_get_gapp()->xsm->Inst->BiasV2V (U)))
#define DVOLT2AIC(U)  (int)(main_get_gapp()->xsm->Inst->VoltOut2Dig ((U)/main_get_gapp()->xsm->Inst->BiasGainV2V ()))



// make automatic n and dnx from float number of steps, keep n below 1000.
void DSPControl::make_auto_n_vector_elments (double fnum){
	dsp_vector.n = 1;
	dsp_vector.dnx = 0;
	if (fnum >= 1.){
		if (fnum <= 1000.){ // automatic n ramp limiter
			dsp_vector.n = (gint32)round (fnum);
			dsp_vector.dnx = 0;
		} else if (fnum <= 10000.){
			dsp_vector.n = (gint32)round (fnum/10.);
			dsp_vector.dnx = 10;
		} else if (fnum <= 100000.){
			dsp_vector.n = (gint32)round (fnum/100.);
			dsp_vector.dnx = 100;
		} else if (fnum <= 1000000.){
			dsp_vector.n = (gint32)round (fnum/1000.);
			dsp_vector.dnx = 1000;
		} else{
			dsp_vector.n = (gint32)round (fnum/10000.);
			dsp_vector.dnx = 10000;
		}
	}
	++dsp_vector.n;
}

// make IV and dz (optional) vector from U_initial, U_final, dZ, n points and V-slope
// Options:
// Ramp-Mode: MAKE_VEC_FLAG_RAMP, auto n computation
// FixV-Mode: MAKE_VEC_FLAG_VHOLD, fix bias, only compute "speed" by Ui,Uf,slope
double DSPControl::make_Vdz_vector (double Ui, double Uf, double dZ, int n, double slope, int source, int options, double long &duration, make_vector_flags flags){
	double dv = fabs (Uf - Ui);

	if (dv < 1e-6)
		return make_delay_vector ((double)n*256./frq_ref, source, options, duration, flags, n);

	if (flags & MAKE_VEC_FLAG_RAMP || n < 2)
		make_auto_n_vector_elments (dv/slope*frq_ref);
	else {
		dsp_vector.n = n; // number of data points
                //		++dsp_vector.n;
		dsp_vector.dnx = abs((gint32)round ((Uf - Ui)*frq_ref/(slope*dsp_vector.n))); // number of steps between data points
	}
	double steps = (double)(dsp_vector.n) * (double)(dsp_vector.dnx+1);	//total number of steps

	duration += (double long) steps;
	dsp_vector.srcs = source & 0xffff; // source channel coding
	dsp_vector.options = options;
	dsp_vector.repetitions = 0;
	dsp_vector.ptr_next = 0x0;
	dsp_vector.ptr_final = flags & MAKE_VEC_FLAG_END ? 0:1; // VPC relative branch to next vector
	dsp_vector.f_du = flags & MAKE_VEC_FLAG_VHOLD ? 0 : (gint32)round (CONST_DSP_F16*main_get_gapp()->xsm->Inst->VoltOut2Dig ((Uf-Ui)/main_get_gapp()->xsm->Inst->BiasGainV2V ())/(steps));
	dsp_vector.f_dz = (gint32)round (CONST_DSP_F16*main_get_gapp()->xsm->Inst->ZA2Dig (dZ/(steps)));
	dsp_vector.f_dx = 0;
	dsp_vector.f_dy = 0;
	dsp_vector.f_dx0 = 0;
	dsp_vector.f_dy0 = 0;
	dsp_vector.f_dphi = 0;
	return main_get_gapp()->xsm->Inst->V2BiasV (main_get_gapp()->xsm->Inst->Dig2VoltOut (VOLT2AIC(Ui) + (double)dsp_vector.f_du*steps/CONST_DSP_F16));
}	

// Copy of Vdz above, but the du steps were used for dx0
double DSPControl::make_Vdx0_vector (double Ui, double Uf, double dZ, int n, double slope, int source, int options, double long &duration, make_vector_flags flags
                                     ){
        double dv = fabs (Uf - Ui);
        if (flags & MAKE_VEC_FLAG_RAMP || n < 2)
                make_auto_n_vector_elments (dv/slope*frq_ref);
        else {
                dsp_vector.n = n; // number of data points
                //              ++dsp_vector.n;
                dsp_vector.dnx = abs((gint32)round ((Uf - Ui)*frq_ref/(slope*dsp_vector.n))); // number of steps between data points
        }
        double steps = (double)(dsp_vector.n) * (double)(dsp_vector.dnx+1);     //total number of steps

        duration += (double long) steps;
        dsp_vector.srcs = source & 0xffff; // source channel coding
        dsp_vector.options = options;
	dsp_vector.repetitions = 0;
	dsp_vector.ptr_next = 0x0;
        dsp_vector.ptr_final = flags & MAKE_VEC_FLAG_END ? 0:1; // VPC relative branch to next vector
        dsp_vector.f_dx0 = flags & MAKE_VEC_FLAG_VHOLD ? 0 : (gint32)round (CONST_DSP_F16*main_get_gapp()->xsm->Inst->VoltOut2Dig ((Uf-Ui)/main_get_gapp()->xsm->Inst->BiasGainV2V ())/(steps)); // !!!!!x ????
        dsp_vector.f_dz = (gint32)round (CONST_DSP_F16*main_get_gapp()->xsm->Inst->ZA2Dig (dZ/(steps)));
        dsp_vector.f_dx = 0;
        dsp_vector.f_dy = 0;
        dsp_vector.f_du = 0;
        dsp_vector.f_dy0 = 0;
        dsp_vector.f_dphi = 0;
        return main_get_gapp()->xsm->Inst->V2BiasV (main_get_gapp()->xsm->Inst->Dig2VoltOut (VOLT2AIC(Ui) + (double)dsp_vector.f_du*steps/CONST_DSP_F16));
}       

// Copy of Vdz above, but the du steps were used for dx0
double DSPControl::make_dx0_vector (double X0i, double X0f, int n, double slope, int source, int options, double long &duration, make_vector_flags flags
                                    ){
        double dv = fabs (X0f - X0i);
        if (flags & MAKE_VEC_FLAG_RAMP || n < 2)
                make_auto_n_vector_elments (dv/slope*frq_ref);
        else {
                dsp_vector.n = n; // number of data points
                //              ++dsp_vector.n;
                dsp_vector.dnx = abs((gint32)round ((X0f - X0i)*frq_ref/(slope*dsp_vector.n))); // number of steps between data points
        }
        double steps = (double)(dsp_vector.n) * (double)(dsp_vector.dnx+1);     //total number of steps

        duration += (double long) steps;
        dsp_vector.srcs = source & 0xffff; // source channel coding
        dsp_vector.options = options;
	dsp_vector.repetitions = 0;
	dsp_vector.ptr_next = 0x0;
        dsp_vector.ptr_final = flags & MAKE_VEC_FLAG_END ? 0:1; // VPC relative branch to next vector
        dsp_vector.f_dx0 = flags & MAKE_VEC_FLAG_VHOLD ? 0 : (gint32)round (CONST_DSP_F16*main_get_gapp()->xsm->Inst->VoltOut2Dig (X0f-X0i)/(steps));
        dsp_vector.f_dz = 0;
        dsp_vector.f_dx = 0;
        dsp_vector.f_dy = 0;
        dsp_vector.f_du = 0;
        dsp_vector.f_dy0 = 0;
        dsp_vector.f_dphi = 0;
        return main_get_gapp()->xsm->Inst->V2BiasV (main_get_gapp()->xsm->Inst->Dig2VoltOut (VOLT2AIC(X0i) + (double)dsp_vector.f_dx0*steps/CONST_DSP_F16));
}       

// make dZ/dX/dY vector from n point (if > 2, else automatic n) and (dX,dY,dZ) slope
double DSPControl::make_ZXYramp_vector (double dZ, double dX, double dY, int n, double slope, int source, int options, double long &duration, make_vector_flags flags){
	double dr = sqrt(dZ*dZ + dX*dX + dY*dY);

	if (flags & MAKE_VEC_FLAG_RAMP || n<2)
		make_auto_n_vector_elments (dr/slope*frq_ref);
	else {
		dsp_vector.n = n;
		dsp_vector.dnx = (gint32)round ( fabs (dr*frq_ref/(slope*dsp_vector.n)));
		++dsp_vector.n;
	}
	double steps = (double)(dsp_vector.n) * (double)(dsp_vector.dnx+1);

	duration += (double long) steps;
	dsp_vector.srcs = source & 0xffff;
	dsp_vector.options = options;
	dsp_vector.ptr_fb = 0;
	dsp_vector.repetitions = 0;
	dsp_vector.ptr_next = 0x0;
	dsp_vector.ptr_final = flags & MAKE_VEC_FLAG_END ? 0:1; // VPC relative branch to next vector
	dsp_vector.f_du = 0;
	dsp_vector.f_dx = (gint32)round (dsp_vector.n > 1 ? (CONST_DSP_F16*main_get_gapp()->xsm->Inst->XA2Dig (dX) / steps) : 0);
	dsp_vector.f_dy = (gint32)round (dsp_vector.n > 1 ? (CONST_DSP_F16*main_get_gapp()->xsm->Inst->YA2Dig (dY) / steps) : 0);
	dsp_vector.f_dz = (gint32)round (dsp_vector.n > 1 ? (CONST_DSP_F16*main_get_gapp()->xsm->Inst->ZA2Dig (dZ) / steps) : 0);
	dsp_vector.f_dx0 = 0;
	dsp_vector.f_dy0 = 0;
	dsp_vector.f_dphi = 0;

	return main_get_gapp()->xsm->Inst->Dig2ZA ((long)round ((double)dsp_vector.f_dz*steps/CONST_DSP_F16));
}

// make dU/dZ/dX/dY vector for n points and ts time per segment
double DSPControl::make_UZXYramp_vector (double dU, double dZ, double dX, double dY, double dSig1, double dSig2, int n, int nrep, int ptr_next, double ts, int source, int options, double long &duration, make_vector_flags flags){
	dsp_vector.n = n;
	if (ts <= (0.01333e-3 * (double)(n))) 
		dsp_vector.dnx = 0; // do N vectors at full speed, no inbetween steps.
	else
		dsp_vector.dnx = (gint32)round ( fabs (ts*frq_ref/(double)(dsp_vector.n)));
	//	++dsp_vector.n;

	double steps = (double)(dsp_vector.n) * (double)(dsp_vector.dnx+1);

	duration += (double long) steps;
	dsp_vector.srcs = source & 0xffff;
	dsp_vector.options = options;
	dsp_vector.ptr_fb = 0;
	dsp_vector.repetitions = nrep;
	dsp_vector.ptr_next = ptr_next;
	dsp_vector.ptr_final = flags & MAKE_VEC_FLAG_END ? 0:1; // VPC relative branch to next vector

	dsp_vector.f_du = (gint32)round (dsp_vector.n > 1 ? (CONST_DSP_F16*main_get_gapp()->xsm->Inst->VoltOut2Dig (dU/main_get_gapp()->xsm->Inst->BiasGainV2V ())/(steps)) : 0);
	dsp_vector.f_dx = (gint32)round (dsp_vector.n > 1 ? (CONST_DSP_F16*main_get_gapp()->xsm->Inst->XA2Dig (dX) / steps) : 0);
	dsp_vector.f_dy = (gint32)round (dsp_vector.n > 1 ? (CONST_DSP_F16*main_get_gapp()->xsm->Inst->YA2Dig (dY) / steps) : 0);
	dsp_vector.f_dz = (gint32)round (dsp_vector.n > 1 ? (CONST_DSP_F16*main_get_gapp()->xsm->Inst->ZA2Dig (dZ) / steps) : 0);
	dsp_vector.f_dx0 = (gint32)round (dsp_vector.n > 1 ? (CONST_DSP_F16*main_get_gapp()->xsm->Inst->ZA2Dig (dSig1) / steps) : 0);
	dsp_vector.f_dy0 = (gint32)round (dsp_vector.n > 1 ? (CONST_DSP_F16*main_get_gapp()->xsm->Inst->ZA2Dig (dSig2) / steps) : 0);
	dsp_vector.f_dphi = 0;

	return main_get_gapp()->xsm->Inst->Dig2ZA ((long)round ((double)dsp_vector.f_dz*steps/CONST_DSP_F16));
}




// make dPhi vector from n point (if > 2, else automatic n) and dPhi slope
double DSPControl::make_phase_vector (double dPhi, int n, double slope, int source, int options, double long &duration, make_vector_flags flags){
	double dr = dPhi*16.;
	slope *= 16.;

	if (flags & MAKE_VEC_FLAG_RAMP || n<2)
		make_auto_n_vector_elments (dr/slope*frq_ref);
	else {
		dsp_vector.n = n;
		dsp_vector.dnx = (gint32)round ( fabs (dr*frq_ref/(slope*dsp_vector.n)));
		++dsp_vector.n;
	}
	double steps = (double)(dsp_vector.n) * (double)(dsp_vector.dnx+1);

	duration += (double long) steps;
	dsp_vector.srcs = source & 0xffff;
	dsp_vector.options = options;
	dsp_vector.ptr_fb = 0;
	dsp_vector.repetitions = 0;
	dsp_vector.ptr_next = 0x0;
	dsp_vector.ptr_final = flags & MAKE_VEC_FLAG_END ? 0:1; // VPC relative branch to next vector
	dsp_vector.f_du = 0;
	dsp_vector.f_dx = 0;
	dsp_vector.f_dy = 0;
	dsp_vector.f_dz = 0;
	dsp_vector.f_dx0 = 0;
	dsp_vector.f_dy0 = 0;
	dsp_vector.f_dphi = (gint32)round (dsp_vector.n > 1 ? (CONST_DSP_F16 * dr / steps) : 0);

	return round ((double)dsp_vector.f_dphi*steps/CONST_DSP_F16/16.);
}

// Make a delay Vector
double DSPControl::make_delay_vector (double delay, int source, int options, double long &duration, make_vector_flags flags, int points){
	if (points > 2){
		double rnum = delay*frq_ref;
		dsp_vector.n = points;
		dsp_vector.dnx = (gint32)round (rnum / points - 1.);
		if (dsp_vector.dnx < 0)
			dsp_vector.dnx = 0;
	} else
		make_auto_n_vector_elments (delay*frq_ref);

	duration += (double long) (dsp_vector.n)*(dsp_vector.dnx+1);
	dsp_vector.srcs = source & 0xffff;
	dsp_vector.options = options;
	dsp_vector.repetitions = 0; // number of repetitions, not used yet
	dsp_vector.ptr_next = 0x0;  // pointer to next vector -- not used, only for loops
	dsp_vector.ptr_final = flags & MAKE_VEC_FLAG_END ? 0:1;   // VPC relative branch to next vector
	dsp_vector.f_du = 0;
	dsp_vector.f_dz = 0;
	dsp_vector.f_dx = 0; // x stepwidth, not used for probing
	dsp_vector.f_dy = 0; // y stepwidth, not used for probing
	dsp_vector.f_dx0 = 0; // x0 stepwidth, not used for probing
	dsp_vector.f_dy0 = 0; // y0 stepwidth, not used for probing
	dsp_vector.f_dphi = 0; // z0 stepwidth, not used for probing
	return (double)((long)(dsp_vector.n)*(long)(dsp_vector.dnx+1))/frq_ref;
}

// Make Vector Table End
void DSPControl::append_null_vector (int options, int index){
	// NULL vector -- just to clean vector table
	dsp_vector.n = 0;
	dsp_vector.dnx = 0;
	dsp_vector.srcs = 0x0000;
	dsp_vector.options = options;
	dsp_vector.repetitions = 0; // number of repetitions
	dsp_vector.ptr_next = 0;  // END
	dsp_vector.ptr_final= 0;  // END
	dsp_vector.f_dx = 0;
	dsp_vector.f_dy = 0;
	dsp_vector.f_dz = 0;
	// append 4 NULL-Vectors, just to clean up the end.
	write_dsp_vector (index);
	write_dsp_vector (index+1);
	write_dsp_vector (index+2);
	write_dsp_vector (index+3);
}

static void via_remote_list_Check_ec(Gtk_EntryControl* ec, remote_args* ra){
	ec->CheckRemoteCmd (ra);
};

// Create Vector Table form Mode (pvm=PV_MODE_XXXXX) and Execute if requested (start=TRUE) or write only
void DSPControl::write_dsp_probe (int start, pv_mode pvm){
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
	
	if (!sranger_common_hwi) return; 

	if (!start) // reset 
		probe_trigger_single_shot = 0;

	switch (pvm){
	case PV_MODE_NONE: // write dummy delay and NULL Vector
                g_free (vp_exec_mode_name); vp_exec_mode_name = g_strdup ("VP: NONE");
		options      = (PL_option_flags & FLAG_FB_ON     ? 0      : VP_FEEDBACK_HOLD)
                        | (PL_option_flags & FLAG_INTEGRATE ? VP_AIC_INTEGRATE : 0);
		ramp_sources = PL_option_flags & FLAG_SHOW_RAMP ? vis_Source : 0x000;
		recover_options = 0;

		make_delay_vector (0.1, ramp_sources, options, vp_duration, MAKE_VEC_FLAG_NORMAL);
		write_dsp_vector (vector_index++);
		make_delay_vector (0.1, ramp_sources, options, vp_duration, MAKE_VEC_FLAG_END);
		write_dsp_vector (vector_index++);
		append_null_vector (options, vector_index);
		sranger_common_hwi->probe_time_estimate = (int)vp_duration; // used for timeout check
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
			write_dsp_vector (vector_index++);

			make_Vdz_vector (ui, u0, dz_i0, n12, IV_slope, vis_Source, options, vp_duration, MAKE_VEC_FLAG_NORMAL);
			write_dsp_vector (vector_index++);

			make_Vdz_vector (u0, uf, dz_0f, n23, IV_slope, vis_Source, options, vp_duration, MAKE_VEC_FLAG_NORMAL);
			write_dsp_vector (vector_index++);

			dU_IV = uf-ui; dU_step = dU_IV/IV_points[0];

			if (IV_option_flags & FLAG_DUAL) {
				// run also reverse probe ramp in dual mode
				make_Vdz_vector (uf, u0, -dz_0f, n23, IV_slope, vis_Source, options, vp_duration, MAKE_VEC_FLAG_NORMAL);
				write_dsp_vector (vector_index++);

				make_Vdz_vector (u0, ui, -dz_i0, n12, IV_slope, vis_Source, options, vp_duration, MAKE_VEC_FLAG_NORMAL);
				write_dsp_vector (vector_index++);

				// Ramp back to given bias voltage   
				make_Vdz_vector (ui, bias, -dz_bi, -1, IV_slope_ramp, ramp_sources, options, vp_duration, MAKE_VEC_FLAG_RAMP);
				write_dsp_vector (vector_index++);

			} else {
				make_Vdz_vector (uf, bias, -(dz_bi+dz_i0+dz_0f), -1, IV_slope_ramp, ramp_sources, options, vp_duration, MAKE_VEC_FLAG_RAMP);
				write_dsp_vector (vector_index++);
			}

			if (IV_repetitions > 1){
				// Final vector, gives the IVC some time to recover   
				make_delay_vector (IV_final_delay, ramp_sources, options, vp_duration, MAKE_VEC_FLAG_NORMAL);
				write_dsp_vector (vector_index++);
				make_delay_vector (IV_recover_delay, ramp_sources, recover_options, vp_duration, MAKE_VEC_FLAG_NORMAL);
				dsp_vector.repetitions = IV_repetitions-1;
				dsp_vector.ptr_next = -vector_index; // go to start
				vp_duration +=	(IV_repetitions-1)*(vp_duration - vp_duration_1);
				write_dsp_vector (vector_index++);
			} else {
				make_delay_vector (IV_final_delay, ramp_sources, options, vp_duration, MAKE_VEC_FLAG_NORMAL);
				write_dsp_vector (vector_index++);
				make_delay_vector (IV_recover_delay, ramp_sources, recover_options, vp_duration, MAKE_VEC_FLAG_NORMAL);
				write_dsp_vector (vector_index++);
			}
			// add automatic conductivity measurement rho(Z) -- HOLD Bias fixed now!
			if (IVdz_repetitions > 0){
				vp_duration_2 =	vp_duration;
				// don't know the reason, but the following delay vector is needed to separate dI/dU and dI/dz
				make_delay_vector (0., ramp_sources, options, vp_duration, MAKE_VEC_FLAG_NORMAL);
				write_dsp_vector (vector_index++);

				// in case of rep > 1 the DSP will jump back to this point
				vpc = vector_index;
	
				make_Vdz_vector (bias, ui, dz_bi, -1, IV_slope_ramp, ramp_sources, options, vp_duration, (make_vector_flags)(MAKE_VEC_FLAG_RAMP | MAKE_VEC_FLAG_VHOLD));
				write_dsp_vector (vector_index++);
	
				make_Vdz_vector (ui, u0, dz_i0, n12, IV_slope, vis_Source, options, vp_duration, MAKE_VEC_FLAG_VHOLD);
				write_dsp_vector (vector_index++);
	
				make_Vdz_vector (u0, uf, dz_0f, n23, IV_slope, vis_Source, options, vp_duration, MAKE_VEC_FLAG_VHOLD);
				write_dsp_vector (vector_index++);
	
				if (IV_option_flags & FLAG_DUAL) {
					make_Vdz_vector (uf, u0, -dz_0f, n23, IV_slope, vis_Source, options, vp_duration, MAKE_VEC_FLAG_VHOLD);
					write_dsp_vector (vector_index++);
	
					make_Vdz_vector (u0, ui, -dz_i0, n12, IV_slope, vis_Source, options, vp_duration, MAKE_VEC_FLAG_VHOLD);
					write_dsp_vector (vector_index++);
	
					make_Vdz_vector (ui, bias, -dz_bi, -1, IV_slope_ramp, ramp_sources, options, vp_duration, (make_vector_flags)(MAKE_VEC_FLAG_RAMP | MAKE_VEC_FLAG_VHOLD));
					write_dsp_vector (vector_index++);
				} else {
					make_Vdz_vector (uf, bias, -(dz_bi+dz_i0+dz_0f), -1, IV_slope_ramp, ramp_sources, options, vp_duration, (make_vector_flags)(MAKE_VEC_FLAG_RAMP | MAKE_VEC_FLAG_VHOLD));
					write_dsp_vector (vector_index++);
				}
	
				if (IVdz_repetitions > 1){
					// Final vector, gives the IVC some time to recover   
					make_delay_vector (IV_final_delay, ramp_sources, options, vp_duration, MAKE_VEC_FLAG_NORMAL);
					write_dsp_vector (vector_index++);
	
					make_delay_vector (IV_recover_delay, ramp_sources, recover_options, vp_duration, MAKE_VEC_FLAG_NORMAL);
					dsp_vector.repetitions = IVdz_repetitions-1;
					dsp_vector.ptr_next = -(vector_index-vpc); // go to rho start
					vp_duration +=	(IVdz_repetitions-1)*(vp_duration - vp_duration_2);
					write_dsp_vector (vector_index++);
				} else {
					make_delay_vector (IV_final_delay, ramp_sources, options, vp_duration, MAKE_VEC_FLAG_NORMAL);
					write_dsp_vector (vector_index++);
					make_delay_vector (IV_recover_delay, ramp_sources, recover_options, vp_duration, MAKE_VEC_FLAG_NORMAL);
					write_dsp_vector (vector_index++);
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
				write_dsp_vector (vector_index++);

				make_Vdz_vector (ui, uf, dz_if, IV_points[IVs], IV_slope, vis_Source, options, vp_duration, MAKE_VEC_FLAG_NORMAL);
				write_dsp_vector (vector_index++);
			
				// info of real values set
				dU_IV   = main_get_gapp()->xsm->Inst->V2BiasV (main_get_gapp()->xsm->Inst->Dig2VoltOut ((long double)dsp_vector.f_du*(long double)(dsp_vector.n-1)*(long double)(dsp_vector.dnx ? dsp_vector.dnx+1 : 1)/CONST_DSP_F16));
				dU_step = main_get_gapp()->xsm->Inst->V2BiasV (main_get_gapp()->xsm->Inst->Dig2VoltOut ((long double)dsp_vector.f_du*(long double)(dsp_vector.dnx ? dsp_vector.dnx+1 : 1)/CONST_DSP_F16));
			
				// add vector for reverse return ramp? -- Force return path if dz != 0
				if (IV_option_flags & FLAG_DUAL) {
					// run also reverse probe ramp in dual mode
					make_Vdz_vector (uf, ui, -dz_if, IV_points[IVs], IV_slope, vis_Source, options, vp_duration, MAKE_VEC_FLAG_NORMAL);
					write_dsp_vector (vector_index++);
					
					if (IVs == (IV_sections-1)){
						// Ramp back to given bias voltage   
						make_Vdz_vector (ui, bias, -dz_bi, -1, IV_slope_ramp, ramp_sources, options, vp_duration, MAKE_VEC_FLAG_RAMP);
						write_dsp_vector (vector_index++);
					}
				} else {
					if (IVs == (IV_sections-1)){
						// Ramp back to given bias voltage   
						make_Vdz_vector (uf, bias, -(dz_if+dz_bi), -1, IV_slope_ramp, ramp_sources, options, vp_duration, MAKE_VEC_FLAG_RAMP);
						write_dsp_vector (vector_index++);
					}
				}
				if (IV_repetitions > 1){
					// Final vector, gives the IVC some time to recover   
					make_delay_vector (IV_final_delay, ramp_sources, options, vp_duration, MAKE_VEC_FLAG_NORMAL);
					write_dsp_vector (vector_index++);
					
					if (IVs == (IV_sections-1)){
						make_delay_vector (IV_recover_delay, ramp_sources, recover_options, vp_duration, MAKE_VEC_FLAG_NORMAL);
						dsp_vector.repetitions = IV_repetitions-1;
						dsp_vector.ptr_next = -vector_index; // go to start
						vp_duration +=	(IV_repetitions-1)*(vp_duration - vp_duration_1);
						write_dsp_vector (vector_index++);
					}
				} else {
					make_delay_vector (IV_final_delay, ramp_sources, options, vp_duration, MAKE_VEC_FLAG_NORMAL);
					write_dsp_vector (vector_index++);
					if (IVs == (IV_sections-1)){
						make_delay_vector (IV_recover_delay, ramp_sources, recover_options, vp_duration, MAKE_VEC_FLAG_NORMAL);
						write_dsp_vector (vector_index++);
					}
				}
			}

			// add automatic conductivity measurement rho(Z) -- HOLD Bias fixed now!
			if ((IVdz_repetitions > 0) && (fabs (IV_dz) > 0.001) && (bias != 0.)){
				vp_duration_2 =	vp_duration;
				// don't know the reason, but the following delay vector is needed to separate dI/dU and dI/dz
				make_delay_vector (0., ramp_sources, options, vp_duration, MAKE_VEC_FLAG_NORMAL);
				write_dsp_vector (vector_index++);

				// in case of rep > 1 the DSP will jump back to this point
				vpc = vector_index;
	
				make_Vdz_vector (bias, ui, dz_bi, -1, IV_slope_ramp, ramp_sources, options, vp_duration, (make_vector_flags)(MAKE_VEC_FLAG_RAMP | MAKE_VEC_FLAG_VHOLD));
				write_dsp_vector (vector_index++);

				make_Vdz_vector (ui, uf, dz_if, IV_points[0], IV_slope, vis_Source, options, vp_duration, MAKE_VEC_FLAG_VHOLD);
				write_dsp_vector (vector_index++);
			
				if (IV_option_flags & FLAG_DUAL) {
					make_Vdz_vector (uf, ui, -dz_if, IV_points[0], IV_slope, vis_Source, options, vp_duration, MAKE_VEC_FLAG_VHOLD);
					write_dsp_vector (vector_index++);

					// Ramp back to given bias voltage   
					make_Vdz_vector (ui, bias, -dz_bi, -1, IV_slope_ramp, ramp_sources, options, vp_duration, (make_vector_flags)(MAKE_VEC_FLAG_RAMP | MAKE_VEC_FLAG_VHOLD));
					write_dsp_vector (vector_index++);
				} else {
					make_Vdz_vector (uf, bias, -(dz_if+dz_bi), -1, IV_slope_ramp, ramp_sources, options, vp_duration, (make_vector_flags)(MAKE_VEC_FLAG_RAMP | MAKE_VEC_FLAG_VHOLD));
					write_dsp_vector (vector_index++);
				}
	
				if (IVdz_repetitions > 1 ){
					// Final vector, gives the IVC some time to recover   
					make_delay_vector (IV_final_delay, ramp_sources, options, vp_duration, MAKE_VEC_FLAG_NORMAL);
					write_dsp_vector (vector_index++);
	
					make_delay_vector (IV_recover_delay, ramp_sources, recover_options, vp_duration, MAKE_VEC_FLAG_NORMAL);
					dsp_vector.repetitions = IVdz_repetitions-1;
					dsp_vector.ptr_next = -(vector_index-vpc); // go to rho start
					vp_duration +=	(IVdz_repetitions-1)*(vp_duration - vp_duration_2);
					write_dsp_vector (vector_index++);
				} else {
					make_delay_vector (IV_final_delay, ramp_sources, options, vp_duration, MAKE_VEC_FLAG_NORMAL);
					write_dsp_vector (vector_index++);
					make_delay_vector (IV_recover_delay, ramp_sources, recover_options, vp_duration, MAKE_VEC_FLAG_NORMAL);
					write_dsp_vector (vector_index++);
				}
			}
		}

		// Step and Repeat along Line defined by dx dy -- if dxy points > 1, auto return?
		if (IV_dxy_points > 1){

			// Move probe to next position and setup auto repeat!
			make_ZXYramp_vector (0., IV_dx/(IV_dxy_points-1), IV_dy/(IV_dxy_points-1), 100, IV_dxy_slope, 
					     ramp_sources, options, vp_duration, MAKE_VEC_FLAG_RAMP);

			dsp_vector.f_dphi = (gint32)round (CONST_DSP_F16*main_get_gapp()->xsm->Inst->VoltOut2Dig (IV_dM/main_get_gapp()->xsm->Inst->BiasGainV2V ()));

			write_dsp_vector (vector_index++);
			make_delay_vector (IV_dxy_delay, ramp_sources, options, vp_duration, MAKE_VEC_FLAG_RAMP);
			dsp_vector.repetitions = IV_dxy_points-1;
			dsp_vector.ptr_next = -(vector_index-vpci); // go to initial IV start for full repeat
			write_dsp_vector (vector_index++);
			vp_duration +=	(IV_dxy_points-1)*(vp_duration - vp_duration_0);

			// add vector for full reverse return path -- YES!, always auto return!
			make_ZXYramp_vector (0., 
					     -IV_dx*(IV_dxy_points)/(IV_dxy_points-1.), 
					     -IV_dy*(IV_dxy_points)/(IV_dxy_points-1.), 100, IV_dxy_slope, 
					     ramp_sources, options, vp_duration, MAKE_VEC_FLAG_RAMP);
			write_dsp_vector (vector_index++);
		}

		// Final vector
		make_delay_vector (0., ramp_sources, options, vp_duration, MAKE_VEC_FLAG_END);
		write_dsp_vector (vector_index++);

		append_null_vector (options, vector_index);

		sranger_common_hwi->probe_time_estimate = (int)vp_duration; // used for timeout check


                if (probe_trigger_raster_points_user > 0 && write_vector_mode != PV_MODE_NONE){
                        double T_probe_cycle   = 1e3 * (double)vp_duration/frq_ref; // Time of full probe cycle in ms
                        double T_raster2raster = 1e3 * main_get_gapp()->xsm->data.s.rx / (main_get_gapp()->xsm->data.s.nx/probe_trigger_raster_points_user) / scan_speed_x; // Time inbetween raster points in ms
                        info = g_strdup_printf ("Tp=%.2f ms, Tr=%.2f ms, Td=%.2f ms", T_probe_cycle, T_raster2raster, T_raster2raster - T_probe_cycle);
                } else
                        info = g_strdup_printf ("Tp=%.2f ms, dU=%.3f V, dUs=%.2f mV, O*0x%02x S*0x%06x", 
                                                1e3*(double)vp_duration/frq_ref, dU_IV, dU_step*1e3, options, vis_Source
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


	case PV_MODE_FZ: // ------------ Special Vector Setup for FZ (Force(or what ever!!)-Distance) "Probe ala AFM"
                g_free (vp_exec_mode_name); vp_exec_mode_name = g_strdup ("VP: FZ");
		options      = (FZ_option_flags & FLAG_FB_ON     ? 0      : VP_FEEDBACK_HOLD)
                        | (FZ_option_flags & FLAG_INTEGRATE ? VP_AIC_INTEGRATE : 0);
		ramp_sources = FZ_option_flags & FLAG_SHOW_RAMP ? vis_Source : 0x000;

		// Ramp to initial Z from "current Z"
		make_ZXYramp_vector (FZ_start, 0., 0., -1, FZ_slope_ramp, ramp_sources, options, vp_duration, MAKE_VEC_FLAG_RAMP);
		write_dsp_vector (vector_index++);

		{
			int lim_mode = FZ_limiter_ch > 0 ? VP_LIMITER_UP :  (FZ_limiter_ch < 0 ? VP_LIMITER_DN : 0);
			int lim_ch = ((abs(FZ_limiter_ch)-1)&3) << 6;
			if (lim_mode)
				options |= lim_ch;

			
			// Ramp to final Z
			make_ZXYramp_vector (FZ_end - FZ_start, 0., 0., FZ_points, FZ_slope, vis_Source, options | lim_mode, vp_duration, MAKE_VEC_FLAG_NORMAL);
			write_dsp_vector (vector_index++);
		}

		// add vector for reverse return ramp?
		if (FZ_option_flags & FLAG_DUAL) {
			// Ramp to initil Z
			make_ZXYramp_vector (FZ_start - FZ_end, 0., 0., FZ_points, FZ_slope, vis_Source, options, vp_duration, MAKE_VEC_FLAG_NORMAL);
			write_dsp_vector (vector_index++);

			// Ramp back to "current Z"
			make_ZXYramp_vector (-FZ_start, 0., 0., -1, FZ_slope_ramp, ramp_sources, options, vp_duration, MAKE_VEC_FLAG_RAMP);
			write_dsp_vector (vector_index++);
		} else {
			// Ramp back to "current Z"
			make_ZXYramp_vector (-FZ_end, 0., 0., -1, FZ_slope_ramp, ramp_sources, options, vp_duration, MAKE_VEC_FLAG_RAMP);
			write_dsp_vector (vector_index++);
		}

		// Final vector, gives the IVC some time to recover   

		if (FZ_repetitions > 1){
			make_delay_vector (FZ_final_delay, ramp_sources, options, vp_duration, MAKE_VEC_FLAG_NORMAL);
			dsp_vector.repetitions = FZ_repetitions-1;
			dsp_vector.ptr_next = -vector_index; // go to start
			write_dsp_vector (vector_index++);

			make_delay_vector (FZ_final_delay, ramp_sources, options, vp_duration, MAKE_VEC_FLAG_END);
			write_dsp_vector (vector_index++);
		} else {
			make_delay_vector (FZ_final_delay, ramp_sources, options, vp_duration, MAKE_VEC_FLAG_END);
			write_dsp_vector (vector_index++);
		}

		append_null_vector (options, vector_index);

		sranger_common_hwi->probe_time_estimate = (int)vp_duration; // used for timeout check

		if (probe_trigger_raster_points_user > 0 && write_vector_mode != PV_MODE_NONE){
                        double T_probe_cycle   = 1e3 * (double)vp_duration/frq_ref; // Time of full probe cycle in ms
                        double T_raster2raster = 1e3 * main_get_gapp()->xsm->data.s.rx / (main_get_gapp()->xsm->data.s.nx/probe_trigger_raster_points_user) / scan_speed_x; // Time inbetween raster points in ms
                        info = g_strdup_printf ("Tp=%.2f ms, Tr=%.2f ms, Td=%.2f ms", T_probe_cycle, T_raster2raster, T_raster2raster - T_probe_cycle);
		} else
                        info = g_strdup_printf ("Tp=%.2f ms", 
                                                1e3*(double)vp_duration/frq_ref
                                                );
		if (FZ_status)
                        gtk_entry_buffer_set_text (GTK_ENTRY_BUFFER (gtk_entry_get_buffer (GTK_ENTRY(FZ_status))), info, -1);
        
		break;



	case PV_MODE_PL: // ------------ Special Vector Setup for PL (Puls)
                g_free (vp_exec_mode_name); vp_exec_mode_name = g_strdup ("VP: PL");
	        options_FBon  = (PL_option_flags & 0) | (PL_option_flags & FLAG_INTEGRATE ? VP_AIC_INTEGRATE : 0);
		options      = (PL_option_flags & FLAG_FB_ON     ? 0      : VP_FEEDBACK_HOLD)
                        | (PL_option_flags & FLAG_INTEGRATE ? VP_AIC_INTEGRATE : 0);
		ramp_sources = PL_option_flags & FLAG_SHOW_RAMP ? vis_Source : 0x000;

		{ 
			double t_ramp  = PL_volt / PL_slope;
			int    ns_ramp = 2+(int)(75000. * fabs(t_ramp));
			ramp_points = ns_ramp;
			if (PL_time_resolution > 0.){
				ramp_points = (int) round(t_ramp/(PL_time_resolution*1e-3));
                                //				std::cout << "ramp_points = " << ramp_points << " ****************************" << std::endl;
				if (ramp_points > 1000)
					ramp_points = 100;
				if (ramp_points < 2)
					ramp_points = 2;
			}else{
				if (ramp_points > 100)
					ramp_points = 100;
			}
		}

		// special self test operation -- generates a log step train
		if (PL_repetitions < 1){
			double u;
			double du=PL_step;
			int k;

			// a) make start delay
			make_delay_vector (PL_duration * 1e-3, vis_Source, options, vp_duration, MAKE_VEC_FLAG_NORMAL);
			write_dsp_vector (vector_index++);

			// b) make log 2x stairs up to PL-Volt from Bias
			for (u=bias, k=0; u<PL_volt && k<17; du*=2., ++k){
				make_Vdz_vector (u, u+du, 0., ramp_points, PL_slope, vis_Source, options, vp_duration, MAKE_VEC_FLAG_NORMAL);
				write_dsp_vector (vector_index++);
				u += du;

				make_delay_vector (PL_duration * 1e-3, vis_Source, options, vp_duration, MAKE_VEC_FLAG_NORMAL);
				write_dsp_vector (vector_index++);
			}
			if (PL_repetitions == -2){
                                // c1) make log 2x stairs down back to Bias
				du /= 2;
				for (k=0; u>bias && k<17; du/=2., ++k){
					make_Vdz_vector (u, u-du, 0., ramp_points, PL_slope, vis_Source, options, vp_duration, MAKE_VEC_FLAG_NORMAL);
					write_dsp_vector (vector_index++);
					u -= du;
					
					make_delay_vector (PL_duration * 1e-3, vis_Source, options, vp_duration, MAKE_VEC_FLAG_NORMAL);
					write_dsp_vector (vector_index++);
				}
			} else {
                                // c2) make one full step down back to Bias
				make_Vdz_vector (u, bias, 0., ramp_points, PL_slope, vis_Source, options, vp_duration, MAKE_VEC_FLAG_NORMAL);
				write_dsp_vector (vector_index++);
			}
		} else {

			if ( strchr (gtk_entry_buffer_get_text (GTK_ENTRY_BUFFER (gtk_entry_get_buffer (GTK_ENTRY(PL_remote_set_target)))), 'D')) // only if DSP_... check of D only.
				remote_set_post_start = TRUE;

			if (PL_time_resolution > 0.)
				make_delay_vector (PL_initial_delay, vis_Source, options, vp_duration, MAKE_VEC_FLAG_NORMAL, (int) round (PL_initial_delay/(PL_time_resolution*1e-3)));
			else
				make_delay_vector (PL_initial_delay, vis_Source, options, vp_duration, MAKE_VEC_FLAG_NORMAL);
			write_dsp_vector (vector_index++);

			make_Vdz_vector (bias, bias+PL_volt, PL_dZ, ramp_points, PL_slope, vis_Source, options, vp_duration, MAKE_VEC_FLAG_NORMAL);
			write_dsp_vector (vector_index++);

			if (PL_time_resolution > 0.)
				make_delay_vector (PL_duration * 1e-3, vis_Source, options, vp_duration, MAKE_VEC_FLAG_NORMAL, (int) round (PL_duration/PL_time_resolution));
			else // auto
				make_delay_vector (PL_duration * 1e-3, vis_Source, options, vp_duration, MAKE_VEC_FLAG_NORMAL);
			write_dsp_vector (vector_index++);

			if (PL_repetitions == 1){ // assume "Tip Tune Purpose", or advanced TS mode
				make_Vdz_vector (bias+PL_volt, bias, -PL_dZ, ramp_points, PL_slope, vis_Source, options, vp_duration, MAKE_VEC_FLAG_NORMAL);
				write_dsp_vector (vector_index++);
				if (fabs(PL_dZ) > 0. && fabs (PL_dZ_ext) > 0.){
					make_ZXYramp_vector (-PL_dZ*PL_dZ_ext, 0., 0., (int) round (PL_dZ_ext*ramp_points), fabs(PL_dZ*PL_slope/PL_volt), vis_Source, options, vp_duration, MAKE_VEC_FLAG_NORMAL);
					write_dsp_vector (vector_index++);
				}
				if (PL_time_resolution > 0.)
					make_delay_vector (PL_final_delay, vis_Source, options_FBon, vp_duration, MAKE_VEC_FLAG_NORMAL, (int) round (PL_final_delay/(PL_time_resolution*1e-3)));
				else
					make_delay_vector (PL_final_delay, vis_Source, options_FBon, vp_duration, MAKE_VEC_FLAG_NORMAL);
				write_dsp_vector (vector_index++);
			} else {
				make_Vdz_vector (bias+PL_volt, bias, -PL_dZ, ramp_points, PL_slope, vis_Source, options, vp_duration, MAKE_VEC_FLAG_NORMAL);
				write_dsp_vector (vector_index++);
			}
			if (PL_repetitions > 1){
				if (PL_time_resolution > 0.)
					make_delay_vector (PL_initial_delay, vis_Source, options, vp_duration, MAKE_VEC_FLAG_NORMAL, (int) round (PL_initial_delay/(PL_time_resolution*1e-3)));
				else
					make_delay_vector (PL_initial_delay, vis_Source, options, vp_duration, MAKE_VEC_FLAG_NORMAL);

				if( fabs(PL_step) > 0.0001 || fabs(PL_stepZ) > 0.01){
					write_dsp_vector (vector_index++);
					
					make_Vdz_vector (bias, bias+PL_step, PL_stepZ, ramp_points, PL_slope, vis_Source, options, vp_duration, MAKE_VEC_FLAG_NORMAL);
					dsp_vector.repetitions = PL_repetitions-1;
					dsp_vector.ptr_next = -4; // go to start
					write_dsp_vector (vector_index++);
					
					make_Vdz_vector (bias+PL_step*PL_repetitions, bias, PL_stepZ*PL_repetitions, ramp_points, PL_slope, vis_Source, options, vp_duration, MAKE_VEC_FLAG_NORMAL);
					write_dsp_vector (vector_index++);
				} else {
					dsp_vector.repetitions = PL_repetitions-1;
					dsp_vector.ptr_next = -3; // go to start
					write_dsp_vector (vector_index++);
				}
			}
		}

		if (PL_time_resolution > 0.)
			make_delay_vector (PL_final_delay, ramp_sources, options, vp_duration, MAKE_VEC_FLAG_END, (int) round (PL_final_delay/(PL_time_resolution*1e-3)));
		else
			make_delay_vector (PL_final_delay, ramp_sources, options, vp_duration, MAKE_VEC_FLAG_END);
		write_dsp_vector (vector_index++);

		append_null_vector (options, vector_index);

		sranger_common_hwi->probe_time_estimate = (int)vp_duration; // used for timeout check

		info = g_strdup_printf ("T_total=%.2f ms, O*0x%02x S*0x%06x", 
                                        1e3*(double)vp_duration/frq_ref, options, vis_Source
                                        );
                if (PL_status)
                        gtk_entry_buffer_set_text (GTK_ENTRY_BUFFER (gtk_entry_get_buffer (GTK_ENTRY(PL_status))), info, -1);
		break;

	case PV_MODE_SP: // ------------ Special Vector Setup for Slow PL (Puls) + Flag
                g_free (vp_exec_mode_name); vp_exec_mode_name = g_strdup ("VP: SP");
		options      = (SP_option_flags & FLAG_FB_ON     ? 0      : VP_FEEDBACK_HOLD)
                        | (SP_option_flags & FLAG_INTEGRATE ? VP_AIC_INTEGRATE : 0);
		ramp_sources = SP_option_flags & FLAG_SHOW_RAMP ? vis_Source : 0x000;

                //	double SP_duration, SP_ramptime, SP_volt, SP_final_delay, SP_flag_on_volt, SP_flag_off_volt;


		ramp_points = (int)(100.*SP_ramptime);

		make_dx0_vector (0., SP_flag_volt, 10, SP_flag_volt/0.1, vis_Source, options, vp_duration, MAKE_VEC_FLAG_NORMAL);
		write_dsp_vector (vector_index++);

		make_delay_vector (SP_final_delay * 60., ramp_sources, options, vp_duration, MAKE_VEC_FLAG_NORMAL);
		write_dsp_vector (vector_index++);

		make_dx0_vector (SP_flag_volt, 0., 10, SP_flag_volt/0.1, vis_Source, options, vp_duration, MAKE_VEC_FLAG_NORMAL);
		write_dsp_vector (vector_index++);


		make_Vdz_vector (bias, bias+SP_volt, 0., ramp_points, SP_volt/(SP_ramptime*60.) , vis_Source, options, vp_duration, MAKE_VEC_FLAG_NORMAL);
		write_dsp_vector (vector_index++);

		make_delay_vector (SP_duration * 60., vis_Source, options, vp_duration, MAKE_VEC_FLAG_NORMAL);
		write_dsp_vector (vector_index++);

		make_Vdz_vector (bias+SP_volt, bias, 0., ramp_points, SP_volt/(SP_ramptime*60.), vis_Source, options, vp_duration, MAKE_VEC_FLAG_NORMAL);
		write_dsp_vector (vector_index++);
			
		make_dx0_vector (0., SP_flag_volt, 10, SP_flag_volt/0.1, vis_Source, options, vp_duration, MAKE_VEC_FLAG_NORMAL);
		write_dsp_vector (vector_index++);

		make_delay_vector (SP_final_delay * 60., ramp_sources, options, vp_duration, MAKE_VEC_FLAG_NORMAL);
		dsp_vector.repetitions = SP_repetitions-1;
		dsp_vector.ptr_next = -5; // go to start
		write_dsp_vector (vector_index++);


		make_dx0_vector (SP_flag_volt, 0., 10, SP_flag_volt/0.1, vis_Source, options, vp_duration, MAKE_VEC_FLAG_NORMAL);
		write_dsp_vector (vector_index++);

		make_delay_vector (SP_final_delay * 60., ramp_sources, options, vp_duration, MAKE_VEC_FLAG_END);
		write_dsp_vector (vector_index++);
		append_null_vector (options, vector_index);

		sranger_common_hwi->probe_time_estimate = (int)vp_duration; // used for timeout check

		info = g_strdup_printf ("T_total=%.2f ms, O*0x%02x S*0x%06x", 
					1e3*(double)vp_duration/frq_ref, options, vis_Source
					);
                if (SP_status)
                        gtk_entry_buffer_set_text (GTK_ENTRY_BUFFER (gtk_entry_get_buffer (GTK_ENTRY(SP_status))), info, -1);
		break;

	case PV_MODE_LP: // ------------ Special Vector Setup for LP (Laserpuls)
                g_free (vp_exec_mode_name); vp_exec_mode_name = g_strdup ("VP: LP");
		options      = (LP_option_flags & FLAG_FB_ON     ? 0      : VP_FEEDBACK_HOLD)
                        | (LP_option_flags & FLAG_INTEGRATE ? VP_AIC_INTEGRATE : 0);
		ramp_sources = LP_option_flags & FLAG_SHOW_RAMP ? vis_Source : 0x000;

		recover_options=0;

		ramp_points = 10;

		make_delay_vector (LP_duration * 1e-3, vis_Source, recover_options, vp_duration, MAKE_VEC_FLAG_NORMAL);
		write_dsp_vector (vector_index++);

		make_ZXYramp_vector (LP_FZ_end, 0., 0., ramp_points, LP_slope, vis_Source, options, vp_duration, MAKE_VEC_FLAG_NORMAL);
		write_dsp_vector (vector_index++);

		make_Vdx0_vector (bias, bias+LP_volt, 0., ramp_points, LP_slope, vis_Source, options, vp_duration, MAKE_VEC_FLAG_NORMAL);
		write_dsp_vector (vector_index++);

		make_delay_vector (LP_triggertime * 1e-3, vis_Source, options, vp_duration, MAKE_VEC_FLAG_NORMAL);
		write_dsp_vector (vector_index++);

		make_Vdx0_vector (bias+LP_volt, bias, 0., ramp_points, LP_slope, vis_Source, options, vp_duration, MAKE_VEC_FLAG_NORMAL);
		write_dsp_vector (vector_index++);
			
		make_delay_vector (LP_final_delay * 1e-3, ramp_sources, options, vp_duration, MAKE_VEC_FLAG_NORMAL);
		write_dsp_vector (vector_index++);

		make_ZXYramp_vector (-LP_FZ_end, 0., 0., ramp_points, LP_slope, vis_Source, recover_options, vp_duration, MAKE_VEC_FLAG_NORMAL);
		dsp_vector.repetitions = LP_repetitions-1;
		dsp_vector.ptr_next = -6; // go to start
		write_dsp_vector (vector_index++);

		make_delay_vector (LP_final_delay * 1e-3, ramp_sources, recover_options, vp_duration, MAKE_VEC_FLAG_END);
		write_dsp_vector (vector_index++);
		append_null_vector (options, vector_index);

		sranger_common_hwi->probe_time_estimate = (int)vp_duration; // used for timeout check

		info = g_strdup_printf ("T_total=%.2f ms, O*0x%02x S*0x%06x", 
					1e3*(double)vp_duration/frq_ref, options, vis_Source
					);
		if (LP_status)
                        gtk_entry_buffer_set_text (GTK_ENTRY_BUFFER (gtk_entry_get_buffer (GTK_ENTRY(LP_status))), info, -1);
                break;
		

	case PV_MODE_TS: // ------------ Special Vector Setup for TS (Time Spectroscopy)
                g_free (vp_exec_mode_name); vp_exec_mode_name = g_strdup ("VP: TS");
		options      = (TS_option_flags & FLAG_FB_ON     ? 0      : VP_FEEDBACK_HOLD)
                        | (TS_option_flags & FLAG_INTEGRATE ? VP_AIC_INTEGRATE : 0);
		ramp_sources = TS_option_flags & FLAG_SHOW_RAMP ? vis_Source : 0x000;

		vp_duration_0 =	vp_duration;
		make_delay_vector (TS_duration * 1e-3, vis_Source, options, vp_duration, MAKE_VEC_FLAG_NORMAL, (int)TS_points);
		write_dsp_vector (vector_index++);

		if (TS_repetitions > 1){ // add rep command?
			make_delay_vector (0.01e-3, ramp_sources, options, vp_duration, MAKE_VEC_FLAG_NORMAL, 1);
			dsp_vector.repetitions = TS_repetitions-1;
			dsp_vector.ptr_next = -1; // go to start, just repeat this
			vp_duration +=	(TS_repetitions-1)*(vp_duration - vp_duration_0);
			write_dsp_vector (vector_index++);
		}

		make_delay_vector (0.01, ramp_sources, options, vp_duration, MAKE_VEC_FLAG_END);
		write_dsp_vector (vector_index++);
		append_null_vector (options, vector_index);

		sranger_common_hwi->probe_time_estimate = (int)vp_duration; // used for timeout check

		info = g_strdup_printf ("T_total=%.2f ms, O*0x%02x S*0x%06x", 
					1e3*(double)vp_duration/frq_ref, options, vis_Source
					);
		if (TS_status)
                        gtk_entry_buffer_set_text (GTK_ENTRY_BUFFER (gtk_entry_get_buffer (GTK_ENTRY(TS_status))), info, -1);
		break;


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
				if (GVP_vnrep[k] < 1)
					GVP_vnrep[k] = 1;
				if (GVP_vpcjr[k] < -vector_index || GVP_vpcjr[k] > 0) // origin of VP, no forward jump
					GVP_vpcjr[k] = -vector_index; // defaults to start
				make_UZXYramp_vector (GVP_du[k], GVP_dz[k], GVP_dx[k], GVP_dy[k], GVP_dsig[k], 0., GVP_points[k], GVP_vnrep[k]-1, GVP_vpcjr[k], GVP_ts[k], vis_Source, options, vp_duration, MAKE_VEC_FLAG_NORMAL);
				write_dsp_vector (vector_index++);
				if (GVP_vnrep[k]-1 > 0)	
					vp_duration +=	(GVP_vnrep[k]-1)*(vp_duration - vpd[k+GVP_vpcjr[k]]);
			}
		}

		options      = (GVP_option_flags & FLAG_FB_ON     ? 0      : VP_FEEDBACK_HOLD)
                        | (GVP_option_flags & FLAG_INTEGRATE ? VP_AIC_INTEGRATE : 0);
		make_delay_vector (GVP_final_delay, ramp_sources, options, vp_duration, MAKE_VEC_FLAG_END);
		write_dsp_vector (vector_index++);

		append_null_vector (options, vector_index);

		sranger_common_hwi->probe_time_estimate = (int)vp_duration; // used for timeout check

		info = g_strdup_printf ("T=%.2f ms", 1e3*(double)vp_duration/frq_ref);
		if (GVP_status)
                        gtk_entry_buffer_set_text (GTK_ENTRY_BUFFER (gtk_entry_get_buffer (GTK_ENTRY(GVP_status))), info, -1);
		break;

	case PV_MODE_TK: // ------------ Special Vector Setup for TK (Tracking)
                g_free (vp_exec_mode_name); vp_exec_mode_name = g_strdup ("VP: Tracking");
		options      = (TK_option_flags & FLAG_FB_ON     ? 0      : VP_FEEDBACK_HOLD)
                        | (TK_option_flags & FLAG_INTEGRATE ? VP_AIC_INTEGRATE : 0);
                //		ramp_sources = TK_option_flags & FLAG_SHOW_RAMP ? vis_Source : 0x000;
                // force all srcs off!!! (else data transfer gets broken)
		ramp_sources = 0x000;

		{ // create "loop" scan with TK_points
			double px, py;

			// init buffer 16ms delay
			make_delay_vector (16.*1e-3, ramp_sources, options, vp_duration, MAKE_VEC_FLAG_NORMAL, 16);

			vp_dt_sec=vp_duration;
			// REF
			int tk_ref = (TK_ref&3) << 6;
			int tk_pc = 0;
			options |= tk_ref;

			make_delay_vector (TK_delay*1e-3, 0, options | VP_TRACK_REF, vp_duration, MAKE_VEC_FLAG_NORMAL, TK_points);
			write_dsp_vector (vector_index++);

			int tmode = TK_mode > 0 ? VP_TRACK_UP : VP_TRACK_DN;
			// V1
			make_ZXYramp_vector (0., px = TK_r, py = 0., TK_points, TK_speed, 0, options | tmode, vp_duration, MAKE_VEC_FLAG_NORMAL);
			write_dsp_vector (vector_index++);
			++tk_pc;

			// V2...Vn
			for (int i=1; i<TK_nodes; ++i){
				double phi = (double)i * M_PI*2 / (double)TK_nodes;
				double dx = TK_r * cos (phi) - px;
				double dy = TK_r * sin (phi) - py;
				px += dx; py += dy;
				make_ZXYramp_vector (0., dx, dy, TK_points, TK_speed, 0, options | tmode, vp_duration, MAKE_VEC_FLAG_NORMAL);
				write_dsp_vector (vector_index++);
				++tk_pc;
			}
			// 2nd circle?
			if (TK_r2 > 0.){
				for (int i=0; i<TK_nodes; ++i){
					double phi = (double)i * M_PI*2 / (double)TK_nodes;
					double dx = TK_r2 * cos (phi) - px;
					double dy = TK_r2 * sin (phi) - py;
					px += dx; py += dy;
					make_ZXYramp_vector (0., dx, dy, TK_points, TK_speed, 0, options | tmode, vp_duration, MAKE_VEC_FLAG_NORMAL);
					write_dsp_vector (vector_index++);
					++tk_pc;
				}
			}

			// V-FIN (return)
			make_ZXYramp_vector (0., -px, -py, TK_points, TK_speed, 0, options | VP_TRACK_FIN, vp_duration, MAKE_VEC_FLAG_NORMAL);
			write_dsp_vector (vector_index++);

			// repetitions
			make_delay_vector (TK_delay*1e-3, ramp_sources, options, vp_duration, MAKE_VEC_FLAG_NORMAL, TK_points);
			dsp_vector.repetitions =  TK_repetitions-1;
			dsp_vector.ptr_next    = -tk_pc-2; // go to start
			write_dsp_vector (vector_index++);

			vp_duration += (vp_duration-vp_dt_sec)*(2*TK_repetitions-1); // estimate
		}

		// end buffer
		make_delay_vector (16.*1e-3, ramp_sources, options, vp_duration, MAKE_VEC_FLAG_NORMAL, 16);
		
		// end mark
		make_delay_vector (TK_delay*1e-3, ramp_sources, options, vp_duration, MAKE_VEC_FLAG_END);
		write_dsp_vector (vector_index++);

		append_null_vector (options, vector_index);

		sranger_common_hwi->probe_time_estimate = (int)vp_duration; // used for timeout check

		info = g_strdup_printf ("T=%.2f ms", 1e3*(double)vp_duration/frq_ref);
		if (TK_status)
                        gtk_entry_buffer_set_text (GTK_ENTRY_BUFFER (gtk_entry_get_buffer (GTK_ENTRY(TK_status))), info, -1);
		break;

	case PV_MODE_AC: // ------------ Special Vector Setup for AC (phase probe)
                g_free (vp_exec_mode_name); vp_exec_mode_name = g_strdup ("VP: LockIn Phase Sweep");
		options      = (AC_option_flags & FLAG_FB_ON     ? 0      : VP_FEEDBACK_HOLD)
                        | (AC_option_flags & FLAG_INTEGRATE ? VP_AIC_INTEGRATE : 0);
		ramp_sources = AC_option_flags & FLAG_SHOW_RAMP ? vis_Source : 0x000;

		if (AC_option_flags & FLAG_DUAL) {
			make_phase_vector (AC_phase_span, AC_points, AC_phase_slope, vis_Source, options, vp_duration, MAKE_VEC_FLAG_NORMAL);
			write_dsp_vector (vector_index++);

			make_delay_vector (AC_final_delay, ramp_sources, options, vp_duration, MAKE_VEC_FLAG_NORMAL);
			write_dsp_vector (vector_index++);

			make_phase_vector (-AC_phase_span, AC_points, AC_phase_slope, vis_Source, options, vp_duration, MAKE_VEC_FLAG_NORMAL);
			write_dsp_vector (vector_index++);

			make_delay_vector (AC_final_delay, ramp_sources, options, vp_duration, MAKE_VEC_FLAG_NORMAL);
			dsp_vector.repetitions = AC_repetitions-1;
			dsp_vector.ptr_next = -3; // go to start
			write_dsp_vector (vector_index++);
		} else {
			make_phase_vector (AC_phase_span, AC_points, AC_phase_slope, vis_Source, options, vp_duration, MAKE_VEC_FLAG_NORMAL);
			write_dsp_vector (vector_index++);

			make_delay_vector (AC_final_delay, ramp_sources, options, vp_duration, MAKE_VEC_FLAG_NORMAL);
			dsp_vector.repetitions = AC_repetitions-1;
			dsp_vector.ptr_next = -1; // go to start
			write_dsp_vector (vector_index++);
		}

		make_delay_vector (AC_final_delay, ramp_sources, options, vp_duration, MAKE_VEC_FLAG_END);
		write_dsp_vector (vector_index++);
		append_null_vector (options, vector_index);

		sranger_common_hwi->probe_time_estimate = (int)vp_duration; // used for timeout check

		info = g_strdup_printf ("T_total=%.2f ms, O*0x%02x S*0x%06x", 
					1e3*(double)vp_duration/frq_ref, options, vis_Source
					);
		if (AC_status)
                        gtk_entry_buffer_set_text (GTK_ENTRY_BUFFER (gtk_entry_get_buffer (GTK_ENTRY(AC_status))), info, -1);
		break;

	case PV_MODE_AX: // ------------ Special Vector Setup for AX (auxillary probe, QMA, CMA, ...)
                g_free (vp_exec_mode_name); vp_exec_mode_name = g_strdup ("VP: AX");
		options      = (AX_option_flags & FLAG_FB_ON     ? 0      : VP_FEEDBACK_HOLD)
                        | (AX_option_flags & FLAG_INTEGRATE ? VP_AIC_INTEGRATE : 0);
		ramp_sources = AX_option_flags & FLAG_SHOW_RAMP ? vis_Source : 0x000;

		make_Vdz_vector (bias, AX_start, 0., -1, AX_slope_ramp, ramp_sources, options, vp_duration, MAKE_VEC_FLAG_RAMP);
		write_dsp_vector (vector_index++);

		AX_slope = fabs(AX_end-AX_start)/(AX_gatetime*AX_points);

		make_Vdz_vector (AX_start, AX_end, 0., AX_points, AX_slope, vis_Source, options, vp_duration, MAKE_VEC_FLAG_NORMAL);
		write_dsp_vector (vector_index++);

		// add vector for reverse return path?
		if (AX_option_flags & FLAG_DUAL) {
			make_Vdz_vector (AX_end, AX_start, 0., AX_points, AX_slope, vis_Source, options, vp_duration, MAKE_VEC_FLAG_NORMAL);
			write_dsp_vector (vector_index++);

			make_Vdz_vector (AX_start, bias, 0., -1, AX_slope_ramp, ramp_sources, options, vp_duration, MAKE_VEC_FLAG_RAMP);
			write_dsp_vector (vector_index++);
		} else {
			make_Vdz_vector (AX_end, bias, 0., -1, AX_slope_ramp, ramp_sources, options, vp_duration, MAKE_VEC_FLAG_NORMAL);
			write_dsp_vector (vector_index++);
		}
			
		make_delay_vector (AX_final_delay, ramp_sources, options, vp_duration, MAKE_VEC_FLAG_NORMAL);
		dsp_vector.repetitions = AX_repetitions-1;
		dsp_vector.ptr_next = -3; // go to start
		write_dsp_vector (vector_index++);

		make_delay_vector (AX_final_delay, ramp_sources, options, vp_duration, MAKE_VEC_FLAG_END);
		write_dsp_vector (vector_index++);
		append_null_vector (options, vector_index);

 		sranger_common_hwi->probe_time_estimate = (int)vp_duration; // used for timeout check

		{
			info = g_strdup_printf ("T=%.2f ms, Slope: %.4f V/s", 1e3*(double)vp_duration/frq_ref, AX_slope);
			if (AX_status)
				gtk_entry_buffer_set_text (GTK_ENTRY_BUFFER (gtk_entry_get_buffer (GTK_ENTRY(AX_status))), info, -1);
		}
		break;
	case PV_MODE_ABORT: // ------------ Special Vector Setup for aborting/killing VP program, protected DSP reset capability
                g_free (vp_exec_mode_name); vp_exec_mode_name = g_strdup ("VP: Abort");
		options      = (ABORT_option_flags & FLAG_FB_ON     ? 0      : VP_FEEDBACK_HOLD)
                        | (ABORT_option_flags & FLAG_INTEGRATE ? VP_AIC_INTEGRATE : 0);
		ramp_sources = ABORT_option_flags & FLAG_SHOW_RAMP ? vis_Source : 0x000;

		if (ABORT_final_delay < 0.){
			ABORT_final_delay = 1.;
			sranger_common_hwi->write_dsp_reset ();
		}

		make_delay_vector (ABORT_final_delay, ramp_sources, options, vp_duration, MAKE_VEC_FLAG_END);
		write_dsp_vector (vector_index++);

		// clear program
		for (; vector_index<36; vector_index+=4)
			append_null_vector (options, vector_index);
		append_null_vector (options, vector_index);

		sranger_common_hwi->probe_time_estimate = (int)vp_duration; // used for timeout check

		break;
	}

	if (start){
                g_message ("Executing Vector Probe Now! Mode: %s", vp_exec_mode_name);
		main_get_gapp()->monitorcontrol->LogEvent ("VectorProbe Execute", vp_exec_mode_name);
		main_get_gapp()->monitorcontrol->LogEvent ("VectorProbe", info, 2);
	}

        g_free (info);

	// --------------------------------------------------

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
	sranger_common_hwi->write_dsp_lockin_probe_final (AC_amp, AC_frq, AC_phaseA, AC_phaseB, AC_lockin_avg_cycels, FZ_limiter_val, 
							  noise_amp,
							  start);

	// Update EC's
	update();

	// Update from DSP
	read_dsp_probe ();
}


void DSPControl::write_dsp_vector (int index){
        if (!sranger_common_hwi) return; 

        sranger_common_hwi->write_dsp_vector (index, &dsp_vector);


	// update GXSM's internal copy of vector list
	dsp_vector_list[index].n = dsp_vector.n;
	dsp_vector_list[index].dnx = dsp_vector.dnx;
	dsp_vector_list[index].srcs = dsp_vector.srcs;
	dsp_vector_list[index].options = dsp_vector.options;
	dsp_vector_list[index].ptr_fb = dsp_vector.ptr_fb;
	dsp_vector_list[index].repetitions = dsp_vector.repetitions;
	dsp_vector_list[index].i = dsp_vector.i;
	dsp_vector_list[index].j = dsp_vector.j;
	dsp_vector_list[index].ptr_next = dsp_vector.ptr_next;
	dsp_vector_list[index].ptr_final = dsp_vector.ptr_final;
	dsp_vector_list[index].f_du = dsp_vector.f_du;
	dsp_vector_list[index].f_dx = dsp_vector.f_dx;
	dsp_vector_list[index].f_dy = dsp_vector.f_dy;
	dsp_vector_list[index].f_dz = dsp_vector.f_dz;
	dsp_vector_list[index].f_dx0 = dsp_vector.f_dx0;
	dsp_vector_list[index].f_dy0 = dsp_vector.f_dy0;
	dsp_vector_list[index].f_dphi = dsp_vector.f_dphi;

	{ 
		double mVf = 10000. / (65536. * 32768.);
		gchar *pvi = g_strdup_printf ("ProbeVector[pc%02d]", index);
		gchar *pvd = g_strdup_printf ("(n:%05d, dnx:%05d, 0x%04x, 0x%04x, r:%4d, pc:%d, f:%d),"
					      "(dU:%6.4f mV, dxzy:%6.4f, %6.4f, %6.4f mV, dxy0:%6.4f, %6.4f mV, dP:%.4f)", 
					      dsp_vector.n, dsp_vector.dnx,
					      dsp_vector.srcs, dsp_vector.options,
					      dsp_vector.repetitions,
					      dsp_vector.ptr_next, dsp_vector.ptr_final,
					      mVf * dsp_vector.f_du, mVf * dsp_vector.f_dx, mVf * dsp_vector.f_dy, mVf * dsp_vector.f_dz,
					      mVf * dsp_vector.f_dx0, mVf * dsp_vector.f_dy0,
					      dsp_vector.f_dphi / CONST_DSP_F16
                                              );

		main_get_gapp()->monitorcontrol->LogEvent (pvi, pvd, 2);
		g_free (pvi);
		g_free (pvd);
	}
}

void DSPControl::read_dsp_vector (int index){
        if (!sranger_common_hwi) return; 
	sranger_common_hwi->read_dsp_vector (index, &dsp_vector);
}


void DSPControl::write_dsp_abort_probe (){
        if (!sranger_common_hwi) return; 
	sranger_common_hwi->write_dsp_abort_probe ();
}

