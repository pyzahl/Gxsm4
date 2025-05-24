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

/*------------------------------------------------------------
 *
 *  File: xsm_limits.h
 *
 *------------------------------------------------------------
 */

/* Anzahl Data Source Channels */
#define MAX_SRCS_CHANNELS 20 // max # scan data source channels transferred at once -- may be more too choose from been muxed

/* Anzahl Channels */
#define MAX_CHANNELS 30

#define DUMMYMIN  -1e30
#define DUMMYMAX   1e30

/* XSM-MAIN */

/* Defaultwerte - falls kei ~/.xsmrc exisitiert */
#define DIGITALRANGE  32768 /* Signed (+/- 32768) */
#define ANALOGVOLTMAX 10.0  /* Signed (+/- 10V) */
#define DEFAULT_V0 1. /* V[0] */
#define DEFAULT_V1 2.
#define DEFAULT_V2 5.
#define DEFAULT_V3 10.
#define DEFAULT_V4 15.
#define VX_DEFAULT 1  /* Index !!! */
#define VY_DEFAULT 1  /* Index !!! */
#define VZ_DEFAULT 2  /* Index !!! */
#define X_PIEZO_SENSI 532. /* A/V */
#define Y_PIEZO_SENSI 532. /* A/V */
#define Z_PIEZO_SENSI 74.  /* A/V */

#define MINPOINTS    1
#define MAXDATALINES 32000
#define MAXVALUES    2000

/* Sonsitige Default Werte */
#define COLMAX 64




