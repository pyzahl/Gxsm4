/*___________________________________________________________________________

	Function defined in ReadWrite_GPIO.asm
	This function reads or writes in the GPIO registers (or anywhere in the memory map)
	with long addressing mode (23-bits). Even if the small model is used.
	
	void WR_GPIO(long address, int *data, int W_R);
	
	address=this is the 23-bits address of the GPIO (or others locations)
	*data=pointer on the variable used to write or to read the GPIO
	W_R= 0: read GPIO 1: write GPIO (T0)
	
___________________________________________________________________________*/     

extern void WR_GPIO(long address, int *data, int W_R);
						
					
									
