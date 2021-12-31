/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 8 c-style: "K&R" -*- */

/*___________________________________________________________________________

	Function defined in ReadWrite_GPIO.c
	address is the 32-bits address of the GPIO (or others locations)
	*data=pointer on the variable used to write or to read the GPIO
	W_R= 0: read GPIO 1: write GPIO (T0)
	
___________________________________________________________________________*/     

#include "FB_spm_dataexchange.h"
#include "ReadWrite_GPIO.h"

#ifndef DSPEMU
#pragma CODE_SECTION(WR_GPIO, ".text:slow")
void WR_GPIO(DSP_UINT32 address, DSP_UINT16 *data, DSP_INT32 W_R){
	if (W_R)
		*(volatile unsigned short*)(address) = *data;
	else
		*data = *(volatile unsigned short*)(address);
}
#else
void WR_GPIO(DSP_UINT32 address, DSP_UINT16 *data, DSP_INT32 W_R){
 	if (W_R)
                switch (address){
                case GPIO_Data_0: DSP_MEMORY(GPIO_Data[0]) = *data; break;
                case GPIO_Dir_0: DSP_MEMORY(GPIO_DataDir[0]) = *data; break;
                }
	else
                switch (address){
                case GPIO_Data_0: *data = DSP_MEMORY(GPIO_Data[0]); break;
                case GPIO_Dir_0: *data = DSP_MEMORY(GPIO_DataDir[0]); break;
                }
}
#endif


					
									
