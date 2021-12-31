/* Gnome gxsm - Gnome X Scanning Microscopy
 * universal STM/AFM/SARLS/SPALEED/... controlling and
 * data analysis software
 *
 * 
 * Gxsm Plugin Name: autoalign.C
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

/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 8 c-style: "K&R" -*- */

#ifndef __AUTOALIGN_VECTOR_TYPES_H
#define __AUTOALIGN_VECTOR_TYPES_H

#include <iostream>
#include <vector>
#include <stack>

using namespace std;

typedef vector <double> Vector_d;
typedef vector <vector <double> > Matrix_d;

typedef vector <int> Vector_i;
typedef vector <vector <int> > Matrix_i;

typedef vector <gboolean> Vector_b;
typedef vector <vector <gboolean> > Matrix_b;



typedef stack <gpointer> stack_pyr;

inline void vector_copy (Vector_d& s, int s_pos, Vector_d& d, int d_pos, int n) {
	for (int i=0; i<n; ++i)
		d[d_pos+i] = s[s_pos+i];
}

#endif
