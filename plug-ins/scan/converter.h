/* Gxsm - Gnome X Scanning Microscopy
 * universal STM/AFM/SARLS/SPALEED/... controlling and
 * data analysis software
 * 
 * Copyright (C) 2003 Kristan Temme
 *
 * Authors: Kristan Temme <etptt@users.sf.net>
 *                                                                                
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

#include <gtk/gtk.h>

#include <dirent.h>
#include <fnmatch.h>

#include "config.h"
#include "gxsm4/plugin.h"
#include "gxsm4/dataio.h"
#include "gxsm4/action_id.h"
#include "gxsm4/util.h"
#include "gxsm4/xsmtypes.h"
#include "gxsm4/glbvars.h"

#include "batch.h"

#define MAX_PATH 1024		// maximum path-length
#define EXT_SEP '.'		//Suffix seperator

class converterData {

  public:
    converterData(const gchar * src, const gchar * dst, const gchar * conv,
		  const gchar * write);
    ~converterData();

    gchar *sourceDir;
    gchar *destDir;
    gchar *convFilter;
    gchar *writeFormat;
    bool m_recursive;
    bool m_overwrite_target;
    bool m_create_subdirs;
};


class converter {

  public:
    converter();
    ~converter();

	/** current_dir parameter is set to a value not equal 0 in case
		of recursive traversal of directory tree */
    void ConvertDir(converterData * work_it, const gchar * current_dir);
    void DoConvert(gchar * pathname, gchar * outputname);
    gint readToAct(gchar * fname);
    gint writeFromCh(gint Ch, gchar * fname);


  private:
    int m_converted;

    gchar *strParse(gchar * fname, converterData * check);
    gchar retStr[MAX_PATH];
	/** Concatenate path parts and make sure, that the path in the
		target array is already terminated with a '/' */
    static void concatenate_dirs(gchar * target, const gchar * add);
	/** Copy the name of the full path to the target array, which
		must be big enough. file and current_dir do not need
		to be set. */
    static void create_full_path(gchar * target,
				 const gchar * source_directory,
				 const gchar * current_dir,
				 const gchar * file);
    static void replace_suffix(gchar * target, gchar * new_suffix);
};

class converterControl:public DlgBase {
  public:

    converterControl();
    ~converterControl();

    void run();
    static void dlg_clicked(GtkDialog * dialog, gint button_number,
			    converterControl * mic);
    static void recursive_click(GtkWidget * dialog, gpointer userdata);
    static void create_subdirs_click(GtkWidget * dialog,
				     gpointer userdata);
    static void overwrite_target_click(GtkWidget * dialog,
				       gpointer userdata);

    converterData *topdata;
    GtkWidget *SrcMask, *DestMask;
    GtkWidget *SrcDir_button, *DestDir_button;
};
