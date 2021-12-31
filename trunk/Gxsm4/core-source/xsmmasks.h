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

#ifndef __XSMMASKS_H
#define __XSMMASKS_H

#define SET_FLAG(X, F) X = (((X)&(~(F))) | (F))
#define CLR_FLAG(X, F) X = ((X)&(~(F)))

#define VIEW_ZOOM     0x01
#define VIEW_Z400     0x02
#define VIEW_Z600     0x04
#define VIEW_COLOR    0x08
#define VIEW_PALETTE  0x10
#define VIEW_INFO     0x20
#define VIEW_TICKS    0x40

#define SCAN_V_DIRECT       0x0001
#define SCAN_V_QUICK        0x0002
#define SCAN_V_HORIZONTAL   0x0004
#define SCAN_V_PERIODIC     0x0008
#define SCAN_V_LOG          0x0010
#define SCAN_V_DIFFERENTIAL 0x0020
#define SCAN_V_PLANESUB     0x0040
#define SCAN_V_HILITDIRECT  0x0080
#define SCAN_V_MASK         0x0FFF
#define SCAN_V_SCALE_HILO   0x1000 // default
#define SCAN_V_SCALE_SMART  0x2000 // tolerant mode, use histogram


#define MODE_LINE      0x01
#define MODE_AUTOSAVE  0x02
#define MODE_SETRANGE  0x04
#define MODE_SETPOINTS 0x08
#define MODE_SETSTEPS  0x10
#define MODE_BZUNIT    0x20
#define MODE_VOLTUNIT  0x40
#define MODE_1DIMSCAN  0x80
#define MODE_2DIMSCAN  0x100
#define MODE_ENERGY_EV 0x200
#define MODE_ENERGY_S  0x400

#define PRINT_FILE     0x01
#define PRINT_PRINTER  0x02
#define PRINT_FRAME    0x04
#define PRINT_SCALE    0x08
#define PRINT_REGIONS  0x10

#define FLG_MESS      0x0001
#define DRAW_PRINT    0x0010


#endif


