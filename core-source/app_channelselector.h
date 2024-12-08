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


#ifndef GXSM_CHANNELSELECTOR_H
#define GXSM_CHANNELSELECTOR_H

#include <config.h>
#include "gapp_service.h"
#include "xsm_limits.h"
#include "action_id.h"
#include "xsmtypes.h"

extern "C++" {
        extern XSMRESOURCES xsmres; // in xsmtypes.h
}

class ChannelSelector : public AppBase{
public:
        ChannelSelector(Gxsm4app *app, int ChAnz=MAX_CHANNELS);
        virtual ~ChannelSelector(){
                delete [] ChSDirWidget;
                delete [] ChModeWidget;
                delete [] ChViewWidget;
                delete [] ChInfoWidget;
                g_clear_object (&ch_settings);
        };
        static gboolean on_drop (GtkDropTarget *target, const GValue  *value, double x, double y, gpointer data);
        
        void SetSDir(int Channel, int Dir);
        void SetMode(int Channel, int Mode);
        void SetView(int Channel, int View);
        void SetInfo(int Channel, const gchar *info);

        void SetModeChannelSignal(int mode_id, const gchar* signal_name, const gchar* signal_label, const gchar *signal_unit, double d2unit = 1.0);

        // position is a running nummer of data sources from 0 .. 15 0..3 => PIDSRC, 4..11 => DAQSRC (historic grouping)
        void ConfigureHardwareMapping(int position, const gchar* signal_name, guint64 msk, const gchar* signal_label, const gchar *signal_unit, double d2unit = 1.0){
                g_message ("ChannelSelector::ConfigureHardwareMapping: %02d for %32s, 0x%08x at scale %g for %s", position, signal_name, msk, d2unit, signal_unit);
                if (position >= 0 && position < 4){
                        xsmres.pidsrc_msk[position] = msk;
                        SetModeChannelSignal(position+ID_CH_M_LAST-2, signal_name, signal_label, signal_unit, d2unit);
                        return;
                }
                if (position >= 4 && position < 16){
                        xsmres.daq_msk[position-4] = msk;
                        SetModeChannelSignal(position+ID_CH_M_LAST-2, signal_name, signal_label, signal_unit, d2unit);
                        return;
                }
                g_warning ("ChannelSelector::ConfigureHardwareMapping: invalid positon %d for %s, 0x%08x", position, signal_name, msk);
        };

        
        static void choice_ChView_callback (GtkWidget *widget, void *data);
        static void choice_ChMode_callback (GtkWidget *widget, void *data);
        static void choice_ChSDir_callback (GtkWidget *widget, void *data);
        static void choice_ChAS_callback (GtkWidget *widget, void *data);
        static void restore_callback (GtkWidget *widget, ChannelSelector *cs);
        static void store_callback (GtkWidget *widget, ChannelSelector *cs); 

private:
        GtkWidget **ChSDirWidget;
        GtkWidget **ChModeWidget;
        GtkWidget **ChViewWidget;
        GtkWidget **ChInfoWidget;
        GtkWidget **RestoreWidget;

        GSettings *ch_settings;
};

#endif
