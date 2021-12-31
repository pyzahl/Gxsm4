/* SRanger and Gxsm - Gnome X Scanning Microscopy Project
 * universal STM/AFM/SARLS/SPALEED/... controlling and
 * data analysis software
 *
 * DSP tools for Linux
 *
 * Copyright (C) 1999,2000,2001,2002 Percy Zahl
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

/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 8 c-style: "K&R" -*- */

#ifndef __DATAPROCESS_H
#define __DATAPROCESS_H

#ifdef USE_ANALOG_16
# include "Analog_16_support/AIC24Driver.h" 
#else
# include "Analog810Driver.h"
#endif

#define AIC_OFFSET_COMPENSATION  0
#define AIC_OFFSET_CALCULATION   1
#define AIC_DATA_PROCESS_NORMAL  2
#define AIC_REQUEST_RECONFIG     3

#define EXTERNAL_FEEDBACK  0
#define EXTERNAL_OFFSET_X  1
#define EXTERNAL_LOG       2
#define EXTERNAL_DELTA     3
#define EXTERNAL_PRB_SINE  4
#define EXTERNAL_PRB_PHASE 5
#define EXTERNAL_BIAS_ADD  6
#define EXTERNAL_EN_MOTOR  7
#define EXTERNAL_EN_PHASE  8
#define EXTERNAL_PID       9
#define EXTERNAL_FEEDBACK_ADD 10 


/* external functions */

interrupt void dataprocess ();
extern void run_offset_move ();

//___________________________________________________________________________
//
//      Extern variable
//___________________________________________________________________________    

extern  struct  iobuf_rec   iobuf;       // Analog810 in/out buffer
extern  int                 QEP_cnt[2];  // FPGA QEP Counters
extern  int                 NbrChByPort; // Half the channel number of the SR2_analog board

#ifdef WATCH_ENABLE
extern WATCH                watch;
extern int                  PRB_ref1stA;
extern int                  PRB_modindex;
#endif

#define AIC_OUT(N) iobuf.mout[N]
#define AIC_IN(N)  iobuf.min[N]
#define COUNTER(N) QEP_cnt[N]

#define	GPIO_Data_0 0x600007
#define	GPIO_Dir_0  0x600009


extern void TSCReadFirst();
extern void TSCReadSecond();
extern void TSCInit();
extern    volatile unsigned long    TimerTemp, MeasuredTime, MaxTime, MinTime;



#endif						
