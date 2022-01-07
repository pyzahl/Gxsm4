/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 8 c-style: "K&R" -*- */

/* Gnome gxsm - Gnome X Scanning Microscopy
 * universal STM/AFM/SARLS/SPALEED/... controlling and
 * data analysis software
 * 
 * Gxsm Plugin Name: Multidim_Movie_Control.C
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
% PlugInModuleIgnore

% BeginPlugInDocuSection
% PlugInDocuCaption: Multi Dimensional Movie Control
% PlugInName: Multidim_Movie_Control
% PlugInAuthor: Percy Zahl
% PlugInAuthorEmail: zahl@users.sf.net
% PlugInMenuPath: windows-sectionMovie Control

% PlugInDescription
Convenience panel for Multi Dimensional Movie Control and playback.

\GxsmScreenShot{Multidim_Movie_Control}{The CCD Control window.}

%% PlugInUsage
%Write how to use it.

%% OptPlugInConfig
%describe the configuration options of your plug in here!

%% OptPlugInRefs

%% OptPlugInKnownBugs

%% OptPlugInNotes
%If you have any additional notes

% EndPlugInDocuSection
 * -------------------------------------------------------------------------------- 
 */


#include <gtk/gtk.h>
#include "config.h"
#include "plugin.h"
#include "surface.h"
#include "unit.h"
#include "pcs.h"
#include "glbvars.h"
//##include "plug-ins/hard/modules/ccd.h"


static void Multidim_Movie_Control_about( void );
static void Multidim_Movie_Control_query( void );
static void Multidim_Movie_Control_cleanup( void );

static void Multidim_Movie_Control_show_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data);
static void Multidim_Movie_Control_StartScan_callback( gpointer );

GxsmPlugin Multidim_Movie_Control_pi = {
	NULL,
	NULL,
	0,
	NULL,
	"Multidim_Movie_Control",
	NULL,
	NULL,
	"Percy Zahl",
	"windows-section",
	N_("Movie Control"),
	N_("open the multi dim movie control panel"),
	"Multi Dimensional Movie Control Panel",
	NULL,
	NULL,
	NULL,
	Multidim_Movie_Control_query,
	// about-function, can be "NULL"
	// can be called by "Plugin Details"
 	Multidim_Movie_Control_about,
	// configure-function, can be "NULL"
	// can be called by "Plugin Details"
	NULL,
	// run-function, can be "NULL", if non-Zero and no query defined, 
	// it is called on menupath->"plugin"
	NULL,
	Multidim_Movie_Control_show_callback, // direct menu entry callback1 or NULL
	NULL, // direct menu entry callback2 or NULL
	// cleanup-function, can be "NULL"
	// called if present at plugin removal
	Multidim_Movie_Control_cleanup
};

static const char *about_text = N_("Gxsm Multidim_Movie_Control Plugin:\n"
				   "This plugin opens a control panel for "
				   "multidimensional movie navigation and playback."
	);

GxsmPlugin *get_gxsm_plugin_info ( void ){ 
	Multidim_Movie_Control_pi.description = g_strdup_printf(N_("Gxsm Multidim_Movie_Control plugin %s"), VERSION);
	return &Multidim_Movie_Control_pi; 
}


class Multidim_Movie_Control : public AppBase{
public:
	Multidim_Movie_Control(Gxsm4app *app);
	virtual ~Multidim_Movie_Control();

	void update();
	static void l_play (GtkWidget *w, Multidim_Movie_Control *mmc);
	static void l_stop (GtkWidget *w, Multidim_Movie_Control *mmc);
	static void l_rewind (GtkWidget *w, Multidim_Movie_Control *mmc);
	static void t_play (GtkWidget *w, Multidim_Movie_Control *mmc);
	static void t_stop (GtkWidget *w, Multidim_Movie_Control *mmc);
	static void t_rewind (GtkWidget *w, Multidim_Movie_Control *mmc);

	static int play_layers (gpointer data);
	static int play_times (gpointer data);
	
private:
	gboolean stop_play_layer;
	gboolean stop_play_time;
	double frame_delay;

	UnitObj *Unity, *Time;
	GtkWidget *contineous_autodisp;
public:
	gboolean t_play_flg;
	gboolean l_play_flg;
	int play_direction;
	GtkWidget *play_mode;
};


Multidim_Movie_Control *Multidim_Movie_ControlClass = NULL;
Multidim_Movie_Control *this_movie_control=NULL;

static void MovieControl_Freeze_callback (gpointer x){	 
         if (this_movie_control)	 
                 this_movie_control->freeze ();	 
}	 
static void MovieControl_Thaw_callback (gpointer x){	 
	if (this_movie_control)	 
		this_movie_control->thaw ();	 
}


static void Multidim_Movie_Control_query(void)
{
        PI_DEBUG_GM (DBG_L2, "PLUGIN: CONTROL=>Multidim_Movie_Control_query");
	// new ...
	Multidim_Movie_ControlClass = new Multidim_Movie_Control (main_get_gapp() -> get_app ());

	Multidim_Movie_Control_pi.app->ConnectPluginToStartScanEvent
		( Multidim_Movie_Control_StartScan_callback );

	Multidim_Movie_Control_pi.status = g_strconcat(N_("Plugin query has attached "),
					   Multidim_Movie_Control_pi.name, 
					   N_(": Multidim_Movie_Control is created."),
					   NULL);
}

static void Multidim_Movie_Control_about(void)
{
	const gchar *authors[] = { "Percy Zahl", NULL};
	gtk_show_about_dialog (NULL,
			       "program-name",  Multidim_Movie_Control_pi.name,
			       "version", VERSION,
			       "license", GTK_LICENSE_GPL_3_0,
			       "comments", about_text,
			       "authors", authors,
			       NULL
			       );
}

static void Multidim_Movie_Control_cleanup( void ){
	PI_DEBUG_GM (DBG_L2, "Multidim_Movie_Control Plugin Cleanup" );
	// delete ...
	if( Multidim_Movie_ControlClass )
		delete Multidim_Movie_ControlClass ;
}

static void Multidim_Movie_Control_show_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data){
	if( Multidim_Movie_ControlClass )
		Multidim_Movie_ControlClass->show();
}

static void Multidim_Movie_Control_StartScan_callback( gpointer ){
	Multidim_Movie_ControlClass->update();
}

#define XSM main_get_gapp()->xsm

Multidim_Movie_Control::Multidim_Movie_Control (Gxsm4app *app):AppBase(app)
{
	stop_play_layer = TRUE;
	stop_play_time  = TRUE;
	frame_delay = 20.;
	t_play_flg = false;
	l_play_flg = false;
	play_direction = 1;

        PI_DEBUG_GM (DBG_L2, "PLUGIN: CONTROL=>Multidim_Movie_Control::Multidim_Movie_Control BUILDING");

	Unity    = new UnitObj(" "," ");
	Time     = new UnitObj("ms","ms");

	AppWindowInit(N_("Multi Dimensional Movie Control"));
        BuildParam *mmc_bp = new BuildParam (v_grid);
        gtk_widget_show (v_grid);

        mmc_bp->new_grid_with_frame (N_("Multi Dimensional Movie Control"));
        mmc_bp->set_no_spin (false);
        mmc_bp->grid_add_ec_with_scale (N_("Layer"), Unity, &XSM->data.display.vlayer, 0, 10, ".0f");
        mmc_bp->set_ec_change_notice_fkt (App::spm_select_layer, main_get_gapp());
        mmc_bp->grid_add_button_icon_name ("media-playback-start", NULL, 1,
                                           G_CALLBACK (Multidim_Movie_Control::l_play), this);
        mmc_bp->grid_add_button_icon_name ("media-playback-stop", NULL, 1,
                                           G_CALLBACK (Multidim_Movie_Control::l_stop), this);
        mmc_bp->grid_add_button_icon_name ("media-seek-backward", NULL, 1,
                                           G_CALLBACK (Multidim_Movie_Control::l_rewind), this);
	mmc_bp->new_line ();

        mmc_bp->grid_add_ec_with_scale (N_("Time"), Unity, &XSM->data.display.vframe, 0, 10, ".0f");
        mmc_bp->set_ec_change_notice_fkt (App::spm_select_time, main_get_gapp());
        mmc_bp->grid_add_button_icon_name ("media-playback-start", NULL, 1,
                                           G_CALLBACK (Multidim_Movie_Control::t_play), this);
        mmc_bp->grid_add_button_icon_name ("media-playback-stop", NULL, 1,
                                           G_CALLBACK (Multidim_Movie_Control::t_stop), this);
        mmc_bp->grid_add_button_icon_name ("media-seek-backward", NULL, 1,
                                           G_CALLBACK (Multidim_Movie_Control::t_rewind), this);
	mmc_bp->new_line ();
        mmc_bp->grid_add_ec (N_("Delay"), Time, &frame_delay, 1., 10000., ".0f");
        contineous_autodisp = mmc_bp->grid_add_check_button (N_("Contineous AutoDisp"));

        play_mode = gtk_combo_box_text_new ();
	gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (play_mode),"once", "Once");
	gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (play_mode),"loop", "Loop");
	gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (play_mode),"rock", "Rock");
	gtk_combo_box_set_active (GTK_COMBO_BOX (play_mode), 0);
        mmc_bp->grid_add_widget (play_mode,3);

	g_object_set_data( G_OBJECT (window), "MMC_EC_list", mmc_bp->get_ec_list_head ());
        
	this_movie_control=this;
	Multidim_Movie_Control_pi.app->ConnectPluginToStartScanEvent (MovieControl_Freeze_callback);	 
	Multidim_Movie_Control_pi.app->ConnectPluginToStopScanEvent (MovieControl_Thaw_callback);

	set_window_geometry ("multi-dim-movie-control");
}

Multidim_Movie_Control::~Multidim_Movie_Control (){
	this_movie_control=NULL;
	delete Unity;
	delete Time;
}

void Multidim_Movie_Control::update(){
	g_slist_foreach((GSList*)g_object_get_data( G_OBJECT (window), "MMC_EC_list"),
			(GFunc) App::update_ec, NULL);
}

void Multidim_Movie_Control::l_play (GtkWidget *w, Multidim_Movie_Control *mmc){
	if (mmc->t_play_flg) return;
	if (XSM->data.s.nvalues <= 1) return;
	switch (gtk_combo_box_get_active (GTK_COMBO_BOX (mmc->play_mode))){
	case 0: mmc->play_direction = 1; break;
	case 1: mmc->play_direction = 1; break;
	case 2: mmc->play_direction = mmc->play_direction>0 ? -1:1; break;
	default: break;
	}
	mmc->stop_play_layer = FALSE;
        g_timeout_add ((guint)mmc->frame_delay, Multidim_Movie_Control::play_layers, mmc);
}

int Multidim_Movie_Control::play_layers (gpointer data){
	Multidim_Movie_Control *mmc = (Multidim_Movie_Control *)data;
	static int id=0;
	int l = XSM->data.display.vlayer;

	l += mmc->play_direction;
	if (l < XSM->data.s.nvalues && l >= 0){
		XSM->data.display.vlayer = l;
		App::spm_select_layer (NULL, main_get_gapp());
		if (mmc->frame_delay > 0.){
			if ( gtk_check_button_get_active (GTK_CHECK_BUTTON (mmc->contineous_autodisp)))
				main_get_gapp()->xsm->ActiveScan->auto_display ();
			else
				main_get_gapp()->spm_update_all ();
			mmc->update ();
			if (!mmc->stop_play_layer)
				g_timeout_add ((guint)mmc->frame_delay, Multidim_Movie_Control::play_layers, mmc);
		}
		return id;
	}
	switch (gtk_combo_box_get_active (GTK_COMBO_BOX (mmc->play_mode))){
	case 0: mmc->play_direction = 1; break;
	case 1: mmc->play_direction = 1; 
		XSM->data.display.vlayer = 0;
		App::spm_select_layer (NULL, main_get_gapp());
		if (!mmc->stop_play_layer)
			g_timeout_add ((guint)mmc->frame_delay, Multidim_Movie_Control::play_layers, mmc);
		break;
	case 2: mmc->play_direction = mmc->play_direction>0 ? -1:1;
		if (!mmc->stop_play_layer)
			g_timeout_add ((guint)mmc->frame_delay, Multidim_Movie_Control::play_layers, mmc);
		break;
	default: break;
	}

	mmc->update ();
	main_get_gapp()->spm_update_all ();

	return id;
}

void Multidim_Movie_Control::l_stop (GtkWidget *w, Multidim_Movie_Control *mmc){
	mmc->stop_play_layer = TRUE;
	mmc->update ();
	main_get_gapp()->spm_update_all ();
	mmc->l_play_flg = false;
}
void Multidim_Movie_Control::l_rewind (GtkWidget *w, Multidim_Movie_Control *mmc){
	XSM->data.display.vlayer=0;
	App::spm_select_layer (NULL, main_get_gapp());
	mmc->update ();
	main_get_gapp()->spm_update_all ();
}

void Multidim_Movie_Control::t_play (GtkWidget *w, Multidim_Movie_Control *mmc){
	if (mmc->l_play_flg) return;
	if (XSM->data.s.ntimes <= 1) return;
	switch (gtk_combo_box_get_active (GTK_COMBO_BOX (mmc->play_mode))){
	case 0: mmc->play_direction = 1; break;
	case 1: mmc->play_direction = 1; break;
	case 2: mmc->play_direction = mmc->play_direction>0 ? -1:1; break;
	default: break;
	}
	mmc->stop_play_time = FALSE;
        g_timeout_add ((guint)mmc->frame_delay, Multidim_Movie_Control::play_times, mmc);
}
	
int Multidim_Movie_Control::play_times (gpointer data){
	Multidim_Movie_Control *mmc = (Multidim_Movie_Control *)data;
	static int id=0;
	int l = XSM->data.display.vframe;

	l += mmc->play_direction;
	if (l < XSM->data.s.ntimes && l >= 0){
		XSM->data.display.vframe = l;
		App::spm_select_time (NULL, main_get_gapp());
		if (mmc->frame_delay > 0.){
			if (gtk_check_button_get_active (GTK_CHECK_BUTTON (mmc->contineous_autodisp)))
			        main_get_gapp()->xsm->ActiveScan->auto_display ();
			else
				main_get_gapp()->spm_update_all ();
			mmc->update ();
		}

		if (!mmc->stop_play_time)
			g_timeout_add ((guint)mmc->frame_delay, Multidim_Movie_Control::play_times, mmc);
		return id;
	}
	switch (gtk_combo_box_get_active (GTK_COMBO_BOX (mmc->play_mode))){
	case 0: mmc->play_direction = 1; break;
	case 1: mmc->play_direction = 1;
		XSM->data.display.vframe = 0;
		App::spm_select_time (NULL, main_get_gapp());
		if (!mmc->stop_play_time)
			g_timeout_add ((guint)mmc->frame_delay, Multidim_Movie_Control::play_times, mmc);
		break;
	case 2: mmc->play_direction = mmc->play_direction>0 ? -1:1;
		if (!mmc->stop_play_time)
			g_timeout_add ((guint)mmc->frame_delay, Multidim_Movie_Control::play_times, mmc);
		break;
	default: break;
	}
	mmc->update ();
	main_get_gapp()->spm_update_all ();
	return id;
}

void Multidim_Movie_Control::t_stop (GtkWidget *w, Multidim_Movie_Control *mmc){
	mmc->stop_play_time = TRUE;
	mmc->update ();
	main_get_gapp()->spm_update_all ();
	mmc->t_play_flg = false;
}
void Multidim_Movie_Control::t_rewind (GtkWidget *w, Multidim_Movie_Control *mmc){
	XSM->data.display.vframe=0;
	App::spm_select_time (NULL, main_get_gapp());
	mmc->update ();
	main_get_gapp()->spm_update_all ();
}
