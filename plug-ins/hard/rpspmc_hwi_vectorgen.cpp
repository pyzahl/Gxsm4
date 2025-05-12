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


static void via_remote_list_Check_ec(Gtk_EntryControl* ec, remote_args* ra){
	ec->CheckRemoteCmd (ra);
};

#define DEBUG_GVP_SCAN (PC, DU, DX, DY, DZ, DA, DB, TS, PTS, OPT, VNR, VPCJ) { \
        dspc->GVP_du[PC] = DU;                                          \
        dspc->GVP_dx[PC] = DX;                                          \
        dspc->GVP_dy[PC] = DY;                                          \
        dspc->GVP_dz[PC] = DZ;                                          \
        dspc->GVP_da[PC] = DA;                                          \
        dspc->GVP_db[PC] = DB;                                          \
        dspc->GVP_dam[PC] = 0;                                          \
        dspc->GVP_dfm[PC] = 0;                                          \
        dspc->GVP_ts[PC] = TS;                                          \
        dspc->GVP_points[PC] = PTS;                                     \
        dspc->GVP_opt[PC] = OPT;                                        \
        dspc->GVP_vnrep[PC] = VNR;                                      \
        dspc->GVP_vpcjr[PC] = VPCJ;                                     \
        }                                                               \


double RPSPMC_Control::make_dUdz_vector (int index, double dU, double dZ, int n, double slope, int source, int options){
        double slew = 100.;
        if (n < 2) n=100;

        if (fabs (dU) > 1e-6 && slope > 1e-6){
                slew = n/(slope*fabs(dU));
        } else {
                g_warning ("make_Udz_vector  dU too small or slope too small, using fallback settings:");
        }
        g_message ("make_Udz_vector  dU=%g dz=%g slew=%g pts/s, slope=%g V/s n=%d", dU, dZ, slew, slope, n);
        
        program_vector.n = n;
        program_vector.slew = slew;
        program_vector.srcs = source;
        program_vector.options = options;
        program_vector.repetitions = 0;
        program_vector.ptr_next = 0;
                
        program_vector.f_du = dU/main_get_gapp()->xsm->Inst->BiasGainV2V ();
        program_vector.f_dx = 0.0;
        program_vector.f_dy = 0.0;
        program_vector.f_dz = main_get_gapp()->xsm->Inst->ZA2Volt (dZ);
        program_vector.f_da = 0.0;
        program_vector.f_db = 0.0;
        program_vector.f_dam = 0.0;
        program_vector.f_dfm = 0.0;

        print_vector ("make_Udz_vector", index);
        write_program_vector (index);

        return dU/slope;
}

// make ZXY rampe vector w slope
double RPSPMC_Control::make_ZXYramp_vector (int index, double dZ, double dX, double dY, int n, double slope, int source, int options){
        double dr = sqrt(dZ*dZ + dX*dX + dY*dY);
        double tm = dr/slope;

        program_vector.n = n;
        program_vector.slew = n/tm;
        program_vector.srcs = source;
        program_vector.options = options;
        program_vector.repetitions = 0;
        program_vector.ptr_next = 0x0;
        program_vector.f_du = 0.0;
        program_vector.f_dx = main_get_gapp()->xsm->Inst->XA2Volt (dX);
        program_vector.f_dy = main_get_gapp()->xsm->Inst->YA2Volt (dY);
        program_vector.f_dz = main_get_gapp()->xsm->Inst->ZA2Volt (dZ);
        program_vector.f_da = 0.0;
        program_vector.f_db = 0.0;
        program_vector.f_dam = 0.0;
        program_vector.f_dfm = 0.0;

        print_vector ("make_ZXYramp_vector", index);
        write_program_vector (index);

        return tm;
}
        
// make dU/dZ/dX/dY vector for n points and ts time per segment
double RPSPMC_Control::make_dUZXYAB_vector (int index, double dU, double dZ, double dX, double dY, double da, double db, double dam, double dfm, int n, int nrep, int ptr_next, double ts, int source, int options, int xopcd, double xval, int xrchi, int xjmpr){
        program_vector.n = n;

//** WARNING ** make_dUZXYAB_vector[0]: slew or n out of range. [n=1000, ts=0.1 s] slew=10000 pts/s-- using safety fallback
        
        if (ts < 1e-9 || n > (1<<30) || n < 2){
                g_warning ("make_dUZXYAB_vector[%d]: slew or n out of range. [n=%d, ts=%g s] slew=%g pts/s ** using safety fallback slew=1000 pts/s.", index, n, ts, program_vector.slew);
                program_vector.slew = 1000.;
        } else
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
        program_vector.f_dam = dam;
        program_vector.f_dfm = dfm;

        program_vectorX.opcd = xopcd;
        program_vectorX.cmpv = xval;
        program_vectorX.rchi = xrchi;
        program_vectorX.jmpr = xjmpr;
        
        print_vector ("make_dUZXYAB_vector", index);
        write_program_vector (index);
        
        return ts;
}

// make dU/dZ/dX/dY vector for n points and ts time per segment
double RPSPMC_Control::make_dUZXYAB_vector_all_volts (int index, double dU, double dZ, double dX, double dY, double da, double db, double dam, double dfm, int n, int nrep, int ptr_next, double ts, int source, int options){
        program_vector.n = n;

//** WARNING ** make_dUZXYAB_vector[0]: slew or n out of range. [n=1000, ts=0.1 s] slew=10000 pts/s-- using safety fallback
        
        if (ts < 1e-9 || n > (1<<30) || n < 2){
                g_warning ("make_dUZXYAB_vector[%d]: slew or n out of range. [n=%d, ts=%g s] slew=%g pts/s ** using safety fallback slew=1000 pts/s.", index, n, ts, program_vector.slew);
                program_vector.slew = 1000.;
        } else
                program_vector.slew = n/ts;
        program_vector.srcs = source;
        program_vector.options = options;
        program_vector.repetitions = nrep;
        program_vector.ptr_next = ptr_next;

        program_vector.f_du = dU;
        program_vector.f_dx = dX;
        program_vector.f_dy = dY;
        program_vector.f_dz = dZ;
        program_vector.f_da = da;
        program_vector.f_db = db;
        program_vector.f_dam = dam;
        program_vector.f_dfm = dfm;

        print_vector ("make_dUZXYAB_vector", index);
        write_program_vector (index);
        
        return ts;
}


// Make a delay Vector
double RPSPMC_Control::make_delay_vector (int index, double delay, int source, int options, int nrep=0, int ptr_next=0, int points=100){
        if (points < 2) points = 100;
        if (delay < 1e-7){
                g_warning ("VVVVV::make_delay_vector delay too small, auto adjusting. asking for %g s", delay);
                delay = 1e-6;
        }
        double slew = points/delay;

        g_message ("VVVVV::make_delay_vector %g pts/s dt=%g n=%d", slew, delay, points);

        program_vector.n = points;
        program_vector.slew = slew;
        program_vector.srcs = source;
        program_vector.options = options;
        program_vector.repetitions = nrep;   // number of repetitions
        program_vector.ptr_next = ptr_next;  // pointer to next vector
        program_vector.f_du = 0.0;
        program_vector.f_dz = 0.0;
        program_vector.f_dx = 0.0;
        program_vector.f_dy = 0.0;
        program_vector.f_da = 0.0;
        program_vector.f_db = 0.0;
        program_vector.f_dam = 0.0;
        program_vector.f_dfm = 0.0;

        print_vector ("make_delay_vector", index);
        write_program_vector (index);

        return delay;
}

// Make Vector Table End
void RPSPMC_Control::append_null_vector (int index, int options){
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
        program_vector.f_dam = 0.0;
        program_vector.f_dfm = 0.0;

        print_vector ("append_null_vector", index);
        write_program_vector (index);
}



void RPSPMC_Control::write_spm_scan_vector_program (double rx, double ry, int nx, int ny, double slew[2], int subscan[4], long int srcs[4], int gvp_options){
        static int subscan_buffer[4] = {-1,-1,-1,-1};
        static int srcs_buffer[4] = {0,0,0,0};
        
        int vector_index=0;

        double tfwd = rx/slew[0];
        double trev = rx/slew[1];

        if (subscan == NULL && subscan_buffer[0] < 0)
                return;
        
        if (subscan && srcs){
                subscan_buffer[0]=subscan[0];
                subscan_buffer[1]=subscan[1];
                subscan_buffer[2]=subscan[2];
                subscan_buffer[3]=subscan[3];
                srcs_buffer[0]=srcs[0];
                srcs_buffer[1]=srcs[1];
                srcs_buffer[2]=srcs[2];
                srcs_buffer[3]=srcs[3];
        }
        
        double xi = -rx/2.+rx*(double)subscan_buffer[0]/(double)nx;
        double yi =  ry/2.-ry*(double)subscan_buffer[2]/(double)ny;
        double ti = sqrt(xi*xi+yi*yi)/slew[1];

        //g_message ("write spm scan GVP from: rx,y: %g V, %g V, slew: %g A/s, %g A/s -> tifr: %g, %g, %g", rx,ry, slew[0], slew[1], ti, tfwd, trev);

        double dx = rx*(double)subscan_buffer[1]/(double)nx; // scan X vector length
        double dy = ry*(double)subscan_buffer[3]/(double)ny; // scan Y vector length

        //g_message ("write spm scan GVP: dx,dy: %g, %g uV", dx*e16, dy*1e6);

        /*
write spm scan GVP: ti, fwd, rev= 1.80086s, 2.5468s, 2.5468s;  xi,yi=(-0.05, 0.05)V, dx,dy=(0.1, 0.1)V nx,ny=(100, 100) subscan_buffer=[[0 100][0 100]], srcs_buffer0=0x00000010
** Message: 20:16:17.263: GVP_write_program_vector[0]: srcs_buffer = 0x00000001
Vec[ 0] XYZU: -5e-05 5e-05 0 0 V <== VPos XYZU: 0 0 0 0 V [0 A 0 A 0 A 0 V] SRCS_BUFFERO=00000001
** Message: 20:16:17.263: GVP_write_program_vector[1]: srcs_buffer = 0x00001001
Vec[ 1] XYZU: 0.0001 0 0 0 V
** Message: 20:16:17.263: GVP_write_program_vector[2]: srcs_buffer = 0x00001001
Vec[ 2] XYZU: -0.0001 0 0 0 V
** Message: 20:16:17.263: GVP_write_program_vector[3]: srcs_buffer = 0x00001001
Vec[ 3] XYZU: 0 1e-06 0 0 V
** Message: 20:16:17.263: GVP_write_program_vector[4]: srcs_buffer = 0x00000000
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

        // ** Message: 23:34:25.068: write spm scan GVP: ti, fwd, rev= 0.0707107s, 0.1s, 0.1s;  xi,yi=(-1, 1)V, dx,dy=(2, 2)V nx,ny=(100, 100) subscan_buffer=[[0 100][0 100]], srcs_buffer=0x00000039, 0x00000000

        g_message ("write spm scan GVP: ti, fwd, rev= %gs, %gs, %gs;  xi,yi=(%g, %g)V, dx,dy=(%g, %g)V nx,ny=(%d, %d) subscan_buffer=[[%d %d][%d %d]], srcs_buffer=0x%08x, 0x%08x",
                   ti, tfwd, trev,
                   xi, yi,
                   dx, dy,
                   nx, ny,
                   subscan_buffer[0], subscan_buffer[1], subscan_buffer[2], subscan_buffer[3], 
                   srcs_buffer[0],srcs_buffer[1]);
        
        // may wait a sec to assumre monitors been up-to-date?
        // initial vector to start
        //?? Vec[ 0] XYZU: -0.005 0.005 0 0 V  [#100, R0 J0 SRCS_BUFFER=00003900] initial Msk=1000

        make_dUZXYAB_vector (vector_index++,
                             0., 0., // GVP_du[k], GVP_dz[k],
                             xi, yi, 0., 0., 0., 0., // GVP_dx[k], GVP_dy[k], GVP_da[k], GVP_db[k], am, fm
                             100, 0, 0, ti, // GVP_points[k], GVP_vnrep[k], GVP_vpcjr[k], GVP_ts[k],
                             srcs_buffer[0], VP_INITIAL_SET_VEC | gvp_options | (3 << 16));

        //DEBUG_GVP_SCAN (PC,         DU, DX, DY, DZ, DA, DB,  TS, PTS, OPT,                VNR, VPCJ)
        //DEBUG_GVP_SCAN (vector_index, 0., xi, xi,  0., 0., 0., ti, 100, VP_INITIAL_SET_VEC,   0,   0);
        //?? Vec[ 1] XYZU: 0.01 0 0 0 V  [#100, R0 J0 SRCS_BUFFER=00003900] initial Msk=0001

        // fwd scan "->"
        make_dUZXYAB_vector (vector_index++,
                             0., 0., // GVP_du[k], GVP_dz[k],
                             dx, 0, 0., 0., 0., 0., // GVP_dx[k], GVP_dy[k], GVP_da[k], GVP_db[k], am, fm
                             subscan_buffer[1], 0, 0, tfwd, // GVP_points[k], GVP_vnrep[k], GVP_vpcjr[k], GVP_ts[k],
                             srcs_buffer[0], gvp_options); // vis_Source, GVP_opt[k]);

        //DEBUG_GVP_SCAN (vector_index, 0., dx, 0.,  0., 0., 0., tfwd, subscan_buffer[1], VP_INITIAL_SET_VEC,   0,   0);

        // rev scan "<-"
        make_dUZXYAB_vector (vector_index++,
                             0., 0., // GVP_du[k], GVP_dz[k],
                             -dx, 0, 0., 0., 0., 0., // GVP_dx[k], GVP_dy[k], GVP_da[k], GVP_db[k], am, fm
                             subscan_buffer[1], 0, 0, trev, // GVP_points[k], GVP_vnrep[k], GVP_vpcjr[k], GVP_ts[k],
                             srcs_buffer[1], gvp_options); // vis_Source, GVP_opt[k]);

        // next line with repeat for all lines from VPC-2 ny times
        make_dUZXYAB_vector (vector_index++,
                             0., 0., // GVP_du[k], GVP_dz[k],
                             0., -dy/(subscan_buffer[3]-1), 0., 0., 0., 0., // GVP_dx[k], GVP_dy[k], GVP_da[k], GVP_db[k], am, fm
                             10, subscan_buffer[3], -2, tfwd/ny, // GVP_points[k], GVP_vnrep[k], GVP_vpcjr[k], GVP_ts[k],
                             srcs_buffer[0], gvp_options); // vis_Source, GVP_opt[k]);

        append_null_vector (vector_index, gvp_options);
}


// Create Vector Table form Mode (pvm=PV_MODE_XXXXX) and Execute if requested (start=TRUE) or write only
void RPSPMC_Control::write_spm_vector_program (int start, pv_mode pvm){
	int options=0;
	int options_FBon=0;
	int ramp_sources=0x000;
        int ramp_options=0;
	int ramp_points;
	int recover_options=0;
	int vector_index = 0;
        int vpci=0;
	double vp_duration_0 = 0.;
	double vp_duration_1 = 0.;
	double vp_duration_2 = 0.;
	double vp_duration = 0.;
	double vp_dt_sec=0.;
	gboolean remote_set_post_start = FALSE;
        int warn_flag=FALSE;
	gchar *info=NULL;
	
	if (!rpspmc_hwi) return; 

	if (!start) // reset 
		probe_trigger_single_shot = 0;

        rpspmc_hwi->resetVPCconfirmed ();

        g_message ("Srcs: %08x vSrcs: %08x", Source, vis_Source);

	switch (pvm){
	case PV_MODE_IV: // ------------ Special Vector Setup for IV Probes "Probe ala STM"
                g_free (vp_exec_mode_name); vp_exec_mode_name = g_strdup ("VP: STS");
		options      = (IV_option_flags & FLAG_FB_ON     ? 0          : VP_FEEDBACK_HOLD);
		ramp_sources = (IV_option_flags & FLAG_SHOW_RAMP ? vis_Source : 0x00000000);
                ramp_options = (RampFBoff_mode ? VP_FEEDBACK_HOLD : 0); // FB on/off for RAMP!

		recover_options = 0; // FeedBack On for recovery!

		vpci = vector_index;
		vp_duration_0 =	vp_duration;

                // ******************** SIMPLE MODE ****************************************
                if (IV_sections  == 0){  // simple mode
                        int vpc=0;
                        double ui,uf;
                        ui = IV_start[0];
                        uf = IV_end[0];
                        double n = IV_points[0];
                        int ni0 = (int)(round (n*fabs (ui/(uf-ui))));
                        int n0f = (int)(round (n*fabs (uf/(uf-ui))));



                        //make_dUZXYAB_vector (GVP_du[k], GVP_dz[k], GVP_dx[k], GVP_dy[k], GVP_da[k], GVP_db[k],
                        //                      GVP_points[k], GVP_vnrep[k], GVP_vpcjr[k], GVP_ts[k],
                        //                      vis_Source, GVP_opt[k]^1); // invert FB flag in bit0, FPGA GVP FB=1 => FB-hold
                        //write_program_vector (vector_index++);
                        //if (GVP_vnrep[k]-1 > 0)	
                        //        vp_duration +=	(GVP_vnrep[k]-1)*(vp_duration - vpd[k+GVP_vpcjr[k]]);

                        int vis_sources = vis_Source;
                        
                        vp_duration_1 =	vp_duration;

                        //double u0=0.0;
                        vp_duration += make_dUZXYAB_vector (vector_index++,
                                                            ui, 0.0,   0.0, 0.0, 0.0, 0.0, 0., 0., // SET Bias using VP_INITIAL
                                                            100, 0, 0, 0.1+(0.01+fabs(ui-bias))/IV_slope_ramp,
                                                            ramp_sources,     // ramp sources
                                                            ramp_options | VP_INITIAL_SET_VEC | (0x08<<16));  // invert FB flag in bit0, FPGA GVP FB=1 => FB-hold, VPSET Bias
                        
                        vp_duration += make_dUZXYAB_vector (vector_index++,
                                                            -ui, IV_dz,   0.0, 0.0, 0.0, 0.0, 0., 0.,
                                                            ni0, 0, 0, (1e-6+fabs(ui))/IV_slope,
                                                            vis_sources,     // ramp sources
                                                            options);  // invert FB flag in bit0, FPGA GVP FB=1 => FB-hold

                        vp_duration += make_dUZXYAB_vector (vector_index++,
                                                            uf, -IV_dz,   0.0, 0.0, 0.0, 0.0, 0., 0.,
                                                            n0f, 0, 0, (1e-6+fabs(uf))/IV_slope,
                                                            vis_sources,     // ramp sources
                                                            options);  // invert FB flag in bit0, FPGA GVP FB=1 => FB-hold

                        if (IV_option_flags & FLAG_DUAL) {
                                // run also reverse probe ramp in dual mode
                                vp_duration += make_dUZXYAB_vector (vector_index++,
                                                                    -uf, IV_dz,   0.0, 0.0, 0.0, 0.0, 0., 0.,
                                                                    n0f, 0, 0, (1e-6+fabs(ui))/IV_slope,
                                                                    vis_sources,     // ramp sources
                                                                    options);  // invert FB flag in bit0, FPGA GVP FB=1 => FB-hold
                                vp_duration += make_dUZXYAB_vector (vector_index++,
                                                                    ui, -IV_dz,   0.0, 0.0, 0.0, 0.0, 0., 0.,
                                                                    ni0, 0, 0, (1e-6+fabs(ui))/IV_slope,
                                                                    vis_sources,     // ramp sources
                                                                    options);  // invert FB flag in bit0, FPGA GVP FB=1 => FB-hold
                                vp_duration += make_dUZXYAB_vector (vector_index++,
                                                                    bias-ui, 0.0,   0.0, 0.0, 0.0, 0.0, 0., 0.,
                                                                    100, 0, 0, 0.1+(1e-6+fabs(bias-ui))/IV_slope_ramp,
                                                                    ramp_sources,     // ramp sources
                                                                    ramp_options);  // invert FB flag in bit0, FPGA GVP FB=1 => FB-hold
                        } else {
                                vp_duration += make_dUZXYAB_vector (vector_index++,
                                                                    bias-uf, 0.0,   0.0, 0.0, 0.0, 0.0, 0., 0.,
                                                                    100, 0, 0, 0.1+(1e-6+fabs(uf-bias))/IV_slope_ramp,
                                                                    ramp_sources,
                                                                    ramp_options);
                        }

                        if (IV_repetitions > 1){
                                // Final vector, gives the IVC some time to recover   
                                if (IV_recover_delay < 1e-4)
                                        IV_recover_delay = 1e-6;
                                vp_duration += make_dUZXYAB_vector (vector_index++,
                                                                    0.0, 0.0,   0.0, 0.0, 0.0, 0.0, 0., 0.,
                                                                    100, IV_repetitions, -vector_index, IV_recover_delay,
                                                                    ramp_sources,     // ramp sources
                                                                    ramp_options);  // invert FB flag in bit0, FPGA GVP FB=1 => FB-hold
                                vp_duration +=	(IV_repetitions-1)*(vp_duration - vp_duration_1);
                        } else {
                                if (IV_recover_delay > 1e-4){
                                        //make_delay_vector (IV_recover_delay, ramp_sources, recover_options, vp_duration, MAKE_VEC_FLAG_NORMAL);
                                        vp_duration += make_dUZXYAB_vector (vector_index++,
                                                                            0.0, 0.0,   0.0, 0.0, 0.0, 0.0, 0., 0.,
                                                                            100, 0.0, 0, IV_recover_delay,
                                                                            ramp_sources,     // ramp sources
                                                                            ramp_options);  // invert FB flag in bit0, FPGA GVP FB=1 => FB-hold
                                }
                        }
                        append_null_vector (vector_index, options);

                } else {                
                // ******************** FULL IV MODE ****************************************
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

                                vp_duration += make_Udz_vector (vector_index++, bias, ui, dz_bi, -1, IV_slope_ramp, ramp_sources, options);
                                vp_duration += make_Udz_vector (vector_index++, ui, u0, dz_i0, n12, IV_slope, vis_Source, options);
                                vp_duration += make_Udz_vector (vector_index++, u0, uf, dz_0f, n23, IV_slope, vis_Source, options);

                                if (IV_option_flags & FLAG_DUAL) {
                                        // run also reverse probe ramp in dual mode
                                        vp_duration += make_Udz_vector (vector_index++, uf, u0, -dz_0f, n23, IV_slope, vis_Source, options);
                                        vp_duration += make_Udz_vector (vector_index++, u0, ui, -dz_i0, n12, IV_slope, vis_Source, options);
                                        // Ramp back to given bias voltage   
                                        vp_duration +=  make_Udz_vector (vector_index++, ui, bias, -dz_bi, -1, IV_slope_ramp, ramp_sources, options);
                                       
                                } else
                                        vp_duration += make_Udz_vector (vector_index++, uf, bias, -(dz_bi+dz_i0+dz_0f), -1, IV_slope_ramp, ramp_sources, options);

                                if (IV_repetitions > 1){
                                        // Final vector, gives the IVC some time to recover   
                                        if (IV_final_delay > 1e-4)
                                                vp_duration += make_delay_vector (vector_index++, IV_final_delay, ramp_sources, options);
                                        if (IV_recover_delay < 1e-4)
                                                IV_recover_delay = 1e-6;
                                        int jpm = -vector_index; // go to start
                                        vp_duration += make_delay_vector (vector_index++, IV_recover_delay, ramp_sources, recover_options, IV_repetitions-1, jpm);
                                        vp_duration +=	(IV_repetitions-1)*(vp_duration - vp_duration_1);
                                } else {
                                        if (IV_final_delay > 1e-4)
                                                vp_duration += vp_duration += make_delay_vector (vector_index++, IV_final_delay, ramp_sources, options);
                                        if (IV_recover_delay > 1e-4)
                                                vp_duration += make_delay_vector (vector_index++, IV_recover_delay, ramp_sources, recover_options);
                                }
                                // add automatic conductivity measurement rho(Z) -- HOLD Bias fixed now!
                                if (IVdz_repetitions > 0){
                                        vp_duration_2 =	vp_duration;

                                        // in case of rep > 1 the DSP will jump back to this point
                                        vpc = vector_index;
	
                                        vp_duration += make_Udz_vector (vector_index++, bias, ui, dz_bi, -1, IV_slope_ramp, ramp_sources, options);
                                        vp_duration += make_Udz_vector (vector_index++, ui, u0, dz_i0, n12, IV_slope, vis_Source, options);
                                        vp_duration += make_Udz_vector (vector_index++, u0, uf, dz_0f, n23, IV_slope, vis_Source, options);
	
                                        if (IV_option_flags & FLAG_DUAL) {
                                                vp_duration += make_Udz_vector (vector_index++, uf, u0, -dz_0f, n23, IV_slope, vis_Source, options);
                                                vp_duration += make_Udz_vector (vector_index++, u0, ui, -dz_i0, n12, IV_slope, vis_Source, options);
                                                vp_duration += make_Udz_vector (vector_index++, ui, bias, -dz_bi, -1, IV_slope_ramp, ramp_sources, options);
                                        } else
                                                vp_duration += make_Udz_vector (vector_index++, uf, bias, -(dz_bi+dz_i0+dz_0f), -1, IV_slope_ramp, ramp_sources, options);
	
                                        if (IVdz_repetitions > 1){
                                                // Final vector, gives the IVC some time to recover   
                                                if (IV_final_delay > 1e-4)
                                                        vp_duration += make_delay_vector (vector_index++, IV_final_delay, ramp_sources, options);
                                                if (IV_recover_delay < 1e-4)
                                                        IV_recover_delay = 1e-6;
                                                int jmp = -(vector_index-vpc); // go to rho start
                                                vp_duration += make_delay_vector (vector_index++, IV_recover_delay, ramp_sources, recover_options, IVdz_repetitions-1, jmp);
                                                vp_duration += (IVdz_repetitions-1)*(vp_duration - vp_duration_2);
                                        } else {
                                                if (IV_final_delay > 1e-4)
                                                        vp_duration += make_delay_vector (vector_index++, IV_final_delay, ramp_sources, options);
                                                if (IV_recover_delay > 1e-4)
                                                        vp_duration += make_delay_vector (vector_index++, IV_recover_delay, ramp_sources, recover_options);
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

                                        vp_duration += make_Udz_vector (vector_index++, bias_prev, ui, dz_bi, -1, IV_slope_ramp, ramp_sources, options);
                                        vp_duration += make_Udz_vector (vector_index++, ui, uf, dz_if, IV_points[IVs], IV_slope, vis_Source, options);
			
                                        // add vector for reverse return ramp? -- Force return path if dz != 0
                                        if (IV_option_flags & FLAG_DUAL) {
                                                // run also reverse probe ramp in dual mode
                                                vp_duration += make_Udz_vector (vector_index++, uf, ui, -dz_if, IV_points[IVs], IV_slope, vis_Source, options);
                                                
                                                if (IVs == (IV_sections-1)) // Ramp back to given bias voltage   
                                                        vp_duration += make_Udz_vector (vector_index++, ui, bias, -dz_bi, -1, IV_slope_ramp, ramp_sources, options);
                                        } else {
                                                if (IVs == (IV_sections-1)) // Ramp back to given bias voltage   
                                                        vp_duration += make_Udz_vector (vector_index++, uf, bias, -(dz_if+dz_bi), -1, IV_slope_ramp, ramp_sources, options);
                                        }

                                        if (IV_repetitions > 1){
                                                // Final vector, gives the IVC some time to recover   
                                                if (IV_final_delay > 1e-4)
                                                        vp_duration += make_delay_vector (vector_index++, IV_final_delay, ramp_sources, options);
                                                if (IVs == (IV_sections-1)){
                                                        if (IV_recover_delay < 1e-4)
                                                                IV_recover_delay = 1e-6;
                                                        int jmp = -vector_index; // go to start
                                                        vp_duration += make_delay_vector (vector_index++, IV_recover_delay, ramp_sources, recover_options, IV_repetitions-1, jmp);
                                                        vp_duration +=	(IV_repetitions-1)*(vp_duration - vp_duration_1);
                                                }
                                        } else {
                                                if (IV_final_delay > 1e-4)
                                                        vp_duration += make_delay_vector (vector_index++, IV_final_delay, ramp_sources, options);
                                                if (IVs == (IV_sections-1)){
                                                        if (IV_recover_delay > 1e-4)
                                                                vp_duration += make_delay_vector (vector_index++, IV_recover_delay, ramp_sources, recover_options);
                                                }
                                        }
                                }

                                // add automatic conductivity measurement rho(Z) -- HOLD Bias fixed now!
                                if ((IVdz_repetitions > 0) && (fabs (IV_dz) > 0.001) && (bias != 0.)){
                                        vp_duration_2 =	vp_duration;

                                        // in case of rep > 1 the DSP will jump back to this point
                                        vpc = vector_index;
	
                                        vp_duration += make_Udz_vector (vector_index++, bias, ui, dz_bi, -1, IV_slope_ramp, ramp_sources, options);
                                        vp_duration += make_Udz_vector (vector_index++, ui, uf, dz_if, IV_points[0], IV_slope, vis_Source, options);
			
                                        if (IV_option_flags & FLAG_DUAL) {
                                                vp_duration += make_Udz_vector (vector_index++, uf, ui, -dz_if, IV_points[0], IV_slope, vis_Source, options);
                                                // Ramp back to given bias voltage   
                                                vp_duration += make_Udz_vector (vector_index++, ui, bias, -dz_bi, -1, IV_slope_ramp, ramp_sources, options);
                                        } else 
                                                vp_duration += make_Udz_vector (vector_index++, uf, bias, -(dz_if+dz_bi), -1, IV_slope_ramp, ramp_sources, options);
	
                                        if (IVdz_repetitions > 1 ){
                                                // Final vector, gives the IVC some time to recover   
                                                if (IV_final_delay > 1e-4)
                                                        vp_duration += make_delay_vector (vector_index++, IV_final_delay, ramp_sources, options);
                                                if (IV_recover_delay < 1e-4)
                                                        IV_recover_delay = 1e-6;
                                                int jmp = -(vector_index-vpc); // go to rho start
                                                vp_duration += make_delay_vector (vector_index++, IV_recover_delay, ramp_sources, recover_options, IVdz_repetitions-1, jmp);
                                                vp_duration +=	(IVdz_repetitions-1)*(vp_duration - vp_duration_2);
                                        } else {
                                                if (IV_final_delay > 1e-4)
                                                        vp_duration += make_delay_vector (vector_index++, IV_final_delay, ramp_sources, options);
                                                if (IV_recover_delay > 1e-4)
                                                        vp_duration += make_delay_vector (vector_index++, IV_recover_delay, ramp_sources, recover_options);
                                        }
                                }
                        }

                        // Step and Repeat along Line defined by dx dy -- if dxy points > 1, auto return?
                        if (IV_dxy_points > 1){

                                // Move probe to next position and setup auto repeat!
                                vp_duration += make_ZXYramp_vector (vector_index++, 0., IV_dx/(IV_dxy_points-1), IV_dy/(IV_dxy_points-1), 100, IV_dxy_slope, ramp_sources, options);

                                if (IV_dxy_delay < 1e-4)
                                        IV_dxy_delay = 1e-6;
                                int jmp = -(vector_index-vpci); // go to initial IV start for full repeat
                                vp_duration += make_delay_vector (vector_index++, IV_dxy_delay, ramp_sources, options, IV_dxy_points-1, jmp);
                                vp_duration +=	(IV_dxy_points-1)*(vp_duration - vp_duration_0);

                                // add vector for full reverse return path -- YES!, always auto return!
                                vp_duration += make_ZXYramp_vector (vector_index++,
                                                                    0., 
                                                                    -IV_dx*(IV_dxy_points)/(IV_dxy_points-1.), 
                                                                    -IV_dy*(IV_dxy_points)/(IV_dxy_points-1.), 100, IV_dxy_slope, 
                                                                    ramp_sources, options);
                        }
                        append_null_vector (vector_index, options);
                }


		//rpspmc_hwi->probe_time_estimate = (int)vp_duration; // used for timeout check

		info = g_strdup_printf ("T=%.2f ms", 1e3*(double)vp_duration);
		if (IV_status)
                        gtk_entry_buffer_set_text (GTK_ENTRY_BUFFER (gtk_entry_get_buffer (GTK_ENTRY(IV_status))), info, -1);
		break;
                
	case PV_MODE_GVP: // ------------ Special Vector Setup for GVP (general vector probe)
                g_free (vp_exec_mode_name); vp_exec_mode_name = g_strdup ("VP: Vector Program");

		// setup vector program
		{
			double vpd[N_GVP_VECTORS];
			int k=0;
			for (k=0; k<N_GVP_VECTORS-1; ++k){ // last must be NULL Vector
				vpd[k] = vp_duration;

				if (GVP_ts[k] <= 0. && GVP_points[k] == 0) // end VP mark found
					break;

				if (GVP_vnrep[k] < 0)
					GVP_vnrep[k] = 0;
				if (GVP_vpcjr[k] < -vector_index || GVP_vpcjr[k] > 0) // origin of VP, no forward jump
					GVP_vpcjr[k] = -vector_index; // defaults to start
                                
                                int viset = 0;
                                if (k==0){
                                        for (int jj=0; jj<8; ++jj){
                                                const gchar *info=GVP_V0Set_ec[jj]->get_info();
                                                if (info)
                                                        if (info[0]=='S')
                                                                viset |= 1<<jj;
                                        }
                                }
				vp_duration += make_dUZXYAB_vector (vector_index++,
                                                                    GVP_du[k], GVP_dz[k], GVP_dx[k], GVP_dy[k], GVP_da[k], GVP_db[k], GVP_dam[k], GVP_dfm[k],
                                                                    GVP_points[k], GVP_vnrep[k], GVP_vpcjr[k], GVP_ts[k],
                                                                    vis_Source, GVP_opt[k]^1 | (viset<<16),     // invert FB flag in bit0, FPGA GVP FB=1 => FB-hold
                                                                    GVPX_opcd[k], GVPX_cmpv[k], GVPX_rchi[k], GVPX_jmpr[k]);
				if (GVP_vnrep[k]-1 > 0)	
					vp_duration +=	(GVP_vnrep[k]-1)*(vp_duration - vpd[k+GVP_vpcjr[k]]);
			}
		}

		append_null_vector (vector_index, options);

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
                rpspmc_hwi->start_data_read (0, 0,0,0,0, NULL,NULL,NULL,NULL); // init data streaming -- non blocking thread is fired up
        }

        g_free (info);

	// --------------------------------------------------


	// Update EC's
	update_GUI();

	// Update from DSP
	read_spm_vector_program ();
}




void RPSPMC_Control::gvp_preview_draw_function (GtkDrawingArea *area, cairo_t *cr,
                                                int             width,
                                                int             height,
                                                RPSPMC_Control *self){

        // DO NEVER RUN SIMULATE WHILE GVP IS ACTIVE!
        if (rpspmc_hwi->is_scanning ())
                return;

        /* from css/styles.css
#gvpcolor1 { background-image: none; background-color: #e8e01b; color: white; font-weight: bold; border-radius: 6px; margin-left: 4px; }
#gvpcolor2 { background-image: none; background-color: #69e81b; color: white; font-weight: bold; border-radius: 6px; margin-left: 4px; }
#gvpcolor3 { background-image: none; background-color: #e81b1b; color: white; font-weight: bold; border-radius: 6px; margin-left: 4px; }
#gvpcolor4 { background-image: none; background-color: #1b82e8; color: white; font-weight: bold; border-radius: 6px; margin-left: 4px; }
#gvpcolor5 { background-image: none; background-color: #1be5e8; color: white; font-weight: bold; border-radius: 6px; margin-left: 4px; }
#gvpcolor6 { background-image: none; background-color: #c61be8; color: white; font-weight: bold; border-radius: 6px; margin-left: 4px; }
        */
        // match CSS, no clue how to read that back in or use w cairo
        const gchar* gvpcolors[9] = { NULL, "#e8e01b","#69e81b","#e81b1b","#1b82e8","#1be5e8","#c61be8","#053b58","#1b06e8" }; 

        // prepare job lookups
        PROBE_VECTOR_GENERIC vi = { 0,0.,0,0, 0,0,0,  0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 };
        PROBE_VECTOR_GENERIC vp = { 0,0.,0,0, 0,0,0,  0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 };
        PROBE_VECTOR_GENERIC v = { 0,0.,0,0, 0,0,0,  0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 };
        double *gvp_y[8] = { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL };
        double *gvp_yp[8] = { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL };
        const gchar *gvp_color[8] = { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL };
        cairo_item_path *gvp_wave[8] = { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL };

        double dumy1, dumy2;
        main_get_gapp()->xsm->hardware->RTQuery ("zxy",  vi.f_dz,  vi.f_dx,  vi.f_dy); // get tip position in volts
        main_get_gapp()->xsm->hardware->RTQuery ("B", vi.f_du, dumy1, dumy2); // Bias, ...
        
        int ns=0; // num selected channels
        for (int i=1; i<=8; ++i)
                if (self->GVP_preview_on[i]){
                        gvp_color[ns] = gvpcolors[i];
                        switch (i){
                        case 1: gvp_y[ns] = &v.f_dx; gvp_yp[ns] = &vp.f_dx; break;
                        case 2: gvp_y[ns] = &v.f_dy; gvp_yp[ns] = &vp.f_dy; break;
                        case 3: gvp_y[ns] = &v.f_dz; gvp_yp[ns] = &vp.f_dz; break;
                        case 4: gvp_y[ns] = &v.f_du; gvp_yp[ns] = &vp.f_du; break;
                        case 5: gvp_y[ns] = &v.f_da; gvp_yp[ns] = &vp.f_da; break;
                        case 6: gvp_y[ns] = &v.f_db; gvp_yp[ns] = &vp.f_db; break;
                        case 7: gvp_y[ns] = &v.f_dam; gvp_yp[ns] = &vp.f_dam; break;
                        case 8: gvp_y[ns] = &v.f_dfm; gvp_yp[ns] = &vp.f_dfm; break;
                        }
                        ++ns;
                }

        if (ns < 1) return;
        
        int n=width-100; //850;
        int m=128;
        int mar=18;

        //gtk_drawing_area_set_content_width (area, n+100);
        gtk_drawing_area_set_content_height (area, m);

        cairo_translate (cr, 50., m/2);
        cairo_scale (cr, 1., 1.);
        cairo_save (cr);

        // coordinate system lines
        cairo_item_path *wave = new cairo_item_path (2);
        wave->set_line_width (0.5);
        wave->set_stroke_rgba (CAIRO_COLOR_BLACK);
        wave->set_xy_fast (0,0,0);
        wave->set_xy_fast (1,n-1,0);
        wave->draw (cr);
        wave->set_stroke_rgba (CAIRO_COLOR_BLACK);
        wave->set_xy_fast (0,0,(m-mar)/2);
        wave->set_xy_fast (1,n-1,(m-mar)/2);
        wave->draw (cr);
        wave->set_stroke_rgba (CAIRO_COLOR_BLACK);
        wave->set_xy_fast (0,0,-(m-mar)/2);
        wave->set_xy_fast (1,n-1,-(m-mar)/2);
        wave->draw (cr);
        delete wave;

        int Ns;
        int N = self->calculate_GVP_total_number_points (Ns);
        // g_message ("N=%d, Ns=%d, W:%d", N, Ns, width);
        if ( N > 0){
                int Nsc=0;
                // gvp waves
                for (int k=0; k<ns; ++k){
                        gvp_wave[k] = new cairo_item_path (N);
                        gvp_wave[k]->set_line_width (2.0);
                        gvp_wave[k]->set_stroke_rgba (gvp_color[k]);
                }
                double t=0;
                int pcp=-1;
                int pc=0;
                int il=0;
                double Tfin = self->simulate_vector_program(N, &v, &pc);
                pc=0;

                // annotations
                cairo_item_text *tms = new cairo_item_text ();
                tms->set_font_face_size ("Ununtu", 8.);
                cairo_item_path *sec = new cairo_item_path (2);
                sec->set_line_width (0.5);
                sec->set_stroke_rgba (CAIRO_COLOR_BLACK);
                
                #define GVPPREVIEW_MAX_ANNOTATIONS 20
                double anno_tyy[GVPPREVIEW_MAX_ANNOTATIONS][1+6];
                int i_anno=0;
                double tp=0.;
                double pcxyp=-100;
                double txyp=-100;
                int    tlabposy=0;
                int    pclabposy=0;
                for (int i=0; i<N; i++){
                        memcpy (&v, &vi, sizeof(v));
                        //memset (&v, 0, sizeof(v));
                        t = self->simulate_vector_program(i, &v, &pc, &il);
                        //g_print ("%03d %02d l{%03d} %g:  %6.3g %6.3g %6.3g %6.3g\n", i, pc, self->program_vector_list[pc].iloop, t, v.f_du,v.f_dx,v.f_dy,v.f_dz);
                        for (int k=0; k<ns; ++k)
                                gvp_wave[k]->set_xy_fast (i, n*t/Tfin, *gvp_y[k]);

                        if (i == N-1){ tp=t; memcpy (&vp, &v, sizeof(vp)); } // use last!
                        if (pcp != pc || i == N-1){ // section annotations
                                if (i_anno < GVPPREVIEW_MAX_ANNOTATIONS){
                                        anno_tyy [i_anno][0]=n*tp/Tfin;
                                        for (int k=0; k<ns; ++k)
                                                anno_tyy [i_anno][1+k] = *gvp_yp[k];
                                        i_anno++;
                                }
                                sec->set_xy_fast (0,n*t/Tfin,-m/2);
                                sec->set_xy_fast (1,n*t/Tfin,m/2);
                                sec->draw (cr);
                                tms->set_anchor (CAIRO_ANCHOR_W);
                                tms->set_stroke_rgba (CAIRO_COLOR_BLACK);
                                const gchar* lab=g_strdup_printf("%g ms",1e3*tp);
                                {
                                        double tlabposx=n*t/Tfin+2;
                                        if (txyp+35 > tlabposx) ++tlabposy%=4; // try to not overlap
                                        else tlabposy=0;
                                        txyp=tlabposx;
                                        if (++Nsc > 200){
                                                g_message ("Annotaions / Section Count Limit reached for meaningful visualization. #S=%d. Stopping plot here.", Nsc);
                                                tms->set_stroke_rgba (CAIRO_COLOR_RED);
                                                tms->set_font_face_size ("Ununtu", 18.);
                                                tms->set_text (100, -10, "STOPPING PLOT: Section Count Limit reached.");
                                                tms->draw (cr);
                                                g_free (lab);
                                                tms->set_font_face_size ("Ununtu", 8.);
                                                break;
                                        } else
                                                tms->set_text (tlabposx, 60-10*tlabposy, lab);
                                        tms->draw (cr);
                                        g_free (lab);
                                }
                                if (i != N-1){
                                        tms->set_stroke_rgba (CAIRO_COLOR_RED);
                                        if (il>0)
                                                lab=g_strdup_printf("%d %d",pc, il);
                                        else
                                                lab=g_strdup_printf("%d",pc);
                                        double tlabposx=n*t/Tfin+2;
                                        if (pcxyp+8 > tlabposx) ++pclabposy%=3; // try to not overlap
                                        else pclabposy=0;
                                        pcxyp=tlabposx;
                                        tms->set_text (tlabposx, -60+10*pclabposy, lab);
                                        tms->draw (cr);
                                        g_free (lab);
                                }
                                pcp = pc;
                        }
                        tp=t; memcpy (&vp, &v, sizeof(vp));
                }
                delete sec;
                double skl = -(m-mar)/2.;

                // auto scaling + max U label
                double gvpy_amax[8];
                for (int k=0; k<ns; ++k){
                        gvpy_amax[k] = gvp_wave[k]->auto_range_y (skl);
                        gvp_wave[k]->draw (cr);
                        delete gvp_wave[k];

                        //tms->set_anchor (k&1 ? CAIRO_ANCHOR_E : CAIRO_ANCHOR_W);
                        tms->set_anchor (CAIRO_ANCHOR_E);
                        tms->set_stroke_rgba (gvp_color[k]);
                        const gchar* lab=g_strdup_printf("%.3g V",gvpy_amax[k]);
                        //tms->set_text (k&1 ? -2 : n+2, -55+(k/2)*8, lab);
                        tms->set_text (-2, -55+k*8, lab);
                        tms->draw (cr);
                        g_free (lab);

                }
                // turn points U labels
                tms->set_anchor (CAIRO_ANCHOR_W);
                for (int i=0; i<i_anno; ++i){
                        for (int k=0; k<ns; ++k){
                                tms->set_stroke_rgba (gvp_color[k]);
                                const gchar* lab=g_strdup_printf("%.3g V",anno_tyy [i][1+k]);
                                tms->set_text (anno_tyy [i+k][0]+20, anno_tyy [i][1+k]/gvpy_amax[k]*skl, lab);
                                tms->draw (cr);
                                g_free (lab);
                        }
                }
                
                delete tms;
        }
}


void RPSPMC_Control::write_program_vector (int index){
        if (!rpspmc_hwi) return; 

        rpspmc_hwi->GVP_write_program_vector (index, &program_vector, program_vector.options & (1<<7) ? &program_vectorX : NULL);

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
	program_vector_list[index].f_dam = program_vector.f_dam;
	program_vector_list[index].f_dfm = program_vector.f_dfm;

        if (program_vector.options & (1<<7)){
                program_vectorX_list[index].opcd = program_vectorX.opcd;
                program_vectorX_list[index].cmpv = program_vectorX.cmpv;
                program_vectorX_list[index].rchi = program_vectorX.rchi;
                program_vectorX_list[index].jmpr = program_vectorX.jmpr;
        } else {
                program_vectorX_list[index].opcd = 0;
                program_vectorX_list[index].cmpv = 0;
                program_vectorX_list[index].rchi = 0;
                program_vectorX_list[index].jmpr = 0;
        }
        
        rpspmc_hwi->last_vector_index = index;

	{ 
		gchar *pvi = g_strdup_printf ("ProbeVector[pc%02d]", index);
		gchar *pvd = g_strdup_printf ("(n:%05d, slew: %g pts/s, 0x%04x, 0x%04x, rep:%4d, jrvpc:%d),"
					      "(dU:%6.4f V, dxzy:%6.4f, %6.4f, %6.4f V, da: %6.4f db: %6.4f V, dAMC: %6.4f dFMC: %6.4f Veq)", 
					      program_vector.n, program_vector.slew,
					      program_vector.srcs, program_vector.options,
					      program_vector.repetitions, program_vector.ptr_next,
					      program_vector.f_du, program_vector.f_dx, program_vector.f_dy, program_vector.f_dz,
					      program_vector.f_da, program_vector.f_db, program_vector.f_dam, program_vector.f_dfm
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

