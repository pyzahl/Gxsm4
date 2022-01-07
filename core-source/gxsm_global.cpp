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

#include "gxsm_app.h"
#include "gxsm_window.h"
#include "gxsm_resoucetable.h"
#include "action_id.h"
#include "surface.h"
#include "xsmtypes.h"

extern "C++" {


#ifdef GXSM_MONITOR_VMEMORY_USAGE
        gint global_ref_counter[32] = {
                0,0,0,0, 0,0,0,0,
                0,0,0,0, 0,0,0,0,
                0,0,0,0, 0,0,0,0,
                0,0,0,0, 0,0,0,0
        };
#endif
        
        XSMRESOURCES xsmres;

        // Alles wird auf dies Basiseinheit "1A" bezogen - nur User I/O in nicht A !!
        // defined in xsmtypes.h
        UnitsTable XsmUnitsTable[] = {
                // Id (used in preferences), Units Symbol, Units Symbol (ps-Version), scale factor, precision1, precision2
                { "AA", UTF8_ANGSTROEM,   "Ang",    1e0, ".1f", ".3f" }, // UFT-8 Ang did not work // PS: "\305"
                { "nm", "nm",  "nm",     10e0, ".1f", ".3f" },
                { "um", UTF8_MU"m",  "um",     10e3, ".1f", ".3f" }, // PS: "\265m"
                { "mm", "mm",  "mm",     10e6, ".1f", ".3f" },
                { "BZ", "%BZ", "BZ",     1e0, ".1f", ".2f" },
                { "sec","\"",  "\"",      1e0, ".1f", ".2f" },
                { "V",  "V",   "V",       1e0, ".2f", ".3f" },
                { "mV", "mV",  "mV",      1e-3, ".2f", ".3f" },
                { "V",  "*V", "V",      1e0, ".2f", ".3f" },
                { "*dV", "*dV","dV",     1e0, ".2f", ".3f" },
                { "*ddV", "*ddV","ddV",  1e0, ".2f", ".3f" },
                { "*V2", "V2", "V2",       1e0, ".2f", ".3f" },
                { "1",  " ",   " ",       1e0, ".3f", ".4f" },
                { "0",  " ",   " ",       1e0, ".3f", ".4f" },
                { "B",  "Bool",   "Bool", 1e0, ".3f", ".4f" },
                { "X",  "X",   "X",       1e0, ".3f", ".4f" },
                { "xV",  "xV",   "xV",    1e0, ".3f", ".4f" },
                { "deg", UTF8_DEGREE, "deg",       1e0, ".3f", ".4f" },
                { "Amp", "A",  "A",       1e9, "g", "g" },
                { "nA", "nA",  "nA",      1e0, ".2f", ".3f" },
                { "pA", "pA",  "pA",      1e-3, ".1f", ".2f" },
                { "nN", "nN",  "nN",      1e0, ".2f", ".3f" },
                { "Hz", "Hz",  "Hz",      1e0, ".2f", ".3f" },
                { "mHz", "mHz",  "mHz",   1e-3, ".2f", ".3f" },
                { "K",  "K",   "K",       1e0, ".2f", ".3f" },
                { "amu","amu", "amu",     1e0, ".1f", ".2f" },
                { "CPS","Cps", "Cps",     1e0, ".1f", ".2f" },
                { "CNT","CNT", "CNT",     1e0, ".1f", ".2f" },
                { "Int","Int", "Int",     1e0, ".1f", ".2f" },
                { "A/s","A/s", "A/s",     1e0, ".2f", ".3f" },
                { "s","s", "s",           1e0, ".2f", ".3f" },
                { "ms","ms", "ms",        1e0, ".2f", ".3f" },
                { NULL, NULL, NULL,       0e0, NULL, NULL }
        };
        
        App *gapp = NULL;

        int restarted = 0;
        int debug_level = 0;
        int logging_level = 2;
        int developer_option = 0;
        int pi_debug_level = 0;

        gboolean force_gxsm_defaults = false;
        gboolean load_files_as_movie = false;
        gboolean gxsm_new_instance = false;

        gboolean generate_preferences_gschema = false;
        gboolean generate_gl_preferences_gschema = false;
        gboolean generate_pcs_gschema = false;
        gboolean generate_pcs_adj_gschema = false;

        gboolean gxsm_build_self_test_script = false;

        // Gloabl Data Access
        gboolean main_get_generate_pcs_gschema () { return generate_pcs_gschema; }
        gboolean main_get_generate_pcs_adj_gschema () { return generate_pcs_adj_gschema; }

        int main_get_debug_level () { return debug_level; }
        int main_get_pi_debug_level () { return pi_debug_level; }

        App* main_get_gapp() { return gapp; }

}
