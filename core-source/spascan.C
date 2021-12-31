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

/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 8 c-style: "K&R" -*- */

#include <locale.h>
#include <libintl.h>


#include "surface.h"
#include "glbvars.h"

#define L_NULL  0
#define L_START 1
#define L_MOVE  2
#define L_END   3

#define QSIZE   5  // Size of Line-Handles (Squares)
#define QSIZE1  QSIZE // Anfang
#define QSIZE2  (QSIZE-2) // Ende

SpaScan::SpaScan(int vtype, int vflg, int ChNo, SCAN_DATA *vd):Scan(vtype, vflg, ChNo, vd, ZD_LONG){
}

SpaScan::~SpaScan(){
}

// END
