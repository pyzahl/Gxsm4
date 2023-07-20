/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 8 c-style: "K&R" -*- */

/* Gnome gxsm - Gnome X Scanning Microscopy
 * universal STM/AFM/SARLS/SPALEED/... controlling and
 * data analysis software
 *
 * Gxsm Hardware Interface Plugin Name: spm_template_hwi.C
 * ===============================================
 * 
 * Copyright (C) 1999 The Free Software Foundation
 *
 * Authors: Percy Zahl <zahl@fkp.uni-hannover.de>
 * additional features: Andreas Klust <klust@fkp.uni-hannover.de>
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

#include <sys/ioctl.h>

#include "config.h"
#include "plugin.h"
#include "xsmhard.h"
#include "glbvars.h"


#include "gxsm_app.h"
#include "gxsm_window.h"
#include "surface.h"

#include "../common/pyremote.h"
#include "spm_template_hwi_structs.h"
#include "spm_template_hwi.h"
#include "spm_template_hwi_emulator.h"

extern "C++" {
        extern SPM_Template_Control *Template_ControlClass;
        extern GxsmPlugin spm_template_hwi_pi;
}


double SPM_emulator::simulate_value (XSM_Hardware *xsmhwi, int xi, int yi, int ch){
        const int N_steps=20;
        const int N_blobs=7;
        const int N_islands=10;
        const int N_molecules=512;
        const int N_bad_spots=16;
        static feature_step stp[N_steps];
        static feature_island il[N_islands];
        static feature_blob blb[N_blobs];
        static feature_molecule mol[N_molecules];
        static feature_lattice lat;
        static double fz = 1./main_get_gapp()->xsm->Inst->Dig2ZA(1);
       
        double x = xi*xsmhwi->Dx-xsmhwi->Dx*xsmhwi->Nx/2; // x in DAC units
        double y = (xsmhwi->Ny-yi-1)*xsmhwi->Dy-xsmhwi->Dy*xsmhwi->Ny/2; // y in DAC units, i=0 is top line, i=Ny is bottom line

        // Please Note:
        // spm_template_hwi_pi.app->xsm->... and main_get_gapp()->xsm->...
        // are identical pointers to the main g-application (gapp) class and it is made availabe vie the plugin descriptor
        // in case the global gapp is not exported or used in the plugin. And either one may be used to access core settings.
        
        x = main_get_gapp()->xsm->Inst->Dig2XA ((long)round(x)); // convert to anstroems for model using instrument class, use Scan Gains
        y = main_get_gapp()->xsm->Inst->Dig2YA ((long)round(y)); // convert to anstroems for model

        x += 1.5*g_random_double_range (-main_get_gapp()->xsm->Inst->Dig2XA(2), main_get_gapp()->xsm->Inst->Dig2XA(2));
        y += 1.5*g_random_double_range (-main_get_gapp()->xsm->Inst->Dig2YA(2), main_get_gapp()->xsm->Inst->Dig2YA(2));
        
        //g_print ("XY: %g %g  [%g %g %d %d]",x,y, Dx,Dy, xi,yi);
        
        xsmhwi->invTransform (&x, &y); // apply rotation! Use invTransform for simualtion.
        x += main_get_gapp()->xsm->Inst->Dig2X0A (x0); // use Offset Gains
        y += main_get_gapp()->xsm->Inst->Dig2Y0A (y0);

        //g_print ("XYR0: %g %g",x,y);

        // use template landscape if scan loaded to CH11 !!
        if (main_get_gapp()->xsm->scan[10]){
                double ix,iy;
                main_get_gapp()->xsm->scan[10]->World2Pixel  (x, y, ix,iy);
                data_z_value = main_get_gapp()->xsm->scan[10]->data.s.dz * main_get_gapp()->xsm->scan[10]->mem2d->GetDataPktInterpol (ix,iy);
                return data_z_value;
        }

        double z=0.0;
        for (int i=0; i<N_steps;     ++i) z += stp[i].get_z (x,y, ch);
        for (int i=0; i<N_islands;   ++i) z +=  il[i].get_z (x,y, ch);
        if (Template_ControlClass->options & 2)
                for (int i=0; i<N_blobs;     ++i) z += blb[i].get_z (x,y, ch);
        double zm=0;
        if (Template_ControlClass->options & 1)
                for (int i=0; i<N_molecules; ++i) zm += mol[i].get_z (x,y, ch);
        
        data_z_value = fz * (z + (zm > 0. ? zm : lat.get_z (x,y, ch)));
        return data_z_value;
}




// Vector Program Engine Emulator

void SPM_emulator::vp_init (){
        reset_params ();
        vector = NULL;

        vp_header_current.n    = 0;
        vp_header_current.srcs = 0;
        vp_header_current.time = 0;
        vp_header_current.move_xyz[i_X] = 0;
        vp_header_current.move_xyz[i_Y] = 0;
        vp_header_current.move_xyz[i_Z] = 0;
        vp_header_current.scan_xyz[i_X] = 0;
        vp_header_current.scan_xyz[i_Y] = 0;
        vp_header_current.scan_xyz[i_Z] = 0;
        vp_header_current.bias    = 0;
        vp_header_current.section = 0;

        for (int i=0; i<16; ++i) vp_data_set[i] = 0;
        
        vp_header_current.section = -1; // still invalid
        vp_num_data_sets = 0;
        section_count=ix=iix=lix = 0;
        vp_time = 0;
        vp_index_all = 0;
        vp_clock_start = clock();
     	vp_next_section ();    // setup vector program start
	vp_append_header_and_positionvector ();
        // fire up thread!
}

void SPM_emulator::vp_stop (){
	vector = NULL;

	// Probe Suffix with full end position vector[6]
	vp_append_header_and_positionvector ();

        // undo adjustement -- TDB: smooth or no action in reality
        scan_xyz_vec[i_Z] -= vp_zpos;
        sample_bias -= vp_bias;

        reset_params ();
}

void SPM_emulator::vp_append_header_and_positionvector (){ // size: 14
	// Section header: [SRCS, N, TIME]_32 :: 6
	if (!vector) return;

        vp_header_current.n    = vector->n;
        vp_header_current.srcs = vector->srcs;
        vp_header_current.time = vp_time;
        vp_header_current.move_xyz[i_X] = move_xyz_vec[i_X];
        vp_header_current.move_xyz[i_Y] = move_xyz_vec[i_Y];
        vp_header_current.move_xyz[i_Z] = move_xyz_vec[i_Z];
        vp_header_current.scan_xyz[i_X] = scan_xyz_vec[i_X];
        vp_header_current.scan_xyz[i_Y] = scan_xyz_vec[i_Y];
        vp_header_current.scan_xyz[i_Z] = scan_xyz_vec[i_Z];
        vp_header_current.bias    = sample_bias;
        vp_header_current.section = section_count;

        g_print ("EMU** HPV [%4d Srcs%08x t=%08d s XYZ %g %g %g in V Bias %g V Sec %d]\n", vector->n, vector->srcs, vp_time,
                 DAC2Volt (scan_xyz_vec[i_X]), DAC2Volt (scan_xyz_vec[i_Y]), DAC2Volt (scan_xyz_vec[i_Z]),
                 DAC2Volt (vp_bias), section_count);
}



void SPM_emulator::vp_add_vector (){
	// check for valid vector *** and for limiter active (freeze)
	if (!vector) return;

        // accumulate adjustements (internal use)
	vp_bias += vector->f_du;
	vp_zpos += vector->f_dz; 

        // adjust parameter vectors
        sample_bias += vector->f_du;
        
	scan_xyz_vec[i_X] += vector->f_dx;
	scan_xyz_vec[i_Y] += vector->f_dy;
        scan_xyz_vec[i_Z] += vector->f_dz;
        
	move_xyz_vec[i_X] += vector->f_dx0;
	move_xyz_vec[i_Y] += vector->f_dy0;
	move_xyz_vec[i_Z] += vector->f_dz0;

#if 0
        g_print ("EMU** VP+ [%4d Srcs%08x t=%08d s XYZ %g %g %g in V Bias %g V Sec %d]\n", vector->n, vector->srcs, vp_time,
                 DAC2Volt (scan_xyz_vec[i_X]), DAC2Volt (scan_xyz_vec[i_Y]), DAC2Volt (scan_xyz_vec[i_Z]),
                 DAC2Volt (vp_bias), section_count);
#endif
}


void SPM_emulator::vp_store_data_srcs ()
{
        gint i=0;
        // matching this to "template hardware def as of SOURCE_SIGNAL_DEF source_signals[] = { ... } 
        if (vector->srcs & 0x000001) // Z monitor
                vp_data_set[i++] = scan_xyz_vec[i_Z];
        if (vector->srcs & 0x000002) // Bias monitor
                vp_data_set[i++] = sample_bias;
        if (vector->srcs & 0x000010) // ADC0-I (current input)
                vp_data_set[i++] = sim_current_func1();
        if (vector->srcs & 0x000020) // ADC1
                vp_data_set[i++] = sim_current_func2();
        if (vector->srcs & 0x000040) // ADC2
                vp_data_set[i++] = vp_zpos; // ADC_IN(2);
        if (vector->srcs & 0x000080) // ADC3
                vp_data_set[i++] = vp_bias; // ADC_IN(3);
        if (vector->srcs & 0x000100) // ADC4
                vp_data_set[i++] = vp_time;
        if (vector->srcs & 0x000200) // ADC5
                vp_data_set[i++] = (double)(clock () - vp_clock_start)*1000/CLOCKS_PER_SEC; // ms
        if (vector->srcs & 0x000400) // ADC6
                vp_data_set[i++] = ADC_IN(6);
        if (vector->srcs & 0x000800) // ADC7
                vp_data_set[i++] = ADC_IN(7);
        if (vector->srcs & 0x000008) // LockIn0 [LockIn0 = LockIn channel after low pass]
                vp_data_set[i++] = 0x0008;
        if (vector->srcs & 0x001000) // Signal swappable
                vp_data_set[i++] = vp_index_all++; // 0x1000;
        if (vector->srcs & 0x002000) // Signal swappable
                vp_data_set[i++] = 0x2000;
        if (vector->srcs & 0x004000) // Signal swappable
                vp_data_set[i++] = 0x4000;
        if (vector->srcs & 0x008000) // Signal swappable
                vp_data_set[i++] = 0x8000;
        if (vector->srcs & 0x000004) // Signal swappable
                vp_data_set[i++] = pulse_counter;

        vp_num_data_sets=i;
}

void SPM_emulator::vp_buffer_section_end_data_srcs ()
{
}

// mini probe program flow interpreter kernel
#define DSP_PROBE_VECTOR_PROGRAM
void SPM_emulator::vp_next_section (){
	//GPIO_subsample = 0;

        if (! vector){ // initialize ?
                vector = &vector_program[0];
                section_count = 0; // init PVC
                g_message ("DSP:  SPM_emulator::vp_next_section INIT");
        }
        else{
                g_message ("DSP:  SPM_emulator::vp_next_section %d [%d]", section_count, vector->i);
                if (!vector->ptr_final){ // end Vector program?
                        vp_stop ();
                        return;
                }
		vp_buffer_section_end_data_srcs (); // store end of section data into matrix buffer -- used for area scan data transfer

#ifdef DSP_PROBE_VECTOR_PROGRAM // full program control capability
                if (vector->i > 0){ // do next loop?
                        --vector->i; // decrement loop counter
                        if (vector->ptr_next){ // loop or branch to next
                                section_count += vector->ptr_next; // adjust PVC (section "count" -- better section-index)
                                vector += vector->ptr_next; // next vector to loop
                        }else{
                                vp_stop ();
                        }
                }
                else{
			vector->i = vector->repetitions; // loop done, reload loop counter for next time

                        ++vector; // increment programm (vector) counter -- just increment!

#if 0
			if (vector->options & VP_LIMITER && (vector->options & (VP_LIMITER_MODE)) != 0 && lix > 0){
				lix = 0;
                                vector += vector->ptr_final; // go to limiter abort vector
			} else {
//				vector += vector->ptr_final; // next vector -- possible...
				++vector; // increment programm (vector) counter -- just increment!
			}
#endif			
			++section_count;
                }
#else // simple
                ++vector; // increment programm (vector) counter
                ++section_count;
#endif
        }
        iix = vector->dnx; // do dnx steps to take data until next point!
        ix = vector->n; // load total steps per section = # vec to add
        vp_point_us = (useconds_t)(iix * 1e6/frq_ref);
}

// manage conditional vector tracking mode -- atom/feature tracking
void SPM_emulator::vp_next_track_vector(){
#if 0
        int value;
        if (vector->options & VP_TRACK_REF){ // take reference?
                PRB_Trk_state = 0;
                PRB_Trk_ie    = 0;
                PRB_Trk_i     = 999;
                PRB_Trk_ref   = *tracker_input;
        } else if ((vector->options & VP_TRACK_FIN) || (PRB_Trk_i <= PRB_Trk_ie )){ // follow to max/min
                if (PRB_Trk_i == 999){ // done with mov to fin now
                        ++PRB_Trk_state;
                        if (PRB_Trk_ie > 0){ // start move to condition match pos
                                PRB_Trk_i = 0;
                                vector -= PRB_Trk_state; // skip back to v1 of TRACK vectors
                                section_count -= PRB_Trk_state; // skip back to v1 of TRACK vectors
                        } else { // done, no move, keep watching!
//                              PRB_Trk_i     = 999; // it's still this!
                                PRB_Trk_state = -1;
                                PRB_Trk_ref   = 0L;
                                clear_probe_data_srcs ();
                                probe_append_header_and_positionvector ();
                        }
                } else if (PRB_Trk_i == PRB_Trk_ie){ // done!
                        vector += PRB_Trk_state - PRB_Trk_ie; // skip forward to end of track sequence
                        section_count += PRB_Trk_state - PRB_Trk_ie; // skip forward to end of track sequence
                        PRB_Trk_i     = 999;
                        PRB_Trk_state = -1;
                        PRB_Trk_ref   = 0L;
                        clear_probe_data_srcs ();
                        probe_append_header_and_positionvector ();
                }
                ++PRB_Trk_i;
        } else if (vector->options & (VP_TRACK_UP | VP_TRACK_DN)){
                ++PRB_Trk_state;
                value = *tracker_input;
                if (vector->options & VP_TRACK_UP){
                        if (value > PRB_Trk_ref){
                                PRB_Trk_ref = value;
                                PRB_Trk_ie  = PRB_Trk_state;
                        }
                } else { // inplies VP_TRACK_DN !!
                        if (value < PRB_Trk_ref){
                                PRB_Trk_ref = value;
                                PRB_Trk_ie  = PRB_Trk_state;
                        }
                }
        }
        ++vector; // increment programm (vector) counter to go to next TRACK check position
        ++section_count; // next
        iix = vector->dnx; // do dnx steps to take data until next point!
        ix = vector->n; // load total steps per section = # vec to add
#endif
}

// trigger condition evaluation
int SPM_emulator::vp_wait_for_trigger (){
#if 0
	if (vector->options == (VP_TRIGGER_P | VP_TRIGGER_N)) // logic
		return ((*trigger_input) & ((vector->options & VP_GPIO_MSK) >> 16)) ? 0:1; // usually tigger_input would point to gpio_data for this GPIO/logic trigger
	else if (vector->options & VP_TRIGGER_P){ // positive
		return (*trigger_input > 0) ? 0:1;
	} else { // VP_TRIGGER_N -> negative 
		return (*trigger_input < 0) ? 0:1;
	}
#endif
}

// prepare for next section or tracking vector -- called from idle processing regime!
void SPM_emulator::vp_process_next_section (){
        // Special VP Track Mode Vector Set?
        if (vector->options & (VP_TRACK_UP | VP_TRACK_DN | VP_TRACK_REF | VP_TRACK_FIN)){
                vp_next_track_vector ();
                
        } else { // run regular VP program mode
                vp_next_section ();
                //                clear_probe_data_srcs ();
                vp_append_header_and_positionvector ();
        }
}

void SPM_emulator::GPIO_check(){
#if 0
        if (!GPIO_subsample--){
                GPIO_subsample = CR_generic_io.gpio_subsample;
                WR_GPIO (GPIO_Data_0, &CR_generic_io.gpio_data_in, 0);
                gpio_data = CR_generic_io.gpio_data_in; // mirror data in 32bit signal
        }
#endif
}

void SPM_emulator::vp_signal_limiter_test(){	
#if 0
        if (lix > 0) { ++lix;  goto run_one_time_step_1; }
        if (vector->options & VP_LIMITER_UP)
                if (*limiter_input > *limiter_updn[0]){
                        ++lix;
                        goto run_one_time_step_1;
                }
        if (vector->options & VP_LIMITER_DN)
                if (*limiter_input < *limiter_updn[1]){
                        ++lix;
                        goto run_one_time_step_1;
                }
        goto limiter_continue;
 run_one_time_step_1:
        if ((vector->options & VP_LIMITER_MODE) != 0) {
                lix = 1; // cancel any "undo" mode, use final vector in next section
                vector->i = 0; // exit any loop
        }
 limiter_continue:
        ;
#endif
}

// run a probe section step:
// decrement and check counter for zero, then initiate next section (...add header info)
// decrement skip data counter, check for zero then
//  -acquire data of all srcs, store away
//  -update current count, restore skip data counter

void SPM_emulator::vp_run (){
        static clock_t last_clock=vp_clock_start;
        // next time step in GVP
        // ** run_one_time_step ();
        // ** add_vector ();

        if (!vector) return;

        //g_print ("%d %d of [%d %d]\n", ix, iix, vector->n, vector->dnx);
        
        // next time step in GVP
        if (ix){ // idle task is working on completion of data management, wait!
#if 0
                // read GPIO if requested (CPU time costy!!!)
                if (vector->options & VP_GPIO_READ)
                        GPIO_check();

                // unsatisfied trigger condition will also HOLD VP as of now!
                if (vector->options & (VP_TRIGGER_P|VP_TRIGGER_N))
                        if (vp_wait_for_trigger ())
                                return;
#endif

//#define RTE_STEPS
#ifdef RTE_STEPS
                vp_integrate_data_srcs (); // leave to to hardware in future!
#else
                double us = ((double)clock () - (double)last_clock)*1e6/CLOCKS_PER_SEC;
                if (vp_point_us > us)
                        usleep ((clock_t)(vp_point_us-us));
                for (;--iix;){ // in DSP or FPGA do this in steps
                        vp_add_vector (); // iix times
                        ++vp_time;
                }
                last_clock = clock();
                --vp_time;
#endif
                if (! iix-- || lix){
                        if (ix--)
                                vp_store_data_srcs ();
                        // Limiter active?
                        if (vector->options & VP_LIMITER){
                                vp_signal_limiter_test();
                        } else if (lix > 0) --lix;
		 
                        iix = vector->dnx; // load steps inbetween "data aq" points
#ifdef RTE_STEPS
                        vp_clear_data_srcs (); // along with integrate -- move to hardware!
#endif
                }

#ifdef RTE_STEPS
                vp_add_vector ();
#endif
        }

        // increment probe time
	++vp_time; 
}

int SPM_emulator::vp_exec_callback(){
        if (vector){
                vp_run ();
                // --- this may run out of hard real time
                if(!ix)
                        vp_process_next_section ();
                // --------------------------------------
        }
        return ix;
}

void SPM_emulator::thread_run_loop(){
        g_message ("SPM_emulator::thread_run_loop()");
        while (vector){
                vp_run ();
                // --- this may run out of hard real time
                if(!ix)
                        vp_process_next_section ();
                // --------------------------------------
        }
        dsp_thread = NULL;
}

gpointer DSP_Thread (void *data){
        SPM_emulator *spm_emu = (SPM_emulator*)data;
        
       	spm_emu->vp_init ();
        spm_emu->thread_run_loop();
        
        return NULL;
}

void SPM_emulator::execute_vector_program(){
#ifdef __VP_TEST__ // exec calls are done by "ReadProbeData" for demonstration only this can be run independently w/o managed data movement
        if (!dsp_thread)
                dsp_thread = g_thread_new ("DSPThread", DSP_Thread, this);
        else
                g_message ("DSP is BUSY");
#endif
}
