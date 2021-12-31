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

#ifndef __XSM_INSTRUMENT_H
#define __XSM_INSTRUMENT_H

#include <iostream>
#include <fstream>

#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <cstring>

#include <unistd.h>

#include "xsmtypes.h"
#include "xsmmasks.h"

/* Instrumentendefinition */

typedef enum { OFM_UNDEFINED, OFM_DSP_OFFSET_ADDING, OFM_ANALOG_OFFSET_ADDING } OFFSET_MODE;

class XSM_Instrument{
public:
  XSM_Instrument(XSMRESOURCES &xsmres);
  virtual ~XSM_Instrument(){};

  virtual void update_piezosensitivity (XSMRESOURCES &xsmres, double temp = -1.);
  virtual double temperature (double diode_volts);

  double set_current_gain_modifier (double f) { if (f > 0.) current_gain_multiplier = f; return f; };
  
  double UOutLimit(double u){
    if(u>AnalogVMaxOut) { u=AnalogVMaxOut; }
    if(u<-AnalogVMaxOut) { u=-AnalogVMaxOut; }
    return u;
  };

  double UInLimit(double u){
    if(u>AnalogVMaxIn) { u=AnalogVMaxIn; }
    if(u<-AnalogVMaxIn) { u=-AnalogVMaxIn; }
    return u;
  };
  
  /* resultierende Grenzwerte */
  virtual double XRangeMax(){ return(xR*Vx); };
  virtual double YRangeMax(){ return(yR*Vy); };
  virtual double ZRangeMax(){ return(zR*Vz); };

  virtual double X0RangeMax(){ return(xR*Vx0); };
  virtual double Y0RangeMax(){ return(yR*Vy0); };
  virtual double Z0RangeMax(){ return(zR*Vz0); };

  virtual double XScanRangeMax(){ return(2*XRangeMax()); };
  virtual double YScanRangeMax(){ return(2*YRangeMax()); };

  virtual double XResolution(){ return(xd*Vx); };
  virtual double YResolution(){ return(yd*Vy); };
  virtual double ZResolution(const gchar *Z_Unit_Alias=NULL){ 
	  if (Z_Unit_Alias){
// Reference Base Units are AA for all Zscales, nA for Current, nN for Force, ...
		  if (strncmp (Z_Unit_Alias, "AA", 2) == 0) return zd*Vz;
		  if (strncmp (Z_Unit_Alias, "nm", 2) == 0) return zd*Vz;
		  if (strncmp (Z_Unit_Alias, "um", 2) == 0) return zd*Vz;
		  if (strncmp (Z_Unit_Alias, "mm", 2) == 0) return zd*Vz;
		  if (strncmp (Z_Unit_Alias, "nA", 2) == 0) return (AnalogVMaxIn/DigRangeIn)/nAmpere2Volt;
		  if (strncmp (Z_Unit_Alias, "pA", 2) == 0) return (AnalogVMaxIn/DigRangeIn)/nAmpere2Volt;
		  if (strncmp (Z_Unit_Alias, "nN", 2) == 0) return (AnalogVMaxIn/DigRangeIn)/nNewton2Volt;
		  if (strncmp (Z_Unit_Alias, "Hz", 2) == 0) return (AnalogVMaxIn/DigRangeIn)/dHertz2Volt;
//		  if (strncmp (Z_Unit_Alias, "Cps", 3) == 0) return (AnalogVMaxIn/DigRangeIn)/CountsPerSec2Volt;
//		  if (strncmp (Z_Unit_Alias, "K", 1) == 0) return (AnalogVMaxIn/DigRangeIn)/Kelvin2Volt;
		  if (strncmp (Z_Unit_Alias, "V" ,1) == 0) return AnalogVMaxIn/DigRangeIn;
		  if (strncmp (Z_Unit_Alias, "1" ,1) == 0) return 1.;
		  return 1.;
	  }
	  return zd*Vz; 
  };
  virtual double X0Resolution(){ return(xd*Vx0); };
  virtual double Y0Resolution(){ return(yd*Vy0); };
  virtual double Z0Resolution(const gchar *Z_Unit_Alias=NULL){ 
	  return zd*Vz0; 
  };

  virtual double XModResolution(){ return xd*Vxm; };
  virtual double YModResolution(){ return yd*Vym; };
  virtual double ZModResolution(const gchar *Z_Unit_Alias=NULL){ 
	  return zd*Vzm; 
  };

  virtual double Dig2nAmpere(long I){return ((double)I*AnalogVMaxIn/(DigRangeIn*nAmpere2Volt));};
  virtual double Dig2V(long U){return(AnalogVMaxIn/(double)DigRangeIn*(double)U);};

  /* DigVal <-> Angstroem -Konversions */
  virtual double Dig2XA(long dig){ return(XResolution()*(double)dig); };
  virtual double Dig2YA(long dig){ return(YResolution()*(double)dig); };
  virtual double Dig2ZA(long dig){ return(ZResolution()*(double)dig); };

  virtual double XA2Dig(double ang){ return(ang/XResolution()); };
  virtual double YA2Dig(double ang){ return(ang/YResolution()); };
  virtual double ZA2Dig(double ang){ return(ang/ZResolution()); };

  virtual double Dig2X0A(long dig){ return(X0Resolution()*(double)dig); };
  virtual double Dig2Y0A(long dig){ return(Y0Resolution()*(double)dig); };
  virtual double Dig2Z0A(long dig){ return(Z0Resolution()*(double)dig); };

  virtual double Dig2XModA(long dig){ return(XModResolution()*(double)dig); };
  virtual double Dig2YModA(long dig){ return(YModResolution()*(double)dig); };
  virtual double Dig2ZModA(long dig){ return(ZModResolution()*(double)dig); };

  virtual double X0A2Dig(double ang){ return(ang/X0Resolution()); };
  virtual double Y0A2Dig(double ang){ return(ang/Y0Resolution()); };
  virtual double Z0A2Dig(double ang){ return(ang/Z0Resolution()); };

  /* Get/Change Verstärkung */
  virtual double VX(int i=-1){ return(i>=0 ? Vx = VList[i] : Vx); };  
  virtual double VY(int i=-1){ return(i>=0 ? Vy = VList[i] : Vy); };  
  virtual double VZ(int i=-1){ return(i>=0 ? Vz = VList[i] : Vz); };  

  virtual double VX0(int i=-1){ return(i>=0 ? Vx0 = VList[i] : Vx0); };  
  virtual double VY0(int i=-1){ return(i>=0 ? Vy0 = VList[i] : Vy0); };  
  virtual double VZ0(int i=-1){ return(i>=0 ? Vz0 = VList[i] : Vz0); };  

  virtual OFFSET_MODE OffsetMode (OFFSET_MODE ofm=OFM_UNDEFINED);

  virtual double VoltOut2Dig(double U){ return U*DigRangeOut/AnalogVMaxOut; };
  virtual double Dig2VoltOut(double dig){ return dig/DigRangeOut*AnalogVMaxOut; };

  virtual double VoltIn2Dig(double U){ return U*DigRangeIn/AnalogVMaxIn; };
  virtual double Dig2VoltIn(double dig){ return dig/DigRangeIn*AnalogVMaxIn; };

  virtual double BiasV2Vabs(double U){ return UOutLimit((U-BiasOffset)/BiasGain); }; // correct Bias gain
  virtual double BiasV2V(double U){ return (U-BiasOffset)/BiasGain; }; // correct Bias gain  DAC(0)=B0 DAC(V)=V*BG+B0 -> V=(DAC(V)-B0)/BG
  virtual double BiasGainV2V(){ return BiasGain; }; // correct Bias gain
  virtual double nAmpere2V(double I){ return UInLimit(I*nAmpere2Volt); };
  virtual double nNewton2V(double F){ return UInLimit(F*nNewton2Volt); };
  virtual double dHertz2V(double v){ return UInLimit(v*dHertz2Volt); };
  virtual double eV2V(double eV){ return UOutLimit(eV/eV2Volt); };
  virtual double V2BiasV(double U){return U*BiasGain+BiasOffset; }; // used to determin the correct voltages for STS ramps
  virtual double V2XAng(double U){return U*xR/AnalogVMaxOut; }; // convert volts (piezo) to Ang (X)
  virtual double V2YAng(double U){return U*yR/AnalogVMaxOut; }; // convert volts (piezo) to Ang (Y)
  virtual double V2ZAng(double U){return U*zR/AnalogVMaxOut; }; // convert volts (piezo) to Ang (Z)

  gchar *type;
  gchar *name;
  gchar *xunitname;
  gchar *yunitname;
  gchar *zunitname;

protected:
  /* STM/AFM Gerätedaten */
  long   DigRangeOut;     /* Digital DA Range [integer] */
  double AnalogVMaxOut; /* Analog DA Range [V] */
  long   DigRangeIn;     /* Digital DA Range [integer] */
  double AnalogVMaxIn; /* Analog DA Range [V] */
  double VList[GAIN_POSITIONS];   /* Liste möglicher Verstärkungsfaktoren */
  double Vx, Vy, Vz; /* Eingestellte Verstärkung "Scan" am Piezodriver [1,2,5,10,15] */
  double Vx0, Vy0, Vz0; /* Eingestellte Verstärkung "Offset" am Piezodriver [1,2,5,10,15] */
  double Vxm, Vym, Vzm; /* Amplifier modulation input gains (1x default) */
  double xPsens, yPsens, zPsens; /* Tube Sensitivitys [A/V] */
  double BiasGain, BiasOffset;     /* Gain, Offset for optional Bias adjust */
  double nAmpere2Volt; /* nA (Tunnelcurrent) to Volt Factor */
  double current_gain_multiplier; /* modify current gain factor (nAmp2V) by this scale */
  double nNewton2Volt; /* nN (Lever-Kraft) to Volt Factor */
  double dHertz2Volt;  /* dHz (Frq. Verstimmung) to Volt Factor */
  double eV2Volt;      /* eV to Volt Factor */

  /* Grenzdaten "Scan" für V=1 */
  double xR, yR, zR; /* Max Range für V=1 */
  double xd, yd, zd; /* Resolution (min dx,y,z) für V=1 */

private:
  OFFSET_MODE offset_mode;
};

class STM_Instrument : public XSM_Instrument{
 public:
  STM_Instrument(XSMRESOURCES &xsmres) : XSM_Instrument(xsmres){ 
    type = g_strdup("STM"); 
    zunitname = yunitname = xunitname = g_strdup("Ang");
  };
  virtual ~STM_Instrument(){ 
    g_free(type); 
    g_free(xunitname);
  };

  /* DigVal <-> Angstroem -Konversions */
  /*  unchanged */
};

class AFM_Instrument : public XSM_Instrument{
 public:
  AFM_Instrument(XSMRESOURCES &xsmres) : XSM_Instrument(xsmres){ 
    type = g_strdup("AFM"); 
    zunitname = yunitname = xunitname = g_strdup("Ang");
  };
  virtual ~AFM_Instrument(){ 
    g_free(type); 
    g_free(xunitname);
  };

  /* DigVal <-> Angstroem -Konversions */
  /*  unchanged */
};
class SPALEED_Instrument : public XSM_Instrument{
 public:
  SPALEED_Instrument(XSMRESOURCES &xsmres, double *E, double *Gt, long *M);
  virtual ~SPALEED_Instrument(){ 
    g_free(type); 
    g_free(xunitname);
    g_free(yunitname);
    g_free(zunitname);
  };

  // to override Amplifier settings, always == 1
  virtual double XRangeMax(){ return(Transform2Usr(xR)); };
  virtual double YRangeMax(){ return(Transform2Usr(yR)); };
  virtual double ZRangeMax(){ return(zR); };

  virtual double XResolution(){ return(xd); };
  virtual double YResolution(){ return(yd); };
  virtual double ZResolution(){ return(1. / (*Gate)); };

  /* DigVal <-> K-space -Konversions */
  virtual double Dig2XA(long dig){ return(Transform2Usr(XResolution()*(double)dig)); };
  virtual double Dig2YA(long dig){ return(Transform2Usr(YResolution()*(double)dig)); };
  virtual double Dig2ZA(long dig){ return(ZResolution()*(double)dig); };

  virtual double XA2Dig(double ang){ return(Transform2V(ang)/XResolution()); };
  virtual double YA2Dig(double ang){ return(Transform2V(ang)/YResolution()); };
  virtual double ZA2Dig(double ang){ return(ang/ZResolution()); };

 private:
  double Transform2V(double x){ 
    return x;
  };
  
  double Transform2Usr(double x){ 
    return x;
  };
  
  long   *Mode;
  double *En;
  double *Gate;
  SUnit  *Su;
  BZUnit *BZu;
};
#endif
