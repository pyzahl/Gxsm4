;____________________________________________________________________________
;
;       -This is an assembly code for the Log function
;
;       October 2011
;
;		Alex Boudreau
;____________________________________________________________________________

    .width   	80
    .length  	55
    .title 		"ASM code for the Log on SR3Pro"

	.asg	B15, SP
	.text
	.align 32
	.global	_LogFunctionASM
	
;******************************************************************************
;* This function must be used with the C context                              *
;* int LogFunctionASM(int x, int *Coeff_Log);                                 *
;* Y=0.14443*Log10(X)                                                         *
;* X input Q23 (between 1 and 2^23-1)                                         *
;* Y output Q23 (between -8388603 and zero)                                   *	
;*                                                                            *	
;* FUNCTION NAME: LogFunctionASM                                              *
;*                                                                            *
;*   Regs Modified     : A0,A1,A2,A3,A4,A5,A6,A7,A8,A9,A10,B0,B1,B2,B3,B4,B5, *
;*                           B6,B7,B8,B9,B10,SP,A16,A17,A18,A19,A20,A21,A22,  *
;*                           A23,A24,A25,A26,A27,A28,A29,A30,A31,B16,B17,B18, *
;*                           B19,B20,B21,B22,B23,B24,B25,B26,B27,B28,B29,B30, *
;*                           B31                                              *
;*   Regs Used         : A0,A1,A2,A3,A4,A5,A6,A7,A8,A9,A10,B0,B1,B2,B3,B4,B5, *
;*                           B6,B7,B8,B9,B10,SP,A16,A17,A18,A19,A20,A21,A22,  *
;*                           A23,A24,A25,A26,A27,A28,A29,A30,A31,B16,B17,B18, *
;*                           B19,B20,B21,B22,B23,B24,B25,B26,B27,B28,B29,B30, *
;*                           B31                                              *
;*   Local Frame Size  : 0 Args + 0 Auto + 24 Save = 24 byte                  *
;******************************************************************************

_LogFunctionASM:
           
           ; Save Context
           
           STW     .D2T2   B10,*SP--(8)      
           STW     .D2T2   B3,*SP--(8)        
           STW     .D2T1   A10,*SP--(8)       

           MV      .L2     B4,B10             
||         MV      .L1     A4,A10            

           ; Test for X leq to zero	 

           CMPGT   .L1     A10,0,A0          
   [!A0]   BNOP    .S1     XLeqZeroLog,4           
|| [!A0]   ZERO    .L1     A4
   [!A0]   MVKH    .S1     0xff800000,A4
 
           ; Compute Exp and Mant on X (A10)
 
           MVK     .S1 0x0001,A0              
           LMBD    .L1 A0, A10, A4	

           ; Log computation 

           SUB     .L1     A4,9,A18          ; |100| 
           LDNDW   .D2T1   *B10,A5:A4        ; |108| 
           LDNDW   .D2T2   *+B10(8),B5:B4    ; |112| 
           ZERO    .L1     A6
           SET     .S1     A6,0x16,0x16,A6
        
        
           SHL     .S1     A10,A18,A3        ; |116| 
           SUB     .L1     A3,A6,A3          ; |116| 
           SHR     .S1     A3,31,A19         ; |116| 
           MPY32SU .M1     A19,A3,A9:A8      ; |116| 
           MPY32SU .M1     A19,A3,A17:A16    ; |116| 
           MPY32U  .M1     A3,A3,A7:A6       ; |116| 
           MV      .L1X    B4,A30            ; |116| 
           SHR     .S2     B4,31,B31         ; |116| 
           ADD     .L1     A16,A8,A8         ; |116| 
           ADD     .L1     A7,A8,A7          ; |116| 
           SHL     .S1     A7,0x9,A8         ; |116| 

           SHR     .S1     A7,0x17,A20       ; |116| 
||         SHRU    .S2X    A6,0x17,B6        ; |116| 

           OR      .L2X    A8,B6,B17         ; |116| 
||         MPY32SU .M1     A20,A3,A7:A6      ; |116| 

           MPY32SU .M2X    A19,B17,B9:B8     ; |116| 
           MPY32U  .M2X    B17,A3,B7:B6      ; |116| 
           MV      .L2X    A4,B27            ; |116| 
           SHR     .S2X    A4,31,B29         ; |116| 
           ADD     .L2X    A6,B8,B8          ; |116| 
           ADD     .L2     B7,B8,B7          ; |116| 

           SHRU    .S1X    B6,0x17,A6        ; |116| 
||         SHR     .S2     B7,0x17,B20       ; |116| 

           SHL     .S2     B7,0x9,B6         ; |116| 
           OR      .L2X    B6,A6,B16         ; |116| 

           MPY32SU .M1X    B20,A3,A9:A8      ; |116| 
||         MPY32U  .M2X    B16,A3,B9:B8      ; |116| 

           MPY32SU .M2X    A19,B16,B7:B6     ; |116| 
           MPY32SU .M1X    B20,A30,A25:A24   ; |116| 
           SHR     .S2     B5,31,B1          ; |116| 
           MPY32SU .M1X    B29,A3,A27:A26    ; |116| 

           MV      .L1X    B8,A6             ; |116| 
||         ADD     .L2X    A8,B6,B6          ; |116| 

           SHRU    .S1     A6,0x17,A6        ; |116| 
||         ADD     .L2     B9,B6,B6          ; |116| 

           SHL     .S2     B6,0x9,B7         ; |116| 

           OR      .L2X    B7,A6,B26         ; |116| 
||         SHR     .S2     B6,0x17,B21       ; |116| 

           MPY32SU .M2X    A19,B26,B7:B6     ; |116| 
           MPY32SU .M1X    B21,A3,A9:A8      ; |116| 
           MPY32U  .M1X    B26,A3,A7:A6      ; |116| 
           LDNDW   .D2T2   *+B10(16),B9:B8   ; |116| 
           MPY32U  .M2     B26,B5,B25:B24    ; |116| 
           ADD     .L1X    A8,B6,A8          ; |116| 
           ADD     .L1     A7,A8,A7          ; |116| 
           SHL     .S1     A7,0x9,A8         ; |116| 

           SHRU    .S2X    A6,0x17,B6        ; |116| 
||         SHR     .S1     A7,0x17,A6        ; |116| 

           OR      .L2X    A8,B6,B6          ; |116| 
||         MPY32SU .M1     A6,A3,A17:A16     ; |116| 

           MPY32SU .M2X    A19,B6,B19:B18    ; |116| 
           MPY32U  .M2X    B6,A3,B23:B22     ; |116| 
           MV      .L1X    B8,A31            ; |116| 
           SHR     .S2     B8,31,B30         ; |116| 
           ADD     .L2X    A16,B18,B7        ; |116| 
           ADD     .L2     B23,B7,B28        ; |116| 
           SHL     .S2     B28,0x9,B7        ; |116| 

           SHRU    .S1X    B22,0x17,A9       ; |116| 
||         MPY32SU .M2     B31,B16,B19:B18   ; |116| 

           OR      .L1X    B7,A9,A21         ; |116| 
||         MPY32SU .M1     A6,A31,A9:A8      ; |116| 

           MPY32U  .M1X    B6,A31,A23:A22    ; |116| 
||         MPY32SU .M2     B30,B6,B7:B6      ; |116| 

           MPY32SU .M2X    A19,B27,B23:B22   ; |116| 

           MPY32U  .M1X    A21,B9,A17:A16    ; |116| 
||         SHR     .S2     B9,31,B2          ; |116| 
||         MPY32SU .M2     B21,B5,B21:B20    ; |116| 
||         ADD     .L2X    A24,B18,B18       ; |116| 

           SHR     .S2     B28,0x17,B8       ; |116| 
||         MPY32SU .M2X    B2,A21,B5:B4      ; |116| 

           MPY32SU .M2     B8,B9,B9:B8       ; |116| 
||         MPY32U  .M1X    B16,A30,A7:A6     ; |116| 
||         ADD     .L2X    A8,B6,B16         ; |116| 

           SHR     .S1     A5,31,A29         ; |116| 
||         MPY32U  .M1X    B17,A5,A25:A24    ; |116| 
||         MPY32U  .M2X    A3,B27,B7:B6      ; |116| 

           ADD     .L2X    A23,B16,B0        ; |116| 
||         ADDU    .L1     A22,A16,A9:A8     ; |116| 
||         MPY32SU .M1X    A29,B17,A23:A22   ; |116| 
||         MPY32SU .M2     B1,B26,B17:B16    ; |116| 

           MPY32SU .M1     A20,A5,A5:A4      ; |116| 
           ADD     .L2     B8,B4,B4          ; |116| 

           ADDU    .L2X    B24,A8,B9:B8      ; |116| 
||         ADD     .L1X    B22,A26,A28       ; |116| 

           ADD     .L2X    A17,B4,B16        ; |116| 
||         ADD     .S2     B20,B16,B4        ; |116| 
||         ADD     .L1X    A7,B18,A7         ; |116| 

           MV      .L2     B6,B30            ; |116| 
||         ADD     .S2     B25,B4,B6         ; |116| 
||         MV      .D2     B8,B31            ; |116| 
||         ADD     .L1X    B7,A28,A3         ; |116| 

           ADD     .S2     B9,B6,B7          ; |116| 
||         ADDU    .L2X    A6,B31,B5:B4      ; |116| 
||         ADD     .L1     A4,A22,A4         ; |116| 
||         ADD     .S1X    A9,B0,A5          ; |116| 

           ADD     .L1X    A5,B16,A5         ; |116| 
||         ADD     .L2X    B5,A7,B6          ; |116| 
||         ADD     .S1     A25,A4,A4         ; |116| 

           ADD     .L1X    B7,A5,A5          ; |116| 
||         ADDU    .L2X    A24,B4,B5:B4      ; |116| 

           ADD     .L2X    B5,A4,B6          ; |116| 
||         ADD     .L1X    B6,A5,A4          ; |116| 

           ADDU    .L2     B30,B4,B5:B4      ; |116| 
           ADD     .L1X    B6,A4,A4          ; |116| 
           ADD     .L1X    B5,A3,A3          ; |116| 
           ADD     .L1     A3,A4,A3          ; |116| 

           LDNDW   .D2T2   *+B10(24),B7:B6   ; |120| 
           NOP             3

           ADD     .L2X    1,A18,B5          ; |124| 
           MPY32   .M2     B5,B7,B5          ; |124| 
           SHRU    .S2     B4,0x17,B4        ; |124| 
           SHL     .S1     A3,0x9,A3         ; |124| 
           OR      .L1X    A3,B4,A3          ; |124| 
           NOP             1
           SUB     .L1X    A3,B5,A3          ; |124| 
           MPY32   .M1X    A3,B6,A5:A4       ; |124| 
           NOP             4
           SHL     .S2X    A5,0x9,B4         ; |124| 
           SHRU    .S1     A4,0x17,A3        ; |124| 
           OR      .L1X    B4,A3,A3          ; |124| 

           CMPGT   .L1     A3,-7,A0          ; |124| 
||         ZERO    .S1     A4                ; |124| 

   [!A0]   ADD     .L1     7,A3,A4           ; |124| 

XLeqZeroLog:    
           LDW     .D2T1   *++SP(8),A10      ; |128| 
           LDW     .D2T2   *++SP(8),B3
           LDW     .D2T2   *++SP(8),B10      ; |128| 
           NOP             3
           RETNOP  .S2     B3,5
	
	.end

