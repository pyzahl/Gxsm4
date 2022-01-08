/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 8 c-style: "K&R" -*- */

/* Gxsm - Gnome X Scanning Microscopy
 * universal STM/AFM/SARLS/SPALEED/... controlling and
 * data analysis software
 * 
 * Copyright (C) 2006 The Free Software Foundation
 *
 * Authors: Thorsten Wagner <stm@users.sf.net>
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


#include <gtk/gtk.h>

#include <dirent.h>
#include <fnmatch.h>

#include "config.h"
#include "plugin.h"
#include "dataio.h"
#include "action_id.h"
#include "util.h"
#include "xsmtypes.h"
#include "glbvars.h"

#include "batch.h"


#define MAX_PATH 1024		// maximum path-length
#define EXT_SEP '.'		//Suffix seperator

class external_converter_Data {

public:
        ~external_converter_Data();

        external_converter_Data(const gchar * src, const gchar * dst,
                                const gchar * conv, const gchar * write);

        gchar *sourceDir;
        gchar *destDir;
        gchar *convBin;
        gchar *writeFormat;
        bool m_recursive;
        bool m_overwrite_target;
        bool m_create_subdirs;

        gchar *sourceFile;
        gchar *destFile;
        //      gchar *destSuffix;
        gchar *converterOptions;
};

class external_converter_Control:public AppBase {
public:

        external_converter_Control(Gxsm4app *app);
        ~external_converter_Control();

        static void recursive_click(GtkWidget * dialog, gpointer userdata);
        static void create_subdirs_click(GtkWidget * dialog,
                                         gpointer userdata);
        static void overwrite_target_click(GtkWidget * dialog,
                                           gpointer userdata);

        void run();
        static void dlg_clicked(GtkDialog * dialog, gint button_number,
                                external_converter_Control * mic);
        external_converter_Data *frontenddata;
        GtkWidget *SourcePath, *DestPath, *DestSuffix, *ConverterBin,
                *ConverterOptions;
};

class external_converter {

public:
        external_converter();
        ~external_converter();

	/** current_dir parameter is set to a value not equal 0 in case
            of recursive traversal of directory tree */
        void ConvertDir(external_converter_Data * work_it,
                        const gchar * current_dir);
        //  gint  readToAct(gchar *fname);  
        //  gint  writeFromCh(gint Ch, gchar *fname);   


private:
        int m_converted;

        gchar *strParse(gchar * fname, external_converter_Data * check);
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

void replacebracket(char *string);
