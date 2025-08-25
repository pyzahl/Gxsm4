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

#include <time.h>
// #include <unistd.h>
#include <signal.h>
#include "glib/gstdio.h"

#include <config.h>

#ifndef __XSM_H
#include "xsm.h"
#endif

#ifndef __PLUGIN_CTRL_H
#include "plugin_ctrl.h"
#endif

#ifndef __GLBVARS_H
#include "glbvars.h"
#endif

#include "gxsm_monitor_vmemory_and_refcounts.h"


GSettings *settings_hwi_interfaces = NULL;


/* Main XSM Object */

Xsm::Xsm(){
        main_get_gapp ()->monitorcontrol->LogEvent("Xsm object", "constructor", 3);

	// check for Cmd-Line override
	if(xsmres.UnitCmd)
		strcpy(xsmres.Unit, xsmres.UnitCmd);

	// Look for Unitalias
	for(AktUnit=XsmUnitsTable; AktUnit->alias; ++AktUnit)
		if(! strcmp(AktUnit->alias, xsmres.Unit)) break;

	if(!AktUnit->alias){
		AktUnit = &XsmUnitsTable[1];
		XSM_DEBUG_ERROR (DBG_L1, "Invalid Unit specified ! Falling back to default." );
	}

	// Setup a set of 14 static base Units used as templates
	Unity       = new UnitObj(" "," ");
	ArcUnit     = new UnitObj(UTF8_DEGREE,"\260"); // 0x00B0 "°"
	HzUnit      = new UnitObj("Hz","Hz");
	RadUnit     = new LinUnit(" "," ",180./M_PI);
	VoltUnit    = new UnitObj("V","V",".2f","Volt");
	BZSymUnit   = new UnitObj("%BZ","%BZ");
	CurrentUnit = new UnitObj("nA","nA","g","Current");
	TimeUnitms  = new LinUnit("ms","ms", "Time", 1e-3);
	TimeUnit    = new UnitObj("s","s", "g", "Time");
	CPSUnit     = new CPSCNTUnit("Cps","Cps","Int.");
	CPSHiLoUnit = new CPSCNTUnit("Cps","Cps","Int.");
	EnergyUnit  = new UnitObj("eV","eV",".2f","Energy");
	BZ_Unit     = new BZUnit("%BZ","%BZ", xsmres.Sensitivity, 1.);
	YSUnit      = new SUnit("S","S", xsmres.SampleLayerDist, xsmres.ThetaChGunInt);

	LenUnit     = NULL;
	LenUnitZ    = NULL;

	// do not delete those!!
	X_Unit = Y_Unit = Z_Unit = NULL;
	Xdt_Unit = Ydt_Unit = Zdt_Unit = NULL;

	if( IS_SPALEED_CTRL ){
		if(! strcmp(AktUnit->alias, "BZ") ){
			X_Unit = BZ_Unit->Copy();
			Y_Unit = BZ_Unit->Copy();
		} else {
			X_Unit = VoltUnit->Copy();
                        Y_Unit = VoltUnit->Copy();
                }
		Z_Unit = CPSUnit;
		X_Unit->SetAlias (AktUnit->alias);
		Y_Unit->SetAlias (AktUnit->alias);
		Z_Unit->SetAlias ("Cps");
		Zdt_Unit = CPSUnit->Copy();
		Xdt_Unit = X_Unit->Copy();
                Ydt_Unit = Y_Unit->Copy();
	}else{
		LenUnit  = new LinUnit(AktUnit->s, AktUnit->pss, "L",AktUnit->fac);
		LenUnit->SetAlias (AktUnit->alias);
		LenUnitZ = new LinUnit(AktUnit->s, AktUnit->pss, "H",AktUnit->fac);
		LenUnitZ->SetAlias (AktUnit->alias);
		Z_Unit = LenUnitZ->Copy();
		X_Unit = LenUnit->Copy();
		Y_Unit = LenUnit->Copy();
		Zdt_Unit = LenUnitZ->Copy();
		Xdt_Unit = X_Unit->Copy();
                Ydt_Unit = Y_Unit->Copy();
	}

        Xdt_Unit->addSuffixSym("/s");
        Ydt_Unit->addSuffixSym("/s");
        Zdt_Unit->addSuffixSym("/s");
        
	// Override Hardware Resource by CmdParam ??
	if(xsmres.HardwareTypeCmd)
		strcpy(xsmres.HardwareType, xsmres.HardwareTypeCmd);
	if(xsmres.DSPDevCmd)
		strcpy(xsmres.DSPDev, xsmres.DSPDevCmd);

	hardware=NULL;
	HwI_plugins = NULL;
	// (re)load_hardware_interface is called later
  
	if(IS_SPALEED_CTRL) /* XSM Instrument anlegen */
		Inst = new SPALEED_Instrument(xsmres, 
					      &data.s.Energy, 
					      &data.s.GateTime, 
					      &ModeFlg);
	else
		Inst = new XSM_Instrument(xsmres);


	ModeFlg = 0;
	ZoomFlg = VIEW_ZOOM | VIEW_Z400;


	data.LoadValues(Inst, hardware); // Check Resources, else use peredefiend default Value

	mradius=1.; // Math2D Radius

	file_counter = 0;
	vp_file_counter = 0;

        file_open_mode = xsm::open_mode::append_time; // default -- please keep matched to menu init
        
	data.ui.MakeUIdefaults();

	data.Xunit      = NULL;
	data.Yunit      = NULL;
	data.Zunit      = NULL;
	data.Vunit      = NULL;

	data.SetXUnit (X_Unit);
	data.SetYUnit (Y_Unit);
	data.SetZUnit (Z_Unit);
	data.SetVUnit (VoltUnit);

	data.CurrentUnit= CurrentUnit->Copy ();
	data.VoltUnit   = VoltUnit->Copy ();
	data.TimeUnit   = TimeUnit->Copy ();
	data.TimeUnitms = TimeUnitms->Copy ();
	data.CPSUnit    = CPSUnit->Copy ();
	data.EnergyUnit = EnergyUnit->Copy ();

	XSM_DEBUG (DBG_L2, "Xsm::Xsm : Init done");
        main_get_gapp ()->monitorcontrol->LogEvent("Xsm object", "destructor completed", 3);
}

Xsm::~Xsm(){
	XSM_DEBUG (DBG_L2, "Xsm::~Xsm ** deleting unit objects");
        main_get_gapp ()->monitorcontrol->LogEvent("Xsm object", "destructor", 3);
        
        if(X_Unit)
                delete X_Unit;
        if(Y_Unit)
                delete Y_Unit;
        
        if(LenUnit)
		delete LenUnit;
	if(LenUnitZ)
		delete LenUnitZ;

        if (Xdt_Unit)
                delete Xdt_Unit;
        if (Zdt_Unit)
                delete Zdt_Unit;
        
	delete YSUnit;
	delete BZ_Unit;
	delete EnergyUnit;
	delete CPSHiLoUnit;
	delete CPSUnit;
	delete TimeUnit;
	delete TimeUnitms;
	delete CurrentUnit;
	delete BZSymUnit;
	delete VoltUnit;
	delete RadUnit;
	delete HzUnit;
	delete ArcUnit;
	delete Unity;

	XSM_DEBUG (DBG_L2, "Xsm::~Xsm ... delete hardware");
	if (HwI_plugins) {
		if (!HwI_plugins -> get_xsm_hwi_class () && hardware)
			delete hardware; // remove build-in default base XSM-Hardware!
		delete HwI_plugins; // clean up HwI, even if empty list, will (should) deallocate internally hardware class
	} else
		delete hardware; // else remove GXSM-build-in HW-class stuff

	hardware=NULL;

	delete Inst;
	Inst=NULL;

	XSM_DEBUG_GM (DBG_L2, "Xsm::~Xsm ... done.");
        main_get_gapp ()->monitorcontrol->LogEvent("Xsm object", "destructor complete", 3);
}


void Xsm::reload_hardware_interface (App *app){
        XSM_DEBUG_GM (DBG_L1, "Xsm::reload_hardware_interface ** %s **", HwI_plugins?"unloading HwI":"scanning for HwI plugins and loading");
	// cleanup
	if (HwI_plugins) {
		if (!HwI_plugins -> get_xsm_hwi_class () && hardware)
			delete hardware; // remove build-in default base XSM-Hardware!
		delete HwI_plugins; // clean up HwI, even if empty list, will (should) deallocate internally hardware class
	} else 
		if (hardware)
			delete hardware; // remove GXSM-build-in HW-class stuff

	HwI_plugins = NULL;
	hardware=NULL;

	// cleanup only?
	if (!app){
                XSM_DEBUG_GM (DBG_L1, "Xsm::reload_hardware_interface ... EXIT ONLY");
                return;
        }
        XSM_DEBUG_GM (DBG_L1, "Xsm::reload_hardware_interface ... HwI %s", hardware?"reloading":"loading");
        
	// Check for HwI PIs
	if(!hardware)
		hardware = HwI_Plugin_Load (app);

        XSM_DEBUG_GM (DBG_L1, "Xsm::reload_hardware_interface ... %s", hardware?"OK":"ERROR: HwI load/hardware connect failure");

	if(!hardware){ // still no Hardware Interface ?
		hardware = new XSM_Hardware; // Erzeuge Hardware Simulations Objekt
                XSM_DEBUG_GM (DBG_L1, "Xsm::reload_hardware_interface ... %s", hardware?"Dummy Hardware Base Class activated":"ERROR HwI Base");
        }

        XSM_DEBUG_GM (DBG_L1, "Xsm::reload_hardware_interface ... done.");
}

UnitObj *Xsm::MakeUnit(const gchar *alias, const gchar *label){
	UnitsTable *u;

	// Look for Unitalias
	for(u=XsmUnitsTable; u->alias; ++u)
		if(! strcmp(u->alias, alias)) break;

	if(!u->alias){
		u = &XsmUnitsTable[1];
		XSM_DEBUG_ERROR (DBG_L3, "Invalid Unit >" << alias << "< specified ! Falling back to default.");
	}
	XSM_DEBUG(DBG_L3, "MakeUnit" << u->alias );
	UnitObj *uob = new LinUnit(u->s, u->pss, label, u->fac);
	uob->SetAlias (u->alias);
	XSM_DEBUG(DBG_L3, "MakeUnit" << u->alias << " done." );

	return uob;
}

void Xsm::UpdateUnits(){
	return; // PZ 20070202 - disabled it.
	if(IsMode(MODE_BZUNIT) && X_Unit != BZ_Unit){
		X_Unit = Y_Unit = BZ_Unit;
		main_get_gapp ()->spm_update_all();
	}else
		if(IsMode(MODE_VOLTUNIT) && X_Unit != VoltUnit){
			X_Unit = Y_Unit = VoltUnit;
			main_get_gapp ()->spm_update_all();
		}
}



// Hardware Interface (HwI) Plugin check and handling
// ==================================================

gint Xsm::HwI_Plugin_Check (const gchar *category){
	gint ret = FALSE;
	gint i;
	gchar **catlist, **cs;
	gchar *fullclass;
        gchar *tmp, *hwilist = NULL;

	if( ! category ) 
		return ret;

        XSM_DEBUG_GM (DBG_L1, "Xsm::HwI_Plugin_Check ** %s\n", category);
        

	// HwI category convention for single and multiple subclass support:
	// -----------------------------------------------------------------
	// valid category strings are, ",:" are special chars here, do not use otherwise

	// "video4linux"                 
	//  -> HwI provides a single interface, will be listed as "video4linux" only

	// "SRanger:SPM:SPA-LEED[:..]"
	//  -> HwI provides multiple interfaces based on SRanger, listed and queried as 
	//     "SRanger:SPM" and "SRanger:SPA-LEED" [and ...]

	// separete possible subclasses
	//	for (cs = catlist = g_strsplit_set (category, ":", 0); *cs; ++cs){ // only since glib 2.4
	for (cs = catlist = g_strsplit (category, ":", 0); *cs; ++cs){
		fullclass = g_strdup (*catlist);
		if (cs == catlist) ++cs; // skip base class
		if (*cs){
			g_free (fullclass);
			fullclass = g_strconcat (*catlist, ":", *cs, NULL);
		}

                i = g_settings_get_int (settings_hwi_interfaces, "hwi-count");
                hwilist = g_settings_get_string (settings_hwi_interfaces, "hwi-list");

                XSM_DEBUG_GM (DBG_L1, "Xsm::HwI_Plugin_Check .. [HWI count=%d, list='%s']  %s\n", i, hwilist, category);

                ++i;

                g_settings_set_int (settings_hwi_interfaces, "hwi-count", i);

                if (strlen(hwilist) > 1){
                        tmp = hwilist;
                        hwilist = g_strdup_printf ("%s;%s", hwilist, fullclass); g_free (tmp);
                } else 
                        hwilist = g_strdup (fullclass);

                XSM_DEBUG_GM (DBG_L1, "Xsm::HwI_Plugin_Check .. [HWI count=%d list='%s']  %s\n", i, hwilist, category);
                
		if ( !strcmp (xsmres.HardwareType, fullclass) ) // match?
			ret = TRUE;

		g_free (fullclass);

		if (! *cs) break;
	}

        g_settings_set_string (settings_hwi_interfaces, "hwi-list", hwilist);
        g_settings_sync ();

        g_free (hwilist);

	g_strfreev (catlist);

	return ret;
}

XSM_Hardware* Xsm::HwI_Plugin_Load (App* app){
        settings_hwi_interfaces = g_settings_new (GXSM_RES_BASE_PATH_DOT".hardware-interfaces");
        g_settings_set_int (settings_hwi_interfaces, "hwi-count", 0);
        g_settings_set_string (settings_hwi_interfaces, "hwi-list", "");
        g_settings_sync ();
        
	gint (*hwi_type_check_func)(const gchar *) =  HwI_Plugin_Check;
	GList *PluginDirs = NULL;
	XSM_DEBUG_GM (DBG_L2, "Xsm::HwI_Plugin_Load ** Load/select GXSM HwI plugin(s)" );
	
	// Make plugin search dir list
	PluginDirs = g_list_prepend
		(PluginDirs, g_strconcat(PACKAGE_PLUGIN_DIR, "/hard", NULL));

        HwI_plugins = new gxsm_hwi_plugins (PluginDirs, hwi_type_check_func, xsmres.HardwareType, app, "GXSM-HwI");

	// and remove list
	GList *node = PluginDirs;
	while(node){
		g_free(node->data);
		node = node->next;
	}
	g_list_free(PluginDirs);

        g_clear_object (&settings_hwi_interfaces);

	return HwI_plugins -> get_xsm_hwi_class ();
}




/* SCAN_DATA */

SCAN_DATA::SCAN_DATA(){ 
        GXSM_LOG_DATAOBJ_ACTION (GXSM_GRC_SCANDATAOBJ, "constructor");
        GXSM_REF_OBJECT (GXSM_GRC_SCANDATAOBJ);

        // initialize to safe defaults, must be set later accordingly
        // NOTE: the dimensions below are obsoleted, plase use meme2d->GetNx (), ... please functions in new code
	s.ntimes=1;             /* Numer of Frames -- for future use, must be 1 so far */
	s.nvalues=1;            /* Numer of Values -- for varing one parameter scans */
	s.nx=0; s.ny=0;           /* Number of data points in X and Y */

        // scan data real world dimensions (scale)
	s.rx=s.ry=s.rz=0.0;         /* Scan Ranges [Ang] */
	s.dx=s.dy=s.dz=s.dl=1.0;      /* Step Width  [Ang] */
	s.alpha=0.0;            /* Scan Angle [deg] */
	s.x0=s.y0=0.0;            /* Offset [Ang] */
	s.sx=s.sy=0.0;            /* Scan (Tip) Position withing scan coordinate system used for local probing, etc. -- normally 0,0 [Ang] */

        // generic and often used parameters, handy to have in the main window -- mirrored to main by HwI PlugIns, not essential --
        // IMPORTANT: DO NOT USE FOR ANY HARDWARE EXCEPT FOR WRITING/SHOWING TO THE USER!
	s.SPA_OrgX=s.SPA_OrgY=0.0; /* secondary (used SPA-LEED only) Offset relativ to (0,0) [V] */
	s.Energy=0.0;            /* Energy (if applicable) [eV], set to negative if not relevant */
	s.GateTime=1.0;          /* GateTime (if applicable) [ms], set to negative if not relevant */
	s.Bias=0.0;              /* some Bias (if applicable) [V], set to < -9.999e10 if not relevant */
        s.Motor=0.0;             /* generic "Motor" (auxillary) Voltage */
	s.Current=0.0;           /* some Curren (if applicable) [nA], set to < -9.999e10 if not relevant */
	s.SetPoint=0.0;          /* some AFM SetPoint (if applicable) [V], set to < -9.999e10 if not relevant */
	s.ZSetPoint=0.0;         /* some Z-SetPoint (if applicable) [A] */
        s.pllref=0.0;            /* pllreference freq. in Hz as set */
        
        // real time window of scan frame
	s.tStart=s.tEnd=0;        /* Scan Start and end time, [UNIX time] */
	s.iyEnd=s.xdir=s.ydir=0;    /* last line valid/stopped if while scan, else last line, x,y scan direction -- managed by spmconacontrol */
	s.pixeltime=1.0;        /* time per pixel - set by HwI */
        
        //-todo-offset-, should be OK now
        if(xsmres.ScanOrgCenter) // if (IS_SPALEED_CTRL)
                orgmode = SCAN_ORG_CENTER;
        else
                orgmode = SCAN_ORG_MIDDLETOP;

        scan_mode = SCAN_MODE_SINGLE_DSPSET;
        scan_repeat_mode = SCAN_REPEAT_MODE_UNIDIR;
        scan_type = SCAN_TYPE_NORMAL;

        UnitObj UnityNA ("N/A","N/A");

        // create 10 default unit objects
        Zunit = UnityNA.Copy ();
        Xunit = UnityNA.Copy ();
        Yunit = UnityNA.Copy ();
        Vunit = UnityNA.Copy ();

        Xdt_unit = UnityNA.Copy ();
        Ydt_unit = UnityNA.Copy ();
        Zdt_unit = UnityNA.Copy ();
        
        CurrentUnit = UnityNA.Copy ();
        VoltUnit = UnityNA.Copy ();
        TimeUnit = UnityNA.Copy ();
        TimeUnitms = UnityNA.Copy ();

        CPSUnit = UnityNA.Copy ();
        EnergyUnit = UnityNA.Copy ();

        UnitsAlloc = TRUE;

        XSM_DEBUG (DBG_L2, "SCAN_DATA::SCANDATA #" << ++scandatacount); cnt=scandatacount; 
        GXSM_LOG_DATAOBJ_ACTION (GXSM_GRC_SCANDATAOBJ, "constructor complete");
}

SCAN_DATA::~SCAN_DATA(){ 
        GXSM_LOG_DATAOBJ_ACTION (GXSM_GRC_SCANDATAOBJ, "destructor");
	if(UnitsAlloc){
                GXSM_LOG_DATAOBJ_ACTION (GXSM_GRC_SCANDATAOBJ, "destructor dealloc units");
                
		if(Zunit) { 
			delete Zunit; Zunit=NULL; 
		}
		if(Xunit) { 
			delete Xunit; Xunit=NULL; 
		}
		if(Yunit) { 
			delete Yunit; Yunit=NULL; 
		}
		if(Zdt_unit) { 
			delete Zdt_unit; Zdt_unit=NULL; 
		}
		if(Xdt_unit) { 
			delete Xdt_unit; Xdt_unit=NULL; 
		}
		if(Ydt_unit) { 
			delete Ydt_unit; Ydt_unit=NULL; 
		}
                
		if(Vunit) { 
			delete Vunit; Vunit=NULL; 
		}
		if(CurrentUnit) { 
			delete CurrentUnit; CurrentUnit=NULL; 
		}
		if(VoltUnit) { 
			delete VoltUnit; VoltUnit=NULL; 
		}
		if(TimeUnit) { 
			delete TimeUnit; TimeUnit=NULL; 
		}
		if(TimeUnitms) { 
			delete TimeUnitms; TimeUnitms=NULL; 
		}
		if(CPSUnit) { 
			delete CPSUnit; CPSUnit=NULL; 
		}
		if(EnergyUnit) { 
			delete EnergyUnit; EnergyUnit=NULL; 
		}
	}
	XSM_DEBUG (DBG_L2, "SCAN_DATA::~SCANDATA #" << scandatacount--); 
        GXSM_UNREF_OBJECT (GXSM_GRC_SCANDATAOBJ);
        GXSM_LOG_DATAOBJ_ACTION (GXSM_GRC_SCANDATAOBJ, "destructor complete");
}


void SCAN_DATA::CpUnits(SCAN_DATA &src){
	XSM_DEBUG (DBG_L2, "SCAN_DATA: CpUnits #" << cnt);

        if (&src == this){
                g_critical ("SCAN_DATA CpUnits: src equal self.");
                return;
        }
        
        if (src.Zunit){
                if (Zunit)
                        delete Zunit;
                Zunit = src.Zunit->Copy();
        }
        
	if (src.Xunit){
                if(Xunit)
                        delete Xunit;
		Xunit = src.Xunit->Copy();
        }
        
	if (src.Yunit){
                if(Yunit)
                        delete Yunit;
		Yunit = src.Yunit->Copy();
        }
        
	if(src.Vunit){
                if (Vunit)
                        delete Vunit;
		Vunit = src.Vunit->Copy();
        }
        
	if(CurrentUnit) delete CurrentUnit;
	CurrentUnit = src.CurrentUnit->Copy();
  
	if(VoltUnit) delete VoltUnit;
	VoltUnit = src.VoltUnit->Copy();
  
	if(TimeUnit) delete TimeUnit;
	TimeUnit = src.TimeUnit->Copy();
  
	if(TimeUnitms) delete TimeUnitms;
	TimeUnitms = src.TimeUnitms->Copy();
  
	if(CPSUnit) delete CPSUnit;
	CPSUnit = src.CPSUnit->Copy();
  
	if(EnergyUnit) delete EnergyUnit;
	EnergyUnit = src.EnergyUnit->Copy();

	UnitsAlloc = TRUE;
}

void SCAN_DATA::SetXUnit(UnitObj *u){
	if(Xunit) delete Xunit;
	Xunit = u->Copy();
	if(Xdt_unit) delete Xdt_unit;
	Xdt_unit = u->Copy();
        Xdt_unit->addSuffixSym("/s");
}

void SCAN_DATA::SetYUnit(UnitObj *u){
	if(Yunit) delete Yunit;
	Yunit = u->Copy();
	if(Ydt_unit) delete Ydt_unit;
	Ydt_unit = u->Copy();
        Ydt_unit->addSuffixSym("/s");
}

void SCAN_DATA::SetVUnit(UnitObj *u){
	if(Vunit) delete Vunit;
	Vunit = u->Copy();
}

void SCAN_DATA::SetZUnit(UnitObj *u){
	if(Zunit) delete Zunit;
	Zunit = u->Copy();
	if(Zdt_unit) delete Zdt_unit;
	Zdt_unit = u->Copy();
        Zdt_unit->addSuffixSym("/s");
}

void SCAN_DATA::SetTimeUnit(UnitObj *u){
	if(TimeUnit) delete TimeUnit;
	TimeUnit = u->Copy();
}

/* Save / Retrive Values from Rescource */
void SCAN_DATA::SaveValues(gchar *SetName){
        // obsolete -- dbust, instant now via pcs
};

void SCAN_DATA::LoadValues(XSM_Instrument *Inst, XSM_Hardware *hardware, gchar *SetName){
	gchar *defaultval;

        // NOTE: pretty much obsoleted, valed default in schema and overriden later, only for gschema writer used and initial default.
	XsmRescourceManager xrm("Values", SetName);

	//    xrm.Get("XSMVersion", XSM_VERSION); -- no sense to read back this, only Informative --
	//    xrm.Get("XSMVerDate", XSM_VERDATE); -- no sense to read back this, only Informative --
	s.rx=s.ry=s.rz=1000.; s.dx=s.dy=s.dl=1.; s.alpha=0.; s.x0=s.y0=0.; // fallback

        if (Inst){
		XSM_DEBUG_GP (DBG_L5, "Inst->XResolution=%g A\n", Inst->XResolution());
		XSM_DEBUG_GP (DBG_L5, "Inst->YResolution=%g A\n", Inst->XResolution());
		defaultval = g_strdup_printf("%g", 2000.*Inst->XResolution());
		xrm.Get("s.rx", &s.rx, defaultval);
		g_free(defaultval);
		defaultval = g_strdup_printf("%g", 2000.*Inst->YResolution());
		xrm.Get("s.ry", &s.ry, defaultval);
		g_free(defaultval);
		xrm.Get("s.rz", &s.rz, "1000");
		defaultval = g_strdup_printf("%g", 16.*Inst->XResolution());
		xrm.Get("s.dx", &s.dx, defaultval);
		g_free(defaultval);
		defaultval = g_strdup_printf("%g", 16.*Inst->YResolution());
		xrm.Get("s.dy", &s.dy, defaultval);
		g_free(defaultval);
	}
	xrm.Get("s.dz", &s.dz, "10");
	xrm.Get("s.dl", &s.dl, "10");
	xrm.Get("s.alpha", &s.alpha, "0");
	xrm.Get("s.x0", &s.x0, "0");
	xrm.Get("s.y0", &s.y0, "0");
	xrm.Get("s.SPA_OrgX", &s.SPA_OrgX, "0");
	xrm.Get("s.SPA_OrgY", &s.SPA_OrgY, "0");

	if(hardware)
		defaultval = g_strdup_printf("%d", (int) MIN(R2INT(s.rx/s.dx), hardware->GetMaxPointsPerLine()));
	else
		defaultval = g_strdup_printf("1000");

	xrm.Get("s.nx", &s.nx, defaultval);
	g_free(defaultval);


	if(hardware)
		defaultval = g_strdup_printf("%d", (int) MIN(R2INT(s.ry/s.dy), hardware->GetMaxLines()));
	else
		defaultval = g_strdup_printf("1000");

	xrm.Get("s.ny", &s.ny, defaultval);
	g_free(defaultval);

	xrm.Get("s.nvalues", &s.nvalues, "1");
	xrm.Get("s.ntimes", &s.ntimes, "1");

	xrm.Get("s.Bias", &s.Bias, "2");
	xrm.Get("s.Current", &s.Current, "1");
	xrm.Get("s.SetPoint", &s.SetPoint, "0");
	xrm.Get("s.GateTime", &s.GateTime, "1");
	xrm.Get("s.Energy", &s.Energy, "72");

	defaultval = g_strdup_printf("%d", SCAN_V_QUICK);
	xrm.Get("display.ViewFlg", &display.ViewFlg, defaultval);
	g_free(defaultval);
	xrm.Get("display.contrast", &display.contrast, "0.5");
	xrm.Get("display.bright", &display.bright, "32.0");
	xrm.Get("display.z_high", &display.z_high, "1e6");
	xrm.Get("display.z_low", &display.z_low, "0.0");
	xrm.Get("display.vrange_z", &display.vrange_z, "25.0");
	xrm.Get("display.voffset_z", &display.voffset_z, "0.0");
	xrm.Get("display.vframe", &display.vframe, "0");
	xrm.Get("display.vlayer", &display.vlayer, "0");
}


// END
