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

/* RT TASK NUMBER ASSIGNMENTS */ // FIXED ALWAYS (0x10, EVEN 0x20, ODD 0x40) ASSIGNMENTS
#define RT_TASK_ADAPTIIR      1  // 0x10 A
#define RT_TASK_QEP           2  // 0x20 E
#define RT_TASK_RATEMETER     3  // 0x20 E
#define RT_TASK_FEEDBACK      4  // 0x20 E

#define RT_TASK_AREA_SCAN     5  // 0x40 O
#define RT_TASK_VECTOR_PROBE  6  // 0x40 O
#define RT_TASK_LOCKIN        7  // 0x40 O
#define RT_TASK_ANALOG_OUTHR  8  // ALWAYS 0x10 A fixed

#define RT_TASK_WAVEPLAY      9  // 0x10 A
#define RT_TASK_MSERVO       10  // ---- N/A 0x10 A
#define RT_TASK_RECORDER     11  // 0x10 A
#define RT_TASK_NONE_12      12  // ----

#ifndef DSP_CC

// FIX ME !!!! (port to MK2 in works)

// hard real time data process (DP) tasks
const gchar *MK2_dsp_dp_process_name[] = {
        "System", // RT000

        // RT Tasks
        "Adaptive IIR and Mixer",          // RT001
        "QEP, GATEING, COUNTER Expansion", // RT002
        "Ratemeter",                       // RT003
        "Z-Servo (Feedback)",              // RT004

        "Area Scan and XY Rotation+Slope", // RT005
        "Vector Probe",                    // RT006
        "Lock-In",                         // RT007
        "Analog OUT Mapping with HR",      // RT008

        "AutoApp and Wave Generator",      // RT009
        "M-servo",                         // RT010
        "Recorder",                        // RT011
        "--",
	NULL
};         

// non hard realtime Idle (ID) tasks and timer tasks
const gchar *MK2_dsp_id_process_name[] = {
        // Ilde Tasks
        // ID001
        "BZ push data from RT_FIFO",
        "Offset Move",
        "Vector Probe Manager",
        "Extern-Control",
        
        // ID005
        "Statemachine Control, exec lnx",
        "Feedback Mixer log update",
        "--SM--",
        "Offset Move init",

        // ID009
        "Scan Controls",
        "Probe Controls",
        "Auto Approach Control",
        "GPIO Pulse Control",

        // ID013
        "GPIO Services",
        "Recorder Control",
        "DSP Core Manager",
        "",

        //ID0017
        "Signal Monitor Update",
        "",
        "",
        "",
        
        //ID0021
        "",
        "",
        "",
        "",

        //ID0025
        "",
        "",
        "",
        "Bias and Zpos Adjusters",
        
        //ID0029
        "",
        "RMS Signal Processing",
        "Noise (random) Signal Generator",
        "1s Timer (Clock)",

	NULL
};


#endif
