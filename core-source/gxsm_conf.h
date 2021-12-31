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

#ifndef __GXSM_CONF_H
#define __GXSM_CONF_H

#include <gtk/gtk.h>

// Dummy replacement for transition
// => only setting value from default

class XsmRescourceManager{
 public:
	XsmRescourceManager(const gchar *prefix = NULL, const gchar *group = NULL) {};
	~XsmRescourceManager() {};

	void PutBool(const gchar *name, gboolean value, gint i=-1) {};
	void Put(const gchar *name, double value, gint i=-1) {};
	void Put(const gchar *name, int value, gint i=-1) {};
	void Put(const gchar *name, const gchar *value, gint i=-1) {};

	gboolean GetBool(const gchar *name, gboolean defaultv=FALSE, gint i=-1) { return defaultv; };
	int Get(const gchar *name, double *value, const gchar *defaultv=NULL, gint i=-1) { *value = atof (defaultv); return 0; };
	int Get(const gchar *name, int *value, const gchar *defaultv=NULL, gint i=-1) { *value = atoi (defaultv); return 0; };
	int Get(const gchar *name, gchar **value, const gchar *defaultv=NULL, gint i=-1) { *value = g_strdup (defaultv); return 0; };
	gchar *GetStr(const gchar *name, const gchar *defaultv=NULL, gint i=-1)  { return g_strdup (defaultv); };
};

#endif /* __GXSM_CONF_H */
