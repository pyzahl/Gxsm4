/* COMMAND FILE TO LINK GXSM FB_SPM PROGRAM */
-l sr2_analog810driver.lib
-l rts55.lib

/* -stack=0x200 */

MEMORY
{
PAGE 0:
      RESERVED    (R   ): origin= 0x000000 length= 0x000100 /* unavailable for allocation */
      vectors     (RWX ): origin= 0x000100 length= 0x0000FC /* interrupt vectors */
      VTRAPKERN   (R   ): origin= 0x0001FC length= 0x000004 /* Trap 31 vector -- keep fixed */

      /* Mail box - do not place any code in it to keep the kernel happy */
      MailBox     (RW  ): origin= 0x000200 length= 0x00020F

      /* Kernel - do not place code or data on top of the kernel */
      RESERVED_2  (R   ): origin= 0x000410 length= 0x0003F0

      /* On-chip Dual Access RAM minus the mail box */
      DARAM_NA    (RWX) : origin= 0x000800 length= 0x0000D0
      DARAM0      (RWX) : origin= 0x0008D0 length= 0x009730
/*    DARAM0      (RWX ): origin= 0x000800 length= 0x009800 */
      DARAM1M     (RWX ): origin= 0x00A000 length= 0x000200
      DARAM1V     (RWX ): origin= 0x00A200 length= 0x000E00
      DARAM1F     (RWX ): origin= 0x00B000 length= 0x005000
/* Why the heck???
      DARAM0      (RWX ): origin= 0x000800 length= 0x001800
      DARAM1      (RWX ): origin= 0x002000 length= 0x002000
      DARAM2      (RWX ): origin= 0x004000 length= 0x002000
      DARAM3      (RWX ): origin= 0x006000 length= 0x002000
      DARAM4      (RWX ): origin= 0x008000 length= 0x002000
      DARAM5      (RWX ): origin= 0x00A000 length= 0x002000
      DARAM6      (RWX ): origin= 0x00C000 length= 0x002000
      DARAM7      (RWX ): origin= 0x00E000 length= 0x002000
*/
      /* A part of off-chip Synchronous Dynamic RAM in CE0 */
      SDRAM       (RWX ): origin= 0x010000 length= 0x3F0000

      FLASH       (RWX ): origin= 0x800000 length= 0x200000 /* off-chip FLASH */
      FPGA        (RW  ): origin= 0xC00000 length= 0x400000 /* FPGA Mode MPNMC=1 */

PAGE 2:
      /* SPACE IO_space PAGE 2 */
      PERIPHERAL  (RW  ): origin= 0x000000 length= 0x00A020 /* Peripheral register */
      IOSPACE_FREE(RW  ): origin= 0x00A021 length= 0x0050CF /* Peripheral register */
}


SECTIONS 
{
	.vectors        > vectors
	.pinit		> DARAM0
	.cinit		> DARAM0
	.text		> DARAM0
        .NAFunct :     {record.obj (.text)} load = DARAM_NA
	MAGIC		> DARAM1M
	PRBVH		> DARAM1V
	PRBDF		> DARAM1F
	dmabuf		> DARAM0
	RecBuffers      > SDRAM
 	LPOLYCOEF	> DARAM0
	Unused_ISR      > DARAM0
	.const          > DARAM0
	.data		> DARAM0
	.bss		> DARAM0
}
