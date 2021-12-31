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
#include "/home/czubanowski/Gxsm/pci32/dspC32/spa/spacmd.h"


#define BB_TEST_CNT       // Test ChanneltronCounting on startup 0.1-100ms
#define BB_POLLING_MODE   // use polling Mode for Gate, else use IRQ


// Burr Brow Configuration:
#define  BB_MEM_BASE 0xcd000

#define  BB_MEM_AUTOPROBE  0
#define  BB_MEM_BASE_START 0xc0000
#define  BB_MEM_BASE_END   0xe0000
#define  BB_MEM_SIZE 0x400
#define  BB_IRQLINE  5

#define  BB_XY_DEFLECTON_BOARD 3
#define  BB_ENERGY_BOARD 1


// BB Id's
#define  BB_PCI20001C1         (0x1+0x2+0x8+0x10)
#define  BB_PCI20001C2         (0x1+0x4+0x8+0x10)
#define  BB_IDMSK              0x1f

#define  BB_PCI20006M2         0xe2 // Analog Output 16bit single channel DAC
#define  BB_PCI20006M3         0xe3 // Analog Output 16bit two channel DAC
#define  BB_PCI20007M          0xea // Counter, Timer, Puls

#define  BB_BASE_MODULE(N)     ( spa[BB_CARD_NO].BB_membase + (((N)&3)<<8) )
// carrier register
#define  BB_MODULE_PRESENT(M)  (readb( BB_BASE_MODULE(0) ) & (1<<(8-(M))))
#define  BB_IRQ_STATUS         readb( BB_BASE_MODULE(0) + 0x40 )

// PCI-20006M 16-Bit DAC Module register offsets
// N=3 für X,Y
// N=1 für X,E
// BB 16bit DACs module on position N
#define  BB_DAC_X(X,N)         writew( (X), BB_BASE_MODULE(N) + 0x0d)
#define  BB_DAC_Y(X,N)         writew( (X), BB_BASE_MODULE(N) + 0x15)
#define  BB_DAC_OUT(N)         writeb(0xff, BB_BASE_MODULE(N) + 0x1b)

// BB Timer/Counter module on position 2
#define  BB_TMR_MODULE_ID      2
// PCI-20006M 16-Bit DAC Module register offsets
#define  BB_TMR_BASE           ( BB_BASE_MODULE(BB_TMR_MODULE_ID) )
#define  BB_TMR_RTGREG         ( BB_TMR_BASE + 0x04 )  // BB TimerCounter Module Base Address
#define  BB_TMR_RTGREG_LO      ( BB_TMR_RTGREG )       // 0x04 Rate Generator count Reg. low 16, (rw)
#define  BB_TMR_RTGREG_HI      ( BB_TMR_RTGREG + 1)    // 0x05 Rate Generator count Reg. hi 16, (rw)

#define  BB_TMR_CNT0REG        ( BB_TMR_BASE + 0x08 )  // Counter 0 count register (rw)
#define  BB_TMR_CNT1REG        ( BB_TMR_BASE + 0x09 )  // Counter 1 count register (rw)  
#define  BB_TMR_CNT2REG        ( BB_TMR_BASE + 0x0A )  // Counter 2 count register (rw)  
#define  BB_TMR_CNT3REG        ( BB_TMR_BASE + 0x06 )  // Counter 3 count register (rw)  

#define  BB_TMR_RTGCNT3CTRL    ( BB_TMR_BASE + 0x07 )  // Rate Generator and Counter 3 control register (w)
#define  BB_TMR_CNT012CTRL     ( BB_TMR_BASE + 0x0B )  // Counter 0,1,2 control register (w)
#define  BB_TMR_CNTGATECTRL    ( BB_TMR_BASE + 0x0C )  // Counter gate control (w)

#define  BB_CRTL_MSK           0x40

typedef struct{
    short lb0;
    short hb0;
    short lb1;
    short hb1;
} BB_GATEGEN;

#ifdef BB_TEST_CNT
static int irq_count=0;
static int timeout_count=0;
#endif



/* Konstanten für DSP  */
#define AD_MAX_VOLT 10.     /* Max Spannung bei Wert DA_MAX_VAL */
#define DA_MAX_VOLT 10.     /* Max Spannung bei Wert DA_MAX_VAL */
#define DA_MAX_VAL  0x7ffe  /* Max Wert für signed 16 Bit Wandler */
#define UDA_MAX_VAL  0xffff  /* Max Wert für unsigend 16 Bit Wandler */

/* Spannung <==> Int-Wert Umrechenfaktoren */
/* Bipolar */
#define         U2FLT(X) ((X)*(float)(DA_MAX_VAL)/AD_MAX_VOLT)
#define         U2INT(X) (int)((X)*(float)(DA_MAX_VAL)/AD_MAX_VOLT)
#define         INT2U(X) ((float)(X)/DA_MAX_VAL*AD_MAX_VOLT)

/* Uinpolar */
#define         UNI_U2FLT(X) ((X)*(float)(UDA_MAX_VAL)/AD_MAX_VOLT)
#define         UNI_U2INT(X) (int)((X)*(float)(UDA_MAX_VAL)/AD_MAX_VOLT)
#define         UNI_INT2U(X) ((float)(X)/UDA_MAX_VAL*AD_MAX_VOLT)


#define  DPRAMBASE     (volatile int*)  (spa[BB_CARD_NO].dsp->virtual_dpram)

#define  CMD_BUFFER    (volatile int*)  (DPRAMBASE+0x00)   /* cmd buffer */
#define  CMD_PARM      (volatile int*)  (DPRAMBASE+0x01)   /* */
#define  BUFFER        (volatile int*)  (DPRAMBASE+DSP_BUFFER_START)   /* */
#define  BUFFERL       (volatile unsigned long*)  (DPRAMBASE+DSP_BUFFER_START)   /* */
#define  DPRAML        (volatile unsigned long*)  (DPRAMBASE)   /* */
#define  LCDBUFFER     (volatile unsigned long*)  (DPRAMBASE+DSP_LCDBUFFER)
#define  MAXSCANPOINTS (DSP_DATA_REG_LEN)

#define  DSPack  spa[BB_CARD_NO].dsp->SrvReqAck=TRUE

#define MD_CMD          0x08    /* Komando (SRQ) abfragen, ausführen aktiv */
#define MD_SCAN         0x10    /* Beamfinder aktiv */
#define MD_BLK          0x80    /* Blinken zur Kontrolle */

#define LEDPORT(X)       *((unsigned long*)(DPRAMBASE+DSP_USR_DIO))=X

void LCDclear(void);
int LCDprintf(const char *format, ...);

void scan2d(void);
void linescan(int n, float y);

unsigned long ChanneltronCounts(float x, float y);

int GetParamI(unsigned int N);
float GetParamF(unsigned int N);

void BB_SetVolt(double x, double y);
void BB_SetEnergy(double En);
unsigned long BB_CntRead(void);
double BB_InitCnt(double gate);
void BB_SetCnt(void);
int BB_ReadyCnt(void);

static void bb_gate_interrupt(int irq, void *dev_id, struct pt_regs *regs);

#define MAX_BB_CARDS 1
#define BB_CARD_NO 0

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

typedef struct{
    struct dspsim_thread_data *dsp;
    SCANP scanp;

    BB_GATEGEN bb_gate_reg;

    char *BB_membase;
    unsigned char bbid;
    unsigned char bbmodid[3];

    int    major, minor;
    int    bb_irq;
    struct wait_queue *waitq;
    struct timer_list timeout_tmr;
    unsigned long bb_timeout_flg;

    int LastSPAMode;
    int SPAMode;

    double extractor;
    double chanhv;
    double chanrepeller;
    double cryfocus;
    double filament, expfilament;
    double gunfocus;
    double gunanode;
    double smpdist;
    double smptemp;
    double S,d,ctheta,sens;
    double growing;
} SPALEED;

static SPALEED spa[MAX_BB_CARDS];


#ifdef BB_TEST_CNT
void bb_test_counting(double E, double ms){
   unsigned long n;
   BB_SetEnergy(E);
   ms=BB_InitCnt(ms);
   spa[BB_CARD_NO].scanp.ms  = ms;
   KDEBUG("BB_SPA: irqcount %d, tmoutcount: %d\n", irq_count, timeout_count);
   n=jiffies;
   KDEBUG("BB_SPA: Testing,   %d/10ms, Cnt(0,0)=%d\n",(int)(ms*10.), (int)ChanneltronCounts(0., 0.));
   n=jiffies-n;
   KDEBUG("BB_SPA: irqcount %d, tmoutcount: %d jiffies: %d\n", irq_count, timeout_count, (int)n);
}
#endif

int InitEmu(struct dspsim_thread_data *dsp){
    int err=0;
    int i;
    // BB init

    KDEBUG("BurrBrow PCI20001CX module\nBB auto detection: %d\n", BB_MEM_AUTOPROBE );

    spa[BB_CARD_NO].dsp   = dsp;
    spa[BB_CARD_NO].major = pcdsp_major;
    spa[BB_CARD_NO].minor = BB_CARD_NO;

    spa[BB_CARD_NO].BB_membase = (unsigned char*)BB_MEM_BASE;
    
    switch( spa[BB_CARD_NO].bbid = (readb( BB_BASE_MODULE(0) ) & BB_IDMSK) ){
    case BB_PCI20001C1: KDEBUG("BB carrier PCI20001C1 found\n"); break;
    case BB_PCI20001C2: KDEBUG("BB carrier PCI20001C2 found\n"); break;
    default: KDEBUG("no BB found at %x, id=%x. Trying to autodetect.\n", 
		    (int)spa[BB_CARD_NO].BB_membase, spa[BB_CARD_NO].bbid); err=1; break;
    }

    if( err && BB_MEM_AUTOPROBE )
	for(spa[BB_CARD_NO].BB_membase = (unsigned char*)BB_MEM_BASE_START;
	    err && (unsigned long)(spa[BB_CARD_NO].BB_membase) < BB_MEM_BASE_END; spa[BB_CARD_NO].BB_membase += BB_MEM_SIZE)
	    switch( spa[BB_CARD_NO].bbid = (readb( BB_BASE_MODULE(0) ) & BB_IDMSK) ){
	    case BB_PCI20001C1: KDEBUG("BB carrier PCI20001C1 found\n"); err=0; break;
	    case BB_PCI20001C2: KDEBUG("BB carrier PCI20001C2 found\n"); err=0; break;
	    default: continue;
	    }

    if(err){
	spa[BB_CARD_NO].BB_membase = (unsigned char*)BB_MEM_BASE;
	KDEBUG("Error: BB SPA-LEED module can't find BB card.\n" );
	return 1;
    }
    else
	KDEBUG("BB SPA-LEED module, BB baseaddr: %x\n", (int)spa[BB_CARD_NO].BB_membase );
				     
    for(i=0; i<3; ++i){
	spa[BB_CARD_NO].bbmodid[i]=0;
	if( BB_MODULE_PRESENT(i+1) == 0 )
	    switch( (spa[BB_CARD_NO].bbmodid[i] = readb( BB_BASE_MODULE(i+1) )) ){
	    case BB_PCI20006M2: KDEBUG("BB module %d is a PCI20006M, single DAC 16bit\n", i+1); break;
	    case BB_PCI20006M3: KDEBUG("BB module %d is a PCI20006M, two DAC 16bit\n", i+1); break;
	    case BB_PCI20007M:  KDEBUG("BB module %d is a PCI20007M, Counter, Timer, Puls\n", i+1); break;
	    default: KDEBUG("unknown BB module with id=%x found\n", spa[BB_CARD_NO].bbmodid[i]); break;
	    }
	else
	    KDEBUG("BB module is %d not present.\n", i+1);
    }

    spa[BB_CARD_NO].waitq  = NULL;
    spa[BB_CARD_NO].bb_irq = request_irq(BB_IRQLINE, 
					 bb_gate_interrupt, 
					 0,
					 PCDSP_DEVICE_NAME,
					 NULL );
//					 &spa[BB_CARD_NO] );

    KDEBUG("requested irq is %d, id=%d\n", BB_IRQLINE, spa[BB_CARD_NO].bb_irq );
    KDEBUG("BB Irq Status is: %x\n", BB_IRQ_STATUS );
    KDEBUG("BB detection OK.\n" );

    // DSP-EMU init
    *CMD_BUFFER=0;
    spa[BB_CARD_NO].LastSPAMode = spa[BB_CARD_NO].SPAMode = MD_CMD;
    LCDclear();
    LCDprintf("-* DSP Emu *-");
    LEDPORT(spa[BB_CARD_NO].SPAMode);  

    spa[BB_CARD_NO].scanp.rotmxx = spa[BB_CARD_NO].scanp.rotmyy = 1.;
    spa[BB_CARD_NO].scanp.rotmxy = spa[BB_CARD_NO].scanp.rotmyx 
	= spa[BB_CARD_NO].scanp.rotoffx = spa[BB_CARD_NO].scanp.rotoffy = 0.;

    spa[BB_CARD_NO].scanp.ms  = 1.;
    spa[BB_CARD_NO].scanp.X0  = spa[BB_CARD_NO].scanp.Y0 = 0.;
    spa[BB_CARD_NO].scanp.Nx  = spa[BB_CARD_NO].scanp.Ny = 40;
    spa[BB_CARD_NO].scanp.N   = 100;
    spa[BB_CARD_NO].scanp.len = 100.;
    spa[BB_CARD_NO].scanp.lenxy = 100.;
    spa[BB_CARD_NO].scanp.E   = 1.;

    spa[BB_CARD_NO].scanp.alpha = spa[BB_CARD_NO].scanp.alphaAlt = 0.;

    spa[BB_CARD_NO].extractor = 0.5;
    spa[BB_CARD_NO].chanhv    = 2000.0;
    spa[BB_CARD_NO].chanrepeller = 1.0;
    spa[BB_CARD_NO].cryfocus  = 0.5;
    spa[BB_CARD_NO].filament  = 2.4;
    spa[BB_CARD_NO].gunfocus  = 0.5;
    spa[BB_CARD_NO].gunanode  = 1.1;
    spa[BB_CARD_NO].smpdist   = 20.;
    spa[BB_CARD_NO].smptemp   = 300.;
    spa[BB_CARD_NO].sens      = 100.;
    spa[BB_CARD_NO].d         = 3.141;
    spa[BB_CARD_NO].ctheta    = 0.996155128; /*cos(5./57.);*/
    spa[BB_CARD_NO].S         = 1.;
    spa[BB_CARD_NO].growing   = 0.;

#ifdef BB_TEST_CNT
    bb_test_counting(1., 0.1);
    bb_test_counting(1., 0.5);
    bb_test_counting(1., 1.0);
    bb_test_counting(1., 5.0);
    bb_test_counting(1., 10.0);
    bb_test_counting(1., 50.0);
    bb_test_counting(1., 100.0);
    bb_test_counting(1., 500.0);
    spa[BB_CARD_NO].scanp.ms  = 1.;
#endif

   return 0;
}

void ExitEmu(void){
    KDEBUG("BB cleanup\n");
#ifdef BB_TEST_CNT
    KDEBUG("BB_SPA: irqcount %d, tmoutcount: %d\n", irq_count, timeout_count);
#endif
    free_irq(BB_IRQLINE, NULL);
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
  switch(*CMD_BUFFER & 0xff){

  case DSP_CMD_SCAN_PARAM:
      spa[BB_CARD_NO].scanp.X0  = GetParamF(DSP_X0);
      spa[BB_CARD_NO].scanp.Y0  = GetParamF(DSP_Y0);
      spa[BB_CARD_NO].scanp.len = GetParamF(DSP_len);
      spa[BB_CARD_NO].scanp.N   = GetParamI(DSP_N);
      spa[BB_CARD_NO].scanp.alpha = GetParamF(DSP_alpha);
      spa[BB_CARD_NO].scanp.ms  = GetParamF(DSP_ms);
      spa[BB_CARD_NO].scanp.E   = GetParamF(DSP_E);

      spa[BB_CARD_NO].scanp.rotmxx = GetParamF(DSP_MXX);
      spa[BB_CARD_NO].scanp.rotmxy = GetParamF(DSP_MXY);
      spa[BB_CARD_NO].scanp.rotmyx = GetParamF(DSP_MYX);
      spa[BB_CARD_NO].scanp.rotmyy = GetParamF(DSP_MYY);
     
     KDEBUG("MXX %d\n", (int)(100*spa[BB_CARD_NO].scanp.rotmxx));
     KDEBUG("MYY %d\n", (int)(100*spa[BB_CARD_NO].scanp.rotmyy));

     /*
     if(spa[BB_CARD_NO].scanp.alpha != spa[BB_CARD_NO].scanp.alphaAlt){
	  spa[BB_CARD_NO].scanp.rotmyy = spa[BB_CARD_NO].scanp.rotmxx 
	      = cos((double)spa[BB_CARD_NO].scanp.alpha);
	  spa[BB_CARD_NO].scanp.rotmyx = -(spa[BB_CARD_NO].scanp.rotmxy
	      = sin((double)spa[BB_CARD_NO].scanp.alpha));
	  spa[BB_CARD_NO].scanp.alphaAlt = spa[BB_CARD_NO].scanp.alpha;
      }
      */
      
      spa[BB_CARD_NO].scanp.rotoffx = spa[BB_CARD_NO].scanp.X0;
      spa[BB_CARD_NO].scanp.rotoffy = spa[BB_CARD_NO].scanp.Y0;
      BB_SetVolt(0., 0.);
      BB_SetEnergy(spa[BB_CARD_NO].scanp.E);
      DSPack;
      break;

  case DSP_CMD_SPACTRL_SET:
      spa[BB_CARD_NO].extractor = GetParamF(DSP_SPACTRL_EXTRACTOR);
      spa[BB_CARD_NO].chanhv    = GetParamF(DSP_SPACTRL_CHANHV);
      spa[BB_CARD_NO].chanrepeller = GetParamF(DSP_SPACTRL_CHANREPELLER);
      spa[BB_CARD_NO].cryfocus  = GetParamF(DSP_SPACTRL_CRYFOCUS);
      spa[BB_CARD_NO].filament  = GetParamF(DSP_SPACTRL_FILAMENT);
      spa[BB_CARD_NO].gunfocus  = GetParamF(DSP_SPACTRL_GUNFOCUS);
      spa[BB_CARD_NO].gunanode  = GetParamF(DSP_SPACTRL_GUNANODE);
      spa[BB_CARD_NO].smpdist   = GetParamF(DSP_SPACTRL_SMPDIST);
      spa[BB_CARD_NO].smptemp   = GetParamF(DSP_SPACTRL_SMPTEMP);
      spa[BB_CARD_NO].growing   = GetParamF(DSP_SPACTRL_GROWING);
      DSPack;
      break;

  case DSP_CMD_SCAN_START:
      spa[BB_CARD_NO].scanp.len = GetParamF(DSP_len);
      spa[BB_CARD_NO].scanp.E   = GetParamF(DSP_E);
      
      spa[BB_CARD_NO].LastSPAMode = spa[BB_CARD_NO].SPAMode;
      spa[BB_CARD_NO].SPAMode = (spa[BB_CARD_NO].SPAMode & ~(MD_CMD)) | MD_SCAN;
      linescan(0, GetParamF(DSP_Y0));
      spa[BB_CARD_NO].SPAMode = spa[BB_CARD_NO].LastSPAMode;
      DSPack;
      break;

  case DSP_CMD_SWAPDPRAM:
      DSPack;
      break;

  case DSP_CMD_SCAN2D:
      spa[BB_CARD_NO].scanp.Nx  = GetParamI(DSP_NX);
      spa[BB_CARD_NO].scanp.Ny  = GetParamI(DSP_NY);
      spa[BB_CARD_NO].scanp.lenxy = GetParamF(DSP_LXY);
      spa[BB_CARD_NO].LastSPAMode = spa[BB_CARD_NO].SPAMode;
      spa[BB_CARD_NO].SPAMode = (spa[BB_CARD_NO].SPAMode & ~(MD_CMD)) | MD_SCAN;
      scan2d();
      spa[BB_CARD_NO].SPAMode = spa[BB_CARD_NO].LastSPAMode;
      DSPack;
      break;

  case DSP_CMD_GETCNT:
      spa[BB_CARD_NO].scanp.ms  = GetParamF(DSP_ms);
      spa[BB_CARD_NO].scanp.E   = GetParamF(DSP_E);

      spa[BB_CARD_NO].scanp.ms = BB_InitCnt(spa[BB_CARD_NO].scanp.ms);
      BB_SetEnergy(spa[BB_CARD_NO].scanp.E);

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
    if(spa[BB_CARD_NO].scanp.Nx > 1)
	Ux  = -spa[BB_CARD_NO].scanp.lenxy/2.;
    else
	Ux = 0.;
    dUx = spa[BB_CARD_NO].scanp.lenxy/spa[BB_CARD_NO].scanp.Nx;
    
    if(spa[BB_CARD_NO].scanp.Ny > 1)
    Uy = spa[BB_CARD_NO].scanp.lenxy/2.;
    else
	Uy = 0.;
    dUy = spa[BB_CARD_NO].scanp.lenxy/spa[BB_CARD_NO].scanp.Ny;
    
    spa[BB_CARD_NO].scanp.ms = BB_InitCnt(spa[BB_CARD_NO].scanp.ms);
    BB_SetEnergy(spa[BB_CARD_NO].scanp.E);

    for(y=Uy, k=j=0; j<spa[BB_CARD_NO].scanp.Ny; j++, y-=dUy){
	BB_SetVolt(Ux, y);
	mysleep(3);
	for(x=Ux, i=0; i<spa[BB_CARD_NO].scanp.Nx; i++, k++, x+=dUx){
	    if( k >= MAXSCANPOINTS )
		return;
	    *(BUFFERL + k) = ChanneltronCounts(x, y);
	}
    }
}

void linescan(int n, float y){
    float x, dx;
    int nmax;
    nmax = n + spa[BB_CARD_NO].scanp.N + 4;
    if( nmax > MAXSCANPOINTS )
	nmax = MAXSCANPOINTS;

    dx = spa[BB_CARD_NO].scanp.len/(float)spa[BB_CARD_NO].scanp.N;
    x = -spa[BB_CARD_NO].scanp.len/2.;

    spa[BB_CARD_NO].scanp.ms = BB_InitCnt(spa[BB_CARD_NO].scanp.ms);
    BB_SetEnergy(spa[BB_CARD_NO].scanp.E);
    BB_SetVolt(x, y);
    mysleep(3);
    for(; n<nmax; n++, x+=dx)
	*(BUFFERL + n) = ChanneltronCounts(x, y);
}

#ifndef BB_POLLING_MODE
static void bb_irq_timeout(unsigned long data){
    SPALEED *thisspa = &spa[data];
    ++thisspa->bb_timeout_flg;
    wake_up_interruptible( &thisspa->waitq );
#ifdef BB_TEST_CNT
    ++timeout_count;
#endif
}
#endif

unsigned long ChanneltronCounts(float x, float y){

#ifdef BB_POLLING_MODE
    // no IRQ is used
    static struct timer_list timeout_tmr = { NULL, NULL, 0, 0, timeout };

    BB_SetVolt(x, y);
    BB_SetCnt();

    if(spa[BB_CARD_NO].scanp.ms > 12.)
    	mysleep( (unsigned long) (spa[BB_CARD_NO].scanp.ms/10.) );
    else{
	wakeups=1000;
	while( BB_ReadyCnt() && --wakeups);
	if( ! BB_ReadyCnt() )
	    return BB_CntRead();
    }

    wakeups=0;
    while( BB_ReadyCnt() && wakeups < 100){
	del_timer(&timeout_tmr);
	timeout_tmr.expires = jiffies + 1;
	add_timer(&timeout_tmr);
	interruptible_sleep_on( &waitq );
	wakeups++;
    } 

#else
    // use IRQ

    BB_SetVolt(x, y);
    BB_SetCnt();

    // use bb_irq_timeout function to wakeup if irq is missed
    init_timer( &spa[BB_CARD_NO].timeout_tmr );
    spa[BB_CARD_NO].timeout_tmr.expires  = jiffies + 1 + (unsigned long)(spa[BB_CARD_NO].scanp.ms/10.) ;
    spa[BB_CARD_NO].timeout_tmr.data     = BB_CARD_NO;
    spa[BB_CARD_NO].timeout_tmr.function = bb_irq_timeout;
    spa[BB_CARD_NO].bb_timeout_flg       = 0;
    add_timer( &spa[BB_CARD_NO].timeout_tmr );

    sti();
    interruptible_sleep_on( &spa[BB_CARD_NO].waitq );
    cli();

    del_timer( &spa[BB_CARD_NO].timeout_tmr );
    
#endif
    if( ! BB_ReadyCnt() )
	return BB_CntRead();
    else
	return 13;
}

// BB implementation
// BurrBrown Stuff is following


void BB_SetVolt(double x, double y){
    double xt,yt;
    xt = x*spa[BB_CARD_NO].scanp.rotmxx + y*spa[BB_CARD_NO].scanp.rotmxy 
	+ spa[BB_CARD_NO].scanp.rotoffx;
    yt = x*spa[BB_CARD_NO].scanp.rotmyx + y*spa[BB_CARD_NO].scanp.rotmyy 
	+ spa[BB_CARD_NO].scanp.rotoffy;

    BB_DAC_X( (int)(xt), BB_XY_DEFLECTON_BOARD );
    BB_DAC_Y( (int)(yt), BB_XY_DEFLECTON_BOARD );
    BB_DAC_OUT( BB_XY_DEFLECTON_BOARD );
}


void BB_SetEnergy(double En){
    BB_DAC_X( U2INT(En), BB_ENERGY_BOARD );
    BB_DAC_Y( U2INT(En), BB_ENERGY_BOARD );
    BB_DAC_OUT( BB_ENERGY_BOARD );
}

unsigned long BB_CntRead(void){
    unsigned long lb0, hb0, lb1, hb1, cnt, stat0, stat1;
    writeb( 0xc6, BB_TMR_CNT012CTRL );
    cnt = 0;

    stat0 = readb( BB_TMR_CNT0REG ) & BB_CRTL_MSK;
    lb0 = 0xff - readb( BB_TMR_CNT0REG );
    hb0 = 0xff - readb( BB_TMR_CNT0REG );
    if(stat0 == 0) 
	cnt = (((hb0<<8) | lb0) + 1) << 16;

    stat1 = readb( BB_TMR_CNT1REG ) & BB_CRTL_MSK;
    lb1 = 0xff - readb( BB_TMR_CNT1REG );
    hb1 = 0xff - readb( BB_TMR_CNT1REG );
    if(stat1 == 0){
	cnt |= (hb1<<8) | lb1;
	++cnt;
    }
    
    return cnt;
}

double BB_InitCnt(double gate){
    // gate in ms
    long n0, n1;
    double factor;

    if( gate < 0.1 ) 
	factor = 1.;
    else 
	if( gate < 1. ) 
	    factor = 10.;
	else
	    if( gate < 10. )
		factor = 100.;
	    else
		if( gate < 100. )
		    factor = 1000.;
		else
		    factor = 10000.;

    n1 = (long)(2.*factor);
    n0 = (long)(4000.*gate/factor);

    if( n0 < 2 ){
	n0 = 2;
	gate = (double)n0 * (double)n1 / 8000.;
    }

    spa[BB_CARD_NO].bb_gate_reg.lb0 = (n0&0xff);
    spa[BB_CARD_NO].bb_gate_reg.hb0 = ((n0>>8)&0xff);
    spa[BB_CARD_NO].bb_gate_reg.lb1 = (n1&0xff);
    spa[BB_CARD_NO].bb_gate_reg.hb1 = ((n1>>8)&0xff);

    return gate;
}

void BB_SetCnt(void){
    writeb( 0x34, BB_TMR_CNT012CTRL );  // Counter 0 in mode 2
    writeb( 0xff, BB_TMR_CNT0REG );     // divides threw    65532
    writeb( 0xff, BB_TMR_CNT0REG );

    writeb( 0x74, BB_TMR_CNT012CTRL );  // Counter 1 in mode 2
    writeb( 0xff, BB_TMR_CNT1REG );     // divides threw    65532
    writeb( 0xff, BB_TMR_CNT1REG );

    writeb( 0x34, BB_TMR_RTGCNT3CTRL );          // rategenerator 0 in mode 2
    writeb( spa[BB_CARD_NO].bb_gate_reg.lb0, BB_TMR_RTGREG_LO ); // divides threw lb0*hb0
    writeb( spa[BB_CARD_NO].bb_gate_reg.hb0, BB_TMR_RTGREG_LO );

    writeb( 0x70, BB_TMR_RTGCNT3CTRL );          // rategenerator 1 in mode 0
    writeb( spa[BB_CARD_NO].bb_gate_reg.lb1, BB_TMR_RTGREG_HI ); // as interrupt on terminalcount
    writeb( spa[BB_CARD_NO].bb_gate_reg.hb1, BB_TMR_RTGREG_HI ); // after lb1rg*hb1rg periods

    writeb( 0xff, BB_TMR_CNTGATECTRL );          // softgate on, count started 
    // Softgate Stuff ... missing right now
    //    writeb( 0x00, BB_TMR_CNTGATECTRL );    // end of scans   Softgate off
}

int BB_ReadyCnt(void){
    writeb( 0xe4, BB_TMR_RTGCNT3CTRL );
    return ( (readb(BB_TMR_RTGREG_HI) & 0x80) == 0 ); // Gate = Low ?
}

// IRQ Handler
static void bb_gate_interrupt(int irq, void *dev_id, struct pt_regs *regs){
//    SPALEED *thisspa = (SPALEED *) dev_id;
    SPALEED *thisspa = &spa[BB_CARD_NO];
    wake_up_interruptible( &thisspa->waitq );
#ifdef BB_TEST_CNT
    ++irq_count;
#endif

}
