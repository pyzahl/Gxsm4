/* Gnome gxsm - Gnome X Scanning Microscopy
 * universal STM/AFM/SARLS/SPALEED/... controlling and
 * data analysis software
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

/* ignore tag evaluated by for docuscangxsmplugins.pl -- this is no main plugin file
% PlugInModuleIgnore
*/

#include <config.h>

#include "probe_scan.h"
#include "core-source/regress.h"
#include "core-source/scan.h"
#include "core-source/dataio.h"

#include "core-source/surface.h"
#include "core-source/glbvars.h"
#include "core-source/xsmtypes.h"

#include "core-source/action_id.h"

#define L_NULL  0
#define L_START 1
#define L_MOVE  2
#define L_END   3

#define QSIZE   5  // Size of Line-Handles (Squares)
#define QSIZE1  QSIZE // Anfang
#define QSIZE2  (QSIZE-2) // Ende

ProbeScan::ProbeScan()
  :Scan(ID_CH_V_NO, SCAN_V_DIRECT, -1, NULL, ZD_DOUBLE){
  PI_DEBUG (DBG_L2, "ProbeScan::ProbeScan");
  StopProbeFlg=TRUE;
  data.Xunit = NULL;
  data.Yunit = NULL;
  data.Zunit  = NULL;
  data.LoadValues(NULL, NULL, "SPM_Probe_Set");
  data.ui.SetTitle("SPM Probe");
  data.s.dz = 1.;
}

ProbeScan::~ProbeScan(){
  data.SaveValues("SPM_Probe_Set");
}

int ProbeScan::Save(gchar *fname){
  const char *ret;
  Dataio *Dio;
  Dio = new NetCDF(this, fname);
  
  Dio->Write();
  ret = Dio->ioStatus();
  delete Dio;
  return(ret?1:0);
}

int ProbeScan::Load(gchar *fname){
  const char *ret;
  Dataio *Dio;
  
  Dio = new NetCDF(this, fname); // NetCDF File
  
  Dio->Read();
  ret = Dio->ioStatus();
  delete Dio;
  return(ret?1:0);
}

int ProbeScan::Probe(XSM_Hardware *hw, SPM_Probe_p *prbp){
	static SPM_Probe_p *lastprbp=NULL;
	static int count=0;
	PI_DEBUG (DBG_L2, "ProbeScan::Probe");
	if(!prbp){ 
		PI_DEBUG (DBG_L2, "ProbeScan::Probe Probe start ERROR prbp object is a zero pointer");
		return -1;
	}

	data.ui.SetTitle (prbp->resTitle);

	if (strncmp (data.ui.originalname, main_get_gapp()->xsm->data.ui.originalname, strlen(main_get_gapp()->xsm->data.ui.originalname)))
		count = 0;
	gchar *s;
	data.ui.SetOriginalName (s=g_strdup_printf("%s-%s%03d", main_get_gapp()->xsm->data.ui.originalname, prbp->resName, ++count));
	g_free (s);

	data.SetXUnit(prbp->XUnit);
	data.SetZUnit(prbp->ZUnit);

	StopProbeFlg=FALSE;
	data.s.nx = prbp->nx; 
	data.s.ny = prbp->nsrcs;
	prbp->Resize();
	mem2d->Resize(data.s.nx, data.s.ny);
	mem2d->data->MkXLookup( (double)prbp->xS, (double)prbp->xE );
	PI_DEBUG (DBG_L2, "ProbeScan::Probe start");
	data.s.tStart=time(0);

	// do probing...

	// here DSP-Access!!
	// no longer this way    hw->Action(ACTION_SPM_PROBE, (void *)prbp);

	PARAMETER_SET hardpar;

	PI_DEBUG (DBG_L2, "PRB scan start * Normal" );

	hardpar.N   = DSP_PRBPANZ;
	hardpar.Cmd = DSP_CMD_PROBESCAN;
	hardpar.hp[DSP_PRBSRCS    ].value = prbp->srcs; // Codierte MUX/Srcs Configuration
	hardpar.hp[DSP_PRBOUTP    ].value = prbp->outp; // Channel to Probe
	hardpar.hp[DSP_PRBNX      ].value = prbp->nx;
	hardpar.hp[DSP_PRBXS      ].value = prbp->xS;
	hardpar.hp[DSP_PRBXE      ].value = prbp->xE;
	hardpar.hp[DSP_PRBACAMP   ].value = prbp->ACAmp;
	hardpar.hp[DSP_PRBACFRQ   ].value = prbp->ACFrq;
	hardpar.hp[DSP_PRBACPHASE ].value = prbp->ACPhase;
	hardpar.hp[DSP_PRBACMULT  ].value = prbp->ACMultiplier;
	hardpar.hp[DSP_PRBDELAY   ].value = prbp->delay;
	hardpar.hp[DSP_PRBCIVAL   ].value = prbp->CIval;
	hardpar.hp[DSP_PRBNAVE    ].value = prbp->nAve;
	hardpar.hp[DSP_PRBGAPADJ  ].value = prbp->GapAdj/main_get_gapp()->xsm->Inst->ZResolution(); // convert to DigUnits!!!!
	hw->SetParameter(hardpar, FALSE);
	prbp->ACMultiplier = hardpar.hp[DSP_PRBACMULT].value;
	prbp->ACFrq        = hardpar.hp[DSP_PRBACFRQ].value;

	data.s.tEnd=time(0);

	PI_DEBUG (DBG_L2, "PRB scan: done" );

	hw->ReadProbeData (prbp->nsrcs, prbp->nx, -1, -1, prbp->scan, 1./prbp->ACMultiplier);

	mem2d->CopyFrom(prbp->scan, 0, 0, 0, 0, data.s.nx, data.s.ny);

	PI_DEBUG (DBG_L2, "ProbeScan::Probe SetVM");

	if(!view || lastprbp!=prbp)
		SetView(ID_CH_V_PROFILE);
	else
		draw();

	lastprbp=prbp;

	return 0;
}

int ProbeScan::Stop(){
	StopProbeFlg=TRUE; 
	return 0;
}


// SPM_Probe_p parameter storage class constructor
SPM_Probe_p::SPM_Probe_p(gchar *SetName, gchar *SetTitle)
{
  resName = g_strdup(SetName);
  resTitle = g_strdup(SetTitle);
  // get default values using the resource manager
  XsmRescourceManager xrm(resName);
  xrm.Get("xS", &xS, "-2.0");
  xrm.Get("xE", &xE, "2.0");
  xrm.Get("GapAdj", &GapAdj, "0.0");
  xrm.Get("ACAmp", &ACAmp, "0.0");
  xrm.Get("ACFrq", &ACFrq, "0.0");
  xrm.Get("Phase", &ACPhase, "0.0");
  xrm.Get("Multi", &ACMultiplier, "1.0");
  xrm.Get("nx", &nx, "512");
  xrm.Get("delay", &delay, "0");
  xrm.Get("channels", &channels, "1");
  xrm.Get("nAve", &nAve, "33");
  xrm.Get("nRep", &nRep, "1");
  xrm.Get("CIval", &CIval, "0.0");
  xrm.Get("srcs", &srcs, "16");
  xrm.Get("outp", &outp, "2");

  nsrcs=1;
  scan = new Mem2d(nx, nsrcs, ZD_DOUBLE);
}

// SPM_Probe_p parameter storage class destructor
SPM_Probe_p::~SPM_Probe_p()
{
  // save the actual values in resources
  XsmRescourceManager xrm(resName);
  xrm.Put("xS", xS);
  xrm.Put("xE", xE);
  xrm.Put("ACAmp", ACAmp);
  xrm.Put("ACFrq", ACFrq);
  xrm.Put("nx", nx);
  xrm.Put("delay", delay);
  xrm.Put("nAve", nAve);
  xrm.Put("nRep", nRep);
  xrm.Put("CIval", CIval);
  xrm.Put("srcs", srcs);
  xrm.Put("outp", outp);

  g_free(resName);
  delete scan;
}
