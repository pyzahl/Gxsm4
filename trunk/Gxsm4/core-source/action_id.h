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

#pragma once

namespace xsm {
        typedef enum { replace, append_time, stitch_2d } open_mode;
};



/* Channel Actions */
#define    ID_CH_V 1
#define    ID_CH_M 101

typedef enum {
    ID_CH_V_NO,
    ID_CH_V_GREY,
    ID_CH_V_SURFACE,
    ID_CH_V_PROFILE,
    ID_CH_V_LAST 
} CHANNEL_VIEW_ACTIONS;

typedef enum {
    ID_CH_NULL,
    ID_CH_M_OFF,
    ID_CH_M_ACTIVE,
    ID_CH_M_ON,
    ID_CH_M_MATH,
    ID_CH_M_X,
    ID_CH_M_LAST 
} CHANNEL_MODE_ACTIONS;

typedef enum {
    ID_CH_D_P,
    ID_CH_D_M,
    ID_CH_D_2ND_P,
    ID_CH_D_2ND_M,
    ID_CH_D_LAST 
} CHANNEL_SDIR_ACTIONS;

/* Print Actions */
typedef enum {
    ID_PRINTM_FIRST,
    ID_PRINTM_AUTO,
    ID_PRINTM_QUICK,
    ID_PRINTM_DIRECT,
    ID_PRINTM_LAST
} PRINT_MODE;

typedef enum {
    ID_PRINTT_FIRST,
    ID_PRINTT_PLAIN,
    ID_PRINTT_TICKSOZ,
    ID_PRINTT_TICKS,
    ID_PRINTT_BAR,
    ID_PRINTT_CIRC,
    ID_PRINTT_LAST
} PRINT_TYPE;

typedef enum {
    ID_PRINTI_FIRST,
    ID_PRINTI_NONE,
    ID_PRINTI_SIZE,
    ID_PRINTI_MORE,
    ID_PRINTI_ALL,
    ID_PRINTI_LAST
} PRINT_INFO;

typedef enum {
    ID_PRINT_FIRST,
    ID_PRINT_TITLE,
    ID_PRINT_AUTO,
    ID_PRINT_DIRECT,
    ID_PRINT_QUICK,
    ID_PRINT_SCALEIT,
    ID_PRINT_FRAME,
    ID_PRINT_TYP,
    ID_PRINT_INFO,
    ID_PRINT_FONTSIZE,
    ID_PRINT_FIGWIDTH,
    ID_PRINT_FILE,
    ID_PRINT_FNAME,
    ID_PRINT_PRT,
    ID_PRINT_PRTCMD,
    ID_PRINT_OK,
    ID_PRINT_CANCEL,
    ID_PRINT_HELP,
    ID_PRINT_LAST
} PRINT_ACTIONS;



typedef enum {
    ID_GL_FIRST,
    ID_GL_nZP,
    ID_GL_Fog,
    ID_GL_Tex,
    ID_GL_Cull,
    ID_GL_Mesh,
    ID_GL_Smooth,
    ID_GL_Dither,
    ID_GL_Ticks,
    ID_GL_ColX,
    ID_GL_Prev,
    ID_GL_LAST
} GL_ACTIONS;


