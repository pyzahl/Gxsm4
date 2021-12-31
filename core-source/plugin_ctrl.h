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

#ifndef __PLUGIN_CTRL_H
#define __PLUGIN_CTRL_H

#include <config.h>
#include <glib.h>

#include <dlfcn.h>

#include "plugin.h"

#define SHARED_LIB_EXT_LINUX  ".so"
#define SHARED_LIB_EXT_DARWIN ".so"

class XSM_Hardware;
class App;

class plugin_ctrl{
 public:
	plugin_ctrl(GList *pi_dirlist, gint (*check)(const gchar *) = NULL, const gchar* splash_info = NULL);
	~plugin_ctrl();
	
	void view_pi_info(void);
	GList *get_pluginlist(){ return plugins; };
	
	int how_many(void){ 
		GList *node=plugins; int i=0; 
		while(node){ ++i; node=node->next; } 
		return i;
	};
	
 private:
	void scan_for_pi(gchar *dirname);
	void add_pi(const gchar *filename, const gchar *name);
	void init_pi(void *pi);
	void cleanup_pi(void *pi);
	
	gint (*Check)(const gchar *);
	
 protected:
	GList *plugins;
};


class gxsm_plugins : public plugin_ctrl{
 public:
	gxsm_plugins(App *app, GList *pi_dirlist, gint (*check)(const gchar *) = NULL, const gchar* splash_info = NULL);
	~gxsm_plugins();

 private:
  App *a;
};


class gxsm_hwi_plugins : public plugin_ctrl{
 public:
	gxsm_hwi_plugins(GList *pi_dirlist, gint (*check)(const gchar *) = NULL, gchar *fulltype=NULL, App *app=NULL, const gchar* splash_info = NULL);
	~gxsm_hwi_plugins();
	
	XSM_Hardware* get_xsm_hwi_class (gchar *hwi_sub_type=NULL);
	
 private:
	XSM_Hardware *xsm_hwi_class; // there can only be one active at a time!
};


#endif
