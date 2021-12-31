/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 8 c-style: "K&R" -*- */

/* SRanger and Gxsm - Gnome X Scanning Microscopy Project
 * universal STM/AFM/SARLS/SPALEED/... controlling and
 * data analysis software
 *
 * DSP tools for Linux
 *
 * Copyright (C) 1999,2000,2001,2002 Percy Zahl
 *
 * Authors: Percy Zahl <zahl@users.sf.net>
 * WWW Home:
 * DSP part:  http://sranger.sf.net
 * Gxsm part: http://gxsm.sf.net
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

#include <gtk/gtk.h>

#include "dspemu_app.h"
#include "dspemu_app_window.h"
#include "dspemu_app_prefs.h"

struct _DSPEmuAppPrefs
{
  GtkDialog parent;
};

typedef struct _DSPEmuAppPrefsPrivate DSPEmuAppPrefsPrivate;

struct _DSPEmuAppPrefsPrivate
{
  GSettings *settings;
  GtkWidget *font;
  GtkWidget *transition;
};

G_DEFINE_TYPE_WITH_PRIVATE(DSPEmuAppPrefs, dspemu_app_prefs, GTK_TYPE_DIALOG)

static void
dspemu_app_prefs_init (DSPEmuAppPrefs *prefs)
{
  DSPEmuAppPrefsPrivate *priv;

  priv = dspemu_app_prefs_get_instance_private (prefs);
  gtk_widget_init_template (GTK_WIDGET (prefs));
  priv->settings = g_settings_new ("org.gtk.mk3dspemu");

  g_settings_bind (priv->settings, "font",
                   priv->font, "font",
                   G_SETTINGS_BIND_DEFAULT);
  g_settings_bind (priv->settings, "transition",
                   priv->transition, "active-id",
                   G_SETTINGS_BIND_DEFAULT);
}

static void
dspemu_app_prefs_dispose (GObject *object)
{
  DSPEmuAppPrefsPrivate *priv;

  priv = dspemu_app_prefs_get_instance_private (DSPEMU_APP_PREFS (object));
  g_clear_object (&priv->settings);

  G_OBJECT_CLASS (dspemu_app_prefs_parent_class)->dispose (object);
}

static void
dspemu_app_prefs_class_init (DSPEmuAppPrefsClass *class)
{
  G_OBJECT_CLASS (class)->dispose = dspemu_app_prefs_dispose;

  gtk_widget_class_set_template_from_resource (GTK_WIDGET_CLASS (class),
                                               "/org/gtk/mk3dspemu/prefs.ui");
  gtk_widget_class_bind_template_child_private (GTK_WIDGET_CLASS (class), DSPEmuAppPrefs, font);
  gtk_widget_class_bind_template_child_private (GTK_WIDGET_CLASS (class), DSPEmuAppPrefs, transition);
}

DSPEmuAppPrefs *
dspemu_app_prefs_new (DSPEmuAppWindow *win)
{
  return g_object_new (DSPEMU_APP_PREFS_TYPE, "transient-for", win, "use-header-bar", TRUE, NULL);
}
