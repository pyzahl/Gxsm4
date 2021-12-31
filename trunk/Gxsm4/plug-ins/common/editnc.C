/* Gnome gxsm - Gnome X Scanning Microscopy
 * universal STM/AFM/SARLS/SPALEED/... controlling and
 * data analysis software
 * 
 * Gxsm Plugin Name: editnc.C
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
% PlugInDocuCaption: View NetCDF file data
% PlugInName: editnc
% PlugInAuthor: Percy Zahl
% PlugInAuthorEmail: zahl@users.sf.net
% PlugInMenuPath: Tools/NetCDF-View

% PlugInDescription
 It shows all information stored in any NetCDF file. Huge data fields
 are truncated and only the first few values are shown.

% PlugInUsage
 Call it from Tools menu and select a NetCDF file when the file open dialog is presented.

%% OptPlugInKnownBugs
%No bugs known.

% OptPlugInNotes
 Only viewing NetCDF data, no editing.

% EndPlugInDocuSection
 * -------------------------------------------------------------------------------- 
 */

#include <sstream>

using namespace std;


#include <gtk/gtk.h>
#include "config.h"
#include "core-source/plugin.h"

static void editnc_about( void );
static void editnc_run(GtkWidget *w, void *data);
static void editnc_init( void );
static void editnc_cleanup( void );

GxsmPlugin editnc_pi = {
		NULL,
		NULL,
		0,
		NULL,
		"NetCDF View",
		NULL,
		NULL,
		"Percy Zahl",
		"tools-section",
		N_("NetCDF-View"),
		N_("List the contents of NetCDF Files"),
		"NetCDF-Dump Utility using a dynamic generated Gtk-Window",
		NULL,
		NULL,
		editnc_init,
		NULL,
		editnc_about,
		NULL,
		editnc_run,
		editnc_cleanup,
};

static const char *about_text = N_("Gxsm EditNc Plugin:\n"
								   "EditNc is a NetCDF-Dump Utility using"
								   " a dynamic generated Gtk-Window:"
                                   "View and (not yet) Edit NetCDF-Data Files");

GxsmPlugin *get_gxsm_plugin_info ( void ){ 
		editnc_pi.description = g_strdup_printf(N_("Gxsm Editnc Plugin %s"), VERSION);
		return &editnc_pi; 
}



/*
 * A derived class, just like NcFile except knows how to "dump" its
 * dimensions, variables, global attributes, and data in ASCII form.
 */

class DumpableNcFile : public NcFile{
public:
		DumpableNcFile(const char* path, NcFile::FileMode mode = ReadOnly) 
				: NcFile(path, mode) { dpath = path; maxvals=0L; } ;
		~DumpableNcFile(){ };
		void dumpdims( void );
		void dumpvars( void );
		void dumpgatts( void );
		void dumpdata( void );
		void dumpall( void );
		static void apply_info_callback(GtkDialog * dialog, gint button_number, DumpableNcFile *dnc);

		static void show_info_callback (GtkWidget *widget, gchar *message){
			gapp->message (message);
		};
		static void free_info_elem(gpointer txt, gpointer data){ g_free((gchar*) txt); };

		void dump_togtkwin( void );
		void setmaxdata(unsigned long maxval) { maxvals=maxval; };
private:
		long maxvals;
		const char *dpath;
};



static void editnc_about(void)
{
		const gchar *authors[] = { editnc_pi.authors, NULL};
		gtk_show_about_dialog (NULL, 
				       "program-name",  editnc_pi.name,
				       "version", VERSION,
				       "license", GTK_LICENSE_GPL_3_0,
				       "comments", about_text,
				       "authors", authors,
				       NULL
				       );
}

static void editnc_run(GtkWidget *w, void *data)
{
		static DumpableNcFile *dnc = NULL;
		static gchar *filename = NULL;
		if( dnc ) delete dnc;
		if( filename ) g_free(filename);
		if( data && editnc_pi.app ){
				dnc = new DumpableNcFile
						( filename = g_strdup
						  ( editnc_pi.app->file_dialog
							( N_("NC file to edit"),".", "*.[nN][cC]*", NULL, "ncedit" )
								  ));
				dnc -> setmaxdata(50);
				dnc -> dump_togtkwin();
		}
		else
				PI_DEBUG (DBG_L2, "Editnc Plugin: Cleanup 1" );
}

static void editnc_showfile(gchar *filename)
{
		static DumpableNcFile *dnc = NULL;
		if( dnc ) delete dnc;
		if( filename ){
				dnc = new DumpableNcFile (filename);
				dnc -> setmaxdata(50);
				dnc -> dump_togtkwin();
		}
		else
				PI_DEBUG (DBG_L2, "Editnc Plugin: Cleanup 2" );
}

static void editnc_init( void ){
		PI_DEBUG (DBG_L2, "editnc_init" );
		//  editnc_pi.app->ConnectPluginToGetNCInfoEvent (editnc_showfile);
		gapp->ConnectPluginToGetNCInfoEvent (editnc_showfile);
}

static void editnc_cleanup( void ){
		editnc_run (NULL, NULL);
		editnc_showfile (NULL);
		editnc_pi.app->ConnectPluginToGetNCInfoEvent (NULL);
		while(gtk_events_pending()) gtk_main_iteration();
}



// ==================================================
// DumpableNcFile
//

void DumpableNcFile::dumpdims( void ){
		for (int n=0; n < num_dims(); n++) {
				NcDim* dim = get_dim(n);
				cout << "\t" << dim->name() << " = " ;
				if (dim->is_unlimited())
						cout << "UNLIMITED" << " ;\t " << "// " << dim->size() <<
								" currently\n";
				else
						cout << dim->size() << " ;\n";
		}
}

void dumpatts(NcVar& var){
		NcToken vname = var.name();
		NcAtt *ap;
		for(int n = 0; (ap = var.get_att(n)); n++) {
				cout << "\t\t" << vname << ":" << ap->name() << " = " ;
				NcValues *vals = ap->values();
				cout << *vals << " ;" << endl ;
				delete ap;
				delete vals;
		}
}

void DumpableNcFile::dumpvars( void )
{
		int n;
		const char *types[] = {"","byte","char","short","long","float","double"};
		NcVar *vp;

		for(n = 0; (vp = get_var(n)); n++) {
				cout << "\t" << types[vp->type()] << " " << vp->name() ;

				if (vp->num_dims() > 0) {
						cout << "(";
						for (int d = 0; d < vp->num_dims(); d++) {
								NcDim* dim = vp->get_dim(d);
								cout << dim->name();
								if (d < vp->num_dims()-1)
								  cout << ", ";		  
						}
						cout << ")";
				}
				cout << " ;\n";
				// now dump each of this variable's attributes
				dumpatts(*vp);
		}
}

void DumpableNcFile::dumpgatts( void )
{
		NcAtt *ap;
		for(int n = 0; (ap = get_att(n)); n++) {
				cout << "\t\t" << ":" << ap->name() << " = " ;
				NcValues *vals = ap->values();
				cout << *vals << " ;" << endl ;
				delete vals;
				delete ap;
		}
}

void DumpableNcFile::dumpdata( )
{
		NcVar *vp;
		for (int n = 0; (vp = get_var(n)); n++) {
				cout << " " << vp->name() << " = ";
				if( maxvals && vp->num_vals() > maxvals)
						cout << "more values than " << maxvals << ", output suppressed !" << endl;
				else{
						NcValues* vals = vp->values();
						cout << *vals << " ;" << endl ;
						delete vals;
				}
		}
}

// Automaticalls pareses NcFile Data to Gtk Dialog

#define PREF_BOX_PACK_MODE    FALSE, TRUE, 2
#define INFO_BOX_PACK_MODE    FALSE, FALSE, 2
#define PREF_BOXEL_PACK_MODE  TRUE, TRUE, 2

void DumpableNcFile::apply_info_callback(GtkDialog * dialog, gint button_number, DumpableNcFile *dnc){
		switch(button_number){
		case 0: 
			dnc->dumpall();
			break;
		case 1: 
			gapp->message ("Sorry, no write back until now !");
			break;
		case 2: 
			gtk_dialog_close(dialog); 
			g_slist_foreach (
					 (GSList*) g_object_get_data (
								      G_OBJECT (dialog), "info_list" ), 
					 (GFunc) free_info_elem, 
					 NULL
					 );
			break;
		}
}


// General NC Formatting Dumpingutil, Output into GTK-Window
void DumpableNcFile::dump_togtkwin( void ){
	GtkWidget *dialog;
	GtkWidget *scrolledwindow;
	GtkWidget *sep;
	GtkWidget *lab;
	GtkWidget *vbox;
	GtkWidget *hbox;
	GtkWidget *VarName;
	GtkWidget *variable;
	GtkWidget *info;
        
	dialog = gtk_dialog_new(N_("Edit NC File"),
				  "Dump to stdout",
				  GNOME_STOCK_BUTTON_APPLY,
				  GNOME_STOCK_BUTTON_CANCEL, 
				  NULL); 
        
	gtk_window_set_policy (GTK_WINDOW (dialog), TRUE, TRUE, FALSE);
	gnome_dialog_set_close(GNOME_DIALOG(dialog), FALSE);
	gnome_dialog_close_hides(GNOME_DIALOG(dialog), FALSE);
	gnome_dialog_set_default(GNOME_DIALOG(dialog), 2);

	gtk_widget_set_size_request (dialog, 500, 400);
        
	scrolledwindow = gtk_scrolled_window_new (NULL, NULL);
	gtk_widget_show (scrolledwindow);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow), 
					GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	gtk_widget_show (vbox);
	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW (scrolledwindow), vbox);
    
	gtk_box_pack_start(GTK_BOX(GNOME_DIALOG(dialog)->vbox),
			   scrolledwindow, TRUE, TRUE, GXSM_WIDGET_PAD);
        
	gtk_box_pack_start (GTK_BOX (vbox), lab=gtk_label_new(dpath), PREF_BOX_PACK_MODE);
	gtk_widget_show (lab);
	gtk_box_pack_start (GTK_BOX (vbox), sep=gtk_hseparator_new(), PREF_BOX_PACK_MODE);
	gtk_widget_show (sep);
	// ===============================================================================
	// DUMP:  global attributes
	// ===============================================================================
	gtk_box_pack_start (GTK_BOX (vbox), lab=gtk_label_new("Global Attributes"), PREF_BOX_PACK_MODE);
	gtk_widget_show (lab);
	gtk_box_pack_start (GTK_BOX (vbox), sep=gtk_hseparator_new(), PREF_BOX_PACK_MODE);
	gtk_widget_show (sep);
	NcAtt *ap;
	for(int n = 0; (ap = get_att(n)); n++) {
		hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
		gtk_widget_show (hbox);
    
		gtk_box_pack_start (GTK_BOX (vbox), hbox, PREF_BOX_PACK_MODE);

		VarName = gtk_label_new (ap->name());
    
		gtk_widget_set_size_request (VarName, 150, -1);
		gtk_widget_show (VarName);
		gtk_label_set_justify(GTK_LABEL(VarName), GTK_JUSTIFY_LEFT);
		gtk_misc_set_alignment (GTK_MISC (VarName), 0.0, 0.5);
		gtk_misc_set_padding (GTK_MISC (VarName), 5, 0);
    
		gtk_box_pack_start (GTK_BOX (hbox), VarName, PREF_BOXEL_PACK_MODE);
      
		variable = gtk_entry_new ();
		gtk_widget_show (variable);
		gtk_box_pack_start (GTK_BOX (hbox), variable, PREF_BOXEL_PACK_MODE);

		NcValues* vals;
		gtk_entry_set_text (GTK_ENTRY (variable), (vals = ap->values())->as_string(0));
		delete vals;
		delete ap;
	}

	GSList *infolist=NULL;
	const gchar *types[] = {"","byte","char","short","long","float","double"};
	NcVar *vp;

	// ===============================================================================

	gtk_box_pack_start (GTK_BOX (vbox), sep=gtk_hseparator_new(), PREF_BOX_PACK_MODE);
	gtk_widget_show (sep);
	gtk_box_pack_start (GTK_BOX (vbox), sep=gtk_hseparator_new(), PREF_BOX_PACK_MODE);
	gtk_widget_show (sep);

	// ===============================================================================
	// DUMP:  dimension value
	// ===============================================================================
  
	gtk_box_pack_start (GTK_BOX (vbox), lab=gtk_label_new("NC Dimensions"), PREF_BOX_PACK_MODE);
	gtk_widget_show (lab);
	gtk_box_pack_start (GTK_BOX (vbox), sep=gtk_hseparator_new(), PREF_BOX_PACK_MODE);
	gtk_widget_show (sep);
	for (int n=0; n < num_dims(); n++) {
		hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
		gtk_widget_show (hbox);
    
		gtk_box_pack_start (GTK_BOX (vbox), hbox, PREF_BOX_PACK_MODE);
      
		NcDim* dim = get_dim(n);
		gchar *dimname = g_strconcat((gchar*)dim->name(), " ", NULL);

		VarName = gtk_label_new (dimname);
		g_free(dimname);
    
		gtk_widget_set_size_request (VarName, 150, -1);
		gtk_widget_show (VarName);
		gtk_label_set_justify(GTK_LABEL(VarName), GTK_JUSTIFY_LEFT);
		gtk_misc_set_alignment (GTK_MISC (VarName), 0.0, 0.5);
		gtk_misc_set_padding (GTK_MISC (VarName), 5, 0);
    
		gtk_box_pack_start (GTK_BOX (hbox), VarName, PREF_BOXEL_PACK_MODE);
      
		variable = gtk_entry_new ();
		gtk_widget_show (variable);
		gtk_box_pack_start (GTK_BOX (hbox), variable, PREF_BOXEL_PACK_MODE);

		gchar *dimval;
		if (dim->is_unlimited())
			dimval = g_strdup_printf("UNLIMITED, %d currently", (int)dim->size());
		else
			dimval = g_strdup_printf("%d", (int)dim->size());
    
		gtk_entry_set_text (GTK_ENTRY (variable), dimval);
		g_free(dimval);

	}

	// ===============================================================================

	gtk_box_pack_start (GTK_BOX (vbox), sep=gtk_hseparator_new(), PREF_BOX_PACK_MODE);
	gtk_widget_show (sep);
	gtk_box_pack_start (GTK_BOX (vbox), sep=gtk_hseparator_new(), PREF_BOX_PACK_MODE);
	gtk_widget_show (sep);

	// ===============================================================================
	// DUMP:  vartype varname(dims)   data
	// ===============================================================================

	gtk_box_pack_start (GTK_BOX (vbox), lab=gtk_label_new("NC Data"), PREF_BOX_PACK_MODE);
	gtk_widget_show (lab);
	gtk_box_pack_start (GTK_BOX (vbox), sep=gtk_hseparator_new(), PREF_BOX_PACK_MODE);
	gtk_widget_show (sep);
	for (int n = 0; (vp = get_var(n)); n++) {
		int unlimited_flag = FALSE;
		hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
		gtk_widget_show (hbox);
      
		gtk_box_pack_start (GTK_BOX (vbox), hbox, PREF_BOX_PACK_MODE);
      
		gchar *vdims = g_strdup(" ");
		if (vp->num_dims() > 0) {
			g_free(vdims);
			vdims = g_strdup("(");
			for (int d = 0; d < vp->num_dims(); d++) {
				gchar *tmp = g_strconcat(vdims, (gchar*)vp->get_dim(d)->name(), 
							 ((d<vp->num_dims()-1)?", ":")"), NULL);

				if (vp->get_dim(d)->is_unlimited())
					unlimited_flag = TRUE;

				g_free(vdims);
				vdims = g_strdup(tmp);
				g_free(tmp);
			}
		}
		gchar *vardef = g_strconcat(types[vp->type()], " ", (gchar*)vp->name(), vdims, NULL);
		g_free(vdims);
		VarName = gtk_label_new (vardef);
		std::cout << vardef << std::endl;
		g_free(vardef);

		gtk_widget_set_size_request (VarName, 150, -1);
		gtk_widget_show (VarName);
		gtk_label_set_justify(GTK_LABEL(VarName), GTK_JUSTIFY_LEFT);
		gtk_misc_set_alignment (GTK_MISC (VarName), 0.0, 0.5);
		gtk_misc_set_padding (GTK_MISC (VarName), 5, 0);
      
		gtk_box_pack_start (GTK_BOX (hbox), VarName, PREF_BOXEL_PACK_MODE);
      
		variable = gtk_entry_new ();
		gtk_widget_show (variable);
		gtk_box_pack_start (GTK_BOX (hbox), variable, PREF_BOXEL_PACK_MODE);

		ostringstream ostr_val;



		if (unlimited_flag){
			ostr_val << "** Unlimited Data Set, data suppressed **";
		} else {
			NcValues *v = vp->values();
			if(vp->type() == ncChar){
				ostr_val << v->as_string(0);
			}else{
				if(v){
					if(v->num() > 1){
						if(v->num() > maxvals)
							ostr_val << "[#="<< vp->num_vals() << "] " << " ... (too many, suppressed) ";
						else
							ostr_val << "[#="<< vp->num_vals() << "] " << *v;
					}else
						ostr_val << *v;
				} else
					ostr_val << "** Empty **";
			}
			delete v;
		}
		ostr_val << ends;
		gtk_entry_set_text (GTK_ENTRY (variable), (const gchar*)ostr_val.str().c_str());

		NcToken vname = vp->name();
		NcAtt *ap;
      
		ostringstream  ostr_att;
		if((ap=vp->get_att(0))){
			delete ap;
			ostr_att << "Details and NetCDF Varibale Attributes:" << endl;

			for(int n = 0; (ap = vp->get_att(n)); n++) {
				NcValues *v;
				ostr_att << vname << ":" 
					 << ap->name() << " = "
					 << *(v=ap->values()) << endl;
				delete ap;
				delete v;
			}
		}
		else
			ostr_att << "Sorry, no info available for \"" << vname << "\" !";

		ostr_att << "\nValue(s):\n" << ostr_val.str().c_str(); 
		ostr_att << ends;
		gchar *infotxt = g_strdup( (const gchar*)ostr_att.str().c_str() );
		infolist = g_slist_prepend( infolist, infotxt);
		info = gtk_button_new_with_label (" Details ");
		gtk_widget_show (info);
		gtk_box_pack_start (GTK_BOX (hbox), info, INFO_BOX_PACK_MODE);
		g_signal_connect (G_OBJECT (info), "clicked",
				    G_CALLBACK (show_info_callback),
				    infotxt);
	}

	g_object_set_data (G_OBJECT (dialog), "info_list", infolist);
 
	g_signal_connect (G_OBJECT(dialog), "clicked",
			    G_CALLBACK(apply_info_callback),
			    this);
  
	gtk_widget_show (dialog);
}

void DumpableNcFile::dumpall( )
{
		cout << "netcdf " << dpath << " {" << endl <<
				"dimensions:" << endl ;

		dumpdims();

		cout << "variables:" << endl;

		dumpvars();

		if (num_atts() > 0)
				cout << "// global attributes" << endl ;

		dumpgatts();

		cout << "data:" << endl;

		dumpdata();

		cout << "}" << endl;
}
