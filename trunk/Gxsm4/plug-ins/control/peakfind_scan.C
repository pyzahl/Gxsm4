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

#include "app_peakfind.h"

#include "regress.h"
#include "scan.h"
#include "dataio.h"

#include "surface.h"
#include "glbvars.h"
#include "xsmtypes.h"

#include "action_id.h"

// #define COPY_PFSCANS_TO_PROFILEVIEW

#define L_NULL  0
#define L_START 1
#define L_MOVE  2
#define L_END   3

#define QSIZE   5  // Size of Line-Handles (Squares)
#define QSIZE1  QSIZE // Anfang
#define QSIZE2  (QSIZE-2) // Ende

PeakFindScan::PeakFindScan()
  :Scan(ID_CH_V_NO, SCAN_V_DIRECT, -1, NULL, ZD_FLOAT){
  PI_DEBUG (DBG_L2, "PeakFindScan::PeakFindScan");
  StopPeakFindFlg=TRUE;
  data.Xunit  = new UnitObj("s","t","g","time");
  data.Zunit  = new UnitObj("CPS","CPS","g","Rate");
  data.LoadValues(NULL, NULL, "SPA_PeakFind_Set");
  data.s.dz = 1.;
  data.s.dx = 1.;
  data.s.dy = 1.;
  data.s.rx = 1.;
  data.s.ry = 1.;
  data.s.x0 = 0.;
  data.s.y0 = 0.;
  data.s.nx = 0;
  data.s.ny = 0;
}

PeakFindScan::~PeakFindScan(){
  PI_DEBUG (DBG_L2, "PeakFindScan::~PeakFindScan" );
  data.SaveValues("SPA_PeakFind_Set");
  if(data.Xunit) delete data.Xunit;
  if(data.Zunit) delete data.Zunit;
  PI_DEBUG (DBG_L2, "done." );
}

int PeakFindScan::Save(gchar *fname){
  const char *ret;
  Dataio *Dio;
  if(!strncasecmp(fname+strlen(fname)-3,".nc",3))
    Dio = new NetCDF(this, fname);
  else
    Dio = new DatFile(this, fname);
  
  Dio->Write();
  ret = Dio->ioStatus();
  delete Dio;
  return(ret?1:0);
}

int PeakFindScan::Load(gchar *fname){
  const char *ret;
  Dataio *Dio;
  
  if(!strncasecmp(fname+strlen(fname)-3,".nc",3))
    Dio = new NetCDF(this, fname); // NetCDF File
  else
    Dio = new DatFile(this, fname); // Dat File
  
  Dio->Read();
  ret = Dio->ioStatus();
  delete Dio;
  return(ret?1:0);
}

int PeakFindScan::PFrun(XSM_Hardware *hw, SPA_PeakFind_p *pfp){
  PI_DEBUG (DBG_L2, "ProbeScan::Probe PF start");

  if(!pfp){ 
    PI_DEBUG (DBG_L2, "ProbeScan::Probe PF start ERROR pfp object is a zero pointer");
    return -1;
  }

  PFhwrun(hw, pfp);

#ifdef COPY_PFSCANS_TO_PROFILEVIEW
  data.s.nx = pfp->npkte; 
  data.s.ny = 4;
  mem2d->Resize(data.s.nx, data.s.ny);
  mem2d->data->MkXLookup( -(double)pfp->npkte/2., (double)pfp->npkte/2. );
  data.s.tStart=time(0);

  for(int i=0; i<data.s.ny; ++i)
    for(int j=0; j<data.s.nx; ++j){
      mem2d->PutDataPkt(pfp->xyscans->GetDataPkt(j,i),j,i);
    }

  data.s.tEnd=time(0);
  PI_DEBUG (DBG_L2, "ProbeScan::Probe SetVM");
  if(!view)
    SetView(ID_CH_V_PROFILE);
  else
    draw();
#endif

  return 0;
}

int PeakFindScan::PFrunI0(XSM_Hardware *hw, SPA_PeakFind_p *pfp){
  PI_DEBUG (DBG_L2, "ProbeScan::Probe PF start");

  if(!pfp){ 
    PI_DEBUG (DBG_L2, "ProbeScan::Probe PF start ERROR pfp object is a zero pointer");
    return -1;
  }

  PFhwrun(hw, pfp);

  data.s.nx = pfp->count+1; 
  data.s.ny = max(mem2d->GetNy(), (pfp->index+1));

  mem2d->Resize(data.s.nx, data.s.ny);
  if(pfp->count==0 && pfp->index==0){
    data.s.tStart=time(0);
    data.s.x0 = 0.;
    data.s.y0 = 0.;
    data.s.rx = 0.;
    data.s.ry = 0.;
    data.s.alpha = 0.;
    data.s.dz = 1.;
  }

  double cps;
  cps = (pfp->xyscans->GetDataPkt(pfp->npkte/2,0)
      + pfp->xyscans->GetDataPkt(pfp->npkte/2-1,0)
      + pfp->xyscans->GetDataPkt(pfp->npkte/2-2,0)
      + pfp->xyscans->GetDataPkt(pfp->npkte/2+1,0)
      + pfp->xyscans->GetDataPkt(pfp->npkte/2+2,0)
      + pfp->xyscans->GetDataPkt(pfp->npkte/2,1)
      + pfp->xyscans->GetDataPkt(pfp->npkte/2-1,1)
      + pfp->xyscans->GetDataPkt(pfp->npkte/2-2,1)
      + pfp->xyscans->GetDataPkt(pfp->npkte/2+1,1)
      + pfp->xyscans->GetDataPkt(pfp->npkte/2+2,1))
      / 10. / pfp->gate;

  // Add Data to Monitor -------
  gchar *datastr=g_strdup_printf("Pf%d Xscan: %10.1f %10.1f %6.3f %6.3f",
				 pfp->index, pfp->I0x, pfp->xI0, pfp->xxf0, pfp->xfwhm);
  main_get_gapp()->monitorcontrol->LogEvent("*ProbePFn", datastr);
  g_free(datastr);

  datastr=g_strdup_printf("Pf%d Yscan: %10.1f %10.1f %6.3f %6.3f",
			pfp->index, pfp->I0y, pfp->yI0, pfp->yxf0, pfp->yfwhm);
  main_get_gapp()->monitorcontrol->LogEvent("*ProbePFn", datastr);
  g_free(datastr);

  datastr=g_strdup_printf("Pf%d Rate: %10.1f CPS", pfp->index, cps);
  main_get_gapp()->monitorcontrol->LogEvent("*ProbePFn", datastr);
  g_free(datastr);
  // ----------------------- done.

  mem2d->PutDataPkt(cps, pfp->count, pfp->index);

  // only once!
  if(pfp->index == 0){
      // set time to X lookup
      data.s.tEnd = time(0);
      data.s.rx   = (double)(data.s.tEnd - data.s.tStart);
      data.s.dl   = data.s.rx;
      mem2d->data->MkXLookup( 0., data.s.rx);
      mem2d->data->MkYLookup( 1., (double)mem2d->GetNy());

      if(pfp->count > 1){
	  if(!view)
	      SetView(ID_CH_V_PROFILE);
	  else
	      draw();
      }
  }

  ++pfp->count;

  return 0;
}

int PeakFindScan::PFhwrun(XSM_Hardware *hw, SPA_PeakFind_p *pfp){
  PI_DEBUG (DBG_L2, "PeakFindScan::PeakFind");
  if(!pfp){ 
    PI_DEBUG (DBG_L2, "PeakFindScan::PeakFind PeakFind start ERROR pfp object is a zero pointer");
    return -1;
  }
  if(!pfp->Mode()) return -1;

  PARAMETER_SET hardpar;
  size_t elem_size=sizeof(long);
  static long linebuffer[DSP_DATA_REG_LEN]; // Max Size for LineData "at once"

  int wdh=pfp->maxloops;

  pfp->x0 = pfp->xorg;
  pfp->y0 = pfp->yorg;
  pfp->Resize();

  while(wdh--){
    // X-Scan setup
    hardpar.N   = DSP_E+1;
    hardpar.Cmd = DSP_CMD_SCAN_PARAM;
    
    hardpar.hp[DSP_X0   ].value = (double)main_get_gapp()->xsm->Inst->YA2Dig(pfp->x0);
    hardpar.hp[DSP_Y0   ].value = (double)main_get_gapp()->xsm->Inst->YA2Dig(pfp->y0);
    hardpar.hp[DSP_len  ].value = (double)main_get_gapp()->xsm->Inst->YA2Dig(pfp->radius*2.);
    hardpar.hp[DSP_N    ].value = pfp->npkte;
    hardpar.hp[DSP_alpha].value = 0.;
    hardpar.hp[DSP_ms   ].value = 1e3*pfp->gate;
    hardpar.hp[DSP_E    ].value = (double)main_get_gapp()->xsm->Inst->eV2V(pfp->energy);
    hw->SetParameter(hardpar);
    
    hardpar.N   = DSP_E+1;
    hardpar.Cmd = DSP_CMD_SCAN_START;
    hardpar.hp[DSP_Y0   ].value = 0.;
    hardpar.hp[DSP_len  ].value = (double)main_get_gapp()->xsm->Inst->YA2Dig(pfp->radius*2.);
    hardpar.hp[DSP_E    ].value = (double)main_get_gapp()->xsm->Inst->eV2V(pfp->energy);
    hw->SetParameter(hardpar, TRUE);
    
    hw->ReadData(linebuffer, pfp->npkte*elem_size);
    //    pfp->xyscans->PutDataLine(0, linebuffer,  MEM_SET);
    for(int i=0; i<pfp->npkte; ++i)
      pfp->xyscans->PutDataPkt((double)linebuffer[i], i,0);
    
    
    // Y-Scan setup
    hardpar.N   = DSP_E+1;
    hardpar.Cmd = DSP_CMD_SCAN_PARAM;
    hardpar.hp[DSP_X0   ].value = (double)main_get_gapp()->xsm->Inst->YA2Dig(pfp->x0);
    hardpar.hp[DSP_Y0   ].value = (double)main_get_gapp()->xsm->Inst->YA2Dig(pfp->y0);
    hardpar.hp[DSP_len  ].value = (double)main_get_gapp()->xsm->Inst->YA2Dig(pfp->radius*2.);
    hardpar.hp[DSP_N    ].value = pfp->npkte;
    hardpar.hp[DSP_alpha].value = 90.;
    hardpar.hp[DSP_ms   ].value = 1e3*pfp->gate;
    hardpar.hp[DSP_E    ].value = (double)main_get_gapp()->xsm->Inst->eV2V(pfp->energy);
    hw->SetParameter(hardpar);
    
    hardpar.N   = DSP_E+1; 
    hardpar.Cmd = DSP_CMD_SCAN_START;
    hardpar.hp[DSP_Y0   ].value = 0.;
    hardpar.hp[DSP_len  ].value = (double)main_get_gapp()->xsm->Inst->YA2Dig(pfp->radius*2.);
    hardpar.hp[DSP_E    ].value = (double)main_get_gapp()->xsm->Inst->eV2V(pfp->energy);
    hw->SetParameter(hardpar, TRUE);
    
    hw->ReadData(linebuffer, pfp->npkte*elem_size);
    //    pfp->xyscans->PutDataLine(1, linebuffer, MEM_SET);
    for(int i=0; i<pfp->npkte; ++i)
      pfp->xyscans->PutDataPkt((double)linebuffer[i], i,1);
    
    // and find max
    pfp->DoFit();
    
    //    PI_DEBUG (DBG_L2, "PF: xy0: (" << pfp->x0 << ", " << pfp->y0 
    //	 << ") new: (" << pfp->xnew << ", " << pfp->ynew 
    //	 << ") shift: " << pfp->shift );

    pfp->x0 = pfp->xnew;
    pfp->y0 = pfp->ynew;

    if(fabs(pfp->shift) < pfp->konvfac) break;

  }
  if(pfp->follow){
    pfp->xorg=pfp->x0;
    pfp->yorg=pfp->y0;
  }

  hw->RestoreParameter();

  return 0;
}

long PeakFindScan::PFget0d(XSM_Hardware *hw, SPA_PeakFind_p *pfp ){
    long cnt;
    PARAMETER_SET hardpar;
    hardpar.N   = DSP_E+1;
    hardpar.Cmd = DSP_CMD_GETCNT;
  
    hardpar.hp[DSP_X0   ].value = (double)main_get_gapp()->xsm->Inst->YA2Dig(pfp->xorg);
    hardpar.hp[DSP_Y0   ].value = (double)main_get_gapp()->xsm->Inst->YA2Dig(pfp->yorg);
    hardpar.hp[DSP_ms   ].value = 1e3*pfp->gate;
    hardpar.hp[DSP_E    ].value = (double)main_get_gapp()->xsm->Inst->eV2V(pfp->energy);
    hw->SetParameter(hardpar);
    hw->ReadData(&cnt, sizeof(long));

    hw->RestoreParameter();

    return cnt;
}

int PeakFindScan::PFget2d(XSM_Hardware *hw, SPA_PeakFind_p *pfp ){
  PARAMETER_SET hardpar;
  pfp->ResizeBuffer();

  hardpar.N   = DSP_E+1;
  hardpar.Cmd = DSP_CMD_SCAN_PARAM;
  
  hardpar.hp[DSP_X0   ].value = (double)main_get_gapp()->xsm->Inst->YA2Dig(pfp->xorg);
  hardpar.hp[DSP_Y0   ].value = (double)main_get_gapp()->xsm->Inst->YA2Dig(pfp->yorg);
  hardpar.hp[DSP_len  ].value = (double)main_get_gapp()->xsm->Inst->YA2Dig(pfp->radius*2.);
  hardpar.hp[DSP_N    ].value = pfp->npkte;
  hardpar.hp[DSP_alpha].value = 0.;
  hardpar.hp[DSP_ms   ].value = 1e3*pfp->gate2d;
  hardpar.hp[DSP_E    ].value = (double)main_get_gapp()->xsm->Inst->eV2V(pfp->energy);
  hw->SetParameter(hardpar);

  hardpar.N   = DSP_LXY+1;
  hardpar.Cmd = DSP_CMD_SCAN2D;
  hardpar.hp[DSP_X0   ].value = 0;
  hardpar.hp[DSP_Y0   ].value = 0;
  hardpar.hp[DSP_len  ].value = 0;
  hardpar.hp[DSP_N    ].value = 0;
  hardpar.hp[DSP_alpha].value = 0;
  hardpar.hp[DSP_ms   ].value = 0;
  hardpar.hp[DSP_E    ].value = 0;
  hardpar.hp[DSP_NX   ].value = pfp->npkte2d;
  hardpar.hp[DSP_NY   ].value = pfp->npkte2d;
  hardpar.hp[DSP_LXY  ].value = (double)main_get_gapp()->xsm->Inst->YA2Dig(pfp->radius*2.);
  hw->SetParameter(hardpar);
  hw->ReadData(pfp->buffer, pfp->buffersize*sizeof(long));

  hw->RestoreParameter();

  return 0;
}



// ****************************************
// ****************************************

// SPM_PeakFind_p parameter storage class constructor
SPA_PeakFind_p::SPA_PeakFind_p(int idx, gchar *SetName)
{
  index=idx;
  resName = g_strdup_printf("%s_%d", SetName, index);
  // get default values using the resource manager
  XsmRescourceManager xrm(resName);
  xrm.Get("xOrg", &xorg, "0.0"); xnew=x0=xorg;
  xrm.Get("yOrg", &yorg, "0.0"); ynew=y0=yorg;
  xrm.Get("relLim", &rellim, "2.0");
  xrm.Get("absLim", &abslim, "10.0");
  xrm.Get("npkte", &npkte, "20");
  xrm.Get("npkte2d", &npkte2d, "40");
  xrm.Get("radius", &radius, "1.0");
  xrm.Get("gate", &gate, "20e-3");
  xrm.Get("gate2d", &gate2d, "5e-4");
  xrm.Get("Energy", &energy, "72.0");
  xrm.Get("Delta", &konvfac, "0.05");

  XsmRescourceManager xrmPC("DSPPeakFindControl");
  xrmPC.Get("xSignum", &xsig, "1.0");
  xrmPC.Get("ySignum", &ysig, "1.0");

  focview = NULL;
  buffer  = NULL;
  buffersize=0;

  xyscans = new Mem2d(npkte_alloc=npkte, 4, ZD_LONG);
  follow=FALSE;
  sleeptime=0.;
  maxloops = 1;
  enabled=TRUE;
  number=0; count=0; name=NULL;
  fitmode = PF_FITMODE_FIT2ND;
  yfwhm = xfwhm = fwhm = 1.;
  yI0 = xI0 = I0 = 1.;
  yxf0 = xxf0  = xf0 = 1.;
  offsetcorrection = 0.;
}

// SPM_PeakFind_p parameter storage class destructor
SPA_PeakFind_p::~SPA_PeakFind_p()
{
    //  PI_DEBUG (DBG_L2, "SPA_PeakFind_p::~SPA_PeakFind_p" );
  // save the actual values in resources
  XsmRescourceManager xrm(resName);
  xrm.Put("xOrg", xorg);
  xrm.Put("yOrg", yorg);
  xrm.Put("relLim", rellim);
  xrm.Put("absLim", abslim);
  xrm.Put("npkte", npkte);
  xrm.Put("npkte2d", npkte2d);
  xrm.Put("radius", radius);
  xrm.Put("gate", gate);
  xrm.Put("gate2d", gate2d);
  xrm.Put("Energy", energy);
  xrm.Put("Delta", konvfac);

  XsmRescourceManager xrmPC("DSPPeakFindControl");
  xrmPC.Put("xSignum", xsig);
  xrmPC.Put("ySignum", ysig);

  if(focview){ delete focview; focview=NULL; }
  if(buffer){ g_free(buffer); buffer=NULL; }

  g_free(resName);
  delete xyscans;
  //  PI_DEBUG (DBG_L2, "done." );
}


int SPA_PeakFind_p::DoFit(){
  double dx, dy;
  if(FitFind(0, dx)) return -1;
  xfwhm = fwhm; skl(xfwhm);
  xI0   = I0;
  xxf0  = xf0;  skltr(xxf0);
  dx    = xxf0;
  
  if(FitFind(1, dy)) return -1;
  yfwhm = fwhm; skl(yfwhm);
  yI0   = I0;
  yxf0  = xf0;  skltr(yxf0);
  dy    = yxf0;

  // abslim & rellim check
  
  shift = sqrt(dx*dx+dy*dy);
  if(shift > rellim){
    dx *= rellim/shift;
    dy *= rellim/shift;
  }

  xnew = x0 + xsig*dx;
  ynew = y0 + ysig*dy;

  if(xnew*xnew + ynew*ynew > abslim*abslim){
    xnew = x0;
    ynew = y0;
    return -1;
  }

  return 0;
}

int SPA_PeakFind_p::FitFind2nd(int xy, double &m){
  double a,b,c;
  int    n = xyscans->GetNx();
  PI_DEBUG (DBG_L2, "SPA_PeakFind_p::FitFind2nd");
  
  I0x = xyscans->GetDataPkt(n/2,0);
  I0y = xyscans->GetDataPkt(n/2,1);

  // calculates best fit Polynom of order 2
  // use order=2, data=xyscans, line=xy; x==xindizes [0..N-1]
  StatInp fit(2, xyscans, xy); 

  // Get Koefs of Poly  (y = a + bx + cx^2)
  a = fit.GetPolyKoef(0);
  b = fit.GetPolyKoef(1);
  c = fit.GetPolyKoef(2);

  fwhm = 0.; // no sense here...
  xf0  = -b/2./c;
  I0   = a+(b+c*xf0)*xf0;
  
  xyscans->data->SetPtr(0,2+xy);
  for (int col = 0; col < n; col++) {
    xyscans->data->SetNext(fit.GetPoly((double)col));
    //    xyscans->data->SetNext(a+(b+c*col)*col);
  }
  
  // max at y'(x_max) = 0:
  if(fabs(c) < 1e-7)
    return -1;
  
  m = xf0 - .5;
  return 0;
}

double tr_log(double x, double a){ return log(((x-a)<1.?1.:(x-a))); }

int SPA_PeakFind_p::FitFindGaus(int xy, double &m){
  int    n = xyscans->GetNx();
  double a, b, c;
  double  offset=0.;
  
  PI_DEBUG (DBG_L2, "SPA_PeakFind_p::FitFindGaus");
  
  I0x = xyscans->GetDataPkt(n/2,0);
  I0y = xyscans->GetDataPkt(n/2,1);
  
  if(offsetcorrection > 0.1){
    double offsets[2];
    offsets[0]  = xyscans->data->Z(0,xy);
    offsets[0] += xyscans->data->Z(1,xy);
    offsets[0] /= 2*offsetcorrection;
    offsets[1]  = xyscans->data->Z(n-2,xy);
    offsets[1] += xyscans->data->Z(n-1,xy);
    offsets[1] /= 2*offsetcorrection;
    offset = offsets[0] > offsets[1] ? offsets[1] : offsets[0];
  }
  
  // calculates best fit Polynom of order 2
  // use order=2, data=xyscans, line=xy; x==xindizes [0..N-1]
  // use log transformation of y values !
  StatInp fit(2, xyscans, xy, tr_log, offset); 

  // Get Koefs of Poly  (y = a + bx + cx^2)
  a = fit.GetPolyKoef(0);
  b = fit.GetPolyKoef(1);
  c = fit.GetPolyKoef(2);

  fwhm = 2.*sqrt(-0.6931472/c);
  xf0  = -b/2./c;
  I0   = exp(a-b*b/4./c);
  
  xyscans->data->SetPtr(0,2+xy);
  for (int col = 0; col < n; col++) {
    xyscans->data->SetNext(offset+I0*exp(c*((double)col-xf0)*((double)col-xf0)));
  }
  
  m = xf0;
  return 0;
}

double tr_inv(double x, double a){ return 1./(((x-a)<1.?1.:(x-a))); }

int SPA_PeakFind_p::FitFindLorenz(int xy, double &m){
  int    n = xyscans->GetNx();
  double a, b, c;
  double  offset=0.;
  
  PI_DEBUG (DBG_L2, "SPA_PeakFind_p::FitFindLorenz");
  
  I0x = xyscans->GetDataPkt(n/2,0);
  I0y = xyscans->GetDataPkt(n/2,1);
  
  if(offsetcorrection > 0.1){
    double offsets[2];
    offsets[0]  = xyscans->data->Z(0,xy);
    offsets[0] += xyscans->data->Z(1,xy);
    offsets[0] /= 2.*offsetcorrection;
    offsets[1]  = xyscans->data->Z(n-2,xy);
    offsets[1] += xyscans->data->Z(n-1,xy);
    offsets[1] /= 2.*offsetcorrection;
    offset = offsets[0] > offsets[1] ? offsets[1] : offsets[0];
  }
  
  // calculates best fit Polynom of order 2
  // use order=2, data=xyscans, line=xy; x==xindizes [0..N-1]
  // use special "save inverse" transformation of y values !
  StatInp fit(2, xyscans, xy, tr_inv, offset); 

  // Get Koefs of Poly  (y = a + bx + cx^2)
  a = fit.GetPolyKoef(0);
  b = fit.GetPolyKoef(1);
  c = fit.GetPolyKoef(2);
  
  xf0  = -b/2./c;
  I0   = 4.*c/(4*a*c-b*b);
  fwhm = 2.*sqrt(1./c/I0);
  
  xyscans->data->SetPtr(0,2+xy);
  for (int col = 0; col < n; col++) {
    xyscans->data->SetNext(offset+I0/(1.+c*I0*((double)col-xf0)*((double)col-xf0)));
  }
  
  m = xf0;
  return 0;
}

