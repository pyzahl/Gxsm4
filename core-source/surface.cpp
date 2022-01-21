/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 8 c-style: "K&R" -*- */

/* Gxsm - Gnome X Scanning Microscopy
 * universal STM/AFM/SARLS/SPALEED/... controlling and
 * data analysis software
 * 
 * Copyright (C) 1999,2000,2001,2002,2003 Percy Zahl
 *
 * Authors: Percy Zahl <zahl@users.sf.net>
 * additional features: Andreas Klust <klust@users.sf.net>
 * WWW Home: http://gxsm.sf.net
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

#include <iostream>
#include <fstream>

#include <cstring>

#include <dirent.h>
#include <fnmatch.h>

#include "surface.h"
#include "xsmmasks.h"
#include "glbvars.h"
#include "action_id.h"

#include "dataio.h"
#include "app_profile.h"

#include "epsfutils.h"

#include "vpdata_read.h"

#include "app_view.h"

//moved to pi
//#include "xsm_mkicons.h"
//#include "app_mkicons.h"

#define CHMAX (1+4*PIDCHMAX+4*DAQCHMAX+2*EXTCHMAX)

const gchar *MathErrString[] = {
	"Math OK",
	"Size of sources is not the same !",
	"Size of selection is invalid !",
	"Division by zero occured !",
	"undefined error"
};



Surface::Surface(App *gapp) : Xsm(){
        gxsm4app = NULL;
        g_app = gapp; // main application base class

        StopScanFlg= TRUE;
	ActiveScan = NULL;
	MasterScan = NULL;
	ActiveChannel = 0;
	MasterChannel = 0;

        SetActiveScanList (); // from external scan control
        
	ScanList = NULL;
	ProfileList = NULL;
	DelProfileList = NULL;

	for(int i=0; i<MAX_CHANNELS; i++){
		scan[i] = NULL; // noch keine Scan Objekte !
		ChannelView[i] = ID_CH_V_GREY;
		ChannelMode[i] = ID_CH_M_OFF;
		ChannelScanMode[i] = ID_CH_M_OFF;
                ChannelScanDir[i] = ID_CH_D_P;
                ChannelASflag[i] = 1;
	}
	SetRedraw();

        default_path = NULL;
}

Surface::~Surface(){
	XSM_DEBUG (DBG_L2, "Surface Destructor");
	RemoveAllProfiles();
	for(int i=0; i<MAX_CHANNELS; i++)
		if(scan[i]){
			XSM_DEBUG (DBG_L2, "killing Scan " << i);
			delete scan[i]; // delete Scan Obj.
		}
}

void Surface::CleanupProfiles(){
	g_slist_foreach ((GSList*) DelProfileList, 
			 (GFunc) Surface::remove_profile, this);
	g_slist_free (DelProfileList);
	DelProfileList = NULL;
}

int Surface::AddProfile(gchar *filename){
	if(!ActiveScan){
		CleanupProfiles ();
		ProfileList = g_slist_prepend (ProfileList,
					       g_app->new_profile_control (filename, "ProfileFromFile"));
	} else {
		vpdata_read (get_app (), filename, ActiveScan);
	}
	return 0;
}

int Surface::AddProfile(ProfileControl *pc){
	CleanupProfiles ();
	ProfileList = g_slist_prepend (ProfileList, pc);
	return 0;
}

int Surface::RemoveProfile(ProfileControl *pc){
	ProfileList = g_slist_remove((GSList*) ProfileList, pc); 
	DelProfileList = g_slist_prepend(DelProfileList, pc);
	// add to "to Delete" List
	return 0;
}

void Surface::remove_profile(ProfileControl *pc, gpointer data){ delete pc; }

void Surface::RemoveAllProfiles(){
	g_slist_foreach((GSList*) ProfileList, 
			(GFunc) Surface::remove_profile, this);
	g_slist_free(ProfileList);
	ProfileList = NULL;
}


int Surface::SetView(int Channel, int choice){
	XSM_DEBUG (DBG_L2,  "SetView " << Channel << " " << choice );
	if(Channel < 0 || Channel >= MAX_CHANNELS) return 1;
	if(scan[Channel]){
		if(ChannelView[Channel] != choice){
			ChannelView[Channel] = scan[Channel]->SetView(choice);
		}
		gapp->channelselector->SetView(Channel, choice);
	}
	else
		ChannelView[Channel] = choice;
	return 0;
}

int Surface::SetVM (int mode){
	//  XSM_DEBUG (DBG_L2, "Surface::SetVM m:" << mode << " rdf:" << redrawflg);
	if (mode)
		data.display.ViewFlg = mode;

	if (ActiveScan && redrawflg)
		ActiveScan->SetVM (data.display.ViewFlg, &data, xsmres.HiLoDelta, (double)xsmres.SmartHistEpsilon);
	
	return 0;
}

int Surface::GetVM (){
        return data.display.ViewFlg;
}

// ScanMode signum: scan dir
int Surface::SetSDir(int Channel, int choice){
	if(Channel < 0 || Channel >= MAX_CHANNELS) return -1;
        ChannelScanDir[Channel] = choice;
	return 0;
}

// keep signum (scan dir)!
int Surface::SetMode(int Channel, int choice, int force){
        // int lastmode;
	XSM_DEBUG (DBG_L2, "Surface::SetMode: " << Channel << " " << choice);
	if (Channel == -1) Channel = ActiveChannel;
	if (Channel < 0 || Channel >= MAX_CHANNELS) return -1;
	// lastmode=ChannelMode[Channel];

	switch (choice){
	case ID_CH_M_ACTIVE: 
		XSM_DEBUG (DBG_L2, "Surface::SetMode: ACTIVE " << Channel);
		ActivateChannel (Channel);
		ChannelMode[Channel] = choice;
		return 0;
	case ID_CH_M_ON: 
		if (scan[Channel]){
			if (ChannelMode[Channel] == ID_CH_M_ACTIVE)
				ActiveScan = NULL;
			ChannelMode[Channel] = choice;
                        if (    ChannelScanMode[Channel] == ID_CH_M_MATH
                            || -ChannelScanMode[Channel] == ID_CH_M_MATH) // new 20170912, fixed MATH channel assignments issue
                                ChannelScanMode[Channel] = choice;
		}
		else{
			gapp->channelselector->SetMode(Channel, ID_CH_M_OFF);
			return 0;
		}
		gapp->channelselector->SetMode (Channel, ChannelMode[Channel]);
		return 0;
	}
	XSM_DEBUG (DBG_L2, "Surface::SetMode: CH " << Channel << ":" << choice);
	ChannelMode[Channel] = choice;
	switch (choice){
	case ID_CH_M_OFF: 
		if (scan[Channel]) //&& (!ScanInProgress())
			if (! (scan[Channel]->get_refcount ())){
				if (force || gapp->question_yes_no (Q_KILL_SCAN,
                                                                    scan[Channel]->view ? scan[Channel]->view->Get_ViewControl () ?
                                                                    (scan[Channel]->view->Get_ViewControl ())->get_window ()
                                                                    : NULL : NULL)){
					if (ActiveScan == scan[Channel]){
						ActiveScan = NULL;
					}
					if (MasterScan == scan[Channel]) // remove reference!!!
						MasterScan = NULL;
                                        XSM_DEBUG (DBG_L2, "Surface::SetMode: OFF confirmed -- delete scan in CH" << Channel);
					delete scan[Channel];
					scan[Channel] = NULL;
					ChannelScanMode[Channel] = choice;
                                        gapp->channelselector->SetInfo (Channel, "-");

				}else{ // locked and in used ...
                                        XSM_DEBUG (DBG_L2, "Surface::SetMode: OFF aborted -- setting CH" << Channel << " to ON.");
					ChannelMode[Channel] = ID_CH_M_ON;
					ChannelScanMode[Channel] = ID_CH_M_ON;
				}
			}
		break;
	case ID_CH_M_X:
                ChannelScanMode[Channel] = choice;
                gapp->channelselector->SetInfo (Channel, "X source");
		break;
	case ID_CH_M_MATH:
                ChannelScanMode[Channel] = choice;
                gapp->channelselector->SetInfo (Channel, "Math target");
		break;
	default:
		if(choice > ID_CH_M_X){
                        if (ChannelScanMode[Channel] != choice){
                                gapp->channelselector->SetInfo (Channel, "New scan source");
                                ChannelScanMode[Channel] = choice;
                        }
		}
		else{
			XSM_DEBUG (DBG_L2, "Surface::SetMode: wrong Mode !");
                        gapp->channelselector->SetInfo (Channel, "error set mode");
                }
		break;
	}
	gapp->channelselector->SetMode(Channel, ChannelMode[Channel]);
	return 0;
}

Scan* Surface::NewScan(int vtype, int vflg, int ChNo, SCAN_DATA *vd){
// ToDo: needs to be more flexible in future...

	if (!strncasecmp (xsmres.InstrumentType, "SPALEED",7))
		return ( new SpaScan (vtype, vflg, ChNo, vd, get_app()));  // data type is ZD_LONG
	else
		return ( new TopoGraphicScan (vtype, vflg, ChNo, vd, get_app())); // data type is ZD_SHORT
}

int Surface::ActivateFreeChannel(){
	int Ch=0;
	// auto activate Channel or use active one
	if ((Ch=FindChan (ID_CH_M_OFF)) < 0){
		XSM_SHOW_ALERT(ERR_SORRY, ERR_NOFREECHAN,"View in new Channel",1);
		return 1;
	}
        
	if (ActivateChannel(Ch))
		return 1;

	return 0;
}

int Surface::ActivateChannel(int NewActiveChannel){
	int Ch;
        XSM_DEBUG (DBG_L2, "Surface::ActivateChannel #" << NewActiveChannel);

        if (NewActiveChannel < 0 || NewActiveChannel >= MAX_CHANNELS)
                return -1;
        
        ActiveChannel=NewActiveChannel;
	if(ActiveScan)
		ActiveScan->draw(1,0);
	ActiveScan = NULL;
	// Falls ein Channel "aktiv", dann auf "on" stellen
	// es darf immer nur ein Channel aktiv sein !!!!
	if((Ch=FindChan(ID_CH_M_ACTIVE)) >= 0){
		gapp->channelselector->SetMode(Ch, ID_CH_M_ON);
		ChannelMode[Ch] = ID_CH_M_ON;
		if((Ch=FindChan(ID_CH_M_ACTIVE)) >= 0)
			XSM_DEBUG (DBG_L2, "zu viele Active Channels !!!!");
	}
	// Falls Scan[Chanel] ex., benutze Ihn !
	if(scan[ActiveChannel]){
		ActiveScan = scan[ActiveChannel];
		scan[ActiveChannel]->Activate();
		gapp->channelselector->SetMode(ActiveChannel, ID_CH_M_ACTIVE);
		ChannelMode[ActiveChannel] = ID_CH_M_ACTIVE;
		ActiveScan->draw(1,0);
		return 0;
	}

	// neuen Scan anlegen
	scan[ActiveChannel] = NewScan(ChannelView[ActiveChannel], data.display.ViewFlg, ActiveChannel, &data);
        scan[ActiveChannel] -> set_main_app (get_app());

	// Fehler ?
	if(!scan[ActiveChannel]){
		XSM_SHOW_ALERT(ERR_SORRY, ERR_NOMEM,"",1);
		return -1;
	}

	// Set ActiveScan
	ActiveScan = scan[ActiveChannel];
	gapp->channelselector->SetMode(ActiveChannel, ID_CH_M_ACTIVE);
	ChannelMode[ActiveChannel] = ID_CH_M_ACTIVE;
	ActiveScan->draw();

	return 0;
}

// Service Fkt, suche Channel Nr. mit fid als ID
int Surface::FindChan(int fid, int dir, int start){
	int i;
	for(i=start; i<MAX_CHANNELS; i++){
		if(dir < 0 && ChannelMode[i] == fid){
			//      XSM_DEBUG (DBG_L2, "Surface::FindChan ID=" << fid << " => Match #" << i);
			return i;
		}                
                if(fid != ID_CH_M_OFF && dir >= 0 && ChannelScanMode[i] == fid && ChannelScanDir[i] == dir){
                        //	XSM_DEBUG (DBG_L2, "Surface::FindChan ID=" << fid << " => Match #" << i);
                        ChannelMode[i] = abs(fid);
                        gapp->channelselector->SetMode(i, abs(fid)); // update to scan mode
                        return i;
                }
	}

	//  XSM_DEBUG (DBG_L2, "Surface::FindChan ID=" << fid << " No Match");
	return -1;
}

void Surface::load_exec (GtkDialog *dialog,  int response, gpointer user_data){
        Surface *s = (Surface *) user_data;
        if (response == GTK_RESPONSE_ACCEPT){
                GtkFileChooser *chooser = GTK_FILE_CHOOSER (dialog);
                g_autoptr(GFile) file = gtk_file_chooser_get_file (chooser);
                gchar *tmp=g_file_get_parse_name (file);
		s->load (tmp);
                g_free (tmp);
                g_object_unref (file);
        }
        gtk_window_destroy (GTK_WINDOW (dialog));
}

int Surface::load(const char *rname){
	int ret;
	int Ch;
	std::ifstream f;
	static gchar *path = NULL;
	static double t0=0., t=0.; // Load Time Reference
        xsm::open_mode open_mode = file_open_mode;

	gchar *cname = NULL;
	gchar *fname = NULL;

	XSM_DEBUG (DBG_L2, "load: " << rname);
	// default path from resources
	if(! path)
		path = g_strdup(xsmres.DataPath);

	// File Dialog ?
	if(! rname){
		GtkWidget *chooser = gtk_file_chooser_dialog_new ("NC/asc/vpdata file to load",
                                                                  NULL, 
								  GTK_FILE_CHOOSER_ACTION_OPEN,
								  _("_Cancel"), GTK_RESPONSE_CANCEL,
								  _("_Open"), GTK_RESPONSE_ACCEPT,
								  NULL);
                GFile *gf=g_file_new_for_path (path);
		gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (chooser), gf, NULL);
                g_object_unref (gf);

		GtkFileFilter *nc_filter = gtk_file_filter_new ();
		gtk_file_filter_set_name (nc_filter, "NetCDF");
		gtk_file_filter_add_pattern (nc_filter, "*.nc");
		gtk_file_filter_add_pattern (nc_filter, "*.cdf");
		gtk_file_filter_add_pattern (nc_filter, "*.netcdf");
		gtk_file_filter_add_pattern (nc_filter, "*.NC");
		gtk_file_filter_add_pattern (nc_filter, "*.hdf");
		gtk_file_filter_add_pattern (nc_filter, "*.HDF");
		gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (chooser), nc_filter);
                g_object_unref (nc_filter);

                GtkFileFilter *vp_filter = gtk_file_filter_new ();
		gtk_file_filter_set_name (vp_filter, "VPdata");
		gtk_file_filter_add_pattern (vp_filter, "*.asc");
		gtk_file_filter_add_pattern (vp_filter, "*.vpdata");
		gtk_file_filter_add_pattern (vp_filter, "*.VPdata");
		gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (chooser), vp_filter);
                g_object_unref (vp_filter);

                GtkFileFilter *x_filter = gtk_file_filter_new ();
		gtk_file_filter_set_name (x_filter, "DAT");
		gtk_file_filter_add_pattern (x_filter, "*.dat");
		gtk_file_filter_add_pattern (x_filter, "*.DAT");
		gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (chooser), x_filter);
                g_object_unref (x_filter);

                GtkFileFilter *all_filter = gtk_file_filter_new ();
		gtk_file_filter_set_name (all_filter, "All");
		gtk_file_filter_add_pattern (all_filter, "*");
		gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (chooser), all_filter);
                g_object_unref (all_filter);
                
                gtk_widget_show (chooser);
                g_signal_connect (chooser, "response",
                                  G_CALLBACK (Surface::load_exec),
                                  this);
                return 1;
	}else{
		if(!strrchr(rname, '/'))
			fname = g_strconcat("./",rname,NULL);
		else
			fname = g_strdup(rname);
	}
	
	// check if type may be .asc (profile)
	if (strstr(fname, ".asc") || strstr(fname, ".vpdata")){
		ret = AddProfile (fname);
		g_free (fname);
		return ret;
	}

	// check if something else (no NetCDF, HDF, DAT, etc. via scan import plugins)
	if (!(  strstr(fname, ".nc") || strstr(fname, ".NC"))){
		ret = gapp->xsm->gnuimport (fname);
		if (ret){
			g_free(fname);
			return ret; // load failed
		}
	} else {
	
		// check for valid name extension length
		if(strlen(fname) <= 3){
			gapp->SetStatus(N_("invalid filename, load aborted."));
			g_free (fname);
			return 1;
		}

		// save path for next time...
		g_free(path);
		path = g_strndup(fname, strrchr(fname, '/')-fname);
	
		// auto activate Channel or use active one
		if(!ActiveScan){
                        open_mode = xsm::open_mode::replace; // always replace if new
                        
			if((Ch=FindChan(ID_CH_M_OFF)) < 0){
				gapp->SetStatus (ERR_SORRY, ERR_NOFREECHAN);
				XSM_SHOW_ALERT (ERR_SORRY, ERR_NOFREECHAN, fname, 1);
				g_free (fname);
				return 1;
			}
			if(ActivateChannel(Ch)){
				gapp->SetStatus(N_("Load: Error activating channel"));
				g_free (fname);
				return 1;
			}
		}
		ActiveScan->CpyDataSet (data);

                // zipped ? unzip it to /tmp/... !
		// depends on the avaibility of gzip and bzip2 for *.gz and *.bz2
		// files respectively
		if(!strncasecmp(fname+strlen(fname)-3,".gz",3) || 
		   !strncasecmp(fname+strlen(fname)-4,".bz2",4)){
			FILE *cmd;
			int fd;
			gchar *cmdstr;
			gapp->SetStatus(N_("Load: unpacking..."), strrchr(fname,'/'));
#   define GXSMTMPDIRPREFIX "/tmp/GxsmUnzipdir"
			char *gxsmtmpdir = g_strdup(GXSMTMPDIRPREFIX"_XXXXXX");
		
			if (!(fd=mkstemp(gxsmtmpdir))){
				g_warning (N_("Could not generate temporary filename: %s\n"),
					   g_strerror(errno));
				XSM_DEBUG(DBG_L2, "ErrorTmpFn: " <<  g_strerror(errno) );
			}
			if(fd) close(fd);
			cmd = popen("rm -rf " GXSMTMPDIRPREFIX "*", "w");
			if(cmd)
				pclose(cmd);
		
		
			if ((mkdir(gxsmtmpdir, 0755) != 0)){
				g_warning (N_("Could not generate temporary directory: %s\n"),
					   g_strerror(errno));
				XSM_DEBUG(DBG_L2, "Error: " <<  g_strerror(errno) );
			}
		
			XSM_DEBUG(DBG_L2, "MkTmpdir: " << gxsmtmpdir );
			XSM_DEBUG(DBG_L2, "FileName:" << fname );
		
			cmdstr = g_strconcat("cp ", fname, " ", gxsmtmpdir, 
					     strrchr(fname, '/'), NULL);
			cmd = popen(cmdstr, "w");
			if(cmd)
				pclose(cmd);
			else{
				gapp->SetStatus(N_("error executing"), cmdstr);
				XSM_SHOW_ALERT(ERR_SORRY, ERR_CPTMP, fname,1);
				g_free(fname);
				g_free(gxsmtmpdir);
				g_free(cmdstr);
				return 0;
			}
			g_free(cmdstr);
		
			cname = g_strconcat(gxsmtmpdir, strrchr(fname,'/'), NULL);
			if (!strncasecmp(fname+strlen(fname)-3,".gz",3))
				cmdstr = g_strconcat("gunzip ", cname, NULL);
			else
				cmdstr = g_strconcat("bunzip2 ", cname, NULL);
			g_free(fname);
			fname = g_strndup(cname, strrchr(cname,'.')-cname);
		
			cmd = popen(cmdstr, "w");
			gapp->SetStatus("sh", cmdstr);
			g_free(cmdstr);
			// gxsmtmpdir done.
			g_free(gxsmtmpdir);
			if(cmd)
				pclose(cmd);
			else{
				gapp->SetStatus(N_("error while unzipping"));
				XSM_SHOW_ALERT(ERR_SORRY, ERR_UNZIP, fname,1);
				g_free(fname);
				return 0;
			}
		}
		
		if (!strncasecmp (fname+strlen(fname)-3,".nc",3)){
                        gchar **tmp = g_strsplit_set (fname, "/", -1);
                        gapp->channelselector->SetInfo (ActiveChannel, tmp[g_strv_length (tmp)-1]);
                        g_strfreev (tmp);

                        switch (open_mode){
                        case xsm::open_mode::replace:
                                {
                                        Dataio *Dio = new NetCDF (ActiveScan, fname); // NetCDF File
                                        Dio->Read (open_mode);
                                        if(Dio->ioStatus ())
                                                XSM_SHOW_ALERT (ERR_SORRY, Dio->ioStatus (), fname,1);
                                        delete Dio;
                                        ActiveScan->GetDataSet(data);
                                        ActiveScan->mem2d->data->update_ranges (0);
                                }
                                break;
                        case xsm::open_mode::append_time:
                                {
                                        Dataio *Dio = new NetCDF (ActiveScan, fname); // NetCDF File
                                        Dio->Read (open_mode);
                                        if(Dio->ioStatus ())
                                                XSM_SHOW_ALERT (ERR_SORRY, Dio->ioStatus (), fname,1);
			
                                        delete Dio;

                                        ActiveScan->GetDataSet(data);

                                        if (!ActiveScan->TimeList) // reference to the first frame/image loaded
                                                t0 = (double)ActiveScan->data.s.tStart;

                                        t = (double)ActiveScan->data.s.tStart;
                                        t -= t0;
                                        gchar *fn = g_strrstr (fname, "/");
                                        ActiveScan->mem2d->add_layer_information (new LayerInformation ("name", 0., fn ? (fn+1):fname));
                                        ActiveScan->mem2d->add_layer_information (new LayerInformation ("t",t, "%.2f s"));
                                        ActiveScan->mem2d->data->update_ranges (0);
                                        gapp->xsm->data.s.ntimes = ActiveScan->append_current_to_time_elements (-1, t);
                                }
                                break;
                        case xsm::open_mode::stitch_2d:
                                {
                                        int current_channel = ActiveChannel;
                                        int ch_tmp=FindChan(ID_CH_M_X);
                                        if (ch_tmp < 0){
                                                ch_tmp=FindChan(ID_CH_M_OFF);
                                                if (ch_tmp < 0){
                                                        gapp->SetStatus(N_("no tmp channel available"));
                                                        return 0;
                                                }
                                        }
                                        ActivateChannel (ch_tmp);
                                        // NetCDF File, load in temp "X" channel for auto stitching into ActiveScan
                                        Dataio *Dio = new NetCDF (scan[ch_tmp], fname);
                                        Dio->Read (open_mode);
                                        if(Dio->ioStatus ())
                                                XSM_SHOW_ALERT (ERR_SORRY, Dio->ioStatus (), fname,1);
                                        
                                        delete Dio;
                                        SetMode (ch_tmp, ID_CH_M_X);
                                        SetMode (current_channel, ID_CH_M_ACTIVE);

                                        StitchScans (scan[ch_tmp], scan[current_channel]);
                                                
                                        ActiveScan->GetDataSet(data);
                                }
                                break;
                        }
		}
		ret = 0;
	}

	// fname done.
	g_free(fname);
	
	// refresh all Parameter and draw Scan
	gapp->spm_update_all();

	if (xsmres.LoadDelay>0){
		clock_t t=clock()+xsmres.LoadDelay*CLOCKS_PER_SEC/10;
		ActiveScan->auto_display ();
		do {
			gapp->check_events_self();
		} while(t < clock());
	} else {
                ActiveScan->draw();
        }
	return ret; 
}


void Surface::save_new_path_exec (GtkDialog *dialog,  int response, gpointer user_data){
        Surface *s = (Surface *) user_data;
        if (response == GTK_RESPONSE_ACCEPT){
                GtkFileChooser *chooser = GTK_FILE_CHOOSER (dialog);
                g_autoptr(GFile) file = gtk_file_chooser_get_file (chooser);
                // filename = gtk_file_chooser_get_filename (chooser);
                g_free (s->default_path);
                s->default_path=g_file_get_parse_name (file);
                g_settings_set_string (gapp->get_as_settings (), "auto-save-folder", s->default_path);
                g_settings_set_string (gapp->get_as_settings (), "auto-save-folder-netcdf", s->default_path);
                g_settings_set_string (gapp->get_as_settings (), "auto-save-folder-probe", s->default_path);
        }
        gtk_window_destroy (GTK_WINDOW (dialog));
}

void Surface::save_exec (GtkDialog *dialog,  int response, gpointer user_data){
        Surface *s = (Surface *) user_data;
        if (response == GTK_RESPONSE_ACCEPT){
                GtkFileChooser *chooser = GTK_FILE_CHOOSER (dialog);
                g_autoptr(GFile) file = gtk_file_chooser_get_file (chooser);
                // filename = gtk_file_chooser_get_filename (chooser);
                gchar *tmp=g_file_get_parse_name (file);
		s->save (MANUAL_SAVE_AS, tmp, -99);
                g_free (tmp);
        }
        gtk_window_destroy (GTK_WINDOW (dialog));
}


// Save Scan(s)
// ========================================
// automode:
//         typedef enum AUTO_SAVE_MODE { MANUAL_SAVE_AS, AUTO_NAME_SAVE, AUTO_NAME_PARTIAL_SAVE, CHANGE_PATH_ONLY };
//         MANUAL_SAVE_AS  use Filedlg
//         2      autogenerate filename and attach <autosave> to it
//         At the end of a scan or a moviescan "2" is NOT used,
//         the autosave extension is used only for backup-intermediate scans
//         to easily distinguish between final saves and intermediate results.
// rname:  NULL: auto or ask (default)
//         !=0 : use it
// chidx: -1: check all channels (default)
//        >=0: save Channel chidx
// forceOverwrite:  TRUE  = force overwrite
//                  FALSE = ask before overwrite (default)
int Surface::save(AUTO_SAVE_MODE automode, char *rname, int chidx, int forceOverwrite){
        static int chidx_save_as = 0;
	gchar *fname = NULL;
	gchar *ffname = NULL;

        if (chidx == -99)
                chidx = chidx_save_as;
        
        // ADJUST PATH ONLY
	if(!default_path || automode == CHANGE_PATH_ONLY){

                g_free (default_path);
                default_path = g_strdup (g_settings_get_string (gapp->get_as_settings (), "auto-save-folder"));
                if (automode == CHANGE_PATH_ONLY){
			GtkWidget *chooser = gtk_file_chooser_dialog_new ("Select path for data files",
                                                                          NULL, 
									  GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
									  _("_Cancel"), GTK_RESPONSE_CANCEL,
									  _("_Select"), GTK_RESPONSE_ACCEPT,
									  NULL);

                        GFile *gf=g_file_new_for_path (default_path);
                        gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (chooser), gf, NULL);
                        //g_free (gf); // when, how FIX-ME GTK4  ???
                        if (automode != CHANGE_PATH_ONLY){
                                GtkFileFilter *all_filter = gtk_file_filter_new ();
                                gtk_file_filter_set_name (all_filter, "All");
                                gtk_file_filter_add_pattern (all_filter, "*");
                                gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (chooser), all_filter);

                                GtkFileFilter *nc_filter = gtk_file_filter_new ();
                                gtk_file_filter_set_name (nc_filter, "NetCDF");
                                gtk_file_filter_add_pattern (nc_filter, "*.nc");
                                gtk_file_filter_add_pattern (nc_filter, "*.cdf");
                                gtk_file_filter_add_pattern (nc_filter, "*.netcdf");
                                gtk_file_filter_add_pattern (nc_filter, "*.NC");
                                gtk_file_filter_add_pattern (nc_filter, "*.hdf");
                                gtk_file_filter_add_pattern (nc_filter, "*.HDF");
                                gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (chooser), nc_filter);

                                GtkFileFilter *vp_filter = gtk_file_filter_new ();
                                gtk_file_filter_set_name (vp_filter, "VPdata");
                                gtk_file_filter_add_pattern (vp_filter, "*.asc");
                                gtk_file_filter_add_pattern (vp_filter, "*.vpdata");
                                gtk_file_filter_add_pattern (vp_filter, "*.VPdata");
                                gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (chooser), vp_filter);

                                GtkFileFilter *x_filter = gtk_file_filter_new ();
                                gtk_file_filter_set_name (x_filter, "DAT");
                                gtk_file_filter_add_pattern (x_filter, "*.dat");
                                gtk_file_filter_add_pattern (x_filter, "*.DAT");
                                gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (chooser), x_filter);
                        }
                        gtk_widget_show (chooser);
                        g_signal_connect (chooser, "response",
                                          G_CALLBACK (Surface::save_new_path_exec),
                                          this);
			return 0;
		}
	}

	if(chidx < 0)
                return -1; // only single specific scan channels -- auto save all is handled by menu-callback(s) directly via scan class Scan->Save(..) now!

        // FILE SAVE DIALOG
        int overwrite;
        do{ // while some error...
                const gchar *defaultname = scan[chidx]->storage_manager.get_name();

                if (rname) {
                        ffname = rname;
                } else {
                        GtkWidget *chooser = gtk_file_chooser_dialog_new ("NC file to save", 
                                                                          NULL,
                                                                          GTK_FILE_CHOOSER_ACTION_SAVE,
                                                                          _("_Cancel"), GTK_RESPONSE_CANCEL,
                                                                          _("_Save"), GTK_RESPONSE_ACCEPT,
                                                                          NULL);
                        GFile *gf=g_file_new_for_path (default_path);
                        gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (chooser), gf, NULL);
                        // g_free (gf); // FIX-ME GTK4
                        gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER (chooser), defaultname);

                        GtkFileFilter *nc_filter = gtk_file_filter_new ();
                        gtk_file_filter_set_name (nc_filter, "NetCDF");
                        gtk_file_filter_add_pattern (nc_filter, "*.nc");
                        gtk_file_filter_add_pattern (nc_filter, "*.cdf");
                        gtk_file_filter_add_pattern (nc_filter, "*.netcdf");
                        gtk_file_filter_add_pattern (nc_filter, "*.NC");
                        gtk_file_filter_add_pattern (nc_filter, "*.hdf");
                        gtk_file_filter_add_pattern (nc_filter, "*.HDF");
                        gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (chooser), nc_filter);

                        GtkFileFilter *all_filter = gtk_file_filter_new ();
                        gtk_file_filter_set_name (all_filter, "All");
                        gtk_file_filter_add_pattern (all_filter, "*");
                        gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (chooser), all_filter);

                        ffname = NULL;
                        chidx_save_as = chidx;
                        gtk_widget_show (chooser);
                        g_signal_connect (chooser, "response",
                                          G_CALLBACK (Surface::save_exec),
                                          this);
                        return 1;
                        /*
                        if (gtk_dialog_run (GTK_DIALOG (chooser)) == GTK_RESPONSE_ACCEPT){
                                ffname = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (chooser));
                        }			
                        gtk_widget_destroy (chooser);

                        if(! ffname){ // cancel ??
                                gapp->SetStatus(N_("Save aborted by user."));
                                return 1;
                        }
                        */
                }
                
                fname = ffname;
                g_free(default_path);
                default_path  = g_strndup(fname, strrchr(fname, '/')-fname);

                g_settings_set_string (gapp->get_as_settings (), "auto-save-folder", default_path);
                
                XSM_DEBUG (DBG_L2, "Save: Name is:<" << fname << ">");
                XSM_DEBUG (DBG_L2, "Save: Path is:<" << default_path << ">");
                // if no extension given, use default type from resources
                if (strncasecmp(fname+strlen(fname)-3,".nc",3)){
                        gchar *fnamebase = g_strdup(fname);
                        g_free(fname);
                        fname = g_strconcat(fnamebase, ".nc", NULL);
                        // fnamebase done.
                        g_free(fnamebase);
                }

                overwrite = GTK_RESPONSE_YES;
                // check if file exists -- if so, ask user for action
                if (g_file_test (fname, G_FILE_TEST_EXISTS)){
                        // Check overwrite on save?
                        if(forceOverwrite == FALSE){
                                GtkWidget *dialog = gtk_message_dialog_new (NULL,
                                                                            GTK_DIALOG_DESTROY_WITH_PARENT,
                                                                            GTK_MESSAGE_WARNING,
                                                                            GTK_BUTTONS_YES_NO,
                                                                            N_("File '%s' exists, overwrite?"),
                                                                            fname);
                                // FIX-ME-GTK4
                                overwrite = GTK_RESPONSE_YES;
                                g_signal_connect_swapped (G_OBJECT (dialog), "response",
                                                          G_CALLBACK (gtk_window_destroy),
                                                          G_OBJECT (dialog));
                                gtk_widget_show (dialog);
                                //overwrite = gtk_dialog_run (GTK_DIALOG (dialog));
                                //gtk_widget_destroy (dialog);
                                if (overwrite != GTK_RESPONSE_YES){
                                        gapp->SetStatus(N_("File exists, save aborted by user."));
                                        return 1;
                                }
                        }else{
                                XSM_DEBUG (DBG_L2, "forceOverwrite enabled: overwriting file!");
                                gapp->SetStatus(N_("File overwrite forced by default."));
                        }
                }
        }while (overwrite != GTK_RESPONSE_YES);
    
        gapp->monitorcontrol->LogEvent("*Save", fname);

        // full NetCDF save: Parameter and Data
        {
                Dataio *Dio = NULL;
                scan[chidx]->CpyUserEntries(data);
      
                if (!strncasecmp(fname+strlen(fname)-3,".nc",3))
                        Dio = new NetCDF(scan[chidx], fname);

                if (Dio){
                        Dio->Write();
                        if(Dio->ioStatus()){
                                gapp->SetStatus(N_("Error"), Dio->ioStatus());
                                XSM_SHOW_ALERT(ERR_SORRY, Dio->ioStatus(), fname,1);
                        }
                        else{
                                scan[chidx]->Saved();
                                gapp->SetStatus(N_("Saving done "), fname);
                        }

                        delete Dio;
                } 
                else
                        gapp->SetStatus(N_("Error, not saved "), fname);
        }
        // fname done.
        g_free(fname);

	gapp->spm_update_all();

	return 0; 
}

// append current scan to time dimension, apply to all selected scans, t<0 measn a free up of all timed data
int Surface::auto_append_in_time (double t){
	int n=1;
	int si,i;
	int Ch[CHMAX];

	for(si=0; si<CHMAX; si++)
		Ch[si] = -1;

	si=0;

//	std::cout << "auto_append_in_time t=" << t << std::endl;

	// Active scan
//	Ch[si++] = FindChan(ID_CH_M_ACTIVE);
	
	// Auto check for all dataaq scans to save
	for(i=0; i<PIDCHMAX; ++i){
		Ch[si++] = FindChan(xsmres.pidchno[i], ID_CH_D_P);
		Ch[si++] = FindChan(xsmres.pidchno[i], ID_CH_D_M);
	}
	
	for(i=0; i<DAQCHMAX; ++i){
		Ch[si++] = FindChan(xsmres.daqchno[i], ID_CH_D_P);
		Ch[si++] = FindChan(xsmres.daqchno[i], ID_CH_D_M);
	}
	
//	for(i=1; i<si; i++) // remove duplicates if any
//		for(int j=i; j<si; j++)
//			if(Ch[ij-1] == Ch[si]) 
//				Ch[si] = -1;

	for(si=0; si<CHMAX; si++){
		if(Ch[si] == -1) continue;
//		std::cout << "auto_append_in_time Ch[" << si << "]" << std::endl;
		if(t < 0.){
			scan[Ch[si]]->free_time_elements ();
		} else {
			scan[Ch[si]]->mem2d->add_layer_information (new LayerInformation ("t",t, "%.2f s"));
			n = scan[Ch[si]]->append_current_to_time_elements (-1, t);
//			std::cout << "auto_append_in_time OK, n=" << n << std::endl;
		}
	}
	return n;
}

int Surface::gnuimport(const char *rname){
	// auto activate Channel or use active one
	if(! GetActiveScan())
		return 1;
	
	XSM_DEBUG(DBG_L2, "Checking file with Import Plugins!" );
	gchar *ret;
	gchar *nametocheck = g_strdup(rname);
	ret = nametocheck;
	gapp->SignalLoadFileEventToPlugins(&ret);
	XSM_DEBUG(DBG_L2, "Result: " << (ret==NULL? "MATCHING IMPORT PLUGIN FOUND":"unknown file type") );
	std::cout << "GNU/AUTO IMPORT result: " << (ret==NULL? "MATCHING IMPORT PLUGIN FOUND for >":"unknown file type >") << nametocheck << "<" << std::endl;
	g_free (nametocheck);

	return ret==NULL?0:1;
}

int Surface::gnuexport(const char *rname){

	if(!ActiveScan){
		XSM_SHOW_ALERT(ERR_SORRY, ERR_NOACTIVESCAN,HINT_ACTIVATESCAN,1);
		return 1;
	}
	if(!ActiveScan->mem2d){
		XSM_SHOW_ALERT(ERR_SORRY, ERR_NOACTIVESCAN,HINT_ACTIVATESCAN,1);
		return 1;
	}
	
	XSM_DEBUG(DBG_L2, "Checking file with Export Plugins!" );
	gchar *ret;
	gchar *nametocheck = g_strdup(rname);
	ret = nametocheck;
	gapp->SignalSaveFileEventToPlugins(&ret);
	XSM_DEBUG(DBG_L2, "Result: " << (ret==NULL? "MATCHING EXPORT PLUGIN FOUND":"unknown file type") );
	g_free (nametocheck);

	return 0;
}

Scan* Surface::GetActiveScan(){
	return ActiveScan;
}


void Surface::GetFromMem2d(Mem2d *m){
	if (!ActiveScan || !m) return;
        ActiveScan->data.s.ntimes=1;
        ActiveScan->data.s.nvalues=1;
        ActiveScan->data.s.nx=m->GetNx ();
        ActiveScan->data.s.ny=m->GetNy ();
        ActiveScan->data.s.rx=1.;
        ActiveScan->data.s.ry=1.;
        ActiveScan->data.s.rz=1.;
        ActiveScan->data.s.dx=1.;
        ActiveScan->data.s.dy=1.;
        ActiveScan->data.s.dz=1.;
        ActiveScan->data.s.dl=1.;
        ActiveScan->data.s.x0=0.;
        ActiveScan->data.s.y0=0.;
        ActiveScan->data.s.alpha=0.;
	
	ActiveScan->data.s.SPA_OrgX=0.;
        ActiveScan->data.s.SPA_OrgY=0.;
        ActiveScan->data.s.Energy=0.;
        ActiveScan->data.s.Bias=0.;
        ActiveScan->data.s.GateTime=0.;
        ActiveScan->data.s.Current=0.;
        ActiveScan->data.s.SetPoint=0.;

        ActiveScan->data.s.tStart=(time_t)0;
        ActiveScan->data.s.tEnd=(time_t)0;


	ActiveScan->data.copy (data);
	//ActiveScan->data.GetDisplay_Param (data);
	ActiveScan->data.GetUser_Info (data);
	ActiveScan->data.GetScan_Param (data);
	ActiveScan->data.CpUnits (data);

	// override with for sure propper nx,ny
        ActiveScan->data.s.nx=m->GetNx ();
        ActiveScan->data.s.ny=m->GetNy ();

	ActiveScan->mem2d->copy (m);

        ActiveScan->data.s.rx = m->data->GetXLookup (m->GetNx ()-1);
	ActiveScan->data.s.ry = m->data->GetYLookup (m->GetNx ()-1);

	ActiveScan->draw ();
}

Scan* Surface::GetMasterScan(){
	return MasterScan;
}

void Surface::SetMasterScan(Scan *ms){
	MasterScan = ms;
}



// Directory File Selection Stuff
// //  char select_mask[256];
// //  int gxsm_select(const struct dirent *item){ return !fnmatch(select_mask, item->d_name, 0); }
// //  
// //MkIconsData::MkIconsData(gchar *InPath, gchar *OutPath, gchar *InMask, 
// //			 gchar *Opt, gchar *IconName){
// //  
// //  	pathname   = InPath   ? InPath   : g_strdup(".") ;
// //  	outputname = OutPath  ? OutPath  : g_strdup("/tmp/icons.ps") ;
// //  	name       = IconName ? IconName : g_strdup("icons.ps") ;
// //  	mask       = InMask   ? InMask   : g_strdup("*.nc") ;
// //  	options    = Opt      ? Opt      : g_strdup("--------") ;
// //    
// //  	redres = 0;
// //  	nix = 4;
// //}
// //  
//MkIconsData::~MkIconsData(){
// //  	g_free(pathname);
// //  	g_free(outputname);
// //  	g_free(name);
// //  	g_free(mask);
// //  	g_free(options);
//}
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
// // void Surface::MkIcons(MkIconsData *mid){
// //  	struct dirent **namelist;
// //  	int n;
// //  	double hi0,lo0;
// //  	lo0=data.display.cpslow;
// //  	hi0=data.display.cpshigh;
// //  
// //  	// auto activate Channel or use active one
// //  	if(!ActiveScan){
// //  		int Ch;
// //  		if((Ch=FindChan(ID_CH_M_OFF)) < 0){
// //  			XSM_SHOW_ALERT(ERR_SORRY, ERR_NOFREECHAN,"for Mk Icons",1);
// //  			return;
// //  		}
// //  		if(ActivateChannel(Ch))
// //  			return;
// //  		ActiveScan->create();
// //  	}
// //  	ActiveScan->CpyDataSet(data);
// //  
// //  	if(mid->nix && ActiveScan){
// //  		int redres;
// //  		const double dpifac=72.*6./300.; // 72.dpi grey bei 300dpi Aufloeung und 6in nutzbarer Breite
// //  		// calculate needed resolution
// //  		switch(mid->options[MkIconOpt_Resolution]){
// //  		case '3': redres = (int)(300.*dpifac); break;
// //  		case '6': redres = (int)(600.*dpifac); break;
// //  		case 'C': redres = (int)(1200.*dpifac); break;
// //  		default:  redres = (int)(600.*dpifac); break;
// //  		}
// //  		redres /= mid->nix;
// //  		XSM_DEBUG (DBG_L2,  "N Icons in X: " << mid->nix << ", Reduce to Pixels less than " << redres );
// //  
// //  		// setup selection mask for "gxsm_select()"
// //  		strcpy(select_mask, mid->mask);
// //  		n = scandir(mid->pathname, &namelist, gxsm_select, alphasort);
// //  		if (n < 0)
// //  			perror("scandir");
// //  		else{
// //  			Scan *Original=NULL, *Icon, *TmpSc, *HlpSc;
// //  			char fname[256];
// //  
// //  			EpsfTools *epsf;
// //  			if(IS_SPALEED_CTRL)
// //  				epsf = new SPA_epsftools;
// //  			else
// //  				epsf = new SPM_epsftools;
// //  
// //  			epsf->SetPaperTyp (mid->options[MkIconOpt_Paper] == 'A' ? A4PAPER:LETTERPAPER);
// //  			epsf->open(mid->outputname, TRUE);
// //  			epsf->NIcons(mid->nix);
// //  			while(n--){
// //  				XSM_DEBUG(DBG_L2, "Loading " << namelist[n]->d_name );
// //  	
// //  				// Load
// //  				sprintf(fname,"%s/%s",mid->pathname,namelist[n]->d_name);
// //  				XSM_DEBUG (DBG_L2, "Icon: " << namelist[n]->d_name);
// //  				load(fname);
// //  	
// //  				Original = ActiveScan;
// //  				G_FREE_STRDUP_PRINTF(Original->data.ui.name, namelist[n]->d_name); // ohne Pfad !!
// //  	
// //  				Icon = NewScan(0, data.display.ViewFlg, 0, &data);
// //  				Icon->create();
// //  				TmpSc = NewScan(0, data.display.ViewFlg, 0, &data);
// //  				TmpSc->create();
// //  
// //  				// Copy all data
// //  				Icon->CpyDataSet(Original->data);
// //  				TmpSc->CpyDataSet(Original->data);
// //  
// //  				// scale down to lower than redres
// //  				// [0] "Resolution"
// //  				// ========================================
// //  				if(CopyScan(Original, Icon))
// //  					XSM_SHOW_ALERT(ERR_MATH, "", "", 1);
// //  
// //  				XSM_DEBUG (DBG_L2,  "W00:" << "nx:" << Icon->data.s.nx << " GetNx:" << Icon->mem2d->GetNx() );
// //  
// //  				while(Icon->data.s.nx > redres && Icon->data.s.ny > redres){
// //  					// Excange Icon <=> TmpSc
// //  					HlpSc = Icon; Icon = TmpSc; TmpSc = HlpSc;
// //  					// TmpSc => Quench2 => Icon
// //  					if(TR_QuenchScan(TmpSc, Icon))
// //  						XSM_SHOW_ALERT(ERR_MATH, "", "", 1);
// //  					XSM_DEBUG (DBG_L2,  "WOK:" << "nx:" << Icon->data.s.nx << " GetNx:" << Icon->mem2d->GetNx() );
// //  					memcpy( &TmpSc->data, &Icon->data, sizeof(SCAN_DATA));
// //  				}
// //  				if(CopyScan(Icon, TmpSc))
// //  					XSM_SHOW_ALERT(ERR_MATH, "", "", 1);
// //  
// //  				XSM_DEBUG (DBG_L2,  "Icon :" << "nx:" << Icon->data.s.nx << " GetNx:" << Icon->mem2d->GetNx() );
// //  				XSM_DEBUG (DBG_L2,  "TmpSc:" << "nx:" << TmpSc->data.s.nx << " GetNx:" << TmpSc->mem2d->GetNx() );
// //  	
// //  				// do some more math...
// //  
// //  				// [1] "E Regression"
// //  				// ========================================
// //  				if(mid->options[MkIconOpt_EReg] != '-'){
// //  					double eps=0.1;
// //  					XSM_DEBUG (DBG_L2,  "doing E regress ..." );
// //  					// Excange Icon <=> TmpSc
// //  					HlpSc = Icon; Icon = TmpSc; TmpSc = HlpSc; 
// //  					// TmpSc => Quench2 => Icon
// //  					if(mid->options[MkIconOpt_EReg] == 'e') eps=0.05; 
// //  					if(mid->options[MkIconOpt_EReg] == 'E') eps=0.3; 
// //  					TmpSc->Pkt2d[0].x = (int)(eps*TmpSc->mem2d->GetNx());
// //  					TmpSc->Pkt2d[0].y = (int)(eps*TmpSc->mem2d->GetNy());
// //  					TmpSc->Pkt2d[1].x = (int)((1.-eps)*TmpSc->mem2d->GetNx());
// //  					TmpSc->Pkt2d[1].y = (int)((1.-eps)*TmpSc->mem2d->GetNy());
// //  					if(BgERegress(TmpSc, Icon))
// //  						XSM_SHOW_ALERT(ERR_MATH, "", "", 1);
// //  					XSM_DEBUG (DBG_L2,  "Ereg - Icon :" << "nx:" << Icon->data.s.nx << " GetNx:" << Icon->mem2d->GetNx() );
// //  					XSM_DEBUG (DBG_L2,  "Ereg - TmpSc:" << "nx:" << TmpSc->data.s.nx << " GetNx:" << TmpSc->mem2d->GetNx() );
// //  				}
// //  				// [2] "Line Regression"
// //  				// ========================================
// //  				if(mid->options[MkIconOpt_LReg] == 'l'){
// //  					XSM_DEBUG (DBG_L2,  "doing BgLin1D ..." );
// //  					// Excange Icon <=> TmpSc
// //  					HlpSc = Icon; Icon = TmpSc; TmpSc = HlpSc; 
// //  					// TmpSc => Quench2 => Icon
// //  					if(BgLin1DScan(TmpSc, Icon))
// //  						XSM_SHOW_ALERT(ERR_MATH, "", "", 1);
// //  					XSM_DEBUG (DBG_L2,  "BgLin1D - Icon :" << "nx:" << Icon->data.s.nx << " GetNx:" << Icon->mem2d->GetNx() );
// //  					XSM_DEBUG (DBG_L2,  "BgLin1D - TmpSc:" << "nx:" << TmpSc->data.s.nx << " GetNx:" << TmpSc->mem2d->GetNx() );
// //  				}
// //  				// [3] "View Mode" override: -qdlph
// //  				// ========================================
// //  				switch(mid->options[MkIconOpt_ViewMode]){
// //  				case 'q': Icon->mem2d->SetDataPktMode (SCAN_V_QUICK); break;
// //  				case 'd': Icon->mem2d->SetDataPktMode (SCAN_V_DIRECT); break;
// //  				case 'l': Icon->mem2d->SetDataPktMode (SCAN_V_LOG); break;
// //  				case 'h': Icon->mem2d->SetDataPktMode (SCAN_V_HORIZONTAL); break;
// //  				case 'p': Icon->mem2d->SetDataPktMode (SCAN_V_PERIODIC); break;
// //  				}
// //  				// [4] "Auto Scaling" -123
// //  				// ========================================
// //  				if(mid->options[MkIconOpt_AutoSkl] != '-'){
// //  					double eps=0.1;
// //  					XSM_DEBUG (DBG_L2,  "doing Skl ..." );
// //  					// Excange Icon <=> TmpSc
// //  					HlpSc = ActiveScan; ActiveScan = Icon; 
// //  					if(mid->options[MkIconOpt_AutoSkl] == '1') eps=0.05; 
// //  					if(mid->options[MkIconOpt_AutoSkl] == '2') eps=0.20; 
// //  					if(mid->options[MkIconOpt_AutoSkl] == '3') eps=0.30; 
// //  					Icon->Pkt2d[0].x = (int)(eps*(double)Icon->mem2d->GetNx());
// //  					Icon->Pkt2d[0].y = (int)(eps*(double)Icon->mem2d->GetNy());
// //  					Icon->Pkt2d[1].x = (int)((1.-eps)*(double)Icon->mem2d->GetNx());
// //  					Icon->Pkt2d[1].y = (int)((1.-eps)*(double)Icon->mem2d->GetNy());
// //  					Icon->PktVal=2;
// //  					Icon->mem2d->AutoHistogrammEvalMode (&Icon->Pkt2d[0], &Icon->Pkt2d[1]);
// //  					// AutoDisplay();
// //  					Icon->PktVal=0;
// //  					ActiveScan = HlpSc;
// //  					XSM_DEBUG (DBG_L2,  "AutoSkl - Icon :" << "nx:" << Icon->data.s.nx << " GetNx:" << Icon->mem2d->GetNx() );
// //  					Original->data.display.contrast = data.display.contrast;
// //  					Original->data.display.bright   = data.display.bright;
// //  					XSM_DEBUG (DBG_L2, "Contrast=" << Original->data.display.contrast);
// //  				}
// //  				if(mid->options[MkIconOpt_Scaling] == 'c'){
// //  					XSM_DEBUG (DBG_L2,  "doing manual Cps hi=" << hi0 << " ... lo=" << lo0 << " Gate=" << data.display.cnttime);
// //  					// Excange Icon <=> TmpSc
// //  					HlpSc = ActiveScan; ActiveScan = Icon; 
// //  					AutoDisplay(hi0*data.display.cnttime, lo0*data.display.cnttime);
// //  					ActiveScan = HlpSc;
// //  	  
// //  				}
// //  
// //  				// Put Icon
// //  				epsf->init();
// //  	
// //  				//	    epsf->SetAbbWidth(figwidth);
// //  				//	    epsf->SetFontSize(fontsize);
// //  	
// //  				epsf->placeimage();
// //  	
// //  				epsf->putsize(Original);
// //  				epsf->putframe();
// //  				epsf->putticks(Original);
// //  				//	  epsf->putbar(ActiveScan);
// //  				//	  epsf->putmore(ActiveScan, title);
// //  				//	  epsf->putgrey(Original, Icon->mem2d, options[MkIconOpt_ELReg] == 'a', FALSE);
// //  				XSM_DEBUG (DBG_L2,  "-Original:" << "nx:" << Original->data.s.nx << " GetNx:" << Original->mem2d->GetNx() );
// //  				XSM_DEBUG (DBG_L2,  "-Icon    :" << "nx:" << Icon->data.s.nx << " GetNx:" << Icon->mem2d->GetNx() );
// //  				// [5] "Scaling": -a
// //  
// //  				epsf->putgrey(Original, Icon->mem2d, mid->options[MkIconOpt_Scaling] == 'a', FALSE);
// //  				epsf->endimage();
// //  				epsf->FootLine(Original);
// //  	
// //  				//	delete Icon;
// //  				delete TmpSc;
// //  			}
// //  			epsf->FootLine(Original, TRUE);
// //  			epsf->close();
// //  			delete epsf;
// //  		}
// //  	}
// //}

// generic MathOp callback preparation and Math-Run function call
// -- one Src and no Dest Scan (some data/stats/graph, etc. output only)
// -- run get's called for current layer & time, run-function can operate on layers/times itself
void Surface::MathOperationNoDest(gboolean (*MOp)(MATHOPPARAMSNODEST)){
	// Src, Quelle: Aktive Channel
	// Punkte: Diagonale Ecken von Rechteck
	int Merr;
	if(ActiveScan){ // ist ein Scan Active ? (als Quelle noeig)
		// call of MathOp( SourceMem2d )
		if( (Merr=(*MOp)(ActiveScan)) )
			XSM_SHOW_ALERT(ERR_MATH, MathErrString[Merr], "", 1);
	}
	else{
		XSM_SHOW_ALERT(ERR_SORRY, ERR_NOACTIVESCAN,HINT_ACTIVATESCANMATH,1);
	}
}

// generic MathOp callback preparation and Math-Run function call
// -- one Src and one Dest Scan
// -- run get's called for current layer & time, run-function can operate on layers/times itself
void Surface::MathOperation(gboolean (*MOp)(MATHOPPARAMS)){
	// Src, Quelle: Aktive Channel
	// Dest, Ziel:  Math Channel, wird ggf, angelegt oder berschrieben !
	// Punkte: Diagonale Ecken von Rechteck
	int Merr;
	int ChDest;
        int start=0;
	if(ActiveScan){ // ist ein Scan Active ? (als Quelle noeig)
                do{
                        if((ChDest=FindChan(ID_CH_M_MATH, -1, start)) < 0){ // ist bereits ein Math Scan vorhanden ? (benutzen !)
                                if((ChDest=FindChan(ID_CH_M_OFF)) < 0){ // keiner Math Scan, dann neuen anlegen, noch moelich ?
                                        // alle belegt !!!
                                        XSM_SHOW_ALERT(ERR_SORRY, ERR_NOFREECHAN,"",1);
                                        return;
                                }
                        }
                        if (scan[ChDest] == ActiveScan){
                                start = ChDest+1;
                                ChDest = -1;
                        }
                } while (ChDest<0);

                // anlegen/erneuern einen Math Scans
                gapp->channelselector->SetMode(ChDest, ID_CH_M_MATH);
                ChannelMode[ChDest] = ID_CH_M_MATH;
                if(! scan[ChDest]){ // Topo,.. Channel neu anlegen ?
                        //      scan[ChDest] = NewScan(ChannelView[ChDest], data.display.ViewFlg, ChDest, &data);
                        scan[ChDest] = new Scan(ChannelView[ChDest], data.display.ViewFlg, ChDest, &data, ActiveScan->mem2d->GetTyp());
                        // Fehler ?
                        if(!scan[ChDest]){
                                XSM_SHOW_ALERT(ERR_SORRY, ERR_NOMEM,"",1);
                                return;
                        }
                        scan[ChDest] -> set_main_app (get_app());
                }
                // Parameter bernehmen, Groesse einstellen, etc.
                scan[ChDest]->create();
                G_FREE_STRDUP_PRINTF(scan[ChDest]->data.ui.name, "%s", ActiveScan->data.ui.name); // Copy FileName
                // call of MathOp( SourceMem2d, DestMem2d )
                if( (Merr=(*MOp)(ActiveScan, scan[ChDest])) ){
                        XSM_SHOW_ALERT(ERR_MATH, MathErrString[Merr], "", 1);
                        SetMode (ChDest, ID_CH_M_OFF, TRUE);
                } 
                else
                        scan[ChDest]->draw();
        }
	else{
		XSM_SHOW_ALERT(ERR_SORRY, ERR_NOACTIVESCAN,HINT_ACTIVATESCANMATH,1);
	}
}

// generic MathOp callback preparation and Math-Run function call
// -- one Src and one Dest Scan
// -- run get's called for current layer & time, run-function can operate on layers/times itself
void Surface::MathOperationS(gboolean (*MOp)(MATHOPPARAMDONLY)){
	// Src, Quelle: None -- creates some thing only
	// Dest, Ziel:  Math Channel, wird ggf, angelegt oder berschrieben !
	// Punkte: Diagonale Ecken von Rechteck
	int Merr;
	int ChDest;
        int start=0;

        do{
                if((ChDest=FindChan(ID_CH_M_MATH, -1, start)) < 0){ // ist bereits ein Math Scan vorhanden ? (benutzen !)
                        if((ChDest=FindChan(ID_CH_M_OFF)) < 0){ // kein Math Scan, dann neuen anlegen, noch moelich ?
                                // alle belegt !!!
                                XSM_SHOW_ALERT(ERR_SORRY, ERR_NOFREECHAN,"",1);
                                return;
                        }
                }
                if (ActiveScan)
                        if (scan[ChDest] == ActiveScan){
                                start = ChDest+1;
                                ChDest = -1;
                        }
        } while (ChDest < 0);

        // anlegen/erneuern einen Math Scans
        gapp->channelselector->SetMode(ChDest, ID_CH_M_MATH);
        ChannelMode[ChDest] = ID_CH_M_MATH;
        if(! scan[ChDest]){ // Channel neu anlegen ?
                scan[ChDest] = new Scan(ChannelView[ChDest], data.display.ViewFlg, ChDest, &data, ZD_DOUBLE);
                // Fehler ?
                if(!scan[ChDest]){
                        XSM_SHOW_ALERT(ERR_SORRY, ERR_NOMEM,"",1);
                        return;
                }
                scan[ChDest] -> set_main_app (get_app());
        }

	// Parameter bernehmen, Groesse einstellen, etc.
        scan[ChDest]->create();
        G_FREE_STRDUP_PRINTF(scan[ChDest]->data.ui.name, "%s", "Math_Operation");
        // call of MathOp( DestMem2d )
        if( (Merr=(*MOp)(scan[ChDest])) ){
                XSM_SHOW_ALERT(ERR_MATH, MathErrString[Merr], "", 1);
                SetMode (ChDest, ID_CH_M_OFF, TRUE);
        } 
        else{
                scan[ChDest]->draw();
	}
}

// generic MathOp callback preparation and Math-Run function call
// -- one Src and one Dest Scan
// -- run function get's called for all layers (values) and times, may not manipulate layer/time settings
void Surface::MathOperation_for_all_vt(gboolean (*MOp)(MATHOPPARAMS)){
	// Src, Quelle: Aktive Channel
	// Dest, Ziel:  Math Channel, wird ggf, angelegt oder berschrieben !
	// Punkte: Diagonale Ecken von Rechteck
	int Merr;
	int ChDest;
        int start=0;
	Scan *Src=NULL, *Dest=NULL;

	if(ActiveScan){ // ist ein Scan Active ? (als Quelle noeig)
                do{
                        if((ChDest=FindChan(ID_CH_M_MATH, -1, start)) < 0){ // ist bereits ein Math Scan vorhanden ? (benutzen !)
                                if((ChDest=FindChan(ID_CH_M_OFF)) < 0){ // keiner Math Scan, dann neuen anlegen, noch moelich ?
                                        // alle belegt !!!
                                        XSM_SHOW_ALERT(ERR_SORRY, ERR_NOFREECHAN,"",1);
                                        return;
                                }
                        }
                        if (scan[ChDest] == ActiveScan){
                                start = ChDest+1;
                                ChDest = -1;
                        }
                } while (ChDest < 0);

		// anlegen/erneuern einen Math Scans
		gapp->channelselector->SetMode(ChDest, ID_CH_M_MATH);
		ChannelMode[ChDest] = ID_CH_M_MATH;
		if(! scan[ChDest]){ // Topo,.. Channel neu anlegen ?
			//      scan[ChDest] = NewScan(ChannelView[ChDest], data.display.ViewFlg, ChDest, &data);
			scan[ChDest] = new Scan(ChannelView[ChDest], data.display.ViewFlg, ChDest, &data, ActiveScan->mem2d->GetTyp());
			// Fehler ?
			if(!scan[ChDest]){
				XSM_SHOW_ALERT(ERR_SORRY, ERR_NOMEM,"",1);
				return;
			}
                        scan[ChDest] -> set_main_app (get_app());
		}
		// Parameter bernehmen, Groesse einstellen, etc.
		scan[ChDest]->create();
		G_FREE_STRDUP_PRINTF(scan[ChDest]->data.ui.name, "%s", ActiveScan->data.ui.name); // Copy FileName

		Src  = ActiveScan;
		Dest = scan[ChDest];

		// call of MathOp( SourceMem2d, DestMem2d ) for all layers and times
		int ti=0; 
		int tf=0;
		int vi=0;
		int vf=0;
		gboolean multidim = FALSE;
		
		if (Src->data.s.ntimes != 1 || Src->mem2d->GetNv () != 1){
			multidim = TRUE;
			do {
				ti=vi=0;
				tf=Src->number_of_time_elements ()-1;
				if (tf < 0) tf = 0;
				vf=Src->mem2d->GetNv ()-1;
#ifdef MULTI_DIM_SETUP_OPTIONS_ENABLE
				setup_multidimensional_data_operations ("Multidimensional Data Operation", Src, ti, tf, vi, vf);
#endif
			} while (ti > tf || vi > vf);
			
			gapp->progress_info_new ("Multidimensional Math Op", 2);
			gapp->progress_info_set_bar_fraction (0., 1);
			gapp->progress_info_set_bar_fraction (0., 2);
			gapp->progress_info_set_bar_text ("Time", 1);
			gapp->progress_info_set_bar_text ("Value", 2);
		}
		
		if (ti || vi) // force init call if not starting with layer and time index == 0! This has to be caugth by the run function, not to crash with Src=Dest=NULL.
			(*MOp)(NULL, NULL);

		int ntimes_tmp = tf-ti+1;
		for (int time_index=ti; time_index <= tf; ++time_index){
			// Mem2d *m = Src->mem2d_time_element (time_index);
			Src->retrieve_time_element (time_index);
			Mem2d *m = Src->mem2d;
			if (multidim)
				gapp->progress_info_set_bar_fraction ((gdouble)(time_index-ti+0.5)/(gdouble)ntimes_tmp, 1);
			
			Dest->mem2d->Resize (m->GetNx (), m->GetNy (), vf-vi+1, m->GetTyp());
			for (int v_index = vi; v_index <= vf; ++v_index){
				m->SetLayer (v_index);
				Dest->mem2d->SetLayer (v_index-vi);
				gapp->progress_info_set_bar_fraction ((gdouble)(v_index-vi)/(gdouble)(vf-vi+0.5), 2);
				
				if( (Merr=(*MOp)(Src, Dest)) ){
					XSM_SHOW_ALERT(ERR_MATH, MathErrString[Merr], "", 1);
					SetMode (ChDest, ID_CH_M_OFF, TRUE);
				} 
			}
			Dest->append_current_to_time_elements (time_index-ti, m->get_frame_time ());
		}
		
		Dest->data.s.ntimes = ntimes_tmp;
		Dest->data.s.nvalues=Dest->mem2d->GetNv ();
		
		if (multidim){
			gapp->progress_info_close ();
			Dest->retrieve_time_element (0);
			Dest->mem2d->SetLayer(0);
		}

		Dest->draw();
	}
	else{
		XSM_SHOW_ALERT(ERR_SORRY, ERR_NOACTIVESCAN,HINT_ACTIVATESCANMATH,1);
	}
}

// generic MathOp callback preparation and Math-Run function call
// -- two Src (Src1,Src2) and one Dest Scan
// -- run get's called for current layer & time, run-function can operate on layers/times itself
void Surface::MathOperationX(gboolean (*MOp)(MATH2OPPARAMS), int IdSrc2, gboolean size_matching){
	// Src1, Quelle: Aktive Channel
	// Src2, second src: first Channel w Type "X"
	// Dest, Ziel:  Math Channel, wird ggf, angelegt oder berschrieben !
	// Punkte: Diagonale Ecken von Rechteck
	int Merr;
	int ChDest;
        int start=0;
	int ChSrc2;
	if(ActiveScan){ // ist ein Scan Active ? (als Quelle noeig)
		if((ChSrc2=FindChan(IdSrc2))>=0){ // ist zweite Quelle vorhanden ?
			if(!scan[ChSrc2]){
				XSM_DEBUG (DBG_L2, "Scr2==0");
				XSM_SHOW_ALERT(ERR_SORRY, ERR_NO2SRC,HINT_MAKESRC2,1);
				return;
			}
			if (size_matching)
				// are both srcs same size?
				if(scan[ChSrc2]->mem2d->GetNx() != ActiveScan->mem2d->GetNx()
				   || scan[ChSrc2]->mem2d->GetNy() != ActiveScan->mem2d->GetNy()
					){
					XSM_SHOW_ALERT(ERR_SORRY, ERR_SIZEDIFF,HINT_SIZEEQ,1);
					return;
				}
                        do{
                                if((ChDest=FindChan(ID_CH_M_MATH, -1, start)) < 0){ // ist bereits ein Math Scan vorhanden ? (benutzen !)
                                        if((ChDest=FindChan(ID_CH_M_OFF)) < 0){ // keiner Math Scan, dann neuen anlegen, noch moelich ?
                                                // alle belegt !!!
                                                XSM_SHOW_ALERT(ERR_SORRY, ERR_NOFREECHAN,"",1);
                                                return;
                                        }
                                }
                                if (scan[ChDest] == ActiveScan){
                                        start = ChDest+1;
                                        ChDest = -1;
                                }
                        } while (ChDest < 0);

			// anlegen/erneuern einen Math Scans
			gapp->channelselector->SetMode(ChDest, ID_CH_M_MATH);
			ChannelMode[ChDest] = ID_CH_M_MATH;
			if(! scan[ChDest]){ // Topo,.. Channel neu anlegen ?
				//	scan[ChDest] = NewScan(ChannelView[ChDest], data.display.ViewFlg, ChDest, &data);
				scan[ChDest] = new Scan(ChannelView[ChDest], data.display.ViewFlg, ChDest, &data, ActiveScan->mem2d->GetTyp());
				// Fehler ?
				if(!scan[ChDest]){
					XSM_SHOW_ALERT(ERR_SORRY, ERR_NOMEM,"",1);
					return;
				}
                                scan[ChDest] -> set_main_app (get_app());
			}
			// Parameter bernehmen, Groee einstellen, etc.
			scan[ChDest]->create();
			// call of MathOp( Source1Mem2d, Source2Mem2d, DestMem2d )
			if( (Merr=(*MOp)(ActiveScan, scan[ChSrc2], scan[ChDest])) ){
				XSM_SHOW_ALERT(ERR_MATH, MathErrString[Merr], "", 1);
				SetMode (ChDest, ID_CH_M_OFF, TRUE);
			}
			else
				scan[ChDest]->draw();
		}
		else{
			XSM_SHOW_ALERT(ERR_SORRY, ERR_NO2SRC,HINT_MAKESRC2,1);
		}
	}
	else{
		XSM_SHOW_ALERT(ERR_SORRY, ERR_NOACTIVESCAN,HINT_ACTIVATESCANMATH,1);
	}
}

 
