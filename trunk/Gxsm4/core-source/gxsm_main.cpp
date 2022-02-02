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
// grep -rl gtk_object_ . | xargs sed -i s/gtk_object_/g_object_/g

// #define GXSM_GLOBAL_MEMCHECK

#ifdef GXSM_GLOBAL_MEMCHECK
#include <mcheck.h>
#endif

#include <new>
#include <cstring>

#include <locale.h>
#include <libintl.h>

#include <config.h>

#include <gtk/gtk.h>

#include "gxsm_app.h"
#include "gxsm_window.h"
#include "action_id.h"
#include "surface.h"

#include "glbvars.h"

/* True if parsing determined that all the work is already done.  */
int just_exit = 0;

static const GOptionEntry gxsm_options[] =
{
	/* Version */
	{ "version", 'V', G_OPTION_FLAG_NONE, G_OPTION_ARG_NONE, NULL, N_("Show the application's version"), NULL },
        
	{ "hardware-card", 'h', 0, G_OPTION_ARG_STRING, &xsmres.HardwareTypeCmd,
          N_("Hardware Card: no | ... (depends on available HwI plugins)"), NULL
        },

	{ "Hardware-DSPDev", 'd', G_OPTION_FLAG_NONE, G_OPTION_ARG_STRING, &xsmres.DSPDevCmd,
	  N_("Hardware DSP Device Path: /dev/sranger0 | ... (depends on module type and index if multiple DSPs)"), NULL
        },

	{ "User-Unit", 'u', G_OPTION_FLAG_NONE, G_OPTION_ARG_STRING, &xsmres.UnitCmd,
	  N_("XYZ Unit: AA | nm | um | mm | BZ | sec | V | 1 "), NULL
        },

	{ "logging-level", 'L', G_OPTION_FLAG_NONE, G_OPTION_ARG_INT, &logging_level,
          N_("Set Gxsm logging/monitor level. omit all loggings: 0, minimal logging: 1, default logging: 2, verbose logging: 3, ..."), NULL
        },

	{ "load-files-as-movie", 'm', G_OPTION_FLAG_NONE, G_OPTION_ARG_NONE, &load_files_as_movie,
          N_("load file from command in one channel as movie"), NULL
        },

	{ "developer", 'y', G_OPTION_FLAG_HIDDEN, G_OPTION_ARG_INT, &developer_option,
          N_("GXSM developer option/modes. Hidden from help. May be critical. Warning!"), NULL
        },

	{ "disable-plugins", 'd', G_OPTION_FLAG_NONE, G_OPTION_ARG_NONE, &xsmres.disableplugins,
          N_("Disable default plugin loading on startup"), NULL
        },
        
	{ "disable-geometry-management", 'g', G_OPTION_FLAG_NONE, G_OPTION_ARG_INT, &geometry_management_off,
          N_("Disable Gxsm Window Geometry load/restore on startup"), NULL
        },
        
	{ "force-configure", 0, G_OPTION_FLAG_NONE, G_OPTION_ARG_NONE, &xsmres.force_config,
          N_("Force to reconfigure Gxsm on startup"), NULL
        },

	{ "force-rebuild-configuration-defaults", 0, G_OPTION_FLAG_NONE, G_OPTION_ARG_NONE, &force_gxsm_defaults,
          N_("Forces to restore all GXSM values to build in defaults at startup"), NULL
        },

	{ "write-gxsm-preferences-gschema", 0, G_OPTION_FLAG_NONE, G_OPTION_ARG_NONE, &generate_preferences_gschema,
          N_("Generate Gxsm preferences gschema file on startup with build in defaults and exit"), NULL
        },

	{ "write-gxsm-gl-preferences-gschema", 0, G_OPTION_FLAG_NONE, G_OPTION_ARG_NONE, &generate_gl_preferences_gschema,
          N_("Generate Gxsm GL preferences gschema file on startup with build in defaults and exit"), NULL
        },

	{ "write-gxsm-pcs-gschema", 0, G_OPTION_FLAG_NONE, G_OPTION_ARG_NONE, &generate_pcs_gschema,
          N_("Generate Gxsm pcs gschema file on startup with build in defaults while execution"), NULL
        },

	{ "write-gxsm-pcs-adj-gschema", 0, G_OPTION_FLAG_NONE, G_OPTION_ARG_NONE, &generate_pcs_adj_gschema,
          N_("Generate Gxsm pcs adjustements gschema file on startup with build in defaults while execution"), NULL
        },

	{ "debug-level", 'D', G_OPTION_FLAG_NONE, G_OPTION_ARG_INT, &debug_level,
          N_("Set Gxsm debug level. 0: no debug output on console, 1: normal, 2: more verbose, ...5 increased verbosity"), "DN"
        },

	{ "pi-debug-level", 'P', G_OPTION_FLAG_NONE, G_OPTION_ARG_INT, &pi_debug_level,
          N_("Set Gxsm Plug-In debug level. 0: no debug output on console, 1: normal, 2: more verbose, ...5 increased verbosity"), "PDN"
        },

	/* New instance */
	{ "new-instance", 's', G_OPTION_FLAG_NONE, G_OPTION_ARG_NONE, &gxsm_new_instance,
          N_("Start a new instance of gxsm4 -- not yet functional, use different user account via ssh -X... for now."), NULL
	},

	/* Build self test python script */
	{ "build-self-test-script", 'T', G_OPTION_FLAG_NONE, G_OPTION_ARG_NONE, &gxsm_build_self_test_script,
          N_("Build gxsm4 self test pyremote script."), NULL
	},

	{ NULL }
};


#define GXSM_STARTUP_VERBOSE
#ifdef GXSM_STARTUP_VERBOSE
# define GXSM_STARTUP_MESSAGE_VERBOSE00(ARGS...) g_message (ARGS) 
#else
# define GXSM_STARTUP_MESSAGE_VERBOSE00(ARGS...) ;
#endif

# define GXSM_STARTUP_MESSAGE_VERBOSE(ARGS...) if (debug_level>0) g_message (ARGS) 

int main (int argc, char **argv)
{
        GError *error = NULL;

        //GXSM_STARTUP_MESSAGE_VERBOSE00 ("GXSM4 main: argc=%d", argc);

#ifdef GXSM_GLOBAL_MEMCHECK
        mtrace(); /* Starts the recording of memory allocations and releases */
#endif

        pcs_set_current_gschema_group ("core-init");

        //GXSM_STARTUP_MESSAGE_VERBOSE00 ("GXSM4 g_option_context_new for command line option parsing");

        GOptionContext *context = g_option_context_new ("List of loadable file(s) .nc, ...");
        g_option_context_add_main_entries (context, gxsm_options, GETTEXT_PACKAGE);
        //FIX-ME-GTK4 ok to ignore?
        //g_option_context_add_group (context, gtk_get_option_group (TRUE));

        //GXSM_STARTUP_MESSAGE_VERBOSE00 ("GXSM4 attempting g_option_context_parse on arguments.");

        if (!g_option_context_parse (context, &argc, &argv, &error)){
                GXSM_STARTUP_MESSAGE_VERBOSE00 ("GXSM4 option parse failed.");
                g_error ("GXSM4 command line option parsing failed: %s", error->message);
                exit (1);
        } else {
                GXSM_STARTUP_MESSAGE_VERBOSE ("GXSM4 option parse RESULTS:");
                GXSM_STARTUP_MESSAGE_VERBOSE ("GXSM4 commandline option parsing results:");
                GXSM_STARTUP_MESSAGE_VERBOSE ("=> xsmres.HardwareTypeCmd = %s", xsmres.HardwareTypeCmd);
                GXSM_STARTUP_MESSAGE_VERBOSE ("=> xsmres.DSPDevCmd  .... = %s", xsmres.DSPDevCmd);
                GXSM_STARTUP_MESSAGE_VERBOSE ("=> xsmres.UnitCmd ....... = %s", xsmres.UnitCmd);
                GXSM_STARTUP_MESSAGE_VERBOSE ("=> xsmres.force_config .. = %d", xsmres.force_config);
                GXSM_STARTUP_MESSAGE_VERBOSE ("=> force gxsm defaults .. = %d", force_gxsm_defaults);
                GXSM_STARTUP_MESSAGE_VERBOSE ("=> debug_level .......... = %d", debug_level);
                GXSM_STARTUP_MESSAGE_VERBOSE ("=> pi_debug_level ....... = %d", pi_debug_level);
                GXSM_STARTUP_MESSAGE_VERBOSE ("=> logging_level ........ = %d", logging_level);
                XSM_DEBUG_GP(DBG_L1, "XSM-DBG L1 TEST => debug_level ..... = %d\n", debug_level);
                PI_DEBUG_GP (DBG_L1, " PI-DBG L1 TEST => pi_debug_level .. = %d\n", pi_debug_level);
        }

        XSM_DEBUG(DBG_L2, "gxsm4_main g_application_run =========================================" );
        GXSM_STARTUP_MESSAGE_VERBOSE ("GXSM4: starting application module -- arguments left argc=%d", argc);
        
        int ret = g_application_run (G_APPLICATION (gxsm4_app_new ()), argc, argv);

#ifdef GXSM_GLOBAL_MEMCHECK
       	muntrace(); /* End the recording of memory allocations and releases */
#endif

        GXSM_STARTUP_MESSAGE_VERBOSE ("GXSM4 main exit with ret=%d", ret);
        return ret;
}
