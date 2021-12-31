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

struct _DSPEmuAppWindow
{
        GtkApplicationWindow parent;
};

typedef struct DSPEmuAppWindowPrivate DSPEmuAppWindowPrivate;

struct DSPEmuAppWindowPrivate
{
        GSettings *settings;
        GtkWidget *stack;
};

G_DEFINE_TYPE_WITH_PRIVATE(DSPEmuAppWindow, dspemu_app_window, GTK_TYPE_APPLICATION_WINDOW);

static void
dspemu_app_window_init (DSPEmuAppWindow *win)
{
        DSPEmuAppWindowPrivate *priv;

        priv = dspemu_app_window_get_instance_private (win);
        gtk_widget_init_template (GTK_WIDGET (win));
        priv->settings = g_settings_new ("org.gtk.mk3dspemu");

        g_settings_bind (priv->settings, "transition",
                         priv->stack, "transition-type",
                         G_SETTINGS_BIND_DEFAULT);
}

static void
dspemu_app_window_dispose (GObject *object)
{
        DSPEmuAppWindow *win;
        DSPEmuAppWindowPrivate *priv;

        win = DSPEMU_APP_WINDOW (object);
        priv = dspemu_app_window_get_instance_private (win);

        g_clear_object (&priv->settings);

        G_OBJECT_CLASS (dspemu_app_window_parent_class)->dispose (object);
}

static void
dspemu_app_window_class_init (DSPEmuAppWindowClass *class)
{
        G_OBJECT_CLASS (class)->dispose = dspemu_app_window_dispose;

        gtk_widget_class_set_template_from_resource (GTK_WIDGET_CLASS (class),
                                                     "/org/gtk/mk3dspemu/window.ui");
        gtk_widget_class_bind_template_child_private (GTK_WIDGET_CLASS (class), DSPEmuAppWindow, stack);
}

DSPEmuAppWindow *
dspemu_app_window_new (DSPEmuApp *app)
{
        return g_object_new (DSPEMU_APP_WINDOW_TYPE, "application", app, NULL);
}

void
dspemu_app_window_open (DSPEmuAppWindow *win,
			GFile            *file)
{
        DSPEmuAppWindowPrivate *priv;
        gchar *basename;
        GtkWidget *scrolled, *view;
        gchar *contents;
        gsize length;
        GtkTextBuffer *buffer;
        GtkTextTag *tag;
        GtkTextIter start_iter, end_iter;

        priv = dspemu_app_window_get_instance_private (win);
        basename = g_file_get_basename (file);

        scrolled = gtk_scrolled_window_new (NULL, NULL);
        gtk_widget_show (scrolled);
        gtk_widget_set_hexpand (scrolled, TRUE);
        gtk_widget_set_vexpand (scrolled, TRUE);
        view = gtk_text_view_new ();
        gtk_text_view_set_editable (GTK_TEXT_VIEW (view), FALSE);
        gtk_text_view_set_cursor_visible (GTK_TEXT_VIEW (view), FALSE);
        gtk_widget_show (view);
        gtk_container_add (GTK_CONTAINER (scrolled), view);
        gtk_stack_add_titled (GTK_STACK (priv->stack), scrolled, basename, basename);

        buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (view));

        if (g_file_load_contents (file, NULL, &contents, &length, NULL, NULL))
                {
                        gtk_text_buffer_set_text (buffer, contents, length);
                        g_free (contents);
                }

        tag = gtk_text_buffer_create_tag (buffer, NULL, NULL);
        g_settings_bind (priv->settings, "font", tag, "font", G_SETTINGS_BIND_DEFAULT);

        gtk_text_buffer_get_start_iter (buffer, &start_iter);
        gtk_text_buffer_get_end_iter (buffer, &end_iter);
        gtk_text_buffer_apply_tag (buffer, tag, &start_iter, &end_iter);

        g_free (basename);
}

guint refresh_inspector (GtkTextBuffer *buffer){ 
        gchar *(*inspector)() = g_object_get_data (G_OBJECT (buffer), "inspector");
        gchar *contents = (*inspector)();
        gtk_text_buffer_set_text (buffer, contents, -1);
        g_free (contents);

        return TRUE;
}

void
dspemu_app_window_insert (DSPEmuAppWindow *win,
                          const gchar     *name,
                          gchar *(*inspector)())
{
        DSPEmuAppWindowPrivate *priv;
        GtkWidget *scrolled, *view;
        gchar *wname = g_strdup (name);
        gchar *contents;
        GtkTextBuffer *buffer;
        GtkTextTag *tag;
        GtkTextIter start_iter, end_iter;

        priv = dspemu_app_window_get_instance_private (win);

        scrolled = gtk_scrolled_window_new (NULL, NULL);
        gtk_widget_show (scrolled);
        gtk_widget_set_hexpand (scrolled, TRUE);
        gtk_widget_set_vexpand (scrolled, TRUE);
        view = gtk_text_view_new ();
        gtk_text_view_set_editable (GTK_TEXT_VIEW (view), FALSE);
        gtk_text_view_set_cursor_visible (GTK_TEXT_VIEW (view), FALSE);
        gtk_widget_show (view);
        gtk_container_add (GTK_CONTAINER (scrolled), view);
        gtk_stack_add_titled (GTK_STACK (priv->stack), scrolled, wname, wname);

        buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (view));

        g_object_set_data (G_OBJECT (buffer), "inspector", inspector);
        contents = (*inspector)();
        gtk_text_buffer_set_text (buffer, contents, -1);
        g_free (contents);

        tag = gtk_text_buffer_create_tag (buffer, NULL, NULL);
        g_settings_bind (priv->settings, "font", tag, "font", G_SETTINGS_BIND_DEFAULT);

        gtk_text_buffer_get_start_iter (buffer, &start_iter);
        gtk_text_buffer_get_end_iter (buffer, &end_iter);
        gtk_text_buffer_apply_tag (buffer, tag, &start_iter, &end_iter);

        g_timeout_add (300, (GSourceFunc)refresh_inspector, buffer);
}

