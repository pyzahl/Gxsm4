/* Gxsm - Gnome X Scanning Microscopy
 * universal STM/AFM/SARLS/SPALEED/... controlling and
 * data analysis software
 * 
 * Copyright (C) 1999,2000,2001 Percy Zahl
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

/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 8 c-style: "K&R" -*- */

// SPA-LEED Emu by kernel

#include <linux/kernel.h>
#include <math.h>
#include "pci32/dspC32/spa/spacmd.h"

/* Konstanten für DSP  */
#define AD_MAX_VOLT 10.     /* Max Spannung bei Wert DA_MAX_VAL */
#define DA_MAX_VOLT 10.     /* Max Spannung bei Wert DA_MAX_VAL */
#define DA_MAX_VAL  0x7ffe  /* Max Wert für signed 16 Bit Wandler */
#define UDA_MAX_VAL  0xffff  /* Max Wert für unsigend 16 Bit Wandler */

/* Spannung <==> Int-Wert Umrechenfaktoren */
/* Bipolar */
#define         U2FLT(X) ((X)*(float)(DA_MAX_VAL)/AD_MAX_VOLT)
#define         U2INT(X) (int)((X)*(float)(DA_MAX_VAL)/AD_MAX_VOLT+.5)
#define         INT2U(X) ((float)(X)/DA_MAX_VAL*AD_MAX_VOLT)

/* Uinpolar */
#define         UNI_U2FLT(X) ((X)*(float)(UDA_MAX_VAL)/AD_MAX_VOLT)
#define         UNI_U2INT(X) (int)((X)*(float)(UDA_MAX_VAL)/AD_MAX_VOLT+.5)
#define         UNI_INT2U(X) ((float)(X)/UDA_MAX_VAL*AD_MAX_VOLT)


#define  DPRAMBASE     (volatile int*)  (spa.dsp->virtual_dpram)

#define  CMD_BUFFER    (volatile int*)  (DPRAMBASE+0x00)   /* cmd buffer */
#define  CMD_PARM      (volatile int*)  (DPRAMBASE+0x01)   /* */
#define  BUFFER        (volatile int*)  (DPRAMBASE+DSP_BUFFER_START)   /* */
#define  BUFFERL       (volatile unsigned long*)  (DPRAMBASE+DSP_BUFFER_START)   /* */
#define  DPRAML        (volatile unsigned long*)  (DPRAMBASE)   /* */
#define  LCDBUFFER     (volatile unsigned long*)  (DPRAMBASE+DSP_LCDBUFFER)
#define  MAXSCANPOINTS (DSP_DATA_REG_LEN)

#define  DSPack  spa.dsp->SrvReqAck=TRUE

#define MD_CMD          0x08    /* Komando (SRQ) abfragen, ausführen aktiv */
#define MD_SCAN         0x10    /* Beamfinder aktiv */
#define MD_BLK          0x80    /* Blinken zur Kontrolle */

#define LEDPORT(X)       *((unsigned long*)(DPRAMBASE+DSP_USR_DIO))=X

void LCDclear(void);
int LCDprintf(const char *format, ...);

void calc_spaparam(void);

void scan2d(void);
void linescan(int n, float y);

unsigned long ChanneltronCounts(float x, float y);

int GetParamI(unsigned int N);
float GetParamF(unsigned int N);

typedef struct{
    float ms;
    float X0, Y0;
    float len;
    int   N;
    int   Nx,Ny;
    float lenxy;
    float alpha, alphaAlt;
    float E;
    double rotmxx, rotmyy, rotmxy, rotmyx, rotoffx, rotoffy;
} SCANP;

SCANP scanp;

typedef struct{
    struct dspsim_thread_data *dsp;
    SCANP scanp;
    int LastSPAMode;
    int SPAMode;

    double extractor;
    double chanhv, chanmult;
    double chanrepeller;
    double cryfocus;
    double filament, emission;
    double gunfocus, focusval;
    double gunanode;
    double smpdist;
    double smptemp;
    double S,d,ctheta,sens;
    double growing;
} SPALEED;

SPALEED spa;

int InitEmu(struct dspsim_thread_data *dsp){
    spa.dsp    = dsp;
    *CMD_BUFFER=0;
    spa.LastSPAMode = spa.SPAMode = MD_CMD;
    LCDclear();
    LCDprintf("-* DSP Emu *-");
    LEDPORT(spa.SPAMode);  

    spa.scanp.rotmxx = spa.scanp.rotmyy = 1.;
    spa.scanp.rotmxy = spa.scanp.rotmyx = spa.scanp.rotoffx = spa.scanp.rotoffy = 0.;

    spa.scanp.ms  = 1.;
    spa.scanp.X0  = spa.scanp.Y0 = 0.;
    spa.scanp.Nx  = spa.scanp.Ny = 40;
    spa.scanp.N   = 100;
    spa.scanp.len = 100.;
    spa.scanp.lenxy = 100.;
    spa.scanp.E   = 72.;

    spa.scanp.alpha = spa.scanp.alphaAlt = 0.;

    spa.extractor = 0.5;
    spa.chanhv    = 2000.0;
    spa.chanrepeller = 1.0;
    spa.cryfocus  = 0.5;
    spa.filament  = 2.4;
    spa.gunfocus  = 0.5;
    spa.gunanode  = 1.1;
    spa.smpdist   = 20.;
    spa.smptemp   = 300.;
    spa.sens      = 100.;
    spa.d         = 3.141;
    spa.ctheta    = cos(5./57.);
    spa.S         = 2*spa.d*spa.ctheta*sqrt(spa.scanp.E/150.4);
    spa.growing   = 0.;

    calc_spaparam();

    return 0;
}

void ExitEmu(void){
    ;
}

int GetParamI(unsigned int N){ 
    float *ptr = (float*)(CMD_PARM + N);
    return (int)(*ptr);
    //    union { int i; float f; unsigned int u;} u; u.i = *(CMD_PARM + N); return u.i; 
}

float GetParamF(unsigned int N){ 
    float *ptr = (float*)(CMD_PARM + N);
    return *ptr;
}

void LCDclear(void){
    int n;
    for(n=0; n < DSP_LCDBUFFERLEN;)
	*(LCDBUFFER+n++) = ' ';
}

int LCDprintf(const char *format, ...){
    va_list ap;                                                                 
    int nr_of_chars;                                                            
    va_start (ap, format);
    nr_of_chars = vsprintf ((char*)LCDBUFFER, format, ap);
    va_end (ap);
    return (nr_of_chars);
}                                                                             
   
void calc_spaparam(){
  double x;
  x = spa.gunfocus-0.5; // Peak Foucus Parameter
  spa.focusval = 50.*x*x+0.05;

  if(spa.filament > 2.85) spa.emission=0.0;
  else{ 
    x = spa.filament; // Filament Emission
    spa.emission = 100.*(x*x*x*x/10.);
  }

  x = (spa.chanhv*1e-3-3.); // Channeltron Characteristic
  x = 1.+x*x*x*x*x/50.;
  if( x < 0. ) spa.chanmult = 0.;
  else  spa.chanmult = x;
}

void ServiceRequest(struct dspsim_thread_data *dsp){
  switch(*CMD_BUFFER & 0xff){

  case DSP_CMD_SCAN_PARAM:
      spa.scanp.X0  = GetParamF(DSP_X0);
      spa.scanp.Y0  = GetParamF(DSP_Y0);
      spa.scanp.len = GetParamF(DSP_len);
      spa.scanp.N   = GetParamI(DSP_N);
      spa.scanp.alpha = GetParamF(DSP_alpha);
      spa.scanp.ms  = GetParamF(DSP_ms);
      spa.scanp.E   = GetParamF(DSP_E);
      spa.S     = 2*spa.d*spa.ctheta*sqrt(spa.scanp.E/150.4);
      if(spa.scanp.alpha != spa.scanp.alphaAlt){
	  spa.scanp.rotmyy = spa.scanp.rotmxx = cos((double)scanp.alpha);
	  spa.scanp.rotmyx = -(spa.scanp.rotmxy = sin((double)spa.scanp.alpha));
	  spa.scanp.alphaAlt = spa.scanp.alpha;
      }
      spa.scanp.rotoffx = spa.scanp.X0;
      spa.scanp.rotoffy = spa.scanp.Y0;
      //      set_dacXY(0.,0.); 
      //      set_dac(DAC_E_CHANNEL, U2INT(spa.scanp.E));
      //      CountMaxAllow = (unsigned long)(3e6*1e-3*spa.scanp.ms);
      DSPack;
      break;

  case DSP_CMD_SPACTRL_SET:
      spa.extractor = GetParamF(DSP_SPACTRL_EXTRACTOR);
      spa.chanhv    = GetParamF(DSP_SPACTRL_CHANHV);
      spa.chanrepeller = GetParamF(DSP_SPACTRL_CHANREPELLER);
      spa.cryfocus  = GetParamF(DSP_SPACTRL_CRYFOCUS);
      spa.filament  = GetParamF(DSP_SPACTRL_FILAMENT);
      spa.gunfocus  = GetParamF(DSP_SPACTRL_GUNFOCUS);
      spa.gunanode  = GetParamF(DSP_SPACTRL_GUNANODE);
      spa.smpdist   = GetParamF(DSP_SPACTRL_SMPDIST);
      spa.smptemp   = GetParamF(DSP_SPACTRL_SMPTEMP);
      spa.growing   = GetParamF(DSP_SPACTRL_GROWING);
      calc_spaparam();
      DSPack;
      break;

  case DSP_CMD_SCAN_START:
      spa.scanp.len = GetParamF(DSP_len);
      spa.scanp.E   = GetParamF(DSP_E);
      
      spa.LastSPAMode = spa.SPAMode;
      spa.SPAMode = (spa.SPAMode & ~(MD_CMD)) | MD_SCAN;
      linescan(0, GetParamF(DSP_Y0));
      spa.SPAMode = spa.LastSPAMode;
      DSPack;
      break;

  case DSP_CMD_SWAPDPRAM:
      DSPack;
      break;

  case DSP_CMD_SCAN2D:
      spa.scanp.Nx  = GetParamI(DSP_NX);
      spa.scanp.Ny  = GetParamI(DSP_NY);
      spa.scanp.lenxy = GetParamF(DSP_LXY);
      spa.LastSPAMode = spa.SPAMode;
      spa.SPAMode = (spa.SPAMode & ~(MD_CMD)) | MD_SCAN;
      scan2d();
      spa.SPAMode = spa.LastSPAMode;
      DSPack;
      break;

  case DSP_CMD_GETCNT:
      spa.scanp.ms  = GetParamF(DSP_ms);
      spa.scanp.E   = GetParamF(DSP_E);

      *(BUFFERL) = ChanneltronCounts(0., 0.);

      DSPack;
      break;

  default: break;
  }
  REQD_DSP = FALSE;
}

void scan2d(void){
    float x,y,Ux, dUx, Uy, dUy;
    int   i,j,k;
    if(spa.scanp.Nx > 1)
	Ux  = -spa.scanp.lenxy/2.;
    else
	Ux = 0.;
    dUx = spa.scanp.lenxy/spa.scanp.Nx;
    
    if(spa.scanp.Ny > 1)
    Uy = spa.scanp.lenxy/2.;
    else
	Uy = 0.;
    dUy = spa.scanp.lenxy/spa.scanp.Ny;
    
    for(y=Uy, k=j=0; j<spa.scanp.Ny; j++, y-=dUy)
	for(x=Ux, i=0; i<spa.scanp.Nx; i++, k++, x+=dUx){
	    if( k >= MAXSCANPOINTS )
		return;
	    *(BUFFERL + k) = ChanneltronCounts(x, y);
	}
}

void linescan(int n, float y){
    float x, dx;
    int nmax;
    nmax = n + spa.scanp.N + 4;
    if( nmax > MAXSCANPOINTS )
	nmax = MAXSCANPOINTS;

    dx = spa.scanp.len/(float)spa.scanp.N;
    x = -spa.scanp.len/2.;

    for(; n<nmax; n++, x+=dx)
	*(BUFFERL + n) = ChanneltronCounts(x, y);
}

double phi_xy(double dx, double dy){
    double q23=0.;
    if(dx<0.)
	q23=180.;
    if(fabs(dx)>1e-5)
	return q23+180.*atan(dy/dx)/M_PI;
    else return dy>0.?90.:-90.;
}

unsigned long ChanneltronCounts(float x, float y){
    static unsigned long noise=957456292L;
    unsigned long bits;
    double xt,yt,r,a,cntnoise,peak,gaus,lor,timefac;
    xt = x*spa.scanp.rotmxx + y*spa.scanp.rotmxy + spa.scanp.rotoffx;
    yt = x*spa.scanp.rotmyx + y*spa.scanp.rotmyy + spa.scanp.rotoffy;
    // rotoffx,y
    // rotmxx,yy,xy,yx
    // spa.scanp.E
    // spa.scanp.ms
    // x,y in Volt
    r  = sqrt(xt*xt+yt*yt)/spa.scanp.E;
    if(r > 10000.){
	a = fmod(phi_xy(xt,yt)+180.,45.)-22.5;
	a *= 2.;
	r -= a*a;
	if(r>12000)
	    return 0;
    }
    xt *= spa.S/spa.sens;
    yt *= spa.S/spa.sens;
    r  = 1.+sqrt(xt*xt+yt*yt);
    xt = xt > 0. ? fmod(xt+5., 10.) - 5. : fmod(xt-5., 10.) + 5.;
    yt = yt > 0. ? fmod(yt+5., 10.) - 5. : fmod(yt-5., 10.) + 5.;
    bits = (noise & 0x80000000 ? 0:1) + (noise & 0x400000 ? 0:1);
    noise <<= 1;
    noise |= bits;
    if(spa.growing>0.){
      timefac = cos((double)jiffies/1000. * M_PI * spa.growing);
    } else timefac = 1.;
    gaus     = timefac*exp((-xt*xt-yt*yt)/spa.focusval/spa.focusval)/spa.focusval;
    lor      = 1./(spa.focusval+xt*xt+yt*yt);
    peak     = spa.emission*spa.scanp.ms*(gaus+lor)/r;
    cntnoise = sqrt(peak/spa.scanp.ms/100) * (10.-spa.chanrepeller) * (noise&0x5f) + (noise&3);
    return (unsigned long)((cntnoise + peak)*spa.chanmult);
}
