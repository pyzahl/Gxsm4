/*___________________________________________________________________________

	Function defined in ReadWrite_GPIO.asm
	This function reads or writes in the GPIO registers (or anywhere in the memory map)
	with long addressing mode (23-bits). Even if the small model is used.
	
	void WR_GPIO(long address, int *data, int W_R);
	
	address=this is the 23-bits address of the GPIO (or others locations)
	*data=pointer on the variable used to write or to read the GPIO
	W_R= 0: read GPIO 1: write GPIO (T0)
	
___________________________________________________________________________*/     

/* MK3Pro-A810 MK2 equiv. */
#define	GPIO_Data_0  0x46000006
#define	GPIO_Dir_0   0x46000008

/* MK3-Pro Register Byte-Address */
#define Port_A_Dat 0x46000000
#define Port_A_Dir 0x46000002
#define Port_B_Dat 0x46000004
#define Port_B_Dir 0x46000006
#define Port_C_Dat 0x46000008
#define Port_C_Dir 0x4600000A
#define Port_D_Dat 0x4600000C
#define Port_D_Dir 0x4600000E

//MK2-A810
//dataprocess.h:#define	GPIO_Data_0 0x600007
//dataprocess.h:#define	GPIO_Dir_0  0x600009

extern void WR_GPIO(DSP_UINT32 address, DSP_UINT16 *data, DSP_INT32 W_R);
