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

#include "SR3PRO_A810Driver.h"
#include "SR3_Reg.h"

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
#define EXTERNAL_100       100
#define EXTERNAL_101       101
#define EXTERNAL_102       102
#define EXTERNAL_103       103
#define EXTERNAL_104       104
#define EXTERNAL_105       105
#define EXTERNAL_106       106
#define EXTERNAL_107       107

/* external functions */

void dataprocess ();
extern void run_offset_move ();

//___________________________________________________________________________
//
//      Extern variable
//___________________________________________________________________________    

extern  struct  iobuf_rec   iobuf;       // Analog810 in/out buffer
extern  int                 NbrChByPort; // Half the channel number of the SR2_analog board

/*___________________________________________________________________________
 
    Functions / variables for TSC timer
___________________________________________________________________________*/

extern void wait(int delay);  
extern void TSCReadFirst();
extern void TSCReadSecond();
extern void TSCInit();
extern    volatile unsigned int    TimerTemp, MeasuredTime, MaxTime, MinTime;
 
/*
 In your main function, call TSCInit() first. Then, you can call
 TSCReadFirst() and TSCReadSecond() around the code you want to
 time. The MeasuredTime, MaxTime, MinTime variables are automatically
 managed by these functions.

 The timer works at 590 MHz and the numbers in the MeasuredTime,
 MaxTime, MinTime varibles are directly the number of CPU cycle at 590 MHz.
 
*/
 
#define AIC_OUT(N) iobuf.mout[N]
#define AIC_IN(N)  iobuf.min[N]
#define COUNTER(N) QEP_cnt[N]

/* MK3Pro-A810 MK2 equiv. */
#define	GPIO_Data_0  0x46000006
#define	GPIO_Dir_0   0x46000008

/* MK3-Pro Register Byte-Address */
#define Port_A_Dat 0x46000000
#define Port_A_Dir 0x46000002
#define Port_B_Dat 0x46000004
#define Port_B_Dir 0x46000006
#define Port_C_Dat 0x46000008
#define Port_C_Dir 0x4600000A
#define Port_D_Dat 0x4600000C
#define Port_D_Dir 0x4600000E

//MK2-A810
//dataprocess.h:#define	GPIO_Data_0 0x600007
//dataprocess.h:#define	GPIO_Dir_0  0x600009


#endif						
