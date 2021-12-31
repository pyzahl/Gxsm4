; /* SRanger and Gxsm - Gnome X Scanning Microscopy Project
;  * universal STM/AFM/SARLS/SPALEED/... controlling and
;  * data analysis software
;  *
;  * DSP tools for Linux
;  *
;  * Copyright (C) 1999,2000,2001,2002 Percy Zahl
;  *
;  * Authors: Percy Zahl <zahl@users.sf.net>
;  * WWW Home:
;  * DSP part:  http://sranger.sf.net
;  * Gxsm part: http://gxsm.sf.net
;  *
;  * This program is free software; you can redistribute it and/or modify
;  * it under the terms of the GNU General Public License as published by
;  * the Free Software Foundation; either version 2 of the License, or
;  * (at your option) any later version.
;  *
;  * This program is distributed in the hope that it will be useful,
;  * but WITHOUT ANY WARRANTY; without even the implied warranty of
;  * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
;  * GNU General Public License for more details.
;  *
;  * You should have received a copy of the GNU General Public License
;  * along with this program; if not, write to the Free Software
;  * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
;  */

		
; -*- Mode: asm; indent-tabs-mode: nil; asm-basic-offset: 8 -*-

	.width   80
        .length  55
        .title "ASM functions"
        .mmregs

	.global _asm_counter_accumulate
;____________________________________________________________________________
;
;       Extern variables
;____________________________________________________________________________

        .global         _QEP_cnt
        .global         _QEP_cnt_old
 
	.global _analog
	
	.asg *(_analog+8+8+8), counter0
	.asg *(_analog+8+8+8+2), counter1
		
;____________________________________________________________________________
;
;      asmfeedback function
;____________________________________________________________________________

	.global	_asm_feedback_log
	.global	_asm_calc_setpoint_log

	.text
		
_asm_counter_accumulate:

        PSHBOTH XAR0
        PSHBOTH XAR1
        PSH     dbl(AC0)
	PSH     mmap(AC0G)      
        PSH     dbl(AC1)
	PSH     mmap(AC1G)

        ; Force Linear mode for AR0 and AR1
        
        BCLR    AR0LC
        BCLR    AR1LC

        ; Init DSP mode

	BSET    SXMD
	BCLR    SATA

        ; Update counter diff sum
        
        MOV     *(_QEP_cnt),AR0
        MOV     *(_QEP_cnt_old),AR1
        MOV     AR0,*(_QEP_cnt_old)             
        SUB     AR1,AR0
        MOV     dbl(counter0),AC1
        ADD     AR0,AC1
        MOV     AC1,dbl(counter0)

        MOV     *(_QEP_cnt+1),AR0
        MOV     *(_QEP_cnt_old+1),AR1
        MOV     AR0,*(_QEP_cnt_old+1)           
        SUB     AR1,AR0
        MOV     dbl(counter1),AC1
        ADD     AR0,AC1
        MOV     AC1,dbl(counter1)
                
	POP     mmap(AC1G)
        POP     dbl(AC1)
	POP     mmap(AC0G)
        POP     dbl(AC0)
        POPBOTH XAR1
        POPBOTH XAR0

	RET

	;; ____________________________________________________________
	;;
	;; END
	;; ____________________________________________________________
	
	.end

