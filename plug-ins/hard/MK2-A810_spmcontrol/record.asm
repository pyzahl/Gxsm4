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
;____________________________________________________________________________
;
;	from Alex
;	Assembler code for recorder function MK2
;	Assembler code for non blocking kernel read/write function
;____________________________________________________________________________



TransferAddress    .set 0x102
HPIC_K             .set 0xA018

SignalAdrr  .set     0x8000    ; In word 0x8000 (0x10000/2)
            .global _recorder
            .asg     (_recorder+0), _pSignal1
            .asg     (_recorder+2), _pSignal2
            .asg     (_recorder+4), _blcklen16
            .asg     (_recorder+6), _blcklen32
 
    .global _RecSignalsASM16
    .global _RecSignalsASM32
   
    .title "ass code non-block recorder kread/write"
    .mmregs
    .text
    .even

;***************************************************************************
;       MACRO : Acknowledge to inform the PC the end of a function
;***************************************************************************

Acknow    .macro

   MOV    #0x0107,port(#HPIC_K)        ; HINT=1 to acknowledge the Host
                                   ; DSPINT=1 to clear the current DSPINT

   .endm

   .global _RecSignalsASM16
   .global _RecSignalsASM32

   .mmregs
   .text
   .even

;***************************************************************************
; ReadMem/WriteMem and ReadProg/WriteProg  not atomic functions
;***************************************************************************

       .global ReadMemNA
       .global    ReadProgNA
ReadMemNA:
ReadProgNA:
       .global WriteMemNA
       .global    WriteProgNA
WriteMemNA:
WriteProgNA:

       ; Save Context
       BCLR    AR3LC
       BCLR    AR2LC
       PSH        mmap(AC0G)
       PSHBOTH AC0
       PSH     mmap(RPTC)
       PSH     mmap(CSR)


       AMOV    #TransferAddress, XAR3
       MOV        dbl(*AR3+), AC0
       SFTL     AC0,#-1
   ;||    BSET     INTM                            ; Disable all Ints
       MOV        *AR3+, AR2                        ; NbWords -> AR2
       CMP        *AR3+ ==#2d, TC1                ; Test ErrorCode
       SUB     #1d,AR2                            ; AR2 = AR2 - 1
       MOV     AR2,CSR                         ; CSR = AR2
       MOV     AC0,XAR2                        ; XAR2 = Transfer addrr >> 1

       BCC         WriteKnNA,TC1

       ; ***** Read Operation

       RPT     CSR                             ; Transfer in a RPT loop
       MOV         *ar2+,*ar3+

       B         ReadWriteKnEndNA

WriteKnNA:

       ; ****** Write Operation

       RPT     CSR                             ; Transfer in a RPT loop
       MOV         *ar3+,*ar2+

ReadWriteKnEndNA:

       ; Update TransferAddress

       BCLR     INTM                        ; Enable Ints
   ||    MOV        XAR2,AC0                     ; TransferAddress = XAR2 << 1
       SFTL     AC0, #1
       MOV     AC0,dbl(*(TransferAddress))


       ; Restore Context

       POP     mmap(CSR)
       POP        mmap(RPTC)
       POPBOTH    AC0
       POP        mmap(AC0G)

       Acknow                   ; flag host operation complete

       RETI


;***************************************************************************
; RecSignalASM16/32:	 SDRAM signal recorder functions
;***************************************************************************

_RecSignalsASM16:

   MOV dbl(*(#_pSignal1)), XAR3
   MOV dbl(*(#_pSignal2)), XAR4
   MOV *(#_blcklen16), AC1

   CMP *(#_blcklen16) == #-1, TC1
   BCC NoRecSignal16,TC1

   AMOV #SignalAdrr,XAR2     ; Init XAR2 on SignalAdrr(blcklen*2)
   MOV XAR2,AC0
   ADD AC1<<#1,AC0
   MOV AC0,XAR2
   MOV *AR3,*AR2            ; Move pSignal1(0) to Signal1(blcklen*2)
   ADD    #1,AC0
   MOV AC0,XAR2
   MOV *AR4,*AR2            ; Move pSignal2(0) to Signal1(blcklen*2+1)

   SUB #1, *(#_blcklen16)

NoRecSignal16:

   RET

_RecSignalsASM32:

   MOV dbl(*(#_pSignal1)), XAR3
   MOV dbl(*(#_pSignal2)), XAR4
   MOV dbl(*(#_blcklen32)), AC1
   MOV #-1,AC0

   CMP AC1 == AC0, TC1
   BCC NoRecSignal32,TC1

   AMOV #SignalAdrr,XAR2         ; Init XAR2 on Signal1(blcklen*2)
   MOV XAR2,AC0
   ADD AC1<<#2,AC0
   MOV AC0,XAR2
   MOV dbl(*AR3),dbl(*AR2)        ; Move pSignal1(0) to Signal1(blcklen*2)
   ADD    #2,AC0
   MOV AC0,XAR2
   MOV dbl(*AR4),dbl(*AR2)        ; Move pSignal2(0) to Signal2(blcklen*2+1)

   SUB #1,AC1
   MOV AC1,dbl(*(#_blcklen32))

NoRecSignal32:

   RET

   .end
