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

/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 8 c-style: "K&R" -*- */

#ifndef __G_DAT_TYPES_H
#define __G_DAT_TYPES_H

// "C" headers
#include <cstring>

// system headers
#include <unistd.h>




/* historical compatibility Stuff follows... */

#ifndef BOOL
  #define BOOL int
#endif
#ifndef FALSE
  #define FALSE 0
#endif
#ifndef TRUE
  #define TRUE 1
#endif


/******* globale Macros ***********/

#ifndef Min
#define Min(a,b) (((a)<(b))?((a)):((b)))
#endif

#ifndef Max
#define Max(a,b) (((a)>(b))?((a)):((b)))
#endif

#ifndef max
#define max(a,b) Max((a),(b))
#endif

#ifndef min
#define min(a,b) Min((a),(b))
#endif


typedef struct{
  short * data;
  long LineNum;
  long nx;
  long ny;
  long dx;
  long dy;
  int View;
} GRARGS;

/* für d2d Import */

/*
 * Strukturen zur Daten-Kompatibilität mit spa4.exe 
 * ==================================================
 */

typedef struct {
  double X,Y,X1,X2,Y1,Y2,dx,dy;        /* 0x0000 - 0x0040 */
  double Xo,Yo,GateTime,XYdist,CpsHigh;
  double CpsLow,Alpha;
  double usr1;                         /* 16*8 = 0x0080 */   
  unsigned char  CReg1,CReg2,CReg3,CReg4;
  unsigned char  CReg5,CReg6,CReg7,CReg8; /* +8 = 0x0088 */
  short  Scanmarker,Points;            /* + 4   = 0x008c */
  double Energy,Focus1,Current;
  double usr2,usr3,usr4;
  double focus2,extractor;
  double dummy3;                       /* +9*8  = 0x00d4  */
  char dummyAA, dummyBB;
  char dateofscan[20];
  char comment[100];
  char __space[100];
  /*  double scanfield[SIZE_D1D]; */          /* 0x015e: array[min1d..max1d] of double; ( -2 .. 2002 ) */
} D1D_SPA_DAT;

typedef struct {
  double X,Y,X1,X2,Y1,Y2,dx,dy;          // im prinzip frei
  double Xo,Yo,GateTime,XYdist,CpsHigh;  //
  double CpsLow,Alpha;                   //
  double usr1;                           // IFaktor,IntInt :frei
  unsigned char CReg1,CReg2,CReg3,CReg4; // BYTE
  unsigned char CReg5,CReg6,CReg7,CReg8; // BYTE
  double Energy,Focus1,Current;          // DOUBLE
  double usr2,usr3,usr4;                 // TotalInt,IntRadius,PeakBottom :frei
  double focus2,extractor;               //
  double dum6;                           //
  short  Scanmarker,PointsX,PointsY;     // Integer
  char   dateofscan[20];                 // String[20]
  char   comment[100];                   // String[100]
} D2D_SPA_DAT;


/**************************************************************************
* EXPLANATION of SCAN1D and SCAN2D IDENTIFIERS (defined at typedefs.tpu)  *
*                                                                         *
* scanfield[min1d..max1d] : measuring channels for 1-dim. measurements    *
*                           (only for scan1d)                             *
* X,Y                     : current sweep position for SetVolt            *
* X1,Y1 /  X2,Y2          : starting and end point of measurement         *
* dx,dy                   : width of measuring steps                      *
* Xo,Yo                   : zeropoint of measuring                        *
* GateTime                : GateTime for each channel measured            *
* XYdist                  : Length of Scan in VOLT (unit for all values)  *
* CpsHigh                 : maximum scaling border for intensity          *
* CpsLow                  : minimum scaling border for intensity          *
* Alpha                   : angle between scaling direction               *
*                           and direction of x-deflection                 *
* IFaktor                 : scaling faktor for current expansion during   *
*                           HighCScan (not used yet)                      *
* IntInt                  : integral spot intensity measured by           *
*                           CentralIntensity                              *
* scanmarker              : current scan position                         *
* points                  : number of measured channels                   *
* points2                 : yet always equal points (only for scan2d)     *
* energy                  : electron energy of scan                       *
* fokus                   : focus adjust of scan (not expl.used yet)      *
* current                 : primary current of measurement                *
* TotalInt                : total integral spot intensity                 *
*                           (refer to CentralIntensity)                   *
* IntRadius               : integration radius used at CentralIntensity   *
* PeakBottom              : background intensity used at CentralIntensity *
* dateofscan              : date of measurement                           *
* comment                 : free (used length is limited to 70 characters)*
* field2d[0..points]^[0..points] :                                        *
*                           measuring channels for 2-dim. measurements    *
*                           points may run from 1 to max2d                *
*                           (only for scan2d)                             *
**************************************************************************/

#ifdef NO_BATCH_INCLUDE

#define OS2HEADERSIZE 9102
#define DUMMYSIZE 960

#define VoltAnz  256	/* Achtung Änderungen machen File I/O unmöglich */

typedef struct{
  unsigned char  hours;
  unsigned char  minutes;
  unsigned char  seconds;
  unsigned char  hundredths;
  unsigned char  day;
  unsigned char  month;
  unsigned short year;
  short          timezone;
  unsigned char  weekday;
} DATETIME;


typedef enum {TopoGraphic, ConstZ, MultiVolt,Spectroscopy, TimeScanMode, ScanTyp5, ScanTyp6 } SCANMODE;

typedef enum {STM_1,STM_2,STM_3,STM_4,STM_5,STM_6} INSTRUMENT;


typedef enum {DisLine, DisGrey, DisDual, DisMulti} DISPMODE;

typedef struct _header {
  SCANMODE ScMode;	  /*  Typ des Scans */
  unsigned short   NumOfVolts;	  /* Anzahl der gescanten Spannungen, eigtl. Voltanz */
  long	  nx[VoltAnz];	  /*  Anzahl der Meßpunkte einer Zeile */
  long	  ny[VoltAnz];	  /*  Anzahl der Meßzeilen */
  long	  dx[VoltAnz];	  /*  Abstand zweier x - Meßpunkte in DA - Einheiten */
  long	  dy[VoltAnz];	  /*  Abstand zweier y - Meßpunkte */
  long	  x_Offset[VoltAnz];	   /*  Offset in x [DA] */
  long	  y_Offset[VoltAnz];	   /*  Offset in y [DA] */
  short	  VoltScanStart[VoltAnz];  /*  Anfang und */
  short	  VoltScanEnd[VoltAnz];    /*	 Ende des Spec - Scans */
  short	  CurVolt[VoltAnz];	   /*  Biasspannung  [mV], Voltscan */
  short	  NumOfTopAve;	  /*  Anzahl der AD- Messungen, die gemittelt werden */
  short	  NumOfSpecAve;   /*  Anzahl der AD- Messungen, die gemittelt werden */
  long	  HV_MinVolt;	  /*  minimale Spannung [mV] des HV - Verstärkers */
  long	  HV_MaxVolt;	  /*  maximale Spannung [mV] des HV - Verstärkers */
  long	  MinDA;	  /*  minimaler DA - Wert; */
  long	  MaxDA;	  /*  maximaler DA - Wert; */
  double   DAtoAng_X;	  /*  Umrechnung DA -> Angstroem */
  double   DAtoAng_Y;	  /*  für X, Y, Z */
  double   DAtoAng_Z;	  /*	*/
  long	  forw_delay;	  /* Software forward Delay zwischen zwei Messungen */
  long	  back_delay;	  /* Software backward Delay zwischen zwei Messungen */
  double   greyfac;	  /*  Skalierung der Grauwerte für Darstellung */
  double   linefac;	  /*  Skalierung der Linien für Darstellung */
  short	  Rotation;	  /*  Winkel [Grad] des Scans, gegen x */
  short	  direction;	  /*  Scanrichtung : 1 = Vorwärts, -1 = Rückwärts */
  INSTRUMENT  Instrument;	  /*  STM1 = 0  STM2 = 1 ... */
  DATETIME StartTime;	  /*  Datum , Uhrzeit, bei Scanstart */
  DATETIME EndTime;	  /*  Datum , Uhrzeit  bei Scanende */
  char	  UserName[40];   /*  Name des Benutzers */
  char	  comment[256];   /*  Kommentar zum Scan */
  double   VDriftX;	  /*  DriftGeschwindigkeit beim Scan */
  double   VDriftY;
  short	  BackInter;	  /*  Intervalle beim autom. zurückziehen */
  double   BackTime;	  /*  Zeit, die zurückgezogen bleibt */
  short	  MinAbhebDA;	  /*   Referenzstrom beim Zurückziehen */
  short	  RefSpanDA;	  /*  Sollstrom am externen Eingang der Regelung */
  unsigned short	  Kennung;	 /*  Kennung: sinnvolle Werte */
  short	  GateOutTime;
  char	  dummy [DUMMYSIZE];	  /*	Noch Platz */
  double   SpecDAtoVolt;   /*  Umrechnung DA zu RampenVolt */
  double   SpecADtoNanAmp; /*	 "       AD in Nano- Ampere */
  short	  NumOfSpecVolts; /*  wieviel Spannungsschritte auf der Rampe */
  short	  SpecXDist;	  /*  Abstand von Spectroskopie Pkt in X */
  short	  SpecYDist;	  /*		"                   In Y */
  double   brightfac;	  /*  Nulloffset für Graudarstellung	*/
} HEADER;

#endif


typedef struct {
  int *Daten;
  int anz;
  int maxlincol;
  int yOff;
  int astep;
} GRAFARGS;

#endif



