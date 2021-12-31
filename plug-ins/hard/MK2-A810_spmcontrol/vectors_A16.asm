
        .global _Reset
	.global ISRKernel
	.global _RESET_K
	.global ISRDMA
	.global	_dataprocess
	
_RESET_K 	.set    0494h		; Do not modify
ISRKernel  	.set    0410h		; Do not modify

	; Enable Interrupt Registers: IER0, IER1 (mode DSP not-halted) DBER0, DBER1 (mode DSP halted)
	; Interrupt flag register: IFR0,IFR1
	
	; ******IER0,DBER0, IFR0
	; BIT#0 : Reserved		BIT#1 : Reserved 	BIT#2 : INT0		BIT#3 : INT2
	; BIT#4 : TINT0			BIT#5 : RINT0	 	BIT#6 : RINT1		BIT#7 : XINT1
	; BIT#8 : Reserved		BIT#9 : DMAC1	 	BIT#10: DSPINT		BIT#11: INT3/WDTINT
	; BIT#12: RINT2/UART	BIT#13: XINT2	 	BIT#14: DMAC4		BIT#15: DMAC5
	
	; ******IER1,DBER1, IFR1
	; BIT#0 : INT1			BIT#1 : XINT0	 	BIT#2 : DMAC0		BIT#3 : INT4
	; BIT#4 : DMAC2			BIT#5 : DMAC3	 	BIT#6 : TINT1		BIT#7 : I2C
	; BIT#8 : BERR			BIT#9 : DLOG	 	BIT#10: RTOS		BIT#11: Reserved
	; BIT#12: Reserved		BIT#13: Reserved	BIT#14: Reserved	BIT#15: Reserved
	
	; ******* Vector table (32-bit stack (default))
	; ******* All other stack mode are not compatible with a 54x code
	; ******* RETA and CFCT registers are used in fast return mode
	; ******* C54X_STK: 32-bit stack (default)
	; ******* USE_RETA: 16-bit stack using RETA	; 
	; ******* NO_RETA: 16-bit stack with out RETA
	
	; *******  .sect 	"vectors"
			.sect 	.vectors

	; Do not modify the Stack Mode and the ResetVect interrupt vector 
	
_Reset	.ivec _RESET_K,USE_RETA		; 0: RESET (Software 0: SINT0)				; Reset (hardware and software)
			.ivec _UnusedIsr			; 1: NMI (Software 1: SINT1)				; Nonmaskable interrupt
			.ivec _UnusedIsr			; 2: INT0 (Software 2: SINT2)				; External interrupt #0
			.ivec _UnusedIsr			; 3: INT2 (Software 3: SINT3)				; External interrupt #2
			.ivec _UnusedIsr			; 4: TINT0 (Software 4: SINT4)				; Timer #0 interrupt
			.ivec _UnusedIsr			; 5: RINT0 (Software 5: SINT5)				; McBSP #0 receive interrupt
			.ivec _UnusedIsr			; 6: RINT1 (Software 6: SINT6)				; McBSP #1 receive interrupt
			.ivec _UnusedIsr			; 7: XINT1 (Software 7: SINT7)				; McBSP #1 transmit interrupt
			.ivec _UnusedIsr			; 8: LCKINT (Software 8: SINT8)				; PLL lock interrupt
			.ivec _UnusedIsr			; 9: DMAC1 (Software 9: SINT9)				; DMA Channel #1 interrupt
			
	; Do not modify the DSPINT interrupt vector
	
DSPINT		.ivec ISRKernel				; 10: DSPINT (Software 10: SINT10)			; Interrupt from host
			.ivec _UnusedIsr			; 11: INT3/WDT INT (Software 11: SINT11)	; External interrupt #3 or Watchdog timer interrupt
			.ivec _UnusedIsr			; 12: RINT2 (Software 12: SINT12)			; McBSP #2 receive interrupt
			.ivec _UnusedIsr			; 13: XINT2 (Software 13: SINT13)			; McBSP #2 transmit interrupt
			.ivec _UnusedIsr			; 14: DMAC4 (Software 14: SINT14)			; DMA Channel #4 interrupt
			.ivec _UnusedIsr			; 15: DMAC5 (Software 15: SINT15)			; DMA Channel #5 interrupt
			.ivec _UnusedIsr			; 16: INT1 (Software 16: SINT16)			; External interrupt #1
			.ivec _UnusedIsr			; 17: XINT0 (Software 17: SINT17)			; McBSP #0 transmit interrupt
DMAINT		.ivec ISRDMA				; 18: DMAC0 (Software 18: SINT18)			; DMA Channel #0 interrupt
			.ivec _UnusedIsr			; 19: --- (Software 19: SINT19)				; Software interrupt #19
			.ivec _UnusedIsr			; 20: DMAC2 (Software 20: SINT20)			; DMA Channel #2 interrupt			
			.ivec _UnusedIsr			; 21: DMAC3 (Software 21: SINT21)			; DMA Channel #3 interrupt
			.ivec _UnusedIsr			; 22: TINT1 (Software 22: SINT22)			; Timer #1 interrupt
			.ivec _UnusedIsr			; 23: IIC (Software 23: SINT23)				; I2C interrupt
			.ivec _UnusedIsr			; 24: BERR (Software 24: SINT24)			; Bus Error interrupt
			.ivec _UnusedIsr			; 25: DLOG (Software 25: SINT25)			; Data Log interrupt
			.ivec _UnusedIsr			; 26: RTOS (Software 26: SINT26)			; Real-time Operating System interrupt
			.ivec _UnusedIsr			; 27: Software int#27 (Software 27: SINT27)	; Software interrupt #27
			.ivec _UnusedIsr			; 28: Software int#28 (Software 28: SINT28)	; Software interrupt #28
			.ivec _UnusedIsr			; 29: Software int#29 (Software 29: SINT29)	; Software interrupt #29
TRAPData	.ivec _dataprocess			; 30: Software int#30 (Software 30: SINT30)	; Software interrupt #30

	; This vector must be keep commented 
		
;TRAPKern	.ivec ReadMem				; 31: Software int#31 (Software 31: SINT31)	; Software interrupt #31		
	
			.sect 	"Unused_ISR"
	
	.global _UnusedIsr					; Default entry for unused interrupt
_UnusedIsr  RETI
			
	.end
	