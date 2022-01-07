
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


#ifndef GXSM_MONITORCONTROL_H
#define GXSM_MONITORCONTROL_H

#include <config.h>
#include "monitor.h"
#include "gapp_service.h"

class MonitorControl : public AppBase, Monitor{
public:
        MonitorControl (Gxsm4app *app, gint loglevel=2, gint maxlines=500);
        virtual ~MonitorControl();
        virtual void AppWindowInit(const gchar *title=NULL, const gchar *sub_title=NULL);

        static void file_open_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data);
        static void file_save_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data);
        static void file_close_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data);        
        static void set_logging_history_radio_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data);        
        static void set_logging_level_radio_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data);        
        virtual void LogEvent(const gchar *Action, const gchar *Entry, gint level=1);

        void set_max_lines (gint ml) { max_lines = ml; };
private:
        gint          max_lines;
        GtkWidget     *log_view;
	GtkTextBuffer *log_buf;
        GtkTextMark   *text_end_mark;
};

#endif
