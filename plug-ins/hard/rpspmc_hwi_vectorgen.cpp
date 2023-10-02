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

#include "rpspmc_pacpll.h"

extern rpspmc_hwi_dev *rpspmc_hwi;

extern "C++" {
        extern RPSPMC_Control *RPSPMC_ControlClass;
        extern GxsmPlugin rpspmc_hwi_pi;
}

void RPSPMC_Control::read_spm_vector_program (){
	if (!rpspmc_hwi) return; 
}

#if 0

// make automatic n and dnx from float number of steps, keep n below 1000.
void RPSPMC_Control::make_auto_n_vector_elments (double fnum){
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
double RPSPMC_Control::make_Vdz_vector (double Ui, double Uf, double dZ, int n, double slope, int source, int options, double long &duration, make_vector_flags flags){
	double dv = fabs (Uf - Ui);

	if (dv < 1e-6)
		return make_delay_vector ((double)n*256./rpspmc_hwi->get_GVP_frq_ref (), source, options, duration, flags, n);

	if (flags & MAKE_VEC_FLAG_RAMP || n < 2)
		make_auto_n_vector_elments (dv/slope*rpspmc_hwi->get_GVP_frq_ref ());
	else {
		program_vector.n = n; // number of data points
                //		++program_vector.n;
		program_vector.dnx = abs((gint32)round ((Uf - Ui)*rpspmc_hwi->get_GVP_frq_ref ()/(slope*program_vector.n))); // number of steps between data points
	}
	double steps = (double)(program_vector.n) * (double)(program_vector.dnx+1);	//total number of steps

	duration += (double long) steps;
	program_vector.srcs = source & 0xffff; // source channel coding
	program_vector.options = options;
	program_vector.repetitions = 0;
	program_vector.ptr_next = 0x0;
	program_vector.ptr_final = flags & MAKE_VEC_FLAG_END ? 0:1; // VPC relative branch to next vector
	program_vector.f_du = flags & MAKE_VEC_FLAG_VHOLD ? 0 : ((Uf-Ui)/main_get_gapp()->xsm->Inst->BiasGainV2V ())/steps;
	program_vector.f_dz = main_get_gapp()->xsm->Inst->ZA2Volt (dZ/steps);
	program_vector.f_dx = 0.0;
	program_vector.f_dy = 0.0;
	program_vector.f_dx0 = 0.0;
	program_vector.f_dy0 = 0.0;
	program_vector.f_dz0 = 0.0;
	return main_get_gapp()->xsm->Inst->V2BiasV (Ui + program_vector.f_du*steps);
}	

// Copy of Vdz above, but the du steps were used for dx0
double RPSPMC_Control::make_Vdx0_vector (double Ui, double Uf, double dZ, int n, double slope, int source, int options, double long &duration, make_vector_flags flags
                                     ){
        double dv = fabs (Uf - Ui);
        if (flags & MAKE_VEC_FLAG_RAMP || n < 2)
                make_auto_n_vector_elments (dv/slope*rpspmc_hwi->get_GVP_frq_ref ());
        else {
                program_vector.n = n; // number of data points
                //              ++program_vector.n;
                program_vector.dnx = abs((gint32)round ((Uf - Ui)*rpspmc_hwi->get_GVP_frq_ref ()/(slope*program_vector.n))); // number of steps between data points
        }
        double steps = (double)(program_vector.n) * (double)(program_vector.dnx+1);     //total number of steps

        duration += (double long) steps;
        program_vector.srcs = source & 0xffff; // source channel coding
        program_vector.options = options;
	program_vector.repetitions = 0;
	program_vector.ptr_next = 0x0;
        program_vector.ptr_final = flags & MAKE_VEC_FLAG_END ? 0:1; // VPC relative branch to next vector
        program_vector.f_dx0 = flags & MAKE_VEC_FLAG_VHOLD ? 0 : ((Uf-Ui)/main_get_gapp()->xsm->Inst->BiasGainV2V ())/steps; // !!!!!x ????
        program_vector.f_dz = main_get_gapp()->xsm->Inst->ZA2Volt (dZ/steps);
        program_vector.f_dx = 0.0;
        program_vector.f_dy = 0.0;
        program_vector.f_du = 0.0;
        program_vector.f_dy0 = 0.0;
        program_vector.f_dz0 = 0.0;
        return main_get_gapp()->xsm->Inst->V2BiasV (Ui + program_vector.f_du*steps);
}       

// Copy of Vdz above, but the du steps were used for dx0
double RPSPMC_Control::make_dx0_vector (double X0i, double X0f, int n, double slope, int source, int options, double long &duration, make_vector_flags flags
                                    ){
        double dv = fabs (X0f - X0i);
        if (flags & MAKE_VEC_FLAG_RAMP || n < 2)
                make_auto_n_vector_elments (dv/slope*rpspmc_hwi->get_GVP_frq_ref ());
        else {
                program_vector.n = n; // number of data points
                //              ++program_vector.n;
                program_vector.dnx = abs((gint32)round ((X0f - X0i)*rpspmc_hwi->get_GVP_frq_ref ()/(slope*program_vector.n))); // number of steps between data points
        }
        double steps = (double)(program_vector.n) * (double)(program_vector.dnx+1);     //total number of steps

        duration += (double long) steps;
        program_vector.srcs = source & 0xffff; // source channel coding
        program_vector.options = options;
	program_vector.repetitions = 0;
	program_vector.ptr_next = 0x0;
        program_vector.ptr_final = flags & MAKE_VEC_FLAG_END ? 0:1; // VPC relative branch to next vector
        program_vector.f_du = 0.0;
        program_vector.f_dx0 = flags & MAKE_VEC_FLAG_VHOLD ? 0.0 : (X0f-X0i)/steps;
        program_vector.f_dz = 0.0;
        program_vector.f_dx = 0.0;
        program_vector.f_dy = 0.0;
        program_vector.f_dy0 = 0.0;
        program_vector.f_dz0 = 0.0;
        return main_get_gapp()->xsm->Inst->V2BiasV (X0i + program_vector.f_dx0*steps);
}       
#endif

// make dZ/dX/dY vector from n point (if > 2, else automatic n) and (dX,dY,dZ) slope
double RPSPMC_Control::make_ZXYramp_vector (double dZ, double dX, double dY, int n, double slope, int source, int options, double &duration, make_vector_flags flags){
	double dr = sqrt(dZ*dZ + dX*dX + dY*dY);
        double tm = dr/slope;
        
        program_vector.n = n;
        program_vector.slew = n/tm;
	duration += tm;
	program_vector.srcs = source & 0xffff;
	program_vector.options = options;
	program_vector.repetitions = 0;
	program_vector.ptr_next = 0x0;
	program_vector.f_du = 0.0;
	program_vector.f_dx = main_get_gapp()->xsm->Inst->XA2Volt (dX);
	program_vector.f_dy = main_get_gapp()->xsm->Inst->YA2Volt (dY);
	program_vector.f_dz = main_get_gapp()->xsm->Inst->ZA2Volt (dZ);
	program_vector.f_da = 0.0;
	program_vector.f_db = 0.0;

	return main_get_gapp()->xsm->Inst->Volt2ZA (program_vector.f_dz);
}

// make dU/dZ/dX/dY vector for n points and ts time per segment
double RPSPMC_Control::make_UZXYramp_vector (double dU, double dZ, double dX, double dY, double da, double db, int n, int nrep, int ptr_next, double ts, int source, int options){
	program_vector.n = n;
	program_vector.slew = n/ts;
	program_vector.srcs = source;
	program_vector.options = options;
	program_vector.repetitions = nrep;
	program_vector.ptr_next = ptr_next;

	program_vector.f_du = dU/main_get_gapp()->xsm->Inst->BiasGainV2V ();
	program_vector.f_dx = main_get_gapp()->xsm->Inst->XA2Volt (dX);
	program_vector.f_dy = main_get_gapp()->xsm->Inst->YA2Volt (dY);
	program_vector.f_dz = main_get_gapp()->xsm->Inst->ZA2Volt (dZ);
	program_vector.f_da = da;
	program_vector.f_db = db;

	return ts;
}


// Make a delay Vector
double RPSPMC_Control::make_delay_vector (double delay, int source, int options, double &duration, make_vector_flags flags, int points){
	duration += delay;
	program_vector.n = points;
	program_vector.slew = points/delay;
	program_vector.srcs = source;
	program_vector.options = options;
	program_vector.repetitions = 0; // number of repetitions, not used yet
	program_vector.ptr_next = 0x0;  // pointer to next vector -- not used, only for loops
	program_vector.f_du = 0.0;
	program_vector.f_dz = 0.0;
	program_vector.f_dx = 0.0;
	program_vector.f_dy = 0.0;
	program_vector.f_da = 0.0;
	program_vector.f_db = 0.0;
	return delay;
}

// Make Vector Table End
void RPSPMC_Control::append_null_vector (int options, int index){
	// NULL vector -- just to clean vector table
	program_vector.n = 0;
	program_vector.slew = 0.0;
	program_vector.n = 0;
	program_vector.srcs = 0x0000;
	program_vector.options = options;
	program_vector.repetitions = 0; // number of repetitions
	program_vector.ptr_next = 0;  // END
	program_vector.f_dx = 0.0;
	program_vector.f_dy = 0.0;
	program_vector.f_dz = 0.0;
	program_vector.f_du = 0.0;
	program_vector.f_da = 0.0;
	program_vector.f_db = 0.0;
	write_program_vector (index);
}

static void via_remote_list_Check_ec(Gtk_EntryControl* ec, remote_args* ra){
	ec->CheckRemoteCmd (ra);
};

void RPSPMC_Control::write_spm_scan_vector_program (double rx, double ry, int nx, int ny, double slew[2], int subscan[4], long int srcs[4]){
        int vector_index=0;

        double tfwd = slew[0]/rx;
        double trev = slew[1]/rx;
        double ti = slew[0]/sqrt(rx*rx+ry*ry);

        // new convert Ang to Volts
        rx = main_get_gapp()->xsm->Inst->XA2Volt (rx); // WARNING, not yet accounting for rotation here in case x and y piezo sensitivities are not the same!
        ry = main_get_gapp()->xsm->Inst->YA2Volt (ry);
        
        double xi = -rx/2.+rx*subscan[0]/nx;
        double yi =  ry/2.-ry*subscan[2]/ny;
        double dx = rx*subscan[1]/nx; // scan X vector length
        double dy = ry*subscan[3]/ny; // scan Y vector length

        /*
write spm scan GVP: ti, fwd, rev= 1.80086s, 2.5468s, 2.5468s;  xi,yi=(-0.05, 0.05)V, dx,dy=(0.1, 0.1)V nx,ny=(100, 100) subscan=[[0 100][0 100]], srcs0=0x00000010
** Message: 20:16:17.263: GVP_write_program_vector[0]: srcs = 0x00000001
Vec[ 0] XYZU: -5e-05 5e-05 0 0 V <== VPos XYZU: 0 0 0 0 V [0 A 0 A 0 A 0 V] SRCSO=00000001
** Message: 20:16:17.263: GVP_write_program_vector[1]: srcs = 0x00001001
Vec[ 1] XYZU: 0.0001 0 0 0 V
** Message: 20:16:17.263: GVP_write_program_vector[2]: srcs = 0x00001001
Vec[ 2] XYZU: -0.0001 0 0 0 V
** Message: 20:16:17.263: GVP_write_program_vector[3]: srcs = 0x00001001
Vec[ 3] XYZU: 0 1e-06 0 0 V
** Message: 20:16:17.263: GVP_write_program_vector[4]: srcs = 0x00000000
Vec[ 4] XYZU: 0 0 0 0 V
** Message: 20:16:17.263: rpspmc_hwi_dev::GVP_execute_vector_program ()
** Message: 20:16:17.263: FifoReadThread Start
** Message: 20:16:17.263: FifoReadThread nx,ny = (100, 100) @ (0, 0)
** Message: 20:16:17.263: Scan VP: waiting for header data
VectorDef{   Decii,        0,      0,      0,     dB,     dA,     dU,     dZ,     dY,      dX,      nxt,  repts,       opts,     nii,      n ,     pc}
vector = {32'd375179,  32'd0,  32'd0,  32'd0,  32'd0,  32'd0,  32'd0,  32'd0,  32'd72, -32'd072,  32'd0,  32'd0000, 32'h0001,  32'd2,  32'd99,  32'd0}; // Vector #0
vector = {32'd530583,  32'd0,  32'd0,  32'd0,  32'd0,  32'd0,  32'd0,  32'd0,  32'd00,  32'd143,  32'd0,  32'd0000, 32'h1001,  32'd2,  32'd99,  32'd1}; // Vector #1
vector = {32'd530583,  32'd0,  32'd0,  32'd0,  32'd0,  32'd0,  32'd0,  32'd0,  32'd00, -32'd143,  32'd0,  32'd0000, 32'h1001,  32'd2,  32'd99,  32'd2}; // Vector #2
vector = {32'd053058,  32'd0,  32'd0,  32'd0,  32'd0,  32'd0,  32'd0,  32'd0,  32'd14,  32'd000, -32'd2,  32'd9900, 32'h1001,  32'd2,  32'd9,  32'd3}; // Vector #3
vector = {32'd000032,  32'd0,  32'd0,  32'd0,  32'd0,  32'd0,  32'd0,  32'd0,  32'd00,  32'd000,  32'd0,  32'd0000, 32'h0,  32'd0,  32'd0,  32'd4}; // Vector #4
{Info: {RESET:{true}}}
        */

        g_message ("write spm scan GVP: ti, fwd, rev= %gs, %gs, %gs;  xi,yi=(%g, %g)V, dx,dy=(%g, %g)V nx,ny=(%d, %d) subscan=[[%d %d][%d %d]], srcs0=0x%08x",
                   ti, tfwd, trev,
                   xi, yi,
                   dx, dy,
                   nx, ny,
                   subscan[0], subscan[1], subscan[2], subscan[3], 
                   srcs[0]);
        
        // may wait a sec to assumre monitors been up-to-date?
        make_UZXYramp_vector (0., 0., // GVP_du[k], GVP_dz[k],
                              xi, yi, 0., 0., // GVP_dx[k], GVP_dy[k], GVP_da[k], GVP_db[k],
                              100, 0, 0, ti, // GVP_points[k], GVP_vnrep[k], GVP_vpcjr[k], GVP_ts[k],
                              0, VP_INITIAL_SET_VEC);
        write_program_vector (vector_index++);
        make_UZXYramp_vector (0., 0., // GVP_du[k], GVP_dz[k],
                              dx, 0, 0., 0., // GVP_dx[k], GVP_dy[k], GVP_da[k], GVP_db[k],
                              subscan[1], 0, 0, tfwd, // GVP_points[k], GVP_vnrep[k], GVP_vpcjr[k], GVP_ts[k],
                              srcs[0], 0); // vis_Source, GVP_opt[k]);
        write_program_vector (vector_index++);
        make_UZXYramp_vector (0., 0., // GVP_du[k], GVP_dz[k],
                              -dx, 0, 0., 0., // GVP_dx[k], GVP_dy[k], GVP_da[k], GVP_db[k],
                              subscan[1], 0, 0, trev, // GVP_points[k], GVP_vnrep[k], GVP_vpcjr[k], GVP_ts[k],
                              srcs[1], 0); // vis_Source, GVP_opt[k]);
        write_program_vector (vector_index++);
        make_UZXYramp_vector (0., 0., // GVP_du[k], GVP_dz[k],
                              0., dy/(subscan[3]-1), 0., 0., // GVP_dx[k], GVP_dy[k], GVP_da[k], GVP_db[k],
                              10, subscan[3], -2, tfwd/ny, // GVP_points[k], GVP_vnrep[k], GVP_vpcjr[k], GVP_ts[k],
                              srcs[0], 0); // vis_Source, GVP_opt[k]);
        write_program_vector (vector_index++);
        append_null_vector (options, vector_index);
}


// Create Vector Table form Mode (pvm=PV_MODE_XXXXX) and Execute if requested (start=TRUE) or write only
void RPSPMC_Control::write_spm_vector_program (int start, pv_mode pvm){
	int options=0;
	int options_FBon=0;
	int ramp_sources=0x000;
	int ramp_points;
	int recover_options=0;
	int vector_index = 0;
	double vp_duration_0 = 0;
	double vp_duration_1 = 0;
	double vp_duration_2 = 0;
	double vp_duration = 0;
	double dU_IV=0.;
	double dU_step=0.;
	int vpci;
	double vp_dt_sec=0.;
	gboolean remote_set_post_start = FALSE;
        int warn_flag=FALSE;
	gchar *info=NULL;
	
	if (!rpspmc_hwi) return; 

	if (!start) // reset 
		probe_trigger_single_shot = 0;


        g_message ("Srcs: %08x vSrcs: %08x", Source, vis_Source);

	switch (pvm){
	case PV_MODE_IV: // ------------ Special Vector Setup for IV Probes "Probe ala STM"
                g_free (vp_exec_mode_name); vp_exec_mode_name = g_strdup ("VP: STS");
		options      = (IV_option_flags & FLAG_FB_ON     ? 0      : VP_FEEDBACK_HOLD)
                        | (IV_option_flags & FLAG_INTEGRATE ? VP_AIC_INTEGRATE : 0);
		ramp_sources = IV_option_flags & FLAG_SHOW_RAMP ? vis_Source : 0x000;
		recover_options = 0; // FeedBack On for recovery!

		vpci = vector_index;
		vp_duration_0 =	vp_duration;
                break;
                
	case PV_MODE_GVP: // ------------ Special Vector Setup for GVP (general vector probe)
                g_free (vp_exec_mode_name); vp_exec_mode_name = g_strdup ("VP: Vector Program");

		// setup vector program
		{
			double vpd[N_GVP_VECTORS];
			int k=0;
			for (k=0; k<N_GVP_VECTORS; ++k){
				vpd[k] = vp_duration;

				if (GVP_ts[k] <= 0. && GVP_points[k] == 0) // end VP mark found
					break;

				if (GVP_vnrep[k] < 0)
					GVP_vnrep[k] = 0;
				if (GVP_vpcjr[k] < -vector_index || GVP_vpcjr[k] > 0) // origin of VP, no forward jump
					GVP_vpcjr[k] = -vector_index; // defaults to start
                                
				vp_duration += make_UZXYramp_vector (GVP_du[k], GVP_dz[k], GVP_dx[k], GVP_dy[k], GVP_da[k], GVP_db[k],
                                                                     GVP_points[k], GVP_vnrep[k], GVP_vpcjr[k], GVP_ts[k],
                                                                     vis_Source, GVP_opt[k]);
				write_program_vector (vector_index++);
				if (GVP_vnrep[k]-1 > 0)	
					vp_duration +=	(GVP_vnrep[k]-1)*(vp_duration - vpd[k+GVP_vpcjr[k]]);
			}
		}

		append_null_vector (options, vector_index);

		//rpspmc_hwi->probe_time_estimate = (int)vp_duration; // used for timeout check

		info = g_strdup_printf ("T=%.2f ms", 1e3*(double)vp_duration);
		if (GVP_status)
                        gtk_entry_buffer_set_text (GTK_ENTRY_BUFFER (gtk_entry_get_buffer (GTK_ENTRY(GVP_status))), info, -1);
		break;
	}

	if (start){
                g_message ("Executing Vector Probe Now! Mode: %s", vp_exec_mode_name);
		main_get_gapp()->monitorcontrol->LogEvent ("VectorProbe Execute", vp_exec_mode_name);
		main_get_gapp()->monitorcontrol->LogEvent ("VectorProbe", info, 2);
                // may want to make sure LockIn settings are up to date...
                rpspmc_hwi->GVP_execute_vector_program(); // non blocking
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
	rpspmc_hwi->write_dsp_lockin_probe_final (AC_amp, AC_frq, AC_phaseA, AC_phaseB, AC_lockin_avg_cycels, FZ_limiter_val, 
							  noise_amp,
							  start);
#endif

	// Update EC's
	update_GUI();

	// Update from DSP
	read_spm_vector_program ();
}


void RPSPMC_Control::write_program_vector (int index){
        if (!rpspmc_hwi) return; 

        rpspmc_hwi->GVP_write_program_vector (index, &program_vector);


	// update GXSM's internal copy of vector list
	program_vector_list[index].n = program_vector.n;
	program_vector_list[index].slew = program_vector.slew;
	program_vector_list[index].srcs = program_vector.srcs;
	program_vector_list[index].options = program_vector.options;
	program_vector_list[index].repetitions = program_vector.repetitions;
	program_vector_list[index].iloop = program_vector.repetitions; // init
	program_vector_list[index].ptr_next = program_vector.ptr_next;
	program_vector_list[index].f_du = program_vector.f_du;
	program_vector_list[index].f_dx = program_vector.f_dx;
	program_vector_list[index].f_dy = program_vector.f_dy;
	program_vector_list[index].f_dz = program_vector.f_dz;
	program_vector_list[index].f_da = program_vector.f_da;
	program_vector_list[index].f_db = program_vector.f_db;

	{ 
		gchar *pvi = g_strdup_printf ("ProbeVector[pc%02d]", index);
		gchar *pvd = g_strdup_printf ("(n:%05d, slew: %g pts/s, 0x%04x, 0x%04x, rep:%4d, jrvpc:%d),"
					      "(dU:%6.4f V, dxzy:%6.4f, %6.4f, %6.4f V, da: %6.4f db: %6.4f V)", 
					      program_vector.n, program_vector.slew,
					      program_vector.srcs, program_vector.options,
					      program_vector.repetitions, program_vector.ptr_next,
					      program_vector.f_du, program_vector.f_dx, program_vector.f_dy, program_vector.f_dz,
					      program_vector.f_da, program_vector.f_db
                                              );

		main_get_gapp()->monitorcontrol->LogEvent (pvi, pvd, 2);
		g_free (pvi);
		g_free (pvd);
	}
}

void RPSPMC_Control::read_program_vector (int index){
        if (!rpspmc_hwi) return; 
	rpspmc_hwi->GVP_read_program_vector (index, &program_vector);
}


void RPSPMC_Control::write_dsp_abort_probe (){
        if (!rpspmc_hwi) return; 
	rpspmc_hwi->GVP_abort_vector_program ();
}

