;____________________________________________________________________________
;
;       -This is an assembly code for the SR3 timer functions
;
;       March 2010
;
;	Alex Boudreau
;____________________________________________________________________________

        .width   	80
        .length  	55
        .title 		"ASM code for timer SR3"
;***************************************************************************
; 	DSP Codes
;***************************************************************************
	
		.text

		.align 32		
		
;***************************************************************************
; wait function for SR3 (delay in A4) : 7 cycles by delay value
;***************************************************************************

        .global _wait   
            	
_wait:

	 MV      .L1     A4,A0              
	
waitLoop_1:
           ADD    .S1     0xffffffff,A0,A0     
   [ A0]   BNOP    .S1     waitLoop_1,5      

	; Return

	B	.S2		B3
	NOP			5

;***************************************************************************
;
; 	Declaration:
;
;	TimerTemp0, TimerTemp1, MeasuredTime, MaxTime, MinTime;
;
;***************************************************************************

	.global _TimerTemp
	.global _MeasuredTime
	.global _MaxTime
	.global _MinTime

	.bss _TimerTemp,4,4
	.bss _MeasuredTime,4,4
	.bss _MaxTime,4,4
	.bss _MinTime,4,4

	.text

;***************************************************************************
; Read TSC counter the first time and update _TimerTemp
;***************************************************************************
	.align 32			

        .global _TSCReadFirst   
            	
_TSCReadFirst:
 	
	; Return (the ints are disable during the branch delay slot)

	B	 .S2	B3
 	MVC  .S2 	TSCL,B0 		; Read the low half of TSC Counter
 	MVKL .S1	_TimerTemp,A0
	MVKH .S1	_TimerTemp,A0
	STW	 .D1	B0,*A0
	NOP
	
;***************************************************************************
; Read TSC counter the second time and update _MeasuredTime _MaxTime _MinTime
;***************************************************************************
	.align 32			; necessary?

        .global _TSCReadSecond   
            	
_TSCReadSecond:
 	

 		MVKL .S1	_TimerTemp,A0
		MVKH .S1	_TimerTemp,A0
		LDW	 .D1	*A0++,B1		; Load last timer in B1
	 	MVC  .S2 	TSCL,B0 		; Read the low half of TSC counter
		NOP  3
		SUB  .L2	B0,B1,B1		; Current Difference in B1
		MVK	 .S2	14,B0	
		SUB	 .L2	B1,B0,B1		; Subtract 14 (Overhead for _TSCReadFirst and _TSCReadSecond)
	 	STW	 .D1	B1,*A0++		; **** Store _MeasuredTime
		LDW	 .D1	*A0,B0			; last _MaxTime in B0		
		NOP 4
		CMPGTU	.L2	B1,B0,B2		; If currentdiff > Maxtime B2=1 
 [B2]   MV	 .S2	B1,B0
 		STW	 .D1	B0,*A0++		; **** Store _MaxTime
		LDW	 .D1	*A0,B0			; last _MinTime in B0	
				
		B	.S2		B3				; Return
		NOP 	3
		CMPLTU	.L2 B1,B0,B2		; If currentdiff < Mintime B2=1
 [B2]   STW	 .D1	B1,*A0			; **** Store _MinTime
 [!B2]  STW	 .D1	B0,*A0			; **** Store _MinTime
		  		
;***************************************************************************
; Init variable _TimerTemp _MeasuredTime _MaxTime _MinTime
;***************************************************************************
	.align 32			; necessary?

        .global _TSCInit   
            	
_TSCInit:

 	MVKL .S1	_TimerTemp,A0
	MVKH .S1	_TimerTemp,A0
 	MVK  .S1	0x0000,A1

 ||	B	.S2		B3
 	MVKL .S1	0xFFFFFFFF,A2
	MVKH .S1	0xFFFFFFFF,A2
 || STW	 .D1	A1,*A0++		; Init TimerTemp
	STW	 .D1	A1,*A0++		; MeasuredTime
	STW	 .D1	A1,*A0++		; MasTime
	STW	 .D1	A2,*A0			; MinTime

	.end

