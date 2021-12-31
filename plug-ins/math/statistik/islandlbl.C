/* Gnome gxsm - Gnome X Scanning Microscopy
 * universal STM/AFM/SARLS/SPALEED/... controlling and
 * data analysis software
 * 
 * Gxsm Plugin Name: islandlbl.C
 * ========================================
 * 
 * Copyright (C) 1999 The Free Software Foundation
 *
 * Authors: Percy Zahl <zahl@fkp.uni-hannover.de>
 * additional features: Andreas Klust <klust@fkp.uni-hannover.de>
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
#include "config.h"
#include "core-source/plugin.h"
#include "stdio.h"

#include "mathilbl.h"

static void islandlbl_init( void );
static void islandlbl_about( void );
static void islandlbl_query( void );
static void islandlbl_configure(void);
static void islandlbl_cleanup( void );
void toggle_button_callback1( GtkWidget *widget, gpointer data);
void toggle_button_callback2( GtkWidget *widget, gpointer data); 
void enter_callback( GtkWidget *widget, GtkWidget *entry );

GxsmPlugin islandlbl_pi = {
  NULL,
  NULL,
  0,
  NULL,
  "Islandlbl-M1S-ST",
  "+STM +AFM",
  NULL,
  "Markus Bierkandt and Percy Zahl",
  "math-statistics-section",
  "...",
  NULL,
  "no more info",
  NULL,
  NULL,
  islandlbl_init,
  islandlbl_query,
  islandlbl_about,
  islandlbl_configure,
  NULL,
  NULL,
  NULL,
  islandlbl_cleanup
};

static const char *about_text = N_("Gxsm Islandlbl Plugin:\n"
				   "Perform some Statistics: Island Labeling, counting, ...\n"
                                   "Mathroutines by Markus Bierkandt 2000,\n"
				   "Pluginsupport by Percy Zahl 2000.");

GxsmPlugin *get_gxsm_plugin_info ( void ){ 
  islandlbl_pi.description = g_strdup_printf(N_("Gxsm MathOneArg islandlbl plugin %s"), VERSION);
  return &islandlbl_pi; 
}

static void islandlbl_query(void)
{
  static GnomeUIInfo menuinfo[] = { 
    { GNOME_APP_UI_ITEM, 
      N_("Step Flaten"), N_("MB's super Step Flaten Algo"), 
      (gpointer) App::math_onearg_callback, (gpointer) StepFlaten,
      NULL, GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_BLANK, 
      0, GDK_CONTROL_MASK, NULL },

    { GNOME_APP_UI_ITEM, 
      N_("Kill step islands"), N_("kills and counts step islands"),
      (gpointer) App::math_onearg_callback, (gpointer) KillStepIslands,
      NULL, GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_BLANK, 
      0, GDK_CONTROL_MASK, NULL },

    { GNOME_APP_UI_ITEM, 
      N_("IslandLabl"), N_("MB's island labeling tool"), 
      (gpointer) App::math_onearg_callback, (gpointer) IslandLabl,
      NULL, GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_BLANK, 
      0, GDK_CONTROL_MASK, NULL },

    GNOMEUIINFO_SEPARATOR,
    GNOMEUIINFO_END
  };

  gnome_app_insert_menus(GNOME_APP(islandlbl_pi.app->getApp()), islandlbl_pi.menupath, menuinfo);

  if(islandlbl_pi.status) g_free(islandlbl_pi.status); 
  islandlbl_pi.status = g_strconcat(N_("Plugin query has attached "),
			  islandlbl_pi.name, 
			  N_(": StepFlaten, KillStepIslands and IslandLabl are ready to use"),
			  NULL);
}

// init-Function
static void islandlbl_init(void)
{
  PI_DEBUG (DBG_L2, "Vorlage Plugin Init");
  filename = g_strdup("/tmp/test.asc");
}

static void islandlbl_about(void)
{
  const gchar *authors[] = { "Markus Bierkandt", "Percy Zahl", NULL};
  gtk_show_about_dialog (NULL, 
			 "program-name",  islandlbl_pi.name,
			 "version", VERSION,
			 "license", GTK_LICENSE_GPL_3_0,
			 "comments", about_text,
			 "authors", authors,
			 NULL
			 );
}



void toggle_button_callback1( GtkWidget *widget, gpointer data)
{
    if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget))){
      inter=1;
      printf("interaktiver Dialog enabled\n");
    }
    else{
      inter=0;
      printf("interaktiver Dialog disabled\n");
    }
}

void toggle_button_callback2( GtkWidget *widget, gpointer data)
{
    if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget))){
      save=1;
      printf("Dateiausgabe enabled\n");
    }
    else{
      save=0;
      printf("Dateiausgabe disabled\n");
    }
}

void enter_callback( GtkWidget *widget, GtkWidget *entry )
{
    // gchar *p = gtk_entry_get_text(GTK_ENTRY(entry));
    // filename=  g_strdup(p);
    // if (p) g_free(p);
    if (filename) g_free(filename);
    filename = g_strdup( gtk_entry_get_text(GTK_ENTRY(entry)));
    printf("Filename: %s\n", filename);
}


static void islandlbl_configure(void)
{
  GtkWidget *dialog;
  GtkWidget *vbox, *fbox;
  GtkWidget *hbox, *hbox_check1, *hbox_check2, *hbox_entry, *entry;
  GtkWidget *checkbutton1, *checkbutton2;
  GtkWidget *input;
  
  dialog = gnome_dialog_new(N_("Monte-Carlo Auswertung Setup"),
			    GNOME_STOCK_BUTTON_OK,
			    NULL); 
        
  gnome_dialog_set_close(GNOME_DIALOG(dialog), FALSE);
  gnome_dialog_close_hides(GNOME_DIALOG(dialog), FALSE);
  gnome_dialog_set_default(GNOME_DIALOG(dialog), 0);

  vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  gtk_widget_show (vbox);

  gtk_box_pack_start(GTK_BOX(GNOME_DIALOG(dialog)->vbox),
		     vbox, TRUE, TRUE, GXSM_WIDGET_PAD);

  //  old
  //  info = gtk_label_new ("Default-Werte");
  //  gtk_widget_show (info);
  //  gtk_box_pack_start(GTK_BOX(vbox), info, TRUE, TRUE, GXSM_WIDGET_PAD);

  // begin new code ... by PZ
  fbox = gtk_frame_new (N_("Default-Werte"));
  gtk_widget_show (fbox);
  gtk_container_add(GTK_CONTAINER(vbox), fbox);

  vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  gtk_widget_show (vbox);
  gtk_container_add (GTK_CONTAINER (fbox), vbox);

  // --- end new code by PZ

  input = islandlbl_pi.app->mygtk_create_input("Stepamount", vbox, hbox);
  Gtk_EntryControl ECvar_sa(islandlbl_pi.app->xsm->Unity, "Value out of range !", &d_sa, 
			 0, 256, ".0f", input);
  input = islandlbl_pi.app->mygtk_create_input("periodische RB", vbox, hbox);
  Gtk_EntryControl ECvar_pr(islandlbl_pi.app->xsm->Unity, "Value out of range !", &d_pr, 
			 -1, 5, ".0f", input);
  
  // Check Button fuer Abfrage Dialog interaktiv / nicht interaktiv
  
  hbox_check1 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_widget_show (hbox_check1);
  gtk_container_add (GTK_CONTAINER (vbox), hbox_check1);
  
  
  checkbutton1 = gtk_check_button_new_with_label("Interaktiver Dialog");
  gtk_box_pack_start (GTK_BOX (hbox_check1), checkbutton1, TRUE, TRUE, 0);
  gtk_widget_show (checkbutton1);
  g_signal_connect (G_OBJECT (checkbutton1), "clicked",
		      G_CALLBACK (toggle_button_callback1), NULL);
  if (inter==1) gtk_toggle_button_set_active ( GTK_TOGGLE_BUTTON (checkbutton1), TRUE );
  else gtk_toggle_button_set_active ( GTK_TOGGLE_BUTTON (checkbutton1), FALSE );

  // Check Button fuer Abfrage Statistik Ausgabe in Datei
  
  hbox_check2 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_widget_show (hbox_check2);
  gtk_container_add (GTK_CONTAINER (vbox), hbox_check2);
  
  
  checkbutton2 = gtk_check_button_new_with_label("Ausgabe in Datei");
  gtk_box_pack_start (GTK_BOX (hbox_check2), checkbutton2, TRUE, TRUE, 0);
  gtk_widget_show (checkbutton2);
  g_signal_connect (G_OBJECT (checkbutton2), "clicked",
		      G_CALLBACK (toggle_button_callback2), NULL);
  if (save==1) gtk_toggle_button_set_active ( GTK_TOGGLE_BUTTON (checkbutton2), TRUE );
  else gtk_toggle_button_set_active ( GTK_TOGGLE_BUTTON (checkbutton2), FALSE );

  // Input fuer Filename
  hbox_entry = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_widget_show (hbox_entry);
  gtk_container_add (GTK_CONTAINER (vbox), hbox_entry);
  
  entry = gtk_entry_new_with_max_length (50);
  g_signal_connect (G_OBJECT (entry), "activate",
		      G_CALLBACK (enter_callback), entry);
  gtk_entry_set_text (GTK_ENTRY (entry), filename);
  gtk_box_pack_start (GTK_BOX (hbox_entry), entry, TRUE, TRUE, 0);
  gtk_widget_show (entry);

  input = islandlbl_pi.app->mygtk_create_input("MC-Base:", vbox, hbox);
  Gtk_EntryControl ECvar_lbn(islandlbl_pi.app->xsm->Unity, "Value out of range !", &d_lbn, 
			 2., 10., ".0f", input);


  gtk_widget_show(dialog);

  gnome_dialog_run_and_close(GNOME_DIALOG(dialog));
}



static void islandlbl_cleanup( void ){
  PI_DEBUG (DBG_L2, "IslandLbl Plugin Cleanup");
  gchar *mp = g_strconcat(islandlbl_pi.menupath, N_("Step Flaten"), NULL);
  gnome_app_remove_menus (GNOME_APP( islandlbl_pi.app->getApp() ), mp, 4);
  g_free(mp);
  if (filename){ 
    g_free(filename);
    filename=NULL;
  }
}
