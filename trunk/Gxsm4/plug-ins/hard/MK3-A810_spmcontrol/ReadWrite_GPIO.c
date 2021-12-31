/*___________________________________________________________________________

	Function defined in ReadWrite_GPIO.c
	address is the 32-bits address of the GPIO (or others locations)
	*data=pointer on the variable used to write or to read the GPIO
	W_R= 0: read GPIO 1: write GPIO (T0)
	
___________________________________________________________________________*/     


#pragma CODE_SECTION(WR_GPIO, ".text:slow")
void WR_GPIO(int address, unsigned short *data, int W_R){
	if (W_R)
		*(volatile unsigned short*)(address) = *data;
	else
		*data = *(volatile unsigned short*)(address);
}
