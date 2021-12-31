/*
 *  Linker command file for Signal Ranger Mk3 Pro
 *

12.10.4 Resources Used By The Kernel
To function properly, the kernel uses the following resources on the DSP. After the user code is 
launched, those resources should not be used or modified, in order to avoid interfering with the 
operation of the kernel, and retain its full functionality.

** The kernel resides between byte-addresses 0x10E08000 and 0x10E08FFF in the L1PRAM of the 
   DSP. The user should avoid loading code into, or modifying memory below byte-address 
   0x10E09000 in the L1PRAM.

** The mailbox of the kernel resides between byte-addresses 0x10F04000 and 0x10F0420B in 
   L1DRAM. This section is reserved and new data cannot be loaded into this section. However, a 
   user function can use the mailbox to develop a level-3 kernel function.

** The kernel locates the interrupt vector table at byte-address 0x10E08000 inL1PRAM.
   The usercode should not relocate the interrupt vectors anywhere else.

** The interrupt vectors from address 0x10E08000 to 0x10E0809F cannot be modified. All new 
   vectors must be defined at address 0x10E080A0 and above.

** The PC (via the USB controller) uses the HPI_Int interrupt from the HPI to request an access to 
   the DSP. The event from the HPI is linked to the INT4 maskable interrupt. If necessary, the usercode 
   can temporarily disable the HPI_Int (or INT4) interrupt through its mask, or the global 
   interrupt mask (GEI by of the CSR register). During the time this interrupt is disabled, all PC 
   access requests are latched, but are held until the interrupt is re-enabled. Once the interrupt is reenabled, 
   the access request resumes normally.

** The kernel initializes the stack at byte-address 0x10F17FF8. The kernel uses less than 128 bytes 
   of stack (between addresses 0x10F17F78 and address 0x10F17FF8). The user code can 
   relocate the stack pointer temporarily, but should replace it before the last return to the kernel (if 
   this last return is intended). Examples where a last return to the kernel is not intended include 
   situations where the user code is a never-ending loop that will be terminated by a board reset or a 
   power shutdown. In these cases, the stack can be relocated freely without concern.

 *
 */

MEMORY
{
	L1PRAM_Vector (RX):	o = 0x10E080A0  l = 0x000160
	L1PRAM_Kernel (RX):	o = 0x10E08200  l = 0xE00
	L1PRAM 	(RX):		o = 0x10E09000  l = 0x7000
	L1DRAM_MailBox (RWI):	o = 0x10F04000	l = 0x20C
/*	L1DRAM	(RWI):		o = 0x10F0420C	l = 0x13DF4 
                                       ..18000
*/
	L1DRAM	(RWI):		o = 0x10F0420C	l = 0x0FCF4
	L1DRAM_M (RWI):		o = 0x10F13F00	l = 0x00100
	L1DRAM_V (RWI):		o = 0x10F14000	l = 0x01000
	L1DRAM_F (RWI):		o = 0x10F15000	l = 0x02f00
	L2RAM (RWXI):      	o = 0x10800000  l = 0x00020000
	FLASH (R): 		o = 0x42000000	l = 0x02000000
	ETHER (RW): 		o = 0x44000000	l = 0x02000000
	FPGA (RW): 		o = 0x46000000	l = 0x02000000
/*	DDR2 (RWI):       	o = 0x80000000  l = 0x8000000 */
	DDR2 (RWI):       	o = 0x80000000  l = 0x7000000
	DDR2M (RWX ):		o = 0x87000000  l = 0x0001000
	DDR2V (RWX ):		o = 0x87001000  l = 0x001F000
	DDR2F (RWX ):		o = 0x87100000  l = 0x0F00000
}

SECTIONS
{
    .bss        >   L1DRAM
    .cinit      >   L1DRAM
    .cio        >   L1DRAM
    .const      >   L1DRAM
    .data       >   L1DRAM
    .far        >   L1DRAM
    .stack      >   L1DRAM
    .switch     >   L1DRAM
    .sysmem     >   L1DRAM

    .vectors       >   L1PRAM_Vector
    .text          >   L1PRAM
    .text:slow     >   L2RAM
    .text:slow_PLL >   L2RAM
    .text:fast_PLL >   L1PRAM
 
    MAGIC       >   L1DRAM_M
    SMAGIC      >   DDR2
    PRBVH       >   L1DRAM_V
    PRBDF       >   L1DRAM_F
    FIFO_DATA   >   L2RAM
    PLLBuffers  >   DDR2
}
