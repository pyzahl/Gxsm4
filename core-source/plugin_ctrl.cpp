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
 * it under the terms of the GNU General Public License as pub1lished by
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


#include <cerrno>

#include <cstdio>
#include <cstring>
#include <cstdlib>

#include <dirent.h>
#include <gmodule.h>

#include "glbvars.h"
#include "plugin_ctrl.h"

// #define XSM_DEBUG(L, DBGTXT) std::cout << "** (" << __FILE__ << ": " << __FUNCTION__ << ") Gxsm-DEBUG-MESSAGE **: " << std::endl << " - " << DBGTXT << std::endl


// Linux
#define GXSM_PI_VOID_PREFIX "_Z"
#define GXSM_PI_VOID_SUFFIX "v"
#define GXSM_PI_VOIDP_SUFFIX "Pv"

// Darwin tests
//#define GXSM_PI_VOID_PREFIX "_Z"
//#define GXSM_PI_VOID_SUFFIX "v.eh"
//#define GXSM_PI_VOIDP_SUFFIX "Pv.eh"


gint pi_total = 0;
gint pi_num = 0;

/**
 * plugin_ctrl::plugin_ctrl - Initialize PlugIn Control Object and scan for PlugIns
 * @pi_dirlist: GList of Directories to scan for PlugIns
 * @check: Function used for auto selecting of PlugIns
 *
 * Constructor of pluging_ctrl class. It scans for PlugIns, loads them
 * if they are matching to Filter Conditions (check argument). 
 * Afterwards all Plugins are initialized.
 */

plugin_ctrl::plugin_ctrl(GList *pi_dirlist, gint (*check) (const gchar *), const gchar* splash_info){
	GList *node = pi_dirlist;
	plugins = NULL;
	Check = check;

	pi_num=0;
	pi_total=1;

        gint file_count = g_list_length (pi_dirlist);
        
	// Scan for Plugins
        gchar *six = g_strdup_printf ("Scanning %s PlugIns.", splash_info?splash_info : "GXSM");

	main_get_gapp ()->GxsmSplash (-0.1, six, " ... ");
	XSM_DEBUG (DBG_L3, six);

        while(node){
		main_get_gapp ()->GxsmSplash ((gdouble)pi_total/file_count, six, (gchar *) node->data);
		scan_for_pi((gchar *) node->data);
		node = node->next;
		++pi_total;
	}
        g_free (six);

	// Init Plugins
        six = g_strdup_printf ("Initializing %s PlugIns.", splash_info?splash_info : "GXSM");
	main_get_gapp ()->GxsmSplash (0.0, six, " --- ");
	XSM_DEBUG (DBG_L3, six);
	node = plugins;
        
        gint pi_count = g_list_length (node);

	while(node){
		main_get_gapp ()->GxsmSplash ((gdouble)pi_num/pi_count, six, (gchar *)((GxsmPlugin *)node->data)->filename);
		init_pi((void *) node->data);
		node = node->next;
		++pi_num;
	}
        g_free (six);
}

/**
 * plugin_ctrl::~plugin_ctrl - Cleanup of Plugins
 *
 * Destructor of plugin_ctrl. It removes all PlugIns and there Menuentries of necessary.
 */

plugin_ctrl::~plugin_ctrl(){
	GList *node = plugins;
	XSM_DEBUG (DBG_L3, "Cleaning up Plugins");
	while(node){
		cleanup_pi((void *) node->data);
		node = node->next;
	}
	g_list_free(plugins);
	XSM_DEBUG (DBG_L3, "Cleaning up Plugins done");
}

void plugin_ctrl::scan_for_pi(gchar *dirname){
	gchar *filename, *ext;
	DIR *dir;
	struct dirent *ent;
	struct stat statbuf;

	XSM_DEBUG_GP (DBG_L3, "plugin_ctrl::scan_for_pi ** scanning for gxsm plugins in PACKAGE_PLUGIN_DIR=%s ", PACKAGE_PLUGIN_DIR);
        XSM_DEBUG_GP (DBG_L2, "plugin_ctrl::scan_for_pi ** scanning for gxsm plugins in: %s", dirname);
        
	dir = opendir(dirname);
	if (!dir)
		return;

	while ((ent = readdir(dir)) != NULL)
	{
		filename = g_strdup_printf("%s/%s", dirname, ent->d_name);
                XSM_DEBUG_GP (DBG_L3, "plugin_ctrl::scan_for_pi ** scanning file for gxsm plugin: %s/%s  [%s]", dirname, ent->d_name, filename);
                
		if (!stat(filename, &statbuf) && S_ISREG(statbuf.st_mode) &&
		    (ext = strrchr(ent->d_name, '.')) != NULL){
			if (!strcmp(ext, SHARED_LIB_EXT_LINUX))
				add_pi (filename, ent->d_name);
			else if (!strcmp(ext, SHARED_LIB_EXT_DARWIN))
				add_pi(filename, ent->d_name);
		}
		g_free(filename);
	}
	closedir(dir);
}

void plugin_ctrl::add_pi(const gchar *filename, const gchar *name){
	gint ok;
        gchar *ext;
	GModule *gxsm_module;
	GxsmPlugin *(*gpi) (void);

	XSM_DEBUG_GM (DBG_L2, "plugin_ctrl::add_pi ** Checking Plugin: %s", filename );

        gchar *pcs_settings_key=NULL;
        {       // set gsettings schema path derived from pi filename
                XSM_DEBUG_GM (DBG_L2, "plugin_ctrl::add_pi ** SET_PCS_GROUP from %s", name );
                gchar *tmp = g_strdup (name);
                if ((ext = strrchr(tmp, '.')) != NULL){
                        *ext = 0;
                        gchar *tmp2 = g_strdelimit (g_strdup (tmp), ". _", '-');
                        gchar *plugin_key = g_ascii_strdown (tmp2, -1);
                        g_free (tmp2);
                        pcs_settings_key = g_strdup_printf ("plugin-%s", plugin_key); 
                        XSM_DEBUG_GM (DBG_L2, "plugin_ctrl::add_pi ** SET_PCS_GROUP, SETTINGS KEY = '%s'", pcs_settings_key );
                        pcs_set_current_gschema_group (pcs_settings_key);
                        g_free (plugin_key);
                } else {
                        pcs_set_current_gschema_group ("plugins-unspecified"); // set fallback just in case
                }
                g_free (tmp);
        }

        
        XSM_DEBUG_GM (DBG_L1, "plugin_ctrl::add_pi ** g_module_open: %s", filename );
	if ((gxsm_module = g_module_open (filename, G_MODULE_BIND_LAZY)) != NULL)
	{
		XSM_DEBUG_GM (DBG_L1, "plugin_ctrl::add_pi ** g_module_open OK... checking for symbol 'get_gxsm_plugin_info'");
		// gcc 2.95:  "get_gxsm_plugin_info__Fv"
		// gcc 3.3:   "_Z20get_gxsm_plugin_infov"
		if (g_module_symbol (gxsm_module, "_Z20""get_gxsm_plugin_info" GXSM_PI_VOID_SUFFIX, (gpointer*)&gpi))
		{
                        XSM_DEBUG_GM (DBG_L1, "plugin_ctrl::add_pi ** PI entry/description table g_module_symbol found: %s", filename );
			GxsmPlugin *p = gpi();
			ok = TRUE;
			if(Check && p -> category){
                                XSM_DEBUG_GM (DBG_L1, "plugin_ctrl::add_pi ** Check category: [%s] %s", p -> category, filename );
				ok = Check( p -> category );
                        }
                        if(ok){
				p->module   = gxsm_module;
				p->filename = g_strdup_printf("%s|%s", filename, pcs_settings_key);
				plugins = g_list_prepend(plugins, p);
                                XSM_DEBUG_GM (DBG_L1, "plugin_ctrl::add_pi ** success: loaded plugin: %s", filename );
			}else{
                                XSM_DEBUG_GM (DBG_L1, "plugin_ctrl::add_pi ** load success, but skipped due to no category match (OK) for: %s", filename );
				g_module_close(gxsm_module);
			}
		}
		else{
                        XSM_DEBUG_GW (DBG_EVER, "plugin_ctrl::add_pi ** reference symbol not found, no valid GXSM plugin. : %s [%s] -- skipping.", filename, g_module_error () );
			g_module_close(gxsm_module);
		}
	}
	else{
		XSM_DEBUG_GW (DBG_EVER, "Failed loading PlugIn <%s>: %s", filename, g_module_error () );
	}

        g_free (pcs_settings_key);
}



void plugin_ctrl::init_pi(void *pi){
	GxsmPlugin *p;
  
	p = (GxsmPlugin *) pi;
        //	if (p) {
        //                XSM_DEBUG_GP (DBG_L2, "INIT_PI:: %s", p->filename);
        //		main_get_gapp ()->GxsmSplash ((gdouble)pi_num/(gdouble)pi_total, p->filename);
        //		main_get_gapp ()->SetStatus (p->filename);
        //		gdk_flush ();
        //	}
	if (p && p->init)
	{
                gchar **k=g_strsplit_set (p->filename,"|",2);
                pcs_set_current_gschema_group (k[1]);
                XSM_DEBUG_GP (DBG_L2, "plugin_ctrl::init_pi -- current pcs gschema group = '%s'\n", k[1] );
                g_strfreev(k);
		p->init();
	}
}

void plugin_ctrl::cleanup_pi(void *pi){
	GxsmPlugin *p;
  
	p = (GxsmPlugin *) pi;
//	if (p) {
//		XSM_DEBUG_GP (DBG_L2, "CLEANUP_PI:: %s", p->filename);
//		main_get_gapp ()->SetStatus (p->filename);
//		gdk_flush ();
//	}
	if (p && p->cleanup)
	{
		XSM_DEBUG (DBG_L3, "Cleanup: " << p->name);
                gchar **k=g_strsplit_set (p->filename,"[]",2);
                pcs_set_current_gschema_group (k[1]);
                XSM_DEBUG_GP (DBG_L2, "plugin_ctrl::cleanup_pi -- SET_PCS_GROUP = '%s'\n", k[1] );
                g_strfreev(k);
		p->cleanup ();
		XSM_DEBUG (DBG_L3, "Cleanup OK, closing module: " << p->filename);
	}
	if (p){
		g_free (p->filename);
		g_module_close (p->module);
	}
	XSM_DEBUG (DBG_L3, "Closed.");
}


void plugin_ctrl::view_pi_info(){
	GList *node = plugins;
	time_t t;
	time (&t);

        GtkFileFilter *f1 = gtk_file_filter_new ();
        gtk_file_filter_set_name (f1, "All");
        gtk_file_filter_add_pattern (f1, "*");

        GtkFileFilter *f2 = gtk_file_filter_new ();
        gtk_file_filter_set_name (f2, "HTML");
        gtk_file_filter_add_pattern (f2, "*.html");
        gtk_file_filter_add_pattern (f2, "*.htm");
        gtk_file_filter_add_pattern (f2, "*.HTM");
        gtk_file_filter_add_pattern (f2, "*.HTML");

        GtkFileFilter *filter[] = { f1, f2, NULL };

	gchar *pi_status_file = main_get_gapp ()->file_dialog_load ("HTML PlugIn status file to save", ".", 
                                                        "/tmp/gxsm_plugins.html",
                                                        filter
                                                        );
	if (!pi_status_file) return;
	std::ofstream html_file;
	html_file.open (pi_status_file);
	html_file << "<H2>GXSM - List of Plug-Ins</H2>" << std::endl
		  << "Dump of all GXSM Plug-In Information Structure Data, "
		  << "generated by \"GXSM->Tools->Plugin Info\"." << std::endl
		  << "See also the GXSM manual for a complete PlugIn documentation." << std::endl
		  << "<p>Table of all GXSM Plug-Ins:" << std::endl
		  << "<p>" << std::endl;

	html_file << "<table cellspacing=\"1\" cellpadding=\"5\" width=\"100%\" border=\"0\" bgcolor=\"#FAFCFF\">" << std::endl;
#define PLUGIN_NAME_TR_TAG_HTML "<tr valign=top BGCOLOR=\"#CACCCF\">"
#define PLUGIN_INFO_TR_TAG_HTML "<tr valign=top BGCOLOR=\"#EAECEF\">"
	int count=0;
	while (node){
		GxsmPlugin *p;
		html_file << PLUGIN_NAME_TR_TAG_HTML << std::endl;
		p = (GxsmPlugin *) node->data;
		if (p){
			html_file << "<td>" << ++count << "</td><td> Plugin \"" << p->name << "\"</td>" << std::endl;
		} else { 
			html_file << "<td>Error:</td><td> no PlugIn info data available for" 
			     << p->filename << "</td>" << std::endl; 
		}
		html_file << "</tr>" << std::endl;
		if (p && p->description && p->menupath)
		{
			html_file << PLUGIN_INFO_TR_TAG_HTML << std::endl
			     << "<td>Pluginpath</td><td>" << p->filename << "</td></tr>" << std::endl
			     << PLUGIN_INFO_TR_TAG_HTML << std::endl
			     << "<td>Name</td><td>" << p->name << "</td></tr>" << std::endl
			     << PLUGIN_INFO_TR_TAG_HTML << std::endl
			     << "<td>Description</td><td>" << p->description << "</td></tr>" << std::endl
			     << PLUGIN_INFO_TR_TAG_HTML << std::endl
			     << "<td>Categorie</td><td>" << (p->category ? p->category:N_("(NULL): All")) << "</td></tr>" << std::endl
			     << PLUGIN_INFO_TR_TAG_HTML << std::endl
			     << "<td>Author</td><td>" << p->authors << "</td></tr>" << std::endl
			     << PLUGIN_INFO_TR_TAG_HTML << std::endl
			     << "<td>Menupath</td><td>" << p->menupath << p->menuentry << "</td></tr>" << std::endl
			     << PLUGIN_INFO_TR_TAG_HTML << std::endl
			     << "<td>Help  </td><td>" << (p->help     ? p->help:"--") << "</td></tr>" << std::endl
			     << PLUGIN_INFO_TR_TAG_HTML << std::endl
			     << "<td>Info  </td><td>" << (p->info     ? p->info:"--") << "</td></tr>" << std::endl
			     << PLUGIN_INFO_TR_TAG_HTML << std::endl
			     << "<td>Error </td><td>" << (p->errormsg ? p->errormsg:N_("--")) << "</td></tr>" << std::endl
			     << PLUGIN_INFO_TR_TAG_HTML << std::endl
			     << "<td>Status</td><td>" << (p->status   ? p->status:N_("--")) << "</td></tr>" << std::endl
			     << PLUGIN_INFO_TR_TAG_HTML << std::endl
			     << "<td>Init-F     </td><td>" << (p->init?     N_("Yes"):N_("No")) << "</td></tr>" << std::endl
			     << PLUGIN_INFO_TR_TAG_HTML << std::endl
			     << "<td>Query-F    </td><td>" << (p->query?    N_("Yes"):N_("No")) << "</td></tr>" << std::endl
			     << PLUGIN_INFO_TR_TAG_HTML << std::endl
			     << "<td>About-F    </td><td>" << (p->about?    N_("Yes"):N_("No")) << "</td></tr>" << std::endl
			     << PLUGIN_INFO_TR_TAG_HTML << std::endl
			     << "<td>Configure-F</td><td>" << (p->configure?N_("Yes"):N_("No")) << "</td></tr>" << std::endl
			     << PLUGIN_INFO_TR_TAG_HTML << std::endl
			     << "<td>Run-F      </td><td>" << (p->run?      N_("Yes"):N_("No")) << "</td></tr>" << std::endl
			     << PLUGIN_INFO_TR_TAG_HTML << std::endl
			     << "<td>Cleanup-F  </td><td>" << (p->cleanup?  N_("Yes"):N_("No")) << "</td></tr>" << std::endl
				;
		}
		else
			html_file << PLUGIN_INFO_TR_TAG_HTML << std::endl
			     << "<td>Sorry</td><td>Plugin has no description/menupath!</td></tr>" << std::endl;

		node = node->next;
	}

	html_file << "</table>" << std::endl
	     << "<p>" << std::endl
	     << "Last Updated: " << ctime(&t) << "<p>" << std::endl;
	html_file.close ();
}


static void app_auto_hookup_menu_to_plugin_callback (App *app, const gchar *menusection, const gchar* label, GCallback gc, gpointer user_data=NULL){

        //g_message (" app_auto_hookup_menu_to_plugin_callback");

        if (!menusection) return;
        if (!label) return;
        if (!gc) return;

        // generate valid action string from menu path
        gchar *tmp = g_strconcat (menusection, "-", label, NULL);
        g_strdelimit (tmp, " /:", '-');
        gchar *tmpaction = g_strconcat ( tmp, NULL);
        GSimpleAction *ti_action;
        
        XSM_DEBUG_GM (DBG_L1, "app_auto_hookup_menu_to_plugin_callback: Plugin Menu Path requested <%s>  MenuItem= %s ==> label, generated action: [%s] <%s>",
                      menusection, label, tmp, tmpaction);

        if (!strcmp (menusection, "windows-section")){
                XSM_DEBUG_GP (DBG_L1, "app_auto_hookup_menu_to_plugin_callback: *** Skipping Section Windows-Menu. Handled by AppBase automatically.");
        } else {
                ti_action = g_simple_action_new (tmpaction, NULL);
                g_signal_connect (ti_action, "activate", gc, GTK_APPLICATION ( app->get_application ()));
                
                if (user_data)
                        g_object_set_data (G_OBJECT (ti_action), "plugin_MOp", user_data);

                g_action_map_add_action (G_ACTION_MAP ( app->get_application ()), G_ACTION (ti_action));

                gchar *app_tmpaction = g_strconcat ( "app.", tmpaction, NULL);

                // attach to menu, fallback to plugins menu
                main_get_gapp ()->gxsm_app_extend_menu (menusection, label, app_tmpaction);
                g_free (app_tmpaction);
        }
        
        g_free (tmpaction);
        g_free (tmp);
}

// ----------------------------------------------------------------------
//
// Gxsm Plugins -- math, control, etc.
//
// ----------------------------------------------------------------------

// gcc3.3: _Z33get_gxsm_math_...v
//           ^^ symbol name length

#define GXSM_PI_MATH_TYPE_MANGLE_NAME(NN,symbol) GXSM_PI_VOID_PREFIX "" NN "get_gxsm_math_" symbol "_plugin_info" GXSM_PI_VOID_SUFFIX


gxsm_plugins::gxsm_plugins(App *app, GList *pi_dirlist, gint (*check)(const gchar *), const gchar* splash_info)
	: plugin_ctrl(pi_dirlist, check, splash_info){
	GList *node;
	GxsmPlugin *p;
	void *(*gpi) (void);

	typedef struct { gint type; gchar *label, *hint; gpointer moreinfo, user_data; } GXSM_MENU_INFO;
	static GXSM_MENU_INFO menuinfo[] = {{ 1, NULL, NULL, NULL, NULL }, { 0, NULL, NULL, NULL, NULL }};


	// Init Gxsm Plugins, attach to menu
	XSM_DEBUG (DBG_L3, "Configuring GXSM Plugins");
	node = plugins;

	while(node) {
		p = (GxsmPlugin *) node->data;
		// Set Application pointer
		p->app = app;
		XSM_DEBUG (DBG_L3, "Setting up menus, ...: " << p->name);

		// if plugins query is defined, its called to install itself, 
		// it can set p->status to anything non zero to force proceeding with the normal menu configuration procedure!
		if( p->query ){
                        XSM_DEBUG (DBG_L3, "Plugin: " << p->name << "->query -- install. ");
                        gchar **k=g_strsplit_set (p->filename,"|",2);
                        pcs_set_current_gschema_group (k[1]);
                        XSM_DEBUG_GP (DBG_L2, "plugin_ctrl::init_pi -- SET_PCS_GROUP = '%s'\n", k[1] );
                        g_strfreev(k);
			p->query ();
		}

                if (p->status)
                        XSM_DEBUG (DBG_L3, "Special Plugin Query instruction: " << p->status);

                if (p->menu_callback1 && p->menupath && p->menuentry){
                        gchar **menupath = g_strsplit (p -> menupath, ",", 2);
                        gchar **label = g_strsplit (p -> menuentry, ",", 2);
                        
                        if (menupath[0] && label[0]){
                                XSM_DEBUG (DBG_L3, ": menu_callback1 found, hooking up at: " << menupath[0] << " -> " << label[0]);

                                app_auto_hookup_menu_to_plugin_callback (app, menupath[0], label[0], G_CALLBACK (p->menu_callback1));

                                if (p->menu_callback2){
                                        if (menupath[1] && label[1]){
                                                XSM_DEBUG (DBG_L3, ": menu_callback2 found, hooking up at: " << menupath[1] << " -> " << label[1]);
                                                app_auto_hookup_menu_to_plugin_callback (app, menupath[1], label[1], G_CALLBACK (p->menu_callback2));
                                        }
                                } else
                                        XSM_DEBUG_GP (DBG_L2, "ERROR in Plugin %s Menu Path description for menu_callback2 %s %s\n", p->name, menupath[1], label[1]);
                        } else
                                XSM_DEBUG_GP (DBG_L2, "ERROR in Plugin %s Menu Path description for menu_callback1 %s %s\n", p->name, menupath[0], label[0]);

                        g_strfreev (label);
                        g_strfreev (menupath);

                }else if (g_module_symbol (p->module, "_Z34get_gxsm_plugin_menu_callback_list", (gpointer*)&gpi)
                          && p->menupath && p->menuentry) {

                        GxsmPluginMenuCallbackList *callback_list = (GxsmPluginMenuCallbackList *) gpi();
                        PIMenuCallback *menu_callback_list = callback_list->menu_callback_list;
                        gchar **menupath = g_strsplit (p -> menupath, ",", 0);
                        gchar **label = g_strsplit (p -> menuentry, ",", 0);
                        
                        for (int i=0; i < callback_list->n && *menu_callback_list && menupath[i] && label[i]; ++i, ++menu_callback_list){
                                XSM_DEBUG (DBG_L3, ": menu_callback expansion list found, hooking up cb#" << i << " at: " << menupath[i] << " -> " << label[i]);
                                app_auto_hookup_menu_to_plugin_callback (app, menupath[i], label[i], G_CALLBACK (*menu_callback_list));
                        }
                        
                        g_strfreev (label);
                        g_strfreev (menupath);

                } else if( !p->query || p->status){ // MODIFIED was if( !p->query || p->status)

                        if (p->menupath && p->menuentry ){
				menuinfo[0].label  = g_strdup (p -> menuentry);
				menuinfo[0].hint   = g_strdup (p -> help);
	
				if(p->run){
					menuinfo[0].moreinfo  = (gpointer) p -> run;
					menuinfo[0].user_data = g_strdup (p -> name);
				} else {
					// check for special Gxsm plugins
					if (g_module_symbol (p->module, GXSM_PI_MATH_TYPE_MANGLE_NAME("33","one_src"), (gpointer*)&gpi)){
						menuinfo[0].moreinfo  = (gpointer) App::math_onearg_callback;
						menuinfo[0].user_data = (gpointer) ((GxsmMathOneSrcPlugin *) gpi()) -> run;
					} else
                                                if (g_module_symbol (p->module, GXSM_PI_MATH_TYPE_MANGLE_NAME("41","no_src_one_dest"), (gpointer*)&gpi)){
                                                        menuinfo[0].moreinfo  = (gpointer) App::math_onearg_dest_only_callback;
                                                        menuinfo[0].user_data = (gpointer) ((GxsmMathNoSrcOneDestPlugin *) gpi()) -> run;
                                                } else
                                                        if (g_module_symbol (p->module, GXSM_PI_MATH_TYPE_MANGLE_NAME("44","one_src_for_all_vt"), (gpointer*)&gpi)){
                                                                menuinfo[0].moreinfo  = (gpointer) App::math_onearg_for_all_vt_callback;
                                                                menuinfo[0].user_data = (gpointer) ((GxsmMathOneSrcPlugin *) gpi()) -> run;
                                                        } else
                                                                if (g_module_symbol (p->module, GXSM_PI_MATH_TYPE_MANGLE_NAME("33","two_src"), (gpointer*)&gpi)){
                                                                        menuinfo[0].moreinfo  = (gpointer) App::math_twoarg_callback;
                                                                        menuinfo[0].user_data = (gpointer) ((GxsmMathTwoSrcPlugin *) gpi()) -> run;
                                                                } else
                                                                        if (g_module_symbol (p->module, GXSM_PI_MATH_TYPE_MANGLE_NAME("52","two_src_no_same_size_check"), (gpointer*)&gpi)){
                                                                                menuinfo[0].moreinfo  = (gpointer) App::math_twoarg_no_same_size_check_callback;
                                                                                menuinfo[0].user_data = (gpointer) ((GxsmMathTwoSrcPlugin *) gpi()) -> run;
                                                                        } else
                                                                                if (g_module_symbol (p->module, GXSM_PI_MATH_TYPE_MANGLE_NAME("41","one_src_no_dest"), (gpointer*)&gpi)){
                                                                                        menuinfo[0].moreinfo  = (gpointer) App::math_onearg_nodest_callback;
                                                                                        menuinfo[0].user_data = (gpointer) ((GxsmMathOneSrcNoDestPlugin *) gpi()) -> run;
                                                                                } else {
                                                                                        XSM_DEBUG (DBG_L3, "Bad Plugin Descriptor found/type unknown: " << p->name);
                                                                                        menuinfo[0].moreinfo = NULL;
                                                                                        menuinfo[0].user_data = NULL;
                                                                                        if(p->status) g_free(p->status);
                                                                                        p->status = g_strconcat(N_("Type of Plugin with name "),
                                                                                                                p->name, 
                                                                                                                N_(" is not a recognized math plugin and may not be usable yet, as it depends on the Query function."),
                                                                                                                NULL);
                                                                                }
				}

                                if( menuinfo[0].moreinfo ){
                                        app_auto_hookup_menu_to_plugin_callback (app, p->menupath, menuinfo[0].label, G_CALLBACK (menuinfo[0].moreinfo), menuinfo[0].user_data);
					if (p->status) g_free(p->status);
					p->status = g_strconcat(N_("Plugin "),
								p->name, 
								N_(" is attached and ready to use"),
								NULL);

                                }
			}
		}
		node = node->next;
	}
}

gxsm_plugins::~gxsm_plugins(){
	GList *node;
	GxsmPlugin *p;

	// Remove Gxsm Plugins from menu
	XSM_DEBUG (DBG_L3, "Removing GXSM Plugins");

	for(node = plugins; node; node = node->next){
		if( !( p = (GxsmPlugin *) node->data ))
			continue;
		XSM_DEBUG (DBG_L3, "Removing: " << p->name);
		// free allocated infostrings here !
		if( p->status ){ 
			g_free( p->status );
			p->status = NULL;
		}
		if( p->description ){
			g_free( p->description ); 
			p->description = NULL;
		}
		if( p->menupath && p->menuentry ){
			gchar *mp = g_strconcat(p->menupath, p->menuentry, NULL);
#if 0
			gpointer dummy;
        // GTK3QQQ
			// check for special Gxsm plugins
			if ( p->run ||
			     g_module_symbol (p->module, GXSM_PI_MATH_TYPE_MANGLE_NAME("41","one_src_no_dest"), &dummy) ||
			     g_module_symbol (p->module, GXSM_PI_MATH_TYPE_MANGLE_NAME("33","one_src"), &dummy) ||
			     g_module_symbol (p->module, GXSM_PI_MATH_TYPE_MANGLE_NAME("52","two_src_no_same_size_check"), &dummy) ||
			     g_module_symbol (p->module, GXSM_PI_MATH_TYPE_MANGLE_NAME("33","two_src"), &dummy) )

				gnome_app_remove_menus (GNOME_APP( p->app->getApp() ),
							(const gchar*) mp, 1);
#endif
			if(mp) g_free(mp);
		}
	}
}



// ----------------------------------------------------------------------
//
// Gxsm HwI Plugins -- Harfware Interface (HwI) Plugins
//
// ----------------------------------------------------------------------


#define GXSM_HWI_TYPE_MANGLE_NAME GXSM_PI_VOID_PREFIX "27" "get_gxsm_hwi_hardware_class" GXSM_PI_VOIDP_SUFFIX

gxsm_hwi_plugins::gxsm_hwi_plugins (GList *pi_dirlist, gint (*check)(const gchar *), gchar *fulltype, App* app, const gchar* splash_info)
	: plugin_ctrl(pi_dirlist, check, splash_info){
	GList *node;
	GxsmPlugin *p;
	void *(*gpi) (void *);
	
	xsm_hwi_class = NULL;	

	// Init Gxsm Plugins, attach to menu
	XSM_HWI_DEBUG (DBG_L3, "Setting up GXSM HwI Plugin(s)");

	main_get_gapp ()->GxsmSplash (-0.1, "Setting up GXSM HwI Plugin(s)", " ... ");

	XSM_DEBUG_GP (DBG_L2, "gxsm_hwi_plugins::gxsm_hwi_plugins -- Setting up GXSM HwI Plugin(s)\n");
	node = plugins;
	if (!node){
		XSM_HWI_DEBUG (DBG_L3, "No GXSM HwI Plugin(s) found!!");
                XSM_DEBUG_GP (DBG_L2, "gxsm_hwi_plugins::gxsm_hwi_plugins -- No GXSM HwI Plugin(s) found!!\n");
	}
	while(node){
		p = (GxsmPlugin *) node->data;
		p->app = app; // referenced for HwI PIs (to attach menuentries, etc.)
		XSM_HWI_DEBUG (DBG_L3, "Asking HwI-PI for XSM_Hardware class reference :: " << p->name);
                XSM_DEBUG_GP (DBG_L2, "gxsm_hwi_plugins::gxsm_hwi_plugins -- Asking HwI-PI for XSM_Hardware class reference :: %s\n", p->name);

                main_get_gapp ()->GxsmSplash (-0.1, "Setting up GXSM HwI Plugin(s), checking:", p->name);

		if (g_module_symbol (p->module, GXSM_HWI_TYPE_MANGLE_NAME, (gpointer*)&gpi)){
			if (p->status) g_free(p->status);
			if ((xsm_hwi_class = (XSM_Hardware*) gpi ((void*) fulltype)))
				p->status = g_strconcat ("HwI <", fulltype ? fulltype:"NULL", "> is selected.", NULL);
			else
				p->status = g_strconcat ("HwI class request error: <", fulltype ? fulltype:"NULL", "> not available.", NULL);
			XSM_HWI_DEBUG (DBG_L3, "Status: '" << p->status << "' :: " << p->name);
		}

		// if plugins query is defined, its called to install itself
		XSM_HWI_DEBUG (DBG_L3, "Looking for Query/Selfinstall of HwI PI :: " << p->name);
		if( p->query ){
                        XSM_DEBUG (DBG_L3, "Plugin: " << p->name << "->query -- install. ");
                        gchar **k=g_strsplit_set (p->filename,"|",2);
                        pcs_set_current_gschema_group (k[1]);
                        XSM_DEBUG_GP (DBG_L2, "plugin_ctrl::init_pi -- SET_PCS_GROUP = '%s'\n", k[1] );
                        g_strfreev(k);

			p->query();
		}
		else{
			XSM_HWI_DEBUG (DBG_L3, "No Query/Selfinstall func. of HwI PI :: " << p->name);
		}

		XSM_HWI_DEBUG (DBG_L2, " loaded HwI PI (OK) :: " << p->name);
		node = node->next;
		if (node){
			p = (GxsmPlugin *) node->data;
			XSM_HWI_DEBUG_ERROR (DBG_L3, "Also attempt to load HwI PI :: " << p->name);
			break;
		}
	}
}


gxsm_hwi_plugins::~gxsm_hwi_plugins(){
	GList *node;
	GxsmPlugin *p;

	// Remove Gxsm Plugins from menu
	XSM_HWI_DEBUG (DBG_L3, "Removing GXSM HwI Plugins");

	xsm_hwi_class = NULL;	

	if (!plugins) return;

	for(node = plugins; node; node = node->next){
		if( !( p = (GxsmPlugin *) node->data ))
			continue;
		XSM_HWI_DEBUG (DBG_L3, "Removing HwI PI: " << p->name);
		// free allocated infostrings here !
		if( p->status ){ 
			g_free( p->status );
			p->status = NULL;
		}
		if( p->description ){
			g_free( p->description ); 
			p->description = NULL;
		}
	}
}

XSM_Hardware* gxsm_hwi_plugins::get_xsm_hwi_class (gchar *hwi_sub_type)
{ 
	return xsm_hwi_class; 
}
