/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 8 c-style: "K&R" -*- */

/* Gnome gxsm - Gnome X Scanning Microscopy
 * universal STM/AFM/SARLS/SPALEED/... controlling and
 * data analysis software
 * 
 * Gxsm Plugin Name: findlocmax.C
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
 * All "% PlugInXXX" commentary tags are mandatory
 * All "% OptPlugInXXX" tags are optional
 * --------------------------------------------------------------------------------
% BeginPlugInDocuSection
% PlugInDocuCaption: Find local maxima -- SARLS
% PlugInName: findlocmax
% PlugInAuthor: M. Langer?
% PlugInAuthorEmail: stefan_fkp@users.sf.net
% PlugInMenuPath: Math/Misc/Find Loc Max

% PlugInDescription
Finds local maximan in a scan. One (unknown to me, PZ) unknown guy
(M.Langer?) of the SARLS group in hannover wrote this PlugIn for
searching of local maxima. It looks somehow specialized to me.

% PlugInUsage
It is only available from \GxsmMenu{Math/Misc/Find Loc Max} if the
instrument type is set to SARLS or a reload of all PlugIns is forced.

% OptPlugInSources
The active channel is used as data source.

%% OptPlugInObjects
%A optional rectangle is used for data extraction...

%% OptPlugInDest
%The computation result is placed into an existing math channel, else
%into a new created math channel.

% OptPlugInFiles
Saves a list of local maxima in Ascii format (List of X Y Z) to a file.

%% OptPlugInRefs
%Any references?

%% OptPlugInKnownBugs
%Are there known bugs? List! How to work around if not fixed?

% OptPlugInNotes
I would appreciate if one of the current SARLS group members could
figure out more about this piece of code.

%% OptPlugInHints
%Any tips and tricks?

% EndPlugInDocuSection
 * -------------------------------------------------------------------------------- 
 */


#include <gtk/gtk.h>
#include "config.h"
#include "plugin.h"
 
using namespace std;


static void findlocmax_init( void );
static void findlocmax_about( void );
static void findlocmax_configure( void );
static void findlocmax_cleanup( void );
static gboolean findlocmax_run( Scan *Src, Scan *Dest );

GxsmPlugin findlocmax_pi = {
	NULL,
	NULL,
	0,
	NULL,
	g_strdup("Findlocmax-M1S-Misc"),
	NULL,
	NULL,
	"Nobody?, PZ",
	"math-misc-section",
	N_("Find Loc Max"),
	N_("Sorry, no help for findlocmax filter!"),
	"no more info",
	NULL,
	NULL,
	findlocmax_init,
	NULL,
	findlocmax_about,
	findlocmax_configure,
	NULL,
	NULL,
	NULL,
	findlocmax_cleanup
};

GxsmMathOneSrcPlugin findlocmax_m1s_pi = {
	findlocmax_run
};

static const char *about_text = N_("Gxsm Findlocmax Plugin\n\n"
                                   "Who has coded this one? -- PZ");

GxsmPlugin *get_gxsm_plugin_info ( void ){ 
	findlocmax_pi.description = g_strdup_printf(N_("Gxsm MathOneArg findlocmax plugin %s"), VERSION);
	return &findlocmax_pi; 
}

GxsmMathOneSrcPlugin *get_gxsm_math_one_src_plugin_info( void ) { 
	return &findlocmax_m1s_pi; 
}

static void findlocmax_init(void)
{
	PI_DEBUG (DBG_L2, "Findlocmax Plugin Init");
}

static void findlocmax_about(void)
{
	const gchar *authors[] = { findlocmax_pi.authors, NULL};
	gtk_show_about_dialog (NULL, 
			       "program-name",  findlocmax_pi.name,
			       "version", VERSION,
			       "license", GTK_LICENSE_GPL_3_0,
			       "comments", about_text,
			       "authors", authors,
			       NULL
			       );
}

static void findlocmax_configure(void)
{
	if(findlocmax_pi.app)
		findlocmax_pi.app->message("Findlocmax Plugin Configuration");
}

static void findlocmax_cleanup(void)
{
	PI_DEBUG (DBG_L2, "Findlocmax Plugin Cleanup");
}


static void cancel_callback (GtkWidget *widget, int *status){
	*status = 1; 
}

void ChangeValue (GtkComboBox* Combo,  gchararray *string){
	GtkTreeIter iter;
	GtkTreeModel *model;
	gtk_combo_box_get_active_iter (GTK_COMBO_BOX (Combo), &iter);
	model = gtk_combo_box_get_model (GTK_COMBO_BOX (Combo));
        if (*string){
                g_free (*string); *string=NULL;
        }
	gtk_tree_model_get (model, &iter, 0, &(*string), -1);
        std::cout << *string << std::endl;
}

void ChangeEntry (GtkEntry* entry,  gchararray *string){
        const gchar *ctxt = gtk_entry_get_text (GTK_ENTRY (entry));
        if (*string)
                g_free (*string);
        *string = g_strdup (ctxt);
        std::cout << *string << std::endl;
}
        
void xyz_file_set (GtkFileChooserButton *chooser,  gchar **string){
        *string = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (chooser));    
        gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (g_object_get_data (G_OBJECT (chooser), "COMBO")), *string, *string);
        std::cout << *string << std::endl;
}

void ChangeIndex (GtkSpinButton *spinner, double *index){
	*index = gtk_spin_button_get_value (spinner);

	std::cout << "Change Index: " << ((int)*index) << std::endl;
}



class flm_setup : public AppBase{
public:
	flm_setup (double &kx, double &ky, double &kz,
                   double &trs,
		   gchararray *mode,
		   gchararray *peak_output_file,
		   double &max_threads
                ){

		GtkWidget *dialog;
                GtkWidget *choice;
                GtkWidget *chooser;

                //UnitObj *meVA    = new UnitObj("meV/" UTF8_ANGSTROEM, "meV/" UTF8_ANGSTROEM );

                GtkDialogFlags flags =  (GtkDialogFlags) (GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT);
                dialog = gtk_dialog_new_with_buttons (N_("Find Local Max Kernel Setup"),
                                                      GTK_WINDOW (main_get_gapp()->get_app_window ()),
                                                      flags,
                                                      _("_OK"),
                                                      GTK_RESPONSE_ACCEPT,
                                                      _("_Cancel"),
                                                      GTK_RESPONSE_REJECT,
                                                      NULL);
                BuildParam bp;
                gtk_container_add (GTK_CONTAINER (gtk_dialog_get_content_area (GTK_DIALOG (dialog))), bp.grid);
                bp.set_error_text ("Value not allowed.");
                bp.input_nx = 5;

		bp.grid_add_ec_with_scale ("kx",   main_get_gapp()->xsm->Unity, &kx,    0.,   100., ".0f", 1.,  4.);
                bp.new_line ();
		bp.grid_add_ec_with_scale ("ky",   main_get_gapp()->xsm->Unity, &ky,    0.,   100., ".0f", 1.,  4.);
                bp.new_line ();
		bp.grid_add_ec_with_scale ("kz",   main_get_gapp()->xsm->Unity, &kz,    0.,   100., ".0f", 1.,  4.);
                bp.new_line ();
		bp.grid_add_ec_with_scale ("treshold", main_get_gapp()->xsm->Unity, &trs, 0., 1e10, ".3f", 1.,  4.);
                bp.new_line ();
		
                bp.grid_add_label ("Mode", NULL, 1, 1.0);
		choice = gtk_combo_box_text_new ();
		gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (choice), "Box", "Box");
		gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (choice), "Sphere", "Sphere");
		gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (choice), "SARLS", "SARLS");
		g_signal_connect(G_OBJECT (choice), "changed", G_CALLBACK (ChangeValue), &(*mode));
		bp.grid_add_widget (choice, 5);

                bp.new_line ();
                chooser = gtk_file_chooser_button_new (_("Peak Output File"), GTK_FILE_CHOOSER_ACTION_SAVE);
                GtkFileFilter *xyz_filter = gtk_file_filter_new ();
                gtk_file_filter_set_name (xyz_filter, "XYZ");
                gtk_file_filter_add_pattern (xyz_filter, "*.xyz");
                gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (chooser), xyz_filter);
                GtkFileFilter *all_filter = gtk_file_filter_new ();
                gtk_file_filter_set_name (all_filter, "All");
                gtk_file_filter_add_pattern (all_filter, "*");
                gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (chooser), all_filter);

                g_object_set_data (G_OBJECT (chooser), "COMBO", choice);
		g_signal_connect (G_OBJECT (chooser), "file-set", G_CALLBACK (xyz_file_set), &(*peak_output_file));
		gtk_widget_show (chooser);
		gtk_combo_box_set_active (GTK_COMBO_BOX (choice), 0);
		bp.grid_add_widget (chooser, 5);
                bp.new_line ();

		bp.grid_add_ec_with_scale ("Max Thread #",   main_get_gapp()->xsm->Unity, &max_threads,    1.,   64., ".0f", 1.,  4.);
                bp.new_line ();
                        
                gtk_widget_show_all (dialog);
                
		gint r=gtk_dialog_run(GTK_DIALOG(dialog));
		gtk_widget_destroy (dialog);
              
                //delete meVA;

                if (r == GTK_RESPONSE_REJECT){
                        *mode = g_strdup ("NONE");
                }
        }

	~flm_setup () {};
};




int SARLS_findlocmax_run(Scan *Src, Scan *Dest)
{
	/* Suche lokaler Maxima im Fokusbereich (Kasten mit 10\B5m*10\B5m) */
	/* Version 0.2 ML  (23/11/98)                                  */

	int line,col,fokline,fokcol,fokxpoints,fokypoints;
	int peakx,peaky,newpeakx,newpeaky;
	double referenzpeak;
	ofstream f;
	int   i;
	const char *fname;
	PI_DEBUG (DBG_L2, "Suche lokaler Maxima und Ausgabe in ASCII-Datei"); 
	fokxpoints=(int)(50000/Src->data.s.dx);
	if (fokxpoints==0) fokxpoints=1;
	fokypoints=(int)(50000/Src->data.s.dy);
	if (fokypoints==0) fokypoints=1;

	/* Empty-Scan erzeugen, alle Datenpunkte Null setzen */

	for(line=0;line<Dest->data.s.ny;line++)
		for(col=0;col<Dest->data.s.nx;col++)
			Dest->mem2d->PutDataPkt(0,col, line);     
  
	/* Peaks aufsp\FCren, mit einem Mindestabstand = Fokus = 10\B5m <- Variablen: fokxpoints, fokypoints */
	/* doppelte Peaks werden leider noch nicht rausgeworfen */

	for(line=0;line<(Src->mem2d->GetNy()-fokypoints);line=line+(fokypoints/2))
		for(col=0;col<(Src->mem2d->GetNx()-fokxpoints);col=col+(fokxpoints/2)){      
			peakx=col;
			peaky=line;  
			referenzpeak=Src->mem2d->GetDataPkt(col,line);            
			do{
				newpeakx=peakx; newpeaky=peaky;
				for(fokcol=(int)(newpeakx-fokxpoints); fokcol<=(newpeakx+fokxpoints); fokcol++)
					for(fokline=(int)(newpeaky-fokypoints); fokline<=(newpeaky+fokypoints); fokline++)
						if((fokcol>=0) && (fokcol<Src->mem2d->GetNx()) && (fokline>=0) && (fokline<Src->mem2d->GetNy())){
							if(referenzpeak<=(double)Src->mem2d->GetDataPkt(fokcol,fokline)){
								referenzpeak=Src->mem2d->GetDataPkt(fokcol,fokline);
								peakx=fokcol;
								peaky=fokline;
							}		 	 
						}	  
			}while(((newpeakx-peakx)!=0) && ((newpeaky-peaky)!=0));
			for(fokline=(newpeaky-fokypoints); fokline<=(newpeaky+fokypoints); fokline++)
				for(fokcol=(newpeakx-fokxpoints); fokcol<=(newpeakx+fokxpoints); fokcol++)
					if((fokcol>=0) && (fokcol<Dest->mem2d->GetNx()) && (fokline>=0) && (fokline<Dest->mem2d->GetNy()))
						if(0<Dest->mem2d->GetDataPkt(fokcol,fokline))
							referenzpeak=0;
			Dest->mem2d->PutDataPkt((SHT)referenzpeak,newpeakx,newpeaky);     
		}    
	
	/* File Selector */

	do{
		fname = findlocmax_pi.app->file_dialog_save ("asc File to Save LOCAL MAXIMA",NULL,NULL);
		if (fname == NULL) return 1;
		if (strncmp(fname+strlen(fname)-4,".asc",4))
			strcat((char*)fname, ".asc");
		PI_DEBUG (DBG_L2, "Save File: " << fname);
		i=2;
		f.open(fname, ios::in);
		if(f.good()){
			f.close();
			if((i=XSM_SHOW_CHOICE(WRN_WARNING, fname, WRN_FILEEXISTS, 3, L_CANCEL, L_OVERWRITE, L_RETRY, 1)) == 1)
				return 1;
		}
	}while(i!=2);
  
	time_t t;
	time(&t); 
	f.open(fname, ios::out | ios::trunc);
	if(!f.good()){
		XSM_SHOW_ALERT(ERR_SORRY, ERR_FILEWRITE,fname,1);
		return 1;
	}
	f << "# LOKALE MAXIMA ASCII-Data\n" << "# Date: " << ctime(&t) << "# " << fname << endl;
	f << "# Fokuspoints x = " << fokxpoints << endl;
	f << "# Fokuspoints y = " << fokypoints << endl;
	f << "# Datenpunktabstand [A]  dx = " << Src->data.s.dx << endl;
	f << "# Datenpunktabstand [A]  dy = " << Src->data.s.dy << endl;
	f << "# x   y	  Intensit\E4t" << endl ;

	for(line=0;line<Dest->data.s.ny;line++)
		for(col=0;col<Dest->data.s.nx;col++)
			if (Dest->mem2d->GetDataPkt(col, line)!=0)     
				f << col << " " << line << " " << Dest->mem2d->GetDataPkt(col, line) << endl;     
	f.close();
        return 0;
}

inline int imin (int x, int y) { return x < y ? x : y; }
inline int imax (int x, int y) { return x > y ? x : y; }

void multi_dim_flm (Scan *Src, Scan *Dest, double kxyz[3], double trs, int maxthreads){
        int kxr=(int)kxyz[0];
        int kyr=(int)kxyz[1];
        int kvr=(int)kxyz[2];
        for (int v=0; v<Dest->mem2d->GetNv (); ++v){
                g_message ("FLM: processing v=%d",v);
                for (int c=0; c<Dest->mem2d->GetNx (); ++c)
                        for (int l=0; l<Dest->mem2d->GetNy (); ++l){
                                double max = 0.;
                                for (int kv=imax (0,v-kvr); kv<=imin (Dest->mem2d->GetNv ()-1, v+kvr); ++kv)
                                        for (int kx=imax (0,l-kxr); kx<=imin (Dest->mem2d->GetNx ()-1, l+kxr); ++kx)
                                                for (int ky=imax (0,c-kyr); ky<=imin (Dest->mem2d->GetNy ()-1, c+kyr); ++ky){
                                                        double kval = Src->mem2d->GetDataPkt (kx, ky, kv);
                                                        if (max < kval)
                                                                max = kval;
                                                }
                                double val = Src->mem2d->GetDataPkt (c,l,v);
                                if (max - val < trs)
                                        Dest->mem2d->PutDataPkt (val, c,l,v);
                                else
                                        Dest->mem2d->PutDataPkt (0., c,l,v);
                        }
        }
}

static gboolean findlocmax_run(Scan *Src, Scan *Dest)
{
        double kxyz[3] = { 4.,4.,4. };
        gchararray mode=NULL;
        gchararray outfile=NULL;
        double maxthreads=1.;
        double trs=1.;
        flm_setup (kxyz[0], kxyz[1], kxyz[2], trs, &mode, &outfile, maxthreads);

        if (mode[0] == 'S')
                SARLS_findlocmax_run (Src, Dest);
        else
                multi_dim_flm (Src, Dest, kxyz, trs, (int)maxthreads);
        
	return MATH_OK;
}
