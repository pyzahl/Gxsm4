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
	.global _feedback_mixer
	
	.asg *(_iobuf+0), fb_signal0	    ; AIC0-in  [+0] 
	.asg *(_iobuf+1), fb_signal1	    ; AIC1-in  [+1] 
	.asg *(_iobuf+2), fb_signal2	    ; AIC2-in  [+2] 
	.asg *(_iobuf+3), fb_signal3	    ; AIC3-in  [+3] 
	.asg *(_iobuf+0), fb_signal	    ; AIC5-in  [+5] 
;	.asg *(_iobuf+5), fb_signal	    ; AIC5-in  [+5] 
	.asg *(_iobuf+13), z_scan       ; AIC5-out [8+5]
	
	.asg *(_feedback+0), fb_cp
	.asg *(_feedback+1), fb_ci
	.asg *(_feedback+2), fb_setpoint
	.asg *(_feedback+3), ca_15          ;  is Q15
	.asg *(_feedback+4), cb_Ic          ;  is 32 bit (long)
	.asg *(_feedback+6), I_cross
	.asg *(_feedback+7), I_offset
	.asg *(_feedback+8), q_factor15
	.asg *(_feedback+9), fb_soll
	.asg *(_feedback+10), fb_ist
	.asg *(_feedback+11), fb_delta
	.asg *(_feedback+12), fb_i_sum      ; is 32 bit (long)
	.asg *(_feedback+14), I_iir         ; is 32 bit (long)
	.asg *(_feedback+16), fb_z
	.asg *(_feedback+17), I_fbw	    
	.asg *(_feedback+24), fb_z_32neg    ; full 32 bit Z negiert
	.asg *(_feedback+26), fb_z_32       ; full 32 bit Z

;;; Mixer
	.asg *(_feedback_mixer+0), mix_lv0
	.asg *(_feedback_mixer+1), mix_lv1
	.asg *(_feedback_mixer+2), mix_lv2
	.asg *(_feedback_mixer+3), mix_lv3
	.asg *(_feedback_mixer+4), mix_gn0
	.asg *(_feedback_mixer+5), mix_gn1
	.asg *(_feedback_mixer+6), mix_gn2
	.asg *(_feedback_mixer+7), mix_gn3
	.asg *(_feedback_mixer+8), mix_m0
	.asg *(_feedback_mixer+9), mix_m1
	.asg *(_feedback_mixer+10), mix_m2
	.asg *(_feedback_mixer+11), mix_m3
	.asg *(_feedback_mixer+12), mix_sp0
	.asg *(_feedback_mixer+13), mix_sp1
	.asg *(_feedback_mixer+14), mix_sp2
	.asg *(_feedback_mixer+15), mix_sp3
	.asg *(_feedback_mixer+16), mix_Zset
	.asg *(_feedback_mixer+17), mix_exec
	.asg *(_feedback_mixer+18), mix_x
	.asg *(_feedback_mixer+19), mix_lnx
	.asg *(_feedback_mixer+20), mix_y
	.asg *(_feedback_mixer+21), fb_delta_mixed

	
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

	.global	_asm_feedback_from_delta
	.global	_asm_calc_setpoint_log

	.global	_asm_calc_mix_log

	.text
		
_asm_calc_mix_log:

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
	
	LD	mix_x, 16, A

;; do absolut, add offset, sat
	ABS	A
	ADD	I_offset << #16, A
	SAT	A
		
	;; compute spm_logn of a, Q15 result
	CALL	_spm_logn
	SFTA	A, 12			; adjustment
	STH	A, mix_lnx		; Q15, new "soll" value

	;; Restore context
	
	POPM	AL
	POPM	AH
	POPM	AG
	POPM	ST0
	POPM	ST1
	POPM	T

	RET

		
_asm_feedback_from_delta:

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


	;; compute Proportional-Integral-Feedback signal, SATuration A:
	;; delta = ist-soll
	;; i_sum += CI*delta
	;; z = CP*delta + i_sum 

	MOV	fb_delta, T			; Temp0 = hi16 of (delta(32) = [sum of all](fb_ist - fb_soll)))

	DLD	fb_i_sum, A		; i_sum(32) -> A
	MACM	fb_ci, T, A		; A += CI * Temp
	SAT	A			; saturate A now
	DST	A, fb_i_sum		; we get:	i_sum(32) += CI * Temp
	MACM	fb_cp, T, A		; A += CP * delta
	SAT     A
	MOV	A, dbl(fb_z_32)		; store new Z as full 32 bit (default Z data transferred)

	NEG	A			; adjust Z signum for "Besocke" 
	SAT	A			; saturate A now:	A = i_sum + CP * Temp
	MOV	A, dbl(fb_z_32neg)	; store new Z as full 32 bit, negated (default output value)
	STH	A, fb_z			; store new Z (high word):		Z = (i_sum+=CI*delta) + CP*delta
;	STH	A, z_scan		; store to AIC buffer -- not here

;	MOV	HI(A<<#3), fb_z_hrbits	; store new Z hrbits part, optional "digits" for enhanced resolution
;	NEG	A
;	SAT	A

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

