/*___________________________________________________________________________

	Function defined in mul32.asm -- from Dsplib

;***********************************************************
 * ; Version 2.40.00                                           
 * ;***********************************************************
 * ; Function:    mul32
 * ; Processor:   C55xx
 * ; Description: Implements a vector add using a single-MAC 
 * ;              approach.  This routine is C-callable.
 * ;
 * ; Usage: ushort oflag = mul32 (LDATA *x,
 * ;                              LDATA *y,
 * ;                              LDATA *r,
 * ;                              ushort nx)
 * ;
 * ;****************************************************************
___________________________________________________________________________*/     

extern unsigned int mul32(int *x, int *y,  int *r, unsigned short nx);
