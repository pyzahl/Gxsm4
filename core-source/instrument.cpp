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

#include "instrument.h"

#include "dt400curve10.h"

/*! 
   XSM device definitions 
*/

XSM_Instrument::XSM_Instrument(XSMRESOURCES &xsmres){
        xsmres_ptr = &xsmres;  // keep a internal ref top global rescources
        DigRangeIn = (long)xsmres.DigRangeIn;
	AnalogVMaxIn = xsmres.AnalogVMaxIn;
	DigRangeOut = (long)xsmres.DigRangeOut;
	AnalogVMaxOut = xsmres.AnalogVMaxOut;

	global_nAmpere2Volt = (double)xsmres.nAmpere2Volt;
	set_current_gain_modifier (1.0); // make sure to have a default
	set_current_gain_modifier (xsmres.current_gain_modifier); // init value from prefrences if not managed by HwI
  
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

	update ();

	type=NULL;
	name=NULL;
	xunitname=NULL;
	yunitname=NULL;
	zunitname=NULL;
}

double XSM_Instrument::set_current_gain_modifier (int pos ) {
	if (pos >= 0 && pos < 9)
		if (xsmres_ptr->VG[pos] > 0.0)
			return set_current_gain_modifier ((double)xsmres_ptr->VG[pos]);
	return current_gain_multiplier;
}

// may call from HwI to custom adjust and override preferences
void XSM_Instrument::override_dig_range (long digital_range){ // always symmetric +/-
	DigRangeIn  = digital_range;
	DigRangeOut = digital_range;
	update ();
}

void XSM_Instrument::override_volt_in_range (double vrange){ // always symmetric +/-
	AnalogVMaxIn = vrange;
	update ();
}

void XSM_Instrument::override_volt_out_range (double vrange){ // always symmetric +/-
	AnalogVMaxOut = vrange;
	update ();
}	

OFFSET_MODE XSM_Instrument::OffsetMode (OFFSET_MODE ofm){ 
	return offset_mode;
}

void XSM_Instrument::update (double temp){
	if (temp > 0.){ // use var temp sensitivity computation
		xPsens = xsmres_ptr->XPiezoAV;
		yPsens = xsmres_ptr->YPiezoAV;
		zPsens = xsmres_ptr->ZPiezoAV;
	} else { // use fixed default sensitivity
		xPsens = xsmres_ptr->XPiezoAV;
		yPsens = xsmres_ptr->YPiezoAV;
		zPsens = xsmres_ptr->ZPiezoAV;
	}

	XSM_DEBUG_GM (DBG_L1, "XSM_Instrument::update ** xsmres->X/Y/ZPiezoAV = %g, %g, %g Ang/V", xPsens, yPsens, zPsens);
	XSM_DEBUG_GM (DBG_L1, "XSM_Instrument::update ** xsmres->DigRangeOut  = %ld", DigRangeOut);

	xR = AnalogVMaxOut*xPsens;
	yR = AnalogVMaxOut*yPsens;
	zR = AnalogVMaxOut*zPsens;

	xd = xR/(double)DigRangeOut;
	yd = yR/(double)DigRangeOut;
	zd = zR/(double)DigRangeOut;

	BiasGain     = (double)xsmres_ptr->BiasGain;
	BiasOffset   = (double)xsmres_ptr->BiasOffset;

	global_nAmpere2Volt = (double)xsmres_ptr->nAmpere2Volt;
	
	set_current_gain_modifier (); // update with eventually new xsmres_ptr->nAmpere2Volt
	
	nNewton2Volt = (double)xsmres_ptr->nNewton2Volt;
	dHertz2Volt  = (double)xsmres_ptr->dHertz2Volt;
	eV2Volt      = (double)xsmres_ptr->EnergyCalibVeV;

   
	XSM_DEBUG_GM (DBG_L1, "XSM_Instrument::update ** xsmres->BiasGain     = %g", BiasGain);
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

