;____________________________________________________________________________
;
;	Assembler code for read write GPIO
;____________________________________________________________________________

        .width   80
        .length  55
        .title "Assembler code read write GPIO"
        .mmregs
        
;____________________________________________________________________________
;
;       _WR_GPIO Assembler code
;  		void WR_GPIO(long address, int *data, int W_R);
; 		address=this is the 32-bits address of the GPIO (or other location) in AC0
;		*data=pointer on the variable to write or to read (AR0)
;		W_R= 0: read 1: write (T0)
;____________________________________________________________________________

		.text
		
		.even

		.global	_WR_GPIO

_WR_GPIO:

	PSHBOTH		XAR1
	
	; Load XAR1 with Address
	
	MOV 	AC0,XAR1 ; Address in XAR1

	; Test W_R

	BCC 	Write_GPIO, T0 == #1

Read_GPIO:

	MOV 	*AR1,*AR0   	; data=*(int*)address

	B 		End_WR_GPIO

Write_GPIO:

	MOV 	*AR0,*AR1   	; *(int*)address=data

End_WR_GPIO:

    POPBOTH		XAR1

	RET

	.end
																																																														
		
		
		
