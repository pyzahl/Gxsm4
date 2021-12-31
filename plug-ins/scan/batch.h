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

/************************************************************************

	      Include File fuer alle Stm - Batch - Programme

			    O. Jusko  7/89

*************************************************************************/
#ifndef __BATCH_H
#define __BATCH_H

#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

// #define PI 3.141592654

#define VoltAnz  256	/* Achtung Aenderung machen File I/O unmoeglich */
/* min and max macros */

#ifndef max
#define max(a,b)    (((a) > (b)) ? (a) : (b))
#define min(a,b)    (((a) < (b)) ? (a) : (b))
#endif

#define PI M_PI

typedef enum { TopoGraphic, ConstZ, MultiVolt, Spectroscopy, TimeScanMode, SPA2DScan, SPA1DScan, SPAKZScan, ScanTyp5, ScanTyp6 } SCANMODE;

typedef enum { DATFIL, BYTFIL, SHTFIL, FLTFIL, DBLFIL, CPXFIL, FIL6, FIL7, PGMFIL, EPSFFIL, TGAFIL, D2DFIL, SPEFIL, UIMFIL } FILETYPE;

/* Falls DOS oder OS2.H not used */
#ifndef INCL_DOS
typedef char   CHAR;
typedef short  SHORT;
typedef long   LONG;
typedef unsigned char   UCHAR;
#ifndef USHORT
typedef unsigned short  USHORT;
#endif
typedef unsigned long   ULONG;
typedef struct {
  UCHAR   hours;
  UCHAR   minutes;
  UCHAR   seconds;
  UCHAR   hundredths;
  UCHAR   day;
  UCHAR   month;
  USHORT  year;
  SHORT   timezone;
  UCHAR   weekday;
} DATETIME;
#endif

#define DATEI 0
#define SKALAR 1
#define INTAKT 2

#ifndef BOOL
  #define BOOL int
#endif
#ifndef TRUE
   #define TRUE 1
#endif
#ifndef FALSE
   #define FALSE 0
#endif

#define MABYT 255
#define MIBYT 0
#define MASHT 32767
#define MISHT -32768
#define MAFLT 3.4e+38
#define MIFLT -3.4e-38
#define MADBL 1.7e+308
#define MIDBL -1.7e-308

// #define DUMMYSIZE 960     /* noch Platz im Header ... PZ 5/7/97 */
#define DUMMYSIZE 952     /* noch Platz im Header ... PZ 5/7/97 */

typedef struct {
// shortSCANMODE ScMode;	  /* Typ des Scans*/
 short    ScMode;	  /* Typ des Scans*/
 USHORT   NumOfVolts;	  /* Anzahl der gescannten Spannungen, eigentl. Voltanz*/
 long	  nx[VoltAnz];	  /* Anzahl der Messpunkte einer Zeile*/
 long	  ny[VoltAnz];	  /* Anzahl der Messzeilen*/
 long	  dx[VoltAnz];	  /* Abstand zweier x - Messpunkte in DA - Einheiten*/
 long	  dy[VoltAnz];	  /* Abstand zweier y - Messpunkte*/
 long	  x_Offset[VoltAnz];	   /* Offset in x [DA]*/
 long	  y_Offset[VoltAnz];	   /* Offset in y [DA]*/
 short	  VoltScanStart[VoltAnz];  /* Anfang und*/
 short	  VoltScanEnd[VoltAnz];    /*	Ende des Spec - Scans*/
 short	  CurVolt[VoltAnz];	   /* Biasspannung  [mV], Voltscan*/
 short	  NumOfTopAve;	  /* Anzahl der AD- Messungen, die gemittelt werden*/
 short	  NumOfSpecAve;   /* Anzahl der AD- Messungen, die gemittelt werden*/
 long	  HV_MinVolt;	  /* minimale Spannung [mV] des HV - Verstaerkers*/
 long	  HV_MaxVolt;	  /* maximale Spannung [mV] des HV - Verstaerkers*/
 long	  MinDA;	  /* minimaler DA - Wert;*/
 long	  MaxDA;	  /* maximaler DA - Wert;*/
 double   DAtoAng_X;	  /* Umrechnung DA -> Angstroem*/
 double   DAtoAng_Y;	  /* fuer X, Y, Z*/
 double   DAtoAng_Z;	  /**/
 long	  forw_delay;	  /* Software forward  Delay zwischen zwei Messungen*/
 long	  back_delay;	  /* Software backward Delay zwischen zwei Messungen*/
 double   greyfac;	  /* Skalierung der Grauwerte fuer Darstellung*/
 double   linefac;	  /* Skalierung der Linien fuer Darstellung*/
 short	  Rotation;	  /* Winkel [Grad] des Scans, gegen x*/
 short	  direction;	  /* Scanrichtung : 1 = Vorwaerts, -1 = Rueckwaerts*/
 short	  Instrument;	  /* Welches STM ? 0 = erstes*/
 DATETIME StartTime;	  /* Datum , Uhrzeit, bei Scanstart*/
 DATETIME EndTime;	  /* Datum , Uhrzeit  bei Scanende*/
 char	  UserName[40];   /* Name des Benutzers*/
 char	  comment[256];   /* Kommentar zum Scan*/
 double   VDriftX;	  /*  DriftGeschwindigkeit beim Scan */
 double   VDriftY;
 SHORT	  BackInter;	  /*  Intervalle beim autom. zurückziehen */
 double   BackTime;	  /*  Zeit, die zurückgezogen bleibt */
 SHORT	  MinAbhebDA;	  /*   Referenzstrom beim Zurückziehen */
 SHORT	  RefSpanDA;	  /*  Sollstrom am externen Eingang der Regelung */
 unsigned short	  Kennung;	 /*  Kennung: sinnvolle Werte */
 short	  GateOutTime;
 CHAR	  dummy [DUMMYSIZE];	  /*	Noch Platz */
 double   SpecDAtoVolt;   /* Umrechnung DA zu RampenVolt*/
 double   SpecADtoNanAmp; /*	"       AD in Nano- Ampere*/
 SHORT	  NumOfSpecVolts; /* wieviel Spannungsschritte auf der Rampe*/
 SHORT	  SpecXDist;	  /* Abstand von Spectroskopie Pkt in X*/
 SHORT	  SpecYDist;	  /*	       "                   In Y  */
 double   brightfac;	  /* Nulloffset fuer Graudarstellung        */
} HEADER;

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
} D2D_SPA_DAT_HEADER;

typedef struct {
  short x,y;
} XYCoord;

typedef struct {
  short lx,yo,rx,yu;
} Rectangle;

typedef union {
  HEADER kopf;
  D2D_SPA_DAT_HEADER d2dkopf;  
  XYCoord xydim;
} FILEHEAD;

/***********************/
/* Function Prototypes */
/***********************/

void *salloc (long, unsigned short);
void sfree (void *);
int stricmp(const char *s1, const char *s2);
char *strupr(char *s1);

long FileCheck (char*, FILETYPE*, FILEHEAD*, FILE**);
int  FileRead  (FILE*, char*, long);
int  FileWrite (char*, char*, FILEHEAD*);


#endif
