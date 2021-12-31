/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 8 c-style: "K&R" -*- */

/* SRanger MK3 and Gxsm - Gnome X Scanning Microscopy Project
 * universal STM/AFM/SARLS/SPALEED/... controlling and
 * data analysis software
 *
 * Important Notice:
 * ===================================================================
 * THIS FILES HAS TO BE IDENTICAL WITH THE CORESPONDING
 * SRANGER-DSP-SOFT SOURCE FILE (same name) AND/OR OTHER WAY ROUND!!
 * It's included in both, the SRanger and Gxsm-2.0 CVS tree for easier
 * independend package usage.
 * ===================================================================
 *
 * Copyright (C) 1999-2011 Percy Zahl
 *
 * Authors: Percy Zahl <zahl@users.sf.net>
 * WWW Home:
 * DSP part:  http://sranger.sf.net
 * Gxsm part: http://gxsm.sf.net
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

// Maximum size for the acquisition buffer

#include "glib.h"
#include "PAC_pll.h"
#include "FB_spm_dataexchange.h"

// PLL Buffers for the acquition 

// Functions declarations -- dummy PAC-PLL for EMU

  // Must be called by the DPS only  (Return 1 if PLL option activated Return 0 if not)
DSP_INT32 StartPLL(DSP_INT32 ioIN, DSP_INT32 ioOUT) {
  return 0;
}

void DataprocessPLL() {				// Must be called by the DPS only
}

void ChangeFreqHL() {
}

void OutputSignalSetUp_HL(){
}

void TestPhasePIResp_HL(){
}

void TestAmpPIResp_HL(){
}

#if 0

// Variables ******************************


// Pointers for the acsiquition

DSP_INT32	*pSignal1; 
DSP_INT32	*pSignal2; 
	
// Block length

DSP_INT32 	blcklen; // Max MaxSizeSigBuf-1 
	
// Sine variables

DSP_INT32 deltaReT; 
DSP_INT32 deltaImT; 
DSP_INT32 volumeSine; 
 
// Phase/Amp detector time constant 

DSP_INT32 Tau_PAC;

// Phase corrector variables
	
DSP_INT32 PI_Phase_Out; 
DSP_INT32 pcoef_Phase,icoef_Phase; 
DSP_INT32 errorPI_Phase; 
DSP_INT64 memI_Phase; 
DSP_INT32 setpoint_Phase; 
DSP_INT32 ctrlmode_Phase; 
DSP_INT64 corrmin_Phase; 
DSP_INT64 corrmax_Phase; 
	
// Min/Max for the output signals
	
DSP_INT32 ClipMin[4]; 
DSP_INT32 ClipMax[4]; 

// Amp corrector variables
	
DSP_INT32 ctrlmode_Amp;
DSP_INT64 corrmin_Amp; 
DSP_INT64 corrmax_Amp; 
DSP_INT32 setpoint_Amp;
DSP_INT32 pcoef_Amp,icoef_Amp; 
DSP_INT32 errorPI_Amp; 
DSP_INT64 memI_Amp;
	
// LP filter selections
	
DSP_INT32 LPSelect_Phase;
DSP_INT32 LPSelect_Amp; 

// Other variables

DSP_INT32 outmix_display[4]; // Must be set at 7 to bypass the PLL output generation
DSP_INT32 InputFiltered;
DSP_INT32 SineOut0; 
DSP_INT32 phase;
DSP_INT32 amp_estimation;
DSP_INT32 Filter64Out[4];

// Mode 2F variable

DSP_INT16 Mode2F; // 0: mode normal (F on analog out/F on the Phase detector) 1: mode 2F (F on analog out/2F on the Phase detector)

// Output of the sine generator (F and 2F)

DSP_INT32 cr1; // Sine F (RE)
DSP_INT32 ci1; // Sine F (IM)
DSP_INT32 cr2; // Sine 2F (RE)
DSP_INT32 ci2; // Sine 2F (IM)
	
	
#endif	
