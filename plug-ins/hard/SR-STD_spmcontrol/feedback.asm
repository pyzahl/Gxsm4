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

;  21+34=55 cycle
		
; -*- Mode: asm; indent-tabs-mode: nil; asm-basic-offset: 8 -*-

		.width   80
        .length  55
        .title "ASM functions"
        .mmregs

	.global _spm_logn
;____________________________________________________________________________
;
;       Extern variables
;____________________________________________________________________________

	.global	_iobuf
	.global _analog
	.global _feedback
	
	.asg *(_iobuf+5), fb_signal	    ; AIC5-in  [+5] 
	.asg *(_iobuf+13), z_scan       ; AIC5-out [+13]
	
	.asg *(_feedback+0), fb_cp
	.asg *(_feedback+1), fb_ci
	.asg *(_feedback+2), fb_setpoint
	.asg *(_feedback+3), fb_soll
	.asg *(_feedback+4), fb_ist
	.asg *(_feedback+5), fb_delta
	.asg *(_feedback+6), fb_i_pl_sum   ; is 32 bit (long)
	.asg *(_feedback+8), fb_i_sum   ; is 32 bit (long)
	.asg *(_feedback+10), fb_z
	
	
;____________________________________________________________________________
;
;       Some defintions
;____________________________________________________________________________

nbrcoef		.set	2048		; Also define in FB_Demo.h
fixgain		.set	32767

;____________________________________________________________________________
;
;      asmfeedback function
;____________________________________________________________________________

	.global	_asm_feedback_log
	.global	_asm_calc_setpoint_log

	.text
		
_asm_feedback_log:

		;; Save context
	
		PSHM	T
		PSHM	ST1
		PSHM	ST0
		PSHM	AG
		PSHM	AH
		PSHM	AL
	
		;; Set DSP mode SXM = 1, FRCT = 0, OVM = 0
	
		SSBX	SXM
		RSBX	FRCT
        RSBX    OVM
	
		;; load signal, abs
	
		LD		fb_signal, 16, A ; +/-10V range
		ABS		A
		ADD		10, A
	
		;; compute spm_logn of a, Q15 result
		CALL	_spm_logn
		SFTA	A, 12			; adjustment
		STH		A, fb_ist		; Q15

		;; compute Proportional-Integral-Feedback signal, SATuration A:
		;; delta = ist-soll
		;; i_pl_sum += CI*delta
		;; z = CP*delta + i_pl_sum 
		SUB		fb_soll, 16, A		; A -= fb_soll, delta -> A

		STH		A, fb_delta			; Q15 test

		SFTA    A, -16 				; need AH in T
		STLM	A, T				; A -> Temp (Temp=delta = (fb_ist - fb_soll))

		DLD		fb_i_pl_sum, A			; i_pl_sum(32) -> A
		MAC		fb_ci, A			; A += CI * Temp
		SAT		A					; saturate A now
		DST		A, fb_i_pl_sum			; we get:	i_pl_sum(32) += CI * Temp
		MAC		fb_cp, A			; A += CP * delta

		NEG		A					; adjust Z signum for "Besocke" 
		SAT		A					; saturate A now:	A = i_pl_sum + CP * Temp

		STH		A, fb_z				; store new Z:		Z = (i_pl_sum+=CI*delta) + CP*delta
		STH		A, z_scan			; store to AIC buffer
		ANDM	#0xFFFE, z_scan		; and mask bit 0 

		;; Restore context
	
		POPM	AL
		POPM	AH
		POPM	AG
		POPM	ST0
		POPM	ST1
		POPM	T

		RET

	;; ____________________________________________________________
	;; 
	;; recalculate "log setpoint" from voltage
	;; ____________________________________________________________

_asm_calc_setpoint_log:

		;; Save context
	
		PSHM	T
		PSHM	ST1
		PSHM	ST0
		PSHM	AG
		PSHM	AH
		PSHM	AL
	
		;; Set DSP mode SXM = 1 , FRCT = 0
	
		SSBX	SXM
		RSBX	FRCT
	
		LD	fb_setpoint, 16, A
	
		;; compute spm_logn of a, Q15 result
		CALL	_spm_logn
		SFTA	A, 12			; adjustment
		STH	A, fb_soll		; Q15, new "soll" value

		;; Restore context
	
		POPM	AL
		POPM	AH
		POPM	AG
		POPM	ST0
		POPM	ST1
		POPM	T

		RET

	;; ____________________________________________________________
	;;
	;; END
	;; ____________________________________________________________
	
		.end

