;***************************************************************************
; Reset subroutine (ISR) : complied when the kernel is loaded
;
; xxxx x000h RESET ISFP
; xxxx x020 NMI ISFP
; xxxx x040 Reserved
; xxxx x060 Reserved
; xxxx x080 INT4 ISFP (HPIINT : already define in the kernel)
; xxxx x0A0 INT5 ISFP (EDMA ch6 and ch34)
; xxxx x0C0 INT6 ISFP
; xxxx x0E0 INT7 ISFP
; xxxx x100 INT8 ISFP
; xxxx x120 INT9 ISFP
; xxxx x140 INT10 ISFP
; xxxx x160 INT11 ISFP
; xxxx x180 INT12 ISFP
; xxxx x1A0 INT13 ISFP
; xxxx x1C0 INT14 ISFP
; xxxx x1E0 INT15 ISFP

;***************************************************************************

	.global _SR3A810ISR
	.global _McBSP1TX_INT
	.global _McBSP1RX_INT

	.sect 	.vectors

	.nocmp
	
	; INT5
	
_VectSRAIC810:							

	STW		.D2T2	B10,*B15--[2]		; An absolute branch (or call) is used to avoid
 || MVKL	.S2     _SR3A810ISR,B10		; far trampoline sections when the ISR is far (more
	MVKH	.S2     _SR3A810ISR,B10		; than 21-bits address from the current PC)
	B		.S2     B10					; Note that the B15 (software stack) is necessary
    LDW		.D2T2   *++B15[2],B10
	NOP     4
	NOP
	NOP
	
	;INT6
	
_VectINT6:

	NOP 
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP	

	;INT7
	

;INT7 (MBXINT1)   -- McBSP (SPI Master, Transmit TX)
_VectINT7: 

	STW .D2T2 B10,*B15--[2] ; An absolute branch (or call) is used to avoid
	|| MVKL .S2     _McBSP1TX_INT,B10 ; far trampoline sections when the ISR is far (more
	MVKH .S2     _McBSP1TX_INT,B10 ; than 21-bits address from the current PC)
	B .S2     B10 ; Note that the B15 (software stack) is necessary
	LDW .D2T2   *++B15[2],B10
	NOP     4
	NOP
	NOP

;INT8 (MBRINT1)   -- McBSP (SPI Master, Read RX)
_VectINT8: 

	STW .D2T2 B10,*B15--[2] ; An absolute branch (or call) is used to avoid
	|| MVKL .S2     _McBSP1RX_INT,B10 ; far trampoline sections when the ISR is far (more
	MVKH .S2     _McBSP1RX_INT,B10 ; than 21-bits address from the current PC)
	B .S2     B10 ; Note that the B15 (software stack) is necessary
	LDW .D2T2   *++B15[2],B10
	NOP     4
	NOP
	NOP

	
	;INT9
	
_VectINT9:

	NOP 
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP	
	
	;INT10
	
_VectINT10:

	NOP 
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP	

	;INT11
	
_VectINT11:

	NOP 
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP		

	;INT12
	
_VectINT12:

	NOP 
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP	

	;INT13
	
_VectINT13:

	NOP 
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP	
	
	; INT14
	
_VectINT14:

	NOP 
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP		

	;INT15
	
_VectINT15:

	NOP 
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP	


	.end
