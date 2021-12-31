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

#ifndef __GXSM_MONITOR_VMEMORY_AND_REFCOUNTS_H
#define __GXSM_MONITOR_VMEMORY_AND_REFCOUNTS_H

#include <config.h>

#define GXSM_GRC_UNITOBJ      0
#define GXSM_GRC_SCANDATAOBJ  1
#define GXSM_GRC_SCANOBJ      2
#define GXSM_GRC_ZDATA        3
#define GXSM_GRC_LAYERINFO    4
#define GXSM_GRC_MEM2D        5
#define GXSM_GRC_MEM2D_SCO    6
#define GXSM_GRC_VOBJ         7
#define GXSM_GRC_CAIRO_ITEM   8
#define GXSM_GRC_PROFILEOBJ   9
#define GXSM_GRC_PROFILECTRL 10
#define GXSM_GRC_PRBHDR      11
#define GXSM_GRC_PRBVEC      12
#define GXSM_GRC_LAST        13

#ifdef GXSM_MONITOR_VMEMORY_USAGE
extern gint global_ref_counter[32];
extern const gchar *grc_name[];
#endif

#ifdef GXSM_MONITOR_VMEMORY_USAGE
#define GXSM_LOG_DATAOBJ_ACTION(ID, WHAT) gapp->monitorcontrol->LogEvent(grc_name[ID], WHAT, 4);
#define GXSM_LOG_ANY_ACTION(ID, WHAT) gapp->monitorcontrol->LogEvent(ID, WHAT, 4);
#define GXSM_REF_OBJECT(ID)   ++global_ref_counter[ID];
#define GXSM_UNREF_OBJECT(ID) --global_ref_counter[ID];
#else
#define GXSM_LOG_DATAOBJ_ACTION(ID, WHAT) ;
#define GXSM_LOG_ANY_ACTION(ID, WHAT) ;
#define GXSM_REF_OBJECT(ID) ;
#define GXSM_UNREF_OBJECT(ID) ;
#endif

#endif
