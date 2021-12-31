/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 8 c-style: "K&R" -*- */

/* Gxsm - Gnome X Scanning Microscopy
 * universal STM/AFM/SARLS/SPALEED/... controlling and
 * data analysis software
 *
 * Gnome resource helper
 * file: gnome-res.c
 * 
 * Copyright (C) 2001,2002,2003 Percy Zahl
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

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>
#include <glib.h>
#include <glib/gi18n.h>

#include "config.h"
#include "gnome-res.h"

/* here debugging is handled special, becasue of the universality of this "add on" gnome code */
extern int debug_level;
#define BE_VERBOSE (debug_level > 3)
#define BE_2       (debug_level > 2)
#define BE_WARNING (debug_level > 1)

#ifdef BE_VERBOSE
# define DEBUG_VERBOSE(format, args...) do { if (BE_VERBOSE) g_print ("** (" __FILE__ ": %s) Gnome-Res-DEBUG **: \n - " format, __FUNCTION__, ##args); } while(0)
#else
# define DEBUG_VERBOSE(format, args...) ;
#endif

/* startup with build in GXSM configuration defaults */
extern int force_gxsm_defaults;

#define PREF_CONFDLG_XSIZE   500
#define PREF_CONFDLG_YSIZE   400
#define PREF_VAR_PADDING     4
#define PREF_VAR_USIZE       150
#define PREF_OPT_USIZE       150
#define PREF_IN_VBOX_PACK_MODE        FALSE, TRUE, 1
#define PREF_IN_HBOX_PACK_MODE        TRUE, TRUE, PREF_VAR_PADDING


/* private functions, callbacks */
static void gnome_res_show_info_callback  (GtkWidget *widget, gpointer message);
static void gnome_res_apply_info_callback (GtkDialog* dialog, 
                                           gint button_number,
                                           gpointer gnome_res_pref
                                           );
static void gnome_res_get_option_callback     (GtkWidget *menuitem, gpointer res);
#if 0
static void gnome_res_new_user_druid_prepare  (GtkWidget *w, gpointer druid, gpointer gp_self);
static void gnome_res_new_user_druid_next     (GtkWidget *w, gpointer druid, gpointer gp_self);
static void gnome_res_new_user_druid_back     (GtkWidget *w, gpointer druid, gpointer gp_self);
static void gnome_res_new_user_druid_finished (GtkWidget *w, gpointer druid, gpointer gp_self);
static void gnome_res_new_user_druid_cancel   (GtkWidget *w, gpointer druid, gpointer gp_self);
#endif

GnomeResPreferences *gnome_res_preferences_new (GnomeResEntryInfoType *res_def, const gchar *base_path){
        GnomeResPreferences *self = g_new (GnomeResPreferences, 1);
        self->res_def = res_def;
        self->pref_base_path = base_path;
        DEBUG_VERBOSE ("gnome_res_preferences_new\n");
        self->pref_ok_message = NULL;
        self->pref_apply_message = NULL;
        self->dialog = NULL;
        self->running = FALSE;
        self->height = PREF_CONFDLG_YSIZE;
        self->auto_apply = FALSE;
        self->block = FALSE;
        self->destroy_on_close = TRUE;
        self->pref_apply_callback = NULL;
        self->pref_apply_callback_data = NULL;
        return self;
}
        
/* helper functions */
/* g_free result!!! */
gchar *gnome_res_get_resource_string_from_variable (GnomeResEntryInfoType *res);
void gnome_res_make_resource_variable_edit_field   (GnomeResPreferences *self, GnomeResEntryInfoType *res, GtkWidget *grif, int col);
void gnome_res_update_variable_edit_field (GnomeResPreferences *self, GnomeResEntryInfoType *res);
        

/* private functions */
void gnome_res_new_user_druid_construct            (GnomeResNewUserDruid *self);
gint gnome_res_new_user_druid_construct_page       (GnomeResNewUserDruid *self);
void gnome_res_new_user_druid_destroy_druidpages   (GnomeResNewUserDruid *self);

#define GCONF_APP_PREFIX "/apps/gxsm4/preferences/default"

void gnome_res_set_height (GnomeResPreferences *self, int height){
        self->height = height;
}

void gnome_res_set_auto_apply (GnomeResPreferences *self, gboolean autoapply){
        self->auto_apply = autoapply;
}
        
void gnome_res_destroy (GnomeResPreferences *self){
        DEBUG_VERBOSE ("gnome_res_destroy\n");
        if (self->running)
                gtk_window_destroy (GTK_WINDOW (self->dialog));
        if (self->pref_ok_message) 
                g_free (self->pref_ok_message);
        if (self->pref_apply_message) 
                g_free (self->pref_apply_message);
        g_free (self);
        DEBUG_VERBOSE ("gnome_res_destroy done.\n");
}

void gnome_res_set_ok_message (GnomeResPreferences *self, const gchar *ok_msg){
        if (self->pref_ok_message) 
                g_free (self->pref_ok_message);
        self->pref_ok_message = g_strdup (ok_msg);
}

void gnome_res_set_apply_message (GnomeResPreferences *self, const gchar *apply_msg){
        if (self->pref_apply_message) 
                g_free (self->pref_apply_message);
        self->pref_apply_message = g_strdup (apply_msg);
}

void gnome_res_set_apply_callback (GnomeResPreferences *self, void (*cb)(gpointer), gpointer data){
        self->pref_apply_callback = cb;
        self->pref_apply_callback_data = data;
}

void gnome_res_set_destroy_on_close (GnomeResPreferences *self, int flg){
        self->destroy_on_close = flg;
}
	
gchar *gnome_res_keytranslate (const gchar *name){
        gchar *name_key = g_strdelimit (g_strdup (name), ".", '/'); 
        gchar *tmp = g_ascii_strdown (name_key, -1);
        gchar *key = g_strconcat (GCONF_APP_PREFIX, "/", tmp, NULL);
        g_free (name_key);
        g_free (tmp);
        return key;
}

// separate path, 1 level  -- free with  g_strfreev ();
gchar **get_path_name (const gchar *pathname){
        gchar *tmp = g_ascii_strdown (pathname, -1);
        gchar **tmpv = g_strsplit (tmp, "/", 2);
        g_free (tmp);
        return tmpv;
}

// in place
gchar *path_2_dot (gchar *path){
        gchar *p;
        for (p=path; *p; ++p) if(*p == '/') *p = '.';
        return path;
}

gchar *make_tuple (const gchar *v){
        return g_strdelimit (g_strdup (v), ", ;", ',');
}

// make valid gschema key
gchar *key_assure (const gchar *key){
        gchar *t;
        gchar *p, *k;
        int nup=0;
        for (p=(gchar*)key; *p; ++p)
                if (g_ascii_isupper(*p))
                        ++nup;

        if (g_ascii_isalpha (*key))
                p = t = g_strndup (key, nup + strlen (key));
        else {
                p = t = g_strndup (key, 1 + nup + strlen (key));
                *p = 'x'; ++p;
        }
        for (k=(gchar*)key; *k; ++p, ++k){
                *p = *k; // copy
                if (*p == '/') *p = '-';
                if (*p == '.') *p = '-';
                if (*p == ' ') *p = '-';
                if (*p == '_') *p = '-';
                if (g_ascii_isupper (*p)){
                        if (k == key)
                                *p = g_ascii_tolower (*p);
                        else{
                                if (*(p-1) != '-' && (g_ascii_islower(*(k-1)) || g_ascii_islower(*(k+1)))) { *p = '-'; ++p; }
                                *p = g_ascii_tolower (*k);
                        }
                }
        }
        return t;
}

gchar* gschema_key_translate (const gchar *key, gint i){
        gchar *name_key = key_assure (g_strdup (key));
	gchar *index = i>=0 ? g_strdup_printf ("%03d",i) : NULL;
	gchar *tmp = g_strconcat (name_key, index, NULL);
	g_free (name_key);
	if (index) 
		g_free (index);
	return tmp;
}

gchar *gschema_write_enum_entry (const gchar *id, GnomeResEntryInfoType *res){
        gchar *gxml_tmp;
        gchar *gxml = NULL;
        const gchar **option;
        int i;
        
        if (res->edit_type == GNOME_RES_EDIT_OPTIONS){
                // Variable Edit: choice (if options) else text entry
                if (res->options && res->type != GNOME_RES_FLOAT_W_UNIT){

                        if (*res->options == NULL) return NULL;
                        
                        gxml = g_strdup_printf ("  <enum id=\"%s\">\n", id);
                        
                        for (i=0, option = res->options; *option; ++option, ++i){
                                if (res->option_text_format){
                                        gchar *menuitemtext = g_strdup_printf (res->option_text_format, *option, res->option_data[i]);

                                        gxml_tmp = g_strdup_printf ("%s"
                                                                    "    <value value=\"%d\" nick=\"%s\"/>\n",
                                                                    gxml, i, menuitemtext
                                                                    );
                                        g_free (gxml); gxml = gxml_tmp;
                                        
                                        g_free (menuitemtext);
                                } else {
                                        gxml_tmp = g_strdup_printf ("%s"
                                                                    "    <value value=\"%d\" nick=\"%s \"/>\n",
                                                                    gxml, i, *option
                                                                    );
                                        g_free (gxml); gxml = gxml_tmp;
                                }
                        }
                        
                        gxml_tmp = g_strdup_printf ("%s"
                                                    "  </enum>\n", gxml);
                        g_free (gxml); gxml = gxml_tmp;
                }
        }
        
        return gxml;
}
        
gchar *gschema_write_choices (GnomeResEntryInfoType *res){
        gchar *gxml_tmp;
        gchar *gxml = NULL;
        const gchar **option;
        int i;
        int ok = 0;
        
        if (res->edit_type == GNOME_RES_EDIT_OPTIONS){
                // Variable Edit: choice (if options) else text entry
                if (res->options && res->type != GNOME_RES_FLOAT_W_UNIT){

                        if (*res->options == NULL) return NULL;
                        
                        gxml = g_strdup_printf ("      <choices>\n");
                        
                        for (i=0, option = res->options; *option; ++option, ++i){
                                if (res->option_text_format){
                                        gchar *menuitemtext = g_strdup_printf (res->option_text_format, *option, res->option_data[i]);

                                        gxml_tmp = g_strdup_printf ("%s"
                                                                    "        <choice value='%s'/>\n",
                                                                    gxml, menuitemtext
                                                                    );
                                        g_free (gxml); gxml = gxml_tmp;

                                        if (strcmp (menuitemtext, res->vdefault) == 0) ok=1;
                                        
                                        g_free (menuitemtext);
                                } else {
                                        gxml_tmp = g_strdup_printf ("%s"
                                                                    "        <choice value='%s'/>\n",
                                                                    gxml, *option
                                                                    );
                                        if (strcmp (*option, res->vdefault) == 0) ok=1;

                                        g_free (gxml); gxml = gxml_tmp;
                                }
                        }
                        
                        gxml_tmp = g_strdup_printf ("%s"
                                                    "      </choices>\n", gxml);
                        g_free (gxml); gxml = gxml_tmp;

                        if (!ok){
                                if (BE_WARNING) g_print ("WARNING: NO CHOICE FOR DEFAULT Res->Path=%s default='%s'\n", res->path, res->vdefault);
                        }
                }
        }
        return gxml;
}
 
gchar *gnome_res_write_gschema (GnomeResPreferences *self){
        GnomeResEntryInfoType *res, *r;
        gchar **path_name = NULL;
        gchar *respath, *key, *prev_respath;
        gchar *tmp;

        gchar *dot_path = path_2_dot (g_strdup (self->pref_base_path));
        gchar *enum_defs;
        gchar *choices_defs;
        gchar *gxml_tmp;
        gchar *gxml = g_strdup_printf ("<schemalist>\n");


        for (res = self->res_def; res->type != GNOME_RES_LAST; ++res){
                const gchar **oid = NULL;
                gchar *id = NULL;
                if (res->type == GNOME_RES_FIRST) continue;
                if (res->edit_type != GNOME_RES_EDIT_OPTIONS) continue;

                oid = res->options; // check for reuse, no dupes!
                for (r = self->res_def; r != res && r->type != GNOME_RES_LAST; ++r)
                        if (r->options == oid)
                                break;
                if (r == res){
                        gchar *resid = key_assure (res->path);
                        id = g_strdup_printf ("%s.Option-%s", dot_path, resid);
                        g_free (resid);
                }
                if (id){
                        gchar *resid = key_assure (id);
                        enum_defs = gschema_write_enum_entry (resid, res);
                        if (enum_defs){
                                gxml_tmp = g_strdup_printf ("%s"
                                                            "%s",
                                                            gxml, enum_defs
                                                            );
                                g_free (enum_defs);
                                g_free (gxml); gxml = gxml_tmp;
                        }
                        g_free (resid);
                        g_free (id);
                }
        }


        DEBUG_VERBOSE ("gnome_res_read_write_gschema\n");
        if (self->res_def->path)
                respath = gschema_key_translate (self->res_def->path, -1); // first entry holds the validation key
        else
                respath = gschema_key_translate ("Unknown/ResSetHasNoValidationKey", -1); // dummy

        DEBUG_VERBOSE ("WRITING GSCHEMA\n");

        prev_respath = NULL;
        
        for (res = self->res_def; res->type != GNOME_RES_LAST; ++res){
                if (res->type == GNOME_RES_FIRST) ++res;
                if (res->type == GNOME_RES_IGNORE_INFO) continue;
                if (res->type == GNOME_RES_IGNORE_INFO) continue;
                if (!res->path) continue;

                path_name = get_path_name (res->path);
                respath = path_name[0];
                key = gschema_key_translate (path_name[1], -1);

                if (BE_VERBOSE) g_print ("Res->Path=%s   --> '%s','%s' \n", res->path, respath, key);

                if (prev_respath ? strcmp (prev_respath, respath) : 1  ){
                        gchar *dot_path_add = path_2_dot (g_strdup (respath));
                        gchar *res_path_add = g_strdup (path_name[0]);

                        if (BE_VERBOSE) g_print ("SCHEMA FOR id='%s', dot-path='%s'+'%s', path='%s'+'%s'\n", key, dot_path, dot_path_add, self->pref_base_path, res_path_add);
                        
                        gxml_tmp = g_strdup_printf ("%s"
                                                    "%s"
                                                    "  <schema id=\"%s.%s\" path=\"/%s/%s/\">\n",
                                                    gxml,
                                                    prev_respath != NULL ? "  </schema>\n\n" : "\n",
                                                    dot_path, dot_path_add,
                                                    self->pref_base_path, res_path_add
                                                    );
                        g_free (gxml); gxml = gxml_tmp;
                        g_free (dot_path_add);
                        g_free (res_path_add);

                        if (prev_respath) g_free (prev_respath);
                        prev_respath = g_strdup (respath);
                }

                if (BE_VERBOSE) g_print ("ResPath=%s = %s\n", respath, res->vdefault);

                switch (res->type){

                case GNOME_RES_BOOL: 
                        if (BE_VERBOSE) g_print ("-BOOL-\n");

                        gxml_tmp = g_strdup_printf ("%s"
                                                    "    <key name=\"%s\" type=\"b\">\n"
                                                    "      <default>%s</default>\n"
                                                    "      <summary>%s</summary>\n"
                                                    "      <description>\n"
                                                    "        %s GXSM OPTIONS=(%s, %s)\n"
                                                    "      </description>\n"
                                                    "    </key>\n",
                                                    gxml, key,
                                                    (!strcasecmp (res->vdefault, "true") || !strcasecmp (res->vdefault, "yes") || !strcasecmp (res->vdefault, "positive")) ? "true" : "false",
                                                    res->comment, res->options[0], res->options[1],
                                                    (gchar*) (res->moreinfo ? res->moreinfo : res->comment)
                                                    );
                        g_free (gxml); gxml = gxml_tmp;
                        break;
                        
                case GNOME_RES_STRING: 
                        if (BE_VERBOSE) g_print ("-STRING-\n");

                        choices_defs = gschema_write_choices (res);
        
                        gxml_tmp = g_strdup_printf ("%s"
                                                    "    <key name=\"%s\" type=\"s\">\n"
                                                    "%s"
                                                    "      <default>'%s'</default>\n"
                                                    "      <summary>%s</summary>\n"
                                                    "      <description>\n"
                                                    "        %s\n"
                                                    "      </description>\n"
                                                    "    </key>\n",
                                                    gxml, key,
                                                    choices_defs ? choices_defs : "\n",
                                                    res->vdefault,
                                                    res->comment,
                                                    (gchar*) (res->moreinfo ? res->moreinfo : res->comment)
                                                    );
                        g_free (gxml); gxml = gxml_tmp;
                        if (choices_defs)
                                g_free (choices_defs);
                        break;

                case GNOME_RES_INT: 
                        if (BE_VERBOSE) g_print ("-INT-\n");

                        gxml_tmp = g_strdup_printf ("%s"
                                                    "    <key name=\"%s\" type=\"x\">\n"
                                                    "      <default>%s</default>\n"
                                                    "      <summary>%s</summary>\n"
                                                    "      <description>\n"
                                                    "        %s\n"
                                                    "      </description>\n"
                                                    "    </key>\n",
                                                    gxml, key,
                                                    res->vdefault,
                                                    res->comment,
                                                    (gchar*) (res->moreinfo ? res->moreinfo : res->comment)
                                                    );
                        g_free (gxml); gxml = gxml_tmp;
                        break;

                case GNOME_RES_FLOAT: 
                        if (BE_VERBOSE) g_print ("-FLOAT/_W_UNIT-\n");

                        gxml_tmp = g_strdup_printf ("%s"
                                                    "    <key name=\"%s\" type=\"d\">\n"
                                                    "      <default>%s</default>\n"
                                                    "      <summary>%s</summary>\n"
                                                    "      <description>\n"
                                                    "        %s\n"
                                                    "      </description>\n"
                                                    "    </key>\n",
                                                    gxml, key,
                                                    res->vdefault,
                                                    res->comment,
                                                    (gchar*) (res->moreinfo ? res->moreinfo : res->comment)
                                                    );
                        g_free (gxml); gxml = gxml_tmp;
                        break;

                case GNOME_RES_FLOAT_W_UNIT: 
                        if (BE_VERBOSE) g_print ("-FLOAT/_W_UNIT-\n");

                        gxml_tmp = g_strdup_printf ("%s"
                                                    "    <key name=\"%s\" type=\"d\">\n"
                                                    "      <default>%s</default>\n"
                                                    "      <summary>Unit: [%s] %s</summary>\n"
                                                    "      <description>\n"
                                                    "        %s\n"
                                                    "      </description>\n"
                                                    "    </key>\n",
                                                    gxml, key,
                                                    res->vdefault, *res->options,
                                                    res->comment,
                                                    (gchar*) (res->moreinfo ? res->moreinfo : res->comment)
                                                    );
                        g_free (gxml); gxml = gxml_tmp;
                        break;

                case GNOME_RES_FLOAT_VEC3:
                        if (BE_VERBOSE) g_print ("-FLOAT_VEC3-\n");

                        tmp = make_tuple (res->vdefault);
                        
                        gxml_tmp = g_strdup_printf ("%s"
                                                    "    <key name=\"%s\" type=\"ad\">\n"
                                                    "      <default>[%s]</default>\n"
                                                    "      <summary>%s</summary>\n"
                                                    "      <description>\n"
                                                    "        %s\n"
                                                    "      </description>\n"
                                                    "    </key>\n",
                                                    gxml, key,
                                                    tmp,
                                                    res->comment,
                                                    (gchar*) (res->moreinfo ? res->moreinfo : res->comment)
                                                    );
                        g_free (gxml); gxml = gxml_tmp;
                        g_free (tmp);
                        break;

                case GNOME_RES_FLOAT_VEC4:
                        if (BE_VERBOSE) g_print ("-FLOAT_VEC4-\n");
                        
                        tmp = make_tuple (res->vdefault);

                        gxml_tmp = g_strdup_printf ("%s"
                                                    "    <key name=\"%s\" type=\"ad\">\n"
                                                    "      <default>[%s]</default>\n"
                                                    "      <summary>%s</summary>\n"
                                                    "      <description>\n"
                                                    "        %s\n"
                                                    "      </description>\n"
                                                    "    </key>\n",
                                                    gxml, key,
                                                    tmp,
                                                    res->comment,
                                                    (gchar*) (res->moreinfo ? res->moreinfo : res->comment)
                                                    );
                        g_free (gxml); gxml = gxml_tmp;
                        g_free (tmp);
                        break;
                default: 
                        if (BE_VERBOSE) g_print ("-non-\n");
                        break;
                }

                g_free (key);
                g_strfreev (path_name);
        }

        gxml_tmp = g_strdup_printf ("%s"
                                    "  </schema>\n"
                                    "</schemalist>\n",
                                    gxml
                                    );
        g_free (gxml); gxml = gxml_tmp;

        if (prev_respath) g_free (prev_respath);
        prev_respath = g_strdup (respath);
        
        if (BE_VERBOSE) g_print ("GSCHEMA WRITTEN.\n");

        return gxml;
}

void string_to_vector (const gchar *s_vec, gdouble *g_vec, int n){
        gchar *vec[3], *tmp;
        gchar *p = g_strdup (s_vec);
        int i;

        g_vec[0] = atof( strtok_r(p, ", ", vec));	
        for (i=1; i<n; ++i){
                tmp = strtok_r(NULL, ", ", vec);
                if (tmp)
                        g_vec[i] = atof(tmp);
        }
        g_free (p);
}

void string_to_vector_f (const gchar *s_vec, gfloat *g_vec, int n){
        gchar *vec[3], *tmp;
        gchar *p = g_strdup (s_vec);
        int i;

        g_vec[0] = atof( strtok_r(p, ", ", vec));	
        for (i=1; i<n; ++i){
                tmp = strtok_r(NULL, ", ", vec);
                if (tmp)
                        g_vec[i] = atof(tmp);
        }
        g_free (p);
}

/*
 * Read data from resource file or use defaults 
 * if entry is not present and set variable to proper value
 */

void gnome_res_read_user_config (GnomeResPreferences *self){
        GnomeResEntryInfoType *res;
        GSettings *settings = NULL;
        gchar **path_name;
        gchar *dot_path = path_2_dot (g_strdup (self->pref_base_path));
        gchar *respath, *key, *prev_dotpath, *dot_path_add;
        gchar *settings_dot_path;
        GVariant *gvar;
        gdouble gdvec[4];
        gdouble *gdvec_p;
        gsize gn;
        int i;

        prev_dotpath = NULL;

#if 0
        // DUMMY
        DEBUG_VERBOSE ("gnome_res_read_user_config\n");
        if (self->res_def->path)
                respath = gnome_res_keytranslate (self->res_def->path); // first entry holds the validation key
        else
                respath = gnome_res_keytranslate ("Unknown/ResSetHasNoValidationKey"); // dummy
        // ----
#endif

        DEBUG_VERBOSE ("TESTING GSETTINGS\n");

        for (res = self->res_def; res->type != GNOME_RES_LAST; ++res){
                if (res->type == GNOME_RES_FIRST) ++res;
                if (res->type == GNOME_RES_IGNORE_INFO) continue;
                if (res->type == GNOME_RES_IGNORE_INFO) continue;
                if (res->path == NULL) continue;

                if (BE_VERBOSE) g_print ("Res->Path=%s\n", res->path);

                path_name = get_path_name (res->path);
                respath = path_name[0];
                key = gschema_key_translate (path_name[1], -1);
                
                dot_path_add = path_2_dot (g_strdup (respath));
                settings_dot_path = g_strconcat (dot_path, ".", dot_path_add, NULL);
                g_free (dot_path_add);
                
                // still same path?
                if (prev_dotpath ? strcmp (prev_dotpath, settings_dot_path) : 1){
                        
                        if (settings) g_object_unref (settings); // garbage collect
                        
                       settings = g_settings_new (settings_dot_path);
                        if (prev_dotpath) g_free (prev_dotpath);
                        prev_dotpath = g_strdup (settings_dot_path);
                }

                // get GVarinat -- all types data!
                gvar = g_settings_get_value (settings, key);
  

                if (BE_VERBOSE) g_print ("SettingsPath=%s  %s = gxsm internal default: (%s)\n", settings_dot_path, key, res->vdefault);
                switch (res->type){
                case GNOME_RES_BOOL: 
                        if (BE_VERBOSE) g_print ("-BOOL-\n");

                        if (force_gxsm_defaults){
                                GVariant *gvar_default = g_variant_new_boolean (!strcasecmp (res->vdefault, "true")
                                                                                || !strcasecmp (res->vdefault, "yes")
                                                                                || !strcasecmp (res->vdefault, "positive"));

                                g_settings_set_value (settings, key, gvar_default);
                                //                                g_variant_unref (gvar_default);
                        }
                        * ((int*) (res->var)) = (int) g_variant_get_boolean (gvar);

                        if (0){ // error fall back
                                * ((int*) (res->var)) = !strcasecmp (res->vdefault, "true") || !strcasecmp (res->vdefault, "yes") || !strcasecmp (res->vdefault, "positive");
                                if (BE_VERBOSE) g_print ("ERROR-FALLBACK ==== ResPath=%s = s[%s] sv=%s\n", respath, res->vdefault, (gchar*)res->var);
                        }
                        break;

                case GNOME_RES_STRING: 
                        if (BE_VERBOSE) g_print ("-STRING-\n");

                        if (force_gxsm_defaults){
                                GVariant *gvar_default = g_variant_new_string (res->vdefault);
                                g_settings_set_value (settings, key, gvar_default);
                                //                                g_variant_unref (gvar_default);
                        }

                        strcpy ((gchar*)res->var, g_variant_get_string (gvar, NULL));

                        if (0){ // error fall back
                                strcpy ((gchar*)res->var, res->vdefault);
                                if (BE_VERBOSE) g_print ("ERROR-FALLBACK ==== ResPath=%s = s[%s] sv=%s\n", respath, res->vdefault, (gchar*)res->var);
                        }
                        break;

                case GNOME_RES_INT: 
                        if (BE_VERBOSE) g_print ("-INT-\n");

                        if (force_gxsm_defaults){
                                GVariant *gvar_default = g_variant_new_int64 (atol (res->vdefault));
                                g_settings_set_value (settings, key, gvar_default);
                                //                                g_variant_unref (gvar_default);
                        }

                        * ((int*) (res->var)) = (int) g_variant_get_int64 (gvar);  // !!!!!!!!!!!!!!!!!!!!!! INT64 may exeed range in 32bit compiled version -- FIXME !!

                         if (0){ // error fall back
                                * ((int*) (res->var)) = atoi (res->vdefault);
                                if (BE_VERBOSE) g_print ("ERROR-FALLBACK ==== ResPath=%s = s[%s] sv=%d\n", respath, res->vdefault, *((int*)res->var));
                         }
                        break;

                case GNOME_RES_FLOAT: 
                case GNOME_RES_FLOAT_W_UNIT: 
                        if (BE_VERBOSE) g_print ("-FLOAT/_W_UNIT-\n");

                        if (force_gxsm_defaults){
                                GVariant *gvar_default = g_variant_new_double (atof (res->vdefault));
                                g_settings_set_value (settings, key, gvar_default);
                                //                                g_variant_unref (gvar_default);
                        }

                        * ((float*) (res->var)) = (float) g_variant_get_double (gvar); // float <-> double !!!!!!!! FIX ME

                        if (0){
                                if (BE_VERBOSE) g_print ("ERROR-FALLBACK ==== ResPath=%s = s[%s] fv=%g\n", respath, res->vdefault, atof (res->vdefault));
                                * ((float*) (res->var)) = atof (res->vdefault);
                        }
                        break;

                case GNOME_RES_FLOAT_VEC3:
                        if (BE_VERBOSE) g_print ("-FLOAT_VEC3-\n");

                        if (force_gxsm_defaults){
                                string_to_vector (res->vdefault, gdvec, 3);
                                GVariant *gvar_default = g_variant_new_fixed_array (G_VARIANT_TYPE_DOUBLE, (gconstpointer) gdvec, 3, sizeof (gdouble));
                                g_settings_set_value (settings, key, gvar_default);
                                //                                g_variant_unref (gvar_default);
                        }
                        gdvec_p = (double*) g_variant_get_fixed_array (gvar, &gn, sizeof (gdouble));

                        for (i=0; i<gn && i<3; ++i)
                                ((float*) (res->var))[i] = gdvec_p[i];
                        
                        if (0){
                                if (BE_VERBOSE) g_print ("ERROR-FALLBACK ==== ResPath=%s = s[%s] fv=%s\n", respath, res->vdefault, res->vdefault);
                                string_to_vector_f (res->vdefault,  ((float*) (res->var)), 3);  
                        }
                        break;

                case GNOME_RES_FLOAT_VEC4:
                        if (BE_VERBOSE) g_print ("-FLOAT_VEC4-\n");

                        if (force_gxsm_defaults){
                                string_to_vector (res->vdefault, gdvec, 4);
                                GVariant *gvar_default = g_variant_new_fixed_array (G_VARIANT_TYPE_DOUBLE, (gconstpointer) gdvec, 4, sizeof (gdouble));
                                g_settings_set_value (settings, key, gvar_default);
                                //                                g_variant_unref (gvar_default);
                        }
                        gdvec_p = (double*)g_variant_get_fixed_array (gvar, &gn, sizeof (gdouble));

                        for (i=0; i<gn && i<4; ++i)
                                ((float*) (res->var))[i] = gdvec_p[i];
                        
                        if (0){
                                if (BE_VERBOSE) g_print ("ERROR-FALLBACK ==== ResPath=%s = s[%s] fv=%s\n", respath, res->vdefault, res->vdefault);
                                string_to_vector_f (res->vdefault,  ((float*) (res->var)), 4);  
                        }
                        break;

                default: 
                        if (BE_VERBOSE) g_print ("-non-\n");
                        break;
                }
                g_free (key);
                g_strfreev (path_name);
        }

        if (settings) g_object_unref (settings); // garbage collect

        if (BE_VERBOSE) g_print ("Read User Config done.\n");
        // reset now
        if (force_gxsm_defaults == 2)
                force_gxsm_defaults = 1;
        else
                force_gxsm_defaults = 0;
}

/*
 * Write data back to resource file
 * note: do not call yourself, proper settings of entry and tmp is required!
 */

void gnome_res_write_user_config (GnomeResPreferences *self){
        GnomeResEntryInfoType *res;
        GSettings *settings = NULL;
        gchar **path_name;
        gchar *dot_path = path_2_dot (g_strdup (self->pref_base_path));
        gchar *respath, *key, *prev_dotpath, *dot_path_add;
        gchar *settings_dot_path;
        gdouble gdvec[4];
        int i;
        gchar *vecstr;

        prev_dotpath = NULL;

        DEBUG_VERBOSE ("gnome_res_write_user_config\n");
        
        for (res = self->res_def; res->type != GNOME_RES_LAST; ++res){
                if (res->type == GNOME_RES_FIRST) ++res;
                if (res->type == GNOME_RES_IGNORE_INFO) continue;
                if (res->path == NULL) continue;
                
                if (BE_VERBOSE) g_print ("Res->Path=%s\n", res->path);

                path_name = get_path_name (res->path);
                respath = path_name[0];
                key = gschema_key_translate (path_name[1], -1);
                
                dot_path_add = path_2_dot (g_strdup (respath));
                settings_dot_path = g_strconcat (dot_path, ".", dot_path_add, NULL);
                g_free (dot_path_add);
                
                // still same path?
                if (prev_dotpath ? strcmp (prev_dotpath, settings_dot_path) : 1){
                        
                        if (settings) g_object_unref (settings); // garbage collect
                        
                        settings = g_settings_new (settings_dot_path);
                        if (prev_dotpath) g_free (prev_dotpath);
                        prev_dotpath = g_strdup (settings_dot_path);
                }

                if (BE_VERBOSE) g_print ("SettingsPath=%s  %s = gxsm internal default: (%s)\n", settings_dot_path, key, res->vdefault);

                switch (res->type){
                case GNOME_RES_BOOL: 
                        if (res->entry)
                                * ((int*) (res->var)) = (strcasecmp (gtk_entry_buffer_get_text (GTK_ENTRY_BUFFER (gtk_entry_get_buffer (GTK_ENTRY (res->entry)))), "true") == 0
                                                         || strcasecmp (gtk_entry_buffer_get_text (GTK_ENTRY_BUFFER (gtk_entry_get_buffer (GTK_ENTRY (res->entry)))), "yes") == 0
                                                         || strcasecmp (gtk_entry_buffer_get_text (GTK_ENTRY_BUFFER (gtk_entry_get_buffer (GTK_ENTRY (res->entry)))), "positive") == 0);
                        if (res->tmp)
                                * ((int*) (res->var)) = (strcasecmp (res->tmp, "true") == 0
                                                         || strcasecmp (res->tmp, "yes") == 0
                                                         || strcasecmp (res->tmp, "positive") == 0);

                        { 
                                GVariant *gvar_default = g_variant_new_boolean ((* ((int*) (res->var)))?TRUE:FALSE);
                                g_settings_set_value (settings, key, gvar_default);
                                //                                g_variant_unref (gvar_default);
                        }
                        break;

                case GNOME_RES_STRING: 
                        if (res->entry)
                                strcpy ((gchar*)res->var, gtk_entry_buffer_get_text (GTK_ENTRY_BUFFER (gtk_entry_get_buffer (GTK_ENTRY (res->entry)))));
                        if (res->tmp)
                                strcpy ((gchar*)res->var, res->tmp);
                        {
                                GVariant *gvar_default = g_variant_new_string ((gchar*)res->var);
                                g_settings_set_value (settings, key, gvar_default);
                                //                                g_variant_unref (gvar_default);
                        }
                        break;

                case GNOME_RES_INT: 
                        if (res->entry)
                                * ((int*) (res->var)) =  atoi (gtk_entry_buffer_get_text (GTK_ENTRY_BUFFER (gtk_entry_get_buffer (GTK_ENTRY (res->entry)))));
                        if (res->tmp)
                                * ((int*) (res->var)) =  atoi (res->tmp);
                        {
                                GVariant *gvar_default = g_variant_new_int64 ((gint64) * ((int*) (res->var)));
                                g_settings_set_value (settings, key, gvar_default);
                                //                                g_variant_unref (gvar_default);
                        }
                        break;

                case GNOME_RES_FLOAT: 
                case GNOME_RES_FLOAT_W_UNIT: 
                        if (res->entry)
                                * ((float*) (res->var)) = atof (gtk_entry_buffer_get_text (GTK_ENTRY_BUFFER (gtk_entry_get_buffer (GTK_ENTRY (res->entry)))));
                        if (res->tmp)
                                * ((float*) (res->var)) = atof (res->tmp);
                        {
                                GVariant *gvar_default = g_variant_new_double ((gdouble) * ((float*) (res->var)));
                                g_settings_set_value (settings, key, gvar_default);
                                //                                g_variant_unref (gvar_default);
                        }
                        break;
 
                case GNOME_RES_FLOAT_VEC3:
                        vecstr=NULL;
                        if (res->entry)
                                vecstr = g_strdup (gtk_entry_buffer_get_text (GTK_ENTRY_BUFFER (gtk_entry_get_buffer (GTK_ENTRY (res->entry)))));
                        if (res->tmp){
                                if (vecstr) g_free (vecstr);
                                vecstr = g_strdup (res->tmp);
                        }
                        if (vecstr){
                                string_to_vector (vecstr, gdvec, 3);
                                GVariant *gvar_default = g_variant_new_fixed_array (G_VARIANT_TYPE_DOUBLE, (gconstpointer) gdvec, 3, sizeof (gdouble));
                                g_settings_set_value (settings, key, gvar_default);

                                for (i=0; i<3; ++i)
                                        ((float*) (res->var))[i] = gdvec[i];   // float <-> double !!!!!!!! FIX ME

                                //                                g_variant_unref (gvar_default);
                        }
                        break;

                case GNOME_RES_FLOAT_VEC4:
                        vecstr=NULL;
                        if (res->entry)
                                vecstr = g_strdup (gtk_entry_buffer_get_text (GTK_ENTRY_BUFFER (gtk_entry_get_buffer (GTK_ENTRY (res->entry)))));
                        if (res->tmp){
                                if (vecstr) g_free (vecstr);
                                vecstr = g_strdup (res->tmp);
                        }
                        if (vecstr){
                                string_to_vector (vecstr, gdvec, 4);
                                GVariant *gvar_default = g_variant_new_fixed_array (G_VARIANT_TYPE_DOUBLE, (gconstpointer) gdvec, 4, sizeof (gdouble));
                                g_settings_set_value (settings, key, gvar_default);

                                for (i=0; i<4; ++i)
                                        ((float*) (res->var))[i] = gdvec[i];   // float <-> double !!!!!!!! FIX ME

                                //                                g_variant_unref (gvar_default);
                        }
                        break;

                default: 
                        break;
                }
                g_free (key);
                g_strfreev (path_name);
        }

        if (settings) g_object_unref (settings); // garbage collect
}        

static void gnome_res_show_info_callback (GtkWidget *widget, gpointer message){
        GtkWidget *dialog = gtk_message_dialog_new (NULL,
                                                    GTK_DIALOG_DESTROY_WITH_PARENT,
                                                    GTK_MESSAGE_INFO,
                                                    GTK_BUTTONS_CLOSE,
                                                    "%s", (const gchar *)message);
        g_signal_connect_swapped (G_OBJECT (dialog), "response",
                                  G_CALLBACK (gtk_window_destroy),
                                  G_OBJECT (dialog));
        gtk_widget_show (dialog);
}

static void gnome_res_apply_info_callback (GtkDialog *dialog, 
                                           gint response, 
                                           gpointer gp_self){
        GnomeResPreferences *self = (GnomeResPreferences *) gp_self;
        switch (response){
        case GTK_RESPONSE_ACCEPT: 
                gnome_res_write_user_config (self);
                gtk_window_destroy (GTK_WINDOW (dialog));
                if (self->pref_ok_message){
                        GtkWidget *dialog = gtk_message_dialog_new (NULL,
                                                                    GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                                                    GTK_MESSAGE_INFO,
                                                                    GTK_BUTTONS_OK,
                                                                    "%s", self->pref_ok_message);
                        g_signal_connect (dialog, "response", G_CALLBACK (gtk_window_destroy), NULL);
                        gtk_widget_show (dialog);
                }
                self->running = FALSE;
                if (self->destroy_on_close)
                        gnome_res_destroy (self);
                if (self->pref_apply_callback)
                        (*self->pref_apply_callback)(self->pref_apply_callback_data);
                break;
        case GTK_RESPONSE_APPLY: 
                gnome_res_write_user_config (self);
                if (self->pref_apply_message){
                        GtkWidget *dialog = gtk_message_dialog_new (NULL,
                                                                    GTK_DIALOG_DESTROY_WITH_PARENT,
                                                                    GTK_MESSAGE_INFO,
                                                                    GTK_BUTTONS_OK,
                                                                    "%s", self->pref_apply_message);
                        g_signal_connect (dialog, "response", G_CALLBACK (gtk_window_destroy), NULL);
                        gtk_widget_show (dialog);
                }
                if (self->pref_apply_callback)
                        (*self->pref_apply_callback)(self->pref_apply_callback_data);
                break;
        case GTK_RESPONSE_CANCEL: 
                gtk_window_destroy (GTK_WINDOW (dialog));
                self->running = FALSE;
                if (self->destroy_on_close)
                        gnome_res_destroy (self);
                break;
        }
}

gchar *gnome_res_get_resource_string_from_variable (GnomeResEntryInfoType *res){
	
        switch (res->type){
        case GNOME_RES_BOOL: 
                return g_strdup_printf ("%s", * ((int*) (res->var)) ? res->options[0] : res->options[1]);
        case GNOME_RES_STRING: 
                return g_strdup_printf ("%s", ((gchar*) (res->var)));
        case GNOME_RES_INT: 
                return g_strdup_printf ("%d", * ((int*) (res->var)));
        case GNOME_RES_FLOAT: 
                return g_strdup_printf ("%g", * ((float*) (res->var)));
        case GNOME_RES_FLOAT_W_UNIT: 
                return g_strdup_printf ("%g %s", * ((float*) (res->var)), *res->options);
        case GNOME_RES_FLOAT_VEC3: 
                return g_strdup_printf ("%g, %g, %g", 
                                        ((float*) (res->var))[0], 
                                        ((float*) (res->var))[1], 
                                        ((float*) (res->var))[2]);
        case GNOME_RES_FLOAT_VEC4: 
                return g_strdup_printf ("%g, %g, %g, %g", 
                                        ((float*) (res->var))[0], 
                                        ((float*) (res->var))[1], 
                                        ((float*) (res->var))[2], 
                                        ((float*) (res->var))[3]);
        default: 
                break;
        }
        return g_strdup ("Illegal Ressource Type");
}


/*
 * Write data back to resource file
 * note: do not call yourself, proper settings of entry and tmp is required!
 */

gboolean gnome_res_check_remote_command (GnomeResPreferences *self, remote_args* data){
        if (data){
                if (data->arglist[0] && data->arglist[1]){
                        for (GnomeResEntryInfoType *res = self->res_def; res->type != GNOME_RES_LAST; ++res){
                                if (res->type == GNOME_RES_FIRST) ++res;
                                if (res->type == GNOME_RES_IGNORE_INFO) continue;
                                if (res->path == NULL) continue;
                
                                gchar** path_name = get_path_name (res->path);
                                //gchar* respath = path_name[0];
                                gchar* key = gschema_key_translate (path_name[1], -1);
                                float value;
                                
                                if(data->arglist[1] && data->arglist[2]){
                                        if(! strcmp(data->arglist[1], key)){
                                                switch (res->edit_type){
                                                case GNOME_RES_EDIT_NO: return FALSE;
                                                case GNOME_RES_EDIT_OPTIONS: return FALSE;
                                                case GNOME_RES_EDIT_VALSLIDER:
                                                        * ((float*) res->var) = value = (float) atof(data->arglist[2]); //gtk_adjustment_get_value (adj);
                                                        if (((GnomeResEntryInfoType *) res)->tmp) 
                                                                g_free (((GnomeResEntryInfoType *) res)->tmp);
                                                        ((GnomeResEntryInfoType *) res)->tmp = g_strdup_printf ("%g", value);
                                                        if (res->changed_callback)
                                                                (*res->changed_callback)(res->moreinfo);
                                                        return TRUE;
                                                        break;
                                                case GNOME_RES_EDIT_COLORSEL: return FALSE;
                                                case GNOME_RES_EDIT_FONTSEL: return FALSE;
                                                default: return FALSE;
                                                }
                                        }
                                }
			}
                }
        }
        return FALSE;
}

        
void gnome_res_adjustment_callback(GtkAdjustment *adj, GnomeResEntryInfoType *res){
        GnomeResPreferences *self = (GnomeResPreferences*) g_object_get_data (G_OBJECT (adj), "nome-res-self");
        if (self->block) return;
        
        * ((float*) res->var) = (float) gtk_adjustment_get_value (adj);

        if (((GnomeResEntryInfoType *) res)->tmp) 
                g_free (((GnomeResEntryInfoType *) res)->tmp);

        ((GnomeResEntryInfoType *) res)->tmp = g_strdup_printf ("%g",(float) gtk_adjustment_get_value (adj));

        if (res->changed_callback)
                (*res->changed_callback)(res->moreinfo);
}

void gnome_res_colorchange_callback(GtkColorButton *colorsel, GnomeResEntryInfoType *res){
        GdkRGBA color;
        gtk_color_chooser_get_rgba (GTK_COLOR_CHOOSER (colorsel), &color);
        ((float *)(res->var))[0] = (float) color.red;
        ((float *)(res->var))[1] = (float) color.green;
        ((float *)(res->var))[2] = (float) color.blue;
        ((float *)(res->var))[3] = (float) color.alpha;

        if (((GnomeResEntryInfoType *) res)->tmp) 
                g_free (((GnomeResEntryInfoType *) res)->tmp);

        ((GnomeResEntryInfoType *) res)->tmp = g_strdup_printf ("%g, %g, %g, %g", 
                                                                color.red, color.green, color.blue, color.alpha);
        if (res->changed_callback)
                (*res->changed_callback)(res->moreinfo);
}


void gnome_res_fontchange_callback(GtkFontChooser *gfp, GnomeResEntryInfoType *res){

        if (((GnomeResEntryInfoType *) res)->tmp) 
                g_free (((GnomeResEntryInfoType *) res)->tmp);

        ((GnomeResEntryInfoType *) res)->tmp = g_strdup (gtk_font_chooser_get_font (GTK_FONT_CHOOSER (gfp)));

        if (res->changed_callback)
                (*res->changed_callback)(res->moreinfo);
}


void gnome_res_make_resource_variable_edit_field (GnomeResPreferences *self, GnomeResEntryInfoType *res, 
                                                  GtkWidget *grid, int col){
        GtkWidget *VarName;
        GtkWidget *variable=NULL;
        int defaultidx;
        int i;
        gchar const **option;
	
        // Variable Name
        VarName = gtk_label_new (res->path);
        gtk_widget_set_size_request (VarName, PREF_VAR_USIZE, -1);
        gtk_widget_show (VarName);
        gtk_label_set_xalign (GTK_LABEL (VarName), 1.0);
        gtk_widget_set_margin_start (GTK_WIDGET (VarName), 2);
        gtk_widget_set_margin_end (GTK_WIDGET (VarName), 5);
	
        gtk_grid_attach (GTK_GRID (grid), VarName, 0, col, 1, 1);
	
        res->entry = NULL;
        res->tmp   = NULL;

        gchar** path_name = get_path_name (res->path);
        //gchar* respath = path_name[0];
        gchar* key = gschema_key_translate (path_name[1], -1);
        
        gtk_widget_set_tooltip_text (VarName, key); // PyRemote gxsm.set key

        switch (res->edit_type){
        case GNOME_RES_EDIT_NO: break;
        case GNOME_RES_EDIT_OPTIONS:
                // Variable Edit: choice (if options) else text entry
                if (res->options && res->type != GNOME_RES_FLOAT_W_UNIT){
                        GtkWidget *wid;
			
                        wid = gtk_combo_box_text_new ();
                        g_object_set_data (G_OBJECT (wid), "gnome-res-self", self);

                        gtk_widget_set_size_request (wid, PREF_OPT_USIZE, -1);
                        gtk_grid_attach (GTK_GRID (grid), wid, 1, col, 1, 1);
			
                        g_signal_connect (G_OBJECT (wid), "changed",
                                          G_CALLBACK (gnome_res_get_option_callback),
                                          (gpointer) res);

                        /* Init Choicelist */
                        defaultidx=0;
                        for (i=0, option = res->options; *option; ++option, ++i){
                                if (res->option_text_format){
                                        gchar *menuitemtext = g_strdup_printf (res->option_text_format, *option, res->option_data[i]);
                                        gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (wid), *option, menuitemtext);
                                        g_free (menuitemtext);
                                } else {
                                        gchar *id = g_strdup_printf ("%d", i);
                                        gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (wid), *option, *option);
                                        g_free (id);
                                }
				
                                // g_object_set_data (G_OBJECT (menuitem), "entrydata", (gpointer)*option);
                                {
                                        gchar *s;
                                        if (strcmp (*option, s=gnome_res_get_resource_string_from_variable (res)) == 0)
                                                defaultidx=i;
                                        g_free (s);
                                }
				
                        }
                        gtk_combo_box_set_active (GTK_COMBO_BOX (wid), defaultidx);
                }
                break;
        case GNOME_RES_EDIT_ENTRY:
                {
                        gchar *resdata;
                        variable = (res->entry = gtk_entry_new ());
			
                        resdata = gnome_res_get_resource_string_from_variable (res);
			
                        gtk_entry_buffer_set_text (GTK_ENTRY_BUFFER (gtk_entry_get_buffer (GTK_ENTRY (variable))), resdata, -1);
                        g_free (resdata);
                }
                break;
        case GNOME_RES_EDIT_VALSLIDER:
                {
                        res->adjustment = gtk_adjustment_new ( (gfloat) *((float*) (res->var)),
                                                               (gfloat) atof (res->options[0]), // lower
                                                               (gfloat) atof (res->options[1]), // upper
                                                               (gfloat) atof (res->options[2]), // step
                                                               (gfloat) atof (res->options[3]), // page inc
                                                               (gfloat) 0);                    // page size

                        variable = gtk_scale_new (GTK_ORIENTATION_HORIZONTAL, res->adjustment);
                        gtk_scale_set_value_pos (GTK_SCALE (variable), GTK_POS_LEFT);
                        gtk_scale_set_digits (GTK_SCALE (variable), (gint) atoi (res->options[4]));

                        g_object_set_data (G_OBJECT (res->adjustment), "nome-res-self", self);
                        g_signal_connect (G_OBJECT (res->adjustment), "value_changed",
                                          G_CALLBACK (gnome_res_adjustment_callback), (gpointer) res);
                }
                break;
        case GNOME_RES_EDIT_COLORSEL:
                {
                        GdkRGBA color;
                        color.red   = ((float*)res->var)[0];
                        color.green = ((float*)res->var)[1];
                        color.blue  = ((float*)res->var)[2];
                        color.alpha = ((float*)res->var)[3];
                        variable = gtk_color_button_new_with_rgba (&color);
                        gtk_color_chooser_set_use_alpha (GTK_COLOR_CHOOSER (variable), TRUE);

                        g_signal_connect (G_OBJECT (variable), "color_set",
                                          G_CALLBACK (gnome_res_colorchange_callback), 
                                          (gpointer) res);
                }
                break;
        case GNOME_RES_EDIT_FONTSEL:
                {
                        variable = gtk_font_button_new_with_font (res->var);
                        //if (res->name)
                        //	gnome_font_picker_set_title (GNOME_FONT_PICKER (variable), res->name);
                        g_signal_connect (G_OBJECT (variable), "font_set",
                                          G_CALLBACK (gnome_res_fontchange_callback), 
                                          (gpointer) res);
                }
                break;
        }
        if (variable)
                gtk_grid_attach (GTK_GRID (grid), variable,  1, col, 1, 1);
}

void gnome_res_update_variable_edit_field (GnomeResPreferences *self, GnomeResEntryInfoType *res){
        switch (res->edit_type){
        case GNOME_RES_EDIT_NO: break;
        case GNOME_RES_EDIT_OPTIONS: break;
        case GNOME_RES_EDIT_ENTRY:
                if (res->entry){
                        gchar *resdata = gnome_res_get_resource_string_from_variable (res);
                        //g_message ("gnome_res_update_variable_edit_field entry %s", resdata);
                        gtk_entry_buffer_set_text (GTK_ENTRY_BUFFER (gtk_entry_get_buffer (GTK_ENTRY (res->entry))), resdata, -1);
                        g_free (resdata);
                }
                break;
        case GNOME_RES_EDIT_VALSLIDER:
                if (res->adjustment){
                        //g_message ("gnome_res_update_variable_edit_field adj %g", (gfloat) *((float*) (res->var)));
                        gtk_adjustment_set_value (GTK_ADJUSTMENT (res->adjustment), (gfloat) *((float*) (res->var)));
                }
                break;
        case GNOME_RES_EDIT_COLORSEL: break;
        case GNOME_RES_EDIT_FONTSEL: break;
        }
}

static void gnome_res_get_option_callback (GtkWidget *combo, gpointer res){
        if (((GnomeResEntryInfoType *) res)->tmp) 
                g_free (((GnomeResEntryInfoType *) res)->tmp);

        ((GnomeResEntryInfoType *) res)->tmp = g_strdup (gtk_combo_box_get_active_id (GTK_COMBO_BOX (combo)));
        GnomeResPreferences *self = (GnomeResPreferences *)g_object_get_data (G_OBJECT (combo), "gnome-res-self");
        if (self->auto_apply && self->pref_apply_callback){
                gnome_res_write_user_config (self);
                (self->pref_apply_callback)(self->pref_apply_callback_data);
        }
}

/*
 * Automaticalls pareses resources to tabbed folder dlg
 */
void gnome_res_run_change_user_config (GnomeResPreferences *self, const gchar *dialog_title){
        GtkWidget *notebook;
        GtkWidget *scrolled_contents;
        GtkWidget *grid;
        GtkWidget *help;
        int col;
        int pageno;
	
        GnomeResEntryInfoType *res;
        const gchar *pagename;

        if (self->running) return;
        self->running = TRUE;
	
        DEBUG_VERBOSE ("gnome_res_run_change_user_config\n");
        
        self->dialog = gtk_dialog_new_with_buttons (N_(dialog_title),
                                                    NULL,
                                                    (GtkDialogFlags)(GTK_DIALOG_DESTROY_WITH_PARENT),
                                                    _("_OK"), GTK_RESPONSE_ACCEPT,
                                                    _("_Apply"), GTK_RESPONSE_APPLY,
                                                    _("_Cancel"), GTK_RESPONSE_CANCEL,
                                                    NULL);
        
        gtk_window_get_resizable (GTK_WINDOW (self->dialog));
        
        gtk_widget_set_size_request  (self->dialog, PREF_CONFDLG_XSIZE, self->height);
        
        notebook = gtk_notebook_new ();
        gtk_notebook_set_scrollable (GTK_NOTEBOOK (notebook), TRUE);

        gtk_box_append (GTK_BOX ( gtk_dialog_get_content_area (GTK_DIALOG (self->dialog))), notebook);
        
        for (res = self->res_def, pageno=0; res->type != GNOME_RES_LAST; ++pageno){
                if (res->type == GNOME_RES_FIRST) ++res;
                DEBUG_VERBOSE ("ResEdit page=%d\n", pageno);
                pagename = res->group;
                scrolled_contents = gtk_scrolled_window_new ();
                gtk_widget_set_size_request (scrolled_contents, 500, 500);

                gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_contents), 
                                                GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
                grid = gtk_grid_new ();
                gtk_scrolled_window_set_child (GTK_SCROLLED_WINDOW (scrolled_contents), grid);
                col = 0;

                for (; res->type != GNOME_RES_LAST && strcmp (res->group, pagename)==0; ++res){
                        DEBUG_VERBOSE ("ResEdit res=%s\n", res->path);
                        res->entry = NULL;
                        if (res->type == GNOME_RES_SEPARATOR){
                                gtk_grid_attach (GTK_GRID (grid), gtk_separator_new (GTK_ORIENTATION_HORIZONTAL), 0, col++, 3, 1);
                                if (res->comment){
                                        help = gtk_label_new (res->comment);
                                        gtk_label_set_justify (GTK_LABEL (help), GTK_JUSTIFY_LEFT);
                                        gtk_grid_attach (GTK_GRID (grid), help,  0, col++, 3, 1);
                                        gtk_grid_attach (GTK_GRID (grid), gtk_separator_new (GTK_ORIENTATION_HORIZONTAL), 0, col++, 3, 1);
                                }
                                continue;
                        }
                        if (res->type == GNOME_RES_IGNORE_INFO) continue;
			
                        gnome_res_make_resource_variable_edit_field (self, res, grid, col);
			
                        help = gtk_button_new_with_label (N_("Help"));
                        gtk_grid_attach (GTK_GRID (grid), help,  2, col++, 1, 1);
                        g_signal_connect (G_OBJECT (help), "clicked",
                                          G_CALLBACK (gnome_res_show_info_callback),
                                          (void*)res->comment);
			
                }
                gtk_notebook_append_page (GTK_NOTEBOOK (notebook),
                                          scrolled_contents,
                                          gtk_label_new (pagename));
        }
	
        g_signal_connect (G_OBJECT (self->dialog), "response",
                          G_CALLBACK (gnome_res_apply_info_callback),
                          (gpointer*) self);

        g_object_set_data (G_OBJECT (self->dialog), "self", (gpointer) self);
        gtk_widget_show (self->dialog); // FIX-ME GTK4 SHOWALL
}

/*
 * Automaticalls pareses resources and updates variables
 */
void gnome_res_update_all (GnomeResPreferences *self){
        int pageno;
	
        GnomeResEntryInfoType *res;
        const gchar *pagename;

        if (!self->running) return;
	
        DEBUG_VERBOSE ("gnome_res_update_all\n");
        
        for (res = self->res_def, pageno=0; res->type != GNOME_RES_LAST; ++pageno){
                if (res->type == GNOME_RES_FIRST) ++res;
                DEBUG_VERBOSE ("ResEdit page=%d\n", pageno);
                pagename = res->group;

                for (; res->type != GNOME_RES_LAST && strcmp (res->group, pagename)==0; ++res){
                        DEBUG_VERBOSE ("ResEdit res=%s\n", res->path);
                        if (res->type == GNOME_RES_SEPARATOR) continue;
                        if (res->type == GNOME_RES_IGNORE_INFO) continue;
                        if (res->entry || res->adjustment)
                                gnome_res_update_variable_edit_field (self, res);
                }
        }
}



/* New user wellcome and initial 
 * res data setup guide
 * ========================================
 */
GnomeResNewUserDruid *gnome_res_new_user_druid_new (GnomeResEntryInfoType *res_def, const gchar *base_path, const gchar *title){
        return gnome_res_new_user_druid_new_with_vals (res_def, base_path, title, NULL, NULL, 
                                                       "Welcome", "Setup", "Done", "Have fun!");
}

GnomeResNewUserDruid *gnome_res_new_user_druid_new_with_vals (GnomeResEntryInfoType *res_def,
                                                              const gchar *base_path, 
                                                              const gchar *title,
                                                              GdkPixbuf *logo,
                                                              GdkPixbuf *watermark,
                                                              const gchar *welcome, 
                                                              const gchar *intro_text, 
                                                              const gchar *finish, 
                                                              const gchar *final_text
                                                              ){
        GnomeResNewUserDruid *self = g_new (GnomeResNewUserDruid, 1);
        DEBUG_VERBOSE ("gnome_res_new_user_druid_new_with_vals: 1 \n");

        self->res_pref = gnome_res_preferences_new (res_def, base_path);
        self->title      = g_strdup (title);
        self->logo       = logo;
        self->watermark  = watermark;
        self->welcome    = g_strdup (welcome);
        self->intro_text = g_strdup (intro_text);
        self->finish     = g_strdup (finish);
        self->final_text = g_strdup (final_text);
        self->startpage  = NULL;
        self->page       = NULL;
        self->finishpage = NULL;
        self->pageindex  = 0;
        self->pagescreated = 0;
	
        gnome_res_read_user_config (self->res_pref);
	
        self->window = gtk_window_new ();
        gtk_window_set_title (GTK_WINDOW (self->window), self->title);
	
        gnome_res_new_user_druid_construct (self);
	
        gtk_window_set_child (GTK_WINDOW (self->window), self->druid);
	
        return self;
}

void gnome_res_new_user_druid_destroy (GnomeResNewUserDruid *self){
        DEBUG_VERBOSE ("gnome_res_new_user_druid_destroy: Druid destroy, free\n");
        g_free (self->title);
        g_free (self->welcome);
        g_free (self->intro_text);
        g_free (self->finish);
        g_free (self->final_text);
        gnome_res_destroy (self->res_pref);
        g_free (self);
}


// for some strange reason the "select" mechanish via depend_func
// works not in Gxsm, but in my druid test Prog :-( 
// Disabled by this define:
#define NO_SELECT_PAGES
void gnome_res_new_user_druid_construct (GnomeResNewUserDruid *self){
        DEBUG_VERBOSE ("gnome_res_new_user_druid_construct: ..1\n");

        // gtk_assist???
#if 0
        self->druid = gnome_druid_new ();

        self->startpage = gnome_druid_page_edge_new_with_vals (
                                                               GNOME_EDGE_START, TRUE,
                                                               self->welcome,  self->intro_text,  self->logo,  self->watermark, NULL);
        gnome_druid_append_page (
                                 GNOME_DRUID (self->druid), GNOME_DRUID_PAGE (self->startpage));
        g_object_set_data (G_OBJECT (self->startpage), "self", (gpointer) self);

        g_signal_connect (G_OBJECT (self->startpage), "cancel",
                          G_CALLBACK (gnome_res_new_user_druid_cancel),
                          (gpointer) self);
#ifndef NO_SELECT_PAGES
        g_signal_connect (G_OBJECT (self->startpage), "next",
                          G_CALLBACK (gnome_res_new_user_druid_next),
                          (gpointer) self);
#endif
	
#ifdef NO_SELECT_PAGES
        do{
                gnome_res_new_user_druid_construct_page (self);

                if (self->page){
                        ++self->pagescreated;
                        DEBUG_VERBOSE ("NEXT: adding page\n");
                        gnome_druid_append_page (GNOME_DRUID (self->druid), 
                                                 GNOME_DRUID_PAGE (self->page));
                        g_object_set_data (G_OBJECT (self->page), "self", (gpointer) self);
                }
        }while (self->page);
#endif


        self->finishpage = gnome_druid_page_edge_new_with_vals (
                                                                GNOME_EDGE_FINISH, TRUE,
                                                                self->finish, self->final_text, self->logo, self->watermark, NULL);

        gnome_druid_append_page 
                (GNOME_DRUID (self->druid), GNOME_DRUID_PAGE (self->finishpage));
        g_object_set_data (G_OBJECT (self->finishpage), "self", (gpointer) self);
        g_object_set_data (G_OBJECT (self->finishpage), "beforeself", (gpointer) self->startpage);
        g_object_set_data (G_OBJECT (self->startpage), "behindeself", (gpointer) self->finishpage);

        g_signal_connect (G_OBJECT (self->finishpage), "finish",
                          G_CALLBACK (gnome_res_new_user_druid_finished),
                          (gpointer) self);
        g_signal_connect (G_OBJECT (self->finishpage), "cancel",
                          G_CALLBACK (gnome_res_new_user_druid_cancel),
                          (gpointer) self);
#ifndef NO_SELECT_PAGES
        g_signal_connect (G_OBJECT (self->startpage), "back",
                          G_CALLBACK (gnome_res_new_user_druid_back),
                          (gpointer) self);
#endif	

#endif
}

gint gnome_res_new_user_druid_construct_page (GnomeResNewUserDruid *self){
#if 0
        static GnomeResEntryInfoType *res=NULL;
        GtkWidget *scrolled_window = NULL;
        GtkWidget *vbox;
        GtkWidget *hbox;
        GtkWidget *help;
        int num;
        const gchar *pagename;
        /*    GdkFont *helptxtfont = gdk_font_load ("-adobe-helvetica-medium-r-normal-*-*-100-*-*-p-*-iso8859-1"); */
	
        DEBUG_VERBOSE ("gnome_res_new_user_druid_construct_page\n");
	
        self->page = NULL;
        if (!res) res = self->res_pref->res_def;
        if (res->type == GNOME_RES_LAST) return FALSE;
	
        for (; res->type != GNOME_RES_LAST; ){
                if (res->type == GNOME_RES_FIRST) ++res;
                DEBUG_VERBOSE ("DruidPageConstruct: path=%s group=%s\n", res->path, res->group);
#ifndef NO_SELECT_PAGES
                if (res->depend_func)
                        if (! (*res->depend_func)()){
                                DEBUG_VERBOSE ("skipping, depfkt does not match!\n");
                                ++res;
                                continue;
                        }
#endif
                pagename = res->group;
                scrolled_window = gtk_scrolled_window_new (NULL, NULL);
                gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window), 
                                                GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
                vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
                //gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (scrolled_window), vbox); // FIX-ME GTK4 ?? works ??
                gtk_scrolled_window_set_child (GTK_SCROLLED_WINDOW (scrolled_window), vbox);
		
                for (num=0; res->type != GNOME_RES_LAST && strcmp (res->group, pagename)==0; ++res){
                        DEBUG_VERBOSE ("in group, path=%s\n", res->path);
                        res->entry = NULL;
                        if (res->level == GNOME_RES_LEVEL_AUTO) continue;
#ifndef NO_SELECT_PAGES
                        if (res->depend_func)
                                if (! (*res->depend_func)()){
                                        DEBUG_VERBOSE ("skipping, depfkt does not match!\n");
                                        continue;
                                }
#endif
                        ++num;
			
                        if (res->type == GNOME_RES_IGNORE_INFO){
                                gtk_box_pack_start (GTK_BOX (vbox), gtk_separator_new (GTK_ORIENTATION_HORIZONTAL), PREF_IN_VBOX_PACK_MODE);
                                // --------------
                                hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
                                // Add Comments and Section Info
                                help = gtk_label_new (res->comment);
                                gtk_label_set_justify (GTK_LABEL (help), GTK_JUSTIFY_LEFT);
                                //gtk_label_set_line_wrap (GTK_LABEL (help), TRUE);
				
                                //help = gtk_text_new (NULL, NULL);
                                //gtk_text_set_word_wrap (GTK_TEXT (help), TRUE);
                                //gtk_text_insert (GTK_TEXT (help), helptxtfont, NULL, NULL, res->comment, -1);
				
                                gtk_box_pack_start (GTK_BOX (hbox), help, PREF_IN_HBOX_PACK_MODE);
                                gtk_box_pack_start (GTK_BOX (vbox), hbox, PREF_IN_VBOX_PACK_MODE);
				
                        }else{

                                gtk_box_pack_start (GTK_BOX (vbox), gtk_separator_new (GTK_ORIENTATION_HORIZONTAL), PREF_IN_VBOX_PACK_MODE);
                                // --------------
                                hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
                                // Help Text/Choices
                                help = gtk_label_new (res->comment);
                                gtk_label_set_justify (GTK_LABEL (help), GTK_JUSTIFY_LEFT);
                                gtk_misc_set_alignment (GTK_MISC (help), 0.0, 0.5);
                                gtk_box_pack_start (GTK_BOX (hbox), help, PREF_IN_HBOX_PACK_MODE);
                                gtk_box_pack_start (GTK_BOX (vbox), hbox, PREF_IN_VBOX_PACK_MODE);
                                // --------------
                                hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
				
                                gnome_res_make_resource_variable_edit_field (self, res, hbox);

                                gtk_box_pack_start (GTK_BOX (vbox), hbox, PREF_IN_VBOX_PACK_MODE);
                        }
                }
                if (num){
                        self->page = gnome_druid_page_standard_new_with_vals (pagename, self->logo, self->watermark);
                        gtk_box_pack_start (GTK_BOX ((GNOME_DRUID_PAGE_STANDARD (self->page))->vbox),
                                            GTK_WIDGET (scrolled_window), TRUE, TRUE, 0);
                        break;
                }
        }
        return self->page ? TRUE:FALSE;
#endif
        return FALSE;
}

GnomeResDruidFinishType gnome_res_new_user_druid_run_druids (GnomeResNewUserDruid *self){
        DEBUG_VERBOSE ("gnome_res_new_user_druid_run_druids\n");
	
        self->finishmode = GNOME_RES_DRUID_RUNNING;
        gtk_widget_show (GTK_WIDGET (self->window)); // FIX-ME GTK4 SHOWALL

        //gtk_main ();
	
        return self->finishmode;
}

void gnome_res_new_user_druid_destroy_druidpages (GnomeResNewUserDruid *self) {
        DEBUG_VERBOSE ("gnome_res_new_user_druid_destroy_druidpages: Druid destroy druids\n");
        g_object_unref (G_OBJECT (self->druid));
        DEBUG_VERBOSE ("Druid destroy window\n");
        gtk_window_destroy (GTK_WINDOW(self->window));
        //	DEBUG_VERBOSE ("Druid destroy startpage\n");
        //	g_object_unref (G_OBJECT (self->startpage));
        // gslist...
        //	DEBUG_VERBOSE ("Druid destroy finishpage\n");
        //    g_object_unref (G_OBJECT (self->finishpage));
        DEBUG_VERBOSE ("Druid destroy done.\n");
}

#if 0
static void gnome_res_new_user_druid_prepare (GtkWidget *w, gpointer druid, gpointer gp_self) {
        GnomeResNewUserDruid *self =  (GnomeResNewUserDruid *) gp_self;
        DEBUG_VERBOSE ("PREPARE: %d (%d)\n", self->pageindex, self->pagescreated);
}

static void gnome_res_new_user_druid_next (GtkWidget *w, gpointer druid, gpointer gp_self) {
        GnomeResNewUserDruid *self =  (GnomeResNewUserDruid *) gp_self;
        DEBUG_VERBOSE ("NEXTv: %d (%d)\n", self->pageindex, self->pagescreated);

        gnome_res_write_user_config (self->res_pref);

        if (++self->pageindex > self->pagescreated){
                GtkWidget *BackPage = self->page ? self->page : self->startpage;
                DEBUG_VERBOSE ("NEXT: checking/creating new page\n");

                gnome_res_new_user_druid_construct_page (self);

                if (self->page){
                        ++self->pagescreated;
                        DEBUG_VERBOSE ("NEXT: adding page\n");
                        gnome_druid_insert_page (GNOME_DRUID (self->druid), 
                                                 GNOME_DRUID_PAGE (BackPage), 
                                                 GNOME_DRUID_PAGE (self->page));
                        /* insert into double linked list */
                        g_object_set_data (G_OBJECT (self->page), "self", (gpointer) self);
                        g_object_set_data (G_OBJECT (self->page), "before_self", (gpointer) BackPage);
                        g_object_set_data (G_OBJECT (self->page), "behind_self", (gpointer) self->finishpage);
                        g_object_set_data (G_OBJECT (BackPage), "behind_self", (gpointer) self->page);

                        g_signal_connect (G_OBJECT (self->page), "cancel",
                                          G_CALLBACK (gnome_res_new_user_druid_cancel),
                                          (gpointer) self);
                        g_signal_connect (G_OBJECT (self->page), "next",
                                          G_CALLBACK (gnome_res_new_user_druid_next),
                                          (gpointer) self);
                        g_signal_connect (G_OBJECT (self->page), "back",
                                          G_CALLBACK (gnome_res_new_user_druid_back),
                                          (gpointer) self);
                        gtk_widget_show_all (GTK_WIDGET (self->page));

                        DEBUG_VERBOSE ("NEXT: setting new page\n");
                        gnome_druid_set_page (GNOME_DRUID (self->druid), GNOME_DRUID_PAGE (self->page));
                }else{ 
                        DEBUG_VERBOSE ("NEXT: no more pages, going to finishpage\n");
                        --self->pageindex;
                        self->page = BackPage;
                        gnome_druid_set_page (GNOME_DRUID (self->druid), GNOME_DRUID_PAGE (self->finishpage));
                }
        }else{
                DEBUG_VERBOSE ("NEXT: pages already created, going to this page\n");
                self->page = (GtkWidget *) g_object_get_data (G_OBJECT (self->page), "behind_self");
                gnome_druid_set_page (GNOME_DRUID (self->druid), GNOME_DRUID_PAGE (self->page));
        }
        DEBUG_VERBOSE ("NEXTn: %d (%d)\n", self->pageindex, self->pagescreated);
}

static void gnome_res_new_user_druid_back (GtkWidget *w, gpointer druid, gpointer gp_self) {
        GnomeResNewUserDruid *self =  (GnomeResNewUserDruid *) gp_self;
        DEBUG_VERBOSE ("BACKv: %d (%d)\n", self->pageindex--, self->pagescreated);
        self->page = (GtkWidget *) g_object_get_data (G_OBJECT (self->page), "before_self");
        gnome_druid_set_page (GNOME_DRUID (self->druid), GNOME_DRUID_PAGE (self->page));
        DEBUG_VERBOSE ("BACKn: %d (%d)\n", self->pageindex, self->pagescreated);
}

static void gnome_res_new_user_druid_finished (GtkWidget *w, gpointer druid, gpointer gp_self) {
        GnomeResNewUserDruid *self =  (GnomeResNewUserDruid *) gp_self;
        DEBUG_VERBOSE ("gnome_res_new_user_druid_finished\n");
        self->finishmode = GNOME_RES_DRUID_FINISH;
        gnome_res_write_user_config (self->res_pref);
        gnome_res_new_user_druid_destroy_druidpages (self);
        gtk_main_quit ();
}

void gnome_res_new_user_druid_cancel (GtkWidget *w, gpointer druid, gpointer gp_self) {
        GnomeResNewUserDruid *self =  (GnomeResNewUserDruid *) gp_self;
        DEBUG_VERBOSE ("gnome_res_new_user_druid_cancel\n");
        //    if (gapp->question (("Really cancel/quit?"), TRUE)){
        self->finishmode = GNOME_RES_DRUID_CANCEL;
        gnome_res_new_user_druid_destroy_druidpages (self);
        gtk_main_quit ();
        //    }
}
#endif

#ifdef __cplusplus
}
#endif /* __cplusplus */
