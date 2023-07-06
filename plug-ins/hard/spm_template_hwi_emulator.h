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

#ifndef __SPM_TEMPLATE_HWI_EMULATOR_H
#define __SPM_TEMPLATE_HWI_EMULATOR_H

#include <sys/ioctl.h>
#include "spm_template_hwi_structs.h"


// ========================================
// primitive surface landscape simulator
// ========================================

class feature{
public:
        feature(double rr=400.){
                ab = 3.5;
                xy[0] = ab*g_random_double_range (-rr, rr);
                xy[1] = ab*g_random_double_range (-rr, rr);
        };
        ~feature(){};

        virtual double get_z(double x, double y, int ch) { return 0.; };
        
        double xy[2];
        double ab;
};

class feature_step : public feature{
public:
        feature_step():feature(400.){
                phi = g_random_double_range (25., 75.);
                m = atan(phi/57.);
                dz = 3.4;
        };
        ~feature_step(){};

        virtual double get_z(double x, double y, int ch) {
                x -= xy[0];
                y -= xy[1];
                return m * x > y ? dz : 0.;
        };
private:
        double phi,m;
        double dz;
};

class feature_island : public feature{
public:
        feature_island(double rr=1000.):feature(rr){
                r2 = g_random_double_range (10*ab, 100*ab);
                r2 *= r2;
        };
        ~feature_island(){};

        virtual double get_z(double x, double y, int ch)  {
                x -= xy[0];
                y -= xy[1];
                return x*x+y*y < r2 ? ab : 0.;
        };
private:
        double r2;
};

class feature_blob : public feature_island{
public:
        feature_blob():feature_island(2000.){
                bz=50.;
                double rr=bz;
                for (int i=0; i<200; ++i){
                        xx[i] = ab*g_random_double_range (-rr, rr);
                        yy[i] = ab*g_random_double_range (-rr, rr);
                        r2[i] = g_random_double_range (10*ab, bz*ab);
                        r2[i] *= r2[i];
                }
        };
        ~feature_blob(){};

        virtual double get_z(double x, double y, int ch)  {
                x -= xy[0];
                y -= xy[1];
                double z=0.;
                for (int i=0; i<200; ++i){
                        z += (x-xx[i])*(x-xx[i])+(y-yy[i])*(y-yy[i]) < r2[i] ? ab : 0.;
                }
                return z;
        };
private:
        double xx[200], yy[200];
        double r2[200];
        double bz;
};

class feature_molecule : public feature{
public:
        feature_molecule(){
                w=10.; h=30.;
                phi = M_PI/6.*g_random_int_range (0, 4);
        };
        ~feature_molecule(){};

        double shape(double x, double y){
                double zx=cos(x/w*(M_PI/2));
                double zy=cos(y/h*(M_PI/2));
                return 6.5*(zx*zx*zy*zy);
        };
        
        virtual double get_z(double x, double y, int ch) {
                x -= xy[0];
                y -= xy[1];
                double xx =  x*cos(phi)+y*sin(phi);
                double yy = -x*sin(phi)+y*cos(phi);
                double rx=w*w;
                double ry=h*h;
                return xx*xx < rx && yy*yy < ry ? shape(xx,yy) : 0.;
        };
private:
        double phi;
        double w,h;
};

class feature_lattice : public feature{
public:
        feature_lattice(){
        };
        ~feature_lattice(){};

        // create dummy data 3.5Ang period
        virtual double get_z(double x, double y, int ch) {
                return 10.*ch + ab * sin(x/ab*2.*M_PI) * cos(y/ab*2.*M_PI);
        };
};



// ========================================
// minimal DSP Simulation Engine
// ========================================

#define MAX_PROGRAM_VECTORS 50

class SPM_emulator{
public:
        SPM_emulator(){
		x0 = y0 = 0.0;
		data_z_value = 0.;
		data_y_count = 0;
		data_y_index = 0;
		data_x_index = 0;

		sim_current  = 0.;
		sim_bias     = 0.;
		sim_z        = 0.;
		data_z_value  = 0.;

		frq_ref = 1000; // real DSP: more 75000 or 150000 Hz
	};
        ~SPM_emulator(){};
        
        double simulate_value (XSM_Hardware *xsmhwi, int xi, int yi, int ch);

	int read_program_vector(int i, PROBE_VECTOR_GENERIC *v){
		if (i >= MAX_PROGRAM_VECTORS || i < 0)
			return 0;
		memcpy (v, &vector_program[i], sizeof (PROBE_VECTOR_GENERIC));
		return -1;
	};
	int write_program_vector(int i, PROBE_VECTOR_GENERIC *v){
		if (i >= MAX_PROGRAM_VECTORS || i < 0)
			return 0;

		// copy/convert to DSP format
		vector_program[i].n = v->n;
		vector_program[i].dnx = v->dnx;
		vector_program[i].srcs = v->srcs;
		vector_program[i].options = v->options;
		vector_program[i].ptr_fb = v->ptr_fb;
		vector_program[i].repetitions = v->repetitions;
		vector_program[i].i = v->i;
		vector_program[i].j = v->j;
		vector_program[i].ptr_next = v->ptr_next;
		vector_program[i].ptr_final = v->ptr_final;
		vector_program[i].f_du = v->f_du;
		vector_program[i].f_dx = v->f_dx;
		vector_program[i].f_dy = v->f_dy;
		vector_program[i].f_dz = v->f_dz;
		vector_program[i].f_dx0 = v->f_dx0;
		vector_program[i].f_dy0 = v->f_dy0;
		vector_program[i].f_dphi = v->f_dphi;


		// check count ranges
		// NULL VECTOR, OK: END
		if (!(vector_program[i].n == 0 && vector_program[i].dnx == 0 && vector_program[i].ptr_next == 0 && vector_program[i].ptr_final == 0))
			if (vector_program[i].dnx < 0 || vector_program[i].dnx > 32767 || vector_program[i].n <= 0 || vector_program[i].n > 32767){
				/*
				gchar *msg = g_strdup_printf ("Probe Vector [pc%02d] not acceptable:\n"
							      "n   = %6d   [0..32767]\n"
							      "dnx = %6d   [0..32767]\n"
							      "Auto adjusting.\n"
							      "Hint: adjust slope/speed/points/time.",
							      index, vector_program[i].n, vector_program[i].dnx );
				main_get_gapp()->warning (msg);
				g_free (msg);
				*/
				if (vector_program[i].dnx < 0) vector_program[i].dnx = 0;
				if (vector_program[i].dnx > 32767) vector_program[i].dnx = 32767;
				if (vector_program[i].n <= 0) vector_program[i].n = 1;
				if (vector_program[i].n > 32767) vector_program[i].n = 32767;
			}

		// setup VPC essentials
		vector_program[i].i = vector_program[i].repetitions; // preload repetitions counter now! (if now already done)
		vector_program[i].j = 0; // not yet used -- XXXX

		return -1;
	};
	int abort_program(){
		return 0;
	};

        double x0,y0; // offset
        double sim_bias;
        double sim_current;
        double sim_z;
        double data_z_value;

        // scan engine
        int data_y_count;
        int data_y_index;
        int data_x_index;

	double frq_ref;
	
	PROBE_VECTOR_GENERIC vector_program[MAX_PROGRAM_VECTORS];

	
};


#endif
