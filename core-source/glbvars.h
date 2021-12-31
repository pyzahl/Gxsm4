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

#ifndef __GLBVARS_H
#define __GLBVARS_H

#ifndef __GNOME_RES_H
#include "gnome-res.h"
#endif

#ifndef __XSMHARD_H
#include "xsmhard.h"
#endif

#ifndef __SURFACE_H
#include "surface.h"
#endif

#ifndef __GXSM_APP_H
#include "gxsm_app.h"
#endif

#ifndef __XSMDEBUG_H
#include "xsmdebug.h"
#endif

#ifndef __XSM_H
#include "xsm.h"
#endif

#ifndef __VERSION_H
#include "version.h"
#endif

#ifndef WORDS_BIGENDIAN
# define WORDS_BIGENDIAN 0
#endif

/*
 * Resources
 */

extern XSMRESOURCES xsmres;
extern GnomeResEntryInfoType xsm_res_def[];
extern UnitsTable XsmUnitsTable[];

extern char *cvsmainrev, *cvsmaindate;

#endif

/* END */




