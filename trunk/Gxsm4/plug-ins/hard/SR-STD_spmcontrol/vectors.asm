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

;____________________________________________________________________________
;
;	vectors.asm
;
;	This section must be loaded from 0080h, and must contain a branch to 
;	"kernel" at HPIINT vector to be compatible with Signal Ranger's kernel
;
;____________________________________________________________________________


        .sect ".vectors"

        .global _c_int00        ; C entry point
        .global AIC_ISR        	; AIC ISR entry point
        
kernel  .set    0112h           ; address of the main function in kernel code

        .align  0x80            ; must be aligned on page boundary
        
        .global	ISRDMA			; Global declaration of ISRDMA

RESET:  BD _c_int00             ; reset vector
        STM #200,SP             ; branch to C entry point
                                ; stack size of 200
        
nmi		RETE					;04; vectors non-maskable external interrupt
		nop
		nop
		nop
sint17	RETE					;08; vectors for trap routine (#17)
		nop
		nop
		nop

        .space  52*16   		;0C-3F: vectors for software interrupts 18-30

int0	RETE					;40; vectors for external interrupt int0
		nop
		nop
		nop
int1	RETE					;44; vectors for external interrupt int1
		nop
		nop	
		nop
int2	RETE					;48; vectors for external interrupt int2
		nop
		nop
		nop
tint0	RETE					;4C; vectors for Timer #0 interrupt
		nop
		nop
		nop
brint0	RETE					;50; vectors for McBSP #0 receive interrupt
		nop
		nop
		nop
bxint0	RETE					;54; vectors for MsBSP #0 transmit interrupt
		nop
		nop
		nop
reserv  RETE					;58; Reserved or DMA channel 0 interrupt (selection by
		nop		   				;DMPREC register)
		nop
		nop
tint1	B		ISRDMA			;5C; vectors for Timer #1 interrupt and DMAC1
		nop
		nop
int3	RETE					;60; vectors for external interrupt int3
		nop
		nop
		nop
HPIINT	B		#kernel			;64; vectors for Host-to-HPI interrupt
		nop
		nop
brint1	RETE					;68; vectors for McBSP #1 receive interrupt
		nop
		nop
		nop
bxint1	RETE					;6c; vectors for McBSP #1 transmit interrupt
		nop
		nop
		nop
dmac4	RETE					;70; vectors for DMA channel 4 interrupt
		nop
		nop
		nop
dmac5	RETE					;74; vectors for DMA channel 5 interrupt
		nop
		nop 
		nop                     
		
		.end
		