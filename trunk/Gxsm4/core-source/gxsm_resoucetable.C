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

#include <dirent.h>
#include <fnmatch.h>
#include <string.h>
#include <cstring>

#include <locale.h>
#include <libintl.h>

#include <gtk/gtk.h>
#include "glbvars.h"
#include "gnome-res.h"

#include "gxsm_resoucetable.h"
// #include "../pixmaps/gxsmdruidlogo.xpm"
// #include "../pixmaps/gxsmwatermark.xpm"

#define UTF8_DOT       "\302\267"
#define UTF8_DEGREE    "\302\260"
#define UTF8_MU        "\302\265"
#define UTF8_ANGSTROEM "\303\205"

#define MANUAL_REF_CHANCONF N_("See Manual, section Channelsetup")
#define MANUAL_REF_PROBECONF N_("See Manual, section Probesetup")

gint ask_if_instrument_is_spaleed_and_hard(){ return ( IS_SPALEED_CTRL && !(IS_NOCARD) ); }
gint ask_if_instrument_is_afm_and_hard(){ return ( IS_AFM_CTRL && !(IS_NOCARD) ); }
gint ask_if_instrument_is_spm_and_hard(){ return ( IS_SPM_CTRL && !(IS_NOCARD) ); }

// Gxsm Resource Table
extern GnomeResEntryInfoType xsm_res_def[];

// Have to define option lists before:
const gchar* TrueFalseList[]      = { "true", "false", NULL };
const gchar* YesNoList[]      = { "yes", "no", NULL };
const gchar* HardwareCardListCore[]   = { "no", NULL };
#define MAX_HWI 32
const gchar* HardwareCardList[MAX_HWI];
// do not used real Angstroem or mu characters here!!!
const gchar* InstrumentTypeList[] = { "STM", "AFM", "SARLS", "SNOM", "SPALEED", "ELSLEED", "CCD", "SCMA", NULL }; 
const gchar* UserXYUnitList[]     = { "AA", "nm", "um", "mm", "BZ", "sec", "amu", "V", "1", NULL };
const gchar* UserZUnitList[]      = { "1", "V", "AA", "nm", "um", "mm", "nA", "pA", "nN", "Hz", "K", "CPS", "Int", "Events", "a.u.", NULL };
const gchar* UserZTypeList[]      = { "BYTE", "SHORT", "LONG", "ULONG", "LLONG", "FLOAT", "DOUBLE", "COMPLEX", "RGBA", NULL };
const gchar* PrbXunitsList[]      = { "1", "V", "AA", "nm", "um", "mm", "BZ", "sec", "amu", "a.u.", NULL };
const gchar* PrbYunitsList[]      = { "1", "V", "AA", "nm", "um", "mm", "nA", "pA", "nN", "Hz", "Amp", "logAmp", "Events", "a.u.", NULL };
const gchar* UserSliderTypeList[] = { "slider", "mover", NULL };
const gchar* InstrumentZPolarityList[] = { "positive", "negative", NULL };
const gchar* UserFileNameConventionList[] = { "digit", "alpha", "date-time", NULL };
const gchar* UserAutosaveUnitList[] = { "percent", "lines", "seconds", NULL };
const gchar* AnalogVXYZIndexList[]  = { "0", "1", "2", "3", "4", "5", "6", "7", "8", NULL };
const gchar* AnalogOffsetModeList[]  = { "undefined", "DSP offset adding", "analog offset adding", NULL };
const gchar* GUIObjectHandleTypes[]    = { "Triangle", "Square", NULL };
//gchar* List[] = { NULL };

// Simple Units Symbols to append to..
const gchar* Unit1[] = { " " };
const gchar* UnitV[] = { "V" };
const gchar* UnitA[] = { UTF8_ANGSTROEM };
const gchar* UnitAV[] = { UTF8_ANGSTROEM "/V" };
const gchar* UnitVnA[] = { "V/nA" };
const gchar* UnitVnN[] = { "V/nN" };
const gchar* UnitVHz[] = { "V/Hz" };
const gchar* UniteVV[] = { "eV/V" };
const gchar* UnitSensi[] = { "BZ" UTF8_DOT "sqrt(eV)/V" };
const gchar* UnitDeg[] = {  UTF8_DEGREE };

#define UNIT_AV UTF8_ANGSTROEM"/V"

// Resource Definition and Information table

GnomeResEntryInfoType xsm_res_def[] = {
	GNOME_RES_ENTRY_FIRST_NAME("GXSM_PREFERENCES_200309010000"),
	GNOME_RES_ENTRY_INFOTEXT 
  // Hardware Settings
	(N_("Hardware"),
	 N_("Select the type of Low-Level Hardware Interface and connection (device): \n\n"
	    "Gxsm itself is entirely Hardware independent, it only suggests a \n"
	    "command compatible device to communicate with via a kernel module. \n"
	    "In case of SPM's a intelligent SPM controller is expected. \n"
	    "This can be a DSP card like the PCI32 or similar running the SPM software, \n"
	    "which can specially adapted to the STM, AFM, SARLS or what ever is similar. \n\n"
	    "In case of SPALEED it's similar, but a special SPALEED command subset is used.\n"
	    "The hardware can be a intelligent DSP card or even a simple AD/DA/counter card, \n"
	    "in the second case the required controller is emulated at kernel level.\n"
	    "")
		),
	GNOME_RES_ENTRY_ASK_PATH_OPTION 
	(GNOME_RES_STRING, "Hardware/Card", "no", &xsmres.HardwareType, HardwareCardList, 
	 N_("Hardware"),
	 N_("Type of Hardware Interface, choice of: \n"
	    " no: data analysis mode (hardware disabled, simulation mode). \n"
	    " Generic Hardware PlugIn (HwI) Selection: \n"
	    " XXX:SPM (SPM comandset), \n"
	    " XXX:SPA (SPA-LEED like), \n"
	    " and others, \n"
	    " SRanger:SPM (old SoftdB Signal Ranger DSP model MK1 (-STD/-SP2) -- obsolete, \n"
	    " SRangerMK2:SPM (SoftdB Signal Ranger DSP model MK2 and MK3 autodetected, \n"
	    " XXX:ccd: special ParPort CCD Cam. TC211 (CCD comandset)")
         ),
	GNOME_RES_ENTRY_ASK_PATH
	( GNOME_RES_STRING, "Hardware/Device", "/dev/sranger_mk2_0", &xsmres.DSPDev,
	  N_("Hardware"),
	  N_("DSP Character Device Path for DSP based Hardware Interfaces:\n"
	     " ignored in case of HardwareType=no\n"
	     " old Signal Ranger SP2/STD use: /dev/sranger0\n"
	     " Signal Ranger MK2 and MK3 specify the first device to start auto detecting: /dev/sranger_mk2_0\n")
          ),
	
	GNOME_RES_ENTRY_INFOTEXT
	( N_("Instrument"),
	  N_("Setup of Instrument type. Btw. this is used to autoselect als essential Plug-Ins.\n"
	     "In addition the userinterface adapts to the Instruments special needs.\n"
	     "")
		),
	GNOME_RES_ENTRY_ASK_PATH_OPTION
	( GNOME_RES_STRING, "Instrument/Type", "STM", &xsmres.InstrumentType, InstrumentTypeList, 
	  N_("Instrument"), 
	  N_("Type of Instrument, choices are:\n"
	     " STM, AFM, SARLS, SPALEED, ELSLEED, CCD, SCMA")
		),
	GNOME_RES_ENTRY_ASK_PATH
	( GNOME_RES_STRING, "Instrument/Name", "Quantum", &xsmres.InstrumentName,
	  N_("Instrument"), 
	  N_("Arbitrary Instrument Name, max. 30 chars !")
		),

	GNOME_RES_ENTRY_AUTO_PATH
	( GNOME_RES_INT,  "Instrument/WorldSizeNxy","4096", &xsmres.world_size_nxy,
          N_("Instrument"),
	  N_("Width and height of world map in pixels")
		),

	GNOME_RES_ENTRY_INFOTEXT_DEPEND 
	( N_("Inst-SPM"),
	  N_("SPM and IVC specific Volt to physical unit conversions:\n\n"
	     "The default values are approximatly matching standard 1/2\" piezo tubes.\n"
	     "\n"
	     "In addition please check Section DataAq/... in Settings/Preferences for\n"
	     "details about data acquisition channel setup later too and refer to manual.\n"
	     ""),
	  ask_if_instrument_is_spm_and_hard
		),
	GNOME_RES_ENTRY_ASK_PATH_DEPEND_UNIT
	( GNOME_RES_FLOAT_W_UNIT,  "Instrument/X-Piezo-AV", "532.0", &xsmres.XPiezoAV, UnitAV, N_("Inst-SPM"),
	  N_("STM/AFM: X Piezo Sensitivity in " UNIT_AV),
	  ask_if_instrument_is_spm_and_hard
		),
	GNOME_RES_ENTRY_ASK_PATH_DEPEND_UNIT
	( GNOME_RES_FLOAT_W_UNIT,  "Instrument/Y-Piezo-AV", "532.0", &xsmres.YPiezoAV, UnitAV, N_("Inst-SPM"),
	  N_("STM/AFM: Y Piezo Sensitivity in " UNIT_AV),
	  ask_if_instrument_is_spm_and_hard 
		),
	GNOME_RES_ENTRY_ASK_PATH_DEPEND_UNIT
	( GNOME_RES_FLOAT_W_UNIT,  "Instrument/Z-Piezo-AV", "74.0", &xsmres.ZPiezoAV, UnitAV, N_("Inst-SPM"),
	  N_("STM/AFM: Z Piezo Sensitivity in " UNIT_AV),
	  ask_if_instrument_is_spm_and_hard 
		),
	GNOME_RES_ENTRY_ASK_PATH_DEPEND_UNIT
	( GNOME_RES_FLOAT_W_UNIT,  "Instrument/Bias-Gain", "1", &xsmres.BiasGain, Unit1, N_("Inst-SPM"),
	  N_("STM/AFM: Bias Gain, Unit: 1 (gain between DAC-Bias-Volt and sample voltage)"),
	  ask_if_instrument_is_spm_and_hard
		),
	GNOME_RES_ENTRY_ASK_PATH_DEPEND_UNIT
	( GNOME_RES_FLOAT_W_UNIT,  "Instrument/Bias-Offset", "0", &xsmres.BiasOffset, UnitV, N_("Inst-SPM"),
	  N_("STM/AFM: Bias Offset, Unit: V (offset = actual reading at DAC setting ZERO)"),
	  ask_if_instrument_is_spm_and_hard
		),
	GNOME_RES_ENTRY_ASK_PATH_DEPEND_UNIT
	( GNOME_RES_FLOAT_W_UNIT,  "Instrument/nAmpere2Volt", "0.1", &xsmres.nAmpere2Volt, UnitVnA, N_("Inst-SPM"),
	  N_("STM/AFM: Tunnel Current Gain, Unit: V/nA"),
	  ask_if_instrument_is_afm_and_hard
		),
	GNOME_RES_ENTRY_ASK_PATH_DEPEND_UNIT
	( GNOME_RES_FLOAT_W_UNIT,  "Instrument/CurrentGainModifier", "1.0", &xsmres.current_gain_modifier, Unit1, N_("Inst-SPM"),
	  N_("STM/AFM: (temporary) Tunnel Current Scaling"),
	  ask_if_instrument_is_afm_and_hard
		),
	GNOME_RES_ENTRY_ASK_PATH_DEPEND_UNIT
	( GNOME_RES_FLOAT_W_UNIT,  "Instrument/nNewton2Volt", "1.0", &xsmres.nNewton2Volt, UnitVnN, N_("Inst-SPM"),
	  N_("AFM: nano Newton to Volt calib. factor"),
	  ask_if_instrument_is_afm_and_hard
		),
	GNOME_RES_ENTRY_AUTO_PATH_UNIT
	( GNOME_RES_FLOAT_W_UNIT, "Instrument/dHertz2Volt", "0.0", &xsmres.dHertz2Volt, UnitVHz, N_("Inst-SPM"),
	  N_("STM/AFM: delta Herz to Volt calib. factor. Set to ZERO for automatic internal PAC/PLL!")
		),
	GNOME_RES_ENTRY_AUTO_PATH_OPTION
	( GNOME_RES_BOOL, "Instrument/ScanOrgCenter", "true", &xsmres.ScanOrgCenter, TrueFalseList, N_("Inst-SPM"), 
	  N_("bool: true=Offset is relative to center of scan, else center of top line")
		),
	GNOME_RES_ENTRY_AUTO_PATH_OPTION
	( GNOME_RES_BOOL, "Instrument/ScannerZPolarity", "negative", &xsmres.ScannerZPolarity, InstrumentZPolarityList, N_("Inst-SPM"), 
	  N_("MK3 only: select scanner Z polarity.")
		),

	// Analog defaults, Piezodrive
	GNOME_RES_ENTRY_INFOTEXT_DEPEND
	( N_(N_("Inst-SPM")),
	  N_("Select your default Piezodrive settings.\n"
	     "\n"
	     "Please have a look at the Section Analog/... in Settings/Preferences for\n"
	     "details about in line and variable gain Piezodrive settings.\n"
	     ""),
	  ask_if_instrument_is_spm_and_hard
		),
	GNOME_RES_ENTRY_AUTO_PATH
	( GNOME_RES_FLOAT,  "Analog/Dig-Range-In", "32767.0" , &xsmres.DigRangeIn, N_("Inst-SPM"),
	  N_("Max AD Digital Value corresponding to Max Voltage")
		),
	GNOME_RES_ENTRY_AUTO_PATH_UNIT
	( GNOME_RES_FLOAT_W_UNIT,  "Analog/Volt-Max-In", "10.0", &xsmres.AnalogVMaxIn, UnitV, N_("Inst-SPM"),
	  N_("Max Voltage of AD [V]")
		),
	GNOME_RES_ENTRY_AUTO_PATH
	( GNOME_RES_FLOAT,  "Analog/Dig-Range-Out", "32747.0" , &xsmres.DigRangeOut, N_("Inst-SPM"),
	  N_("Max DA Digital Value corresponding to Max Voltage")
		),
	GNOME_RES_ENTRY_AUTO_PATH_UNIT
	( GNOME_RES_FLOAT_W_UNIT,  "Analog/Volt-Max-Out", "10.4", &xsmres.AnalogVMaxOut, UnitV, N_("Inst-SPM"),
	  N_("Max Voltage of DA [V] -- note: MK3-A810 new gen., else 10.0V.")
		),
/* has to match: V[0..GAIN_POSITIONS-1] !! */
        GNOME_RES_ENTRY_AUTO_PATH
	( GNOME_RES_FLOAT,  "Analog/V1",  "1.0", &xsmres.V[0], N_("Inst-SPM"),
	  N_("External Amplifier Gain in Position 0")
		),
	GNOME_RES_ENTRY_AUTO_PATH
	( GNOME_RES_FLOAT,  "Analog/V2",  "2.0", &xsmres.V[1], N_("Inst-SPM"),
	  N_("External Amplifier Gain in Position 1")
		),
	GNOME_RES_ENTRY_AUTO_PATH
	( GNOME_RES_FLOAT,  "Analog/V3",  "5.0", &xsmres.V[2], N_("Inst-SPM"),
	  N_("External Amplifier Gain in Position 2")
		),
	GNOME_RES_ENTRY_AUTO_PATH
	( GNOME_RES_FLOAT,  "Analog/V4", "10.0", &xsmres.V[3], N_("Inst-SPM"),
	  N_("External Amplifier Gain in Position 3")
		),
	GNOME_RES_ENTRY_AUTO_PATH
	( GNOME_RES_FLOAT,  "Analog/V5", "15.0", &xsmres.V[4], N_("Inst-SPM"),
	  N_("External Amplifier Gain in Position 4")
		),
	GNOME_RES_ENTRY_AUTO_PATH
	( GNOME_RES_FLOAT,  "Analog/V6", "0.1", &xsmres.V[5], N_("Inst-SPM"),
	  N_("External Amplifier Gain in Position 5")
		),
	GNOME_RES_ENTRY_AUTO_PATH
	( GNOME_RES_FLOAT,  "Analog/V7", "0.2", &xsmres.V[6], N_("Inst-SPM"),
	  N_("External Amplifier Gain in Position 6")
		),
	GNOME_RES_ENTRY_AUTO_PATH
	( GNOME_RES_FLOAT,  "Analog/V8", "0.3", &xsmres.V[7], N_("Inst-SPM"),
	  N_("External Amplifier Gain in Position 7")
		),
	GNOME_RES_ENTRY_AUTO_PATH
	( GNOME_RES_FLOAT,  "Analog/V9", "0.5", &xsmres.V[8], N_("Inst-SPM"),
	  N_("External Amplifier Gain in Position 8")
		),
#if 1
	GNOME_RES_ENTRY_ASK_PATH_OPTION_FMT_DEPEND
	( GNOME_RES_INT, "Analog/VX-default","1", &xsmres.VXdefault, AnalogVXYZIndexList, 
	  "# %s -> x %g", xsmres.V,
	  N_("Inst-SPM"),
	  N_("Default Amplifier Positionindex [0,1,2,..8] used by X-Channel"),
	  ask_if_instrument_is_spm_and_hard
		),
	GNOME_RES_ENTRY_ASK_PATH_OPTION_FMT_DEPEND
	( GNOME_RES_INT, "Analog/VY-default","1", &xsmres.VYdefault, AnalogVXYZIndexList, 
	  "# %s -> x %g", xsmres.V,
	  N_("Inst-SPM"),
	  N_("Default Amplifier Positionindex [0,1,2,..8] used by Y-Channel"),
	  ask_if_instrument_is_spm_and_hard
		),
	GNOME_RES_ENTRY_ASK_PATH_OPTION_FMT_DEPEND
	( GNOME_RES_INT,    "Analog/VZ-default","2", &xsmres.VZdefault, AnalogVXYZIndexList, 
	  "# %s -> x %g", xsmres.V,
	  N_("Inst-SPM"),
	  N_("Default Amplifier Positionindex [0,1,2,..8] used by Z-Channel"),
	  ask_if_instrument_is_spm_and_hard
		),

	GNOME_RES_ENTRY_ASK_PATH_OPTION_FMT_DEPEND
	( GNOME_RES_INT, "Analog/VX0-default","1", &xsmres.VX0default, AnalogVXYZIndexList, 
	  "# %s -> x %g", xsmres.V,
	  N_("Inst-SPM"),
	  N_("Default Amplifier Positionindex [0,1,2,..8] used by X-Offset-Channel"),
	  ask_if_instrument_is_spm_and_hard
		),
	GNOME_RES_ENTRY_ASK_PATH_OPTION_FMT_DEPEND
	( GNOME_RES_INT, "Analog/VY0-default","1", &xsmres.VY0default, AnalogVXYZIndexList, 
	  "# %s -> x %g", xsmres.V,
	  N_("Inst-SPM"),
	  N_("Default Amplifier Positionindex [0,1,2,..8] used by Y-Offset-Channel"),
	  ask_if_instrument_is_spm_and_hard
		),
	GNOME_RES_ENTRY_ASK_PATH_OPTION_FMT_DEPEND
	( GNOME_RES_INT,    "Analog/VZ0-default","2", &xsmres.VZ0default, AnalogVXYZIndexList, 
	  "# %s -> x %g", xsmres.V,
	  N_("Inst-SPM"),
	  N_("Default Amplifier Positionindex [0,1,2,..8] used by Z-Offset-Channel"),
	  ask_if_instrument_is_spm_and_hard
          ),
#endif
	GNOME_RES_ENTRY_AUTO_PATH_OPTION
	( GNOME_RES_BOOL, "Analog/Analog-Offset-Adding", "false", &xsmres.AnalogOffsetAdding, TrueFalseList, N_("Inst-SPM"), 
	  N_("Offset adding modus for piezo amplifier:\nchoose from:\n 'DSP offset adding': false\n 'Analog offset adding': true")
		),

	GNOME_RES_ENTRY_FLOAT_VEC3
	( "Analog/AmpModulationGains", "AmpModulationGains", "1.0 1.0 1.0",  xsmres.XYZ_modulation_gains, N_("Inst-SPM"),
	  N_("setup HV Amplifier modulation input gains -- used for Z slope compensation"), NULL
		),

	GNOME_RES_ENTRY_INFOTEXT_DEPEND
	( N_("Inst-SPA"),
	  N_("SPA-LEED specific Volt to physical unit conversions.\n\n"
	     "In addition some geometry parameters and sample properties are required.\n"
	     ""),
	  ask_if_instrument_is_spaleed_and_hard
		),
	GNOME_RES_ENTRY_ASK_PATH_DEPEND
	( GNOME_RES_FLOAT,  "InstrumentSpa/XCalibV","10.0", &xsmres.XCalibVA, N_("Inst-SPA"),
	  N_("SPALEED: X Calibration factor 1V am DA => +/-10V/15V at oktopol rear/front"), 
	  ask_if_instrument_is_spaleed_and_hard
		),
	GNOME_RES_ENTRY_ASK_PATH_DEPEND
	( GNOME_RES_FLOAT,  "InstrumentSpa/YCalibV","10.0", &xsmres.YCalibVA, N_("Inst-SPA"),
	  N_("SPALEED: Y Calibration factor 1V am DA => +/-10V/15V at oktopol rear/front"), 
	  ask_if_instrument_is_spaleed_and_hard
		),
	GNOME_RES_ENTRY_ASK_PATH_DEPEND_UNIT
	( GNOME_RES_FLOAT_W_UNIT,  "InstrumentSpa/EnergyCalibVeV","100", &xsmres.EnergyCalibVeV, UniteVV, N_("Inst-SPA"),
	  N_("SPA-LEED: Energy 2 Volt Factor"), 
	  ask_if_instrument_is_spaleed_and_hard
		),
	GNOME_RES_ENTRY_ASK_PATH_DEPEND_UNIT
	( GNOME_RES_FLOAT_W_UNIT,  "InstrumentSpa/Sensitivity","50.0", &xsmres.Sensitivity, UnitSensi, N_("Inst-SPA"),
	  N_("SPA-LEED: default Sensitivity"), 
	  ask_if_instrument_is_spaleed_and_hard
		),
	GNOME_RES_ENTRY_ASK_PATH_DEPEND_UNIT
	( GNOME_RES_FLOAT_W_UNIT,  "InstrumentSpa/ThetaChGunInt","2.5", &xsmres.ThetaChGunInt, UnitDeg, N_("Inst-SPA"),
	  N_("SPA-LEED: half angle between E-gun and Channeltron in internal Geom."), 
	  ask_if_instrument_is_spaleed_and_hard
		),
	GNOME_RES_ENTRY_ASK_PATH_DEPEND_UNIT
	( GNOME_RES_FLOAT_W_UNIT,  "InstrumentSpa/ThetaChGunExt","57.5", &xsmres.ThetaChGunExt, UnitDeg, N_("Inst-SPA"),
	  N_("SPA-LEED: half angle between E-gun and Channeltron in external Geom."),
	  ask_if_instrument_is_spaleed_and_hard
		),
  // Sample SPA-LEED
	GNOME_RES_ENTRY_ASK_PATH_DEPEND_UNIT
	( GNOME_RES_FLOAT_W_UNIT,  "Sample/LayerDist","3.141", &xsmres.SampleLayerDist, UnitA, N_("Inst-SPA"),
	  N_("SPA_LEED: Sample Layer Distance d0 [A] - used for Phase S calculations"), 
	  ask_if_instrument_is_spaleed_and_hard
		),
	GNOME_RES_ENTRY_ASK_PATH_DEPEND_UNIT
	( GNOME_RES_FLOAT_W_UNIT,  "Sample/UnitLen","5.431", &xsmres.SampleUnitLen, UnitA, N_("Inst-SPA"),
	  N_("SPA-LEED: Sample Unit Cell Length a0 [A] - used for Phase K calibration"), 
	  ask_if_instrument_is_spaleed_and_hard
		),
  
  
  // sample Channelconfiguration AFM
	GNOME_RES_ENTRY_SEPARATOR (N_("DataAq"), 
				   N_("MK2-A810 Notes: (refer to manual for older SRanger model)\n"
				      "PID Input source signals are always A0..A4 for mix mode MK2-A810\n"
				      "and the resulting computed/adjsted Topo (Z out) is always mapped to PIDSrcA1,\n"
				      "Note Mk2-A810: X,Y,Z-Offset only are Out 0,1,2.\n"
				      "X,Y,Z-Scan out are Out 3,4,5 and bias 6. Out 7 is auxillary (M,Phase,...)"
				      )
				   ),

	GNOME_RES_ENTRY_AUTO_PATH
	( GNOME_RES_STRING, "DataAq/PIDSrcA1", "Topo,*", xsmres.pidsrc[0], N_("DataAq"),
	  MANUAL_REF_CHANCONF
		),
	GNOME_RES_ENTRY_AUTO_PATH_OPTION
	( GNOME_RES_STRING, "DataAq/PIDSrcA1-ZUnit", "AA", &xsmres.pidsrcZunit[0], UserZUnitList, N_("DataAq"),
	  MANUAL_REF_CHANCONF
		),
	GNOME_RES_ENTRY_AUTO_PATH_OPTION
	( GNOME_RES_STRING, "DataAq/PIDSrcA1-ZType", "FLOAT", &xsmres.pidsrcZtype[0], UserZTypeList, N_("DataAq"),
	  MANUAL_REF_CHANCONF
		),
	GNOME_RES_ENTRY_AUTO_PATH
	( GNOME_RES_STRING, "DataAq/PIDSrcA1Z-Label", "Z", &xsmres.pidsrcZlabel[0], N_("DataAq"),
	  MANUAL_REF_CHANCONF
		),

	GNOME_RES_ENTRY_SEPARATOR (N_("DataAq"), NULL),

	GNOME_RES_ENTRY_AUTO_PATH
	( GNOME_RES_STRING, "DataAq/PIDSrcA2", "MIX1", xsmres.pidsrc[1], N_("DataAq"),
	  MANUAL_REF_CHANCONF
		),
	GNOME_RES_ENTRY_AUTO_PATH_OPTION
	( GNOME_RES_STRING, "DataAq/PIDSrcA2-ZUnit", "V", &xsmres.pidsrcZunit[1], UserZUnitList, N_("DataAq"),
	  MANUAL_REF_CHANCONF
		),
	GNOME_RES_ENTRY_AUTO_PATH_OPTION
	( GNOME_RES_STRING, "DataAq/PIDSrcA2-ZType", "FLOAT", &xsmres.pidsrcZtype[1], UserZTypeList, N_("DataAq"),
	  MANUAL_REF_CHANCONF
		),
	GNOME_RES_ENTRY_AUTO_PATH
	( GNOME_RES_STRING, "DataAq/PIDSrcA2-ZLabel", "U", &xsmres.pidsrcZlabel[1], N_("DataAq"),
	  MANUAL_REF_CHANCONF
		),

	GNOME_RES_ENTRY_SEPARATOR (N_("DataAq"), NULL),

	GNOME_RES_ENTRY_AUTO_PATH
	( GNOME_RES_STRING, "DataAq/PIDSrcA3", "MIX2", xsmres.pidsrc[2], N_("DataAq"),
	  MANUAL_REF_CHANCONF
		),
	GNOME_RES_ENTRY_AUTO_PATH_OPTION
	( GNOME_RES_STRING, "DataAq/PIDSrcA3-ZUnit", "V", &xsmres.pidsrcZunit[2], UserZUnitList, N_("DataAq"),
	  MANUAL_REF_CHANCONF
		),
	GNOME_RES_ENTRY_AUTO_PATH_OPTION
	( GNOME_RES_STRING, "DataAq/PIDSrcA3-ZType", "FLOAT", &xsmres.pidsrcZtype[2], UserZTypeList, N_("DataAq"),
	  MANUAL_REF_CHANCONF
		),
	GNOME_RES_ENTRY_AUTO_PATH
	( GNOME_RES_STRING, "DataAq/PIDSrcA3-ZLabel", "U", &xsmres.pidsrcZlabel[2], N_("DataAq"),
	  MANUAL_REF_CHANCONF
		),

	GNOME_RES_ENTRY_SEPARATOR (N_("DataAq"), NULL),

	GNOME_RES_ENTRY_AUTO_PATH
	( GNOME_RES_STRING, "DataAq/PIDSrcA4", "MIX3", xsmres.pidsrc[3], N_("DataAq"),
	  MANUAL_REF_CHANCONF
		),
	GNOME_RES_ENTRY_AUTO_PATH_OPTION
	( GNOME_RES_STRING, "DataAq/PIDSrcA4-ZUnit", "V", &xsmres.pidsrcZunit[3], UserZUnitList, N_("DataAq"),
	  MANUAL_REF_CHANCONF
		),
	GNOME_RES_ENTRY_AUTO_PATH_OPTION
	( GNOME_RES_STRING, "DataAq/PIDSrcA4-ZType", "FLOAT", &xsmres.pidsrcZtype[3], UserZTypeList, N_("DataAq"),
	  MANUAL_REF_CHANCONF
		),
	GNOME_RES_ENTRY_AUTO_PATH
	( GNOME_RES_STRING, "DataAq/PIDSrcA4-ZLabel", "U", &xsmres.pidsrcZlabel[3], N_("DataAq"),
	  MANUAL_REF_CHANCONF
		),

	GNOME_RES_ENTRY_SEPARATOR (N_("DataAq"), 
				   N_("Source Channels for data acquisition:\n"
				      "A1..A4 -> MK2-A810: ADC0..3")
				   ),

	GNOME_RES_ENTRY_AUTO_PATH
	( GNOME_RES_STRING, "DataAq/DataSrcA1", "ADC0-ITunnel", xsmres.daqsrc[0], N_("DataAq"),
	  MANUAL_REF_CHANCONF
		),
	GNOME_RES_ENTRY_AUTO_PATH_OPTION
	( GNOME_RES_STRING, "DataAq/DataSrcA1-ZUnit", "nA", &xsmres.daqZunit[0], UserZUnitList, N_("DataAq"),
	  MANUAL_REF_CHANCONF
		),
	GNOME_RES_ENTRY_AUTO_PATH_OPTION
	( GNOME_RES_STRING, "DataAq/DataSrcA1-ZType", "FLOAT", &xsmres.daqZtype[0], UserZTypeList, N_("DataAq"),
	  MANUAL_REF_CHANCONF
		),
	GNOME_RES_ENTRY_AUTO_PATH
	( GNOME_RES_STRING, "DataAq/DataSrcA1-ZLabel", "I", &xsmres.daqZlabel[0], N_("DataAq"),
	  MANUAL_REF_CHANCONF
		),

	GNOME_RES_ENTRY_SEPARATOR (N_("DataAq"), NULL),

	GNOME_RES_ENTRY_AUTO_PATH
	( GNOME_RES_STRING, "DataAq/DataSrcA2", "ADC1", xsmres.daqsrc[1], N_("DataAq"),
	  MANUAL_REF_CHANCONF
		),
	GNOME_RES_ENTRY_AUTO_PATH_OPTION
	( GNOME_RES_STRING, "DataAq/DataSrcA2-ZUnit", "V", &xsmres.daqZunit[1], UserZUnitList, N_("DataAq"),
	  MANUAL_REF_CHANCONF
		),
	GNOME_RES_ENTRY_AUTO_PATH_OPTION
	( GNOME_RES_STRING, "DataAq/DataSrcA2-ZType", "FLOAT", &xsmres.daqZtype[1], UserZTypeList, N_("DataAq"),
	  MANUAL_REF_CHANCONF
		),
	GNOME_RES_ENTRY_AUTO_PATH
	( GNOME_RES_STRING, "DataAq/DataSrcA2-ZLabel", "ADC1", &xsmres.daqZlabel[1], N_("DataAq"),
	  MANUAL_REF_CHANCONF
		),

	GNOME_RES_ENTRY_SEPARATOR (N_("DataAq"), NULL),

	GNOME_RES_ENTRY_AUTO_PATH
	( GNOME_RES_STRING, "DataAq/DataSrcA3", "ADC2", xsmres.daqsrc[2], N_("DataAq"),
	  MANUAL_REF_CHANCONF
		),
	GNOME_RES_ENTRY_AUTO_PATH_OPTION
	( GNOME_RES_STRING, "DataAq/DataSrcA3-ZUnit", "V", &xsmres.daqZunit[2], UserZUnitList, N_("DataAq"),
	  MANUAL_REF_CHANCONF
		),
	GNOME_RES_ENTRY_AUTO_PATH_OPTION
	( GNOME_RES_STRING, "DataAq/DataSrcA3-ZType", "FLOAT", &xsmres.daqZtype[2], UserZTypeList, N_("DataAq"),
	  MANUAL_REF_CHANCONF
		),
	GNOME_RES_ENTRY_AUTO_PATH
	( GNOME_RES_STRING, "DataAq/DataSrcA3-ZLabel", "ADC2", &xsmres.daqZlabel[2], N_("DataAq"),
	  MANUAL_REF_CHANCONF
		),

	GNOME_RES_ENTRY_SEPARATOR (N_("DataAq"), NULL),

 	GNOME_RES_ENTRY_AUTO_PATH
	( GNOME_RES_STRING, "DataAq/DataSrcA4", "ADC3", xsmres.daqsrc[3], N_("DataAq"),
	  MANUAL_REF_CHANCONF
		),
	GNOME_RES_ENTRY_AUTO_PATH_OPTION
	( GNOME_RES_STRING, "DataAq/DataSrcA4-ZUnit", "V", &xsmres.daqZunit[3], UserZUnitList, N_("DataAq"),
	  MANUAL_REF_CHANCONF
		),
	GNOME_RES_ENTRY_AUTO_PATH_OPTION
	( GNOME_RES_STRING, "DataAq/DataSrcA4-ZType", "FLOAT", &xsmres.daqZtype[3], UserZTypeList, N_("DataAq"),
	  MANUAL_REF_CHANCONF
		),
	GNOME_RES_ENTRY_AUTO_PATH
	( GNOME_RES_STRING, "DataAq/DataSrcA4-ZLabel", "ADC3", &xsmres.daqZlabel[3], N_("DataAq"),
	  MANUAL_REF_CHANCONF
		),

	GNOME_RES_ENTRY_SEPARATOR (N_("DataAq"), 
				   N_("Source Channels for data acquisition:\n"
				      "B1..4 MK2-A810: ADC4..8")
				   ),

	GNOME_RES_ENTRY_AUTO_PATH
	( GNOME_RES_STRING, "DataAq/DataSrcB1", "ADC4", xsmres.daqsrc[4], N_("DataAq"),
	  MANUAL_REF_CHANCONF
		),
	GNOME_RES_ENTRY_AUTO_PATH_OPTION
	( GNOME_RES_STRING, "DataAq/DataSrcB1-ZUnit", "V", &xsmres.daqZunit[4], UserZUnitList, N_("DataAq"),
	  MANUAL_REF_CHANCONF
		),
	GNOME_RES_ENTRY_AUTO_PATH_OPTION
	( GNOME_RES_STRING, "DataAq/DataSrcB1-ZType", "FLOAT", &xsmres.daqZtype[4], UserZTypeList, N_("DataAq"),
	  MANUAL_REF_CHANCONF
		),
	GNOME_RES_ENTRY_AUTO_PATH
	( GNOME_RES_STRING, "DataAq/DataSrcB1-ZLabel", "ADC4", &xsmres.daqZlabel[4], N_("DataAq"),
	  MANUAL_REF_CHANCONF
		),

	GNOME_RES_ENTRY_SEPARATOR (N_("DataAq"), NULL),

	GNOME_RES_ENTRY_AUTO_PATH
	( GNOME_RES_STRING, "DataAq/DataSrcB2", "ADC5", xsmres.daqsrc[5], N_("DataAq"),
	  MANUAL_REF_CHANCONF
		),
	GNOME_RES_ENTRY_AUTO_PATH_OPTION
	( GNOME_RES_STRING, "DataAq/DataSrcB2-ZUnit", "V", &xsmres.daqZunit[5], UserZUnitList, N_("DataAq"),
	  MANUAL_REF_CHANCONF
		),
	GNOME_RES_ENTRY_AUTO_PATH_OPTION
	( GNOME_RES_STRING, "DataAq/DataSrcB2-ZType", "FLOAT", &xsmres.daqZtype[5], UserZTypeList, N_("DataAq"),
	  MANUAL_REF_CHANCONF
		),
	GNOME_RES_ENTRY_AUTO_PATH
	( GNOME_RES_STRING, "DataAq/DataSrcB2-ZLabel", "ADC5", &xsmres.daqZlabel[5], N_("DataAq"),
	  MANUAL_REF_CHANCONF
		),

	GNOME_RES_ENTRY_SEPARATOR (N_("DataAq"), NULL),

	GNOME_RES_ENTRY_AUTO_PATH
	( GNOME_RES_STRING, "DataAq/DataSrcB3", "ADC6", xsmres.daqsrc[6], N_("DataAq"),
	  MANUAL_REF_CHANCONF
		),
	GNOME_RES_ENTRY_AUTO_PATH_OPTION
	( GNOME_RES_STRING, "DataAq/DataSrcB3-ZUnit", "V", &xsmres.daqZunit[6], UserZUnitList, N_("DataAq"),
	  MANUAL_REF_CHANCONF
		),
	GNOME_RES_ENTRY_AUTO_PATH_OPTION
	( GNOME_RES_STRING, "DataAq/DataSrcB3-ZType", "FLOAT", &xsmres.daqZtype[6], UserZTypeList, N_("DataAq"),
	  MANUAL_REF_CHANCONF
		),
	GNOME_RES_ENTRY_AUTO_PATH
	( GNOME_RES_STRING, "DataAq/DataSrcB3-ZLabel", "ADC6", &xsmres.daqZlabel[6], N_("DataAq"),
	  MANUAL_REF_CHANCONF
		),

	GNOME_RES_ENTRY_SEPARATOR (N_("DataAq"), NULL),

	GNOME_RES_ENTRY_AUTO_PATH
	( GNOME_RES_STRING, "DataAq/DataSrcB4", "ADC7", xsmres.daqsrc[7], N_("DataAq"),
	  MANUAL_REF_CHANCONF
		),
	GNOME_RES_ENTRY_AUTO_PATH_OPTION
	( GNOME_RES_STRING, "DataAq/DataSrcB4-ZUnit", "V", &xsmres.daqZunit[7], UserZUnitList, N_("DataAq"),
	  MANUAL_REF_CHANCONF
		),
	GNOME_RES_ENTRY_AUTO_PATH_OPTION
	( GNOME_RES_STRING, "DataAq/DataSrcB4-ZType", "FLOAT", &xsmres.daqZtype[7], UserZTypeList, N_("DataAq"),
	  MANUAL_REF_CHANCONF
		),
	GNOME_RES_ENTRY_AUTO_PATH
	( GNOME_RES_STRING, "DataAq/DataSrcB4-ZLabel", "ADC7", &xsmres.daqZlabel[7], N_("DataAq"),
	  MANUAL_REF_CHANCONF
		),


	GNOME_RES_ENTRY_SEPARATOR (N_("DataAq"), 
				   N_("Source Channels C1,D1:\n"
				      "DSP generated data")
				   ),

	GNOME_RES_ENTRY_AUTO_PATH
	( GNOME_RES_STRING, "DataAq/DataSrcC1", "dIdV", xsmres.daqsrc[8], N_("DataAq"),
	  MANUAL_REF_CHANCONF
		),
	GNOME_RES_ENTRY_AUTO_PATH_OPTION
	( GNOME_RES_STRING, "DataAq/DataSrcC1-ZUnit", "a.u.", &xsmres.daqZunit[8], UserZUnitList, N_("DataAq"),
	  MANUAL_REF_CHANCONF
		),
	GNOME_RES_ENTRY_AUTO_PATH_OPTION
	( GNOME_RES_STRING, "DataAq/DataSrcC1-ZType", "FLOAT", &xsmres.daqZtype[8], UserZTypeList, N_("DataAq"),
	  MANUAL_REF_CHANCONF
		),
	GNOME_RES_ENTRY_AUTO_PATH
	( GNOME_RES_STRING, "DataAq/DataSrcC1-ZLabel", "LockIn1st", &xsmres.daqZlabel[8], N_("DataAq"),
	  MANUAL_REF_CHANCONF
		),

	GNOME_RES_ENTRY_SEPARATOR (N_("DataAq"), NULL),

	GNOME_RES_ENTRY_AUTO_PATH
	( GNOME_RES_STRING, "DataAq/DataSrcD1", "ddIdV", xsmres.daqsrc[9], N_("DataAq"),
	  MANUAL_REF_CHANCONF
		),
	GNOME_RES_ENTRY_AUTO_PATH_OPTION
	( GNOME_RES_STRING, "DataAq/DataSrcD1-ZUnit", "a.u.", &xsmres.daqZunit[9], UserZUnitList, N_("DataAq"),
	  MANUAL_REF_CHANCONF
		),
	GNOME_RES_ENTRY_AUTO_PATH_OPTION
	( GNOME_RES_STRING, "DataAq/DataSrcD1-ZType", "FLOAT", &xsmres.daqZtype[9], UserZTypeList, N_("DataAq"),
	  MANUAL_REF_CHANCONF
		),
	GNOME_RES_ENTRY_AUTO_PATH
	( GNOME_RES_STRING, "DataAq/DataSrcD1-ZLabel", "LockIn2nd", &xsmres.daqZlabel[9], N_("DataAq"),
	  MANUAL_REF_CHANCONF
		),

	GNOME_RES_ENTRY_SEPARATOR (N_("DataAq"), NULL),

	GNOME_RES_ENTRY_AUTO_PATH
	( GNOME_RES_STRING, "DataAq/DataSrcE1", "I0-avg", xsmres.daqsrc[10], N_("DataAq"),
	  MANUAL_REF_CHANCONF
		),
	GNOME_RES_ENTRY_AUTO_PATH_OPTION
	( GNOME_RES_STRING, "DataAq/DataSrcE1-ZUnit", "a.u.", &xsmres.daqZunit[10], UserZUnitList, N_("DataAq"),
	  MANUAL_REF_CHANCONF
		),
	GNOME_RES_ENTRY_AUTO_PATH_OPTION
	( GNOME_RES_STRING, "DataAq/DataSrcE1-ZType", "FLOAT", &xsmres.daqZtype[10], UserZTypeList, N_("DataAq"),
	  MANUAL_REF_CHANCONF
		),
	GNOME_RES_ENTRY_AUTO_PATH
	( GNOME_RES_STRING, "DataAq/DataSrcE1-ZLabel", "LockIn0", &xsmres.daqZlabel[10], N_("DataAq"),
	  MANUAL_REF_CHANCONF
		),

	GNOME_RES_ENTRY_SEPARATOR (N_("DataAq"), NULL),

	GNOME_RES_ENTRY_AUTO_PATH
	( GNOME_RES_STRING, "DataAq/DataSrcF1", "Counter", xsmres.daqsrc[11], N_("DataAq"),
	  MANUAL_REF_CHANCONF
		),
	GNOME_RES_ENTRY_AUTO_PATH_OPTION
	( GNOME_RES_STRING, "DataAq/DataSrcF1-ZUnit", "Events", &xsmres.daqZunit[11], UserZUnitList, N_("DataAq"),
	  MANUAL_REF_CHANCONF
		),
	GNOME_RES_ENTRY_AUTO_PATH_OPTION
	( GNOME_RES_STRING, "DataAq/DataSrcF1-ZType", "FLOAT", &xsmres.daqZtype[11], UserZTypeList, N_("DataAq"),
	  MANUAL_REF_CHANCONF
		),
	GNOME_RES_ENTRY_AUTO_PATH
	( GNOME_RES_STRING, "DataAq/DataSrcF1-ZLabel", "Counts", &xsmres.daqZlabel[11], N_("DataAq"),
	  MANUAL_REF_CHANCONF
		),

  // Data storage defaults
	GNOME_RES_ENTRY_AUTO_PATH
	( GNOME_RES_STRING, "Paths/Logfiles", "~", &xsmres.LogFilePath, N_("Paths"),
	  N_("where to put logfiles")
		),
	GNOME_RES_ENTRY_AUTO_PATH
	( GNOME_RES_STRING, "Paths/Data", ".", &xsmres.DataPath, N_("Paths"),
	  N_("default data path")
		),
	GNOME_RES_ENTRY_AUTO_PATH
	( GNOME_RES_STRING, "Paths/RemoteFifo", "remote", &xsmres.RemoteFifo, N_("Paths"),
	  N_("path/name of remote control fifo, read cmds")
		),
	GNOME_RES_ENTRY_AUTO_PATH
	( GNOME_RES_STRING, "Paths/RemoteFifoOut", "remoteecho", &xsmres.RemoteFifoOut, N_("Paths"),
	  N_("path/name of remote control fifo, status out")
		),
	GNOME_RES_ENTRY_AUTO_PATH
	( GNOME_RES_STRING, "Paths/PyremoteFile", "remote", &xsmres.PyremoteFile, N_("Paths"),
	  N_("path/name of python-remote file, without path and .py suffix, must be in cwd.")
		),
	GNOME_RES_ENTRY_AUTO_PATH
	( GNOME_RES_STRING, "Paths/Plugins", ".", &xsmres.UserPluginPath, N_("Paths"),
	  N_("searchpath to additional GxsmPlugins")
		),
	GNOME_RES_ENTRY_AUTO_PATH
	( GNOME_RES_STRING, "Paths/GxsmPalette", PACKAGE_PALETTE_DIR, &xsmres.GxsmPalettePath, N_("Paths"),
	  N_("searchpath to Gxsm Palette Pixmaps (1024x1 ACSII .pnm)")
		),
	GNOME_RES_ENTRY_AUTO_PATH
	( GNOME_RES_STRING, "Paths/UserPalette", ".", &xsmres.UserPalettePath, N_("Paths"),
	  N_("searchpath to additional Gxsm Palette Pixmaps (1024x1 ACSII .pnm)")
		),
	GNOME_RES_ENTRY_AUTO_PATH
	( GNOME_RES_STRING, "Paths/ProfileXColorsPath", "profile_colors.xcolor", &xsmres.ProfileXColorsPath, N_("Paths"),
	  N_("searchpath to optional list of XColors used for multiple profiles, file like rgb.txt or xcolors only)")
		),
  
	// User configuration
	GNOME_RES_ENTRY_INFOTEXT
	( N_("User"),
	  N_("User interface specific adjustments.\n"
	     "Have a look at the Preferences Dialog later for some more...\n"
	     "")
		),
	GNOME_RES_ENTRY_AUTO_PATH_OPTION
	( GNOME_RES_BOOL, "User/SaveWindowGeometry", "true", &xsmres.geomsave, TrueFalseList, N_("User"), 
	  N_("bool: true=save window positions, false: do not")
		),
	GNOME_RES_ENTRY_ASK_PATH_OPTION
	( GNOME_RES_STRING, "User/XYUnit","AA", &xsmres.Unit, UserXYUnitList, N_("User"),
	  N_("XY Unit alias, choose one of: AA (" UTF8_ANGSTROEM "), nm, um (" UTF8_MU "m), mm, BZ, sec, V, 1")
		),
	GNOME_RES_ENTRY_ASK_PATH
	( GNOME_RES_INT, "User/HiLoDelta", "2", &xsmres.HiLoDelta, N_("User"), 
	  N_("Gridsize used by Autodisp, e.g: 1: all Pixels are examined")
		),
	GNOME_RES_ENTRY_ASK_PATH
	( GNOME_RES_FLOAT, "User/SmartHistEpsilon", "0.05", &xsmres.SmartHistEpsilon, N_("User"), 
	  N_("Smart Histogram Epsilon for hitogram evaluanted range compression (positive) or expand (negative).")
		),

	GNOME_RES_ENTRY_SEPARATOR (N_("User"), N_("Y limit in log view of profile.")),

	GNOME_RES_ENTRY_AUTO_PATH
	( GNOME_RES_FLOAT,  "User/ProfileLogLimit","1e-20", &xsmres.ProfileLogLimit, N_("User"),
	  N_("Minimal Y value allwed in profile view in log view mode.")
		),
  
	GNOME_RES_ENTRY_SEPARATOR (N_("User"), NULL),

	GNOME_RES_ENTRY_AUTO_PATH_OPTION
	( GNOME_RES_STRING, "User/FileNameConvention", "digit", &xsmres.FileNameConvention, UserFileNameConventionList, N_("User"),
	  N_("File numbering mode: digit, alpha")
		),

	// Autosave improvements
	GNOME_RES_ENTRY_AUTO_PATH_OPTION
	( GNOME_RES_STRING, "User/AutosaveUnit", "percent", &xsmres.AutosaveUnit, UserAutosaveUnitList, N_("User"), 
	  N_("Autosaveunit: Save every percent, lines, seconds")
		),
	GNOME_RES_ENTRY_AUTO_PATH
	( GNOME_RES_INT, "User/AutosaveValue", "100", &xsmres.AutosaveValue, N_("User"), 
	  N_("Rate of Autosaves, depending on Unit.")
	  ),
	GNOME_RES_ENTRY_AUTO_PATH_OPTION
	( GNOME_RES_STRING, "User/AutosaveOverwritemode", "false", &xsmres.AutosaveOverwritemode, TrueFalseList, N_("User"), 
	  N_("true = Overwrite unasked during Autosave, false = ask before overwrite")
	  ),


	GNOME_RES_ENTRY_SEPARATOR (N_("User"), NULL),
	GNOME_RES_ENTRY_AUTO_PATH
	( GNOME_RES_INT, "User/LoadDelay", "10", &xsmres.LoadDelay, N_("User"), 
	  N_("Delay after loading file in 1/10s:\n"
	     "if zero no waiting for pending events.")
	  ),
	GNOME_RES_ENTRY_FLOAT_VEC4
	( "User/LoadCorrectXYZ", "CorrectXYZ", "1.0 1.0 1.0 1.0", &xsmres.LoadCorrectXYZ, N_("User"),
	  N_("set NC file load XYZ scale correction factor vector [Xs, Ys, ZsTopo, Zs*]. Default: no change, set to [1,1,1,1].\n"
             "ZsTopo -- applies to Z of Topo dedicated data, Zs* to every thing else -- strongly advised to leave that at 1."
	     "XYZ scale correction factor vector [Xs, Ys, ZsTopo, Zs*]] is applied to scan scale (dx,dy,dz, rx,..) at NC-file load time once.\n"
             "This may be used to correct slight piezo calibartion deviations or temperature related scalings.\n"
             "Only on the fly applied at scan laod time. But you may save the file again, then it's applied."
             ), NULL
        ),


	GNOME_RES_ENTRY_SEPARATOR (N_("User"), NULL),

	GNOME_RES_ENTRY_ASK_PATH_OPTION_DEPEND
	( GNOME_RES_STRING, "User/SliderControlType", "mover", &xsmres.SliderControlType, UserSliderTypeList, N_("User"),
	  N_("Slider/Approch control type:\n"
	     " slider: Omicron Box/similar, single steps via DSP's digital IO,\n"
	     " mover: Mover/Besocke via sawtooth like ramp voltages from Piezodrive"),
	  ask_if_instrument_is_spm_and_hard
		),

	GNOME_RES_ENTRY_ASK_PATH_OPTION
	( GNOME_RES_STRING, "User/Palette", PACKAGE_PALETTE_DIR G_DIR_SEPARATOR_S "xxsmpalette.pnm", &xsmres.Palette,  (const gchar**)xsmres.PalPathList, N_("User"), 
	  N_("Path/name of your favorite palette, (1024x1 pnm RGB Bitmap (ascii))")
		),

	GNOME_RES_ENTRY_SEPARATOR (N_("User"), NULL),

	GNOME_RES_ENTRY_ASK_PATH
	( GNOME_RES_STRING, "User/griplottitle", "Gxsm Profile Plot", &xsmres.griplottitle, N_("User"), 
	  N_("Title used in plots")
		),
	GNOME_RES_ENTRY_ASK_PATH
	( GNOME_RES_STRING, "User/command1", PACKAGE_PROFILEPLOT_DIR G_DIR_SEPARATOR_S "gxsmplot1d-1.gri.sh", &xsmres.gricmd1d[0], N_("User"), 
	  N_("Path/name of your 1d profile plot file #1")
		),
	GNOME_RES_ENTRY_ASK_PATH
	( GNOME_RES_STRING, "User/command2", PACKAGE_PROFILEPLOT_DIR G_DIR_SEPARATOR_S "gxsmplot1d-2.gri.sh", &xsmres.gricmd1d[1], N_("User"), 
	  N_("Path/name of your 1d profile plot file #2")
		),
	GNOME_RES_ENTRY_ASK_PATH
	( GNOME_RES_STRING, "User/command3", PACKAGE_PROFILEPLOT_DIR G_DIR_SEPARATOR_S "gxsmplot1d-3.gri.sh", &xsmres.gricmd1d[2], N_("User"), 
	  N_("Path/name of your 1d profile plot file #3")
		),
	GNOME_RES_ENTRY_ASK_PATH
	( GNOME_RES_STRING, "User/command4", PACKAGE_PROFILEPLOT_DIR G_DIR_SEPARATOR_S "gxsmplot1d-4.gri.sh", &xsmres.gricmd1d[3], N_("User"), 
	  N_("Path/name of your 1d profile plot file #4")
		),
	GNOME_RES_ENTRY_ASK_PATH
	( GNOME_RES_STRING, "User/command5", PACKAGE_PROFILEPLOT_DIR G_DIR_SEPARATOR_S "gxsmplot1d-xmgrace.sh", &xsmres.gricmd1d[4], N_("User"), 
	  N_("Path/name of your 1d profile plot file #5")
		),
	GNOME_RES_ENTRY_ASK_PATH
	( GNOME_RES_STRING, "User/command6", PACKAGE_PROFILEPLOT_DIR G_DIR_SEPARATOR_S "gxsmplot1d-matplot.py", &xsmres.gricmd1d[5], N_("User"), 
	  N_("Path/name of your 1d profile plot file #6")
		),

	GNOME_RES_ENTRY_SEPARATOR (N_("User"), NULL),

	GNOME_RES_ENTRY_ASK_PATH
	( GNOME_RES_STRING, "User/2d-gricmdfile1", PACKAGE_PROFILEPLOT_DIR G_DIR_SEPARATOR_S "gxsmplot2d-1.gri", &xsmres.gricmd2d[0], N_("User"), 
	  N_("Path/name of your 2d gri cmd file #1")
		),
	GNOME_RES_ENTRY_ASK_PATH
	( GNOME_RES_STRING, "User/2d-gricmdfile2", PACKAGE_PROFILEPLOT_DIR G_DIR_SEPARATOR_S "gxsmplot2d-2.gri", &xsmres.gricmd2d[1], N_("User"), 
	  N_("Path/name of your 2d gri cmd file #2")
		),
	GNOME_RES_ENTRY_ASK_PATH
	( GNOME_RES_STRING, "User/2d-gricmdfile3", PACKAGE_PROFILEPLOT_DIR G_DIR_SEPARATOR_S "gxsmplot2d-3.gri", &xsmres.gricmd2d[2], N_("User"), 
	  N_("Path/name of your 2d gri cmd file #3")
		),
	GNOME_RES_ENTRY_ASK_PATH
	( GNOME_RES_STRING, "User/2d-gricmdfile4", PACKAGE_PROFILEPLOT_DIR G_DIR_SEPARATOR_S "gxsmplot2d-4.gri", &xsmres.gricmd2d[3], N_("User"), 
	  N_("Path/name of your 2d gri cmd file #4")
          ),

	GNOME_RES_ENTRY_SEPARATOR (N_("User"), NULL),

	GNOME_RES_ENTRY_COLORSEL
	( "User/ProfileFrameColor", "Profile Frame Color", "0.0 0.0 0.0 1.0", 
	  &xsmres.ProfileFrameColor, N_("User"),
	  N_("Profile Graph Color for canvas frame."), NULL
		),
	GNOME_RES_ENTRY_COLORSEL
	( "User/ProfileCanvasColor", "Profile Canvas Color", "1.0 1.0 1.0 1.0", 
	  &xsmres.ProfileCanvasColor, N_("User"),
	  N_("Profile Graph Color for canvas."), NULL
          ),

	GNOME_RES_ENTRY_FONTSEL
	( "User/ProfileTicFont", "Profile Ticmarks Font", "Unutu 11", 
	  &xsmres.ProfileTicFont, N_("User"), 
	  N_("Profile Graph Font for tickmarks."), 
	  NULL
          ),
	GNOME_RES_ENTRY_COLORSEL
	( "User/ProfileTicColor", "Profile Ticmark Color", "0.0 0.0 0.0 1.0", 
	  &xsmres.ProfileTicColor, N_("User"),
	  N_("Profile Graph Color for ticmarks."), NULL
          ),
	GNOME_RES_ENTRY_COLORSEL
	( "User/ProfileGridColor", "ProfileGridColor", "0.7 0.7 0.7 0.5", 
	  &xsmres.ProfileGridColor, N_("User"),
	  N_("Profile Graph Color for XY grid lines."), NULL
          ),

	GNOME_RES_ENTRY_FONTSEL
	( "User/ProfileLabelFont", "Profile Label Font", "Georgia 12", 
	  &xsmres.ProfileLabelFont, N_("User"), 
	  N_("Profile Graph Font for X and Y axis labels."), 
	  NULL
          ),
	GNOME_RES_ENTRY_COLORSEL
	( "User/ProfileLabelColor", "Profile Label Color", "0.0 0.0 0.0 1.0", 
	  &xsmres.ProfileLabelColor, N_("User"),
	  N_("Profile Graph Color for X and Y axis labels"), NULL
          ),

	GNOME_RES_ENTRY_FONTSEL
	( "User/ProfileTitleFont", "Profile Title Font", "Georgia 14", 
	  &xsmres.ProfileTitleFont, N_("User"), 
	  N_("Profile Graph Font for title."), 
	  NULL
          ),
	GNOME_RES_ENTRY_COLORSEL
	( "User/ProfileTitleColor", "Profile Title Color", "1.0 0.0 0.0 1.0", 
	  &xsmres.ProfileTitleColor, N_("User"),
	  N_("Profile Graph Color for title."), NULL
          ),

	GNOME_RES_ENTRY_AUTO_PATH
	( GNOME_RES_INT, "User/ProfileLineWidth", "1", &xsmres.ProfileLineWidth, N_("User"), 
	  N_("Line width used for profiles (pixels)")
	  ),

	GNOME_RES_ENTRY_AUTO_PATH
	( GNOME_RES_INT, "User/ProfileOrgMode", "0", &xsmres.LineProfileOrgMode, N_("User"), 
	  N_("Origin Modus for Line Profile: 0: ZeroLeft, 1: ZeroCenter, 2:PrjX, 3:PrjY")
	  ),

	GNOME_RES_ENTRY_SEPARATOR (N_("User"), NULL),

	GNOME_RES_ENTRY_AUTO_PATH
	( GNOME_RES_STRING, "User/RedLineHistoryColors", "LightSalmon,salmon,red", &xsmres.RedLineHistoryColors, N_("User"), 
	  N_("Commata separated list of xcolor names like: \"LightSalmon,salmon,red\" ")
	  ),
	GNOME_RES_ENTRY_AUTO_PATH
	( GNOME_RES_STRING, "User/BlueLineHistoryColors", "black,purple,blue", &xsmres.BlueLineHistoryColors, N_("User"), 
	  N_("Commata separated list of xcolor names like: \"black,purple,blue\" ")
	  ),
	GNOME_RES_ENTRY_AUTO_PATH
	( GNOME_RES_INT, "User/RedLineWidth", "1", &xsmres.RedLineWidth, N_("User"), 
	  N_("Line width for current/last RedLine (pixels)")
	  ),

	GNOME_RES_ENTRY_SEPARATOR (N_("User"), NULL),

	GNOME_RES_ENTRY_AUTO_PATH_OPTION
	( GNOME_RES_BOOL, "User/MenuTooltips", "true", &xsmres.menutooltips, TrueFalseList, N_("User"), 
	  N_("Enable Menu Tooltips: true|false")
		),

	GNOME_RES_ENTRY_AUTO_PATH_OPTION
	( GNOME_RES_BOOL, "User/AcceptDatK0", "false", &xsmres.datnullok, TrueFalseList, N_("User"),
	  N_("Accept Dat Kennung 0x000, Attention this is a safety hole if enabled !")
		),

	GNOME_RES_ENTRY_AUTO_PATH_OPTION
	( GNOME_RES_BOOL, "GUI/layerfields", "true", &xsmres.gui_layerfields, TrueFalseList, N_("GUI"),
	  N_("Select here, if you want use layered (3d) scans.")
		),

	GNOME_RES_ENTRY_SEPARATOR (N_("GUI"), NULL),

	GNOME_RES_ENTRY_AUTO_PATH_OPTION
	( GNOME_RES_STRING, "GUI/HandleType", GUIObjectHandleTypes[0], 
	  &xsmres.HandleType, GUIObjectHandleTypes, N_("GUI"), 
	  N_("Select shape for object handle type")
		),

	GNOME_RES_ENTRY_COLORSEL
	( "GUI/ObjHandleActiveBgColor", "ObjHandleActiveBgColor", "0 1 1 0.3", 
	  &xsmres.HandleActBgColor, N_("GUI"),
	  N_("Object Handle Bg Color, active [red, green, blue, alpha]"), NULL
		),

	GNOME_RES_ENTRY_COLORSEL
	( "GUI/ObjHandleInActiveBgColor", "ObjHandleInActiveBgColor", "0.6 0.6 0.6 0.3", 
	  &xsmres.HandleInActBgColor, N_("GUI"),
	  N_("Object Handle Bg Color, inactive [red, green, blue, alpha]"), NULL
		),

	GNOME_RES_ENTRY_AUTO_PATH
	( GNOME_RES_INT,  "GUI/HandleLineWidth","1", &xsmres.HandleLineWidth, N_("GUI"),
	  N_("Width of handle outline in pixels")
		),

	GNOME_RES_ENTRY_AUTO_PATH
	( GNOME_RES_FLOAT,  "GUI/HandleSize","100.0", &xsmres.HandleSize, N_("GUI"),
	  N_("Percentage of default size")
		),

	GNOME_RES_ENTRY_AUTO_PATH
	( GNOME_RES_INT,  "GUI/ObjectLineWidth","2", &xsmres.ObjectLineWidth, N_("GUI"),
	  N_("Line width of object (connections) in pixels")
		),

	GNOME_RES_ENTRY_COLORSEL
	( "GUI/ObjLabColor", "ObjectLabelColor", "1 0 0 1", 
	  &xsmres.ObjectLabColor, N_("GUI"),
	  N_("Object Label Color [red, green, blue, alpha]"), NULL
		),

	GNOME_RES_ENTRY_FONTSEL
	( "GUI/ObjLabFont", "Object Label Font", "Sans Bold 12", 
	  &xsmres.ObjectLabFont, N_("GUI"),
	  N_("Select object label font."), NULL
		),

	GNOME_RES_ENTRY_COLORSEL
	( "GUI/ObjElmColor", "Elm Color", "0 0 1 1", 
	  &xsmres.ObjectElmColor, N_("GUI"),
	  N_("Object Element Color [red, green, blue, alpha]"), NULL
		),

	GNOME_RES_ENTRY_COLORSEL
	( "GUI/ObjIntColor", "Int Color", "1 0 0 1", 
	  &xsmres.ObjectIntColor, N_("GUI"),
	  N_("Object Integration Marker Color [red, green, blue, alpha]"), NULL
		),

	GNOME_RES_ENTRY_COLORSEL
	( "GUI/OSDColor", "OSD Color", "1 0 0 1", 
	  &xsmres.OSDColor, N_("GUI"),
	  N_("OSD Text Color [red, green, blue, alpha]"), NULL
		),

	GNOME_RES_ENTRY_FONTSEL
	( "GUI/OSDFont", "OSD Font", "Sans Bold 8", 
	  &xsmres.OSDFont, N_("GUI"),
	  N_("Select OSD text font"), NULL
		),

	GNOME_RES_ENTRY_LAST
};

void gxsm_init_dynamic_res(){
	for (int i=0; i<MAXPALANZ; ++i)
		xsmres.PalPathList[i] = NULL;
	for (int i=0; i<PIDCHMAX; ++i)
		xsmres.pidsrcZd2u[i] = 0.; 
	for (int i=0; i<DAQCHMAX; ++i)
		xsmres.daqZd2u[i] = 0.;
}

// Directory File Selection Stuff
#ifdef IS_MACOSX
int select_pnm(struct dirent *item){ return !fnmatch("*.pnm", item->d_name, 0); }
#else
int select_pnm(const struct dirent *item){ return !fnmatch("*.pnm", item->d_name, 0); }
#endif

// Fill HwI selection list
void gxsm_search_for_HwI(){
	int j,k;

	for (k=0; HardwareCardListCore[k] && k<MAX_HWI-1; ++k)
		HardwareCardList[k] = HardwareCardListCore[k]; 	

        GSettings *gs_tmp = g_settings_new (GXSM_RES_BASE_PATH_DOT".hardware-interfaces");
        gchar *hwilist = g_settings_get_string (gs_tmp, "hwi-list");
        gchar **hwilistv = g_strsplit (hwilist, ";", MAX_HWI-2);
        
	for (j=0; hwilistv[j] && k<MAX_HWI-2; ++j, ++k)
                HardwareCardList[k] = hwilistv[j];
	
	HardwareCardList[k] = NULL;
        
        // g_strfreev (hwilistv); -- do not free!!
        g_free (hwilist);
        g_clear_object (&gs_tmp);
}

void gxsm_search_for_palette(){
	struct dirent **namelist;
	int n, palnum;

	palnum=0;

	n = scandir (xsmres.GxsmPalettePath, &namelist, select_pnm, alphasort);
	if (n < 0){
		gchar *message = g_strconcat ("Please check Gxsm preferences, Path/Palette: Could not find Gxsm palette files in <",
					      xsmres.GxsmPalettePath,
					      ">, reason is: ", g_strerror(errno),
					      NULL);
		XSM_DEBUG_WARNING (DBG_EVER, message);
		GtkWidget *dialog = gtk_message_dialog_new (gapp->get_window(),
							    GTK_DIALOG_DESTROY_WITH_PARENT,
							    GTK_MESSAGE_WARNING,
							    GTK_BUTTONS_OK,
							    "%s", message);
                g_signal_connect (dialog, "response", G_CALLBACK (gtk_window_destroy), NULL);
                gtk_widget_show (dialog);
		g_free (message);
	} else {
		while(n--) {
			if (palnum < MAXPALANZ){
				if (xsmres.PalPathList[palnum])
					g_free (xsmres.PalPathList[palnum]);

				xsmres.PalPathList[palnum++] = g_strconcat (xsmres.GxsmPalettePath, 
									    "/", 
									    namelist[n]->d_name, 
									    NULL);
			}
			free(namelist[n]);
		}
		free(namelist);
	}

	n = scandir(xsmres.UserPalettePath, &namelist, select_pnm, alphasort);
	if (n < 0)
		g_warning ("Could not find User palette files: %s\n",
			   g_strerror(errno));
	else {
		while(n--) {
			if (palnum < MAXPALANZ){
				if (xsmres.PalPathList[palnum])
					g_free (xsmres.PalPathList[palnum]);

				xsmres.PalPathList[palnum++] = g_strconcat (xsmres.UserPalettePath, 
									    "/", 
									    namelist[n]->d_name, 
									    NULL);
			}
			free(namelist[n]);
		}
		free(namelist);
	}

	xsmres.PalPathList[palnum] = NULL;
}
