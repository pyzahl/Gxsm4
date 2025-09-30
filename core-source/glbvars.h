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

/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 8 c-style: "K&R" -*- */

#ifndef __GLBVARS_H
#define __GLBVARS_H


#ifndef WORDS_BIGENDIAN
#define WORDS_BIGENDIAN 1   // if not defined for what ever reason, assume BIGENDIAN. Seams to be missing def. for ARM architecture.
#endif

/*
 * Resources
 */

#include "gnome-res.h"
#include "xsmtypes.h"
#include "gxsm_app.h"

extern "C++" {

        extern XSMRESOURCES xsmres; // in xsmtypes.h
        extern UnitsTable XsmUnitsTable[]; // in xsmtypes.h

        extern GnomeResEntryInfoType xsm_res_def[]; // in gnome-res.h
        extern App *gapp;

        extern int restarted;
        extern int debug_level;
        extern int logging_level;
        extern int developer_option;
        extern int pi_debug_level;
        extern int geometry_management_off;
        
        extern gboolean force_gxsm_defaults;
        extern gboolean load_files_as_movie;
        extern gboolean gxsm_new_instance;

        extern gboolean generate_preferences_gschema;
        extern gboolean generate_gl_preferences_gschema;
        extern gboolean generate_pcs_gschema;
        extern gboolean generate_pcs_adj_gschema;

        extern gboolean gxsm_build_self_test_script;

        // Gloabl Data Access
        extern gboolean main_get_generate_pcs_gschema ();
        extern gboolean main_get_generate_pcs_adj_gschema ();

        extern int main_get_debug_level ();
        extern int main_get_pi_debug_level ();

        extern App* main_get_gapp();
  
} // extern C++

#endif

/* END */




