/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 8 c-style: "K&R" -*- */

/* Gnome gxsm - Gnome X Scanning Microscopy
 * universal STM/AFM/SARLS/SPALEED/... controlling and
 * data analysis software
 * 
 * Gxsm Plugin Name:vectorutil.h
 * ========================================
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


#include <glib.h>
#include <math.h>
#include "string.h"


inline void set_vec (double u[3], double c){
	for (int i=0; i<3; ++i)
		u[i] = c;
}

inline void copy_vec (double u[3], const double *v){
#if 1
        memcpy(u, v, 3*sizeof(double));
#else
	for (int i=0; i<3; ++i)
		u[i] = v[i];
#endif
}

inline void copy_vec4 (double u[4], const double *v){
#if 1
        memcpy(u, v, 4*sizeof(double));
#else
	for (int i=0; i<4; ++i)
		u[i] = v[i];
#endif
}

inline void max_vec_elements (double u[3], const double *a, const double *b){
	for (int i=0; i<3; ++i)
		u[i] = a[i] > b[i] ? a[i] : b[i];
}

inline void min_vec_elements (double u[3], const double *a, const double *b){
	for (int i=0; i<3; ++i)
		u[i] = a[i] < b[i] ? a[i] : b[i];
}

inline void middle_vec_elements (double u[3], const double *a, const double *b){
	for (int i=0; i<3; ++i)
		u[i] =  0.5 * (a[i] + b[i]);
}

inline double dot_prod (const double v1[3], const double v2[3]){
	return v1[0]*v2[0] + v1[1]*v2[1] + v1[2]*v2[2];
}

inline void add_to_vec (double u[3], const double v[3]){
	for (int i=0; i<3; ++i)
		u[i] += v[i];
}

inline void sub_from_vec (double u[3], const double v[3]){
	for (int i=0; i<3; ++i)
		u[i] -= v[i];
}

inline void mul_vec_vec (double u[3], double v[3]){
	for (int i=0; i<3; ++i)
		u[i] *= v[i];
}

inline void make_mat_zeros (double m[3][3]){
	for (int i=0; i<3; ++i)
                for (int j=0; j<3; ++j)
                        m[i][j] = 0.;
}
 
inline void make_mat_kd (double m[3][3]){
	for (int i=0; i<3; ++i)
                for (int j=0; j<3; ++j)
                        if (i == j)
                                m[i][j] = 1.;
                        else
                                m[i][j] = 0.;
}

inline void make_mat_rot_xy_tr (double m[3][3], double phi, double tr[3]){
        make_mat_kd (m);
        double s=sin (phi*M_PI/180.);
        double c=cos (phi*M_PI/180.);
        m[0][0] = c;
        m[0][1] = -s;
        m[1][0] = s;
        m[1][1] = c;
	for (int i=0; i<2; ++i)
                m[i][2] = tr[i];
}
 
inline void mul_mat_vec (double u[3], double m[3][3], double v[3]){
	for (int i=0; i<3; ++i){
                u[i] = 0.;
                for (int j=0; j<3; ++j)
                        u[i] += m[i][j]*v[j];
        }
}

 inline void mul_mattr_vec (double u[3], double m[3][3], double v[3]){
	for (int i=0; i<3; ++i){
                u[i] = 0.;
                for (int j=0; j<3; ++j)
                        u[i] += m[j][i]*v[j];
        }
}

// 2d rot/affine + transformation, copy 3rd,4th dim
inline void mul_mat_vec_tc4 (double u[4], double m[3][3], double v[4]){
        double tmp[3] = { v[0], v[1], 1. };
	for (int i=0; i<3; ++i){
                u[i] = 0.;
                for (int j=0; j<3; ++j)
                        u[i] += m[i][j]*tmp[j];
        }
        u[2] += v[2]; // translate
        u[3] = v[3]; // copy only
}

inline void mul_vec_scalar (double u[3], const double f){
	for (int i=0; i<3; ++i)
		u[i] *= f;
}

inline void neg_vec (double u[3]){
	for (int i=0; i<3; ++i)
		u[i] = -u[i];
}

inline double norm_vec (const double u[3]){
        return (double)sqrt(u[0]*u[0] + u[1]*u[1] + u[2]*u[2]);
}
        
inline void norm_cross_prod (const double v1[3], const double v2[3], double n[3]){
	double d;
	n[0] = v1[1]*v2[2] - v1[2]*v2[1];
	n[1] = v1[2]*v2[0] - v1[0]*v2[2];
	n[2] = v1[0]*v2[1] - v1[1]*v2[0];
	d = sqrt(n[0]*n[0] + n[1]*n[1] + n[2]*n[2]);
	n[0] /= d;
	n[1] /= d;
	n[2] /= d;
}

inline void norm3pkte (const double p1[3], const double p2[3], const double p3[3], double n[3]){
	double v1[3], v2[3];

	v1[0] = p2[0] - p1[0];
	v1[1] = p2[1] - p1[1];
	v1[2] = p2[2] - p1[2];

	v2[0] = p3[0] - p1[0];
	v2[1] = p3[1] - p1[1];
	v2[2] = p3[2] - p1[2];

	norm_cross_prod (v1, v2, n);
}

#ifdef USR_STDCOUT
inline void print_vec (const gchar *name, const double v[3]){
        std::cout << name << " = [ " << v[0] << ", " << v[1] << ", " << v[2] << "]" << std::endl;
}

inline void print_vec4 (const gchar *name, const double v[4]){
        std::cout << name << " = [ " << v[0] << ", " << v[1] << ", " << v[2] << ", " << v[3] << "]" << std::endl;
}
#endif

inline void g_print_vec (const gchar *name, const double v[3]){
        g_message ("%s = (%g, %g, %g)", name, v[0], v[1], v[2]);
}

inline void g_print_vec4 (const gchar *name, const double v[3]){
        g_message ("%s = (%g, %g, %g, %g)", name, v[0], v[1], v[2], v[3]);
}

inline void g_print_mat (const gchar *name, const double m[3][3]){
        g_message ("%s   /%6g, %6g, %6g\\", name, m[0][0], m[0][1], m[0][2]);
        g_message ("%s = |%6g, %6g, %6g|",  name, m[1][0], m[1][1], m[1][2]);
        g_message ("%s   \\%6g, %6g, %6g/", name, m[2][0], m[2][1], m[2][2]);
}

