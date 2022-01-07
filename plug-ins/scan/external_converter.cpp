/* Gnome gxsm - Gnome X Scanning Microscopy
 * universal STM/AFM/SARLS/SPALEED/... controlling and
 * data analysis software
 * 
 * Gxsm Plugin Name: external_converter.C
 * ========================================
 * 
 * Copyright (C) 2006 The Free Software Foundation
 *
 * Authors: Thorsten Wagner <stm@users.sf.net>
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

/* Please do not change the Begin/End lines of this comment section!
 * this is a LaTeX style section used for auto generation of the PlugIn Manual 
 * Chapter. Add a complete PlugIn documentation inbetween the Begin/End marks!
 * --------------------------------------------------------------------------------
% BeginPlugInDocuSection
% PlugInDocuCaption: external converter 
% PlugInName: external_converter
% PlugInAuthor: Thorsten Wagner
% PlugInAuthorEmail: stm@users.sf.net
% PlugInMenuPath: Tools/external_converter 
% PlugInDescription
Simple plugin to call an external converter
% PlugInUsage
Registers itself. Select source file,a destination folder, a suitable
suffix, the external converter (full path) and press okey. You can
also path some options to the external program
% EndPlugInDocuSection
 * -------------------------------------------------------------------------------- 
 */

#include "external_converter.h"
#include <dirent.h>
#include <fnmatch.h>
#define UMASK (S_IRUSR | S_IWUSR | S_IXUSR )

using namespace std;

// Plugin Prototypes
static void external_converter_init(void);
static void external_converter_query(void);
static void external_converter_about(void);
static void external_converter_configure(void);
static void external_converter_cleanup(void);

static void external_convert_callback(GSimpleAction * simple,
				      GVariant * parameter,
				      gpointer user_data);


//
// Gxsm plugin discription
//
GxsmPlugin external_converter_pi = {
    NULL,			// filled in and used by Gxsm, don't touch !
    NULL,			// filled in and used by Gxsm, don't touch !
    0,				// filled in and used by Gxsm, don't touch !
    NULL,			// filled in just after init() is called !!!
    "external_converter",
    NULL,
    g_strdup("Calls an external converter"),
    "Thorsten Wagner",
    "tools-section",
    N_("External Converter"),
    NULL,
    "No further info",
    NULL,
    NULL,
    external_converter_init,
    external_converter_query,
    external_converter_about,
    external_converter_configure,
    NULL,
    external_convert_callback,
    NULL,
    external_converter_cleanup
};

//
//Text used in the About Box
//
static const char *about_text = N_("Tool to call an external converter.");


//
// Symbol "get_gxsm_plugin_info" is resolved by dlsym from Gxsm, used to get Plugin's info!! 
// Essential Plugin Function!!
//
GxsmPlugin *get_gxsm_plugin_info(void)
{
    external_converter_pi.description =
	g_strdup_printf(N_
			("Tool to call an external converter. Version: %s"),
			VERSION);
    return &external_converter_pi;
}

//
// Query Function, installs Plugin's in the apropriate menupath
// !!!! make sure the "external_converter_cleanup()" function (see below) !!!!
// !!!! removes the correct menuentries !!!!
//
static void external_converter_query(void)
{
    if (external_converter_pi.status)
	g_free(external_converter_pi.status);
    external_converter_pi.status =
	g_strconcat(N_("Plugin query has attached "),
		    external_converter_pi.name,
		    N_(": File external_converter is ready to use"), NULL);
}

//
// init-Function
//

static void external_converter_init(void)
{
    PI_DEBUG(DBG_L2, "external_converter Plugin Init");
}

//
// about-Function
//
static void external_converter_about(void)
{
    const gchar *authors[] = { external_converter_pi.authors, NULL };
    gtk_show_about_dialog(NULL,
			  "program-name", external_converter_pi.name,
			  "version", VERSION,
			  N_("(C) 2006 the Free Software Foundation"),
			  "comments", about_text,
			  "authors", authors, NULL, NULL, NULL);
}

//
// configure-Function
//
static void external_converter_configure(void)
{
    if (external_converter_pi.app)
	external_converter_pi.
	    app->message("External Converter Plugin Configuration");
}

//
// cleanup-Function, make sure the Menustrings are matching those above!!!
//
static void external_converter_cleanup(void)
{
    PI_DEBUG(DBG_L2, "External Converter Plugin Cleanup");
#if 0
    gnome_app_remove_menus(GNOME_APP(external_converter_pi.app->getApp()),
			   N_("Tools/External Converter"), 1);
#endif
}


//Gtk+ Signal Funktion
static void external_convert_callback(GSimpleAction * simple,
				      GVariant * parameter,
				      gpointer user_data)
{
    external_converter_Control *Popup = new external_converter_Control();
    PI_DEBUG(DBG_L2, "External Converter dialog will open");
    Popup->run();
    PI_DEBUG(DBG_L2, "External Converter dialog has returned");
}


////////////////////////////////////////
// ANFANG external_converter
///////////////////////////////////////

gchar *select_mask;

external_converter::external_converter():m_converted(0)
{
    PI_DEBUG(DBG_L2, "Generating converter object!");
}


void external_converter::concatenate_dirs(gchar * target,
					  const gchar * add)
{	
	/** assure that the directories are delimited with / */
    int len = strlen(target);
    if (len > 0 && target[len - 1] != '/')
	strcat(target, "/");
    if (strncmp(target,add,(strlen(target)<strlen(add)?strlen(target):strlen(add))-1)!=0)
    	strcat(target, add);
return;
}


void external_converter::create_full_path(gchar * target,
					  const gchar * source_directory,
					  const gchar * current_dir,
					  const gchar * file)
{
    if (source_directory)
	if (strncmp(source_directory, "file://",7)==0)
		sprintf(target, "%s",source_directory+7);
        else	
		sprintf(target, "%s",source_directory);
    else
	*target = '\0';

    if (current_dir)
	concatenate_dirs(target, current_dir);

    if (file)
	concatenate_dirs(target, file);
    PI_DEBUG(DBG_L2, "target is now:"<<target);
return;
}


void external_converter::replace_suffix(gchar * target, gchar * new_suffix)
{
    PI_DEBUG(DBG_L2, "replace_suffix");
    int len = strlen(target);

    for (int i = len - 1; i > -1; --i) {
	if (target[i] == EXT_SEP) {
	    strcpy(target + i + 1, new_suffix);
	    return;
	}
    }
}



void external_converter::ConvertDir(external_converter_Data * work_it,
				    const gchar * current_dir)
{
    PI_DEBUG(DBG_L2, "ConvertDir:"<<current_dir);
    gchar source_dir[MAX_PATH];

    create_full_path(source_dir, work_it->sourceDir, NULL, NULL);

    DIR *dir = opendir(source_dir);
    if (dir) {
	struct stat file_stat;
	dirent *current_file;
	gchar source_name[MAX_PATH];
	gchar target_name[MAX_PATH];

	while ((current_file = readdir(dir))) {
	    create_full_path(source_name, work_it->sourceDir, current_dir,
			     current_file->d_name);
/*
	    if (lstat(source_name, &file_stat) != -1) {
		// go recursively in case of directories
		if (S_ISDIR(file_stat.st_mode)
		    && *current_file->d_name != '.') {
		    if (work_it->m_recursive) {
			PI_DEBUG(DBG_L2,"enter subdir");
			if (work_it->m_create_subdirs) {
				PI_DEBUG(DBG_L2,"create subdir");
			    create_full_path(target_name, work_it->destDir,
					     current_dir,
					     current_file->d_name);
			    //      if (lstat(target_name, &file_stat) != -1
			    //              && mkdir(target_name, S_IRUSR | S_IWUSR
			    //                       | S_IXUSR | S_IRGRP | S_IWGRP | S_IXGRP)
			    //                       == -1)
			    if (strcmp(source_name, work_it->destDir))
				if (mkdir(target_name, S_IRUSR | S_IWUSR
					  | S_IXUSR | S_IRGRP | S_IWGRP |
					  S_IXGRP)
				    == -1) {
				    PI_DEBUG(DBG_L2,"Could not create directory "<<target_name);
				} else
				    PI_DEBUG(DBG_L2,"Create directory "<< target_name);
			}
			create_full_path(source_name, 0, current_dir,
					 current_file->d_name);

			ConvertDir(work_it, source_name);
		    }
		    // otherwise check whether the file matches the mask
		    // Note: S_ISREG didn't seem to work correctly on my
		    // system - otherwise I would use it as well
		} else
		    if (!fnmatch
			(work_it->convBin, current_file->d_name, 0)) {
		    create_full_path(source_name, work_it->sourceDir,
				     current_dir, current_file->d_name);
		    create_full_path(target_name, work_it->destDir,
				     work_it->m_create_subdirs ?
				     current_dir : 0,
				     current_file->d_name);
			PI_DEBUG(DBG_L2,"just testing");
		    replace_suffix(target_name, work_it->writeFormat);

		    if (!work_it->m_overwrite_target
			&& lstat(target_name, &file_stat) != -1) {
			PI_DEBUG(DBG_L2,target_name << " already exists!");
			continue;
		    }

		    ++m_converted;
		    PI_DEBUG(DBG_L2,"Converting file " << m_converted);
		    char command[1000];
		    snprintf(command, 1000, "%s %s \"%s\" \"%s\"",
			     work_it->convBin,
			     work_it->converterOptions, source_name,
			     target_name);
		    PI_DEBUG(DBG_L2,"command: "<<command);
		    int errornum = 0;
		    errornum = system(command);
		    if (errornum != 0)
			PI_DEBUG(DBG_L2,"Error! "<<errornum);
		}
	    }*/
	}
    }
PI_DEBUG(DBG_L2,"dir done");
    closedir(dir);
return;
}




////////////////////////////////////////
// ENDE external_converter
///////////////////////////////////////

////////////////////////////////////////
// ANFANG external_converter_Control
///////////////////////////////////////

gint file_error(GtkWindow * window, const gchar * err_msg,
		const gchar * dir);

external_converter_Control::external_converter_Control()
{
    PI_DEBUG(DBG_L2, "Generating external_converter_control object!");
    frontenddata =
	new external_converter_Data(getenv("PWD"), getenv("PWD"), "*.nc",
				    "top");
    XsmRescourceManager xrm("FilePathSelectionHistory",
			    "external_converter");
    xrm.Get("SourceFile", &frontenddata->sourceFile, getenv("PWD"));
    xrm.Get("DestFile", &frontenddata->destFile, getenv("PWD"));
    xrm.Get("DestSuffix", &frontenddata->writeFormat, ".top");
    xrm.Get("convBin", &frontenddata->convBin, "/usr/local/bin/nc2top");
    xrm.Get("ConverterOptions", &frontenddata->converterOptions, "-v");
}


external_converter_Control::~external_converter_Control()
{
    PI_DEBUG(DBG_L2, "Deleting external_converter_control object!");
    delete frontenddata;
}


void external_converter_Control::run()
{
    PI_DEBUG(DBG_L2, "run");
    GtkWidget *dialog;
    GtkWidget *VarName;
    GtkWidget *variable;
    GtkWidget *help;
    GtkWidget *hbox;
    GtkWidget *vbox;

    // create dialog window
    GtkDialogFlags flags = GTK_DIALOG_MODAL;
    dialog =
	gtk_dialog_new_with_buttons(N_("External Converter"), NULL, flags,
				    N_("Convert"), GTK_RESPONSE_ACCEPT,N_("Cancel"),GTK_RESPONSE_CANCEL,
				    NULL);

    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_box_pack_start(GTK_BOX
		       (gtk_dialog_get_content_area(GTK_DIALOG(dialog))),
		       vbox, TRUE, TRUE, GXSM_WIDGET_PAD);

    // Create entry for source file
    hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

    VarName = gtk_label_new(N_("Source Path"));
    gtk_label_set_justify(GTK_LABEL(VarName), GTK_JUSTIFY_LEFT);
    gtk_label_set_justify(GTK_LABEL(VarName), GTK_JUSTIFY_LEFT);
    gtk_widget_set_size_request(VarName, 100, -1);
    gtk_box_pack_start(GTK_BOX(hbox), VarName, TRUE, TRUE, 0);

    SourcePath =
	gtk_file_chooser_button_new("source folder",
				    GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER);
    gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(SourcePath),
					frontenddata->sourceFile);
    gtk_box_pack_start(GTK_BOX(hbox), GTK_WIDGET(SourcePath), TRUE, TRUE,
		       0);

    // Create widget for destination path
    hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

    VarName = gtk_label_new(N_("Destination Path"));
    gtk_label_set_justify(GTK_LABEL(VarName), GTK_JUSTIFY_LEFT);
    gtk_label_set_justify(GTK_LABEL(VarName), GTK_JUSTIFY_LEFT);
    gtk_widget_set_size_request(VarName, 100, -1);
    gtk_box_pack_start(GTK_BOX(hbox), VarName, TRUE, TRUE, 0);

    DestPath =
	gtk_file_chooser_button_new("destination folder",
				    GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER);
    gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(DestPath),
					frontenddata->destFile);
    gtk_box_pack_start(GTK_BOX(hbox), GTK_WIDGET(DestPath), TRUE, TRUE, 0);


    // Create entry for converter binary
    hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

    VarName = gtk_label_new(N_("Converter Path"));
    gtk_label_set_justify(GTK_LABEL(VarName), GTK_JUSTIFY_LEFT);
    gtk_widget_set_size_request(VarName, 100, -1);
    gtk_box_pack_start(GTK_BOX(hbox), VarName, TRUE, TRUE, 0);

    ConverterBin =
	gtk_file_chooser_button_new("external converter",
				    GTK_FILE_CHOOSER_ACTION_OPEN);
    // neet do fix that frontenddata->convBin is set as default file name !!!       
    //gtk_file_chooser_set_current_folder_file (GTK_FILE_CHOOSER(ConverterBin), "/user/bin");
    //gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER(ConverterBin), "nc2top");    
    gtk_file_chooser_set_file(GTK_FILE_CHOOSER(ConverterBin),
			      g_file_parse_name("/usr/local/bin/nc2top"), NULL);

    gtk_box_pack_start(GTK_BOX(hbox), GTK_WIDGET(ConverterBin), TRUE,
		       TRUE, 0);

    // Create widget for destination mask
    hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

    VarName = gtk_label_new(N_("Suffix"));
    gtk_label_set_justify(GTK_LABEL(VarName), GTK_JUSTIFY_LEFT);
    gtk_widget_set_size_request(VarName, 100, -1);
    gtk_box_pack_start(GTK_BOX(hbox), VarName, TRUE, TRUE, 0);

    DestSuffix = gtk_entry_new();
    gtk_box_pack_start(GTK_BOX(hbox), DestSuffix, TRUE, TRUE, 0);
    gtk_entry_set_text(GTK_ENTRY(DestSuffix), frontenddata->writeFormat);

    help = gtk_button_new_with_label(N_("Help"));
    gtk_box_pack_start(GTK_BOX(hbox), help, TRUE, TRUE, 0);
    g_signal_connect(G_OBJECT(help), "clicked",
		     G_CALLBACK(show_info_callback)),
		     (void
		      *) (N_
			  ("Enter the file format suffix without any special characters.")));


    g_signal_connect(G_OBJECT(dialog), "clicked",
		     G_CALLBACK(external_converter_Control::dlg_clicked),
		     this);

// Create widget for converter options
    hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

    VarName = gtk_label_new(N_("Converter Options"));
    gtk_label_set_justify(GTK_LABEL(VarName), GTK_JUSTIFY_LEFT);
    gtk_widget_set_size_request(VarName, 100, -1);
    gtk_box_pack_start(GTK_BOX(hbox), VarName, TRUE, TRUE, 0);

    ConverterOptions = gtk_entry_new();
    gtk_box_pack_start(GTK_BOX(hbox), ConverterOptions, TRUE, TRUE, 0);
    gtk_entry_set_text(GTK_ENTRY(ConverterOptions),
		       frontenddata->converterOptions);

    help = gtk_button_new_with_label(N_("Help"));
    gtk_box_pack_start(GTK_BOX(hbox), help, TRUE, TRUE, 0);
    g_signal_connect(G_OBJECT(help), "clicked",
		     G_CALLBACK(show_info_callback),
		     (void
		      *) (N_
			  ("Enter options for the external converter.")));


// Create checkboxes for various options
    hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

    GtkWidget *toggle_recursive =
	gtk_check_button_new_with_label("Recursive");
    gtk_box_pack_start(GTK_BOX(hbox), toggle_recursive, TRUE, TRUE, 0);
    g_signal_connect(G_OBJECT(toggle_recursive), "clicked",
		     G_CALLBACK
		     (external_converter_Control::recursive_click), this);

    GtkWidget *toggle_overwrite_target =
	gtk_check_button_new_with_label("Overwrite Target File");
    gtk_box_pack_start(GTK_BOX(hbox), toggle_overwrite_target, TRUE, TRUE,
		       0);
    g_signal_connect(G_OBJECT(toggle_overwrite_target), "clicked",
		     G_CALLBACK
		     (external_converter_Control::overwrite_target_click),
		     this);

    GtkWidget *toggle_create_subdirs =
	gtk_check_button_new_with_label("Create sub-directories");
    gtk_box_pack_start(GTK_BOX(hbox), toggle_create_subdirs, TRUE, TRUE,
		       0);
    g_signal_connect(G_OBJECT(toggle_create_subdirs), "clicked",
		     G_CALLBACK
		     (external_converter_Control::create_subdirs_click),
		     this);
    // show all widgets and start dialog
    gtk_widget_show_all(dialog);
    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_CANCEL) {
    PI_DEBUG(DBG_L2, "cancel was pressed, no conversion done.");
	gtk_widget_destroy(dialog);
	return;
    } else {
	PI_DEBUG(DBG_L2, "convert was pressed, now checking parameter");
// get data from widgets (after closing dialog window)
    g_free(frontenddata->sourceDir);
    frontenddata->sourceDir =
	g_strdup(gtk_file_chooser_get_uri(GTK_FILE_CHOOSER(SourcePath)));
/*    if (access(frontenddata->sourceDir, F_OK)) {
	gtk_widget_destroy(dialog);
	PI_DEBUG(DBG_L2, "failed to access source dir");
	return;
    }
*/

    g_free(frontenddata->destDir);
    frontenddata->destDir =
	g_strdup(gtk_file_chooser_get_uri(GTK_FILE_CHOOSER(DestPath)));
/*    if (access(frontenddata->destDir, F_OK)) {
	gtk_widget_destroy(dialog);
	PI_DEBUG(DBG_L2, "failed to access destination dir");
	return;
    }
*/
    g_free(frontenddata->writeFormat);
    frontenddata->writeFormat =
	g_strdup(gtk_entry_get_text(GTK_ENTRY(DestSuffix)));

    g_free(frontenddata->convBin);
    frontenddata->convBin =
	g_strdup(g_file_get_parse_name
		 (gtk_file_chooser_get_file
		  (GTK_FILE_CHOOSER(ConverterBin))));
/*    if (access(frontenddata->convBin, F_OK)) {
	gtk_widget_destroy(dialog);
	PI_DEBUG(DBG_L2, "failed to access converter binary");
	return;
    }
*/
    g_free(frontenddata->converterOptions);
    frontenddata->writeFormat =
	g_strdup(gtk_entry_get_text(GTK_ENTRY(ConverterOptions)));
    PI_DEBUG(DBG_L2, "parameter okay");
    external_converter converter_obj;
    converter_obj.ConvertDir(frontenddata,getenv("PWD"));	
    return;
}
}

void external_converter_Control::recursive_click(GtkWidget * widget,
						 gpointer userdata)
{
    PI_DEBUG(DBG_L2, "recursive_click");
    ((external_converter_Control *) userdata)->frontenddata->m_recursive =
	gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));
}


void external_converter_Control::overwrite_target_click(GtkWidget * widget,
							gpointer userdata)
{
    PI_DEBUG(DBG_L2, "overwrite_target_click");
    ((external_converter_Control *) userdata)->
	frontenddata->m_overwrite_target =
	gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));
}


void external_converter_Control::create_subdirs_click(GtkWidget * widget,
						      gpointer userdata)
{
    PI_DEBUG(DBG_L2, "create_subdirs_click");
    ((external_converter_Control *) userdata)->
	frontenddata->m_create_subdirs =
	gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));
}




void external_converter_Control::dlg_clicked(GtkDialog * dialog,
					     gint button_number,
					     external_converter_Control *
					     mic)
{
    PI_DEBUG(DBG_L2, "dlg_clicked");
    g_free(mic->frontenddata->sourceFile);
    mic->frontenddata->sourceFile =
	g_strdup(gtk_file_chooser_get_uri
		 (GTK_FILE_CHOOSER(mic->SourcePath)));
    if (mic->frontenddata->sourceFile == NULL && !button_number)
	if (file_error
	    (GTK_WINDOW(dialog), "Source file does not exist", NULL)) {
	    gtk_widget_destroy(GTK_WIDGET(dialog));
	    goto finish;
	}
    g_free(mic->frontenddata->destFile);
    mic->frontenddata->destFile =
	g_strdup(gtk_file_chooser_get_uri
		 (GTK_FILE_CHOOSER(mic->DestPath)));
    if (access(mic->frontenddata->destFile, F_OK) && !button_number)
	if (file_error
	    (GTK_WINDOW(dialog), "Destination Folder does not exist",
	     mic->frontenddata->destFile)) {
	    gtk_widget_destroy(GTK_WIDGET(dialog));
	    goto finish;
	}

    g_free(mic->frontenddata->writeFormat);
    mic->frontenddata->writeFormat =
	g_strdup(gtk_entry_get_text(GTK_ENTRY(mic->DestSuffix)));

    g_free(mic->frontenddata->convBin);
    mic->frontenddata->convBin =
	g_strdup(gtk_file_chooser_get_uri
		 (GTK_FILE_CHOOSER(mic->ConverterBin)));
    if (access(mic->frontenddata->convBin, F_OK) && !button_number)
	if (file_error
	    (GTK_WINDOW(dialog), "Converter does not exist", NULL)) {
	    gtk_widget_destroy(GTK_WIDGET(dialog));
	    goto finish;
	}

    g_free(mic->frontenddata->converterOptions);
    mic->frontenddata->converterOptions =
	g_strdup(gtk_entry_get_text(GTK_ENTRY(mic->ConverterOptions)));

    switch (button_number) {
    case 0:{
	    gtk_widget_destroy(GTK_WIDGET(dialog));
	    //show_info_callback(NULL, N_("Converting ..."));
	    struct stat file_stat;
	    int len = 0;
	    gchar source_name[MAX_PATH];

	    lstat(mic->frontenddata->sourceFile, &file_stat);


	    if (!S_ISDIR(file_stat.st_mode)) {
		//file
		char command[1000];
		len = strlen(mic->frontenddata->sourceFile);
		gchar *fname =
		    g_strndup(mic->frontenddata->sourceFile, 1000);
		gchar *buffer;
		while (buffer = strstr(fname, "/")) {
		    fname = g_strndup(buffer + 1, 1000);
		}
		buffer =
		    g_strndup(fname,
			      strlen(fname) - strlen(strrchr(fname, '.')));
		//printf("Buffer %s fname %s\n",buffer,fname);                          
		len = strlen(mic->frontenddata->destFile);
		// make sure that the directory is with terminating '/' 
		if (len > 0) {
		    if (mic->frontenddata->destFile[len - 1] != '/')
			strcat(mic->frontenddata->destFile, "/");
		}

		PI_DEBUG(DBG_L2,"Starting conversion");
		//check if input file is a file or directory
		snprintf(command, 1000, "%s %s \"%s\" \"%s%s.%s\" &",
			 mic->frontenddata->convBin,
			 mic->frontenddata->converterOptions,
			 mic->frontenddata->sourceFile,
			 mic->frontenddata->destFile, buffer,
			 mic->frontenddata->writeFormat);
		PI_DEBUG(DBG_L2,"Command:%s\n"<<command);
		system(command);
	    } else {
		//directory
		mic->frontenddata->sourceDir =
		    mic->frontenddata->sourceFile;
		mic->frontenddata->destDir = mic->frontenddata->destFile;
		external_converter converter_obj;
		converter_obj.ConvertDir(mic->frontenddata, 0);
	    }
	    break;
	}

    case 1:{
	    gtk_widget_destroy(GTK_WIDGET(dialog));
	    break;
	}
    }
  finish:
    XsmRescourceManager xrm("FilePathSelectionHistory",
			    "external_converter");
    xrm.Put("SourceFile", mic->frontenddata->sourceFile);
    xrm.Put("DestFile", mic->frontenddata->destFile);
    xrm.Put("DestSuffix", mic->frontenddata->writeFormat);
    xrm.Put("convBin", mic->frontenddata->convBin);
    xrm.Put("ConverterOptions", mic->frontenddata->converterOptions);

    mic->DlgDone();
return;

}

////////////////////////////////////////
//ANFANG converterData
///////////////////////////////////////

external_converter_Data::external_converter_Data(const gchar * src,
						 const gchar * dst,
						 const gchar * conv,
						 const gchar *
						 write):m_recursive(false),
m_overwrite_target(false), m_create_subdirs(false)
{
    sourceDir = g_strdup(src);
    destDir = g_strdup(dst);
    sourceFile = g_strdup(src);
    destFile = g_strdup(dst);
    convBin = g_strdup(conv);
    writeFormat = g_strdup(write);
}

external_converter_Data::~external_converter_Data()
{
    g_free(sourceDir);
    g_free(destDir);
    g_free(convBin);
    g_free(writeFormat);
}

////////////////////////////////////////
// ENDE converterData
////////////////////////////////////////
