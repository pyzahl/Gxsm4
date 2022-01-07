/* Gnome gxsm - Gnome X Scanning Microscopy universal
 * STM/AFM/SARLS/SPALEED/... controlling and data analysis software
 *
 * Gxsm Plugin Name: mkicons.C
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
% PlugInDocuCaption: MkIcons -- Make icons
% PlugInName: mkicons
% PlugInAuthor: Stefan Schr\"oder
% PlugInAuthorEmail: stefan_fkp@users.sf.net
% PlugInMenuPath: Tools

% PlugInDescription
This plugin helps you printing several nc-images on one page.

% PlugInUsage
 Choose Mkicons from the \GxsmMenu{Tools} menu. 

% EndPlugInDocuSection
 *
 --------------------------------------------------------------------------------
*/

#include <gtk/gtk.h>
#include <dirent.h>
#include <fnmatch.h>
#include "config.h"
#include "plugin.h"
#include "epsfutils.h"
#include "glbvars.h"
#include "mkicons.h"
#include "pyremote.h"

#include "action_id.h" 
// wegen ID_CH_*

// Plugin Prototypes
static void mkicons_init( void );
static void mkicons_about( void );
static void mkicons_configure( void );
static void mkicons_query( void );
static void mkicons_cleanup( void );
static void mkicons_run(GtkWidget *w, void *data);

// for remotecontrolPI
static void mkicons_run_non_interactive(GtkWidget *w, gpointer mki);

// Fill in the GxsmPlugin Description here
GxsmPlugin mkicons_pi = {
	NULL,                   // filled in and used by Gxsm, don't touch !
	NULL,                   // filled in and used by Gxsm, don't touch !
	0,                      // filled in and used by Gxsm, don't touch !
	NULL,                   // The Gxsm-App Class Ref.pointer (called "gapp" in Gxsm) is 
	// filled in here by Gxsm on Plugin load, 
	// just after init() is called !!!
	// ----------------------------------------------------------------------
	// Plugins Name, CodeStly is like: Name-M1S[ND]|M2S-BG|F1D|F2D|ST|TR|Misc
	"Mkicons",
	NULL,
	// Description, is shown by PluginViewer (Plugin: listplugin, Tools->Plugin Details)
	g_strdup("Mkicons PI."),
	// Author(s)
	"Stefan Schroeder",
	// Menupath to position where it is appended to
	"tools-section",
	// Menuentry
	N_("Makeicons"),
	// help text shown on menu
	N_("make icons."),
	// more info...
	"no more info",
	NULL,          // error msg, plugin may put error status msg here later
	NULL,          // Plugin Status, managed by Gxsm, plugin may manipulate it too
	// init-function pointer, can be "NULL", 
	// called if present at plugin load
	mkicons_init,  
	// query-function pointer, can be "NULL", 
	// called if present after plugin init to let plugin manage it install itself
	mkicons_query, // query should be "NULL" for Gxsm-Math-Plugin !!!
	// about-function, can be "NULL"
	// can be called by "Plugin Details"
	mkicons_about,
	// configure-function, can be "NULL"
	// can be called by "Plugin Details"
	mkicons_configure,
	// run-function, can be "NULL", if non-Zero and no query defined, 
	// it is called on menupath->"plugin"
	NULL, // run should be "NULL" for Gxsm-Math-Plugin !!!
	// cleanup-function, can be "NULL"
	// called if present at plugin removal
  NULL, // direct menu entry callback1 or NULL
  NULL, // direct menu entry callback2 or NULL

	mkicons_cleanup
};

// Text used in Aboutbox, please update!!a
static const char *about_text = N_("Gxsm Plugin\n\n"
                                   "Printer Plugin + Control.");

// Symbol "get_gxsm_plugin_info" is resolved by dlsym from Gxsm, used to get Plugin's info!! 
GxsmPlugin *get_gxsm_plugin_info ( void ){ 
	mkicons_pi.description = g_strdup_printf(N_("Gxsm mkicons plugin %s"), VERSION);
	return &mkicons_pi; 
}

// 5.) Start here with the plugins code, vars def., etc.... here.
// ----------------------------------------------------------------------
//


static void mkicons_init(void)
{
  PI_DEBUG (DBG_L2, "mkicons Plugin Init");
}

static void mkicons_about(void)
{
	const gchar *authors[] = { mkicons_pi.authors, NULL};
	gtk_show_about_dialog (NULL,
			       "program-name",  mkicons_pi.name,
			       "version", VERSION,
			       "license", GTK_LICENSE_GPL_3_0,
			       "comments", about_text,
			       "authors", authors,
			       NULL
			       );
}

static void mkicons_configure(void)
{
	if(mkicons_pi.app)
		mkicons_pi.app->message("Mkicons Plugin Configuration");
}

static void mkicons_query(void)
{
	mkicons_pi.app->ConnectPluginToMkiconsEvent (mkicons_run); // connect
}


static void mkicons_cleanup(void)
{
	mkicons_pi.app->ConnectPluginToMkiconsEvent (NULL); // disconnect
}

static void mkicons_run( GtkWidget *w, void *data )
{
  	static MkIconsPIControl *mki = NULL;
          if(w){
                  if(!mki)
                          mki = new MkIconsPIControl();
                  if(!mki->running()){
 		mki->run();
 		}
          }
else
                  if(mki){ // destroy if exists
                          delete mki;
  				}
 				return;
}
/////////////////////////////////////////////////////////////////////////////

//============================================================//
//  MkIconsPIControl class 
//============================================================//

typedef union {
	struct { unsigned char oi, x, y, z; } s;
	unsigned long   l; // default Eintrag
} OptIndex;
 
const char *Opt_Paper[]       = {"A4", "Letter", NULL};
const char *Opt_Resolution[]  = {"300dpi", "600dpi", "1200dpi", NULL};
const char *Opt_ERegression[] = {"no", "E 30% margin", "E 5% margin", NULL};
const char *Opt_LRegression[] = {"no", "lin.Reg.", NULL};
const char *Opt_ViewMode[]    = {"default", "quick", "direct", "logarithmic", "perodic", "horizontal", NULL};
const char *Opt_AutoScaling[] = {"default", "auto 5% margin", "auto 20% margin", "auto 30% margin", NULL};
const char *Opt_Scaling[]     = {"no", "min-max", "Cps-lo-hi", NULL};
 
 
#define MK_ICONS_KEYBASE "MkIcons"
 
MKICONSPI_OPTIONS OptionsList[] = {
 	{ "Paper",   Opt_Paper,  "AL", 0 },
 	{ "Resolution",   Opt_Resolution,  "36C", 1 },
 	{ "E-Regression", Opt_ERegression, "-Ee", 0 },
 	{ "L-Regression", Opt_LRegression, "-l",  0 },
 	{ "View-Mode",    Opt_ViewMode,    "-qdlph", 0 },
 	{ "Auto-Scaling", Opt_AutoScaling, "-123",  0 },
 	{ "Scaling",      Opt_Scaling,     "-ac",  0 },
 	{ NULL, 0 }
 };
 
 
 MkIconsPIControl::MkIconsPIControl (){
 	PI_DEBUG(DBG_L4, "MkIconsPIControl::MkIconsPIControl");

	XsmRescourceManager xrm(MK_ICONS_KEYBASE);
	
	icondata = new MkIconsPIData(
		xrm.GetStr("SourcePath","."),
		xrm.GetStr("DestPath","/tmp/icons.ps"),
		xrm.GetStr("SourceMask","*.nc"),
		xrm.GetStr("Options","----------"),
		xrm.GetStr("IconFile","icons.ps")
		);
	icondata = new MkIconsPIData();
 
 	PI_DEBUG(DBG_L4, "MkIconsPIControl::MkIconsPIControl OK.");
 }
 
 MkIconsPIControl::~MkIconsPIControl (){
	// save defaults to resource-database

	PI_DEBUG(DBG_L4, "MkIconsPIControl::~MkIconsPIControl");
	MKICONSPI_OPTIONS *opt;
 	XsmRescourceManager xrm(MK_ICONS_KEYBASE);
 	for(opt = OptionsList; opt->name; ++opt)
 		xrm.Put(opt->name, opt->init);
 	
 	xrm.Put("SourcePath", icondata->pathname);
 	xrm.Put("DestPath", icondata->outputname);
 	xrm.Put("SourceMask", icondata->mask);
 	xrm.Put("Options", icondata->options);
 	xrm.Put("IconFile", icondata->name);
 	
	PI_DEBUG(DBG_L4, "MkIconsPIControl::~MkIconsPIControl done.");
 	delete icondata;
 }

void MkIconsPIControl::run(){
	GtkWidget *dialog;
	GtkWidget *vbox;
	GtkWidget *hbox;
	GtkWidget *VarName;
	GtkWidget *variable;
	GtkWidget *help;
	GtkWidget *separator;

	PI_DEBUG(DBG_L4, "MkIconsPIControl::run");
	
  	dialog = gtk_dialog_new_with_buttons (N_("GXSM make icons"),
  					      NULL,
  					      (GtkDialogFlags)(GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT),
  					      N_("1x1"), 1, N_("2x3"), 2, N_("4x6"), 4, N_("6x9"), 6,
  					      GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
  					      NULL); 
          
  	vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  	gtk_widget_show (vbox);
  	
  	gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area (GTK_DIALOG(dialog))),
			   vbox, TRUE, TRUE, GXSM_WIDGET_PAD);
          
  
  	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
  	gtk_widget_show (hbox);
  	gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  	
  	VarName = gtk_label_new (N_("Source Path"));
  	gtk_widget_set_size_request (VarName, 100, -1);
  	gtk_widget_show (VarName);
  	gtk_label_set_justify(GTK_LABEL(VarName), GTK_JUSTIFY_LEFT);
  	gtk_misc_set_alignment (GTK_MISC (VarName), 0.0, 0.5);
  	gtk_misc_set_padding (GTK_MISC (VarName), 5, 0);
  	gtk_box_pack_start (GTK_BOX (hbox), VarName, TRUE, TRUE, 0);
  	
  	SrcPath = variable = gtk_entry_new ();
  	gtk_widget_show (variable);
  	gtk_box_pack_start (GTK_BOX (hbox), variable, TRUE, TRUE, 0);
 	gtk_entry_set_text (GTK_ENTRY (variable), icondata->pathname);
 // 	gtk_entry_set_text (GTK_ENTRY (variable), "otto");
  	
  	help = gtk_button_new_with_label (N_("Help"));
  	gtk_widget_show (help);
  	gtk_box_pack_start (GTK_BOX (hbox), help, TRUE, TRUE, 0);
  	g_signal_connect (G_OBJECT (help), "clicked",
  			    G_CALLBACK (show_info_callback),
  			    (void*)(N_("Set to full pathname of data source directory.")));
  	
  	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
  	gtk_widget_show (hbox);
  	gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
    
  	VarName = gtk_label_new (N_("Selection Mask"));
  	gtk_widget_set_size_request (VarName, 100, -1);
  	gtk_widget_show (VarName);
  	gtk_label_set_justify(GTK_LABEL(VarName), GTK_JUSTIFY_LEFT);
  	gtk_misc_set_alignment (GTK_MISC (VarName), 0.0, 0.5);
  	gtk_misc_set_padding (GTK_MISC (VarName), 5, 0);
  	gtk_box_pack_start (GTK_BOX (hbox), VarName, TRUE, TRUE, 0);
    
  	SrcMask = variable = gtk_entry_new ();
  	gtk_widget_show (variable);
  	gtk_box_pack_start (GTK_BOX (hbox), variable, TRUE, TRUE, 0);
 	gtk_entry_set_text (GTK_ENTRY (variable), icondata->mask);
 // 	gtk_entry_set_text (GTK_ENTRY (variable), "*.ncFritz");
  
  	help = gtk_button_new_with_label (N_("Help"));
  	gtk_widget_show (help);
  	gtk_box_pack_start (GTK_BOX (hbox), help, TRUE, TRUE, 0);
  	g_signal_connect (G_OBJECT (help), "clicked",
  			    G_CALLBACK (show_info_callback),
  			    (void*)(N_("Select subset of files via wildcard.")));
  	
  	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
  	gtk_widget_show (hbox);
  	gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  	
  	VarName = gtk_label_new (N_("Icon Filename"));
  	gtk_widget_set_size_request (VarName, 100, -1);
  	gtk_widget_show (VarName);
  	gtk_label_set_justify(GTK_LABEL(VarName), GTK_JUSTIFY_LEFT);
  	gtk_misc_set_alignment (GTK_MISC (VarName), 0.0, 0.5);
  	gtk_misc_set_padding (GTK_MISC (VarName), 5, 0);
  	gtk_box_pack_start (GTK_BOX (hbox), VarName, TRUE, TRUE, 0);
    
  	IconName = variable = gtk_entry_new ();
  	gtk_widget_show (variable);
  	gtk_box_pack_start (GTK_BOX (hbox), variable, TRUE, TRUE, 0);
 	gtk_entry_set_text (GTK_ENTRY (variable), icondata->outputname);
 // 	gtk_entry_set_text (GTK_ENTRY (variable), "asd");
  	
  	help = gtk_button_new_with_label (N_("Help"));
  	gtk_widget_show (help);
  	gtk_box_pack_start (GTK_BOX (hbox), help, TRUE, TRUE, 0);
  	g_signal_connect (G_OBJECT (help), "clicked",
  			    G_CALLBACK (show_info_callback),
  			    (void*)(N_("Full Pathname to Iconfile, openmode is append !")));
  	
  	separator = gtk_hseparator_new ();
  	/* The last 3 arguments to gtk_box_pack_start are:
  	 * expand, fill, padding. */
  	gtk_box_pack_start (GTK_BOX (vbox), separator, FALSE, TRUE, 5);
  	gtk_widget_show (separator);
  	
  	// make Option Menus
  	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
  	gtk_widget_show (hbox);
  	gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  
  	GtkWidget *mvbox, *label, *wid, *menu, *menuitem;
  	MKICONSPI_OPTIONS *opt;
  	char const **item;
  	OptIndex OptI;
  	int i,j;
  
  	PI_DEBUG(DBG_L4, "MkIconsPIControl::run - Setting options");
  
  
  	XsmRescourceManager xrm(MK_ICONS_KEYBASE);
  	for(j=0, opt = OptionsList; opt->name; ++j, ++opt){
  		// Init from rescources
  		gchar *idefault=g_strdup_printf("%d", opt->init);
  		xrm.Get(opt->name, &opt->init, idefault);
  		g_free(idefault);
  
  		mvbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  		gtk_widget_show (mvbox);
  		gtk_box_pack_start (GTK_BOX (hbox), mvbox, TRUE, TRUE, 0);
  		
  		label = gtk_label_new (opt->name);
  		gtk_widget_show (label);
  		gtk_container_add (GTK_CONTAINER (mvbox), label);
  		
  		wid = gtk_option_menu_new ();
  		gtk_widget_show (wid);
  		gtk_container_add (GTK_CONTAINER (mvbox), wid);
  		
  		menu = gtk_menu_new ();
  		
  		// fill options in
  		for(i=0, item=opt->list; *item; ++i, ++item){
  			// make menuitem
  			menuitem = gtk_menu_item_new_with_label (*item);
  			gtk_widget_show (menuitem);
  			gtk_menu_append (GTK_MENU (menu), menuitem);
  			/* connect with signal-handler if selected */
  			OptI.l = 0L;
  			OptI.s.oi = j;
  			OptI.s.x  = i;
  			g_object_set_data(G_OBJECT (menuitem), "optindex", (gpointer)OptI.l);
  			g_signal_connect (G_OBJECT (menuitem), "activate",
  					    G_CALLBACK (MkIconsPIControl::option_choice_callback),
  					    this);
  		}
  		gtk_option_menu_set_menu (GTK_OPTION_MENU (wid), menu);
  		gtk_option_menu_set_history (GTK_OPTION_MENU (wid), opt->init);
  	}
  
  	PI_DEBUG(DBG_L4, "MkIconsPIControl::run - Setting options ... OK");
  	
  	PI_DEBUG(DBG_L4, "MkIconsPIControl::run - Dlg show, run");
  
  	gtk_widget_show(dialog);
    	dlg_clicked (gtk_dialog_run (GTK_DIALOG(dialog)));
  	gtk_widget_destroy (dialog);

}

void MkIconsPIControl::option_choice_callback(GtkWidget *widget, MkIconsPIControl *mki){
	OptIndex i;
	i.l=(long)g_object_get_data( G_OBJECT (widget), "optindex");
	mki->icondata->options[i.s.oi] = OptionsList[i.s.oi].id[OptionsList[i.s.oi].init=i.s.x];
	PI_DEBUG(DBG_L2, "MkIconsPIControl::option_choice_callback=" << mki->icondata->options );
}

void MkIconsPIControl::dlg_clicked(gint response){

	PI_DEBUG(DBG_L4, "MkIconsPIControl::clicked - " << response);

	g_free(icondata->pathname); 
	icondata->pathname = g_strdup(gtk_entry_get_text (GTK_ENTRY (SrcPath)));
	
	g_free(icondata->mask); 
	icondata->mask = g_strdup(gtk_entry_get_text (GTK_ENTRY (SrcMask)));
	
	g_free(icondata->outputname); 
	icondata->outputname = g_strdup(gtk_entry_get_text (GTK_ENTRY (IconName)));

	switch(response){
	case 1:
		show_info_callback(NULL, N_("Generating 1x1 Icon Pages !"));
		icondata->nix=1;
		MkIconsPI(icondata);
		break;
	case 2:
		show_info_callback(NULL, N_("Generating 2x3 Icon Pages !"));
		icondata->nix=2;
		MkIconsPI(icondata);
		break;
	case 4: 
		show_info_callback(NULL, N_("Generating 4x6 Icon Pages !"));
		icondata->nix=4;
		MkIconsPI(icondata);
		break;
	case 6: 
		show_info_callback(NULL, N_("Generating 6x9 Icon Pages !"));
		icondata->nix=6;
		MkIconsPI(icondata);
		break;
	case GTK_RESPONSE_CANCEL: 
		break;
	}
}


//============================================================//
//  MkIconsPIData class 
//============================================================//

// Directory File Selection Stuff
char select_mask[256];
#ifdef IS_MACOSX
int gxsm_select(struct dirent *item){ return !fnmatch(select_mask, item->d_name, 0); }
#else
int gxsm_select(const struct dirent *item){ return !fnmatch(select_mask, item->d_name, 0); }
#endif

// //  This is a parameter container class with no methods.
MkIconsPIData::MkIconsPIData(const gchar *InPath, const gchar *OutPath, const gchar *InMask, 
			     const gchar *Opt, const gchar *IconName){
  
  	pathname   = InPath   ? g_strdup (InPath)   : g_strdup(".") ;
  	outputname = OutPath  ? g_strdup (OutPath)  : g_strdup("/tmp/icons.ps") ;
  	name       = IconName ? g_strdup (IconName) : g_strdup("icons.ps") ;
  	mask       = InMask   ? g_strdup (InMask)   : g_strdup("*.nc") ;
  	options    = Opt      ? g_strdup (Opt)      : g_strdup("--------") ;
    
  	redres = 0;
  	nix = 4;
}
  
MkIconsPIData::~MkIconsPIData(){
  	g_free(pathname);
  	g_free(outputname);
  	g_free(name);
  	g_free(mask);
  	g_free(options);
}

// //  
// //  // Options: ...out of date...
// //  // EReg:              
// //  //0       [-Ee] do E-Regression, '-': keine 'E': 30% Rand, 'e': 5% Rand
// //  //  HighRes 1200DPI:  
// //  //1       [-h]  Hi Resolution, '-': RedRes=250, 'h': =700 Pixel in A4 Breite
// //  //   AutoSkl
// //  //2       [-a]  Auto Skaling to Min-Max, '-' use defaults, 'a': do skl
// //  //    Lin1D
// //  //3       [-l]  do LineRegression, '-': keine, 'l': do LinReg
// //  //4     [-0123] eps=0.1, 0.05, 0.25, 0.3
// //  // ----
// //  // ehal
// //  // E
// //  // x

//============================================================//
//  MkIconsPI function 
//============================================================//

void MkIconsPI(MkIconsPIData *mid){
  	struct dirent **namelist;
  	int n;
  	double hi0,lo0;
  	lo0=main_get_gapp()->xsm->data.display.cpslow;
  	hi0=main_get_gapp()->xsm->data.display.cpshigh;
  
  	// auto activate Channel or use active one
  	if(!main_get_gapp()->xsm->ActiveScan){
  		int Ch;
  		if((Ch=main_get_gapp()->xsm->FindChan(ID_CH_M_OFF)) < 0){
  			XSM_SHOW_ALERT(ERR_SORRY, ERR_NOFREECHAN,"for Mk Icons",1);
  			return;
  		}
  		if(main_get_gapp()->xsm->ActivateChannel(Ch))
  			return;
  		main_get_gapp()->xsm->ActiveScan->create();
  	}
  	main_get_gapp()->xsm->ActiveScan->CpyDataSet(main_get_gapp()->xsm->data);
  
  	if(mid->nix && main_get_gapp()->xsm->ActiveScan){
  		int redres;
  		const double dpifac=72.*6./300.; // 72.dpi grey bei 300dpi Auflösung und 6in nutzbarer Breite
  		// calculate needed resolution
  		switch(mid->options[MkIconOpt_Resolution]){
  		case '3': redres = (int)(300.*dpifac); break;
  		case '6': redres = (int)(600.*dpifac); break;
  		case 'C': redres = (int)(1200.*dpifac); break;
  		default:  redres = (int)(600.*dpifac); break;
  		}
  		redres /= mid->nix;
  		PI_DEBUG (DBG_L2,  "N Icons in X: " << mid->nix << ", Reduce to Pixels less than " << redres );
  
  		// setup selection mask for "gxsm_select()"
  		strcpy(select_mask, mid->mask);
  		n = scandir(mid->pathname, &namelist, gxsm_select, alphasort);
  		if (n < 0)
  			perror("scandir");
  		else{
  			Scan *Original=NULL, *Icon, *TmpSc, *HlpSc;
  			char fname[256];
  
  			EpsfTools *epsf;
  			if(IS_SPALEED_CTRL)
  				epsf = new SPA_epsftools;
  			else
  				epsf = new SPM_epsftools;
  
  			epsf->SetPaperTyp (mid->options[MkIconOpt_Paper] == 'A' ? A4PAPER:LETTERPAPER);
  			epsf->open(mid->outputname, TRUE);
  			epsf->NIcons(mid->nix);
  			while(n--){
  				PI_DEBUG(DBG_L2, "Loading " << namelist[n]->d_name );
  	
  				// Load
  				sprintf(fname,"%s/%s",mid->pathname,namelist[n]->d_name);
  				PI_DEBUG (DBG_L2, "Icon: " << namelist[n]->d_name);
  				main_get_gapp()->xsm->load(fname);
  	
  				Original = main_get_gapp()->xsm->ActiveScan;
  				G_FREE_STRDUP_PRINTF(Original->data.ui.name, "%s", namelist[n]->d_name); // ohne Pfad !!
  	
  				Icon = main_get_gapp()->xsm->NewScan(0, main_get_gapp()->xsm->data.display.ViewFlg, 0, &main_get_gapp()->xsm->data);
  				Icon->create();
  				TmpSc = main_get_gapp()->xsm->NewScan(0, main_get_gapp()->xsm->data.display.ViewFlg, 0, &main_get_gapp()->xsm->data);
  				TmpSc->create();
  
  				// Copy all data
  				Icon->CpyDataSet(Original->data);
  				TmpSc->CpyDataSet(Original->data);
  
  				// scale down to lower than redres
  				// [0] "Resolution"
  				// ========================================
  				if(CopyScan(Original, Icon))
  					XSM_SHOW_ALERT(ERR_MATH, "", "", 1);
  
  				PI_DEBUG (DBG_L2,  "W00:" << "nx:" << Icon->data.s.nx << " GetNx:" << Icon->mem2d->GetNx() );
  
  				while(Icon->data.s.nx > redres && Icon->data.s.ny > redres){
  					// Excange Icon <=> TmpSc
  					HlpSc = Icon; Icon = TmpSc; TmpSc = HlpSc;
  					// TmpSc => Quench2 => Icon
  					if(TR_QuenchScan(TmpSc, Icon))
  						XSM_SHOW_ALERT(ERR_MATH, "", "", 1);
  					PI_DEBUG (DBG_L2,  "WOK:" << "nx:" << Icon->data.s.nx << " GetNx:" << Icon->mem2d->GetNx() );
  					memcpy( &TmpSc->data, &Icon->data, sizeof(SCAN_DATA));
  				}
  				if(CopyScan(Icon, TmpSc))
  					XSM_SHOW_ALERT(ERR_MATH, "", "", 1);
  
  				PI_DEBUG (DBG_L2,  "Icon :" << "nx:" << Icon->data.s.nx << " GetNx:" << Icon->mem2d->GetNx() );
  				PI_DEBUG (DBG_L2,  "TmpSc:" << "nx:" << TmpSc->data.s.nx << " GetNx:" << TmpSc->mem2d->GetNx() );
  	
  				// do some more math...
  
  				// [1] "E Regression"
  				// ========================================
  				if(mid->options[MkIconOpt_EReg] != '-'){
  					double eps=0.1;
  					PI_DEBUG (DBG_L2,  "doing E regress ..." );
  					// Excange Icon <=> TmpSc
  					HlpSc = Icon; Icon = TmpSc; TmpSc = HlpSc; 
  					// TmpSc => Quench2 => Icon
  					if(mid->options[MkIconOpt_EReg] == 'e') eps=0.05; 
  					if(mid->options[MkIconOpt_EReg] == 'E') eps=0.3; 
  					TmpSc->Pkt2d[0].x = (int)(eps*TmpSc->mem2d->GetNx());
  					TmpSc->Pkt2d[0].y = (int)(eps*TmpSc->mem2d->GetNy());
  					TmpSc->Pkt2d[1].x = (int)((1.-eps)*TmpSc->mem2d->GetNx());
  					TmpSc->Pkt2d[1].y = (int)((1.-eps)*TmpSc->mem2d->GetNy());
  					if(BgERegress(TmpSc, Icon))
  						XSM_SHOW_ALERT(ERR_MATH, "", "", 1);
  					PI_DEBUG (DBG_L2,  "Ereg - Icon :" << "nx:" << Icon->data.s.nx << " GetNx:" << Icon->mem2d->GetNx() );
  					PI_DEBUG (DBG_L2,  "Ereg - TmpSc:" << "nx:" << TmpSc->data.s.nx << " GetNx:" << TmpSc->mem2d->GetNx() );
  				}
  				// [2] "Line Regression"
  				// ========================================
  				if(mid->options[MkIconOpt_LReg] == 'l'){
  					PI_DEBUG (DBG_L2,  "doing BgLin1D ..." );
  					// Excange Icon <=> TmpSc
  					HlpSc = Icon; Icon = TmpSc; TmpSc = HlpSc; 
  					// TmpSc => Quench2 => Icon
  					if(BgLin1DScan(TmpSc, Icon))
  						XSM_SHOW_ALERT(ERR_MATH, "", "", 1);
  					PI_DEBUG (DBG_L2,  "BgLin1D - Icon :" << "nx:" << Icon->data.s.nx << " GetNx:" << Icon->mem2d->GetNx() );
  					PI_DEBUG (DBG_L2,  "BgLin1D - TmpSc:" << "nx:" << TmpSc->data.s.nx << " GetNx:" << TmpSc->mem2d->GetNx() );
  				}
  				// [3] "View Mode" override: -qdlph
  				// ========================================
  				switch(mid->options[MkIconOpt_ViewMode]){
  				case 'q': Icon->mem2d->SetDataPktMode (SCAN_V_QUICK); break;
  				case 'd': Icon->mem2d->SetDataPktMode (SCAN_V_DIRECT); break;
  				case 'l': Icon->mem2d->SetDataPktMode (SCAN_V_LOG); break;
  				case 'h': Icon->mem2d->SetDataPktMode (SCAN_V_HORIZONTAL); break;
  				case 'p': Icon->mem2d->SetDataPktMode (SCAN_V_PERIODIC); break;
  				}
  				// [4] "Auto Scaling" -123
  				// ========================================
  				if(mid->options[MkIconOpt_AutoSkl] != '-'){
  					double eps=0.1;
  					PI_DEBUG (DBG_L2,  "doing Skl ..." );
  					// Excange Icon <=> TmpSc
  					HlpSc = main_get_gapp()->xsm->ActiveScan; main_get_gapp()->xsm->ActiveScan = Icon; 
  					if(mid->options[MkIconOpt_AutoSkl] == '1') eps=0.05; 
  					if(mid->options[MkIconOpt_AutoSkl] == '2') eps=0.20; 
  					if(mid->options[MkIconOpt_AutoSkl] == '3') eps=0.30; 
  					Icon->Pkt2d[0].x = (int)(eps*(double)Icon->mem2d->GetNx());
  					Icon->Pkt2d[0].y = (int)(eps*(double)Icon->mem2d->GetNy());
  					Icon->Pkt2d[1].x = (int)((1.-eps)*(double)Icon->mem2d->GetNx());
  					Icon->Pkt2d[1].y = (int)((1.-eps)*(double)Icon->mem2d->GetNy());
  					Icon->PktVal=2;
  					Icon->mem2d->AutoHistogrammEvalMode (&Icon->Pkt2d[0], &Icon->Pkt2d[1]);
  					// AutoDisplay();
  					Icon->PktVal=0;
  					main_get_gapp()->xsm->ActiveScan = HlpSc;
  					PI_DEBUG (DBG_L2,  "AutoSkl - Icon :" << "nx:" << Icon->data.s.nx << " GetNx:" << Icon->mem2d->GetNx() );
  					Original->data.display.contrast = main_get_gapp()->xsm->data.display.contrast;
  					Original->data.display.bright   = main_get_gapp()->xsm->data.display.bright;
  					PI_DEBUG (DBG_L2, "Contrast=" << Original->data.display.contrast);
  				}
  				if(mid->options[MkIconOpt_Scaling] == 'c'){
  					PI_DEBUG (DBG_L2,  "doing manual Cps hi=" << hi0 << " ... lo=" << lo0 << " Gate=" << main_get_gapp()->xsm->data.display.cnttime);
  					// Excange Icon <=> TmpSc
  					HlpSc = main_get_gapp()->xsm->ActiveScan; main_get_gapp()->xsm->ActiveScan = Icon; 
					main_get_gapp()->xsm->AutoDisplay(hi0*main_get_gapp()->xsm->data.display.cnttime, lo0*main_get_gapp()->xsm->data.display.cnttime);
  					main_get_gapp()->xsm->ActiveScan = HlpSc;
  	  
  				}
  
  				// Put Icon
  				epsf->init();
  	
  				//	    epsf->SetAbbWidth(figwidth);
  				//	    epsf->SetFontSize(fontsize);
  	
  				epsf->placeimage();
  	
  				epsf->putsize(Original);
  				epsf->putframe();
  				epsf->putticks(Original);
  				//	  epsf->putbar(main_get_gapp()->xsm->ActiveScan);
  				//	  epsf->putmore(main_get_gapp()->xsm->ActiveScan, title);
  				//	  epsf->putgrey(Original, Icon->mem2d, options[MkIconOpt_ELReg] == 'a', FALSE);
  				PI_DEBUG (DBG_L2,  "-Original:" << "nx:" << Original->data.s.nx << " GetNx:" << Original->mem2d->GetNx() );
  				PI_DEBUG (DBG_L2,  "-Icon    :" << "nx:" << Icon->data.s.nx << " GetNx:" << Icon->mem2d->GetNx() );
  				// [5] "Scaling": -a
  
  				//	main_get_gapp()->check_events();
  
  				epsf->putgrey(Original, Icon->mem2d, mid->options[MkIconOpt_Scaling] == 'a', FALSE);
  				epsf->endimage();
  				epsf->FootLine(Original);
  	
  				//	delete Icon;
  				delete TmpSc;
  			}
  			epsf->FootLine(Original, TRUE);
  			epsf->close();
  			delete epsf;
  		}
  	}
}
