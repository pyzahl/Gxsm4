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

/* RT TASK NUMBER ASSIGNMENTS */
#define RT_TASK_ANALOG_IN     1
#define RT_TASK_QEP_COUNTER   2
#define RT_TASK_FEEDBACK      3
#define RT_TASK_AREA_SCAN     4
#define RT_TASK_VECTOR_PROBE  5
#define RT_TASK_LOCKIN        6
#define RT_TASK_ANALOG_OUTHR  7
#define RT_TASK_WAVEPLAY_PL   8
#define RT_TASK_NONE          9
#define RT_TASK_MSERVO       10
#define RT_TASK_SCO0         11
#define RT_TASK_SCO1         12

#define ID_TASK_BZ_PUSH            0
#define ID_TASK_MOVE_AREA_SCAN     1


#ifndef DSP_CC

// hard real time data process (DP) tasks
const gchar *MK3_dsp_dp_process_name[] = {
        "System", // RT000

        // RT Tasks
        "PACPLL, Recorder, Analog IN, Mixer, adap.IIR", //RT001
        "QEP, GATEING, COUNTER Expansion",
        "Z-servo Control and Feedback Mode Logic",
        "Area Scan, Rotation Transform",

        "Vector Probe Machine", //RT005
        "LockIn", //RT006
        "Analog OUT Mapping with HR",  // RT007
        "Coarse Motion Wave Generator: AutoApp/Mover/Pulse", // RT008

        "McBSP Fast Transfer", //RT009
        "M-servo",
        "SCO 0",
        "SCO 1",
	NULL
};         

// non hard realtime Idle (ID) tasks and timer tasks
const gchar *MK3_dsp_id_process_name[] = {
        // Ilde Tasks
        // ID001
        "BZ push data from RT_FIFO",
        "Offset Move",
        "Vector Probe Manager",
        "",
        
        // ID005
        "Statemachine Control, exec lnx",
        "Feedback Mixer log update",
        "Signal Management",
        "Offset Move init",

        // ID009
        "Scan Controls",
        "Probe Controls",
        "Auto Approach Control",
        "GPIO Pulse Control",

        // ID013
        "GPIO Services",
        "PACPLL Control",
        "DSP Core Management",
        "AIC Control**",

        //ID0017
        "Signal Monitor Update",
        "McBSP Idle Transfers",
        "McBSP-Freq M-Servo @Watch",
        "McBSP-Control M-Servo @Watch",
        
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
        "1s Timer",

	NULL
};


#endif
