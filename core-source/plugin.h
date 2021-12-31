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

#ifndef __PLUGIN_H
#define __PLUGIN_H

#include <config.h>
#include <glib.h>
#include <gmodule.h>

#include "gxsm_app.h"
#include "scan.h"

class App;

/*
 * Gxsm PlugIn Infostruct:
 *
 * PlugIn Function Call: 
 *   (void*) get_plugin_info( void )
 * should return a valid Pointer to "struct GxsmPlugin" as defined here:
 *
 */

typedef void (*PIMenuCallback) (GSimpleAction *simple, GVariant *parameter, gpointer user_data);
#define PI_MENU_CALLBACK(f)    ((PIMenuCallback) (f))
	
typedef struct
{
  GModule *module;           /* Filled in by Gxsm */
  char *filename;         /* Filled in by Gxsm */
  int  Gxsm_session;      /* The session ID for attaching to the control socket */
  App *app;               /* Calling Application Object */
  const gchar *name;             /* unique Plugin name */
  const gchar *category;         /* Plugin's Category - used to autodecide on Pluginloading or ignoring */
  gchar *description;      /* The description that is shown in the preferences box */
  const gchar *authors;          /* Plugins author */
  const gchar *menupath;         /* The plugins menuposition to append to */
  const gchar *menuentry;        /* The plugins menuentry */
  const gchar *help;             /* The plugins help text */
  const gchar *info;             /* Additional info about Plugin */
  char *errormsg;         /* Plugin Status Message */
  char *status;           /* Plugin Status, managed by Gxsm */
  void (*init) (void);    /* Called when the plugin is enabled */
  void (*query) (void);   /* Called after init and app is set. */
  void (*about) (void);   /* Show the about box */
  void (*configure) (void); /* User Configuration */
  void (*run) (GtkWidget *, void *); /* if run != NULL this handler is installed at menupath as default, else for special Gxsm-Plugins is searched! */
  PIMenuCallback menu_callback1;    /* direct menu entry callback1 or NULL  ["entry1[, ..]"]  */
  PIMenuCallback menu_callback2;    /* direct menu entry callback2 or NULL  ["entr1, entry2"] */
  void (*cleanup) (void); /* Called when the plugin is disabled or when Gxsm exits */
}
GxsmPlugin;

/*
 * Arbitrary Expansion of Menu Entry Callback hookups
 */

/* returned by
 * GxsmPluginMenuCallbackList* get_gxsm_plugin_menu_callback_list ( void ) 
 */
typedef struct
{
	gint n; // number of callbacks to manage, still list must be NULL terminated!
	PIMenuCallback *menu_callback_list;
}
GxsmPluginMenuCallbackList;


/*
 * Gxsm special PlugIns...
 */

/* Math Plugins */

/* returned by
 * (void*) get_gxsm_math_one_src_plugin_info( void ) 
 */
typedef struct
{
  gboolean (*run) (Scan *Src, Scan *Dest);
}
GxsmMathOneSrcPlugin;

/* returned by
 * (void*) get_gxsm_math_two_src_plugin_info( void ) 
 * or
 * (void*) get_gxsm_math_two_src_no_same_size_check_plugin_info( void ) 
 */
typedef struct
{
  gboolean (*run) (Scan *Src1, Scan *Src2, Scan *Dest);
}
GxsmMathTwoSrcPlugin;

/* returned by
 * (void*) get_gxsm_math_one_src_no_dest_plugin_info( void ) 
 */
typedef struct
{
  gboolean (*run) (Scan *Src);
}
GxsmMathOneSrcNoDestPlugin;

/* returned by
 * (void*) get_gxsm_math_no_src_one_dest_plugin_info( void ) 
 */
typedef struct
{
  gboolean (*run) (Scan *Dest);
}
GxsmMathNoSrcOneDestPlugin;


/* Instrument Control Plugins */

/* GXSM Hardware Interface Plugins */

/*
 * A pointer to allocated subclass of class XSM_hardware is returned by
 * (void*) get_gxsm_hwi_hardware_class( void *data )
 * It is allocated at PI-init
 * and
 * deleted at PI-cleanup
 */

#endif
