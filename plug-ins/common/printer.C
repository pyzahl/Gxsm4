/* Gnome gxsm - Gnome X Scanning Microscopy
 * universal STM/AFM/SARLS/SPALEED/... controlling and
 * data analysis software
 *
 * Gxsm Plugin Name: printer.C
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
% PlugInDocuCaption: Postscript printing tool
% PlugInName: printer
% PlugInAuthor: Stefan Schr\"oder
% PlugInAuthorEmail: stefan_fkp@users.sf.net
% PlugInMenuPath: File/Print

% PlugInDescription
This plugin replaces the GXSM core print utility.

% PlugInUsage
 Choose Print from the \GxsmMenu{File} menu. 
This Plugin can be run from the remote python script with
\verb+emb.action("print_PI")+. The default action is to print
to a file with the same name as the nc-file. Other defaults are
read from the gconf-registry. We don not print to a printer by default
to allow mangling of the resulting PS-files after the PS-file creation.
(It's a feature, not a bug. Later, configuration of the print action
may be possible. Stay tuned.)
% EndPlugInDocuSection
 *
 --------------------------------------------------------------------------------
*/

#include <gtk/gtk.h>
#include "config.h"
#include "plugin.h"
#include "epsfutils.h"
#include "glbvars.h"
#include "printer.h"
#include "pyremote.h"
#include "mkicons.h"

// Plugin Prototypes
static void printer_init( void );
static void printer_about( void );
static void printer_configure( void );
static void printer_query( void );
static void printer_cleanup( void );
static void printer_run(GtkWidget *w, void *data);

// for remotecontrolPI
static void printer_run_non_interactive(GtkWidget *w, gpointer pc);

// Fill in the GxsmPlugin Description here
GxsmPlugin printer_pi = {
	NULL,                   // filled in and used by Gxsm, don't touch !
	NULL,                   // filled in and used by Gxsm, don't touch !
	0,                      // filled in and used by Gxsm, don't touch !
	NULL,                   // The Gxsm-App Class Ref.pointer (called "gapp" in Gxsm) is 
	// filled in here by Gxsm on Plugin load, 
	// just after init() is called !!!
	// ----------------------------------------------------------------------
	// Plugins Name, CodeStly is like: Name-M1S[ND]|M2S-BG|F1D|F2D|ST|TR|Misc
	(char *)"Printer",
	NULL,
	// Description, is shown by PluginViewer (Plugin: listplugin, Tools->Plugin Details)
	(char *)"Multifunction Printer PI.",
	// Author(s)
	N_("Stefan Schroeder"),
	// Menupath to position where it is appended to
	N_("_File/"),
	// Menuentry
	N_("Print"),
	// help text shown on menu
	N_("Printer Control."),
	// more info...
	(char *)"no more info",
	NULL,          // error msg, plugin may put error status msg here later
	NULL,          // Plugin Status, managed by Gxsm, plugin may manipulate it too
	// init-function pointer, can be "NULL", 
	// called if present at plugin load
	printer_init,  
	// query-function pointer, can be "NULL", 
	// called if present after plugin init to let plugin manage it install itself
	printer_query, // query should be "NULL" for Gxsm-Math-Plugin !!!
	// about-function, can be "NULL"
	// can be called by "Plugin Details"
	printer_about,
	// configure-function, can be "NULL"
	// can be called by "Plugin Details"
	printer_configure,
	// run-function, can be "NULL", if non-Zero and no query defined, 
	// it is called on menupath->"plugin"
	NULL, // run should be "NULL" for Gxsm-Math-Plugin !!!
	// cleanup-function, can be "NULL"
	// called if present at plugin removal
  NULL, // direct menu entry callback1 or NULL
  NULL, // direct menu entry callback2 or NULL

	printer_cleanup
};

// Text used in Aboutbox, please update!!a
static const char *about_text = N_("Gxsm Plugin\n\n"
                                   "Printer Plugin + Control.");

// Symbol "get_gxsm_plugin_info" is resolved by dlsym from Gxsm, used to get Plugin's info!! 
GxsmPlugin *get_gxsm_plugin_info ( void ){ 
	printer_pi.description = g_strdup_printf(N_("Gxsm printer plugin %s"), VERSION);
	return &printer_pi; 
}

// 5.) Start here with the plugins code, vars def., etc.... here.
// ----------------------------------------------------------------------
//

#define PI_PRINT_FILE     0x01
#define PI_PRINT_PRINTER  0x02
#define PI_PRINT_PREVIEW  0x04
#define PI_PRINT_FRAME    0x10
#define PI_PRINT_SCALE    0x20
#define PI_PRINT_REGIONS  0x40
 

const gchar *pOpt_Type[] = {"plain", "with ticks", "with ticks, Off.=0", "with bar", "circular", NULL};
const gchar *pOpt_Info[] = {"none", "size only", "more", "all", NULL};


MKICONSPI_OPTIONS pOptionsList[] = {
	{ "Type", pOpt_Type, "ptzbc", 0 },
	{ "Info", pOpt_Info, "nsma", 0 },
	{ NULL, 0 }
};

union pOptIndex {
	struct { unsigned char oi, x, y, z; } s;
	unsigned long   l;
};


PIPrintControl::PIPrintControl (){
	// get defaults
	// gnome_config_push_prefix (gnome_client_get_config_prefix (gnome_master_client ()));
	// gnome_config_get_string("PrintPS/title=.")
	// gnome_config_pop_prefix ();

	pdata = NULL;
	Fw = NULL;
	Fs = NULL;

}

void PIPrintControl::cleanup (){

	if(Fw)
		delete Fw;
	Fw = NULL;
	
	if(Fs)
		delete Fs;
	Fs = NULL;
	
	if(pdata)
		delete pdata;
	pdata = NULL;
}

PIPrintControl::~PIPrintControl (){

	cleanup();
}


PIPrintPSData::PIPrintPSData(){
	gchar *tmp, *p;
	MKICONSPI_OPTIONS *opt;

  	XsmRescourceManager xrm("PrintOptions");

	title  = xrm.GetStr ("Titel", "My image (by GXSM)");
	prtcmd = xrm.GetStr ("PrintCommand", "lpr");
	previewcmd = xrm.GetStr ("PreviewCommand", "gv");

	PI_DEBUG (DBG_L4, "PIPrintPSData::PIPrintPSData()");

	if( main_get_gapp()->xsm->ActiveScan ){
		tmp = g_strdup(main_get_gapp()->xsm->ActiveScan->data.ui.name);
		if(strrchr(tmp, '.') > tmp){
			*(p = strrchr(tmp, '.')) = 0;
			fname  = g_strconcat(tmp, ".ps", NULL);
			*p = '.';
		}else
			fname  = g_strdup("/tmp/noname.ps");
		g_free(tmp);
	}
	else
		fname  = g_strdup("/tmp/noscan.ps");

	ptUnit = new UnitObj("pt","pt");
	mmUnit = new UnitObj("mm","mm");
	fontsize = 12.;
	figwidth = 177.;
	mode = 0;
	typ  = 0;
	info = 0;	
	PI_DEBUG (DBG_L4, "PIPrintPSData::PIPrintPSData done");
}
void PIPrintControl::run(){
	GtkWidget *dialog;
	GtkWidget *vbox;
	GtkWidget *hbox;
	GtkWidget *VarName;
	GtkWidget *variable;
	GtkWidget *help;
	GtkWidget *separator;
	GtkWidget *checkbutton;
	GtkWidget *radiobutton;
	GSList    *radiogroup;

	DlgStart();
	pdata = new PIPrintPSData();

	dialog = gnome_dialog_new(N_("Gxsm Print"),
				  GNOME_STOCK_BUTTON_OK, 
				  GNOME_STOCK_BUTTON_CANCEL, 
				  NULL); 
        
	gnome_dialog_set_close(GNOME_DIALOG(dialog), FALSE);
	gnome_dialog_close_hides(GNOME_DIALOG(dialog), FALSE);
	gnome_dialog_set_default(GNOME_DIALOG(dialog), 3);
        
	//  gtk_widget_set_size_request (dialog, 500, 400);
        
	vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	gtk_widget_show (vbox);

	gtk_box_pack_start(GTK_BOX(GNOME_DIALOG(dialog)->vbox),
			   vbox, TRUE, TRUE, GXSM_WIDGET_PAD);
        

	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_widget_show (hbox);
	/* The last 3 arguments to gtk_box_pack_start are:
	 * expand, fill, padding. */
	gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
                        
	VarName = gtk_label_new (N_("Title"));
	gtk_widget_set_size_request (VarName, 50, -1);
	gtk_widget_show (VarName);
	gtk_label_set_justify(GTK_LABEL(VarName), GTK_JUSTIFY_LEFT);
	gtk_misc_set_alignment (GTK_MISC (VarName), 0.0, 0.5);
	gtk_misc_set_padding (GTK_MISC (VarName), 5, 0);
	gtk_box_pack_start (GTK_BOX (hbox), VarName, TRUE, TRUE, 0);
  
	title = variable = gtk_entry_new ();
	gtk_widget_set_size_request (title, 200, -1);
	gtk_widget_show (variable);
	gtk_box_pack_start (GTK_BOX (hbox), variable, TRUE, TRUE, 0);
	gtk_entry_set_text (GTK_ENTRY (variable), pdata->title);

	help = gtk_button_new_with_label (N_("Help"));
	gtk_widget_show (help);
	gtk_box_pack_start (GTK_BOX (hbox), help, TRUE, TRUE, 0);
	g_signal_connect (G_OBJECT (help), "clicked",
			    G_CALLBACK (show_info_callback),
			    (void*)(N_("Title is placed above Scanimage.")));


	separator = gtk_hseparator_new ();
	gtk_box_pack_start (GTK_BOX (vbox), separator, FALSE, TRUE, 5);
	gtk_widget_show (separator);

	// make pOption Menus
	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_widget_show (hbox);
	gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

	GtkWidget *mvbox, *label, *wid, *menu, *menuitem;
	MKICONSPI_OPTIONS *opt;
	char const **item;
	pOptIndex pOptI;
	int i,j;

	XsmRescourceManager xrm("PrintOptions");
	for(j=0, opt = pOptionsList; opt->name; ++j, ++opt){
		// Init from ressources
		gchar *idefault=g_strdup_printf("%d",opt->init);
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
			menuitem = gtk_menu_item_new_with_label (*item);
			gtk_widget_show (menuitem);
			gtk_menu_append (GTK_MENU (menu), menuitem);
			/* connect with signal-handler if selected */
			pOptI.l = 0L;
			pOptI.s.oi = j;
			pOptI.s.x  = i;
			g_object_set_data(G_OBJECT (menuitem), "optindex", (gpointer)pOptI.l);
			g_signal_connect (G_OBJECT (menuitem), "activate",
					    G_CALLBACK (PIPrintControl::option_choice_callback),
					    this);
		}
		gtk_option_menu_set_menu (GTK_OPTION_MENU (wid), menu);
		gtk_option_menu_set_history (GTK_OPTION_MENU (wid), opt->init);
	}
 
	// Scale, Frame -buttons

	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_widget_show (hbox);
	gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

	scalebutton = checkbutton = gtk_check_button_new_with_label( "scale full range");
	gtk_box_pack_start (GTK_BOX (hbox), checkbutton, TRUE, TRUE, 0);
	gtk_widget_show (checkbutton);

	framebutton = checkbutton = gtk_check_button_new_with_label( "Frame");
	gtk_box_pack_start (GTK_BOX (hbox), checkbutton, TRUE, TRUE, 0);
	gtk_widget_show (checkbutton);

	regionbutton = checkbutton = gtk_check_button_new_with_label( "Regions (exp.)");
	gtk_box_pack_start (GTK_BOX (hbox), checkbutton, TRUE, TRUE, 0);
	gtk_widget_show (checkbutton);

	separator = gtk_hseparator_new ();
	gtk_box_pack_start (GTK_BOX (vbox), separator, FALSE, TRUE, 5);
	gtk_widget_show (separator);

	GtkWidget *input;
	input = mygtk_create_input("Fontsize", vbox, hbox);
	Fs = new Gtk_EntryControl (pdata->ptUnit, "Value out of range", &pdata->fontsize, 
				   1., 64., ".1f", input);
	
	input = mygtk_add_input("Figwidth", hbox);
	Fw = new Gtk_EntryControl (pdata->mmUnit, "Value out of range", &pdata->figwidth, 
				   10., 1000., ".1f", input);
	
	// Destination selection

	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_widget_show (hbox);
	gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
                        
	filebutton = radiobutton = gtk_radio_button_new_with_label (NULL, N_("Save to file"));
	gtk_widget_set_size_request (radiobutton, 100, -1);
	gtk_box_pack_start (GTK_BOX (hbox), radiobutton, TRUE, TRUE, 0);
	gtk_widget_show (radiobutton);
	radiogroup = gtk_radio_button_group (GTK_RADIO_BUTTON (radiobutton));
  
	fname = variable = gtk_entry_new ();
	gtk_widget_show (variable);
	gtk_box_pack_start (GTK_BOX (hbox), variable, TRUE, TRUE, 0);
	gtk_entry_set_text (GTK_ENTRY (variable), pdata->fname);

	// Print
	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_widget_show (hbox);
	gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
                     
	printbutton = radiobutton = gtk_radio_button_new_with_label (radiogroup, N_("Print command"));
	radiogroup = gtk_radio_button_group (GTK_RADIO_BUTTON (radiobutton));
	gtk_widget_set_size_request (radiobutton, 100, -1);
	gtk_box_pack_start (GTK_BOX (hbox), radiobutton, TRUE, TRUE, 0);
	gtk_widget_show (radiobutton);

	prtcmd = variable = gtk_entry_new ();
	gtk_widget_show (variable);
	gtk_box_pack_start (GTK_BOX (hbox), variable, TRUE, TRUE, 0);
	gtk_entry_set_text (GTK_ENTRY (variable), pdata->prtcmd);
  
	// Preview 
	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_widget_show (hbox);
	gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
                        
	previewbutton = radiobutton = gtk_radio_button_new_with_label (radiogroup, N_("Preview via"));
	radiogroup = gtk_radio_button_group (GTK_RADIO_BUTTON (radiobutton));
	gtk_widget_set_size_request (radiobutton, 100, -1);
	gtk_box_pack_start (GTK_BOX (hbox), radiobutton, TRUE, TRUE, 0);
	gtk_widget_show (radiobutton);

	previewcmd = variable = gtk_entry_new ();
	gtk_widget_show (variable);
	gtk_box_pack_start (GTK_BOX (hbox), variable, TRUE, TRUE, 0);
	gtk_entry_set_text (GTK_ENTRY (variable), pdata->previewcmd);

	// Setup defaults

	gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON (scalebutton) , TRUE);
	gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON (filebutton) , FALSE);
	gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON (printbutton) , TRUE);
	gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON (previewbutton) , FALSE);

	pdata->typ  = (int)pOptionsList[0].id[pOptionsList[0].init];
	pdata->info = (int)pOptionsList[1].id[pOptionsList[1].init];

	// ========================================

	g_signal_connect(G_OBJECT(dialog), "clicked",
			   G_CALLBACK(PIPrintControl::dlg_clicked),
			   this);
  
	gtk_widget_show(dialog);
	
	
}



void PIPrintControl::option_choice_callback(GtkWidget *widget, PIPrintControl *pc){
	pOptIndex i;
	i.l=(long)g_object_get_data( G_OBJECT (widget), "optindex");
	switch(i.s.oi){
	case 0: pc->pdata->typ  = (int)pOptionsList[i.s.oi].id[pOptionsList[i.s.oi].init=i.s.x]; break;
	case 1: pc->pdata->info = (int)pOptionsList[i.s.oi].id[pOptionsList[i.s.oi].init=i.s.x]; break;
	}//  cout << "PrintControl::option_choice_callback=" << (char)pc->pdata->typ << (char)pc->pdata->info << endl;
}

void PIPrintControl::dlg_clicked(GnomeDialog * dialog, gint button_number, PIPrintControl *pc){

	g_free(pc->pdata->title); 
	g_free(pc->pdata->fname); 
	g_free(pc->pdata->prtcmd); 
	g_free(pc->pdata->previewcmd); 
	pc->pdata->title  = g_strdup(gtk_entry_get_text (GTK_ENTRY (pc->title)));
	pc->pdata->fname  = g_strdup(gtk_entry_get_text (GTK_ENTRY (pc->fname)));
	pc->pdata->prtcmd = g_strdup(gtk_entry_get_text (GTK_ENTRY (pc->prtcmd)));
	pc->pdata->previewcmd = g_strdup(gtk_entry_get_text (GTK_ENTRY (pc->previewcmd)));
 
	pc->pdata->mode = 0;
	if (GTK_TOGGLE_BUTTON (pc->framebutton)->active)   
		pc->pdata->mode |= PI_PRINT_FRAME;
	if (GTK_TOGGLE_BUTTON (pc->scalebutton)->active)
		pc->pdata->mode |= PI_PRINT_SCALE;
	if (GTK_TOGGLE_BUTTON (pc->regionbutton)->active)
		pc->pdata->mode |= PI_PRINT_REGIONS;

	if (GTK_TOGGLE_BUTTON (pc->filebutton)->active)
		pc->pdata->mode |= PI_PRINT_FILE;
	if (GTK_TOGGLE_BUTTON (pc->printbutton)->active)
		pc->pdata->mode |= PI_PRINT_PRINTER;
	if (GTK_TOGGLE_BUTTON (pc->previewbutton)->active)
		pc->pdata->mode |= PI_PRINT_PREVIEW;

	switch(button_number){
	case 0: 
		gnome_dialog_close(dialog); 
		PIPrintPS(pc->pdata);
		break;
	case 1: 
		gnome_dialog_close(dialog); 
		break;
	}
//  cout << "pdata->mode" << pc->pdata->mode<< endl;
	pc->cleanup();

	pc->DlgDone();
}


PIPrintPSData::~PIPrintPSData(){

	MKICONSPI_OPTIONS *opt;

	XsmRescourceManager xrm("PrintOptions");

	xrm.Put ("Titel", title);
	xrm.Put ("PrintCommand", prtcmd);
	xrm.Put ("PreviewCommand", previewcmd);

	for(opt = pOptionsList; opt->name; ++opt)
		xrm.Put(opt->name, opt->init);

	g_free(title);
	g_free(fname);
	g_free(prtcmd);
	g_free(previewcmd);

	delete ptUnit;
	delete mmUnit;
}

// init-Function
static void printer_init(void)
{
  PI_DEBUG (DBG_L2, "printer Plugin Init");

 // This is action remote stuff, stolen from the peak finder PI.
   remote_action_cb *ra;
   GtkWidget *dummywidget = gtk_menu_item_new();
 
   ra = g_new( remote_action_cb, 1);
   ra -> cmd = g_strdup_printf("print_PI");
   ra -> RemoteCb = &printer_run_non_interactive;
   ra -> widget = dummywidget;
   ra -> data = NULL;
   main_get_gapp()->RemoteActionList = g_slist_prepend ( main_get_gapp()->RemoteActionList, ra );
   PI_DEBUG (DBG_L2, "printer-plugin: Adding new Remote Cmd: print_PI");
 // remote action stuff
}

static void printer_run_non_interactive( GtkWidget *widget , gpointer ppc )
{
   PI_DEBUG (DBG_L2, "printer-plugin: print is called from script.");

        PIPrintPSData *pdata = new PIPrintPSData();

//	pdata->fontsize = 10.0;
	
//	pdata->figwidth = 150.0;

//	pdata->typ  = 0;
//	pdata->info = 0;

//	g_free(pdata->title); 
//	g_free(pdata->fname); 
//	g_free(pdata->prtcmd); 
//	g_free(pdata->previewcmd); 

//	pdata->title  = g_strdup("Mein Titel");
//	pdata->fname  = g_strdup("/tmp/test.ps");
//	pdata->prtcmd = g_strdup("lpr");
//	pdata->previewcmd = g_strdup("gv");
 
	pdata->mode = 1;
	PIPrintPS(pdata);

   return;
}


// about-Function
static void printer_about(void)
{
	const gchar *authors[] = { printer_pi.authors, NULL};
	gtk_show_about_dialog (NULL, "program-name",  printer_pi.name,
					"version", VERSION,
					  "license", GTK_LICENSE_GPL_3_0,
					  "comments", about_text,
					  "authors", authors,
					  NULL,NULL,NULL
				));
}

// configure-Function
static void printer_configure(void)
{
	if(printer_pi.app)
		printer_pi.app->message("Printer Plugin Configuration");
}

static void printer_query(void)
{
	printer_pi.app->ConnectPluginToPrinterEvent (printer_run); // connect
}


// cleanup-Function
static void printer_cleanup(void)
{
	printer_pi.app->ConnectPluginToPrinterEvent (NULL); // disconnect
//  cout << "Printer Plugin Cleanup" << endl;
}

// run-Function
static void printer_run( GtkWidget *w, void *data )
{
	static PIPrintControl *pc = NULL;
        if(w){
                if(!pc)
                        pc = new PIPrintControl();
                if(!pc->running())
                        pc->run();
        }else
                if(pc) // destroy if exists
                        delete pc;
	//PIPrintPSData *ppsd = new PIPrintPSData;
	//PIPrintPS(ppsd);
}

void PIPrintPS(PIPrintPSData *ppsd){
	gchar *fname;
	Scan *ActiveScan;
	GError *printer_failure;
	ActiveScan = main_get_gapp()->xsm->GetActiveScan();
	
	if(!ActiveScan)
		XSM_SHOW_ALERT(ERR_SORRY, ERR_NOACTIVESCAN,HINT_ACTIVATESCAN,1);
	else{
		if(ppsd->mode & PI_PRINT_PRINTER || ppsd->mode & PI_PRINT_PREVIEW){
			g_file_open_tmp("GXSM_TEMPFILE_XXXXXX", &fname, &printer_failure); 
			//old fname = g_strdup("/tmp/GXSM_PI_PRINT_TMP.ps");
		}
		else{
			if(ppsd->mode & PI_PRINT_FILE)
				fname = g_strdup(ppsd->fname);
			else{
				g_file_open_tmp("GXSM_ERROR_XXXXXX", &fname, &printer_failure); 
				//fname = g_strdup("/tmp/GXSM_FILE_ERROR.ps"); // Error !!
			}
		}
		EpsfTools *eps;
		if(IS_SPALEED_CTRL)
			eps = new SPA_epsftools;
		else
			eps = new SPM_epsftools;

		eps->open(fname, TRUE, ppsd->typ, ppsd->info);
    
		eps->SetAbbWidth(ppsd->figwidth);
		eps->SetFontSize(ppsd->fontsize);
    
		eps->placeimage();

		if(ppsd->info == 's' || ppsd->info == 'm')
			eps->putsize(ActiveScan);
		if(ppsd->mode & PI_PRINT_FRAME)
			eps->putframe();
		if(ppsd->typ == 'z')
			eps->putticks(ActiveScan);
		if(ppsd->typ == 't')
			eps->putticks(ActiveScan, FALSE);
		if(ppsd->typ == 'b')
			eps->putbar(ActiveScan);
		if(ppsd->info == 'm' || ppsd->info == 'a')
			eps->putmore(ActiveScan, ppsd->title);
    
		eps->putgrey(ActiveScan, ActiveScan->mem2d,
			     ppsd->mode & PI_PRINT_SCALE ? TRUE:FALSE, 
			     FALSE, 
			     ppsd->typ == 'c');

		eps->endimage();
		
		if (ppsd->mode & PI_PRINT_REGIONS){
			scan_object_data* test_scan_obj;
		
			for (unsigned int i=0; i< ActiveScan->number_of_object(); i++){
				test_scan_obj = (scan_object_data*)ActiveScan->get_object_data (i);
				
				/* Handle different objects here: */
				if (test_scan_obj->get_name()[0] == 'L'){ // print 'Line'
					PI_DEBUG (DBG_L4, "PIprinter::PrintPS. Printing Line.");
					double A, B, C, D;
					test_scan_obj->get_xy(0, A, B);
					test_scan_obj->get_xy(1, C, D);
					
					eps->putline(ActiveScan, int(A), int(B), int(C), int(D));
				}
				else if (test_scan_obj->get_name()[0] == 'R'){ // print 'Rectangle'
					PI_DEBUG (DBG_L4, "PIprinter::PrintPS. Printing Rectangle.");
					double A, B, C, D;
					test_scan_obj->get_xy(0, A, B);
					test_scan_obj->get_xy(1, C, D);
					
					eps->putline(ActiveScan, int(A), int(B), int(C), int(B));
					eps->putline(ActiveScan, int(C), int(B), int(C), int(D));
					eps->putline(ActiveScan, int(A), int(B), int(A), int(D));
					eps->putline(ActiveScan, int(A), int(D), int(C), int(D));
				}
				else if (test_scan_obj->get_name()[0] == 'P' &&
					 test_scan_obj->get_name()[2] == 'i'){ // print 'Point'
					PI_DEBUG (DBG_L4, "PIprinter::PrintPS. Printing Point.");
					double A, B;
					test_scan_obj->get_xy(0, A, B);
					eps->putcircle(ActiveScan, int(A), int(B), int(A+1), int(B+1));
				}
				else if (test_scan_obj->get_name()[0] == 'C'){ // print 'Circle'
					PI_DEBUG (DBG_L4, "PIprinter::PrintPS. Printing Circle.");
					double A, B, C, D;
					test_scan_obj->get_xy(0, A, B);
					test_scan_obj->get_xy(1, C, D);
					
					eps->putcircle(ActiveScan, int(A), int(B), int(C), int(D));
				}
				else{
					PI_DEBUG (DBG_L4, "PIprinter::PrintPS. Don't know how to print this: " << test_scan_obj->get_name() );
				}
			}
		}

		//      eps->FootLine();

		eps->close();
		delete eps;

		if(ppsd->mode & PI_PRINT_PRINTER  || ppsd->mode & PI_PRINT_PREVIEW){
			FILE *lprcmd;
			char command[512];
			char retinfo[64];
			int ret;
			sprintf(command, "%s %s\n", 
				ppsd->mode & PI_PRINT_PREVIEW ? 
				ppsd->previewcmd : ppsd->prtcmd, fname);
			lprcmd=popen(command,"r");
			ret=pclose(lprcmd);
			sprintf(retinfo, "%s", ret? "Exec. Error":"OK");
			XSM_SHOW_ALERT("PI PRINT INFO", command, retinfo,1);
			// remove tmp file
			sprintf(command, "rm %s\n", fname);
			lprcmd=popen(command,"r");
			pclose(lprcmd);
		}
		g_free(fname);
	}
}
