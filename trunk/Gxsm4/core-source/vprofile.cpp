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


#include "view.h"
#include "mem2d.h"
#include "xsmmasks.h"
#include "glbvars.h"
#include "app_profile.h"

// ============================== Class Profiles ==============================

Profiles::Profiles(Scan *sc, int ChNo):View(sc, ChNo){
        XSM_DEBUG (DBG_L2, "Profiles::Profiles");
        profile = NULL;
}

Profiles::Profiles():View(){
        profile = NULL;
}

Profiles::~Profiles(){
        XSM_DEBUG (DBG_L2, "Profiles::~");
        hide();
}

void Profiles::hide(){
        XSM_DEBUG (DBG_L2, "Profiles::hide");
        if(profile)
                delete profile;
        profile=NULL;
}

int Profiles::update(int y1, int y2){
        if(!profile) 
                return 0;

        if(y2 < y1){
                if(ChanNo == main_get_gapp ()->xsm->ActiveChannel)
                        profile->SetActive(TRUE);
                else
                        profile->SetActive(FALSE);
                return 0;
        }

        scan->mem2d->SetDataPktMode(data->display.ViewFlg);

        profile->NewData(scan, y1, FALSE);

        //  profile->UpdateArea();

        return 0;
}

int Profiles::draw(int zoomoverride){
        gchar *titel=NULL;

        if(!scan->mem2d) { 
                XSM_DEBUG (DBG_L2, "Profiles: no mem2d !"); 
                return 1; 
        }

        if(ChanNo > 0)
                titel = g_strdup_printf("Ch%d: %s %s", 
                                        ChanNo+1,data->ui.name, scan->mem2d->GetEname());
        else
                titel = g_strdup_printf("Probe: %s %s", 
                                        data->ui.name, scan->mem2d->GetEname());

        if(!profile){
                profile = new ProfileControl(titel);
                profile->NewData(scan, 0, FALSE);
                //    profile->SetMode(PROFILE_MODE_XGRID | PROFILE_MODE_YGRID | PROFILE_MODE_CONNECT);
                //    profile->SetScaling(PROFILE_SCALE_XAUTO | PROFILE_SCALE_YAUTO | PROFILE_SCALE_YEXPAND);
        }

        //  scan->Pkt2d[0].x=0;
        //  scan->Pkt2d[0].y=0;
        //  scan->Pkt2d[1].x=mem2d->GetNx()-1;
        //  scan->Pkt2d[1].y=0;

        //  profile->NewData(scan, 0, FALSE);
        profile->UpdateArea();

        profile->SetTitle(titel);  
        g_free(titel);
        profile->show();

        XSM_DEBUG (DBG_L5, "Profiles::draw ret");

        return 0;
}
