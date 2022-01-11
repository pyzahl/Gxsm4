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


/* This file belongs to the pyremote-plugin. */

#ifndef PYREMOTE__H
#define PYREMOTE__H

#include <gtk/gtk.h>
#include <math.h>
#include "cairo_item.h"

#include "gxsm_app.h"
#include "gxsm_window.h"

struct remote_action_cb {
    const gchar  *cmd;
    void (*RemoteCb)(GtkWidget *widget , void* data);
    GtkWidget *widget;
    gpointer data;
};

// Plugin Prototypes
static void pyremote_init( void );
static void pyremote_about( void );
static void pyremote_configure( void );
static void pyremote_cleanup( void );
static void pyremote_run(GtkWidget *w, void *data);

#endif

