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

#ifndef __MONITOR_H
#define __MONITOR_H

#include <fstream>

#include <config.h>

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#include <gtk/gtk.h>

#define MAXMONITORFIELDS 10

class Monitor{
public:
        Monitor(gint loglevel=2);
        virtual ~Monitor();

#ifdef GXSM_MONITOR_VMEMORY_USAGE
        gchar* get_vmem_and_refcounts_info ();
        static gint auto_log_timeout_func (gpointer data);
        gint parseLine (char* line);
        gint getValue (const gchar *what);
#endif
                
        virtual void Messung (double val=0., gchar *txt=NULL);
        virtual void LogEvent (const gchar *Action, const gchar *Entry, gint level=1) {
                if (logging_level >= level)
                        PutLogEvent (Action, Entry);
        };

        void set_logging_level (gint lv=1) { logging_level=lv; };
        void SetLogName (char *name);
        void PutLogEvent (const gchar *Action, const gchar *Entry, gint r=0);
        gint AppLine ();
        
protected:
        gint logging_level;
        double dt;
        char   *logname;
private:
        gchar *Fields[MAXMONITORFIELDS];
        guint vmem_auto_log_interval_seconds;
        gint64 t0;
};

#endif




