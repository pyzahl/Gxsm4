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

#include "instrument.h"

#include "dt400curve10.h"

/*! 
   XSM device definitions 
*/

XSM_Instrument::XSM_Instrument(XSMRESOURCES &xsmres){
	DigRangeIn = (long)xsmres.DigRangeIn;
	AnalogVMaxIn = xsmres.AnalogVMaxIn;
	DigRangeOut = (long)xsmres.DigRangeOut;
	AnalogVMaxOut = xsmres.AnalogVMaxOut;
	current_gain_multiplier = 1.0;
  
	for( int i=0; i<GAIN_POSITIONS; ++i)
		VList[i] = xsmres.V[i];
	
	Vx0 = Vx = VList[xsmres.VXdefault];
	Vy0 = Vy = VList[xsmres.VYdefault];
	Vz0 = Vz = VList[xsmres.VZdefault];

	offset_mode  = xsmres.AnalogOffsetAdding ? OFM_ANALOG_OFFSET_ADDING : OFM_DSP_OFFSET_ADDING;

	if (OffsetMode () == OFM_ANALOG_OFFSET_ADDING){
		Vx0 = VList[xsmres.VX0default];
		Vy0 = VList[xsmres.VY0default];
		Vz0 = VList[xsmres.VZ0default];
	}

	Vxm = xsmres.XYZ_modulation_gains[0];
	Vym = xsmres.XYZ_modulation_gains[1];
	Vzm = xsmres.XYZ_modulation_gains[2];

	update_piezosensitivity (xsmres);

	type=NULL;
	name=NULL;
	xunitname=NULL;
	yunitname=NULL;
	zunitname=NULL;
}

// may call from HwI to custom adjust and override preferences
void XSM_Instrument::override_dig_range (long digital_range, XSMRESOURCES &xsmres){ // always symmetric +/-
	DigRangeIn  = digital_range;
	DigRangeOut = digital_range;
	update_piezosensitivity (xsmres);
}

void XSM_Instrument::override_volt_in_range (double vrange, XSMRESOURCES &xsmres){ // always symmetric +/-
	AnalogVMaxIn = vrange;
	update_piezosensitivity (xsmres);
}

void XSM_Instrument::override_volt_out_range (double vrange, XSMRESOURCES &xsmres){ // always symmetric +/-
	AnalogVMaxOut = vrange;
	update_piezosensitivity (xsmres);
}	

OFFSET_MODE XSM_Instrument::OffsetMode (OFFSET_MODE ofm){ 
	return offset_mode;
}

void XSM_Instrument::update_piezosensitivity (XSMRESOURCES &xsmres, double temp){
	if (temp > 0.){ // use var temp sensitivity computation
		xPsens = xsmres.XPiezoAV;
		yPsens = xsmres.YPiezoAV;
		zPsens = xsmres.ZPiezoAV;
	} else { // use fixed default sensitivity
		xPsens = xsmres.XPiezoAV;
		yPsens = xsmres.YPiezoAV;
		zPsens = xsmres.ZPiezoAV;
	}

	XSM_DEBUG_GM (DBG_L1, "XSM_Instrument::update_piezosensitivity ** xsmres.X/Y/ZPiezoAV = %g, %g, %g Ang/V", xPsens, yPsens, zPsens);
	XSM_DEBUG_GM (DBG_L1, "XSM_Instrument::update_piezosensitivity ** xsmres.DigRangeOut  = %ld", DigRangeOut);

	xR = AnalogVMaxOut*xPsens;
	yR = AnalogVMaxOut*yPsens;
	zR = AnalogVMaxOut*zPsens;

	xd = xR/(double)DigRangeOut;
	yd = yR/(double)DigRangeOut;
	zd = zR/(double)DigRangeOut;

	BiasGain     = (double)xsmres.BiasGain;
	BiasOffset   = (double)xsmres.BiasOffset;

	set_current_gain_modifier ((double)xsmres.current_gain_modifier);
	nAmpere2Volt = (double)xsmres.nAmpere2Volt * current_gain_multiplier;
	nNewton2Volt = (double)xsmres.nNewton2Volt;
	dHertz2Volt  = (double)xsmres.dHertz2Volt;
	eV2Volt      = (double)xsmres.EnergyCalibVeV;

   
	XSM_DEBUG_GM (DBG_L1, "XSM_Instrument::update_piezosensitivity ** xsmres.BiasGain     = %g", BiasGain);
}

double XSM_Instrument::temperature (double diode_volts){
	int i;
	for (i=0; curve_10[i][1] > 0. && curve_10[i][1] > diode_volts; ++i); 
	double dv = diode_volts - curve_10[i][1];
	return curve_10[i][0] + 1000.*dv / curve_10[i][2];
}

SPALEED_Instrument::SPALEED_Instrument(XSMRESOURCES &xsmres, double *E, double *Gt, long *M) : XSM_Instrument(xsmres){ 
	type = g_strdup("SPALEED"); 
	xunitname = g_strdup("V");
	yunitname = g_strdup("V");
	zunitname = g_strdup("Cps");
	BZu  = new BZUnit("%BZ","%BZ", xsmres.Sensitivity, 1.);
	Su   = new SUnit("S","S", xsmres.SampleLayerDist, xsmres.ThetaChGunInt);
	En   = E; // Pointer to actual Energy
	Gate = Gt; // Pointer to actual GateTime
	Mode = M; // Pointer to actual mapping mode BZ/Volt
	
	xPsens = xsmres.XCalibVA;
	yPsens = xsmres.YCalibVA;
	
	xR = AnalogVMaxOut*xPsens;
	yR = AnalogVMaxOut*yPsens;
	
	xd = xR/(double)DigRangeOut;
	yd = yR/(double)DigRangeOut;

}

