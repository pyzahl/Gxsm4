/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 8 c-style: "K&R" -*- */

/* Gxsm - Gnome X Scanning Microscopy
 * universal STM/AFM/SARLS/SPALEED/... controlling and
 * data analysis software
 * 
 * Copyright (C) 2001,2002,2003 Percy Zahl
 *
 * WWW Home: http://garm.sf.net
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

#ifndef __GNOME_RES_H__
#define __GNOME_RES_H__

#include <gtk/gtk.h>
#include "remoteargs.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

        /*
         * Gnome Ressource Types 
         */
        typedef enum {
                GNOME_RES_FIRST,          /* mark of first record, all other field are empty/NULL */
                GNOME_RES_BOOL,           /* variable of gint type, but only boolean values true/false allowed */
                GNOME_RES_STRING,         /* variable of gchar* type */
                GNOME_RES_INT,            /* variable of gint type */
                GNOME_RES_FLOAT,          /* variable of float type */
                GNOME_RES_FLOAT_W_UNIT,   /* variable of float type, with unit, unit Alias Symbol/String hold in options field */
                GNOME_RES_FLOAT_VEC3,     /* variable of float[3] type */
                GNOME_RES_FLOAT_VEC4,     /* variable of float[4] type */
                GNOME_RES_IGNORE_INFO,    /* variable not used, only infotext in comment field is of interest */
                GNOME_RES_SEPARATOR,      /* just a separator */
                GNOME_RES_LAST            /* mark of last record, all other field are empty/NULL */
        } GnomeResType;

        /*
         * Gnome Ressource Edit Types 
         */
        typedef enum {
                GNOME_RES_EDIT_NO,        /* not editable */
                GNOME_RES_EDIT_ENTRY,     /* use Gtk Text Entry */
                GNOME_RES_EDIT_OPTIONS,   /* use Gtk Options Menu */
                GNOME_RES_EDIT_VALSLIDER, /* use Gtk Value Slider, only for skalar numeric data (_INT, _FLOAT) !!
                                             options string format = {"Minval","Maxval","Step", "Page", "Digits"}
                                          */
                GNOME_RES_EDIT_COLORSEL,  /* use GtkColorSelection, use _FLOAT_VEC3 */
                GNOME_RES_EDIT_FONTSEL    /* use GtkFontSelection, use _STRING */
        } GnomeResEditType;

        /*
         * Define handlingmodes if Gnome Druids are running to configure at first startup
         */
        typedef enum {
                GNOME_RES_LEVEL_AUTO,          /* do not ask user, just use default value */
                GNOME_RES_LEVEL_ASKFOR,        /* ask user for value, prompt with default/current value */
                GNOME_RES_LEVEL_ASKFOR_DEPEND, /* ask user for value if depend function returns true */
                GNOME_RES_LEVEL_IGNORE,        /* do not care about this entry */
                GNOME_RES_LEVEL_INFOTEXT       /* just show comment */
        } GnomeResLevel;
	
        /*
         * Gnome Resource Entry descriptor 
         */
        typedef struct {
                GnomeResType type;      /* Entry Type                           */
                GnomeResEditType edit_type; /* Entry Type                           */
                const gchar *path;      /* resource path/name                   */
                const gchar *name;      /* variable name, if not NULL, it's used instead of path */
                const gchar *vdefault;  /* variables default value              */
                void *var;              /* pointer to the variable, type must match Entry Type   */
                const gchar **options;  /* NULL, or list of option strings      */
                const gchar *option_text_format; /* NULL, or formatting string */
                float *option_data;     /* NULL or data for formatting string */
                const gchar *group;     /* Group number, eg. page no used by folder */
                const gchar *comment;   /* comment: explanation for user        */
                GnomeResLevel level;        /* how to handle by first startup       */
                gint (*depend_func)();  /* to figure out dependencies for asking */
                void (*changed_callback)(gpointer);  /* called if nonnull and entry changes */
                gpointer moreinfo;      /* special future use, depends on type  */
                GtkWidget *entry;       /* Edit Entry - internal use, init with NULL! */
                GtkAdjustment *adjustment;  /* Adjustment - internal use, init with NULL! */
                gchar *tmp;             /* temporary variable storage as text - internal use, init with NULL! */
        } GnomeResEntryInfoType;

        /* Macros to simplify usage, do use at least the GNOME_RES_ENTRY_NAME macro! */

#define GNOME_RES_ENTRY(type, edit_type, path, name, vdefault, var, options, option_text_format, option_data, group, comment, level, depend_func, changed_callback, moreinfo) \
        { type, edit_type, path, name, vdefault, var, options, option_text_format, option_data, group, comment, level, depend_func, changed_callback, moreinfo, NULL, NULL, NULL }

#define GNOME_RES_ENTRY_NAME(type, edit_type, path, name, vdefault, var, options, group, comment, level, depend_func, moreinfo) \
        GNOME_RES_ENTRY(type, edit_type, path, name, vdefault, var, options, NULL, NULL, group, comment, level, depend_func, NULL, moreinfo )

#define GNOME_RES_ENTRY_FLOAT(path, name, vdefault, var, group, comment, changed_callback) \
        GNOME_RES_ENTRY(GNOME_RES_FLOAT, GNOME_RES_EDIT_ENTRY, path, name, vdefault, var, NULL, NULL, NULL, group, comment, GNOME_RES_LEVEL_AUTO, NULL, changed_callback, NULL)

#define GNOME_RES_ENTRY_FLOAT_VEC3(path, name, vdefault, var, group, comment, changed_callback) \
        GNOME_RES_ENTRY(GNOME_RES_FLOAT_VEC3, GNOME_RES_EDIT_ENTRY, path, name, vdefault, var, NULL, NULL, NULL, group, comment, GNOME_RES_LEVEL_AUTO, NULL, changed_callback, NULL)

#define GNOME_RES_ENTRY_FLOAT_VEC4(path, name, vdefault, var, group, comment, changed_callback) \
        GNOME_RES_ENTRY(GNOME_RES_FLOAT_VEC4, GNOME_RES_EDIT_ENTRY, path, name, vdefault, var, NULL, NULL, NULL, group, comment, GNOME_RES_LEVEL_AUTO, NULL, changed_callback, NULL)

#define GNOME_RES_ENTRY_FLOATSLIDER(path, name, vdefault, var, options, group, comment, changed_callback) \
        GNOME_RES_ENTRY(GNOME_RES_FLOAT, GNOME_RES_EDIT_VALSLIDER, path, name, vdefault, var, options, NULL, NULL, group, comment, GNOME_RES_LEVEL_AUTO, NULL, changed_callback, NULL)

#define GNOME_RES_ENTRY_STRING(path, vdefault, var, group, comment) \
        GNOME_RES_ENTRY(GNOME_RES_STRING, GNOME_RES_EDIT_ENTRY, path, NULL, vdefault, var, NULL, NULL, NULL, group, comment, GNOME_RES_LEVEL_AUTO, NULL, NULL, NULL )

#define GNOME_RES_ENTRY_COLORSEL(path, name, vdefault, var, group, comment, changed_callback) \
        GNOME_RES_ENTRY(GNOME_RES_FLOAT_VEC4, GNOME_RES_EDIT_COLORSEL, path, name, vdefault, var, NULL, NULL, NULL, group, comment, GNOME_RES_LEVEL_AUTO, NULL, changed_callback, NULL)

#define GNOME_RES_ENTRY_FONTSEL(path, name, vdefault, var, group, comment, changed_callback) \
        GNOME_RES_ENTRY(GNOME_RES_STRING, GNOME_RES_EDIT_FONTSEL, path, name, vdefault, var, NULL, NULL, NULL, group, comment, GNOME_RES_LEVEL_AUTO, NULL, changed_callback, NULL)

#define GNOME_RES_ENTRY_OPTION(type, path, vdefault, var, options, group, comment) \
        GNOME_RES_ENTRY(type, GNOME_RES_EDIT_OPTIONS, path, NULL, vdefault, var, options, NULL, NULL, group, comment, GNOME_RES_LEVEL_AUTO, NULL, NULL, NULL )

#define GNOME_RES_ENTRY_MARK(type)                                      \
        GNOME_RES_ENTRY(type, GNOME_RES_EDIT_NO, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, GNOME_RES_LEVEL_IGNORE, NULL, NULL, NULL )

#define GNOME_RES_ENTRY_MARK_NAME(type,pathname)                        \
        GNOME_RES_ENTRY(type, GNOME_RES_EDIT_NO, pathname, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, GNOME_RES_LEVEL_IGNORE, NULL, NULL, NULL )

#define GNOME_RES_ENTRY_FIRST                   \
        GNOME_RES_ENTRY_MARK (GNOME_RES_FIRST)

#define GNOME_RES_ENTRY_FIRST_NAME(pathname)                    \
        GNOME_RES_ENTRY_MARK_NAME (GNOME_RES_FIRST, pathname)

#define GNOME_RES_ENTRY_LAST                    \
        GNOME_RES_ENTRY_MARK (GNOME_RES_LAST)

#define GNOME_RES_ENTRY_PATH(type, path, vdefault, var, options, group, comment, level, depend_func) \
        GNOME_RES_ENTRY_NAME( type, GNOME_RES_EDIT_ENTRY, path, NULL, vdefault, var, options, group, comment, level, depend_func, NULL )

#define GNOME_RES_ENTRY_PATH_OPTION(type, path, vdefault, var, options, group, comment, level, depend_func) \
        GNOME_RES_ENTRY_NAME( type, GNOME_RES_EDIT_OPTIONS, path, NULL, vdefault, var, options, group, comment, level, depend_func, NULL )

#define GNOME_RES_ENTRY_INFOTEXT(group, comment)                        \
        GNOME_RES_ENTRY_NAME( GNOME_RES_IGNORE_INFO, GNOME_RES_EDIT_NO, NULL, NULL, NULL, NULL, NULL, group, comment, GNOME_RES_LEVEL_INFOTEXT, NULL, NULL )

#define GNOME_RES_ENTRY_SEPARATOR(group, comment)                       \
        GNOME_RES_ENTRY_NAME( GNOME_RES_SEPARATOR, GNOME_RES_EDIT_NO, NULL, NULL, NULL, NULL, NULL, group, comment, GNOME_RES_LEVEL_AUTO, NULL, NULL )

#define GNOME_RES_ENTRY_INFOTEXT_DEPEND(group, comment, depend_func)    \
        GNOME_RES_ENTRY_NAME( GNOME_RES_IGNORE_INFO, GNOME_RES_EDIT_NO, NULL, NULL, NULL, NULL, NULL, group, comment, GNOME_RES_LEVEL_INFOTEXT, depend_func, NULL )

#define GNOME_RES_ENTRY_AUTO_PATH(type, path, vdefault, var, group, comment) \
        GNOME_RES_ENTRY_PATH(type, path, vdefault, var, NULL, group, comment, GNOME_RES_LEVEL_AUTO, NULL )

#define GNOME_RES_ENTRY_AUTO_PATH_UNIT(type, path, vdefault, var, option_unit, group, comment) \
        GNOME_RES_ENTRY_PATH(type, path, vdefault, var, option_unit, group, comment, GNOME_RES_LEVEL_AUTO, NULL )

#define GNOME_RES_ENTRY_AUTO_PATH_OPTION(type, path, vdefault, var, options, group, comment) \
        GNOME_RES_ENTRY_PATH_OPTION(type, path, vdefault, var, options, group, comment, GNOME_RES_LEVEL_AUTO, NULL )

#define GNOME_RES_ENTRY_AUTO_PATH_OPTION_FMT(type, path, vdefault, var, options, option_text_format, option_data, group, comment) \
        GNOME_RES_ENTRY( type, GNOME_RES_EDIT_OPTIONS, path, NULL, vdefault, var, options, option_text_format, option_data, group, comment, GNOME_RES_LEVEL_AUTO, NULL, NULL, NULL )

#define GNOME_RES_ENTRY_ASK_PATH(type, path, vdefault, var, group, comment) \
        GNOME_RES_ENTRY_PATH(type, path, vdefault, var, NULL, group, comment, GNOME_RES_LEVEL_ASKFOR, NULL )

#define GNOME_RES_ENTRY_ASK_PATH_UNIT(type, path, vdefault, var, option_unit, group, comment) \
                GNOME_RES_ENTRY_PATH(type, path, vdefault, var, option_unit, group, comment, GNOME_RES_LEVEL_ASKFOR, NULL )

#define GNOME_RES_ENTRY_ASK_PATH_OPTION(type, path, vdefault, var, options, group, comment) \
        GNOME_RES_ENTRY_PATH_OPTION(type, path, vdefault, var, options, group, comment, GNOME_RES_LEVEL_ASKFOR, NULL )

#define GNOME_RES_ENTRY_ASK_PATH_OPTION_FMT(type, path, vdefault, var, options, option_text_format, option_data, group, comment) \
        GNOME_RES_ENTRY( type, GNOME_RES_EDIT_OPTIONS, path, NULL, vdefault, var, options, option_text_format, option_data, group, comment, GNOME_RES_LEVEL_ASKFOR, NULL, NULL, NULL )

#define GNOME_RES_ENTRY_ASK_PATH_DEPEND(type, path, vdefault, var, group, comment, depend_func) \
        GNOME_RES_ENTRY_PATH(type, path, vdefault, var, NULL, group, comment, GNOME_RES_LEVEL_ASKFOR_DEPEND, depend_func )

#define GNOME_RES_ENTRY_ASK_PATH_DEPEND_UNIT(type, path, vdefault, var, option_unit, group, comment, depend_func) \
        GNOME_RES_ENTRY_PATH(type, path, vdefault, var, option_unit, group, comment, GNOME_RES_LEVEL_ASKFOR_DEPEND, depend_func )

#define GNOME_RES_ENTRY_ASK_PATH_OPTION_DEPEND(type, path, vdefault, var, options, group, comment, depend_func) \
        GNOME_RES_ENTRY(type, GNOME_RES_EDIT_OPTIONS, path, NULL, vdefault, var, options, NULL, NULL, group, comment, GNOME_RES_LEVEL_ASKFOR_DEPEND, depend_func, NULL, NULL )

#define GNOME_RES_ENTRY_ASK_PATH_OPTION_FMT_DEPEND(type, path, vdefault, var, options, option_text_format, option_data, group, comment, depend_func) \
        GNOME_RES_ENTRY( type, GNOME_RES_EDIT_OPTIONS, path, NULL, vdefault, var, options, option_text_format, option_data, group, comment, GNOME_RES_LEVEL_ASKFOR_DEPEND, depend_func, NULL, NULL )


        /*
         * User Configuration Dialog
         */

        typedef struct {
                GnomeResEntryInfoType *res_def;
                const gchar *pref_base_path;
                gchar *pref_ok_message;
                gchar *pref_apply_message;
                GtkWidget *dialog;
                int destroy_on_close;
                int running;
                int height;
                gboolean block;
                gboolean auto_apply;
                void (*pref_apply_callback)(gpointer);
                gpointer pref_apply_callback_data;
        } GnomeResPreferences;

        GnomeResPreferences *gnome_res_preferences_new (GnomeResEntryInfoType *res_def, const gchar *base_path);
        void gnome_res_set_height (GnomeResPreferences *gnome_res_pref, int height);
        void gnome_res_set_auto_apply (GnomeResPreferences *gnome_res_pref, gboolean autoapply);
        void gnome_res_set_ok_message (GnomeResPreferences *gnome_res_pref, const gchar *ok_msg);
        void gnome_res_set_apply_message (GnomeResPreferences *gnome_res_pref, const gchar *apply_msg);
        void gnome_res_set_apply_callback (GnomeResPreferences *self, void (*cb)(gpointer), gpointer data);
        void gnome_res_run_change_user_config(GnomeResPreferences *gnome_res_pref, const gchar *dialog_title);
        void gnome_res_set_destroy_on_close (GnomeResPreferences *self, int flg);
        void gnome_res_destroy (GnomeResPreferences *gnome_res_pref);

        void gnome_res_read_user_config (GnomeResPreferences *gnome_res_pref);
        void gnome_res_write_user_config (GnomeResPreferences *gnome_res_pref);
        void gnome_res_update_all (GnomeResPreferences *self);

        /*
         * generates a gschema "xsm file" in new allocated memory buffer returned, g_free when done with it.
         */
	gchar* gnome_res_write_gschema (GnomeResPreferences *gnome_res_pref);

        /*
         * New User Druid
         */

        typedef enum {
                GNOME_RES_DRUID_RUNNING,
                GNOME_RES_DRUID_FINISH,
                GNOME_RES_DRUID_CANCEL,
        } GnomeResDruidFinishType;

        typedef struct {
                GnomeResPreferences *res_pref;
	
                GtkWidget *window;
                GtkWidget *druid;
                GtkWidget *startpage, *page, *finishpage;
                GnomeResEntryInfoType *res_current;
	
                GdkPixbuf *logo, *watermark;
                gchar *title, *welcome, *intro_text, *finish, *final_text;

                gint pageindex, pagescreated;
                GnomeResDruidFinishType finishmode;
        } GnomeResNewUserDruid;

        GnomeResNewUserDruid *gnome_res_new_user_druid_new (GnomeResEntryInfoType *res_def, const gchar *base_path, const gchar *title);
        GnomeResNewUserDruid *gnome_res_new_user_druid_new_with_vals (GnomeResEntryInfoType *res_def,
                                                                      const gchar *base_path,
                                                                      const gchar *title,
                                                                      GdkPixbuf *logo,
                                                                      GdkPixbuf *watermark,
                                                                      const gchar *welcome, 
                                                                      const gchar *intro_text, 
                                                                      const gchar *finish, 
                                                                      const gchar *final_text
                                                                      );
        GnomeResDruidFinishType gnome_res_new_user_druid_run_druids (GnomeResNewUserDruid *self);
        void gnome_res_new_user_druid_destroy (GnomeResNewUserDruid *self);

        gboolean gnome_res_check_remote_command (GnomeResPreferences *self, remote_args* data);

/* key/path convertsion utils */
        gchar **get_path_name (const gchar *pathname);
        gchar *path_2_dot (gchar *path);
        gchar *make_tuple (const gchar *v);
        gchar *key_assure (const gchar *key);
        gchar* gschema_key_translate (const gchar *key, gint i);


#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* __GNOME_RES_H__ */
