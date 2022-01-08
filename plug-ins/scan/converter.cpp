/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 8 c-style: "K&R" -*- */

/* Gnome gxsm - Gnome X Scanning Microscopy
 * universal STM/AFM/SARLS/SPALEED/... controlling and
 * data analysis software
 * 
 * Gxsm Plugin Name: converter.C
 * ========================================
 * 
 * Copyright (C) 2003 The Free Software Foundation
 *
 * Authors: Kristan Temme <etptt@users.sf.net>
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
% PlugInDocuCaption: Converter 
% PlugInName: Converter
% PlugInAuthor: Kristan Temme and Thorsten Wagner
% PlugInAuthorEmail: stm@users.sf.net
% PlugInMenuPath: Tools/Converter 
% PlugInDescription
Simple file conversion utility, which converts files all kinds of data files supported by GXSM-2
% PlugInUsage
Registers itself for converting files. Just choose the right file extensions for the conversion (e.g. .nc to .top). You can also select the source and destination folder by the browse buttons.
% EndPlugInDocuSection
 * -------------------------------------------------------------------------------- 
 */

#include "converter.h"

#define IMGMAXCOLORS 64
#define WSXM_MAXCHARS 1000
#define UMASK (S_IRUSR | S_IWUSR | S_IXUSR )

using namespace std;

// Plugin Prototypes
static void converter_init(void);
static void converter_query(void);
static void converter_about(void);
static void converter_configure(void);
static void converter_cleanup(void);

static void converter_convert_callback(GSimpleAction * simple,
				       GVariant * parameter,
				       gpointer user_data);


// Fill in the GxsmPlugin Description here
GxsmPlugin converter_pi = {
    NULL,			// filled in and used by Gxsm, don't touch !
    NULL,			// filled in and used by Gxsm, don't touch !
    0,				// filled in and used by Gxsm, don't touch !
    NULL,			// The Gxsm-App Class Ref.pointer (called "gapp" in Gxsm) is 
    // filled in here by Gxsm on Plugin load, 
    // just after init() is called !!!
    "Converter",
    NULL,			// PlugIn's Categorie, set to NULL for all, I just don't want this always to be loaded!
    // Description, is shown by PluginViewer (Plugin: listplugin, Tools->Plugin Details)
    NULL,
    "Kristan Temme and Thorsten Wagner",
    "tools-section",
    N_("Converter"),
    N_("Batch file conversion tool"),
    N_("N/A"),
    NULL,			// error msg, plugin may put error status msg here later
    NULL,			// Plugin Status, managed by Gxsm, plugin may manipulate it too
    converter_init,
    converter_query,
    // about-function, can be "NULL"
    // can be called by "Plugin Details"
    converter_about,
    // configure-function, can be "NULL"
    // can be called by "Plugin Details"
    converter_configure,
    // run-function, can be "NULL", if non-Zero and no query defined, 
    // it is called on menupath->"plugin"
    NULL,
    // cleanup-function, can be "NULL"
    // called if present at plugin removal
    converter_convert_callback,	// direct menu entry callback1 or NULL
    NULL,			// direct menu entry callback2 or NULL

    converter_cleanup
};


//
//Text used in the About Box
//
static const char *about_text = N_("Gxsm Converter for supported files.");


//
// Symbol "get_gxsm_plugin_info" is resolved by dlsym from Gxsm, used to get Plugin's info!! 
// Essential Plugin Function!!
//
GxsmPlugin *get_gxsm_plugin_info(void)
{
    converter_pi.description =
	g_strdup_printf(N_("Gxsm converter file converter %s"), VERSION);
    return &converter_pi;
}

//
// Query Function, installs Plugin's in the apropriate menupath
// !!!! make sure the "converter_cleanup()" function (see below) !!!!
// !!!! removes the correct menuentries !!!!
//
static void converter_query(void)
{
    if (converter_pi.status)
	g_free(converter_pi.status);
    converter_pi.status = g_strconcat(N_("Plugin query has attached "),
				      converter_pi.name,
				      N_
				      (": File converter is ready to use"),
				      NULL);
}

//
// init-Function
//

static void converter_init(void)
{
    PI_DEBUG(DBG_L2, "converter Plugin Init");
}

//
// about-Function
//
static void converter_about(void)
{
    const gchar *authors[] = { converter_pi.authors, NULL };
    gtk_show_about_dialog(NULL,
			  "program-name", converter_pi.name,
			  "version", VERSION,
			  "license", GTK_LICENSE_GPL_3_0,
			  "comments", about_text, "authors", authors,
			  NULL);
}

//
// configure-Function
//
static void converter_configure(void)
{
    if (converter_pi.app)
	converter_pi.app->message("converter Plugin Configuration");
}

//
// cleanup-Function, make sure the Menustrings are matching those above!!!
//
static void converter_cleanup(void)
{
    PI_DEBUG(DBG_L2, "converter Plugin Cleanup");
#if 0
    gnome_app_remove_menus(GNOME_APP(converter_pi.app->getApp()),
			   N_("Tools/Converter"), 1);
#endif
}


//Gtk+ Signal Funktion
static void
converter_convert_callback(GSimpleAction * simple, GVariant * parameter,
			   gpointer user_data)
{
    converterControl *Popup = new converterControl();
    PI_DEBUG(DBG_L2, "converter dialog will open");    
    Popup->run();
    PI_DEBUG(DBG_L2, "converter dialog has closed");
}


////////////////////////////////////////
// ANFANG converter
///////////////////////////////////////

gchar *select_mask;

converter::converter():m_converted(0)
{
    PI_DEBUG(DBG_L2, "Generating converter object!");
}


void converter::concatenate_dirs(gchar * target, const gchar * add)
{
    PI_DEBUG(DBG_L2, "concatenate dirs");
  /** assure that the directories are delimited with / */
    int len = strlen(target);
    if (len > 0 && target[len - 1] != '/')
	strcat(target, "/");
    strcat(target, add);
}


void converter::create_full_path(gchar * target,
				 const gchar * source_directory,
				 const gchar * current_dir,
				 const gchar * file)
{
    if (source_directory)
	strcpy(target, source_directory);
    else
	*target = '\0';

    if (current_dir)
	concatenate_dirs(target, current_dir);

    if (file)
	concatenate_dirs(target, file);
    PI_DEBUG(DBG_L2, g_strdup(target));
}


void converter::replace_suffix(gchar * target, gchar * new_suffix)
{
    int len = strlen(target);

    for (int i = len - 1; i > -1; --i) {
	if (target[i] == EXT_SEP) {
	    strcpy(target + i + 1, new_suffix);
	    return;
	}
    }
}


/** Convert all the data files in the given directory */
void converter::ConvertDir(converterData * work_it,
			   const gchar * current_dir)
{
    gchar source_dir[MAX_PATH];

    create_full_path(source_dir, work_it->sourceDir, current_dir, 0);

    DIR *dir = opendir(source_dir);

    if (dir) {
	struct stat file_stat;
	dirent *current_file;
	gchar source_name[MAX_PATH];
	gchar target_name[MAX_PATH];

	while ((current_file = readdir(dir))) {
	    create_full_path(source_name, work_it->sourceDir, current_dir,
			     current_file->d_name);
	    if (lstat(source_name, &file_stat) != -1) {
		// go recursively in case of directories
		if (S_ISDIR(file_stat.st_mode)
		    && *current_file->d_name != '.') {
		    if (work_it->m_recursive) {
			if (work_it->m_create_subdirs) {
			    create_full_path(target_name, work_it->destDir,
					     current_dir,
					     current_file->d_name);
			    //      if (lstat(target_name, &file_stat) != -1
			    //              && mkdir(target_name, S_IRUSR | S_IWUSR
			    //                       | S_IXUSR | S_IRGRP | S_IWGRP | S_IXGRP)
			    //                       == -1)
			    if (mkdir(target_name, S_IRUSR | S_IWUSR
				      | S_IXUSR | S_IRGRP | S_IWGRP |
				      S_IXGRP)
				== -1) {
				std::cout << "Could not create directory "
				    << target_name << std::endl;
			    } else
				std::cout << "Create directory "
				    << target_name << std::endl;
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
			(work_it->convFilter, current_file->d_name, 0)) {
		    create_full_path(source_name, work_it->sourceDir,
				     current_dir, current_file->d_name);
		    create_full_path(target_name, work_it->destDir,
				     work_it->m_create_subdirs ?
				     current_dir : 0,
				     current_file->d_name);
		    replace_suffix(target_name, work_it->writeFormat);

		    if (!work_it->m_overwrite_target
			&& lstat(target_name, &file_stat) != -1) {
			std::cout << target_name << " already exists!"
			    << std::endl;
			continue;
		    }

		    ++m_converted;
		    std::cout << "Converting file " << m_converted << ": "
			<< source_name << std::endl;
		    DoConvert(source_name, target_name);
		}
	    }
	}
    }

    closedir(dir);
}


void converter::DoConvert(gchar * pathname, gchar * outputname)
{
    if (main_get_gapp()->xsm->ActivateFreeChannel()) {
	PI_DEBUG(DBG_L2, "Nc2top Error: finding Free Channel");
	return;
    }

    gint Ch = main_get_gapp()->xsm->FindChan(ID_CH_M_ACTIVE);

    if ((Ch = readToAct(pathname)) < 0)
	PI_DEBUG(DBG_L2,
		 "readToAct: Conversion of " << pathname << "failed");

    else {
	if (!writeFromCh(Ch, outputname))
	    PI_DEBUG(DBG_L2,
		     "Converted: " << pathname << " to " << outputname);
	else
	    PI_DEBUG(DBG_L2,
		     "writeFromCh: Conversion of " << pathname <<
		     "failed");
    }

    main_get_gapp()->xsm->SetMode(Ch, ID_CH_M_OFF, TRUE);
}


gint converter::readToAct(gchar * fname)
{

    Scan *ActiveScan = NULL;
    Dataio *Dio = NULL;
    gint ret = -1;

    ActiveScan = main_get_gapp()->xsm->GetActiveScan();
    ret = main_get_gapp()->xsm->FindChan(ID_CH_M_ACTIVE);

    // check if something else (no NetCDF)
    if (!(strstr(fname, ".nc") || strstr(fname, ".NC"))) {
	main_get_gapp()->xsm->gnuimport(fname);
	return ret;
    } else {
	if (!strncasecmp(fname + strlen(fname) - 3, ".nc", 3))
	    Dio = new NetCDF(ActiveScan, fname);	// NetCDF File
	else
	    ret = -1;		//Error: no import filter found!

	Dio->Read();
	if (Dio->ioStatus()) {
	    //Some error occured 
	    ret = -1;
	}
	delete Dio;

	ActiveScan->GetDataSet(main_get_gapp()->xsm->data);
	// refresh all Parameter and draw Scan
	main_get_gapp()->spm_update_all();
	ActiveScan->draw();

	return ret;
    }
}

gint converter::writeFromCh(gint Ch, gchar * fname)
{
    gint ret;

    if (!(strstr(fname, ".nc") || strstr(fname, ".NC"))) {
	//Make sure We are using the right channel
	if (main_get_gapp()->xsm->FindChan(ID_CH_M_ACTIVE) != Ch)
	    main_get_gapp()->xsm->ActivateChannel(Ch);

	ret = main_get_gapp()->xsm->gnuexport(fname);
	return ret;
    } else {
	ret = main_get_gapp()->xsm->save(AUTO_NAME_SAVE, fname, Ch, true);
	return ret;
    }

    return -1;
}


/** Generates the full path to the target file */
gchar *converter::strParse(gchar * name, converterData * check)
{
    PI_DEBUG(DBG_L2, "str parse");
  /** copy the argument */
    int len = strlen(name);
    gchar *fname = strdup(name);

    if (len > 0) {
    /** Chop off the suffix of the name */
	int index = len;
	for (; index > 0; --index) {
	    if (fname[index] == EXT_SEP)
		break;
	}
	fname[index] = '\0';

	len = strlen(check->destDir);

    /** make sure that the directory is with terminating '/' */
	if (len > 0) {
	    if (check->destDir[len - 1] != '/')
		strcat(check->destDir, "/");
	}

	snprintf(retStr, MAX_PATH, "%s%s.%s", check->destDir, fname,
		 check->writeFormat);
	return retStr;
    } else
	return NULL;
}

////////////////////////////////////////
// ENDE converter
///////////////////////////////////////

////////////////////////////////////////
// ANFANG converterControl
///////////////////////////////////////

gint file_error(GtkWindow * window, const gchar * err_msg,
		const gchar * dir);

converterControl::converterControl()
{
    topdata =
	new converterData(getenv("PWD"), getenv("PWD"), "*.nc", "top");
    XsmRescourceManager xrm("FilePathSelectionHistory", "Converter");
    xrm.Get("SourcePath", &topdata->sourceDir, getenv("PWD"));
    xrm.Get("DestPath", &topdata->destDir, getenv("PWD"));
    xrm.Get("InputFilter", &topdata->convFilter, "*.nc");
    xrm.Get("OutputFilter", &topdata->writeFormat, "top");
}


converterControl::~converterControl()
{
    delete topdata;
}


void
 converterControl::run()
{
    GtkWidget *dialog;
    GtkWidget *vbox;
    GtkWidget *hbox;
    GtkWidget *VarName;
    GtkWidget *variable;
    GtkWidget *help;

    // create dialog window
    GtkDialogFlags flags = GTK_DIALOG_MODAL;
    dialog = gtk_dialog_new_with_buttons("Converter",
					 NULL,
					 flags,
					 N_("_OK"),
					 GTK_RESPONSE_OK,
					 N_("_CANCEL"),
					 GTK_RESPONSE_CANCEL, NULL);


    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_show (vbox);
    gtk_box_append (GTK_BOX (gtk_dialog_get_content_area(GTK_DIALOG(dialog))), vbox);

    // create widget for source file
    hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_show (hbox);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

    VarName = gtk_label_new(N_("Source Path"));
    gtk_widget_set_size_request(VarName, 100, -1);
    gtk_label_set_justify(GTK_LABEL(VarName), GTK_JUSTIFY_LEFT);
    gtk_box_pack_start(GTK_BOX(hbox), VarName, TRUE, TRUE, 0);

    SrcDir_button =
	gtk_file_chooser_button_new("source folder",
				    GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER);
    gtk_box_pack_start(GTK_BOX(hbox), GTK_WIDGET(SrcDir_button), TRUE,
		       TRUE, 0);

    // create widget for file mask
    hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

    VarName = gtk_label_new(N_("Selection Mask"));
    gtk_widget_set_size_request(VarName, 100, -1);
    gtk_label_set_justify(GTK_LABEL(VarName), GTK_JUSTIFY_LEFT);
    gtk_box_pack_start(GTK_BOX(hbox), VarName, TRUE, TRUE, 0);

    SrcMask = gtk_entry_new();
    gtk_box_pack_start(GTK_BOX(hbox), SrcMask, TRUE, TRUE, 0);
    gtk_entry_set_text(GTK_ENTRY(SrcMask), topdata->convFilter);

    help = gtk_button_new_with_label(N_("Help"));
    gtk_box_pack_start(GTK_BOX(hbox), help, TRUE, TRUE, 0);
    g_signal_connect(G_OBJECT(help), "clicked",
		     G_CALLBACK(show_info_callback),
		     (void
		      *) (N_("Select subset of files via wildcard.")));
    hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

    // create widget for destination path    
    VarName = gtk_label_new(N_("Destination Path"));
    gtk_widget_set_size_request(VarName, 100, -1);
    gtk_label_set_justify(GTK_LABEL(VarName), GTK_JUSTIFY_LEFT);
    gtk_box_pack_start(GTK_BOX(hbox), VarName, TRUE, TRUE, 0);

    DestDir_button =
	gtk_file_chooser_button_new("target folder",
				    GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER);
    gtk_box_pack_start(GTK_BOX(hbox), GTK_WIDGET(DestDir_button), TRUE,
		       TRUE, 0);

    hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

    VarName = gtk_label_new(N_("Format"));
    gtk_widget_set_size_request(VarName, 100, -1);
    gtk_label_set_justify(GTK_LABEL(VarName), GTK_JUSTIFY_LEFT);
    gtk_box_pack_start(GTK_BOX(hbox), VarName, TRUE, TRUE, 0);

    DestMask = gtk_entry_new();
    gtk_box_pack_start(GTK_BOX(hbox), DestMask, TRUE, TRUE, 0);
    gtk_entry_set_text(GTK_ENTRY(DestMask), topdata->writeFormat);

    help = gtk_button_new_with_label(N_("Help"));
    gtk_box_pack_start(GTK_BOX(hbox), help, TRUE, TRUE, 0);
    g_signal_connect(G_OBJECT(help), "clicked",
		     G_CALLBACK(show_info_callback),
		     (void
		      *) (N_
			  ("Enter the file format suffix without any special characters")));

    hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
    GtkWidget *toggle_recursive =
	gtk_check_button_new_with_label("Recursive");

    gtk_box_pack_start(GTK_BOX(hbox), toggle_recursive, TRUE, TRUE, 0);
    GtkWidget *toggle_overwrite_target =
	gtk_check_button_new_with_label("Overwrite Target File");
    g_signal_connect(G_OBJECT(toggle_recursive), "clicked",
		     G_CALLBACK(converterControl::recursive_click), this);

    gtk_box_pack_start(GTK_BOX(hbox), toggle_overwrite_target, TRUE, TRUE,
		       0);
    g_signal_connect(G_OBJECT(toggle_overwrite_target), "clicked",
		     G_CALLBACK(converterControl::overwrite_target_click),
		     this);

    GtkWidget *toggle_create_subdirs =
	gtk_check_button_new_with_label("Create sub-directories");
    gtk_box_pack_start(GTK_BOX(hbox), toggle_create_subdirs, TRUE, TRUE,
		       0);
    g_signal_connect(G_OBJECT(toggle_create_subdirs), "clicked",
		     G_CALLBACK(converterControl::create_subdirs_click),
		     this);

    gtk_widget_show_all(dialog);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_CANCEL) {
	gtk_widget_destroy(dialog);
	return;
    }

    g_free(topdata->sourceDir);
    topdata->sourceDir =
	g_strdup(gtk_file_chooser_get_uri
		 (GTK_FILE_CHOOSER(SrcDir_button)));
    if (topdata->sourceDir == NULL) {
	gtk_widget_destroy(dialog);
	return;
    }

    g_free(topdata->convFilter);
    topdata->convFilter = g_strdup(gtk_entry_get_text(GTK_ENTRY(SrcMask)));


    g_free(topdata->destDir);
    topdata->destDir =
	g_strdup(gtk_file_chooser_get_uri
		 (GTK_FILE_CHOOSER(DestDir_button)));
    if (access(topdata->destDir, F_OK)) {
	gtk_widget_destroy(dialog);
	//    if(file_error(GTK_WINDOW(dialog),"Destination directory does not exist",topdata->destDir)){
	return;
    }

    g_free(topdata->writeFormat);
    topdata->writeFormat =
	g_strdup(gtk_entry_get_text(GTK_ENTRY(DestMask)));

    show_info_callback(NULL, N_("Converting ..."));
    converter converter_obj;
    converter_obj.ConvertDir(topdata, 0);

    PI_DEBUG(DBG_L2,
	     "Running with the following options: \n\tSource path=" <<
	     topdata->sourceDir << "\n\tInput filer=" << topdata->
	     convFilter << "\n\tdestination path=" << topdata->
	     destDir << "\n\tOutput filter=" << topdata->writeFormat);

    XsmRescourceManager xrm("FilePathSelectionHistory", "Converter");
    xrm.Put("SourcePath", topdata->sourceDir);
    xrm.Put("DestPath", topdata->destDir);
    xrm.Put("InputFilter", topdata->convFilter);
    xrm.Put("OutputFilter", topdata->writeFormat);

    gtk_widget_destroy(dialog);
    return;
}


void converterControl::recursive_click(GtkWidget * widget,
				       gpointer userdata)
{
    ((converterControl *) userdata)->topdata->m_recursive =
	gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));
}


void converterControl::overwrite_target_click(GtkWidget * widget,
					      gpointer userdata)
{
    ((converterControl *) userdata)->topdata->m_overwrite_target =
	gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));
}


void converterControl::create_subdirs_click(GtkWidget * widget,
					    gpointer userdata)
{
    ((converterControl *) userdata)->topdata->m_create_subdirs =
	gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));
}


gint
file_error(GtkWindow * window, const gchar * err_msg, const gchar * dir)
{
    GtkWidget *dialog;
    if (dir != NULL) {
	gchar buffer[1000];
	snprintf(buffer, 1000, "Create Directory %s?", dir);
	dialog = gtk_message_dialog_new(window,
					GTK_DIALOG_DESTROY_WITH_PARENT,
					GTK_MESSAGE_QUESTION,
					GTK_BUTTONS_YES_NO, "%s", buffer);
	gtk_window_set_title(GTK_WINDOW(dialog), err_msg);

        gtk_widget_show (dialog);

        int response = GTK_RESPONSE_NONE;
        g_signal_connect (G_OBJECT (dialog), "response", G_CALLBACK (GnomeAppService::on_dialog_response_to_user_data), &response);
        
        // FIX-ME GTK4 ??
        // wait here on response
        while (response == GTK_RESPONSE_NONE)
                while(g_main_context_pending (NULL)) g_main_context_iteration (NULL, FALSE);
        
	if (response == GTK_RESPONSE_YES) {
	    printf("create\n");
	    if (mkdir(dir, UMASK)) {
		PI_DEBUG(DBG_L2, "Error: creating the directory");
		return 1;
	    }
	    return 0;
	} else {
	    PI_DEBUG(DBG_L2, "Aborting ..." << err_msg);
	    return 1;
	}
    }

    else {
	dialog = gtk_message_dialog_new(window,
					GTK_DIALOG_DESTROY_WITH_PARENT,
					GTK_MESSAGE_ERROR,
					GTK_BUTTONS_OK, "%s", err_msg);
	gtk_window_set_title(GTK_WINDOW(dialog), "Error");

        gtk_widget_show (dialog);

        int response = GTK_RESPONSE_NONE;
        g_signal_connect (G_OBJECT (dialog), "response", G_CALLBACK (GnomeAppService::on_dialog_response_to_user_data), &response);
        
        // FIX-ME GTK4 ??
        // wait here on response
        while (response == GTK_RESPONSE_NONE)
                while(g_main_context_pending (NULL)) g_main_context_iteration (NULL, FALSE);
	PI_DEBUG(DBG_L2, "Aborting ... " << err_msg);

	return 1;
    }
}

////////////////////////////////////////
// ENDE converterControl
///////////////////////////////////////

////////////////////////////////////////
//ANFANG converterData
///////////////////////////////////////

converterData::converterData(const gchar * src, const gchar * dst, const gchar * conv, const gchar * write):m_recursive(false), m_overwrite_target(false),
    m_create_subdirs
    (false)
{
    sourceDir = g_strdup(src);
    destDir = g_strdup(dst);
    convFilter = g_strdup(conv);
    writeFormat = g_strdup(write);
}

converterData::~converterData()
{
    g_free(sourceDir);
    g_free(destDir);
    g_free(convFilter);
    g_free(writeFormat);
}

////////////////////////////////////////
// ENDE converterData
////////////////////////////////////////
