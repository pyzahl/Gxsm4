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

	.global _TSCInit

	.global _TimerTemp
	.global _MeasuredTime
	.global _MaxTime
	.global _MinTime

	.bss _TimerTemp,4,4
	.bss _MeasuredTime,4,4
	.bss _MaxTime,4,4
	.bss _MinTime,4,4


	
;***************************************************************************
; Timer0 register address set
;***************************************************************************


T0CNT1  .set            0x1008
T0CNT2  .set            0x1009

T0PRD1  .set            0x100C
T0PRD2  .set            0x100D

T0GCTL1 .set            0x1012 
T0CTL1  .set            0x1010
	
; Init DSP mode

;;	BCLR    OVM
;;      BCLR    SXM
              
; Init timer 0 (see SPRU618b.pdf)
; Dual mode unchained with no prescaler
; Timer0 at 150 MHz

	.sect ".text"
	.align 4
	
_TSCInit:	
	MOV     #0x00, port(#T0CTL1)    ; Disable timer 0
        MOV     #0, port(#T0CNT1)               ; Init Cnt1,2 at 0
        MOV     #0, port(#T0CNT2)
        MOV     #0xFFFF, port(#T0PRD1)  ; Load the timer period at 0xFFFFFFFF
        MOV     #0xFFFF, port(#T0PRD2)
        MOV     #0x5, port(#T0GCTL1)    ; Remove the reset on Timer0 and Dual mode unchained
	MOV     #0x80, port(#T0CTL1)    ; Enable Timer0 in continous mode
	RET
	                

; Read Timer0

        .global _TSCReadFirst

_TSCReadFirst:	
	PSHBOTH XAR0
	PSHBOTH XAR1
        PSH     dbl(AC0)
        MOV     port(#T0CNT1),AR0       ; CNT1 must read first, then a copy of the current CNT2 is done
        MOV     port(#T0CNT2),AR1
        MOV     AR1,HI(AC0)
        OR      AR0,AC0                 ; ACO contain the 32-bit timer value
        MOV     AC0,dbl(*(_TimerTemp))
	POP     dbl(AC0)
        POPBOTH XAR1
        POPBOTH XAR0
	RET
	

; Read Timer0 again

        .global _TSCReadSecond
_TSCReadSecond:
	PSHBOTH XAR0
	PSHBOTH XAR1
        PSH     dbl(AC0)
        MOV     dbl(*(_TimerTemp)),AC0
        MOV     port(#T0CNT1),AR0       ; CNT1 must read first, then a copy of the current CNT2 is done
        MOV     port(#T0CNT2),AR1
        MOV     AR1,HI(AC1)
        OR      AR0,AC1                         ; AC1 contain the 32-bit timer value
                
; Compute Time and save it
                
        SUB     AC1,AC0
        ABS     AC0
        MOV     AC0,dbl(*(_MeasuredTime))
	POP     dbl(AC0)
        POPBOTH XAR1
        POPBOTH XAR0
	RET

	.end
