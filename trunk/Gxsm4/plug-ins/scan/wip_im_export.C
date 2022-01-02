/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 8 c-style: "K&R" -*- */

/* Gnome gxsm - Gnome X Scanning Microscopy
 * universal STM/AFM/SARLS/SPALEED/... controlling and
 * data analysis software
 *
 * Gxsm Plugin Name: wip_im_export.C
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
% PlugInDocuCaption: WIP (WiTeC) Import
% PlugInName: wip_im_Export
% PlugInAuthor: Percy Zahl
% PlugInAuthorEmail: zahl@users.sf.net
% PlugInMenuPath: File/Import/WIP

% PlugInDescription
The \GxsmEmph{wip\_im\_export} plug-in allows importing selected data sets of WIP (WiTeC-Project) files.

% PlugInUsage
The plug-in is called by \GxsmMenu{File/Import/WIP}.

%% OptPlugInKnownBugs

%% OptPlugInRefs


% EndPlugInDocuSection
 * -------------------------------------------------------------------------------- 
 */

#include <gtk/gtk.h>
#include "config.h"
#include "core-source/plugin.h"
#include "core-source/dataio.h"
#include "core-source/action_id.h"
#include "core-source/util.h"
#include "core-source/xsmtypes.h"
#include "core-source/glbvars.h"
#include "core-source/gapp_service.h"

// custom includes go here
// -- START EDIT --
// -- END EDIT --

// WIP file header
typedef struct {
        gchar  tag_name[8]; // WIP file id tag "WIT_PRCT" for project file
} WIP_header_tag;

// WIP tag part initial
typedef struct {
        gint32 name_length;
} WIP_tag_i;

// WIP tag final part
typedef struct {
	gint32 kind;
	gint32 data_start_position;
        gint32 t1;
	gint32 data_stop_position;
        gint32 t2;
        //	guint64 data_start_position;
        //	guint64 data_stop_position;
} WIP_tag_f;


typedef struct {
        gint64 size;
        gchar *name;
        WIP_tag_f tag;
        gchar *data;
} WIP_data;

// WIP DATA kinds
typedef enum { 
        WIP_LIST=0, // List of tags
	WIP_INT64=1, WIP_INT32=2, WIP_INT16=3, WIP_INT8=4,
	WIP_UINT32=5, WIP_UINT16=6, WIP_UINT8=7,
	WIP_BOOL=8,     // 1 byte
	WIP_FLOAT=9,    // 4 byte floating
	WIP_DOUBLE=10,  // 8 byte floating point
	WIP_EXTENDED=11 // 10 byte floating point
} WIP_data_kind;

// WIP TAG kinds
typedef enum { 
        WIP_TAG_LIST=0, // Tag List of one or more
	WIP_TAG_EXTENDED=1, // 10 byte float
	WIP_TAG_DOUBLE=2, // 8 byte float
	WIP_TAG_FLOAT=3, // 4 byte float
	WIP_TAG_LARGE_INT=4, // 8 byte int
	WIP_TAG_INT=5, //4 byte int
	WIP_TAG_WORD=6, // 4 byte unsigned int
	WIP_TAG_BYTE=7, // 1 byte
	WIP_TAG_BOOL=8, // 1 byte bool
	WIP_TAG_STRING=9 //4 byte int = #chars followed by n chars
} WIP_tag_kind;

const gchar *tag_kind_lookup[] = {
        "LIST",
        "EXTENDED",
        "DOUBLE",
        "FLOAT",
        "LARGE_INT",
        "INTEGER",
        "WORD",
        "BYTE",
        "BOOL",
        "STRING",
        "10???"
};

static inline guint16 swap_u16 (guint16 x){
        return ((x & 0xff) << 8) | ((x >> 8) & 0xff);
}

static inline guint32 swap_u32 (guint32 x){
        return ((x & 0xff) << 24) | (((x >> 8)  & 0xff) << 16) | (((x >> 16) & 0xff) << 8) | ((x >> 24) & 0xff);
}

static inline guint64 swap_u64 (guint64 x){
        return ( (((x      ) & 0xff) << 56) | (((x >>  8)  & 0xff) << 48) | (((x >> 16) & 0xff) << 40) | (((x >> 24) & 0xff) << 32)	
                 | (((x >> 32) & 0xff) << 24) | (((x >> 40)  & 0xff) << 16) | (((x >> 48) & 0xff) <<  8) | (((x >> 56) & 0xff)      ) );
}
// -mno-strict-align ??? 
static inline gfloat check_and_swap_float (gfloat *x){
	if (WORDS_BIGENDIAN){
		guint32 u32 = * ((guint32*)x);
		gfloat   f;
		u32 = swap_u32 (u32);
		f = * ((gfloat*)(&u32));
		return f;
	} else return *x;
}


// enable std namespace
using namespace std;

// make a new derivate of the base class "Dataio"
class Wip_ImExportFile : public Dataio{
public:
	Wip_ImExportFile(Scan *s, const char *n); 
	virtual ~Wip_ImExportFile();
	virtual FIO_STATUS Read(xsm::open_mode mode=xsm::open_mode::replace);
	virtual FIO_STATUS Write();
        gint64 read_tag (gint64 start=8, gint64 stop=-1, WIP_data *wip_data=NULL, int level=0, GtkTreeIter *toplevel=NULL, int newlevel=1); 
        GtkTreeModel *create_and_fill_model (void);
        GtkWidget *create_view_and_model (void);
        void wip_im_export_configure_select ();

private:
        FIO_STATUS import_data(const char *fname); 
	ifstream f;
        
        GtkTreeStore  *treestore;
        GtkTreeIter   data_caption;
};

// Plugin Prototypes
static void wip_im_export_init (void);
static void wip_im_export_query (void);
static void wip_im_export_about (void);
static void wip_im_export_configure_select (Wip_ImExportFile *w);
static void wip_im_export_cleanup (void);

static void wip_im_export_filecheck_load_callback (gpointer data );
static void wip_im_export_filecheck_save_callback (gpointer data );

static void wip_im_export_import_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data);
static void wip_im_export_export_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data);

// Fill in the GxsmPlugin Description here
GxsmPlugin wip_im_export_pi = {
        NULL,                   // filled in and used by Gxsm, don't touch !
        NULL,                   // filled in and used by Gxsm, don't touch !
        0,                      // filled in and used by Gxsm, don't touch !
        NULL,                   // The Gxsm-App Class Ref.pointer (called "gapp" in Gxsm) is 
        // filled in here by Gxsm on Plugin load, 
        // just after init() is called !!!
        // -- START EDIT --
        "WIP-ImExport",            // PlugIn name
        NULL,                   // PlugIn's Categorie, set to NULL for all, I just don't want this always to be loaded!
        // Description, is shown by PluginViewer (Plugin: listplugin, Tools->Plugin Details)
        NULL,
        "Percy Zahl",
        "file-import-section,file-export-section", // sep. im/export menuentry path by comma!
        N_("WIP,WIP"), // menu entry (same for both)
        N_("WIP import,WIP export"), // short help for menu entry
        N_("WIP: WiTeC Project import filter -- experimental, read/view tree model only. Need for data selection."), // info
        // -- END EDIT --
        NULL,          // error msg, plugin may put error status msg here later
        NULL,          // Plugin Status, managed by Gxsm, plugin may manipulate it too
        wip_im_export_init,
        wip_im_export_query,
        // about-function, can be "NULL"
        // can be called by "Plugin Details"
        wip_im_export_about,
        // configure-function, can be "NULL"
        // can be called by "Plugin Details"
        NULL, //wip_im_export_configure,
        // run-function, can be "NULL", if non-Zero and no query defined, 
        // it is called on menupath->"plugin"
        NULL,
        // cleanup-function, can be "NULL"
        // called if present at plugin removal
        wip_im_export_import_callback, // direct menu entry callback1 or NULL
        wip_im_export_export_callback, // direct menu entry callback2 or NULL

        wip_im_export_cleanup
};

// Text used in Aboutbox, please update!!
static const char *about_text = N_("This GXSM plugin imports WIP (UView) image data files (CCD/ELMITEC-LEEM)");

static const char *file_mask = "*.da[tv]";

int offset_index_value=0;
int step_index_value=1;
int max_index_value=1;
double start_value=0.;
double step_value=1.;

int offset_index_time=0;
int step_index_time=1;
int max_index_time=1;
double start_time=0.;
double step_time=1.;

double realtime0=0.;
double realtime0_user=0.;

double Temp_Offset=0.;

#define FILE_DIM_V 0x01
#define FILE_DIM_T 0x02
#define FILE_DIM_VIDEO 0x04
int file_dim=0;

// Symbol "get_gxsm_plugin_info" is resolved by dlsym from Gxsm, used to get Plugin's info!! 
// Essential Plugin Function!!
GxsmPlugin *get_gxsm_plugin_info ( void ){ 
        wip_im_export_pi.description = g_strdup_printf(N_("Gxsm im_export plugin %s"), VERSION);
        return &wip_im_export_pi; 
}

// Query Function, installs Plugin's in File/Import and Export Menupaths!
// ----------------------------------------------------------------------
// Import Menupath is "File/Import/PNG"
// Export Menupath is "File/Export/PNGt"
// ----------------------------------------------------------------------

static void wip_im_export_query(void)
{
	if(wip_im_export_pi.status) g_free(wip_im_export_pi.status); 
	wip_im_export_pi.status = g_strconcat (
                                               N_("Plugin query has attached "),
                                               wip_im_export_pi.name, 
                                               N_(": File IO Filters are ready to use."),
                                               NULL);
	
	// register this plugins filecheck functions with Gxsm now!
	// This allows Gxsm to check files from DnD, open, 
	// and cmdline sources against all known formats automatically - no explicit im/export is necessary.
	wip_im_export_pi.app->ConnectPluginToLoadFileEvent (wip_im_export_filecheck_load_callback);
	//	wip_im_export_pi.app->ConnectPluginToSaveFileEvent (wip_im_export_filecheck_save_callback);
}


// 5.) Start here with the plugins code, vars def., etc.... here.
// ----------------------------------------------------------------------
//


// init-Function
static void wip_im_export_init(void)
{
	PI_DEBUG (DBG_L2, wip_im_export_pi.name << " Plugin Init");
}

// about-Function
static void wip_im_export_about(void)
{
	const gchar *authors[] = { wip_im_export_pi.authors, NULL};
	gtk_show_about_dialog (NULL,
			       "program-name",  wip_im_export_pi.name,
			       "version", VERSION,
			       "license", GTK_LICENSE_GPL_3_0,
			       "comments", about_text,
			       "authors", authors,
			       NULL
			       );
}

// cleanup-Function, remove all "custom" menu entrys here!
static void wip_im_export_cleanup(void)
{
	PI_DEBUG (DBG_L2, "Plugin Cleanup done.");
}


enum
        {
                COL_TAG_NAME = 0,
                COL_TAG_KIND,
                COL_TAG_ITEMS,
                COL_TAG_DATA,
                COL_TAG_START,
                NUM_COLS
        } ;

void age_cell_data_func (GtkTreeViewColumn *col,
                         GtkCellRenderer   *renderer,
                         GtkTreeModel      *model,
                         GtkTreeIter       *iter,
                         gpointer           user_data)
{
        guint  items;
        gchar  buf[64];

        gtk_tree_model_get(model, iter, COL_TAG_ITEMS, &items, -1);

        if (items > 0)
                {
                        g_snprintf(buf, sizeof(buf), "%d", items);

                        g_object_set(renderer, "foreground-set", FALSE, NULL); /* print this normal */
                }
        else
                {
                        g_snprintf(buf, sizeof(buf), "invalid");
                        g_object_set(renderer, "foreground", "Red", "foreground-set", TRUE, NULL); /* make red */
                }

        g_object_set(renderer, "text", buf, NULL);
}

GtkTreeModel *Wip_ImExportFile::create_and_fill_model (void)
{

        treestore = gtk_tree_store_new(NUM_COLS, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_UINT, G_TYPE_STRING, G_TYPE_UINT64);

#if 0
        GtkTreeIter    toplevel, child;

        /* Append a top level row and leave it empty */
        gtk_tree_store_append(treestore, &toplevel, NULL);
        gtk_tree_store_set(treestore, &toplevel,
                           COL_TAG_NAME, "Maria",
                           COL_TAG_KIND, "Incognito",
                           -1);

        /* Append a second top level row, and fill it with some data */
        gtk_tree_store_append(treestore, &toplevel, NULL);
        gtk_tree_store_set(treestore, &toplevel,
                           COL_TAG_NAME, "Jane",
                           COL_TAG_KIND, "Average",
                           COL_TAG_ITEMS, (guint) 1962,
                           -1);

        /* Append a child to the second top level row, and fill in some data */
        gtk_tree_store_append(treestore, &child, &toplevel);
        gtk_tree_store_set(treestore, &child,
                           COL_TAG_NAME, "Janinita",
                           COL_TAG_KIND, "Average",
                           COL_TAG_ITEMS, (guint) 1985,
                           COL_TAG_DATA, "Test",
                           -1);
#endif

        read_tag ();

        return GTK_TREE_MODEL(treestore);
}


GtkWidget *Wip_ImExportFile::create_view_and_model (void)
{
	GtkTreeViewColumn   *col;
	GtkCellRenderer     *renderer;
	GtkWidget           *view;
	GtkTreeModel        *model;

	view = gtk_tree_view_new();

	/* --- Column #1 --- */

	col = gtk_tree_view_column_new();

	gtk_tree_view_column_set_title(col, "Tag Name");

	/* pack tree view column into tree view */
	gtk_tree_view_append_column(GTK_TREE_VIEW(view), col);

	renderer = gtk_cell_renderer_text_new();

	/* pack cell renderer into tree view column */
	gtk_tree_view_column_pack_start(col, renderer, TRUE);

	/* connect 'text' property of the cell renderer to model column that contains the first name */
	gtk_tree_view_column_add_attribute(col, renderer, "text", COL_TAG_NAME);


	/* --- Column #2 --- */

	col = gtk_tree_view_column_new();

	gtk_tree_view_column_set_title(col, "Kind");

	/* pack tree view column into tree view */
	gtk_tree_view_append_column(GTK_TREE_VIEW(view), col);

	renderer = gtk_cell_renderer_text_new();

	/* pack cell renderer into tree view column */
	gtk_tree_view_column_pack_start(col, renderer, TRUE);

	/* connect 'text' property of the cell renderer to model column that contains the last name */
	gtk_tree_view_column_add_attribute(col, renderer, "text", COL_TAG_KIND);

	/* set 'weight' property of the cell renderer to bold print (we want all last names in bold) */
	g_object_set(renderer, "weight", PANGO_WEIGHT_BOLD, "weight-set", TRUE, NULL);


	/* --- Column #3 --- */

	col = gtk_tree_view_column_new();

	gtk_tree_view_column_set_title(col, "#Items");

	/* pack tree view column into tree view */
	gtk_tree_view_append_column(GTK_TREE_VIEW(view), col);

	renderer = gtk_cell_renderer_text_new();

	/* pack cell renderer into tree view column */
	gtk_tree_view_column_pack_start(col, renderer, TRUE);

	/* connect a cell data function */
	gtk_tree_view_column_set_cell_data_func(col, renderer, age_cell_data_func, NULL, NULL);


	model = create_and_fill_model();

	gtk_tree_view_set_model(GTK_TREE_VIEW(view), model);

	g_object_unref(model); /* destroy model automatically with view */

	gtk_tree_selection_set_mode(gtk_tree_view_get_selection(GTK_TREE_VIEW(view)), GTK_SELECTION_NONE);

	/* --- Column #4 --- */

	col = gtk_tree_view_column_new();

	gtk_tree_view_column_set_title(col, "Content");

	/* pack tree view column into tree view */
	gtk_tree_view_append_column(GTK_TREE_VIEW(view), col);

	renderer = gtk_cell_renderer_text_new();

	/* pack cell renderer into tree view column */
	gtk_tree_view_column_pack_start(col, renderer, TRUE);

	/* connect 'text' property of the cell renderer to model column that contains the first name */
	gtk_tree_view_column_add_attribute(col, renderer, "text", COL_TAG_DATA);


	return view;
}

// configure-Function
void Wip_ImExportFile::wip_im_export_configure_select ()
{
	if(wip_im_export_pi.app){
		XsmRescourceManager xrm("WIP_IM_EXPORT");
		GtkWidget *dialog = gtk_dialog_new_with_buttons (N_("WITeC Project (WIP) Import"),
								 NULL,
								 (GtkDialogFlags)(GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT),
								 _("_OK"), GTK_RESPONSE_ACCEPT,
                                                                 _("_Cancel"), GTK_RESPONSE_CANCEL,
								 NULL);
		

                GtkWidget *scrolled_contents = gtk_scrolled_window_new ();
                gtk_widget_set_hexpand (scrolled_contents, TRUE);
                gtk_widget_set_vexpand (scrolled_contents, TRUE);

                gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_contents), 
                                                GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

		gtk_box_append (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG(dialog))), scrolled_contents);

		GtkWidget *view = create_view_and_model ();
                gtk_scrolled_window_set_child (GTK_SCROLLED_WINDOW (scrolled_contents), view);

		gtk_widget_show (dialog);

                int response = GTK_RESPONSE_NONE;
                g_signal_connect (G_OBJECT (dialog), "response", G_CALLBACK (GnomeAppService::on_dialog_response_to_user_data), &response);
        
                // FIX-ME GTK4 ??
                // wait here on response
                while (response == GTK_RESPONSE_NONE)
                        while(g_main_context_pending (NULL)) g_main_context_iteration (NULL, FALSE);
	}
}




gint64 Wip_ImExportFile::read_tag (gint64 start, gint64 stop, WIP_data *wip_data, int level, GtkTreeIter *toplevel, int newlevel){
	WIP_tag_i tag_i;
	WIP_data  tag_data;
	int data_size=1;
	GtkTreeIter child;
	GtkTreeIter toplevel_local;
	if (toplevel)
                memcpy (&toplevel_local, toplevel, sizeof(toplevel_local));

	gchar *lv=new gchar[level+1];
	memset (lv, '.', level+1);
	lv[level]=0;

	//	std::cout << "---THIS TAG start: " << start << " :" << std::endl;

	f.seekg (start);
	f.read ((char*)&tag_i, sizeof (WIP_tag_i));

	//	std::cout << "TAG name len: " << start << " :" << tag_i.name_length << std::endl;

	if (tag_i.name_length > 0 && tag_i.name_length < 1000){
                tag_data.name = new gchar[tag_i.name_length+1];

                memset (tag_data.name, 0, tag_i.name_length+1);
                f.read ((char*)tag_data.name, tag_i.name_length);

                memset (&tag_data.tag, 0, sizeof (WIP_tag_f));
                f.read ((char*)&tag_data.tag, sizeof (WIP_tag_f));

                tag_data.size = tag_data.tag.data_stop_position-tag_data.tag.data_start_position; 

                gchar *preview = NULL;
                gchar *tmp1 = NULL;
                gchar *tmp2 = NULL;
                tag_data.data = NULL;
                if (tag_data.size < 100 && tag_data.size > 0  && tag_data.tag.kind > 0){ // just for preview if small
                        //	    std::cout << "data read len: " << tag_data.size << ", kind=" << tag_data.tag.kind << std::endl;
                        tag_data.data = new gchar[tag_data.size];
                        f.read ((char*)tag_data.data, tag_data.size);
                        //	    std::cout << "data read OK." << std::endl;
	  
                        switch (tag_data.tag.kind){
                        case WIP_TAG_STRING:{
                                int len = (int)*(gint32*)tag_data.data;
                                if (len > 0 && len < 1000){
                                        tmp1 = g_strndup (tag_data.data+4, len);
                                        preview = g_strdup_printf("%s", tmp1);
                                        g_free (tmp1);
                                } else
                                        preview = g_strdup_printf("[%d] -N/A-", len);
                        }
                                break;
                        case WIP_TAG_LARGE_INT:
                                data_size=8;
                                preview = g_strdup_printf ("%d", (int)*((gint64*)tag_data.data));
                                for (int i=1; i<tag_data.size/data_size; ++i){
                                        tmp1 = preview;
                                        preview = g_strconcat(tmp1, tmp2=g_strdup_printf (", %d", (int)*(((gint64*)tag_data.data)+i)), NULL);
                                        g_free (tmp1);
                                        g_free (tmp2);
                                }
                                break;
                        case WIP_TAG_INT:
                                data_size=4;
                                preview = g_strdup_printf ("%d", (int)*((gint32*)tag_data.data));
                                for (int i=1; i<tag_data.size/data_size; ++i){
                                        tmp1 = preview;
                                        preview = g_strconcat(tmp1, tmp2=g_strdup_printf (", %d", (int)*(((gint32*)tag_data.data)+i)), NULL);
                                        g_free (tmp1);
                                        g_free (tmp2);
                                }
                                break;
                        case WIP_TAG_WORD:
                                data_size=2;
                                preview = g_strdup_printf ("%d", (int)*((guint16*)tag_data.data));
                                for (int i=1; i<tag_data.size/data_size; ++i){
                                        tmp1 = preview;
                                        preview = g_strconcat(tmp1, tmp2=g_strdup_printf (", %d", (int)*(((guint16*)tag_data.data)+i)), NULL);
                                        g_free (tmp1);
                                        g_free (tmp2);
                                }
                                break;
                        case WIP_TAG_BYTE:
                                data_size=1;
                                preview = g_strdup_printf ("%d", (int)*((gint8*)tag_data.data));
                                for (int i=1; i<tag_data.size/data_size; ++i){
                                        tmp1 = preview;
                                        preview = g_strconcat(tmp1, tmp2=g_strdup_printf (", %d", (int)*(((gint8*)tag_data.data)+i)), NULL);
                                        g_free (tmp1);
                                        g_free (tmp2);
                                }
                                break;
                        case WIP_TAG_BOOL:
                                data_size=1;
                                preview = g_strdup_printf ("%s", (int)*((gint8*)tag_data.data)?"true":"false");
                                for (int i=1; i<tag_data.size/data_size; ++i){
                                        tmp1 = preview;
                                        preview = g_strconcat(tmp1, tmp2=g_strdup_printf (", %s", (int)*(((gint8*)tag_data.data)+i)?"T":"F"), NULL);
                                        g_free (tmp1);
                                        g_free (tmp2);
                                }
                                break;
                        case WIP_TAG_FLOAT:
                                data_size=4;
                                preview = g_strdup_printf ("%f", *((float*)tag_data.data));
                                for (int i=1; i<tag_data.size/data_size; ++i){
                                        tmp1 = preview;
                                        preview = g_strconcat(tmp1, tmp2=g_strdup_printf (", %f", *(((float*)tag_data.data)+i)), NULL);
                                        g_free (tmp1);
                                        g_free (tmp2);
                                }
                                break;
                        case WIP_TAG_DOUBLE:
                                data_size=8;
                                preview = g_strdup_printf ("%f", *((double*)tag_data.data));
                                for (int i=1; i<tag_data.size/data_size; ++i){
                                        tmp1 = preview;
                                        preview = g_strconcat(tmp1, tmp2=g_strdup_printf (", %f", (double) *(((double*)tag_data.data)+i)), NULL);
                                        g_free (tmp1);
                                        g_free (tmp2);
                                }
                                break;
                        case WIP_TAG_EXTENDED: // 10 byte floating point
                                data_size=10;
                                preview = g_strdup_printf ("%f", (double) *((long double*)tag_data.data));
                                for (int i=1; i<tag_data.size/data_size; ++i){
                                        tmp1 = preview;
                                        preview = g_strconcat(tmp1, tmp2=g_strdup_printf (", %f", (double) *(((long double*)tag_data.data)+i)), NULL);
                                        g_free (tmp1);
                                        g_free (tmp2);
                                }
                                break;
                        }
                        g_free (tag_data.data);
                }
                if (!preview){
                        if (tag_data.tag.kind == WIP_LIST)
                                preview = g_strdup(lv);
                        else
                                preview = g_strdup("[...] big data array or invalid data size");
                }

#if 0
                std::cout << lv << "+" << tag_data.name << ": K=" << tag_data.tag.kind
                          << ", data size: " << tag_data.size << ", =" << preview << std::endl;
#endif

                if (newlevel){
                        /* Append new level */
                        gtk_tree_store_append(treestore, &child, toplevel);
                        gtk_tree_store_set(treestore, &child,
                                           COL_TAG_NAME, tag_data.name ,
                                           COL_TAG_KIND, tag_kind_lookup[tag_data.tag.kind],
                                           COL_TAG_ITEMS, tag_data.size/data_size,
                                           COL_TAG_DATA, preview,
                                           COL_TAG_START, start,
                                           -1);
                } else {
                        /* Append at current level */
                        gtk_tree_store_append(treestore, &child, &toplevel_local);
                        gtk_tree_store_set(treestore, &child,
                                           COL_TAG_NAME, tag_data.name ,
                                           COL_TAG_KIND, tag_kind_lookup[tag_data.tag.kind],
                                           COL_TAG_ITEMS, tag_data.size/data_size,
                                           COL_TAG_DATA, preview,
                                           COL_TAG_START, start,
                                           -1);
                }

                if (level == 2 && strncmp( tag_data.name, "Data", 4)==0)
                        memcpy (&data_caption, &child, sizeof(data_caption));

                if (strncmp( tag_data.name, "Caption", 7)==0)
                        gtk_tree_store_set(treestore, &data_caption,
                                           COL_TAG_DATA, preview,
                                           -1);

                g_free (preview);
#if 0	  
                std::cout << "TAG data size: " << tag_data.size << std::endl;
                std::cout << "TAG data start: " << (tag_data.tag.data_start_position) << std::endl;
                std::cout << "TAG data stop: " << (tag_data.tag.data_stop_position) << std::endl;

                for (int i=0; i<sizeof(WIP_tag_f); ++i)
                        //	    std::setf(std::hex);
                        std::cout << " " << (int)((guint8)(*((char*)(&tag_data.tag)+i)));
                std::cout << std::endl;
#endif
                //	f.seek (tag_data.tag.data_start_position);
	  
                if (tag_data.tag.kind == WIP_LIST)
                        read_tag (tag_data.tag.data_start_position, tag_data.tag.data_stop_position, 
                                  NULL, level+1, &child, 1);

                if (level>0 && tag_data.tag.data_stop_position < stop)
                        read_tag (tag_data.tag.data_stop_position, stop, NULL, level, &toplevel_local, 0);

	}

	return 0;
}

Wip_ImExportFile::Wip_ImExportFile(Scan *s, const char *n) : Dataio(s,n){
        treestore = NULL;
}

Wip_ImExportFile::~Wip_ImExportFile(){
}

FIO_STATUS Wip_ImExportFile::Read(xsm::open_mode mode){
	FIO_STATUS ret;
	gchar *fname=NULL;

	PI_DEBUG (DBG_L2, "reading");

	fname = (gchar*)name;

	// name should have at least 4 chars: ".ext"
	if (fname == NULL || strlen(fname) < 4)
		return status=FIO_NOT_RESPONSIBLE_FOR_THAT_FILE;

	if (strncasecmp (fname+strlen(fname)-4,".WIP", 4) && strncasecmp (fname+strlen(fname)-4,".wip", 4))
		return status=FIO_NOT_RESPONSIBLE_FOR_THAT_FILE;

	realtime0=0.;
	file_dim=0;

	// check for file exists and is OK !
	// else open File Dlg
	ifstream f;
	f.open(fname, ios::in);
	if(!f.good()){
                PI_DEBUG (DBG_L2, "Error at file open. File not good/readable.");
                return status=FIO_OPEN_ERR;
	}
	f.close();
	scan->SetVM(SCAN_V_DIRECT);
		
	// Check all known File Types:
	if ((ret=import_data (fname)) !=  FIO_NOT_RESPONSIBLE_FOR_THAT_FILE)
                return ret;
	
	return  status=FIO_NOT_RESPONSIBLE_FOR_THAT_FILE;
}

FIO_STATUS Wip_ImExportFile::import_data(const char *fname){
	WIP_header_tag header_tag;
	WIP_tag_i tag_i;
	
	gchar *error_msg = NULL;

	f.open(fname, ios::in);
	if(!f.good())
		return FIO_OPEN_ERR;

	std::cout << "WIP FILE READ" << std::endl;

	// Checking resposibility for this file as good as possible, use
	// extension(s) (most simple), magic numbers, etc.

	f.read((char*)&header_tag, sizeof(header_tag)); // read header
	std::cout << "Found: >>" << header_tag.tag_name << " <<" << std::endl;

        // file type sanity check
	if(strncmp ((char*)&header_tag, "WIT_PRCT", 8)){ // check GME type, skip, do not complain!
                std::cout << "not a WiTeC Project file: >>" << header_tag.tag_name << " <<" << std::endl;
		f.close ();
		return FIO_NOT_RESPONSIBLE_FOR_THAT_FILE;
	}

#if 0
	if (WORDS_BIGENDIAN){
		uks_fileheader.size    = swap_u16(uks_fileheader.size);
		uks_fileheader.starttime   = swap_u64(uks_fileheader.starttime);

	}
#endif

	wip_im_export_configure_select ();

	scan->data.s.ntimes = 1;

  	return status=FIO_OK; 
}

FIO_STATUS Wip_ImExportFile::Write(){
	// start exporting -------------------------------------------
	return status=FIO_OK; 
}

// Plugin's Notify Cb's, registered to be called on file load/save to check file
// return via filepointer, it is set to Zero or passed as Zero if file has been processed!
// That's all fine, you should just change the Text Stings below...


static void wip_im_export_filecheck_load_callback (gpointer data ){
	gchar **fn = (gchar**)data;
	if (*fn){
		PI_DEBUG (DBG_L2, "checking for WIP file type >" << *fn << "<" );

		Scan *dst = gapp->xsm->GetActiveScan();
		if(!dst){ 
			gapp->xsm->ActivateFreeChannel();
			dst = gapp->xsm->GetActiveScan();
		}
		Wip_ImExportFile fileobj (dst, *fn);

		FIO_STATUS ret = fileobj.Read(); 
		if (ret != FIO_OK){ 
			// I'am responsible! (But failed)
			if (ret != FIO_NOT_RESPONSIBLE_FOR_THAT_FILE)
				*fn=NULL;
			// no more data: remove allocated and unused scan now, force!
                        //			gapp->xsm->SetMode(-1, ID_CH_M_OFF, TRUE); 
			PI_DEBUG (DBG_L2, "Read Error " << ((int)ret) );
		}else{
			// got it!
			*fn=NULL;

			// Now update gxsm main window data fields
			gapp->xsm->ActiveScan->GetDataSet(gapp->xsm->data);
			gapp->spm_update_all();
			dst->draw();
		}
	}else{
		PI_DEBUG (DBG_L2, "Skipping" << *fn << "<" );
	}
}

static void wip_im_export_filecheck_save_callback (gpointer data ){
	gchar **fn = (gchar**)data;
	if (*fn){
		Scan *src;
		PI_DEBUG (DBG_L2, "Saving/(checking) >" << *fn << "<" );

		Wip_ImExportFile fileobj (src = gapp->xsm->GetActiveScan(), *fn);

		FIO_STATUS ret;
		ret = fileobj.Write(); 

		if(ret != FIO_OK){
			// I'am responsible! (But failed)
			if (ret != FIO_NOT_RESPONSIBLE_FOR_THAT_FILE)
				*fn=NULL;
			PI_DEBUG (DBG_L2, "Write Error " << ((int)ret) );
		}else{
			// write done!
			*fn=NULL;
		}
	}else{
		PI_DEBUG (DBG_L2, "Skipping >" << *fn << "<" );
	}
}

// Menu callback functions -- usually no need to edit

static void wip_im_export_import_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data){
	gchar **help = g_strsplit (wip_im_export_pi.help, ",", 2);
	gchar *dlgid = g_strconcat (wip_im_export_pi.name, "-import\n", wip_im_export_pi.info, NULL);
	gchar *fn = gapp->file_dialog_load (help[0], NULL, file_mask, NULL);
	g_strfreev (help); 
	g_free (dlgid);
	if (fn){
                wip_im_export_filecheck_load_callback (&fn);
                g_free (fn);
	}
}

static void wip_im_export_export_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data){
	gchar **help = g_strsplit (wip_im_export_pi.help, ",", 2);
	gchar *dlgid = g_strconcat (wip_im_export_pi.name, "-export\n", wip_im_export_pi.info, NULL);
	gchar *fn = gapp->file_dialog_save (help[1], NULL, file_mask, NULL);
	g_strfreev (help); 
	g_free (dlgid);
	if (fn){
                wip_im_export_filecheck_save_callback (&fn);
                g_free (fn);
	}
}
