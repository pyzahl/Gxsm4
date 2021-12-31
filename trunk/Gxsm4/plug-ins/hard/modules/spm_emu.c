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

// SPM Emu by kernel

#include <linux/kernel.h>
#include <math.h>
#include "include/dsp-pci32/xsm/xsmcmd.h"
#include "include/dsp-pci32/xsm/mover.h"

//------------ begin instrument definition ---------------

#define TITLE "** STM-SIM **"

#define TYP_STM
#define OSZI_ENABLE

#define MAXDA_XVAL 32763L
#define MAXDA_YVAL 32763L
#define MAXDAVAL   32763L

#define ZSIGNUM(Z)          (Z)  /* Vorzeichenanpassung Z Signal, ggf. -(Z) füt CI > 0 */
#define ZDATASIGNUM(Z)      (-(Z))  /* Vorzeichenanpassung Z Daten, bei neg. war Contrast falsch !! positiv 31.8.1999 PZ */
/****** Funktion  ........  IO-Belegung Stress/AFM: ***********/
#define OSZI_bit(X,Y)       ;
#define SL_OFF              3
#define SLZ_P               2
#define SLZ_M               1
#define SLIDERPORT(X)       ;
#define SLIDERPORT_bit(X,Y) ;
// MB Test Mode Bit0: Trigger Bit1: Z--, Bit2 Z++
#define ZSLIDER_ON          
#define ZSLIDER_OFF         
/* Trigger: 0     = disable
 * Trigger: 0-1-0 = Step
 * ----TRIG_ON-OFF-ON------
 */
#define SLIDERTRIGGER_ON    
#define SLIDERTRIGGER_OFF   
/* Bits 5,6 Gain-Reeds */

/* DSP-Kernel-Programm Modes
 * - Int09 ist globale Zeitbasis
 * Modes (Bitcodiert):
 * 0 1 2 3  4  5  6  7
 * 1 2 4 8 10 20 40 80
 * - - - 
 *  LCD
 */
#define MD_CMD 		0x0004 	/* Komando (SRQ) abfragen, ausführen aktiv */
#define MD_PID		0x0008	/* PID-Regler aktiv */
#define MD_MOVE		0x0100	/* MoveTo Xnew, Ynew wird ausgeführt */
#define MD_MOVEOFF      0x0300	/* MoveTo new Offset */
#define	MD_SCAN		0x0010	/* Linescan ausführen */
#define MD_BLK		0x0080	/* Blinken zur Kontrolle */
#define	MD_TIPDN	0x1000	/* Spitze annähern starten */
#define MD_ITU		0x0020	/* I Tunnel ~> Sollwert, Flag 1 wenn Tunnelstrom detektiert und im Regelbereich */
#define MD_CRASH	0x0040	/* I Tunnel >> Sollwert, Flag 1 wenn Tunnelstrom nicht mehr im Regelbereich */
#define MD_PROBE        0x2000  /* Probe control (spectroscopy etc.) */

//#define USE_MOVER_APP
#define USE_IO_APP

#define AFM_MOVER
#define AFM_MOVER_NO_Q

//------------ end instrument definition ---------------
#define REGEL_DT (1./(float)50000.)
#define  TIP_ZPiezoMax   3

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


#define  DPRAMBASE     (volatile int*)  (spm.dsp->virtual_dpram)

#define  CMD_BUFFER    (volatile int*)  (DPRAMBASE+0x00)   /* cmd buffer */
#define  CMD_PARM      (volatile int*)  (DPRAMBASE+0x01)   /* */
#define  BUFFER        (volatile int*)  (DPRAMBASE+DSP_BUFFER_START)   /* */
#define  BUFFERL       (volatile unsigned long*)  (DPRAMBASE+DSP_BUFFER_START)   /* */
#define  BUFFERS       (volatile short*)          (DPRAMBASE+DSP_BUFFER_START)
#define  DPRAML        (volatile unsigned long*)  (DPRAMBASE)   /* */
#define  LCDBUFFER     (volatile unsigned long*)  (DPRAMBASE+DSP_LCDBUFFER)
#define  MAXSCANPOINTS (DSP_DATA_REG_LEN)

#define  DSPack  spm.dsp->SrvReqAck=TRUE

#define LEDPORT(X)       *((unsigned long*)(DPRAMBASE+DSP_USR_DIO))=X

void LCDclear(void);
int LCDprintf(const char *format, ...);

int GetParamI(unsigned int N);
float GetParamF(unsigned int N);

#define SURFSIZE 64

int surftab[SURFSIZE];

#define MAXCHANNELS 10
#define MAXDATABUFFERS 5

const float maxval = +3.2765e4;
const float minval = -3.2765e4;

typedef struct{
	int     MV_Xnew;
	int     MV_Ynew;
	int     MV_XPos;
	int     MV_YPos;
	int	MV_stepsize;	/* max. Schrittweite */
	unsigned int	MV_nRegel;	/* Anzahl Reglerdurchläufe je Schritt */
	int	LS_nx2scan;	/* Scanlinelänge */
	int	LS_nx_pre;	/* Scan-Vorlauf in Me punkten */
	int	LS_srcs;	/* Source Channels */
	int	LS_dnx;		/* Me punktabstand */
	int	LS_stepsize;	/* max. Schrittweite */
	unsigned int	LS_nRegel;	/* Anzahl Reglerdurchläufe je Schritt */
	int	LS_nAve;	/* Anzahl durchzuführender Mittelungen */
	int	LS_IntAve;	/* =0: kein IntAve, =1: Aufsummieren von Punkte zu Punkt */
	int     LS_LastVal;     /* vorhergehender Wert für 32bit DPRAM Mode */
	long	LS_xE, LS_xnext; /* Für interne Verwendung */
	int	LS_AveCount;	/* Mittelungszähler */
	int	LS_SmpFlg;	/* Sample Flag */
	int	LS_Xindex, LS_Xinc;	/* LS_BUFFER Index */
	int     LS_ChPID;

	int     PRB_nx;
	int     PRB_xS;
	int     PRB_xE;
	double  PRB_ACAmp;
	double  PRB_ACFrq;
	double  PRB_ACPhase;
	double  PRB_GapAdj;
	int     PRB_ACdelay;
	int     PRB_CIval;
	int     PRB_AveCount;
	int     PRB_srcs;
	int     PRB_outp;
	int     PRB_nAve;
	int     PRB_ChAnz;
	int     PRB_ChAkt;
	int     PRB_Xindex;
	int     PRB_XPos;
	int     PRB_delay;
	int     PRB_dnx;
	int     PRB_CPIS[3];
	int     PRB_oldU;
	int     PRB_oldZ;
	float   PRB_ChSum[MAXCHANNELS];   /* Summierungsvariable für ChXX */

	int     TIP_nWarte;
	int     TIP_nSteps;
	int     TIP_DUzRev;
	int     TIP_DUz;
	int     TIP_Zdn;
	int     TIP_Mode;
	int     afm_piezo_amp;
	int     afm_u_piezomax;
	int     afm_piezo_speed;
	int     afm_piezo_steps;
	int     afm_mover_mode;
	int     afm_mover_app;
	int     AFM_MV_count;
	int     AFM_MV_Scount;
	
/* PID-Regler Variable */
	int   U_tunnel;     /* Vorgabetunnelspannung */
	long  I_tunnel_soll;/* Tunnelsollstrom */

	float Spg;
	float I_ist_alt, I_ist, I_soll, I_delta;   /* PI-Reglerparameter */
	float U_z;          /* Z-Spannungswert vom Regler vorgegeben */
	float Const_P;      /* Regler P Konstante */
	float Const_I;      /* Regler I Konstante */
	float Const_S;      /* Regler Slope Konstante */
	float SetPoint;     /* Regler D Konstante */
	float I_sum;       /* I-Integral */
	
	int   LogOffset;  /* für log. Regler */
	float LogFaktor;  /* für log. Regler */
	float LogSkl;
	float LogSklNeu;
	
	unsigned long *databuffer[MAXDATABUFFERS];
	unsigned long *channelptr[MAXCHANNELS];
	int     LS_ChAnz, LS_ChAkt;
	float   LS_ChSum[MAXCHANNELS];   /* Summierungsvariable für ChXX */
	float   *ChPtr[MAXCHANNELS];     /* Pointer to Channel Source */  
	
	float adc_values[MAXCHANNELS];
	long  dacbufferZ;
	long  dacbufferU;

	long  dacXval, dacYval, dacUval, dacZval;

	double rotmxx, rotmyy, rotmxy, rotmyx, rotoffx, rotoffy;
} SCANP;

SCANP scanp;

typedef struct{
    struct dspsim_thread_data *dsp;
    SCANP scanp;
    int LastSPMMode;
    int SPMMode;
} SPM;

SPM spm;

void calc_xy( void );
void DACoutXY(long ix, long iy);
void DACoutX(long value);
void DACoutY(long value);
void DACoutZ(long value);
void DACoutU(long value);

void run_testscan( void );
void run_testprbscan( void );

void run_dspsim_loop( void );


#ifdef __PPC__
#include "mathemu.c"
#endif

int InitEmu(struct dspsim_thread_data *dsp){
	int i;
	spm.dsp    = dsp;
	*CMD_BUFFER=0;
	spm.LastSPMMode = spm.SPMMode = MD_CMD;
	LCDclear();
	LCDprintf("Welcome to the SPM/DSP Emu");
	LEDPORT(spm.SPMMode);  
	
	spm.scanp.MV_XPos = 0;
	spm.scanp.MV_YPos = 0;
	spm.scanp.rotmxx = spm.scanp.rotmyy = 1.;
	spm.scanp.rotmxy = spm.scanp.rotmyx = spm.scanp.rotoffx = spm.scanp.rotoffy = 0.;
// ... more
	
#define SURFCORR 20
	for(i=0; i<SURFSIZE; ++i){
		if (i<SURFSIZE/4){
			surftab[i] = -SURFCORR;
			continue;
		}
		if (i<SURFSIZE/2){
			surftab[i] = -SURFCORR+(i-SURFSIZE/4)*SURFCORR*2/(SURFSIZE/4);
			continue;
		}
		if (i<3*SURFSIZE/4){
			surftab[i] = SURFCORR;
			continue;
		}
		surftab[i] = SURFCORR-(i-3*SURFSIZE/4)*SURFCORR*2/(SURFSIZE/4);
	}

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
   
void ServiceRequest(struct dspsim_thread_data *dsp){
	double XYflt;
	switch(*CMD_BUFFER & 0xff){

	case DSP_CMD_SET_WERTE:
		spm.scanp.U_tunnel = U2INT (GetParamF (DSP_UTUNNEL));
		spm.scanp.Spg = (spm.scanp.I_ist_alt = GetParamF (DSP_ITUNNEL));
		if(spm.scanp.Spg > AD_MAX_VOLT) spm.scanp.Spg = AD_MAX_VOLT;
		if(spm.scanp.Spg < -AD_MAX_VOLT) spm.scanp.Spg = -AD_MAX_VOLT;
		spm.scanp.Const_P = GetParamF (DSP_CP);
		spm.scanp.Const_I = GetParamF (DSP_CI);
		spm.scanp.SetPoint = GetParamF (DSP_CD);
		spm.scanp.Const_S = GetParamF (DSP_CS);
		
		if(spm.scanp.Const_S > 2. || spm.scanp.Const_S < 0.) spm.scanp.Const_S=1.; // for savety
		
		DACoutU (spm.scanp.U_tunnel);	/* Tunnelspg. setzen */
		
		DSPack;
		break;
		
	case DSP_CMD_SET_LOG_WERTE:
		DSPack;
		break;

	case DSP_CMD_ROTPARAM:
		/* Moveto absolute */
		spm.scanp.rotmxx = GetParamF (DSP_ROTXX);
		spm.scanp.rotmxy = GetParamF (DSP_ROTXY);
		spm.scanp.rotmyx = GetParamF (DSP_ROTYX);
		spm.scanp.rotmyy = GetParamF (DSP_ROTYY);
		XYflt  = GetParamF (DSP_ROTOFFX);
		spm.scanp.MV_Xnew = (XYflt>maxval?maxval:(XYflt<minval?minval:XYflt)); 
		spm.scanp.MV_Xnew -= spm.scanp.rotoffx;
		XYflt   = GetParamF (DSP_ROTOFFY);
		spm.scanp.MV_Ynew = (XYflt>maxval?maxval:(XYflt<minval?minval:XYflt));
		spm.scanp.MV_Ynew -= spm.scanp.rotoffy;
		
		spm.LastSPMMode = spm.SPMMode; /* [12.04.1996] PZ: keine anderen Komandos zulassen !! */
		spm.SPMMode = (spm.SPMMode & ~(MD_SCAN | MD_CMD)) | MD_MOVEOFF;
		/* in Move-To-Offset-Modus übergehen */
		/* DSPack: Acknowledge erst, wenn MoveTo beendet !! */		
		break;
    
	case DSP_CMD_MOVETO_X:
		/* Moveto bezüglich Origin = rotoffx,y ! */
		spm.scanp.MV_Xnew = GetParamI (DSP_MOVETOX);
		spm.LastSPMMode = spm.SPMMode; /* [12.04.1996] PZ: keine anderen Komandos zulassen !! */
		spm.SPMMode = (spm.SPMMode & ~(MD_SCAN | MD_CMD)) | MD_MOVE;
		/* in Move-Modus übergehen */
		spm.scanp.MV_XPos = spm.scanp.MV_Xnew;
		/* DSPack: Acknowledge erst, wenn MoveTo beendet !! */
		DSPack;
		break;
    
	case DSP_CMD_MOVETO_Y:
		/* Moveto bezüglich Origin = rotoffx,y ! */
		spm.scanp.MV_Ynew = GetParamI (DSP_MOVETOY);
		spm.LastSPMMode = spm.SPMMode; /* [12.04.1996] PZ: keine anderen Komandos zulassen !! */
		spm.SPMMode = (spm.SPMMode & ~(MD_SCAN | MD_CMD)) | MD_MOVE;
		/* in Move-Modus übergehen */
		spm.scanp.MV_YPos = spm.scanp.MV_Ynew;
		/* DSPack: Acknowledge erst, wenn MoveTo beendet !! */    
		DSPack;
		break;
		
	case DSP_CMD_MOVETO_PARAM:
		
		spm.scanp.MV_stepsize = GetParamI (DSP_MVSTEPSZ);
		spm.scanp.MV_nRegel   = GetParamI (DSP_MVNREGEL);
		DSPack;
		break;
		
	case DSP_CMD_HALT:			/* Regler stoppen ! */
		LCDclear();
		LCDprintf(" FB off");
		spm.SPMMode &= ~MD_PID;
		DSPack;
		break;
		
	case DSP_CMD_START:		/* Regler starten ! */
		LCDclear();
		LCDprintf(" FB on");
		if(!(spm.SPMMode & MD_PID)) spm.SPMMode |= MD_PID;
		DSPack;
		break;
		
	case DSP_CMD_SWAPDPRAM:
		DSPack;
		break;
		
	case DSP_CMD_LINESCAN:
		
		spm.scanp.LS_nx2scan = GetParamI (DSP_LSNX);
		if( spm.scanp.LS_nx2scan > MAXSCANPOINTS ) spm.scanp.LS_nx2scan = MAXSCANPOINTS;
		if( spm.scanp.LS_nx2scan & 1 ) ++spm.scanp.LS_nx2scan; /* Gerade Anzahl (nur in 32bit DPRAM Mode wichtig ! */
		spm.scanp.LS_dnx     = GetParamI (DSP_LSDNX);
		spm.scanp.LS_stepsize= GetParamI (DSP_LSSTEPSZ);
		spm.scanp.LS_nRegel  = GetParamI (DSP_LSNREGEL);
		spm.scanp.LS_nAve    = GetParamI (DSP_LSNAVE);
		spm.scanp.LS_IntAve  = GetParamI (DSP_LSINTAVE);
		spm.scanp.LS_nx_pre  = GetParamI (DSP_LSNXPRE);
		spm.scanp.LS_srcs    = GetParamI (DSP_LSSRCS);
		
		LCDclear();
		LCDprintf("LS dnx: %d %x ", spm.scanp.LS_dnx, spm.scanp.LS_srcs);
		
#define MSK_PID(X)  (1<<((X)&3))
#define MSK_MUXA(X) (1<<(((X)&3)+4))
#define MSK_MUXB(X) (1<<(((X)&3)+8))
#define MSK_AUX(X)  (1<<(((X)&3)+12))
		
#define CHECK_PID(N) if(spm.scanp.LS_srcs & MSK_PID(N)){ if(spm.scanp.LS_ChAnz<MAXCHANNELS) { spm.scanp.ChPtr[spm.scanp.LS_ChAnz]=&spm.scanp.U_z; spm.scanp.LS_ChPID=N; spm.scanp.LS_ChAnz++; }}

#define CHECK_SRCA(N) if(spm.scanp.LS_srcs & MSK_MUXA(N)){ if(spm.scanp.LS_ChAnz<MAXCHANNELS) { spm.scanp.ChPtr[spm.scanp.LS_ChAnz]=&spm.scanp.adc_values[N]; spm.scanp.LS_ChAnz++; }}

#define CHECK_SRCB(N) if(spm.scanp.LS_srcs & MSK_MUXB(N)){ if(spm.scanp.LS_ChAnz<MAXCHANNELS) { spm.scanp.ChPtr[spm.scanp.LS_ChAnz]=&spm.scanp.adc_values[N+4]; spm.scanp.LS_ChAnz++; }}

#define CHECK_AUX(N) if(spm.scanp.LS_srcs & MSK_AUX(N)){ if(spm.scanp.LS_ChAnz<MAXCHANNELS) { spm.scanp.ChPtr[spm.scanp.LS_ChAnz]=&spm.scanp.adc_values[N+8]; spm.scanp.LS_ChAnz++; }}

		// setup channel-table
		spm.scanp.LS_ChAnz=0; spm.scanp.LS_ChAkt=0;
		// PID-(Z)Wert (no AD Port), PID Src (I,F,dF,..)
		CHECK_PID(0)
		else CHECK_PID(1)
		else CHECK_PID(2)
		else CHECK_PID(3)
		else { spm.scanp.LS_ChPID=-1; }
		
		// AD SRC "A" Ch0..3 
		CHECK_SRCA(0)
		CHECK_SRCA(1)
		CHECK_SRCA(2)
		CHECK_SRCA(3)
		
		// AD SRC "B" Ch0..3 
		CHECK_SRCB(0)
		CHECK_SRCB(1)
		CHECK_SRCB(2)
		CHECK_SRCB(3)
		
		// auxillary SRC "AUX", Soft. gen. data
		CHECK_AUX(0)
		CHECK_AUX(1)
		CHECK_AUX(2)
		CHECK_AUX(3)
		
		spm.scanp.LS_xnext = spm.scanp.MV_XPos;		/* nächste X Position zum Werteaufnahme */
		spm.scanp.LS_xE = spm.scanp.MV_XPos + (long)spm.scanp.LS_dnx*(long)(spm.scanp.LS_nx2scan + 2*spm.scanp.LS_nx_pre);	/* X-Endposition */
		spm.scanp.LS_SmpFlg = 0;		/* gleich ersten Me wert aufnehemen */
		spm.scanp.LS_AveCount = 0;		/* Mittelungszähler löschen */
		/* LS_BUFFER Index auf Start Wert 
		 * LS_Xinc = +1 : X+ Scan
		 * LS_Xinc = -1 : X- Scan
		 */
		if(spm.scanp.LS_dnx > 0){
			spm.scanp.LS_Xinc = 1;   /* Index increment */
			spm.scanp.LS_Xindex = -spm.scanp.LS_nx_pre; /* first Index */
		}
		else{
			spm.scanp.LS_Xinc = -1;   /* Index decrement */
			spm.scanp.LS_Xindex = spm.scanp.LS_nx2scan+spm.scanp.LS_nx_pre; /* last Index */
		}
		
		spm.LastSPMMode = spm.SPMMode;
		spm.SPMMode = (spm.SPMMode & ~(MD_SCAN | MD_CMD)) | MD_SCAN;
//		wakeup run_dspsim_loop;
		run_testscan();
		DSPack;
		/* DSPack: Acknowledge erst, wenn LineScan beendet !! */
		break;
		
	case DSP_CMD_PROBESCAN:
		
		spm.scanp.PRB_nx = GetParamI (DSP_PRBNX);
		if( spm.scanp.PRB_nx > MAXSCANPOINTS ) spm.scanp.PRB_nx = MAXSCANPOINTS;
		if( spm.scanp.PRB_nx & 1 ) ++spm.scanp.PRB_nx; /* Gerade Anzahl (nur in 32bit DPRAM Mode wichtig ! */
		spm.scanp.PRB_xS     = U2INT (GetParamF (DSP_PRBXS));
		spm.scanp.PRB_xE     = U2INT (GetParamF (DSP_PRBXE));
		spm.scanp.PRB_ACAmp  = U2INT (GetParamF (DSP_PRBACAMP));
		spm.scanp.PRB_ACFrq  = GetParamI (DSP_PRBACFRQ);
		spm.scanp.PRB_ACPhase= GetParamI (DSP_PRBACPHASE);
		spm.scanp.PRB_GapAdj = GetParamI (DSP_PRBGAPADJ);
		spm.scanp.PRB_delay  = GetParamI (DSP_PRBDELAY);
		spm.scanp.PRB_CIval  = GetParamF (DSP_PRBCIVAL);
		spm.scanp.PRB_srcs   = GetParamI (DSP_PRBSRCS);
		spm.scanp.PRB_outp   = GetParamI (DSP_PRBOUTP);
		spm.scanp.PRB_nAve   = GetParamI (DSP_PRBNAVE);
		
		LCDclear();
		LCDprintf("PRB nx: %d %04x ", spm.scanp.PRB_nx, spm.scanp.PRB_srcs);
#define MSK_PID(X)  (1<<((X)&3))
#define MSK_MUXA(X) (1<<(((X)&3)+4))
#define MSK_MUXB(X) (1<<(((X)&3)+8))
#define MSK_OUTP(X)  (1<<(((X)&3)+12))
		
#define CHECK_PRBSRCA(N) if(spm.scanp.PRB_srcs & MSK_MUXA(N)){ if(spm.scanp.PRB_ChAnz<MAXCHANNELS) { spm.scanp.ChPtr[spm.scanp.PRB_ChAnz]=&spm.scanp.adc_values[N]; spm.scanp.PRB_ChAnz++; }}

		/* setup channel-table
		   
		Einstellung der Meßwertaufnahme nach demselben Prinzip wie beim linescan:
		PRB_srcs ist eine Bitmaske für die einzelnen Wandler.  In den 16Bit sind mit je
		4Bit kodiert: PIDsrc, MuxA, MuxB, Aux
		Ist z.B. das 6. Bit gesetzt wird der 2. Kanal von MuxA aufgenommen.
		*/
		spm.scanp.PRB_ChAnz=1; spm.scanp.PRB_ChAkt=0;
		
		// PCI32: AD Ch0..3 
		CHECK_PRBSRCA(0)
		CHECK_PRBSRCA(1)
		CHECK_PRBSRCA(2)
		CHECK_PRBSRCA(3)
		
		spm.scanp.PRB_dnx = ((float)spm.scanp.PRB_xE - (float)spm.scanp.PRB_xS) / (float)spm.scanp.PRB_nx;  /* Schrittweite berechnen */
		spm.scanp.PRB_Xindex = 0;
		
		spm.scanp.PRB_AveCount = 0;	        /* Mittelungszähler zurücksetzen */
		{ 
			int i;
			for(i=0; i<spm.scanp.PRB_ChAnz; ++i)  /* Kanalsummen zurücksetzen */
				spm.scanp.PRB_ChSum[i] = 0.;
		}
		spm.LastSPMMode = spm.SPMMode;
		
		/* Regler adjust, first store old values */
		if(spm.scanp.PRB_CIval >= 0.){
			spm.scanp.PRB_CPIS[0] = spm.scanp.Const_I;
			spm.scanp.PRB_CPIS[1] = spm.scanp.Const_P;
			spm.scanp.PRB_CPIS[2] = spm.scanp.Const_S;
			spm.scanp.Const_S = spm.scanp.Const_P = 0.; 
			spm.scanp.Const_I = spm.scanp.PRB_CIval; // disable/slowdown Feedback by this !
		}
		
		spm.SPMMode = (spm.SPMMode & ~(MD_PROBE | MD_CMD)) | MD_PROBE;
		
		spm.scanp.dacbufferZ = ZSIGNUM((int)(spm.scanp.U_z));
		spm.scanp.PRB_oldU = spm.scanp.dacbufferU;
		spm.scanp.PRB_oldZ = spm.scanp.dacbufferZ;
		
		/* Startposition anfahren */
		spm.scanp.PRB_XPos = spm.scanp.PRB_xS;
		switch(spm.scanp.PRB_outp){
		case 0: DACoutX((long)spm.scanp.PRB_XPos); break;
		case 1: DACoutY((long)spm.scanp.PRB_XPos); break;
		case 2: DACoutZ((long)spm.scanp.PRB_XPos); break;
		case 3: DACoutU((long)spm.scanp.PRB_XPos); break;
		}
		
		/* DSPack: Acknowledge erst, wenn ProbeScan beendet !! */		
		run_testprbscan();
		
		DSPack;
		break;  /* Ende Probe Scan */
		
		
		
	case DSP_CMD_APPROCH_PARAM:
		
		spm.scanp.TIP_nSteps = GetParamI (DSP_TIPNSTEPS);
		spm.scanp.TIP_nWarte = GetParamI (DSP_TIPNWARTE)/REGEL_DT;
		spm.scanp.TIP_DUz    = GetParamI (DSP_TIPDUZ);
		spm.scanp.TIP_DUzRev = GetParamI (DSP_TIPDUZREV);
		DSPack;
		break;
		
#ifdef AFM_MOVER
	case DSP_CMD_AFM_SLIDER_PARAM:
		
		spm.scanp.afm_piezo_amp   = (long) U2INT (GetParamF (DSP_AFM_SLIDER_AMP));
		spm.scanp.afm_u_piezomax  = spm.scanp.afm_piezo_amp/2;
		spm.scanp.afm_piezo_speed = GetParamI (DSP_AFM_SLIDER_SPEED);
		spm.scanp.afm_piezo_steps = GetParamI (DSP_AFM_SLIDER_STEPS);
		LCDclear();
		LCDprintf(" Slider #: %ld",afm_piezo_steps);
		DSPack;
		break;
		
	case DSP_CMD_AFM_MOV_XP:
		spm.scanp.AFM_MV_count = 0L;
		spm.scanp.AFM_MV_Scount = 0L;
# ifdef USE_MOVER_APP
		spm.scanp.afm_mover_mode = AFM_MOV_XP;
# else
#  ifdef CARD_PCI32
#   ifdef TYP_STM
		spm.scanp.afm_mover_mode = AFM_MOV_XP;
#   else
		spm.scanp.afm_mover_mode = AFM_MOV_QP;
#   endif
#  else
		spm.scanp.afm_mover_mode = AFM_MOV_XP;
#  endif
# endif
		spm.LastSPMMode = spm.SPMMode; /* [12.04.1996] PZ: keine anderen Komandos zulassen !! */
		spm.SPMMode = (spm.SPMMode & ~(MD_CMD | MD_PID)) | MD__AFMADJ;
		DSPack;
		break;  
	case DSP_CMD_AFM_MOV_XM:
		spm.scanp.AFM_MV_count = 0L;
		spm.scanp.AFM_MV_Scount = 0L;
# ifdef USE_MOVER_APP
		spm.scanp.afm_mover_mode = AFM_MOV_XM;
# else
#  ifdef CARD_PCI32
#   ifdef TYP_STM
		spm.scanp.afm_mover_mode = AFM_MOV_XM;
#   else
		spm.scanp.afm_mover_mode = AFM_MOV_QM;
#   endif
#  else
		spm.scanp.afm_mover_mode = AFM_MOV_XM;
#  endif
# endif
		spm.LastSPMMode = spm.SPMMode; /* [12.04.1996] PZ: keine anderen Komandos zulassen !! */
		spm.SPMMode = (spm.SPMMode & ~(MD_CMD | MD_PID)) | MD__AFMADJ;
		DSPack;
		break;  
	case DSP_CMD_AFM_MOV_YP:
		spm.scanp.AFM_MV_count = 0L;
		spm.scanp.AFM_MV_Scount = 0L;
		spm.scanp.afm_mover_mode = AFM_MOV_YP;
		spm.LastSPMMode = spm.SPMMode; /* [12.04.1996] PZ: keine anderen Komandos zulassen !! */
		spm.SPMMode = (spm.SPMMode & ~(MD_CMD | MD_PID)) | MD__AFMADJ;
		DSPack;
		break;
	case DSP_CMD_AFM_MOV_YM:
		spm.scanp.AFM_MV_count = 0L;
		spm.scanp.AFM_MV_Scount = 0L;
		spm.scanp.afm_mover_mode = AFM_MOV_YM;
		spm.LastSPMMode = spm.SPMMode; /* [12.04.1996] PZ: keine anderen Komandos zulassen !! */
		spm.SPMMode = (spm.SPMMode & ~(MD_CMD | MD_PID)) | MD__AFMADJ;
		DSPack;
		break;
#endif    
		
	case DSP_CMD_APPROCH: /* start Auto Approch with Z Slider */
		LCDclear();
		LCDprintf(" Auto App Go");
#ifdef AFM_MOVER
		spm.scanp.afm_mover_app=0;
#endif
		
#ifdef TYP_STM
		if(! (spm.SPMMode & MD_SCAN || spm.SPMMode & MD_MOVE 
		      || spm.SPMMode & MD_TIPDN || spm.SPMMode & MD_ITU))
#else
			if(! (SPMMode & MD_SCAN || spm.SPMMode & MD_MOVE 
			      || spm.SPMMode & MD_TIPDN))
#endif
			{
				SLIDERPORT(SL_OFF);
				SLIDERTRIGGER_OFF;	/* Trigger=0 */
				ZSLIDER_ON;       	/* Z-- Aktivieren */
				spm.scanp.TIP_Mode = TIP_ZPiezoMax;
				spm.scanp.TIP_Zdn = 1;
				spm.SPMMode |= MD_TIPDN;
				/* 29.04.96 PZ: Regler wird jetzt verwendet */
			}
		DSPack;
		break;
		
	case DSP_CMD_APPROCH_MOV_XP: /* start Auto Approch with Besock Rotator */
#ifdef AFM_MOVER
		LCDclear();
		LCDprintf(" Auto AppXp Go");
		spm.scanp.afm_mover_app=1;
# ifdef TYP_STM
		if(! (spm.SPMMode & MD_SCAN || spm.SPMMode & MD_MOVE 
		      || spm.SPMMode & MD_TIPDN || spm.SPMMode & MD_ITU))
# else
			if(! (spm.SPMMode & MD_SCAN || spm.SPMMode & MD_MOVE 
			      || spm.SPMMode & MD_TIPDN))
# endif
			{
				spm.scanp.TIP_Mode = TIP_ZPiezoMax;
				spm.scanp.TIP_Zdn = 1;
				spm.SPMMode |= MD_TIPDN;
			}
#else
		LCDclear();
		LCDprintf(" no mover support !");
#endif
		DSPack;
		break;
		
	case DSP_CMD_CLR_PA: //clears all params e.g. reacts on pressing the STOP-button at the slider control window
#ifdef MD__AFMADJ
		spm.scanp.AFM_MV_Scount = spm.scanp.afm_piezo_steps;
#endif
		LCDclear();
		LCDprintf(" clear all pa");
		
		//LCDclear();
		//LCDprintf(" stop app!");
		spm.SPMMode &= ~MD_TIPDN;
		ZSLIDER_OFF;
		SLIDERPORT(SL_OFF);
		
		/* XXXXX */
		//for(i=10; i--;)     /* little delay to avoid trigger low while z-- aktive ... */
		//  ZSLIDER_OFF;   	/* Z-- release */
		
		SLIDERTRIGGER_OFF;	        /* Trigger high: no further approach of slider;  O.Brunke 05.01.01 */ 
		
		DSPack;
		break;
		
	case DSP_CMD_ALL_PA:  //Was passiert hier?? Percy fragen!!!
		
		LCDclear();
		LCDprintf(" stop clear");
		spm.SPMMode &= ~MD_TIPDN;
		SLIDERPORT(SL_OFF);
		DSPack;
		break;
		
// report some DSP software information
	case DSP_CMD_GETINFO:  /* is the same as the old "DSP_CMD_READY" Dummy-Komando */
		sprintf ((char*)BUFFERL, 
			 "*123-567-9abXdefS   M   -   D   S   P   -   S   O   F   T   -   I   N   F   O   -   -   *xxx\n"
			 "*--KERNEL-DSP-SPM-EMU-SOFT-INFO--*\n"
			 "XSM-EMU-Version: >1.5\n"
			 "CVS-Version: >1.5\n"
			 "StartMessage: " TITLE "\n"
			 "SRAM: 0 kW -- only linux kernel limited\n"
			 "*--Features--*\n"
			 "INSTRUMENT-TYPE: SPM\n"
			 "SCAN: Yes\n"
			 "MOVE: Yes\n"
			 "PROBE: Yes\n"
			 "ACPROBE: No\n"
			 "*--PROBE-CONFIG--*\n"
			 "ProbeDataMode: short\n"
			 "*--EOF--*\n"
			 "@@@@END@@@@\n"
			 );
		DSPack;
		break;
	default: 
		LCDclear();
		LCDprintf("unkownd CMD=0x%2x", *CMD_BUFFER & 0xff);
		break;
	}
	REQD_DSP = FALSE;
}

void calc_xy( void ){
	long valuexi,valueyi;
	float valuex,valuey;
	
	valuex= (float) spm.scanp.MV_XPos;
	valuey= (float) spm.scanp.MV_YPos;
  
	valuexi= (long)(valuex*spm.scanp.rotmxx + valuey*spm.scanp.rotmxy) 
		- spm.scanp.rotoffx;
	valueyi= (long)(valuex*spm.scanp.rotmyx + valuey*spm.scanp.rotmyy) 
		- spm.scanp.rotoffy;

	/* Range check */
	if(valuexi > MAXDA_XVAL)
		valuexi = MAXDA_XVAL;
	else
		if(valuexi < -MAXDA_XVAL)
			valuexi = -MAXDA_XVAL;
	
	if(valueyi > MAXDA_YVAL)
		valueyi = MAXDA_YVAL;
	else
		if(valueyi < -MAXDA_YVAL)
			valueyi = -MAXDA_YVAL;

	DACoutXY(valuexi, valueyi);
}

void DACoutXY(long ix, long iy){
	spm.scanp.dacXval = ix;
	spm.scanp.dacYval = iy;
}

void DACoutX(long value){
	spm.scanp.dacXval = value;
}
void DACoutY(long value){
	spm.scanp.dacYval = value;
}

void DACoutZ(long value){
	spm.scanp.dacZval = value;
}
void DACoutU(long value){
	spm.scanp.dacUval = value;
}



void run_testscan( void ){
	int n,i;
	for(n=0; n<spm.scanp.LS_nx2scan; n++){
		calc_xy ();
		for (i=0; i<spm.scanp.LS_ChAnz; ++i)
			*(BUFFERS + i*spm.scanp.LS_nx2scan + n ) = n/10
				+ spm.scanp.U_tunnel * i
				+ (spm.scanp.LS_dnx > 0 ? 1:-1)
				* (   surftab[(32767+spm.scanp.dacXval) % SURFSIZE]
				      * surftab[(32767+spm.scanp.dacYval) % SURFSIZE] );
		spm.scanp.MV_XPos += spm.scanp.LS_dnx;
	}
}

void run_testprbscan( void ){
	int n,i,sv;
	float pv, dv;
	calc_xy ();
	
	sv = surftab[(32767+spm.scanp.dacXval) % SURFSIZE]
		* surftab[(32767+spm.scanp.dacYval) % SURFSIZE];

	pv = spm.scanp.PRB_xS;
	dv = (spm.scanp.PRB_xE - spm.scanp.PRB_xS)/spm.scanp.PRB_nx;
	for (n=0; n<spm.scanp.PRB_nx; ++n, pv += dv)
		for (i=0; i<spm.scanp.LS_ChAnz; ++i)
			*(BUFFERS + n) = (int) (pv) + sv;
}

void run_dspsim_loop( void ){
	for(;;){
		if(0){
		}
		else break;
	}
}
