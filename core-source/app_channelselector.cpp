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

#include <locale.h>
#include <libintl.h>
#include <gdk/gdk.h>

#include "app_channelselector.h"

#include "unit.h"
#include "pcs.h"
#include "xsmtypes.h"
#include "action_id.h"
#include "glbvars.h"
#include "surface.h"

/* Channel Menu */

static const char* choice_ChView[] =
	{ 
		"No",
		"Grey 2D",
#if ENABLE_3DVIEW_HAVE_GL_GLEW
		"Surface 3D",
#endif
		0
	};

static const char* choice_ChMode[] =
	{ 
		"Off",
		"Active",
		"On",
		"Math",
		"X",
		0
	};

static const char* choice_ChModeExternalData[EXTCHMAX+1] =
	{ 
		"DataMap0",
		"DataMap1",
		"DataMap2",
		"DataMap3",
		"DataMap4",
		"DataMap5",
		"DataMap6",
		"DataMap7",
		0
	};

static const char* choice_ChSDir[] =
	{ 
		"->",
		"<-",
		"2>",
		"<2",
		0
	};

static int NumCh = 0;

static int alife = 0;

ChannelSelector::ChannelSelector (Gxsm4app *app, int ChAnz):AppBase(app, "CHS"){
	GtkWidget* wid;
	GtkWidget* dropDownMenu;
	char txt[32];
	int i,j,k,l;
	NumCh = ChAnz;

        XSM_DEBUG(DBG_L2, "ChannelSelector::ChannelSelector");

	ChSDirWidget = new GtkWidget*[ChAnz];
	ChModeWidget = new GtkWidget*[ChAnz];
	ChViewWidget = new GtkWidget*[ChAnz];
        ChInfoWidget = new GtkWidget*[ChAnz];
	RestoreWidget = new GtkWidget*[1];

	for(i=0; i<ChAnz; i++){
                ChSDirWidget[i] = NULL;
                ChModeWidget[i] = NULL;
                ChViewWidget[i] = NULL;
        }
  
	AppWindowInit (CHANNELSELECTOR_TITLE);

        ch_settings = g_settings_new (GXSM_RES_BASE_PATH_DOT".gui.channelselector");
        int showPrevious = g_settings_get_boolean (ch_settings, "auto-restore") ? 1:0;

        // add scrolled area for channels
        
	GtkWidget *scrollarea = gtk_scrolled_window_new ();
        gtk_widget_set_hexpand (scrollarea, TRUE);
        gtk_widget_set_vexpand (scrollarea, TRUE);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrollarea),
					GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
        gtk_grid_attach (GTK_GRID (v_grid), scrollarea, 0,1, 20,1);
        gtk_widget_show (scrollarea); // FIX-ME GTK4 SHOWALL

	// add store/restore buttons and entry
	static  const char* presets[] = {"A","B","C","Default"};
  
	if (showPrevious == 1){
                wid = gtk_button_new_with_label("Store");
                gtk_grid_attach (GTK_GRID (v_grid), wid, 1,100, 2,1);
                g_signal_connect (G_OBJECT (wid), "clicked",
                                  G_CALLBACK (ChannelSelector::store_callback),
                                  this);
                gtk_widget_show(wid);
                
                dropDownMenu = gtk_combo_box_text_new ();
                RestoreWidget[0] = dropDownMenu;
                gtk_widget_show (dropDownMenu);
                for (k = 0; k<4;k++)
                        gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (dropDownMenu), g_strdup_printf ("%d",k), presets[k]);
                
		gtk_combo_box_set_active_id (GTK_COMBO_BOX (dropDownMenu), "0");
                gtk_grid_attach (GTK_GRID (v_grid), dropDownMenu, 3,100, 2,1);
                
                wid = gtk_button_new_with_label("Restore");
                gtk_grid_attach (GTK_GRID (v_grid), wid, 5,100, 5,1);
                gtk_widget_show(wid);
                g_signal_connect (G_OBJECT (wid), "clicked",
                                  G_CALLBACK (ChannelSelector::restore_callback),
                                  this);    
        }

        gtk_widget_show (v_grid); // FIX-ME GTK4 SHOWALL

        // new grid in scrolled container
        v_grid = gtk_grid_new ();
        gtk_widget_show (v_grid); // FIX-ME GTK4 SHOWALL

	gtk_scrolled_window_set_child (GTK_SCROLLED_WINDOW (scrollarea), v_grid);
        gtk_grid_attach (GTK_GRID (v_grid), wid=gtk_label_new ("Ch"),   0, 0, 1, 1);
        gtk_widget_show (wid); // FIX-ME GTK4 SHOWALL
	gtk_widget_set_tooltip_text (wid, N_("Channel Number"));
        gtk_grid_attach (GTK_GRID (v_grid), wid=gtk_label_new ("View"), 1, 0, 1, 1);
        gtk_widget_show (wid); // FIX-ME GTK4 SHOWALL
	gtk_widget_set_tooltip_text (wid, N_("Scan View Mode"));
        gtk_grid_attach (GTK_GRID (v_grid), wid=gtk_label_new ("Mode"), 2, 0, 1, 1);
        gtk_widget_show (wid); // FIX-ME GTK4 SHOWALL
	gtk_widget_set_tooltip_text (wid, N_("Scan Mode/Data Source"));
        gtk_grid_attach (GTK_GRID (v_grid), wid=gtk_label_new ("Dir"),  3, 0, 1, 1);
        gtk_widget_show (wid); // FIX-ME GTK4 SHOWALL
	gtk_widget_set_tooltip_text (wid, N_("Scan Direction"));
        gtk_grid_attach (GTK_GRID (v_grid), wid=gtk_label_new ("AS"),  4, 0, 1, 1);
        gtk_widget_show (wid); // FIX-ME GTK4 SHOWALL
	gtk_widget_set_tooltip_text (wid, N_("Auto save enable\nfor scan data sources only."));
        gtk_grid_attach (GTK_GRID (v_grid), wid=gtk_label_new ("Info"),  5, 0, 1, 1);
        gtk_widget_show (wid); // FIX-ME GTK4 SHOWALL

        // create channels and add to grid
        // GType types[1] = { G_TYPE_FILE };
        // GType types[] = { GDK_TYPE_FILE_LIST };
        GType types[1] = { GDK_TYPE_FILE_LIST }; // file type only
        GtkDropTarget *target;
        for(i=1; i<=ChAnz; i++){
		sprintf(txt,"% 2d", i);

		wid = gtk_label_new (txt);
		g_object_set_data  (G_OBJECT (wid), "ChNo", GINT_TO_POINTER (i));
		gtk_grid_attach (GTK_GRID (v_grid), wid, 0, i, 1, 1);
                gtk_widget_show (wid); // FIX-ME GTK4 SHOWALL
                target = gtk_drop_target_new (G_TYPE_INVALID, GDK_ACTION_COPY);
                gtk_drop_target_set_gtypes (target, types, G_N_ELEMENTS (types));
                g_signal_connect (target, "drop", G_CALLBACK (AppBase::gapp_load_on_drop_files), GINT_TO_POINTER (i-1));
                gtk_widget_add_controller (GTK_WIDGET (wid), GTK_EVENT_CONTROLLER (target));

		/* View */
		wid = gtk_combo_box_text_new ();
		ChViewWidget[i-1] = wid;
		g_object_set_data  (G_OBJECT (wid), "ChNo", GINT_TO_POINTER (i));
		gtk_grid_attach (GTK_GRID (v_grid), wid, 1, i, 1, 1);
                gtk_widget_show (wid); // FIX-ME GTK4 SHOWALL
                target = gtk_drop_target_new (G_TYPE_INVALID, GDK_ACTION_COPY);
                gtk_drop_target_set_gtypes (target, types, G_N_ELEMENTS (types));
                g_signal_connect (target, "drop", G_CALLBACK (AppBase::gapp_load_on_drop_files), GINT_TO_POINTER (i-1));
                gtk_widget_add_controller (GTK_WIDGET (wid), GTK_EVENT_CONTROLLER (target));

		// Add Channel View Modes
		for(j=0; choice_ChView[j]; j++)
                        gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (wid), NULL, choice_ChView[j]);

                /* connect with signal-handler if selected */
                g_signal_connect (G_OBJECT (wid), "changed",
                                  G_CALLBACK (ChannelSelector::choice_ChView_callback),
                                  NULL);
		gtk_combo_box_set_active (GTK_COMBO_BOX (wid), 1);

		/* Mode */
		wid = gtk_combo_box_text_new ();
		ChModeWidget[i-1] = wid;
		g_object_set_data  (G_OBJECT (wid), "ChNo", GINT_TO_POINTER (i));
		gtk_grid_attach (GTK_GRID (v_grid), wid, 2, i, 1, 1);
                gtk_widget_show (wid); // FIX-ME GTK4 SHOWALL
                target = gtk_drop_target_new (G_TYPE_INVALID, GDK_ACTION_COPY);
                gtk_drop_target_set_gtypes (target, types, G_N_ELEMENTS (types));
                g_signal_connect (target, "drop", G_CALLBACK (AppBase::gapp_load_on_drop_files), GINT_TO_POINTER (i-1));
                gtk_widget_add_controller (GTK_WIDGET (wid), GTK_EVENT_CONTROLLER (target));

                // g_print ("ChannelSelector::ChannelSelector 8 MD [%d]\n",i);

		// Add Default Channel Modes
		for(j=0; choice_ChMode[j]; j++)
                        gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (wid), NULL, choice_ChMode[j]);
		gtk_combo_box_set_active (GTK_COMBO_BOX (wid), 0);

		// Add PID Srcs
		for(l=ID_CH_M_LAST-1, k=0; k<PIDCHMAX; k++, j++){
			if(strcmp(xsmres.pidsrc[k], "-")){
				GString *txt = g_string_new(xsmres.pidsrc[k]);
				gchar **g2;
				g2=g_strsplit(txt->str,",",2);
				if(g2[1])
					xsmres.piddefault=l+1;
                                gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (wid), NULL, g2[0]);
				g_string_free(txt, TRUE);
				g_strfreev(g2);
				xsmres.pidchno[k]=l+1;
				++l;
			}
			else
				xsmres.pidchno[k]=999;
			//			std::cout << "Channelselector create: CH" << (l-1) << " PIDSRC[" << k << "]: " << xsmres.pidsrc[k] << std::endl;
		}

		// Add DAQChannels
		for(k=0; k<DAQCHMAX; k++, j++){
                        //                        g_print ("ChannelSelector::ChannelSelector 10 DAQ [%d, %d]\n",i,k);
			if(strcmp(xsmres.daqsrc[k], "-")){
				GString *txt = g_string_new(xsmres.daqsrc[k]);
				gchar **g2;
                                //                                g_print ("ChannelSelector::ChannelSelector 10 DAQ [%d, %d] %s\n",i,k,xsmres.daqsrc[k]);
				g2=g_strsplit(txt->str,",",2);
				if(g2[1])
					xsmres.daqdefault=l+1;

                                gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (wid), NULL, g2[0]);
				xsmres.daqchno[k]=l+1;
				g_string_free(txt, TRUE);
				g_strfreev(g2);
				++l;
			}
			else
				xsmres.daqchno[k]=999;
			//			std::cout << "Channelselector create: CH" << (l-1) << " DAQSRC[" << k << "]: " << xsmres.daqsrc[k] << std::endl;
		}

		for(j=0; choice_ChModeExternalData[j]; j++){
                        gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (wid), NULL, choice_ChModeExternalData[j]);
                        ++l;
                        xsmres.extchno[j]=l;
                }

                g_signal_connect (G_OBJECT (wid), "changed",
                                  G_CALLBACK (ChannelSelector::choice_ChMode_callback),
                                  NULL);

		/* Add Scan Directions */
		wid = gtk_combo_box_text_new ();
		ChSDirWidget[i-1] = wid;
		g_object_set_data  (G_OBJECT (wid), "ChNo", GINT_TO_POINTER (i));
		gtk_grid_attach (GTK_GRID (v_grid), wid, 3, i, 1, 1);
                gtk_widget_show (wid); // FIX-ME GTK4 SHOWALL

		for(int j=0; choice_ChSDir[j]; j++)
                        gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (wid), NULL, choice_ChSDir[j]);

                /* connect with signal-handler if selected */
                g_signal_connect (G_OBJECT (wid), "changed",
                                  G_CALLBACK (ChannelSelector::choice_ChSDir_callback),
                                  NULL),
		gtk_combo_box_set_active (GTK_COMBO_BOX (wid), 0);


		wid = gtk_check_button_new ();
		g_object_set_data  (G_OBJECT (wid), "ChNo", GINT_TO_POINTER (i));
		gtk_grid_attach (GTK_GRID (v_grid), wid, 4, i, 1, 1);
                gtk_check_button_set_active (GTK_CHECK_BUTTON (wid), true);
                g_signal_connect (G_OBJECT (wid), "toggled",
                                  G_CALLBACK (ChannelSelector::choice_ChAS_callback),
                                  NULL);        
                gtk_widget_show (wid); // FIX-ME GTK4 SHOWALL

		wid = gtk_label_new ("-");
                ChInfoWidget[i-1] = wid;
		gtk_grid_attach (GTK_GRID (v_grid), wid, 5, i, 1, 1);
                gtk_widget_show (wid); // FIX-ME GTK4 SHOWALL

        }
        gtk_widget_show (v_grid);  // FIX-ME GTK4 SHOWALL

        alife = 1;

	set_window_geometry ("channel-selector");
}

void ChannelSelector::restore_callback (GtkWidget *widget, ChannelSelector *cs){

	gint32 *array;
        gsize n_stores;

	// setup the storage compartement key
	gchar *store_key = g_strdup_printf ("channel-setup-%c",'a'+atoi (gtk_combo_box_get_active_id (GTK_COMBO_BOX (cs->RestoreWidget[0]))));

	XSM_DEBUG_GP(DBG_L2, "CannelSelector::restore_callback:  storage key: '%s'\n", store_key);

        // <key name="channel-setup-a" type="a(iii)">
        GVariant *storage = g_settings_get_value (cs->ch_settings, store_key);
        array = (gint32*) g_variant_get_fixed_array (storage, &n_stores, 3*sizeof (gint32));
	// XSM_DEBUG_GP(DBG_L2, "CannelSelector::restore_callback:  n_stores: %d\n", n_stores);

	for (int channel=0; channel < (gint)n_stores && channel < NumCh; ++channel){
                XSM_DEBUG_GP(DBG_L2,
                             "CannelSelector::restore_callback:  ch[%d] : V%d, M%d, D%d\n",
                             channel,
                             array[3*channel + 0],
                             array[3*channel + 1],
                             array[3*channel + 2]
                             );

		main_get_gapp ()->xsm->SetView (channel, array[3*channel+0]);
		main_get_gapp ()->xsm->SetMode (channel, 1+array[3*channel+1]);
		main_get_gapp ()->xsm->SetSDir (channel, array[3*channel+2]);
        }
        
        return;
}

void ChannelSelector::store_callback (GtkWidget *widget, ChannelSelector *cs){
	gint32 *array;
        gsize n_stores = NumCh;

	// setup the storage compartement key
	gchar *store_key = g_strdup_printf ("channel-setup-%c",'a'+atoi (gtk_combo_box_get_active_id (GTK_COMBO_BOX (cs->RestoreWidget[0]))));

	XSM_DEBUG_GP(DBG_L2, "CannelSelector::store_callback:  storage key: '%s'\n", store_key);

        array = g_new (gint32, 3*n_stores);

	for (int channel=0; channel<(gint)n_stores; ++channel){
                array[3*channel + 0] = gtk_combo_box_get_active (GTK_COMBO_BOX (cs->ChViewWidget[channel]));
                array[3*channel + 1] = gtk_combo_box_get_active (GTK_COMBO_BOX (cs->ChModeWidget[channel]));
                array[3*channel + 2] = gtk_combo_box_get_active (GTK_COMBO_BOX (cs->ChSDirWidget[channel]));

                XSM_DEBUG_GP(DBG_L2,
                             "CannelSelector::store_callback:  ch[%d] : V%d, M%d, D%d\n",
                             channel,
                             array[3*channel + 0],
                             array[3*channel + 1],
                             array[3*channel + 2]
                             );
        }
        
        
        // <key name="channel-setup-a" type="a(iii)">
        GVariant *storage = g_variant_new_fixed_array (g_variant_type_new ("(iii)"), array, n_stores, 3*sizeof (gint32));
        
        g_settings_set_value (cs->ch_settings, store_key, storage);

        // g_free array, storage ????
        
	return;
}

void ChannelSelector::choice_ChSDir_callback (GtkWidget *widget, void *data){

        if (gtk_combo_box_get_active (GTK_COMBO_BOX (widget)) == -1)
                return;
            
        gint ch = GPOINTER_TO_INT (g_object_get_data  (G_OBJECT (widget), "ChNo"));
        if (ch > 0)
                ch--;
        else
                return;
	gint i  = (gtk_combo_box_get_active (GTK_COMBO_BOX (widget)));

        XSM_DEBUG_GP(DBG_L3, "ChannelSelector::choice_ChSDir: CH[%d] %d\n", ch, i);

	main_get_gapp ()->xsm->SetSDir (ch, i);
	return;
}

void ChannelSelector::choice_ChView_callback (GtkWidget *widget, void *data){

        if (gtk_combo_box_get_active (GTK_COMBO_BOX (widget)) == -1)
                return;
            
        gint ch = GPOINTER_TO_INT (g_object_get_data  (G_OBJECT (widget), "ChNo"));
        if (ch > 0)
                ch--;
        else
                return;
	gint i  = (gtk_combo_box_get_active (GTK_COMBO_BOX (widget)));

        XSM_DEBUG_GP(DBG_L3, "ChannelSelector::choice_ChView: CH[%d] %d\n", ch, i);

	main_get_gapp ()->xsm->SetView (ch, i);
	return;
}

void ChannelSelector::choice_ChMode_callback (GtkWidget *widget, void *data){

        if (gtk_combo_box_get_active (GTK_COMBO_BOX (widget)) == -1)
                return;
            
        gint ch = GPOINTER_TO_INT (g_object_get_data  (G_OBJECT (widget), "ChNo"));
        if (ch > 0)
                ch--;
        else
                return;
	gint i  = (gtk_combo_box_get_active (GTK_COMBO_BOX (widget)));

        XSM_DEBUG_GP(DBG_L1, "ChannelSelector::choice_ChMode: CH[%d] %d\n", ch, i+1);

	main_get_gapp ()->xsm->SetMode (ch, i+1); // ID_CH_M_OFF := 1 is first valid mode id
	return;
}

void ChannelSelector::choice_ChAS_callback (GtkWidget *widget, void *data){
        gint ch = GPOINTER_TO_INT (g_object_get_data  (G_OBJECT (widget), "ChNo"));
        if (ch > 0)
                ch--;
        else
                return;

        main_get_gapp ()->xsm->SetChannelASflg (ch, gtk_check_button_get_active (GTK_CHECK_BUTTON (widget)) ? 1:0);
	return;
}

void ChannelSelector::SetSDir(int Channel, int SDir){
	gtk_combo_box_set_active (GTK_COMBO_BOX (ChSDirWidget[Channel]), SDir);
}

void ChannelSelector::SetMode(int Channel, int Mode){
        XSM_DEBUG_GP(DBG_L1, "ChannelSelector::SetMode: CH[%d] %d\n", Channel, Mode);
	gtk_combo_box_set_active (GTK_COMBO_BOX (ChModeWidget[Channel]), Mode-1);
}

void ChannelSelector::SetView(int Channel, int View){
	gtk_combo_box_set_active (GTK_COMBO_BOX (ChViewWidget[Channel]), View);
}

void ChannelSelector::SetInfo(int Channel, const gchar *info){
#define SRCS_INFO_ON
#ifdef SRCS_INFO_ON
        gchar *infox = NULL;
        gint mode_pos  = (gtk_combo_box_get_active (GTK_COMBO_BOX (ChModeWidget[Channel])));
        int k = mode_pos - (ID_CH_M_LAST - 1);
        if (k >= 0){
                if (k < PIDCHMAX){
                        infox = g_strdup_printf("%s [%08x]", info, xsmres.pidsrc_msk[k]);
                } else {
                        k -= PIDCHMAX;
                        if (k < DAQCHMAX) {
                                infox = g_strdup_printf("%s [%08x]", info, xsmres.daq_msk[k]);
                        } else {
                                k -= DAQCHMAX;
                                infox = g_strdup_printf("%s [EXT%d]", info, k);
                        }
                }
        } else infox = g_strdup_printf("%s [--]", info);
        gtk_label_set_text (GTK_LABEL(ChInfoWidget[Channel]), infox);
        g_free (infox);
#else
        gtk_label_set_text (GTK_LABEL(ChInfoWidget[Channel]), info);
#endif
}

// mode_id: combobox item position index
void ChannelSelector::SetModeChannelSignal(int mode_id, const gchar* signal_name, const gchar* signal_label, const gchar *signal_unit, double d2unit){
	int k,l;

        while (!alife){
                XSM_DEBUG(DBG_EVER, "GXSM FATAL: not ready, FIX GAPP/HwI STARTUP ORDER !!!!!!!!!!!");
                return;
        }

	if (mode_id < 0 || mode_id > 20){
		XSM_DEBUG(DBG_EVER, "GXSM FATAL: SetModeChannelSignal :: index out of range. Signal: " << signal_name << " mode_id=" << mode_id);
		return;
	}

        XSM_DEBUG(DBG_L2, "Signal: " << signal_name << " mode_id=" << mode_id << " label=" << signal_label);

	for (int i=0; i<MAX_CHANNELS; ++i){
                int current_id=-1;

                if (ChModeWidget[i]){
                        if ((current_id = gtk_combo_box_get_active (GTK_COMBO_BOX (ChModeWidget[i]))) == mode_id)
                                gtk_combo_box_set_active (GTK_COMBO_BOX (ChModeWidget[i]), -1);
                                
                        gtk_combo_box_text_remove (GTK_COMBO_BOX_TEXT (ChModeWidget[i]), mode_id);
                        gtk_combo_box_text_insert (GTK_COMBO_BOX_TEXT (ChModeWidget[i]), mode_id,
                                                   NULL,
                                                   signal_name);

                        if (current_id > 0)
                                gtk_combo_box_set_active (GTK_COMBO_BOX (ChModeWidget[i]), current_id);
                }
	}
	for(l=ID_CH_M_LAST-1, k=0; k<PIDCHMAX; k++, l++){
		if (l == mode_id){
		        strncpy (xsmres.pidsrc[k], signal_name, CHLABELLEN);
			strncpy (xsmres.pidsrcZlabel[k], signal_name, CHLABELLEN);
			strncpy (xsmres.pidsrcZunit[k], signal_unit, 8);
			xsmres.pidsrcZd2u[k] = d2unit;
		}
                //		std::cout << l << "=> " << xsmres.pidsrc[k] << std::endl;
	}
	for(int k=0; k<DAQCHMAX; k++, l++){
		if (l == mode_id){
			strncpy (xsmres.daqsrc[k], signal_name, CHLABELLEN);
			strncpy (xsmres.daqZlabel[k], signal_name, CHLABELLEN);
			strncpy (xsmres.daqZunit[k], signal_unit, 8);
			xsmres.daqZd2u[k] = d2unit;
		}
                //		std::cout << l << "=> " << xsmres.daqsrc[k] << std::endl;
	}
}
