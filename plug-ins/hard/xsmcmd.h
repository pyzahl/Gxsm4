/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 8 c-style: "K&R" -*- */

/* Gxsm - Gnome X Scanning Microscopy Project
 * universal STM/AFM/SARLS/SPALEED/... controlling and
 * data analysis software
 *
 * DSP tools for Linux
 *
 * Copyright (C) 1999,2000,2001 Percy Zahl
 *
 * Authors: Percy Zahl <zahl@users.sf.net>
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


/*@
 *@	File:	xsmcmd.h
 *@	Datum:	25.10.1995
 *@	Author:	P.Zahl
 *@	Zweck:	DSP Komando ID's mit Parameterübergabepositionen
 *@
 *@	History:
 *@	=====================================================================
 *@	V1.00 Basisversion
 *@@    24.4.96 PZ: DPRAM16/32 Implementation
 *@@    24.4.96 PZ: Def. von Datenübertragungsmodi
 *@@    14.12.97 PZ: Namensaenderung in xsmcmd.h
 *@@    12.11.99 PZ: Splitting: DPRAM Zuweisungen / CmdIds
 */


/*
 * DSP Komandos zur Komunikation -- oder quasi Kommunication auf HwI level... -- this is a bit wicked file use...
 */


#define DSP_CMD_ROTPARAM		0x02	/* Rotation + Translatiosparameter */
#define DSP_CMD_SET_WERTE		0x11	/* PID Reglerwerte, etc. setzten */
#define DSP_CMD_SET_LOG_WERTE		0x12	/* LOG Reglerwerte, etc. setzten */
#define DSP_CMD_SET_TRANSFER_FKT        0x12    /* LOG/LIN Transferkunktionsparameter setzen */
#define DSP_CMD_GET_WERTE		0x13	/* PID Reglerwerte, etc. anfordern */

/* VODO */
#define DSP_CMD_SPRUNG_PARAM            0x14	/* Parameter fuer SprungAntwort setzen */
/* VODO end */

#define DSP_CMD_MOVETO_X		0x20	/* X-Position anfahren */
#define DSP_CMD_MOVETO_Y		0x21	/* Y-Position anfahren */
#define DSP_CMD_MOVETO_PARAM	        0x22	/* Moveto Parameter / Dynamik */
#define DSP_CMD_MOVETO_XY	        0x23	/* Moveto Parameter / Dynamik */

#define DSP_CMD_ZPOS_ADJUST_ON          0x25   /* normal -- post ZPos manipulation auto zero active -- MK3 only so far */
#define DSP_CMD_ZPOS_ADJUST_HOLD        0x26   /* normal -- post ZPos manipulation auto zero disbaled -- MK3 */

#define DSP_CMD_LINESCAN		0x30	/* LineScan ausfhren */
#define DSP_CMD_LINESCAN_PARAM	        0x31	/* LineScan Parameter / Dynamik */
#define DSP_CMD_SWAPDPRAM	        0x32	/* Swap Memory1,2,.. from buffer to dpram */
#define DSP_CMD_MOVE_DATA2DPRAM	        0x33	/* move data from buffer to dpram */
#define DSP_CMD_2D_HS_AREASCAN          0x34    /* perform a complete 2D high speed area scan */

#define DSP_CMD_HALT			0x40	/* Regler stopp */
#define DSP_CMD_START			0x41	/* Regler start */
#define DSP_CMD_UZ_CLR			0x42	/* U_Z auf Null setzen */

#define DSP_CMD_APPROCH			0x50	/* Autom. Spitzen Ann„herung */
#define DSP_CMD_FAPPROCH		0x51	/* Schnelle Spitzen Ann„herung */
#define DSP_CMD_BACK			0x52	/* Spitze Zurckziehen */
#define DSP_CMD_APPROCH_PARAM		0x55	/* Parameter zur Spitzenann„herung */

#define DSP_CMD_APPROCH_MOV_XP		0x56	/* Autom. Spitzen Annäherung via Ramp on X */

/* nur Omicron STM / Quantum */
#define DSP_CMD_PROBE_LI		0x60	/* Probe Positionieren: links */
#define DSP_CMD_PROBE_RE		0x61	/* Probe Positionieren: rechtes */
#define DSP_CMD_PROBE_VOR		0x62	/* Probe Positionieren: vor */
#define DSP_CMD_PROBE_ZUR		0x63	/* Probe Positionieren: zurck */

/* nur HoP/AK AFM / SICAF */
#define DSP_CMD_AFM_SLIDER_PARAM	0x60	/* Parameter fuer Slider */
#define DSP_CMD_AFM_MOV_XP		0x61	/* Linsenmover in X+ */
#define DSP_CMD_AFM_MOV_XM		0x62	/* Linsenmover in X- */
#define DSP_CMD_AFM_MOV_YP		0x63	/* Linsenmover in Y+ */
#define DSP_CMD_AFM_MOV_YM		0x64	/* Linsenmover in Y- */
#define DSP_CMD_AFM_MOV_ZP		0x65	/* Linsenmover in Z+ */
#define DSP_CMD_AFM_MOV_ZM		0x66	/* Linsenmover in Z- */


#define DSP_CMD_CLR_PA			0x70	/* Port PA = 0x00 setzten */
#define DSP_CMD_ALL_PA			0x71	/* Port PA = 0xff setzten */

#define DSP_CMD_GPIO_SETUP              0x73    /* GPIO configuration A810 */

#define DSP_CMD_Z0_STOP                 0x75
#define DSP_CMD_Z0_P                    0x76
#define DSP_CMD_Z0_M                    0x77
#define DSP_CMD_Z0_AUTO                 0x78
#define DSP_CMD_Z0_CENTER               0x79
#define DSP_CMD_Z0_GOTO                 0x80

#define DSP_CMD_SETOSZI                 0x80    /* Oszi Setup */

#define DSP_CMD_READY			0xfe	/* Dummy: nur ACK Antwort */
#define DSP_CMD_GETINFO			0xfe	/* Same as Dummy, but new DSP soft 
						 * puts info some Config/Version info DPRAM 
						 */

#define DSP_CMD_PROBESCAN               0x90    /* Probe Scan ausführen */ 

/* PC -> PC31/DSP DPRAM Daten šbergabevereinbarungen */

/* Je nach Komando wird der DPRAM-Bereich DSP_CTRL_PARAM_L
 * für Parameterübergaben verwendet 
 */

#define DSP_CMD_PARAM     DSP_CTRL_PARAM

/* DSP_CMD_SET_WERTE */
#define DSP_UTUNNEL       0
#define DSP_ITUNNEL       1
#define DSP_CP            2
#define DSP_CI            3
#define DSP_CD            4 /* ist Setpoint !! */
#define DSP_TAU           5
#define DSP_FB_FRQ        6
#define DSP_FIR_FG        7
#define DSP_CS            8


/* DSP_CMD_SET_TRANSFER_FKT */
#define DSP_LOGOFF        0
#define DSP_LOGSKL        1
#define DSP_LIN_LOG       2


/* DSP_CMD_ROTPARAM */
#define DSP_ROTXX         0
#define DSP_ROTXY         1
#define DSP_ROTYX         2
#define DSP_ROTYY         3
#define DSP_ROTOFFX       4
#define DSP_ROTOFFY       5

/* DSP_CMD_MOVETO_PARAM */
#define DSP_MOVETOX        0
#define DSP_MOVETOY        0
#define DSP_MOVETOXY_X     0
#define DSP_MOVETOXY_Y     1

#define DSP_MVSTEPSZ      0
#define DSP_MVNREGEL      1

/* DSP_CMD_XXXSCAN_PARAM */
/* monitor current number of valid points in Buffer */
#define DSP_ACTUAL_SCANPOINTS 13
#define DSP_TRANSFERED_SCANPOINTS 14

/* DSP_CMD_LINESCAN_PARAM */
/* and DSP_CMD_LINESCAN */
#define DSP_NX2SCAN       0

#define DSP_LSNX          0
#define DSP_LSDNX         1
#define DSP_LSSTEPSZ      2
#define DSP_LSNREGEL      3
#define DSP_LSNAVE        4
#define DSP_LSINTAVE      5
#define DSP_LSNXPRE       6
#define DSP_LSSRCS        7
#define DSP_LSYINDEX      8

/* DSP_CMD_2D_HS_AREASCAN same as above and */
#define DSP_AS_NY2SCAN    9
#define DSP_AS_DNY       10

/* DSP_CMD_MOVE_DATA2DPRAM */
#define DSP_MVD_CMD_START       0
#define DSP_MVD_CMD_LEN         1

/* DSP_CMD_APPROACH_PARAM */
#define DSP_TIPNSTEPS     0
#define DSP_TIPNWARTE     1
#define DSP_TIPDUZ        2
#define DSP_TIPDUZREV     3

/* DSP_CMD_AFM_SLIDER_PARAM */	
#define DSP_AFM_SLIDER_AMP   0
#define DSP_AFM_SLIDER_SPEED 1
#define DSP_AFM_SLIDER_STEPS 2

/* Static Oszi Parameter Positionens im DPRAM Bereich DSP_USR_DATA_L */
/* Offset in USR-Region */
#define DSP_OSZI_OFF_POS  0
#define DSP_OSZI_OFF_N    1
#define DSP_OSZI_OFF_DATA 2

/* Abs. Pos- for DSP */
#define DSP_OSZI_POS      (DSP_USR_OSZI_CTRL+DSP_OSZI_OFF_POS)
#define DSP_OSZI_N        (DSP_USR_OSZI_CTRL+DSP_OSZI_OFF_N)

/* DSP_CMD_SETOSZI */
#define DSP_OSZI_TIMEBASE 0
#define DSP_OSZI_CHX      1
#define DSP_OSZI_CHY      2

/* Spezialpositionen für Oszi */
#define DSP_OSZI_CH12      15
#define DSP_OSZI_CH34      16
#define DSP_OSZI_CH56      17
#define DSP_OSZI_CH78      18
#define DSP_OSZI_TM        19
#define DSP_OSZI_DATAREADY 20
#define DSP_OSZI_CNT       21


/* DSP_CMD_PROBESCAN */
#define DSP_PRBSRCS       0
#define DSP_PRBOUTP       1
#define DSP_PRBNX         2
#define DSP_PRBXS         3
#define DSP_PRBXE         4
#define DSP_PRBACAMP      5
#define DSP_PRBACFRQ      6
#define DSP_PRBDELAY      7
#define DSP_PRBCIVAL      8
#define DSP_PRBACPHASE    9
#define DSP_PRBNAVE       10
#define DSP_PRBACMULT     11
#define DSP_PRBGAPADJ     12
#define DSP_PRBPANZ DSP_PRBGAPADJ+1


/* Sub cmds for Probe */
#define DSP_PRB_CMD_PROBESCAN         1
#define DSP_PRB_CMD_ACMOD_START 2
#define DSP_PRB_CMD_ACMOD_NEXT  3
#define DSP_PRB_CMD_ACMOD_QUIT  4


/****************************************/
