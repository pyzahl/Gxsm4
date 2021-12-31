/* Gnome gxsm - Gnome X Scanning Microscopy
 * universal STM/AFM/SARLS/SPALEED/... controlling and
 * data analysis software
 * 
 * Gxsm Plugin Name: listplugins.C
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


/* Please do not change the Begin/End lines of this comment section!
 * this is a LaTeX style section used for auto generation of the PlugIn Manual 
 * Chapter. Add a complete PlugIn documentation inbetween the Begin/End marks!
 * --------------------------------------------------------------------------------
% BeginPlugInDocuSection
% PlugInDocuCaption: Show plug-in details
% PlugInName: listplugins
% PlugInAuthor: Percy Zahl
% PlugInAuthorEmail: zahl@users.sf.net
% PlugInMenuPath: Tools/Plugin Details

% PlugInDescription
\label{plugin:listplugins}
This plugin lists all currently loaded plugins and shows all available
information about those.  In addition, it allows to call/run the About and
Configure subroutines of each plugin -- if available. 

% PlugInUsage
Call it using the menu \GxsmMenu{Tools/Plugin Details}.

%% OptPlugInKnownBugs
%No known.

% EndPlugInDocuSection
 * -------------------------------------------------------------------------------- 
 */


#include <gtk/gtk.h>
#include "config.h"
#include "core-source/plugin.h"

// ===========================================
// borrowed from gimp "pluginsdetails" PlugIn
// ===========================================

#define DBL_LIST_WIDTH  250
#define DBL_WIDTH       (DBL_LIST_WIDTH + 300)
#define DBL_HEIGHT      200

typedef struct  
{
  GtkWidget *dlg;
  GtkWidget *clist;
  GtkWidget *ctree;
  GtkWidget *search_entry;
  GtkWidget *descr_scroll;
  GtkWidget *name_button;
  GtkWidget *blurb_button;
  GtkWidget *scrolled_win;
  GtkWidget *ctree_scrolled_win;
  GtkWidget *info_table;
  GtkWidget *paned;
  GtkWidget *left_paned;
  GtkWidget *info_align;
  gint       num_plugins;
  gint       ctree_row;
  gint       clist_row;
  gint       c1size;
  gboolean   details_showing;
} PDesc;

PDesc *plugindesc = NULL;

static void 
dialog_search_callback (GtkDialog *dlg, 
			gpointer   data);
static gint 
procedure_clist_select_callback (GtkWidget      *widget,
				 gint            row, 
				 gint            column, 
				 GdkEventButton *bevent,
				 gpointer        data);
static gint 
procedure_ctree_select_callback (GtkWidget      *widget,
				 GtkWidget      *row, 
				 gint            column, 
				 gpointer        data);
static void 
clist_click_column (GtkCList *clist, 
		    gint      column, 
		    gpointer  data);
static gint 
name_sort (GtkCList      *clist,
	   gconstpointer  ptr1,
	   gconstpointer  ptr2);
static void 
insert_into_ctree (PDesc      *pdesc,
		   gchar      *name,
		   gchar      *xtimestr,
		   gchar      *menu_str,
		   gchar      *types_str,
		   GHashTable *ghash,
		   GxsmPlugin      *GPlugin);
static void
get_plugin_info (PDesc *pdesc,
                 const gchar *search_text);
static gint 
page_select_callback (GtkNotebook     *notebook,
		      GtkNotebookPage *page,
		      guint            page_num,
		      gpointer         data);
static gchar *
format_menu_path (gchar *s);

static void
details_callback (GtkWidget *widget, 
		  gpointer   data);

static void
exec_callback (GtkWidget *widget, 
	       gpointer   data);


// ========================================
// Gxsm PlugIn Setup Part
// ========================================

static void listplugins_about( void );
static void listplugins_run(GtkWidget *w, void *data);
static void listplugins_cleanup( void );
void List_GxsmPlugins( void );

GxsmPlugin listplugins_pi = {
  NULL,
  NULL,
  0,
  NULL,
  "GxsmPlugindetails",
  NULL,
  NULL,
  "Some code from Andy Thomas's Gimp Plugin: \"plugin details\",\n"
  "Gxsm Version by Percy Zahl",
  "tools-section",
  N_("Plugin Details"),
  N_("List/Configure available Gxsm Plugins"),
  "no more info",
  NULL,
  NULL,
  NULL,
  NULL,
  listplugins_about,
  NULL,
  listplugins_run,
  listplugins_cleanup
};

static const char *about_text = N_("Plugin Control for Gxsm:\n\n"
				   "- View Gxsm-Plugin details and status\n"
				   "- Call plugins About or Configurationfunction\n"
                                   "Plugin written in inspiration of \"The Gimp\" - Plugin Details:\n"
				   "Displays plugin details"
				   "Helps browse the plugin menus system. You can "
				   "search for plugin names, sort by name or menu "
				   "location and you can view a tree representation "
				   "of the plugin menus. Can also be of help to find "
				   "where new plugins have installed themselves in "
				   "the menuing system"
 				   );


GxsmPlugin *get_gxsm_plugin_info ( void ){ 
  listplugins_pi.description = g_strdup_printf(N_("Gxsm Plugin Details %s"), VERSION);
  return &listplugins_pi; 
}

static void listplugins_about(void)
{
  const gchar *authors[] = { "Percy Zahl", "Andy Thomas, (Gimp: Plugin Details)", NULL};
  gtk_show_about_dialog (NULL, 
			 "program-name",  listplugins_pi.name,
			 "version", VERSION,
			 "license", GTK_LICENSE_GPL_3_0,
			 "comments", about_text,
			 "authors", authors,
			 NULL
			 );
}

static void listplugins_run(GtkWidget *w, void *data)
{
  if(listplugins_pi.app){
    if(listplugins_pi.app->GxsmPlugins)
      List_GxsmPlugins();
    else
      listplugins_pi.app->message("Sorry, no Gxsm Plugins to list!");
  }
  else
	  PI_DEBUG (DBG_L2, "Listplugins Plugin Run, sorry no Application Class available!" );
}

static void listplugins_cleanup( void ){
  if( plugindesc ){
    if( plugindesc->dlg )
      gnome_dialog_close( GNOME_DIALOG (plugindesc->dlg) );
    g_free( plugindesc );
    plugindesc = NULL;
  }
  while(gtk_events_pending()) gtk_main_iteration();
}

GtkWidget* create_GxsmPluginList (void);

void List_GxsmPlugins( void )
{
  GtkWidget *Dlg = create_GxsmPluginList ();
  gtk_widget_show (Dlg);
}


// ===================================================================================
// code borrowed from gimp "pluginsdetails" PlugIn, modified for Gxsm PlugIns...
// ===================================================================================

static void dialog_clicked_cb(GtkDialog * dialog, gint button_number, PDesc *p){
  switch(button_number){
  case 0:  dialog_search_callback(dialog, p); break;
  case 1:  listplugins_cleanup(); break;
  default: g_assert_not_reached();
  }
}

GtkWidget* create_GxsmPluginList ( void )
{
  GtkWidget  *button;
  GtkWidget  *hbox, *searchhbox, *vbox;
  GtkWidget  *label, *notebook, *swindow;
  gchar      *clabels[4];

  plugindesc = g_new0 (PDesc, 1);

  /* the dialog box */
  plugindesc->dlg = gnome_dialog_new (N_("Plugin Descriptions"), NULL);
  gtk_window_set_policy (GTK_WINDOW (plugindesc->dlg), TRUE, TRUE, FALSE);
  gnome_dialog_set_close(GNOME_DIALOG(plugindesc->dlg), FALSE);
  gnome_dialog_close_hides(GNOME_DIALOG(plugindesc->dlg), FALSE);
  gnome_dialog_set_default(GNOME_DIALOG(plugindesc->dlg), 0);

  GtkWidget *dialog_action_area = GNOME_DIALOG (plugindesc->dlg)->action_area;
  gtk_widget_show (dialog_action_area);
  gtk_button_box_set_layout (GTK_BUTTON_BOX (dialog_action_area), GTK_BUTTONBOX_END);
  gtk_button_box_set_spacing (GTK_BUTTON_BOX (dialog_action_area), 8);

  gnome_dialog_append_button (GNOME_DIALOG (plugindesc->dlg), GNOME_STOCK_BUTTON_APPLY);
  GtkWidget *button1 = GTK_WIDGET (g_list_last (GNOME_DIALOG (plugindesc->dlg)->buttons)->data);
  gtk_widget_show (button1);
  GTK_WIDGET_SET_FLAGS (button1, GTK_CAN_DEFAULT);

  gnome_dialog_append_button (GNOME_DIALOG (plugindesc->dlg), GNOME_STOCK_BUTTON_CANCEL);
  GtkWidget *button2 = GTK_WIDGET (g_list_last (GNOME_DIALOG (plugindesc->dlg)->buttons)->data);
  gtk_widget_show (button2);
  GTK_WIDGET_SET_FLAGS (button2, GTK_CAN_DEFAULT);

  g_signal_connect(G_OBJECT(plugindesc->dlg), "clicked",
  		     G_CALLBACK(dialog_clicked_cb),
		     plugindesc);

  plugindesc->details_showing = FALSE;

  /* hbox : left=notebook ; right=description */
  
  plugindesc->paned = hbox = gtk_hpaned_new ();

  gtk_box_pack_start (GTK_BOX (GNOME_DIALOG (plugindesc->dlg)->vbox), 
		      hbox, TRUE, TRUE, 0);
  gtk_widget_show (hbox);

//  gtk_paned_set_handle_size (GTK_PANED (hbox), 0);
  gtk_paned_set_gutter_size (GTK_PANED (hbox), 0);

  /* left = vbox : the list and the search entry */
  
  plugindesc->left_paned = vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 3); 
  gtk_paned_pack1 (GTK_PANED (hbox), vbox, FALSE, FALSE);
  gtk_widget_show (vbox);

  /* left = notebook */

  notebook = gtk_notebook_new ();
  gtk_box_pack_start (GTK_BOX (vbox), notebook, TRUE, TRUE, 0);

  /* list : list in a scrolled_win */
  
  clabels[0] = g_strdup (N_("Menu Path")); 
  clabels[1] = g_strdup (N_("Entry")); 
  clabels[2] = g_strdup (N_("Name")); 
  clabels[3] = g_strdup (N_("Type")); 
  plugindesc->clist = gtk_clist_new_with_titles (4, clabels); 
  g_free(clabels[3]);
  g_free(clabels[2]);
  g_free(clabels[1]);
  g_free(clabels[0]);


  g_signal_connect (G_OBJECT (plugindesc->clist), "click_column",
		      G_CALLBACK (clist_click_column),
		      NULL);
  gtk_clist_column_titles_show (GTK_CLIST (plugindesc->clist));
  swindow = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (swindow),
				  GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_clist_set_selection_mode (GTK_CLIST (plugindesc->clist),
			        GTK_SELECTION_BROWSE);

  gtk_widget_set_size_request (plugindesc->clist, DBL_LIST_WIDTH, DBL_HEIGHT);
  g_signal_connect (G_OBJECT (plugindesc->clist), "select_row",
		      G_CALLBACK (procedure_clist_select_callback),
		      plugindesc);
  
  label = gtk_label_new (N_("List View"));
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), swindow, label);
  gtk_container_add (GTK_CONTAINER (swindow), plugindesc->clist);
  gtk_widget_show (plugindesc->clist);
  gtk_widget_show (swindow);

  /* notebook->ctree */
  clabels[0] = g_strdup (N_("Menu Path/Name")); 
  clabels[1] = g_strdup (N_("Name")); 
  clabels[2] = g_strdup (N_("Type")); 
  plugindesc->ctree = gtk_ctree_new_with_titles (3, 0, clabels);  
  g_free(clabels[2]);
  g_free(clabels[1]);
  g_free(clabels[0]);

  plugindesc->ctree_scrolled_win =
    swindow = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (swindow),
				  GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_widget_set_size_request (plugindesc->ctree, DBL_LIST_WIDTH, DBL_HEIGHT);
  g_signal_connect (G_OBJECT (plugindesc->ctree), "tree_select_row",
		      G_CALLBACK (procedure_ctree_select_callback),
		      plugindesc);
  gtk_clist_set_column_auto_resize (GTK_CLIST (plugindesc->ctree), 0, TRUE);
  gtk_clist_set_column_auto_resize (GTK_CLIST (plugindesc->ctree), 1, TRUE);
  gtk_clist_set_column_auto_resize (GTK_CLIST (plugindesc->ctree), 2, TRUE);
  gtk_clist_column_titles_passive (GTK_CLIST (plugindesc->ctree));

  label = gtk_label_new (N_("Tree View"));
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), swindow, label);
  gtk_container_add (GTK_CONTAINER (swindow), plugindesc->ctree);

  gtk_widget_show (plugindesc->ctree);
  gtk_widget_show (swindow);

  g_signal_connect_after (G_OBJECT (notebook), "switch_page",
			    G_CALLBACK (page_select_callback),
			    plugindesc);
  
  gtk_widget_show (notebook);

  /* search entry & details button */

  searchhbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
  gtk_box_pack_start (GTK_BOX (vbox),
		      searchhbox, FALSE, FALSE, 0);
  gtk_widget_show (searchhbox);

  label = gtk_label_new (N_("Search:"));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_box_pack_start (GTK_BOX (searchhbox), 
		      label, FALSE, FALSE, 0);
  gtk_widget_show(label);

  plugindesc->search_entry = gtk_entry_new ();
  gtk_box_pack_start (GTK_BOX (searchhbox), 
		      plugindesc->search_entry, TRUE, TRUE, 0);
  gtk_widget_show (plugindesc->search_entry);

  button = gtk_button_new_with_label (N_("Details >>"));
  gtk_misc_set_padding (GTK_MISC (GTK_BIN (button)->child), 2, 0);
  g_signal_connect (G_OBJECT (button), "clicked",
		      G_CALLBACK (details_callback),
		      plugindesc);
  gtk_box_pack_start (GTK_BOX (searchhbox), button,
		      FALSE, FALSE, 0);
  gtk_widget_show (button);

  /* right = description */
  /* the right description is build on first click of the Details button */
 
  /* now build the list */
  dialog_search_callback (NULL, (gpointer) plugindesc);

  gtk_clist_set_selection_mode (GTK_CLIST (plugindesc->ctree),
				GTK_SELECTION_BROWSE);

  gtk_widget_show (plugindesc->clist); 
  gtk_widget_show (plugindesc->dlg);
  
  gtk_clist_select_row (GTK_CLIST (plugindesc->clist), 0, 0);
  gtk_clist_moveto (GTK_CLIST (plugindesc->clist), 0, 0, 0.0, 0.0);

  plugindesc->c1size = GTK_PANED (plugindesc->paned)->child1_size;
  GTK_PANED (plugindesc->paned)->child1_resize = TRUE;

  return plugindesc->dlg;
}



/* Bit of a fiddle but sorta has the effect I want... */

static void
details_callback (GtkWidget *widget, 
		  gpointer   data)
{
  /* Show or hide the details window */ 
  PDesc *pdesc = (PDesc *)data;
  GtkLabel *lab = GTK_LABEL (GTK_BIN (widget)->child);

  /* This is a lame hack: 
     We add the description on the right on the first details_callback.
     Otherwise the window reacts quite weird on resizes */
  if (pdesc->descr_scroll == NULL)
    {
      pdesc->descr_scroll = gtk_scrolled_window_new (NULL, NULL);
      gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (pdesc->descr_scroll),
				      GTK_POLICY_ALWAYS, 
				      GTK_POLICY_ALWAYS);
      gtk_widget_set_size_request (pdesc->descr_scroll, DBL_WIDTH - DBL_LIST_WIDTH, -1);
      gtk_paned_pack2 (GTK_PANED (pdesc->paned), pdesc->descr_scroll,
		       FALSE, TRUE);
      gtk_clist_select_row (GTK_CLIST(pdesc->clist), pdesc->clist_row, -1);
    }

  if (pdesc->details_showing == FALSE)
    {
      GTK_PANED (pdesc->paned)->child1_resize = FALSE;
      gtk_paned_set_gutter_size (GTK_PANED (pdesc->paned), 6);
      gtk_label_set_text (lab, N_("Details <<"));
      gtk_widget_show (pdesc->descr_scroll);
      pdesc->details_showing = TRUE;
    }
  else
    {
      GtkWidget *p = GTK_WIDGET (pdesc->paned)->parent;
      GTK_PANED (pdesc->paned)->child1_resize = TRUE;
      GTK_PANED (pdesc->paned)->child2_resize = TRUE;
      gtk_paned_set_gutter_size (GTK_PANED (pdesc->paned), 0);
      gtk_label_set_text (lab, N_("Details >>"));
      gtk_widget_hide (pdesc->descr_scroll);
      gtk_paned_set_position (GTK_PANED (pdesc->paned),
			      p->allocation.width);/*plugindesc->c1size);*/
      pdesc->details_showing = FALSE;
    }
}

static gchar *
format_menu_path (gchar *s)
{
  gchar **str_array;
  gchar  *newstr = NULL;

  if (!s)
    return s;

  str_array = g_strsplit (s, "/", 0);

  newstr = g_strjoinv ("->", str_array);

  g_strfreev (str_array);

  return newstr;
}

static int add_GxsmPluginInfo(int table_row, PDesc *pdesc, const gchar *Label, const gchar *value){
  gchar *Value = NULL;
  GtkWidget *label;

  if(value)
    Value = g_strdup(value);
  else
    Value = g_strdup("--");

  if(strlen(Value) < 40){

    label = gtk_label_new (Label);
    gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5); 
    gtk_table_attach (GTK_TABLE (pdesc->info_table), label,
		      0, 1, table_row, table_row+1, GTK_FILL, GTK_FILL, 3, 6);
    gtk_widget_show(label);
    
    label = gtk_entry_new ();
    gtk_entry_set_text (GTK_ENTRY (label), Value);
    gtk_entry_set_editable (GTK_ENTRY (label), FALSE);
    gtk_table_attach (GTK_TABLE(pdesc->info_table), label,
		      1, strlen(Value)<10 ? 2:4, table_row, table_row+1, GTK_FILL, GTK_FILL, 0, 0);
    gtk_widget_show (label);
  }
  else{
    GtkWidget *text;
    GtkWidget *help;
    GtkWidget *vscrollbar;

    label = gtk_label_new (Label);
    gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5); 
    gtk_table_attach (GTK_TABLE (pdesc->info_table), label,
		      0, 1, table_row, table_row+1, 
		      GTK_FILL, GTK_FILL, 3, 0);
    gtk_widget_show (label);
    
    help = gtk_table_new (2, 2, FALSE);
    gtk_table_set_row_spacing (GTK_TABLE (help), 0, 2);
    gtk_table_set_col_spacing (GTK_TABLE (help), 0, 2);
    gtk_table_attach (GTK_TABLE (pdesc->info_table), help,
		      1, 4, table_row, table_row+1, GTK_FILL, GTK_FILL, 3, 0);
    gtk_widget_show (help);
    table_row++;
    
    text = gtk_text_view_new ();
//    gtk_text_view_set_border_window_size (GTK_TEXT_VIEW (text), GTK_TEXT_WINDOW_LEFT, 2);
//    gtk_text_view_set_border_window_size (GTK_TEXT_VIEW (text), GTK_TEXT_WINDOW_RIGHT, 2);
//    gtk_text_view_set_border_window_size (GTK_TEXT_VIEW (text), GTK_TEXT_WINDOW_TOP, 2);
//    gtk_text_view_set_border_window_size (GTK_TEXT_VIEW (text), GTK_TEXT_WINDOW_BOTTOM, 2);
    gtk_text_view_set_editable (GTK_TEXT_VIEW (text), FALSE);
    gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (text), GTK_WRAP_WORD);

    gtk_widget_set_size_request (text, -1, 60);
    gtk_table_attach (GTK_TABLE (help), text, 0, 1, 0, 1,
		      (GtkAttachOptions)(GTK_EXPAND | GTK_SHRINK | GTK_FILL),
		      (GtkAttachOptions)(GTK_EXPAND | GTK_SHRINK | GTK_FILL), 
		      0, 0);
    gtk_widget_show (text);
    
    label = gtk_hseparator_new (); /* ok, not really a label ... :) */
    gtk_table_attach (GTK_TABLE (pdesc->info_table), label,
		      0, 4, table_row, table_row+1, GTK_FILL, GTK_FILL, 3, 6);
    gtk_text_buffer_set_text (GTK_TEXT_BUFFER (gtk_text_view_get_buffer (GTK_TEXT_VIEW (text))), Value, -1);
    gtk_widget_show(label);
  }

  g_free(Value);

  return ++table_row;
}

static void
exec_callback (GtkWidget *widget, 
	       gpointer   data){
  void (*run)(void) = (void (*)(void)) data;
  run();
}

static gint
procedure_general_select_callback (PDesc *pdesc,
				   GxsmPlugin *GPlugin)
{
  GtkWidget *label, *button;
  GtkWidget *old_table;
  GtkWidget *old_align;
  gint       table_row = 0;
  gchar     *str;

  g_return_val_if_fail (pdesc != NULL, FALSE);
  g_return_val_if_fail (GPlugin != NULL, FALSE);

  if (pdesc->descr_scroll == NULL)
    return FALSE;

  old_table = pdesc->info_table;
  old_align = pdesc->info_align;

  pdesc->info_table = gtk_table_new (10, 5, FALSE);
  pdesc->info_align = gtk_alignment_new (0.5, 0.5, 0, 0);

  gtk_table_set_col_spacings (GTK_TABLE (pdesc->info_table), 3);

  /* Number of plugins */

  str = g_strdup_printf (N_("Number of Plugin Interfaces: %d"),
			 pdesc->num_plugins);
  label = gtk_label_new (str);
  g_free (str);
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (pdesc->info_table), label,
		    0, 4, table_row, table_row+1, GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (label);
  table_row++;

  label = gtk_hseparator_new (); /* ok, not really a label ... :) */
  gtk_table_attach (GTK_TABLE (pdesc->info_table), label,
		    0, 4, table_row, table_row+1, GTK_FILL, GTK_FILL, 3, 6);
  gtk_widget_show (label);
  table_row++;

  /* menu path */

  label = gtk_label_new (N_("Menu Path:"));
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5); 
  gtk_table_attach (GTK_TABLE (pdesc->info_table), label,
		    0, 1, table_row, table_row+1, GTK_FILL, GTK_FILL, 3, 0);
  gtk_widget_show(label);

  gchar *mp = g_strconcat(GPlugin->menupath, GPlugin->menuentry, NULL);
  gchar *fmp = format_menu_path (mp);
  label = gtk_label_new (fmp);
  g_free(fmp);
  g_free(mp);
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (pdesc->info_table), label,
		    1, 4, table_row, table_row+1, GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (label);
  table_row++;

  label = gtk_hseparator_new (); /* ok, not really a label ... :) */
  gtk_table_attach (GTK_TABLE (pdesc->info_table), label,
		    0, 4, table_row, table_row+1, GTK_FILL, GTK_FILL, 3, 6);
  gtk_widget_show (label);
  table_row++;

  /* show GxsmPlugin Infos */

  table_row = add_GxsmPluginInfo(table_row, pdesc, N_("Plugin"), GPlugin->filename);
  table_row = add_GxsmPluginInfo(table_row, pdesc, N_("Name"), GPlugin->name);
  table_row = add_GxsmPluginInfo(table_row, pdesc, N_("Description"), GPlugin->description);
  table_row = add_GxsmPluginInfo(table_row, pdesc, N_("Category"), GPlugin->category 
				 ? GPlugin->category : N_("(NULL): All"));
  table_row = add_GxsmPluginInfo(table_row, pdesc, N_("Authors"), GPlugin->authors);
  table_row = add_GxsmPluginInfo(table_row, pdesc, N_("Help"), GPlugin->help);
  table_row = add_GxsmPluginInfo(table_row, pdesc, N_("Info"), GPlugin->info);
  table_row = add_GxsmPluginInfo(table_row, pdesc, N_("Error"), GPlugin->errormsg);
  table_row = add_GxsmPluginInfo(table_row, pdesc, N_("Status"), GPlugin->status);

  label = gtk_hseparator_new (); /* ok, not really a label ... :) */
  gtk_table_attach (GTK_TABLE (pdesc->info_table), label,
		    0, 4, table_row, table_row+1, GTK_FILL, GTK_FILL, 3, 6);
  gtk_widget_show (label);
  table_row++;

  table_row = add_GxsmPluginInfo(table_row, pdesc, N_("Init"),      (GPlugin->init      ? N_("Yes") : N_("No")));
  table_row = add_GxsmPluginInfo(table_row, pdesc, N_("Query"),     (GPlugin->query     ? N_("Yes") : N_("No")));

  if(GPlugin->about){
    button = gtk_button_new_with_label (N_("Call"));
    g_signal_connect (G_OBJECT (button), "clicked",
			G_CALLBACK (exec_callback),
			(gpointer) GPlugin->about);
    gtk_table_attach (GTK_TABLE (pdesc->info_table), button,
		      2, 4, table_row, table_row+1, GTK_FILL, GTK_FILL, 3, 6);
    gtk_widget_show (button);
  }
  table_row = add_GxsmPluginInfo(table_row, pdesc, N_("About"),     (GPlugin->about     ? N_("Yes") : N_("No")));

  if(GPlugin->configure){
    button = gtk_button_new_with_label (N_("Call"));
    g_signal_connect (G_OBJECT (button), "clicked",
			G_CALLBACK (exec_callback),
			(gpointer) GPlugin->configure);
    gtk_table_attach (GTK_TABLE (pdesc->info_table), button,
		      2, 4, table_row, table_row+1, GTK_FILL, GTK_FILL, 3, 6);
    gtk_widget_show (button);
  }
  table_row = add_GxsmPluginInfo(table_row, pdesc, N_("Configure"), (GPlugin->configure ? N_("Yes") : N_("No")));

  table_row = add_GxsmPluginInfo(table_row, pdesc, N_("Run"),       (GPlugin->run       ? N_("Yes") : N_("No")));
  table_row = add_GxsmPluginInfo(table_row, pdesc, N_("Cleanup"),   (GPlugin->cleanup   ? N_("Yes") : N_("No")));

  label = gtk_hseparator_new (); // ok, not really a label ... :)
  gtk_table_attach (GTK_TABLE (pdesc->info_table), label,
		    0, 4, table_row, table_row+1, GTK_FILL, GTK_FILL, 3, 6);
  gtk_widget_show (label);
  table_row++;

  /* Remove old and replace with new */

  if (old_table)
    gtk_widget_destroy (old_table);

  if (old_align)
    gtk_widget_destroy (old_align);

  gtk_container_add (GTK_CONTAINER (pdesc->info_align),pdesc->info_table);

  gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (pdesc->descr_scroll), 
					 pdesc->info_align);

  gtk_widget_show (pdesc->info_table);
  gtk_widget_show (pdesc->info_align);

  return FALSE;
}

static void 
expand_to (PDesc        *pdesc,
	   GtkCTreeNode *parent)
{
  if(parent)
    {
      expand_to (pdesc, (GTK_CTREE_ROW (parent))->parent);
      gtk_ctree_expand (GTK_CTREE (pdesc->ctree), parent);      
    }
}

static gint
procedure_clist_select_callback (GtkWidget      *widget,
				 gint            row, 
				 gint            column, 
				 GdkEventButton *bevent,
				 gpointer        data)
{
  GxsmPlugin *GPlugin;
  GtkCTreeNode * found_node; 
  PDesc *pdesc = (PDesc*)data;

  g_return_val_if_fail (pdesc != NULL, FALSE);

  GPlugin = (GxsmPlugin *) gtk_clist_get_row_data (GTK_CLIST (widget), row);

  if (!GPlugin)
    return FALSE;

  /* Must select the correct one in the ctree structure */

  found_node = gtk_ctree_find_by_row_data (GTK_CTREE (pdesc->ctree),
					   NULL, GPlugin);
  
  if (found_node)
    {
      GtkCTreeRow   *ctr;
      GtkCTreeNode  *parent;
      gint sel_row;

      /* Make sure this is expanded */

      ctr = GTK_CTREE_ROW (found_node);

      parent = GTK_CTREE_NODE (ctr->parent);

      expand_to (pdesc, parent);

      sel_row = gtk_clist_find_row_from_data (GTK_CLIST (pdesc->ctree), GPlugin);

      gtk_widget_hide (pdesc->ctree); 
      gtk_widget_show (pdesc->ctree); 

      gtk_signal_handler_block_by_func (G_OBJECT(pdesc->ctree),
					G_CALLBACK (procedure_ctree_select_callback),
					pdesc);

      gtk_clist_select_row (GTK_CLIST (pdesc->ctree), sel_row, -1);  
      gtk_ctree_select (GTK_CTREE (pdesc->ctree), found_node);

      gtk_clist_moveto (GTK_CLIST (pdesc->ctree),
			sel_row,
			0,
			0.5, 0.5);

      gtk_signal_handler_unblock_by_func (G_OBJECT (pdesc->ctree),
					  G_CALLBACK (procedure_ctree_select_callback),
					  pdesc);

      pdesc->ctree_row = sel_row;
    }
  else
    {
      g_warning ("Failed to find node in ctree");
    }

  return procedure_general_select_callback (pdesc, GPlugin);
}

/* This was an attempt to get around a problem in gtk 
 * where the scroll windows that contain ctree and clist
 * refuse to respond to the moveto funcions unless the
 * widgets are visible.
 * Still did not work 100% even after this.
 * Note the scrollbars are in the correct position but the
 * widget needs to be redrawn at the correct location.
 */

static gint
page_select_callback (GtkNotebook     *notebook,
		      GtkNotebookPage *page,
		      guint            page_num,
		      gpointer         data)
{
  PDesc *pdesc = (PDesc *)data;

  if (page_num == 0)
    {
      gtk_clist_select_row (GTK_CLIST (pdesc->clist), pdesc->clist_row, -1);  
      gtk_clist_moveto (GTK_CLIST (pdesc->clist),
			pdesc->clist_row,
			0,
			0.5, 0.0);
    }
  else
    {
      gtk_clist_select_row (GTK_CLIST (pdesc->ctree), pdesc->ctree_row, -1);  
      gtk_clist_moveto (GTK_CLIST (pdesc->ctree),
			pdesc->ctree_row,
			0,
			0.5, 0.0);
    }

  return FALSE;
}

static gint
procedure_ctree_select_callback (GtkWidget *widget,
				 GtkWidget *row, 
				 gint       column, 
				 gpointer   data)
{
  GxsmPlugin *GPlugin;
  PDesc *pdesc;
  gboolean is_leaf;
  gint sel_row;

  /* row is not a leaf the we have no interest in it */

  gtk_ctree_get_node_info (GTK_CTREE (widget),
			   GTK_CTREE_NODE (row),
			   NULL,
			   NULL,
			   NULL,
			   NULL,
			   NULL,
			   NULL,
			   &is_leaf,
			   NULL);

  if (!is_leaf)
    return FALSE;

  pdesc = (PDesc*) data;

  GPlugin = (GxsmPlugin *) gtk_ctree_node_get_row_data (GTK_CTREE (widget),
						 GTK_CTREE_NODE (row));

  /* Must set clist to this one */
  /* Block signals */

  gtk_signal_handler_block_by_func (G_OBJECT (pdesc->clist),
				    G_CALLBACK (procedure_clist_select_callback),
				    pdesc);

  sel_row = gtk_clist_find_row_from_data (GTK_CLIST (pdesc->clist), GPlugin);
  gtk_clist_select_row (GTK_CLIST (pdesc->clist), sel_row, -1);  
  gtk_clist_moveto (GTK_CLIST (pdesc->clist),
		    sel_row,
		    0,
		    0.5, 0.5);

  gtk_signal_handler_unblock_by_func (G_OBJECT (pdesc->clist),
				      G_CALLBACK (procedure_clist_select_callback),
				      pdesc);

  pdesc->clist_row = sel_row;
  
  return procedure_general_select_callback (pdesc, GPlugin);
}

static GtkCTreeNode *
get_parent (PDesc       *pdesc,
	    GHashTable  *ghash,
	    gchar       *mpath)
{
    GtkCTreeNode *parent;
    GtkCTreeNode *last_parent;
    gchar *tmp_ptr;
    gchar *str_ptr;
    gchar *leaf_ptr;
    gchar *labels[3];
    
    if (mpath == NULL)
	return NULL; /* Parent is root */
    
    parent = (GtkCTreeNode *) g_hash_table_lookup (ghash, mpath);
    
    if (parent)
	{
	    /* found node */
	    return parent;
	}
    
    /* Next one up */
    tmp_ptr = g_strdup (mpath);
    
    str_ptr = strrchr (tmp_ptr,'/');
    
    if (str_ptr == NULL)
	{
	    /*       printf("Root node for %s\n",mpath); */
	    leaf_ptr = mpath;
	    if (tmp_ptr)
		    g_free (tmp_ptr);
	    tmp_ptr = g_strdup("<root>");
	    last_parent = NULL;
	}
    else
	{
	    leaf_ptr = g_strdup(str_ptr+1);
	    
	    *str_ptr = '\000';
	    
	    last_parent = get_parent (pdesc, ghash, tmp_ptr);
	}
    
    labels[0] = g_strdup (leaf_ptr);
    labels[1] = g_strdup (""); 
    labels[2] = g_strdup (""); 
    
    /*   printf("get_parent::creating node %s under %s\n",leaf_ptr,tmp_ptr); */
    
    parent = gtk_ctree_insert_node (GTK_CTREE (pdesc->ctree),
				    last_parent,
				    NULL,
				    labels,
				    4,
				    NULL,
				    NULL,
				    NULL,
				    NULL,
				    FALSE,
				    FALSE);
    
    g_hash_table_insert (ghash, mpath, parent); 
    
    g_free(labels[2]);
    g_free(labels[1]);
    g_free(labels[0]);
    
    return parent;
}

static void
insert_into_ctree (PDesc      *pdesc,
		   gchar      *name,
		   gchar      *xtimestr,
		   gchar      *menu_str,
		   gchar      *types_str,
		   GHashTable *ghash,
		   GxsmPlugin      *GPlugin)
{
  gchar *labels[3];
  gchar *str_ptr;
  gchar *tmp_ptr;
  //  gchar *leaf_ptr;
  GtkCTreeNode *parent = NULL;
  GtkCTreeNode *leaf_node = NULL;

  /* Find all nodes */
  /* Last one is the leaf part */

  tmp_ptr = g_strdup (menu_str);

  str_ptr = strrchr (tmp_ptr, '/');

  if (str_ptr == NULL)
    return; /* No node */

  //  leaf_ptr = g_strdup (str_ptr + 1);

  *str_ptr = '\000';

  /*   printf("inserting %s...\n",menu_str); */

  parent = get_parent (pdesc, ghash, tmp_ptr);

  /* Last was a leaf */
  //   printf("found leaf %s parent = %p\n",leaf_ptr,parent);
  labels[0] = g_strdup (name);
  labels[1] = g_strdup (xtimestr);
  labels[2] = g_strdup (types_str);

  leaf_node = gtk_ctree_insert_node (GTK_CTREE (pdesc->ctree),
				     parent,
				     NULL,
				     labels,
				     4,
				     NULL,
				     NULL,
				     NULL,
				     NULL,
				     TRUE,
				     FALSE);
  g_free(labels[2]);
  g_free(labels[1]);
  g_free(labels[0]);

  gtk_ctree_node_set_row_data (GTK_CTREE (pdesc->ctree), leaf_node, GPlugin);
}

void GxsmPlugin_free (gpointer p)
{
  //  GxsmPlugin *pinfo = (GxsmPlugin *) p;
  // may be used later to free some stuff...
}

static void
get_plugin_info (PDesc *pdesc,
                 const gchar *search_text)
{
  gint  row_count = 0;
  gchar *menu_strs;

  GHashTable* ghash = g_hash_table_new (g_str_hash, g_str_equal);

  // Gxsm Plugin Query...
  pdesc->num_plugins = listplugins_pi.app->GxsmPlugins->how_many();

  for( GList *node = listplugins_pi.app->GxsmPlugins->get_pluginlist();
       node; node = node->next )
    {
      GxsmPlugin *pinfo = (GxsmPlugin *) node->data;
      gchar *labels[4];

      if (search_text)
	if( !strstr( pinfo->menuentry, search_text) &&
	    !strstr( pinfo->name, search_text)
	    ){
	  continue;
	}

      labels[0] = g_strdup (pinfo->menupath);
      labels[1] = g_strdup (pinfo->menuentry);
      labels[2] = g_strdup (pinfo->name);
      labels[3] = g_strdup (pinfo->query ? "Query-SelfInstall" : pinfo->run ? "Run-Cb" : "Math-PlugIn");
      
      gtk_clist_insert (GTK_CLIST (pdesc->clist), row_count, labels);
      
      gtk_clist_set_row_data_full (GTK_CLIST (pdesc->clist), row_count,
				   pinfo, GxsmPlugin_free);
      
      row_count++;
      
      /* Now do the tree view.... */
      gtk_signal_handler_block_by_func (G_OBJECT(pdesc->ctree),
					G_CALLBACK (procedure_ctree_select_callback),
					pdesc);
      menu_strs = g_strconcat(pinfo->menupath, pinfo->menuentry, NULL);
      insert_into_ctree (pdesc,
			 labels[1], //pinfo->menuentry,
			 labels[2], //pinfo->name,
			 menu_strs,
			 labels[3],
			 ghash,
			 pinfo);
      g_free(labels[3]);
      g_free(labels[2]);
      g_free(labels[1]);
      g_free(labels[0]);
      g_free(menu_strs);

      gtk_signal_handler_unblock_by_func (G_OBJECT (pdesc->ctree),
					  G_CALLBACK (procedure_ctree_select_callback),
					  pdesc);
  }

  // HwI Plugin(s)
  for( GList *node = listplugins_pi.app->xsm->HwI_plugins->get_pluginlist();
       node; node = node->next )
    {
      GxsmPlugin *pinfo = (GxsmPlugin *) node->data;
      gchar *labels[4];

      if (search_text)
	if( !strstr( pinfo->menuentry, search_text) &&
	    !strstr( pinfo->name, search_text)
	    ){
	  continue;
	}

      labels[0] = g_strdup (pinfo->menupath);
      labels[1] = g_strdup (pinfo->menuentry);
      labels[2] = g_strdup (pinfo->name);
      labels[3] = g_strdup (pinfo->query ? "HwI-Query-SelfInstall" : pinfo->run ? "HwI-Run-Cb" : "HwI-PlugIn");
      
      gtk_clist_insert (GTK_CLIST (pdesc->clist), row_count, labels);
      
      gtk_clist_set_row_data_full (GTK_CLIST (pdesc->clist), row_count,
				   pinfo, GxsmPlugin_free);
      
      row_count++;
      
      /* Now do the tree view.... */
      gtk_signal_handler_block_by_func (G_OBJECT(pdesc->ctree),
					G_CALLBACK (procedure_ctree_select_callback),
					pdesc);
      menu_strs = g_strconcat(pinfo->menupath, pinfo->menuentry, NULL);
      insert_into_ctree (pdesc,
			 labels[1], //pinfo->menuentry,
			 labels[2], //pinfo->name,
			 menu_strs,
			 labels[3],
			 ghash,
			 pinfo);
      g_free(labels[3]);
      g_free(labels[2]);
      g_free(labels[1]);
      g_free(labels[0]);
      g_free(menu_strs);

      gtk_signal_handler_unblock_by_func (G_OBJECT (pdesc->ctree),
					  G_CALLBACK (procedure_ctree_select_callback),
					  pdesc);
  }
}


static void 
dialog_search_callback (GtkDialog *Dlg, 
			gpointer   data)
{
  PDesc *pdesc = (PDesc*)data;
  const gchar *search_text = NULL;
 
  if (Dlg != NULL)
    {
      /* The result of a button press... read entry data */
      search_text = gtk_entry_get_text (GTK_ENTRY (plugindesc->search_entry));
    }

  gtk_clist_freeze (GTK_CLIST (pdesc->ctree));
  gtk_clist_clear (GTK_CLIST (pdesc->ctree));
  gtk_clist_freeze (GTK_CLIST (pdesc->clist));
  gtk_clist_clear (GTK_CLIST (pdesc->clist));

  get_plugin_info (pdesc, search_text);

  gtk_clist_columns_autosize (GTK_CLIST (plugindesc->clist));

  gtk_clist_sort (GTK_CLIST (pdesc->clist));
  gtk_clist_thaw (GTK_CLIST (pdesc->clist));
  gtk_ctree_sort_recursive (GTK_CTREE (pdesc->ctree), NULL);
  gtk_clist_thaw (GTK_CLIST (pdesc->ctree));
}


static gint
name_sort (GtkCList      *clist,
	   gconstpointer  ptr1,
	   gconstpointer  ptr2)
{
  GtkCListRow *row1 = (GtkCListRow *) ptr1;
  GtkCListRow *row2 = (GtkCListRow *) ptr2;

  /* Get the data for the row */
  GxsmPlugin *row1_GxsmPlugin = (GxsmPlugin *) row1->data;
  GxsmPlugin *row2_GxsmPlugin = (GxsmPlugin *) row2->data;

  /* Want to sort on the date field */

  if (row2_GxsmPlugin->name < row1_GxsmPlugin->name)
    {
      return -1;
    }

  if (row2_GxsmPlugin->name > row1_GxsmPlugin->name)
    {
      return 1;
    }

  return 0;
}


static void 
clist_click_column (GtkCList *clist, 
		    gint      column, 
		    gpointer  data)
{
  if (column == 1)
    {
      gtk_clist_set_compare_func (clist, name_sort);
    }
  else
    {
      gtk_clist_set_compare_func (clist, NULL); /* Set back to default */
    }

  if (column == clist->sort_column)
    {
      if (clist->sort_type == GTK_SORT_ASCENDING)
	clist->sort_type = GTK_SORT_DESCENDING;
      else
	clist->sort_type = GTK_SORT_ASCENDING;
    }
  else
    gtk_clist_set_sort_column (clist, column);
  
  gtk_clist_sort (clist);
}
