/*___________________________________________________________________________

	Function defined in ReadWrite_GPIO.asm
	This function reads or writes in the GPIO registers (or anywhere in the memory map)
	with long addressing mode (23-bits). Even if the small model is used.
	
	void WR_GPIO(long address, int *data, int W_R);
	
	address=this is the 23-bits address of the GPIO (or others locations)
	*data=pointer on the variable used to write or to read the GPIO
	W_R= 0: read GPIO 1: write GPIO (T0)
	
___________________________________________________________________________*/     

// A810 GPIO (on back of controller bits 0...15 via address GPIO_Data_0
extern void WR_GPIO(int address, unsigned short *data, int W_R);

#define WRITE_GPIO_0(X) *(volatile unsigned short*)(GPIO_Data_0) = (X)
#define READ_GPIO_0()  (*(volatile unsigned short*)(GPIO_Data_0))

// DSP GPIO (internal MK3-Pro, JP7):
// GPIO_SET_DATA23 or GPIO_CLR_DATA23 register to clear or to set the GP53,54,55:
// E = 1110 = 2+4+8, 55: 8=1000, 54: 4=0100, 53: 2=0010 

#if 0
#define SET_DSP_GP53 GPIO_SET_DATA23=0x00200000
#define SET_DSP_GP54 GPIO_SET_DATA23=0x00400000
#define SET_DSP_GP55 GPIO_SET_DATA23=0x00800000

#define CLR_DSP_GP53 GPIO_CLR_DATA23=0x00200000
#define CLR_DSP_GP54 GPIO_CLR_DATA23=0x00400000
#define CLR_DSP_GP55 GPIO_CLR_DATA23=0x00800000

#else

#define SET_DSP_GP53
#define SET_DSP_GP54 
#define SET_DSP_GP55

#define CLR_DSP_GP53
#define CLR_DSP_GP54
#define CLR_DSP_GP55

#endif
