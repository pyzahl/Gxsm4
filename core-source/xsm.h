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

#pragma once

#ifndef __XSM_H
#define __XSM_H

// "C++" headers
#include <iostream>
#include <fstream>

// "C" headers
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <cstring>

// system headers
#include <unistd.h>

#include <glib.h>

// Gxsm headers
#include "instrument.h"
#include "xsmhard.h"
#include "xsmtypes.h"
#include "action_id.h"
#include "unit.h"
#include "limits.h"


class gxsm_hwi_plugins;
class App;


class Xsm{
public:
        Xsm ();
        virtual ~Xsm ();

        double XStepMin(){ return Inst->XResolution(); };
        double XStepMax(){ return Inst->XRangeMax()/50.; };
        double YStepMin(){ return Inst->YResolution(); };
        double YStepMax(){ return Inst->YRangeMax()/50.; };

        double XMinPoints(){ return 2.; };
        double XMaxPoints(){ if (hardware) return (double)hardware->GetMaxPointsPerLine(); else return (double)MAXDATALINES; };
        double YMinPoints(){ return 2.; };
        double YMaxPoints(){ return (double)MAXDATALINES; };

        double MaxValues(){ return MAXVALUES; };

        double XRangeMin(){ return Inst->XResolution(); };
        double XRangeMax(){ return Inst->XScanRangeMax(); };
        double YRangeMin(){ return Inst->YResolution(); };
        double YRangeMax(){ return Inst->YScanRangeMax(); };

        double XOffsetMin(){ return -Inst->X0RangeMax(); };
        double XOffsetMax(){ return +Inst->X0RangeMax(); };
        double YOffsetMin(){ return -Inst->Y0RangeMax(); };
        double YOffsetMax(){ return +Inst->Y0RangeMax(); };

        UnitObj *MakeUnit(const gchar *alias, const gchar *label);

        void SetModeFlg(long m){ ModeFlg = m | (ModeFlg&~m); UpdateUnits(); };
        void UpdateUnits();
        void ClrModeFlg(long m){ ModeFlg = (ModeFlg&~m); };
        gboolean IsMode(long m){ return(ModeFlg&m); };
        long MausMode(long m=0){ if(m) MausModeFlg=m; return(MausModeFlg); };

        void reload_hardware_interface (App *app);

        static gint       HwI_Plugin_Check (const gchar *category);
        XSM_Hardware     *HwI_Plugin_Load (App* app);
        gxsm_hwi_plugins *HwI_plugins; /* GXSM Hardware Interface Plugin */

        XSM_Instrument   *Inst; /* XSM Instrument - Gerätedaten */
        XSM_Hardware     *hardware;       /* Universellen Hardware-Objekt */

        SCAN_DATA data;        /* Daten zum verändern */

        UnitObj *Unity;
        UnitObj *LenUnit;
        UnitObj *LenUnitZ;

        UnitObj *ArcUnit;
        UnitObj *HzUnit;
        UnitObj *RadUnit;
        UnitObj *VoltUnit;
        UnitObj *BZSymUnit;
        UnitObj *TimeUnitms;
        UnitObj *TimeUnit;
        UnitObj *EnergyUnit;
        CPSCNTUnit *CPSUnit;
        CPSCNTUnit *CPSHiLoUnit;
        BZUnit  *BZ_Unit;
        SUnit   *YSUnit;
        UnitObj *CurrentUnit;

        UnitsTable *AktUnit;
        UnitObj *X_Unit;
        UnitObj *Y_Unit;
        UnitObj *Z_Unit;

        UnitObj *Xdt_Unit;
        UnitObj *Ydt_Unit;
        UnitObj *Zdt_Unit;

        

        xsm::open_mode file_open_mode;

        double mradius;  // radius for 2D convolution kernels
  
        long MausModeFlg;
        long ModeFlg;
        long ZoomFlg;

        int GetFileCounter() { return file_counter; };
        void FileCounterInc() { file_counter++; };
        int GetNextVPFileCounter() { return ++vp_file_counter; };
        void ResetVPFileCounter() { vp_file_counter=0; };
        
        int    file_counter; /* File Counter */
        int    vp_file_counter; /* VP File Counter */

protected:
private:
};

#endif
