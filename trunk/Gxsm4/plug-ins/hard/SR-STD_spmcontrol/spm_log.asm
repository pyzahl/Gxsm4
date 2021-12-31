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

;; calculates natural logarith, 
;; uses taylor series approximation and scaling

	.asg    *sp(0), Exp
	.asg    *sp(1), Mant
	.asg    *sp(2), Tmp

	.mmregs		; assign Memory-Mapped Register names as global symbols
	.def	_spm_logn
    .global _spm_logn

_logpolyc	.sect "LPOLYCOEF"
	.word	04000h		;; Const_4000, Q15
	.word	0DC56h		;; B6, Q19
	.word	054adh		;; B6, Q19
	.word	09e8ah		;; B4, Q17
	.word	050d5h		;; B3, Q16
	.word	0c056h		;; B2, Q15
	.word	03ffdh		;; B1, Q14
	.word	0062dh		;; B0, Q30
	.word	00001h		;; Const_1
	.word	058B9h		;; Const_ln2, Q15

		;; y = 0.4343 * ln(x)
		;;   |             x := M(x) * 2^P(x) --- M: Mantissa, P: Power
		;;   = 0.4343 * ln(M*2^P)
		;;   = 0.4343 * ( ln(1 + (2M-1)) + (P-1)ln(2) )

	.text		; begin assembling into .text section
	
_spm_logn:
        PSHM    ST0                                 ; 1 cycle
        PSHM    ST1                                 ; 1 cycle
        RSBX    OVA                                 ; 1 cycle
        RSBX    OVB                                 ; 1 cycle
		FRAME	#-3

		;; Normalize x

		;; LD	x,16,A		; A = x<<16
		EXP		A			; T = number of leading bits
		ST		T, Exp		; Exp = number of leading bits
		NORM	A			; A = A<<T
		STH		A, Mant		; Mant = M (between 0.5 and 1.0)
		NOP					; avoid pipline confict???

		;; Polynomial approximation
   
		STM		_logpolyc, AR2	; load poly coef table
		LD		Mant, 1, A		; A <- 2*M  
		SUB		*AR2+,1, A		; A <- (2*M-1) Q15
		STLM	A, T			; U <- (2*M-1) Q15 (between 0.0 and 1.0)
		LD		*AR2+,16,A		; A <- B6
		LD		*AR2+,16,B		; B <- B5
		POLY	*AR2+			; A(32-16) <- B6*U + B5
								; Q34 + Q18<<16 = Q34				
		POLY	*AR2+          	; A <- (B6*U + B5)*U + B4
								; Q33 + Q17<<16 = Q33	
		POLY	*AR2+			; A <- ((B6*U + B5)*U + B4)*U + B3
								; Q32 + Q16<<16 = Q32
		POLY	*AR2+	       	; A <- (((B6*U + B5)*U + B4)*U + B3)*U + B2
								; Q31 + Q15<<16 = Q31
		POLY	*AR2           	; A <- ((((B6*U + B5)*U + B4)*U + B3)*U + B2)*U + B1
								; Q30 + Q14<<16 = Q30							
		SFTA	A,1,A			; Q14<<1 = Q15 (accumulator high)	
		MPYA	A										
		ADD		*AR2+,A			; A <- (((((B6*U + B5)*U + B4)*U + B3)*U + B2)*U + B1)*U + B0
								; Q30 + Q30 = Q30                                               
		STH		A,1,Tmp			; Tmp <- (((((B6*U + B5)*U + B4)*U + B3)*U + B2)*U  + B1)*U + B0
								; = f(2*M-1)
								; Q14<<1 = Q15 (accumulator high)					
		;; Process exponent

		LD		Exp,A			; A <- number of leading bits
		NEG		A				; A <- exponent = P
		SUB		*AR2+,A			; A <- P-1                
		STLM	A,T         	; T <- P-1
		MPY		*AR2,A			; A <- (P-1)*ln(2)
								; Q0*Q15 = Q15

		;; Add both contributions

		ADD		Tmp,A			; A <- f(2*M(x)-1) + (P(x)-1)*ln(2)
								; Q15 

		FRAME	#3
        POPM    ST1
        POPM    ST0

		RET                     ; 5 cycles
	
;; end of file.
